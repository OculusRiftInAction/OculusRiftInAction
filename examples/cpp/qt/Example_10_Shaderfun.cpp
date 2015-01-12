#ifdef _DEBUG
#define BRAD_DEBUG 1
#endif

#include "Common.h"
#include <QtOpenGL/QGLContext>
#include <QtOpenGL/QGLFunctions>
#include <QDomDocument>
#include <QXmlQuery>
#include <QQuickTextDocument>
#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#else
#include <oglplus/images/png.hpp>
#endif

#include "Shadertoy.h"

const char * ORG_NAME = "Oculus Rift in Action";
const char * ORG_DOMAIN = "oculusriftinaction.com";
const char * APP_NAME = "ShadertoyVR";

using namespace oglplus;

#define UI_X 1280
#define UI_Y 720

static const float ROOT_2 = sqrt(2.0f);
static const float INV_ROOT_2 = 1.0f / ROOT_2;
static uvec2 UI_SIZE(UI_X, UI_Y);
static float UI_ASPECT = aspect(vec2(UI_SIZE));
static float UI_INVERSE_ASPECT = 1.0f / UI_ASPECT;

typedef std::shared_ptr<oglplus::VertexShader> VertexShaderPtr;
typedef std::shared_ptr<oglplus::FragmentShader> FragmentShaderPtr;

static ImagePtr loadImageWithAlpha(const std::vector<uint8_t> & data, bool flip) {
  using namespace oglplus;
#ifdef HAVE_OPENCV
  cv::Mat image = cv::imdecode(data, cv::IMREAD_UNCHANGED);
  if (flip) {
    cv::flip(image, image, 0);
  }
  ImagePtr result(new images::Image(image.cols, image.rows, 1, 4, image.data,
    PixelDataFormat::BGRA, PixelDataInternalFormat::RGBA8));
  return result;
#else
  std::stringstream stream(std::string((const char*)&data[0], data.size()));
  return ImagePtr(new images::PNGImage(stream));
#endif
}

static TexturePtr loadCursor(Resource res) {
  using namespace oglplus;
  TexturePtr texture(new Texture());
  Context::Bound(TextureTarget::_2D, *texture)
    .MagFilter(TextureMagFilter::Linear)
    .MinFilter(TextureMinFilter::Linear);

  ImagePtr image = loadImageWithAlpha(Platform::getResourceByteVector(res), true);
  // FIXME detect alignment properly, test on both OpenCV and LibPNG
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  Texture::Storage2D(TextureTarget::_2D, 1, PixelDataInternalFormat::RGBA8, image->Width() * 2, image->Height() * 2);
  {
    size_t size = image->Width() * 2 * image->Height() * 2 * 4;
    uint8_t * empty = new uint8_t[size]; memset(empty, 0, size);
    images::Image blank(image->Width() * 2, image->Height() * 2, 1, 4, empty);
    Texture::SubImage2D(TextureTarget::_2D, blank, 0, 0);
  }
  Texture::SubImage2D(TextureTarget::_2D, *image, image->Width(), 0);
  DefaultTexture().Bind(TextureTarget::_2D);
  return texture;
}

namespace shadertoy {

  ChannelInputType channelTypeFromString(const QString & channelTypeStr) {
    ChannelInputType channelType = ChannelInputType::TEXTURE;
    if (channelTypeStr == "tex") {
      channelType = ChannelInputType::TEXTURE;
    } else if (channelTypeStr == "cube") {
      channelType = ChannelInputType::CUBEMAP;
    } 
    return channelType;
  }


  // FIXME move struct into the header and make save / load functions 
  // helpers defined here.  Shadertoy header should not be bound to Qt
  struct Shader {
    static const char * CHANNEL_REGEX; 
    static const char * XML_ROOT_NAME;
    static const char * XML_FRAGMENT_SOURCE;
    static const char * XML_CHANNEL;
    static const char * XML_CHANNEL_ATTR_TYPE;
    static const char * XML_CHANNEL_ATTR_ID;
    // Deprecated
    static const char * XML_CHANNEL_ATTR_SOURCE;

    QString url;
    QString name;
    QString fragmentSource;
    ChannelInputType channelTypes[MAX_CHANNELS];
    QUrl channelTextures[MAX_CHANNELS];

    Shader() {
      loadPreset(SHADERTOY_SHADERS_DEFAULT_XML);
    };

    Shader(Resource res) {
      loadPreset(res);
    };

    Shader(const QString & file) {
      loadFile(file);
    };

    Shader(QIODevice & ioDevice) {
      loadXml(ioDevice);
    };

    void loadPreset(Resource res) {
      QByteArray presetData = oria::qt::toByteArray(res);
      loadXml(QBuffer(&presetData));
    }

    void loadFile(const QString & file) {
      loadXml(QFile(file));
    }

    // FIXME no error handling.  
    void saveFile(const QString & name) {
      QDomDocument doc = writeDocument();
      QFile file(name);
      file.open(QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text);
      file.write(doc.toByteArray());
      file.close();
    }
    
    // FIXME no error handling.  
    void loadXml(QIODevice & ioDevice) {
      QDomDocument dom;
      dom.setContent(&ioDevice);

      auto children = dom.documentElement().childNodes();
      for (int i = 0; i < children.count(); ++i) {
        auto child = children.at(i);
        if (child.nodeName() == "url") {
          url = child.firstChild().nodeValue();
        } if (child.nodeName() == XML_FRAGMENT_SOURCE) {
          fragmentSource = child.firstChild().nodeValue();
        } else if (child.nodeName() == XML_CHANNEL) {
          auto attributes = child.attributes();
          int channelIndex = -1;
          QString source;
          if (attributes.contains(XML_CHANNEL_ATTR_ID)) {
            channelIndex = attributes.namedItem(XML_CHANNEL_ATTR_ID).nodeValue().toInt();
          }

          if (channelIndex < 0 || channelIndex >= shadertoy::MAX_CHANNELS) {
            continue;
          }


          // Compatibility mode
          if (attributes.contains(XML_CHANNEL_ATTR_SOURCE)) {
            source = attributes.namedItem(XML_CHANNEL_ATTR_SOURCE).nodeValue();
            QRegExp re(CHANNEL_REGEX);
            if (!re.exactMatch(source)) {
              continue;
            }
            channelTypes[channelIndex] = channelTypeFromString(re.cap(1));
            channelTextures[channelIndex] = QUrl("preset://" + re.cap(1) + "/" + re.cap(2));
            continue;
          }

          if (attributes.contains(XML_CHANNEL_ATTR_TYPE)) {
            channelTypes[channelIndex] = channelTypeFromString(attributes.namedItem(XML_CHANNEL_ATTR_SOURCE).nodeValue());
            channelTextures[channelIndex] = child.firstChild().nodeValue();
          }
        }
      }
    }


    QDomDocument writeDocument() {
      QDomDocument result;
      QDomElement root = result.createElement(XML_ROOT_NAME);
      result.appendChild(root);

      for (int i = 0; i < MAX_CHANNELS; ++i) {
        if (channelTextures[i] != QUrl()) {
          QDomElement channelElement = result.createElement(XML_CHANNEL);
          channelElement.setAttribute(XML_CHANNEL_ATTR_ID, i);
          channelElement.setAttribute(XML_CHANNEL_ATTR_TYPE, channelTypes[i] == ChannelInputType::CUBEMAP ? "cube" : "tex");
          channelElement.appendChild(result.createTextNode(channelTextures[i].toDisplayString()));
          root.appendChild(channelElement);
        }
      }
      root.appendChild(result.createElement(XML_FRAGMENT_SOURCE)).
        appendChild(result.createCDATASection(fragmentSource));

      return result;
    }
  };

  const char * Shader::CHANNEL_REGEX = "(\\w+)(\\d{2})";
  const char * Shader::XML_ROOT_NAME = "shadertoy";
  const char * Shader::XML_FRAGMENT_SOURCE = "fragmentSource";
  const char * Shader::XML_CHANNEL = "channel";
  const char * Shader::XML_CHANNEL_ATTR_ID = "id";
  const char * Shader::XML_CHANNEL_ATTR_SOURCE = "source";
  const char * Shader::XML_CHANNEL_ATTR_TYPE = "type";
}

struct ShadertoyRenderer : public QRiftWindow {
  Q_OBJECT
protected:
  struct Channel {
    oglplus::Texture::Target target;
    TexturePtr texture;
    vec3 resolution;
  };
  struct TextureData {
    TexturePtr tex;
    uvec2 size;
  };

  typedef std::map<QUrl, TextureData> TextureMap;
  typedef std::map<QUrl, QUrl> CanonicalUrlMap;
  CanonicalUrlMap canonicalUrlMap;
  TextureMap textureCache;

  // The currently active input channels
  Channel channels[4];
  QUrl channelSources[4];

  float eyePosScale{ 1.0f };
  float startTime{ 0.0f };
  // The current fragment source
  LambdaList uniformLambdas;
  // Contains the current 'camera position'
  vec3 position;
  // Geometry for the skybox used to render the scene
  ShapeWrapperPtr skybox;
  // A vertex shader shader, constant throughout the application lifetime
  VertexShaderPtr vertexShader;
  // The fragment shader used to render the shadertoy effect, as loaded 
  // from a preset or created or edited by the user
  FragmentShaderPtr fragmentShader;
  // The compiled shadertoy program
  ProgramPtr shadertoyProgram;

  void setupShadertoy() {
    initTextureCache();

    setShaderSourceInternal(oria::qt::toString(Resource::SHADERTOY_SHADERS_DEFAULT_FS));
    assert(shadertoyProgram);
    skybox = oria::loadSkybox(shadertoyProgram);
  }

  void initTextureCache() {
    using namespace shadertoy;
    for (int i = 0; i < MAX_TEXTURES; ++i) {
      Resource res = TEXTURES[i];
      if (NO_RESOURCE == res) {
        continue;
      }
      QUrl url(QString("image://resources/") + QString(Resources::getResourceMnemonic(res).c_str()));
      TextureData & cacheEntry = textureCache[url];
      cacheEntry.tex = oria::load2dTexture(res, cacheEntry.size);

      // Backward compatibility
      QUrl alternate = QUrl(QString().sprintf("preset://tex/%d", i));
      canonicalUrlMap[alternate] = url;
      textureCache[alternate] = cacheEntry;

      alternate = QUrl(QString().sprintf("preset://tex/%02d", i));
      canonicalUrlMap[alternate] = url;
      textureCache[alternate] = cacheEntry;
    }
    for (int i = 0; i < MAX_CUBEMAPS; ++i) {
      Resource res = CUBEMAPS[i];
      if (NO_RESOURCE == res) {
        continue;
      }
      static int resourceOrder[] = {
        0, 1, 2, 3, 4, 5
      };
      QUrl url(QString("image://resources/") + QString(Resources::getResourceMnemonic(res).c_str()));
      uvec2 size;
      TextureData & cacheEntry = textureCache[url];
      cacheEntry.tex = oria::loadCubemapTexture(res, resourceOrder, false);

      // Backward compatibility
      QUrl alternate = QUrl(QString().sprintf("preset://cube/%d", i));
      canonicalUrlMap[alternate] = url;
      textureCache[alternate] = cacheEntry;

      alternate = QUrl(QString().sprintf("preset://cube/%02d", i));
      canonicalUrlMap[alternate] = url;
      textureCache[alternate] = cacheEntry;
    }
  }

  void renderShadertoy() {
    using namespace oglplus;
    if (!shadertoyProgram) {
      return;
    }
    MatrixStack & mv = Stacks::modelview();
    mv.withPush([&] {
      mv.untranslate();
      oria::renderGeometry(skybox, shadertoyProgram, uniformLambdas);
    });
    for (int i = 0; i < 4; ++i) {
      oglplus::DefaultTexture().Active(0);
      DefaultTexture().Bind(Texture::Target::_2D);
      DefaultTexture().Bind(Texture::Target::CubeMap);
    }
    oglplus::Texture::Active(0);
  }

  void updateUniforms() {
    using namespace shadertoy;
    std::map<std::string, GLuint> activeUniforms = oria::getActiveUniforms(shadertoyProgram);
    shadertoyProgram->Bind();
    //    UNIFORM_DATE;
    for (int i = 0; i < 4; ++i) {
      const char * uniformName = shadertoy::UNIFORM_CHANNELS[i];
      if (activeUniforms.count(uniformName)) {
        // glUniform1i(activeUniforms[uniformName], i);
        // context()->functions()->glUniform1i(activeUniforms[uniformName], i);
        //Uniform<GLuint>(*skyboxProgram, uniformName).Set(i);
      }
    }
    if (activeUniforms.count(UNIFORM_RESOLUTION)) {
      vec3 textureSize = vec3(ovr::toGlm(this->eyeTextures[0].Header.TextureSize), 0);
      Uniform<vec3>(*shadertoyProgram, UNIFORM_RESOLUTION).Set(textureSize);
    }
    NoProgram().Bind();

    uniformLambdas.clear();
    if (activeUniforms.count(UNIFORM_GLOBALTIME)) {
      uniformLambdas.push_back([&] {
        Uniform<GLfloat>(*shadertoyProgram, UNIFORM_GLOBALTIME).Set(Platform::elapsedSeconds() - startTime);
      });
    }

    if (activeUniforms.count(shadertoy::UNIFORM_POSITION)) {
      uniformLambdas.push_back([&] {
//        if (!uiVisible) {
          Uniform<vec3>(*shadertoyProgram, shadertoy::UNIFORM_POSITION).Set(
            (ovr::toGlm(getEyePose().Position) + position) * eyePosScale);
//        }
      });
    }
    for (int i = 0; i < 4; ++i) {
      if (activeUniforms.count(UNIFORM_CHANNELS[i]) && channels[i].texture) {
        uniformLambdas.push_back([=] {
          if (this->channels[i].texture) {
            Texture::Active(i);
            this->channels[i].texture->Bind(channels[i].target);
          }
        });
      }
    }
  }

  virtual void setShaderSourceInternal(QString source) {
    try {
      position = vec3();
      if (!vertexShader) {
        vertexShader = VertexShaderPtr(new VertexShader());
        vertexShader->Source(Platform::getResourceString(Resource::SHADERTOY_SHADERS_DEFAULT_VS));
        vertexShader->Compile();
      }

      QString header = shadertoy::SHADER_HEADER;
      for (int i = 0; i < 4; ++i) {
        const Channel & channel = channels[i];
        // "uniform sampler2D iChannel0;\n"
        QString line; line.sprintf("uniform sampler%s iChannel%d;\n",
          channel.target == Texture::Target::CubeMap ? "Cube" : "2D", i);
        header += line;
      }
      header += shadertoy::LINE_NUMBER_HEADER;
      FragmentShaderPtr newFragmentShader(new FragmentShader());
      source.replace(QRegExp("\\bgl_FragColor\\b"), "FragColor").replace(QRegExp("\\btexture2D\\b"), "texture");
      source.insert(0, header);
      QByteArray qb = source.toLocal8Bit();
      GLchar * fragmentSource = (GLchar*)qb.data();
      StrCRef src(fragmentSource);
      newFragmentShader->Source(GLSLSource(src));
      newFragmentShader->Compile();
      ProgramPtr result(new Program());
      result->AttachShader(*vertexShader);
      result->AttachShader(*newFragmentShader);

      result->Link();
      shadertoyProgram.swap(result);
      if (!skybox) {
        skybox = oria::loadSkybox(shadertoyProgram);
      }
      fragmentShader.swap(newFragmentShader);
      updateUniforms();
      startTime = Platform::elapsedSeconds();
      emit compileSuccess();
    } catch (ProgramBuildError & err) {
      emit compileError(QString(err.Log().c_str()));
    }
  }

  virtual TexturePtr loadTexture(const QUrl & source) {
    qDebug() << "Looking for texture " << source;
    if (!textureCache.count(source)) {
      qDebug() << "Texture " << source << " not found, loading";
      // FIXME
      QFile f(source.toLocalFile());
      f.open(QFile::ReadOnly);
      QByteArray ba = f.readAll();
      std::vector<uint8_t> v;  v.assign(ba.constData(), ba.constData() + ba.size());
      textureCache[source].tex = oria::load2dTexture(v);
    }
    return textureCache[source].tex;
  }

  virtual void setChannelTextureInternal(int channel, shadertoy::ChannelInputType type, const QUrl & textureSource) {
    using namespace oglplus;
    Channel & channelRef = channels[channel];
    if (textureSource == channelSources[channel]) {
      return;
    }

    channelSources[channel] = textureSource;

    if (QUrl() == textureSource) {
      channels[channel].texture.reset();
      channels[channel].target = Texture::Target::_2D;
      return;
    }

    Channel newChannel;
    uvec2 size;
    switch (type) {
    case shadertoy::ChannelInputType::TEXTURE:
      newChannel.texture = loadTexture(textureSource);
      newChannel.target = Texture::Target::_2D;
      //      newChannel.resolution = vec3(textureCache[res].size, 0);
      break;

    case shadertoy::ChannelInputType::CUBEMAP:
    {
      newChannel.texture = loadTexture(textureSource);
      newChannel.target = Texture::Target::CubeMap;
    }
    break;
    case shadertoy::ChannelInputType::VIDEO:
      break;
    case shadertoy::ChannelInputType::AUDIO:
      break;
    }

    channels[channel] = newChannel;
  }

  virtual void setShaderInternal(const shadertoy::Shader & shader) {
    for (int i = 0; i < shadertoy::MAX_CHANNELS; ++i) {
      setChannelTextureInternal(i, shader.channelTypes[i], shader.channelTextures[i]);
    }
    setShaderSourceInternal(shader.fragmentSource);
  }

signals:
  void compileError(QString source);
  void compileSuccess();
};

class ShadertoyRiftWidget : public ShadertoyRenderer {
  Q_OBJECT
  
  typedef std::atomic<GLuint> AtomicGlTexture;
  typedef std::pair<GLuint, GLsync> SyncPair;
  typedef std::queue<SyncPair> TextureTrashcan;
  typedef std::atomic<vec2> ActomicVec2;
  typedef std::vector<GLuint> TextureDeleteQueue;
  typedef std::mutex Mutex;
  typedef std::unique_lock<Mutex> Lock;
  // A cache of all the input textures available 
  QDir configPath;

  shadertoy::Shader activeShader;

  //////////////////////////////////////////////////////////////////////////////
  // 
  // Offscreen UI
  //

  QOffscreenUi uiWindow;
  GlslHighlighter highlighter;
  QQuickItem * editorControl;
  int activePresetIndex{ 0 };
  float savedEyePosScale{ 1.0f };

  //////////////////////////////////////////////////////////////////////////////
  // 
  // Shader Rendering information
  //

  // We actually render the shader to one FBO for dynamic framebuffer scaling,
  // while leaving the actual texture we pass to the Oculus SDK fixed.
  // This allows us to have a clear UI regardless of the shader performance
  FramebufferWrapperPtr shaderFramebuffer;

  // The current mouse position as reported by the main thread
  ActomicVec2 mousePosition;
  bool uiVisible{ false };

  // The shadertoy rendering resolution scale.  1.0 means full resolution 
  // as defined by the Oculus SDK as the ideal offscreen resolution 
  // pre-distortion
  float texRes{ 1.0f };

  // A wrapper for passing the UI texture from the app to the widget
  AtomicGlTexture uiTexture{ 0 };
  TextureTrashcan textureTrash;
  TextureDeleteQueue textureDeleteQueue;
  Mutex textureLock;
  QTimer timer;

  // GLSL and geometry for the UI
  ProgramPtr uiProgram;
  ShapeWrapperPtr uiShape;
  TexturePtr mouseTexture;
  ShapeWrapperPtr mouseShape;

  // For easy compositing the UI texture and the mouse texture
  FramebufferWrapperPtr uiFramebuffer;

  // Geometry and shader for rendering the possibly low res shader to the main framebuffer
  ProgramPtr planeProgram;
  ShapeWrapperPtr plane;

  // Measure the FPS for use in dynamic scaling
  GLuint exchangeUiTexture(GLuint newUiTexture) {
    return uiTexture.exchange(newUiTexture);
  }

public:
  ShadertoyRiftWidget() : timer(this) {
    {
      QString configLocation = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
      configPath = QDir(configLocation);
      configPath.mkpath("shaders");
    }

    connect(&timer, &QTimer::timeout, this, &ShadertoyRiftWidget::onTimer);
    timer.start(100);

    connect(this, &ShadertoyRiftWidget::fpsUpdated, this, [&](float fps){
      setItemText("fps", QString().sprintf("%0.2f", fps));
    });
    setupOffscreenUi();
    loadPreset(shadertoy::PRESETS[0]);
  }

private:

  virtual void initializeRiftRendering() {
    QRiftWindow::initializeRiftRendering();
    setupShadertoy();


    // The geometry and shader for rendering the 2D UI surface when needed
    uiProgram = oria::loadProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_TEXTURED_FS);
    uiShape = oria::loadPlane(uiProgram, UI_ASPECT);

    // The geometry and shader for scaling up the rendered shadertoy effect
    // up to the full offscreen render resolution.  This is then compositied 
    // with the UI window
    planeProgram = oria::loadProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_TEXTURED_FS);
    plane = oria::loadPlane(planeProgram, 1.0);

    mouseTexture = loadCursor(Resource::IMAGES_CURSOR_PNG);
    mouseShape = oria::loadPlane(uiProgram, UI_INVERSE_ASPECT);

    uiFramebuffer = FramebufferWrapperPtr(new FramebufferWrapper());
    uiFramebuffer->init(UI_SIZE);

    shaderFramebuffer = FramebufferWrapperPtr(new FramebufferWrapper());
    shaderFramebuffer->init(ovr::toGlm(eyeTextures[0].Header.TextureSize));
    DefaultFramebuffer().Bind(Framebuffer::Target::Draw);
  }

  void setupOffscreenUi() {
    qApp->setFont(QFont("Arial", 24, QFont::Bold));
    uiWindow.pause();
    uiWindow.setup(
      QUrl::fromLocalFile("C:\\Users\\bdavis\\Git\\OculusRiftExamples\\resources\\shadertoy\\Combined.qml"),
      QSize(UI_SIZE.x, UI_SIZE.y), context());

    connect(&uiWindow, &QOffscreenUi::textureUpdated, this, [&] {
      GLuint newTexture = uiWindow.m_fbo->takeTexture();
      GLuint oldTexture = exchangeUiTexture(newTexture);
      if (oldTexture) {
        glDeleteTextures(1, &oldTexture);
      }
    });

    editorControl = uiWindow.m_rootItem->findChild<QQuickItem*>("shaderTextEdit");
    highlighter.setDocument(
      editorControl->property("textDocument").value<QQuickTextDocument*>()->textDocument());

    QObject::connect(uiWindow.m_rootItem, SIGNAL(toggleUi()),
      this, SLOT(onToggleUi()));
    QObject::connect(uiWindow.m_rootItem, SIGNAL(channelTextureChanged(int, int, QString)),
      this, SLOT(onChannelTextureChanged(int, int, QString)));
    QObject::connect(uiWindow.m_rootItem, SIGNAL(shaderSourceChanged(QString)),
      this, SLOT(onShaderSourceChanged(QString)));

    // FIXME add confirmation for when the user might lose edits.
    QObject::connect(uiWindow.m_rootItem, SIGNAL(loadNextPreset()), 
      this, SLOT(onLoadNextPreset()));
    QObject::connect(uiWindow.m_rootItem, SIGNAL(loadPreviousPreset()), 
      this, SLOT(onLoadPreviousPreset()));
    QObject::connect(uiWindow.m_rootItem, SIGNAL(recenterPose()),
      this, SLOT(onRecenterPosition()));
    QObject::connect(uiWindow.m_rootItem, SIGNAL(modifyTextureResolution(double)),
      this, SLOT(onModifyTextureResolution(double)));
    QObject::connect(uiWindow.m_rootItem, SIGNAL(modifyPositionScale(double)),
      this, SLOT(onModifyPositionScale(double)));
    QObject::connect(uiWindow.m_rootItem, SIGNAL(resetPositionScale()),
      this, SLOT(onResetPositionScale()));
    QObject::connect(uiWindow.m_rootItem, SIGNAL(toggleEyePerFrame()),
      this, SLOT(onToggleEyePerFrame()));
    QObject::connect(uiWindow.m_rootItem, SIGNAL(startShutdown()),
      this, SLOT(onShutdown()));
    QObject::connect(uiWindow.m_rootItem, SIGNAL(restartShader()),
      this, SLOT(onRestartShader()));

    setItemText("eps", QString().sprintf("%0.2f", eyePosScale));
    setItemText("res", QString().sprintf("%0.2f", texRes));

    uiWindow.setProxyWindow(this);
  }

  void setItemProperty(const QString & itemName, const QString & property, const QVariant & value) {
    QQuickItem * item = uiWindow.m_rootItem->findChild<QQuickItem*>(itemName);
    if (nullptr != item) {
      item->setProperty(property.toLocal8Bit(), value);
    }
  }

  void setItemText(const QString & itemName, const QString & text) {
    setItemProperty(itemName, "text", text);
  }


private slots:
  void onToggleUi() {
  uiVisible = !uiVisible;
  if (uiVisible) {
    savedEyePosScale = eyePosScale;
    eyePosScale = 0.0f;
    uiWindow.resume();
    editorControl->setProperty("readOnly", false);
  } else {
    eyePosScale = savedEyePosScale;
    editorControl->setProperty("readOnly", true);
    uiWindow.pause();
  }
}

  void onLoadNextPreset() {
    int newPreset = (activePresetIndex + 1) % shadertoy::MAX_PRESETS;
    loadPreset(shadertoy::PRESETS[newPreset]);
  }

  void onLoadPreviousPreset() {
    int newPreset = (activePresetIndex + shadertoy::MAX_PRESETS - 1) % shadertoy::MAX_PRESETS;
    loadPreset(shadertoy::PRESETS[newPreset]);
  }

  void onChannelTextureChanged(const int & channelIndex, const int & channelType, const QString & texturePath) {
    queueRenderThreadTask([&, channelIndex, channelType, texturePath] {
      setChannelTextureInternal(channelIndex, 
        (shadertoy::ChannelInputType)channelType, 
        QUrl("image://resources/" + texturePath));
      updateUniforms();
    });
  }

  void onShaderSourceChanged(const QString & shaderSource) {
    queueRenderThreadTask([&, shaderSource] {
      setShaderSourceInternal(shaderSource);
      updateUniforms();
    });
  }

  void onRecenterPosition() {
    queueRenderThreadTask([&] {
      ovrHmd_RecenterPose(hmd);
    });
  }

  void onModifyTextureResolution(double scale) {
    float newRes = scale * texRes;
    newRes = std::max(0.1f, std::min(1.0f, newRes));
    if (newRes != texRes) {
      queueRenderThreadTask([&, newRes] {
        texRes = newRes;
      });
      // FIXME update the UI
      setItemText("res", QString().sprintf("%0.2f", newRes));
    }
  }

  void onModifyPositionScale(double scale) {
    float newPosScale = scale * eyePosScale;
    queueRenderThreadTask([&, newPosScale] {
      eyePosScale = newPosScale;
    });
    // FIXME update the UI
    setItemText("eps", QString().sprintf("%0.2f", newPosScale));
  }

  void onResetPositionScale() {
    queueRenderThreadTask([&] {
      eyePosScale = 1.0f;
    });
    // FIXME update the UI
    setItemText("eps", QString().sprintf("%0.2f", 1.0f));
  }

  void onToggleEyePerFrame() {
    bool newEyePerFrameMode = !eyePerFrameMode;
    queueRenderThreadTask([&, newEyePerFrameMode] {
      eyePerFrameMode = newEyePerFrameMode;
    });
    setItemText("epf", newEyePerFrameMode ? "On" : "Off");
  }

  void onRestartShader() {
    queueRenderThreadTask([&] {
      startTime = Platform::elapsedSeconds();
    });
  }

  void onShutdown() {
    stop();
    QApplication::instance()->quit();
  }

  void onTimer() {
    TextureDeleteQueue tempTextureDeleteQueue;

    // Scope the lock tightly
    {
      Lock lock(textureLock);
      if (!textureDeleteQueue.empty()) {
        tempTextureDeleteQueue.swap(textureDeleteQueue);
      }
    }

    if (!tempTextureDeleteQueue.empty()) {
      uiWindow.deleteOldTextures(tempTextureDeleteQueue);
    }
  }

private:
  void loadShader(const shadertoy::Shader & shader) {
    assert(!shader.fragmentSource.isEmpty());
    activeShader = shader;
    editorControl->setProperty("text", shader.fragmentSource);
    for (int i = 0; i < 4; ++i) {
      QUrl url = activeShader.channelTextures[i];
        
      while (canonicalUrlMap.count(url)) {
        url = canonicalUrlMap[url];
      }
      qDebug() << url;
      setItemProperty(QString().sprintf("channel%d", i), "source",  url);
    }
    // FIXME update the channel texture buttons
    queueRenderThreadTask([&, shader] {
      setShaderInternal(shader);
      updateUniforms();
    });
  }

  void loadPreset(shadertoy::Preset & preset) {
    for (int i = 0; i < shadertoy::MAX_PRESETS; ++i) {
      if (shadertoy::PRESETS[i].res == preset.res) {
        activePresetIndex = i;
      }
    }
    loadShader(shadertoy::Shader(preset.res));
  }

  void loadFile(const QString & file) {
    loadShader(shadertoy::Shader(file));
  }

  void updateFps(float fps) {
    emit fpsUpdated(fps);
  }

  ///////////////////////////////////////////////////////
  //
  // Event handling customization
  // 
  void mouseMoveEvent(QMouseEvent * me) { 
    // Make sure we don't show the system cursor over the window
    qApp->setOverrideCursor(QCursor(Qt::BlankCursor));
    // Interpret the mouse position as NDC coordinates
    vec2 mp = oria::qt::toGlm(me->localPos());
    mp /= oria::qt::toGlm(size());
    mp *= 2.0f;
    mp -= 1.0f;
    mp.y *= -1.0f;
    mousePosition.store(mp);
    QRiftWindow::mouseMoveEvent(me);
  }

  bool event(QEvent * e) {
    static bool dismissedHmd = false;
    switch (e->type()) {
    case QEvent::KeyPress:
      if (!dismissedHmd) {
        // Allow the user to remove the HSW message early
        ovrHSWDisplayState hswState;
        ovrHmd_GetHSWDisplayState(hmd, &hswState);
        if (hswState.Displayed) {
          ovrHmd_DismissHSWDisplay(hmd);
          dismissedHmd = true;
          return true;
        }
      }
    case QEvent::KeyRelease:
    case QEvent::Scroll:
    case QEvent::Wheel:
      if (QApplication::sendEvent(uiWindow.m_quickWindow, e)) {
        return true;
      }
      break;

    case QEvent::MouseMove:
//      mouseMoveEvent((QMouseEvent*)e);
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
      forwardMouseEvent((QMouseEvent*)e);
      return QRiftWindow::event(e);
    }

    return QRiftWindow::event(e);
  }

  void resizeEvent(QResizeEvent *e) {}

  void forwardMouseEvent(QMouseEvent * e) {
    QPointF pos = e->localPos();
    pos.rx() /= size().width();
    pos.ry() /= size().height();
    pos.rx() *= UI_SIZE.x;
    pos.ry() *= UI_SIZE.y;
    QMouseEvent mappedEvent(e->type(), pos, e->screenPos(), e->button(), e->buttons(), e->modifiers());
    QCoreApplication::sendEvent(uiWindow.m_quickWindow, &mappedEvent);
  }

  ///////////////////////////////////////////////////////
  //
  // Rendering functionality
  // 
  vec2 textureSize() {
    return vec2(ovr::toGlm(eyeTextures[0].Header.TextureSize));
  }

  uvec2 renderSize() {
    return uvec2(texRes * textureSize());
  }

  void renderScene() {
    Context::Enable(Capability::Blend);
    Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);

    Context::Disable(Capability::ScissorTest);
    Context::Disable(Capability::DepthTest);
    Context::Disable(Capability::CullFace);
    // Context::Clear().DepthBuffer().ColorBuffer();

    // Render the shadertoy effect into a framebuffer, possibly at a 
    // smaller resolution than recommended
    shaderFramebuffer->Bound([&] {
      oria::viewport(renderSize());
      renderShadertoy();
    });
    oria::viewport(textureSize());

    // Re-render the shadertoy texture to the current framebuffer, 
    // stretching it to fit the scene
    Stacks::withIdentity([&] {
      shaderFramebuffer->color.Bind(Texture::Target::_2D);
      oria::renderGeometry(plane, planeProgram, LambdaList({ [&] {
        Uniform<vec2>(*planeProgram, "UvMultiplier").Set(vec2(texRes));
      } }));
    });

    if (uiVisible) {
      static GLuint lastUiTexture = 0;
      static GLsync lastUiSync;
      GLuint currentUiTexture = uiTexture.exchange(0);
      if (0 == currentUiTexture) {
        currentUiTexture = lastUiTexture;
      } else {
        // If the texture has changed, push it into the trash bin for 
        // deletion once it's finished rendering
        if (lastUiTexture) {
          textureTrash.push(SyncPair(lastUiTexture, lastUiSync));
        }
        lastUiTexture = currentUiTexture;
      }

      MatrixStack & mv = Stacks::modelview();
      MatrixStack & pr = Stacks::projection();
      if (currentUiTexture) {
        Texture::Active(0);
        // Composite the UI image and the mouse sprite
        uiFramebuffer->Bound([&] {
          Context::Clear().ColorBuffer();
          oria::viewport(UI_SIZE);
          // Clear out the projection and modelview here.
          Stacks::withIdentity([&] {
            glBindTexture(GL_TEXTURE_2D, currentUiTexture);
            oria::renderGeometry(plane, uiProgram);

            // Render the mouse sprite on the UI
            vec2 mp = mousePosition.load();
            mv.translate(vec3(mp.x, mp.y, 0.0f));
            mv.scale(vec3(0.1f));
            mouseTexture->Bind(Texture::Target::_2D);
            oria::renderGeometry(mouseShape, uiProgram);
          });
        });
        lastUiSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        oria::viewport(textureSize());
        mv.withPush([&] {
          mv.translate(vec3(0, 0, -1));
          uiFramebuffer->BindColor();
          oria::renderGeometry(uiShape, uiProgram);
        });
      }
    }

    TextureDeleteQueue tempTextureDeleteQueue;
    while (!textureTrash.empty()) {
      SyncPair & top = textureTrash.front();
      GLuint & texture = top.first;
      GLsync & sync = top.second;
      GLenum result = glClientWaitSync(sync, 0, 0);
      if (GL_ALREADY_SIGNALED == result || GL_CONDITION_SATISFIED == result) {
        tempTextureDeleteQueue.push_back(top.first);
        textureTrash.pop();
      } else {
        break;
      }
    }

    if (!tempTextureDeleteQueue.empty()) {
      Lock lock(textureLock);
      textureDeleteQueue.insert(textureDeleteQueue.end(), 
        tempTextureDeleteQueue.begin(), tempTextureDeleteQueue.end());
    }
  }

public slots:

  void onSixDofMotion(const vec3 & tr, const vec3 & mo) {
    SAY("%f, %f, %f", tr.x, tr.y, tr.z);
    queueRenderThreadTask([&, tr, mo] {
      position += tr;
    });
  }

signals:
  void fpsUpdated(float);
};

class ShadertoyApp : public QApplication {
  Q_OBJECT

  ShadertoyRiftWidget riftRenderWidget;

  // A timer for updating the UI
  QWidget desktopWindow;
  shadertoy::Shader activeShader;

public:
  ShadertoyApp(int argc, char ** argv) : QApplication(argc, argv) { 
    QCoreApplication::setOrganizationName(ORG_NAME);
    QCoreApplication::setOrganizationDomain(ORG_DOMAIN);
    QCoreApplication::setApplicationName(APP_NAME);
    setupDesktopWindow();
    riftRenderWidget.start();
    riftRenderWidget.requestActivate();
  }

  virtual ~ShadertoyApp() {
  }

private:
  void setupDesktopWindow() {
    desktopWindow.setLayout(new QFormLayout());
    QLabel * label = new QLabel("Your Oculus Rift is now active.  Please put on your headset.  Share and enjoy");
    desktopWindow.layout()->addWidget(label);
    desktopWindow.show();
  }


signals:
  void channelTextureChanged(int channelIndex, shadertoy::ChannelInputType, const QUrl & textureSource);
  void shaderLoaded(const shadertoy::Shader & shader);
  void uiToggled(bool newValue);
  void mouseMoved();
  void refreshUserShaders();
  void toggleOvrFlag(ovrHmdCaps cap);
  void sixDof(vec3 tr, vec3 ro);
};

MAIN_DECL { 
  try { 
    ovr_Initialize();
#ifndef _DEBUG
    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", "."); 
#endif
    QT_APP_WITH_ARGS(ShadertoyApp);
    return app.exec(); 
  } catch (std::exception & error) { 
    SAY_ERR(error.what()); 
  } catch (const std::string & error) { 
    SAY_ERR(error.c_str()); 
  } 
  return -1; 
} 

#include "Example_10_Shaderfun.moc"




#if 0
SiHdl si;
SiGetEventData getEventData;
bool nativeEventFilter(const QByteArray & eventType, void * message, long * result) {
  // On Windows, eventType is set to "windows_generic_MSG" for messages sent 
  // to toplevel windows, and "windows_dispatcher_MSG" for system - wide 
  // messages such as messages from a registered hot key.In both cases, the 
  // message can be casted to a MSG pointer.The result pointer is only used 
  // on Windows, and corresponds to the LRESULT pointer.
  MSG & msg = *(MSG*)message;
  SiSpwEvent siEvent;
  vec3 tr, ro;
  long * md = siEvent.u.spwData.mData;
  SiGetEventWinInit(&getEventData, msg.message, msg.wParam, msg.lParam);
  if (SiGetEvent(si, 0, &getEventData, &siEvent) == SI_IS_EVENT) {
    switch (siEvent.type) {
    case SI_MOTION_EVENT:
      emit sixDof(
        vec3(md[SI_TX], md[SI_TY], -md[SI_TZ]),
        vec3(md[SI_RX], md[SI_RY], md[SI_RZ]));
      break;

    default:
      break;
    }
    return true;
  }
  return false;
}
#endif


#if 0
SpwRetVal result = SiInitialize();
int cnt = SiGetNumDevices();
SiDevID devId = SiDeviceIndex(0);
SiOpenData siData;
SiOpenWinInit(&siData, (HWND)riftRenderWidget.effectiveWinId());
si = SiOpen("app", devId, SI_NO_MASK, SI_EVENT, &siData);
installNativeEventFilter(this);
#endif
