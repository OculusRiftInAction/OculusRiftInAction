#ifdef _DEBUG
#define BRAD_DEBUG 1
#endif

#include "Common.h"
#include <QtOpenGL/QGLContext>
#include <QtOpenGL/QGLFunctions>
#include <QDomDocument>
#include <QXmlQuery>

#include "Shadertoy.h"

const char * ORG_NAME = "Oculusr Rift in Action";
const char * ORG_DOMAIN = "oculusriftinaction.com";
const char * APP_NAME = "ShadertoyVR";

using namespace oglplus;

#define UI_X 1280
#define UI_Y 720

static const float ROOT_2 = sqrt(2.0f);
static const float INV_ROOT_2 = 1.0f / ROOT_2;
static uvec2 UI_SIZE(UI_X, UI_Y);


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
          // FIXME
          //QString source; source.sprintf("%s%d",
          //  (channelTypes[i] == ChannelInputType::CUBEMAP ? "cube" : "tex"),
          //  channelIndices[i]);
          //channelElement.setAttribute(XML_CHANNEL_ATTR_SOURCE, source);
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

class ShadertoyRiftWidget : public QRiftWidget {
  Q_OBJECT
  typedef std::atomic<GLuint> AtomicGlTexture;
  typedef std::shared_ptr<oglplus::VertexShader> VertexShaderPtr;
  typedef std::shared_ptr<oglplus::FragmentShader> FragmentShaderPtr;
  typedef std::pair<shadertoy::ChannelInputType, int> ChannelPair;
  typedef std::vector<ChannelPair> ChannelVector;
  typedef std::pair<GLuint, GLsync> SyncPair;
  typedef std::queue<SyncPair> TextureTrashcan;

  struct TextureData {
    TexturePtr tex;
    uvec2 size;
  };
  typedef std::map<QUrl, TextureData> TextureMap;

  // A cache of all the input textures available 
  TextureMap textureCache;

  void initTextureCache() {
    using namespace shadertoy;
    for (int i = 0; i < MAX_TEXTURES; ++i) {
      Resource res = TEXTURES[i];
      if (NO_RESOURCE == res) {
        continue;
      }

      uvec2 size;
      TexturePtr ptr = oria::load2dTexture(res, size);
      textureCache[QUrl("preset://tex/" + i)].tex = ptr;
      textureCache[QUrl("preset://tex/" + i)].size = size;
      textureCache[QUrl(QString().sprintf("preset://tex/%02d", i))].tex = ptr;
      textureCache[QUrl(QString().sprintf("preset://tex/%02d", i))].size = size;
    }
    for (int i = 0; i < MAX_CUBEMAPS; ++i) {
      Resource res = CUBEMAPS[i];
      if (NO_RESOURCE == res) {
        continue;
      }
      static int resourceOrder[] = {
        0, 1, 2, 3, 4, 5
      };
      uvec2 size;
      TexturePtr ptr = oria::loadCubemapTexture(res, resourceOrder, false);
      textureCache[QUrl("preset://cube/" + i)].tex = ptr;
      textureCache[QUrl(QString().sprintf("preset://cube/%02d", i))].tex = ptr;
    }
  }

  // Contains the current 'camera position'
  vec3 position;


  // Geometry for the skybox used to render the scene
  ShapeWrapperPtr skybox;
  // A vertex shader shader, constant throughout the application lifetime
  VertexShaderPtr vertexShader;
  // The fragment shader used to render the shadertoy effect, as loaded 
  // from a preset or created or edited by the user
  FragmentShaderPtr fragmentShader;

  // We actually render the shader to one FBO for dynamic framebuffer scaling,
  // while leaving the actual texture we pass to the Oculus SDK fixed.
  // This allows us to have a clear UI regardless of the shader performance
  FramebufferWrapperPtr shaderFramebuffer;
  ProgramPtr shadertoyProgram;

  // The currently active input channels
  struct Channel {
    QUrl source;
    oglplus::Texture::Target target;
    TexturePtr texture;
    vec3 resolution;
  };

  Channel channels[4];

  // The shadertoy rendering resolution scale.  1.0 means full resolution 
  // as defined by the Oculus SDK as the ideal offscreen resolution 
  // pre-distortion
  float texRes{ 1.0f };
  float eyePosScale{ 1.0f };
  float startTime{ 0.0f };

  bool uiVisible{ false };
  
  // A wrapper for passing the UI texture from the app to the widget
  AtomicGlTexture uiTexture{ 0 };
  ProgramPtr uiProgram;
  ShapeWrapperPtr uiShape;

  // GLSL and geometry for the UI
  ProgramPtr planeProgram;
  ShapeWrapperPtr plane;

  // Measure the FPS for use in dynamic scaling
  RateCounter fps;

  // The current fragment source
  LambdaList uniformLambdas;


protected:

public:
  explicit ShadertoyRiftWidget(QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0)
    : QRiftWidget(parent, shareWidget, f) {
  }

  explicit ShadertoyRiftWidget(QGLContext *context, QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0)
    : QRiftWidget(context, parent, shareWidget, f) {
  }

  explicit ShadertoyRiftWidget(const QGLFormat& format, QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0)
    : QRiftWidget(format, parent, shareWidget, f) {
  }


  int currentPresetIndex = 0;

  GLuint exchangeUiTexture(GLuint newUiTexture) {
    return uiTexture.exchange(newUiTexture);
  }

protected:
  virtual void setup() {
    QRiftWidget::setup();
    initTextureCache();
  }
  
  void mouseMoveEvent(QMouseEvent * me) {
    emit mouseMoved(me->localPos());
    QRiftWidget::mouseMoveEvent(me);
  }

  void keyPressEvent(QKeyEvent * ke) {
    int key = ke->key();

    // Allow the use to 
    ovrHSWDisplayState hswState;
    ovrHmd_GetHSWDisplayState(hmd, &hswState);
    if (hswState.Displayed) {
      ovrHmd_DismissHSWDisplay(hmd);
      return;
    }

    // FUCK, I'm running out of easily locatable keys
    // FIXME support joystick buttons?
    switch (key) {
    case Qt::Key_Q:
      if (Qt::ControlModifier == ke->modifiers()) {
        emit shutdown();
      }
      return;

    case Qt::Key_Escape:
    case Qt::Key_F1:
      uiVisible = !uiVisible;
      emit uiVisibleChanged(uiVisible);
      return;

    case Qt::Key_F3:
      eyePerFrameMode = !eyePerFrameMode;
      break;

    case Qt::Key_F4:
      if (Qt::ShiftModifier == ke->modifiers()) {
        startTime = Platform::elapsedSeconds();
      } else {
        emit recompileShader();
      }
      return;

      return;

    case Qt::Key_F5:
      if (Qt::ShiftModifier == ke->modifiers()) {
        eyePosScale *= INV_ROOT_2;
      } else {
        texRes = std::max(texRes * INV_ROOT_2, 0.05f);
        emit textureResolutionChanged(texRes);
      }
      return;

    case Qt::Key_F6:
      if (Qt::ShiftModifier == ke->modifiers()) {
        eyePosScale = 1.0f;
      } else {
        texRes = std::max(texRes * 0.95f, 0.05f);
        emit textureResolutionChanged(texRes);
      }
      return;

    case Qt::Key_F7:
      if (Qt::ShiftModifier == ke->modifiers()) {
        eyePosScale = 1.0f;
      } else {
        texRes = std::min(texRes * 1.05f, 1.0f);
        emit textureResolutionChanged(texRes);
      }
      return;

    case Qt::Key_F8:
      if (Qt::ShiftModifier == ke->modifiers()) {
        eyePosScale *= ROOT_2;
      } else {
        texRes = std::min(texRes * ROOT_2, 1.0f);
        emit textureResolutionChanged(texRes);
      }
      return;

    case Qt::Key_F9:
      emit loadPreviousPreset();
      return;

    case Qt::Key_F10:
      emit loadNextPreset();
      return;

    case Qt::Key_F11:
      // FIXME implement a quicksave listener
      emit saveShader();
      return;

    case Qt::Key_F2:
    case Qt::Key_F12:
      ovrHmd_RecenterPose(hmd);
      return;
    }

    // Didn't handle the key
    QRiftWidget::keyPressEvent(ke);
  }

  void enterEvent ( QEvent * event ) {
    qApp->setOverrideCursor( QCursor( Qt::BlankCursor ) );
  }

  vec2 textureSize() {
    return vec2(ovr::toGlm(eyeTextures[0].Header.TextureSize));
  }

  uvec2 renderSize() {
    return uvec2(texRes * textureSize());
  }

  void renderSkybox() {
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

  void renderScene() {
    Context::Disable(Capability::Blend);
    Context::Disable(Capability::ScissorTest);
    Context::Disable(Capability::DepthTest);
    Context::Disable(Capability::CullFace);
    Context::Clear().DepthBuffer().ColorBuffer();

    // Render the shadertoy effect into a framebuffer
    oria::viewport(textureSize());
    shaderFramebuffer->Bound([&] {
      oria::viewport(renderSize());
      renderSkybox();
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
      GLuint currentUiTexture = uiTexture.exchange(0);
      if (0 == currentUiTexture) {
        currentUiTexture = lastUiTexture;
      } else {
        // If the texture has changed, push it into the trash bin for 
        // deletion once it's finished rendering
        if (lastUiTexture) {
          glDeleteTextures(1, &lastUiTexture);
        }
        lastUiTexture = currentUiTexture;
      }

      MatrixStack & mv = Stacks::modelview();
      MatrixStack & pr = Stacks::projection();
      if (currentUiTexture) {
        mv.withPush([&] {
          mv.translate(vec3(0, 0, -1));
          Texture::Active(0);
          glBindTexture(GL_TEXTURE_2D, currentUiTexture);
          oria::renderGeometry(uiShape, uiProgram);
        });
      }
    }
  }

  virtual void initGl() {
    QRiftWidget::initGl();

    // The geometry and shader for rendering the 2D UI surface when needed
    uiProgram = oria::loadProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_TEXTURED_FS);
    uiShape = oria::loadPlane(uiProgram, (float)UI_X / (float)UI_Y);

    // The geometry and shader for scaling up the rendered shadertoy effect
    // up to the full offscreen render resolution.  This is then compositied 
    // with the UI window
    planeProgram = oria::loadProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_TEXTURED_FS);
    plane = oria::loadPlane(planeProgram, 1.0);

    // This function 
    setShaderSourceInternal(oria::qt::toString(Resource::SHADERTOY_SHADERS_DEFAULT_FS));
    assert(shadertoyProgram);
    skybox = oria::loadSkybox(shadertoyProgram);
    shaderFramebuffer = FramebufferWrapperPtr(new FramebufferWrapper());
    shaderFramebuffer->init(ovr::toGlm(eyeTextures[0].Header.TextureSize));
    DefaultFramebuffer().Bind(Framebuffer::Target::Draw);
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
      OutputDebugStringA(fragmentSource);
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
    if (textureSource == channelRef.source) {
      return;
    }

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

  void updateUniforms() {
    using namespace shadertoy;
    std::map<std::string, GLuint> activeUniforms = oria::getActiveUniforms(shadertoyProgram);
    shadertoyProgram->Bind();
    //    UNIFORM_DATE;
    for (int i = 0; i < 4; ++i) {
      const char * uniformName = shadertoy::UNIFORM_CHANNELS[i];
      if (activeUniforms.count(uniformName)) {
        context()->functions()->glUniform1i(activeUniforms[uniformName], i);
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
        if (!uiVisible) {
          Uniform<vec3>(*shadertoyProgram, shadertoy::UNIFORM_POSITION).Set(
            (ovr::toGlm(getEyePose().Position)  + position) * eyePosScale
          );
        }
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

  void onFrameEnd() {
    fps.increment();
  }

  virtual void resizeEvent(QResizeEvent *e) { }
  virtual void paintEvent(QPaintEvent *) { }

public slots:
  void onSixDofMotion(const vec3 & tr, const vec3 & mo) {
    SAY("%f, %f, %f", tr.x, tr.y, tr.z);
    queueRenderThreadTask([&, tr, mo] {
      position += tr;
    });
  }

  void setTextureResolution(float texRes) {
    queueRenderThreadTask([&, texRes] {
      this->texRes = texRes;
    });
  }

  void setChannelTexture(int channel, shadertoy::ChannelInputType type, const QUrl & textureSource) {
    queueRenderThreadTask([&, channel, type, textureSource] {
      setChannelTextureInternal(channel, type, textureSource);
      updateUniforms();
    });
  }

  void toggleOvrFlag(ovrHmdCaps flag) {
    int caps = ovrHmd_GetEnabledCaps(hmd);
    if (caps & flag) {
      ovrHmd_SetEnabledCaps(hmd, caps & ~flag);
    } else {
      ovrHmd_SetEnabledCaps(hmd, caps | flag);
    }
  }

  void setShaderSource(QString source) {
    queueRenderThreadTask([&, source] {
      setShaderSourceInternal(source);
      updateUniforms();
    });
  }

  void setShader(const shadertoy::Shader & shader) {
    queueRenderThreadTask([&, shader] {
      setShaderInternal(shader);
      updateUniforms();
    });
  }

signals:
  void compileError(QString source);
  void compileSuccess();
  void uiVisibleChanged(bool uiVisible);
  void loadNextPreset();
  void loadPreviousPreset();
  void recompileShader();
  void textureResolutionChanged(float texRes);
  void mouseMoved(QPointF localPos);
  void shutdown();
  void saveShader();
};

class ShadertoyApp : public QApplication {
  Q_OBJECT

  ShadertoyRiftWidget riftRenderWidget;

  // A timer for updating the UI
  QTimer timer;
  // Messages comeing from the rift widget arrive on the rift's render thread, 
  // so anything we need to happen on the main UI thread must be queued here
  // and pulled off by a timer.
  TaskQueueWrapper tasks;
  bool uiVisible{ false };

  // These structures are used to contain the UI elements for the application.  
  // FIXME Move to QML for controls and offscreen rendering using QQuickRenderControl
  ForwardingGraphicsView uiView;
  QGraphicsScene uiScene;
  QGLWidget uiGlWidget;

  QWidget desktopWindow;
  QFont defaultFont{ "Arial", 24, QFont::Bold };


  // A custom text widget with syntax highlighting for GLSL 
  GlslEditor shaderTextWidget;
  QTextEdit filenameTextWidget;

  shadertoy::Shader activeShader;
  QDir configPath;
  // The various UI 'panes'
  QGraphicsProxyWidget * uiEditDialog;
  QGraphicsProxyWidget * uiChannelDialog;
  QGraphicsProxyWidget * uiLoadDialog;
  QGraphicsProxyWidget * uiSaveDialog;

  // Holds the most recently captured UI image
  std::map<QUrl, QIcon> iconCache;
  QPixmap currentWindowImage;
  int activePresetIndex{ 0 };
  int activeChannelIndex{ 0 };

  enum UiState {
    INACTIVE,
    EDIT,
    CHANNEL,
    LOAD,
    SAVE,
  };

  void initIconCache() {
    using namespace shadertoy;
    for (int i = 0; i < MAX_TEXTURES; ++i) {
      Resource res = TEXTURES[i];
      if (NO_RESOURCE == res) {
        continue;
      }
      QIcon icon = QIcon(QPixmap::fromImage(oria::qt::loadImageResource(res).scaled(QSize(128, 128))));
      iconCache[QUrl("preset://tex/" + i)] = icon;
      iconCache[QUrl(QString().sprintf("preset://tex/%02d", i))] = icon;
    }
    for (int i = 0; i < MAX_CUBEMAPS; ++i) {
      Resource res = CUBEMAPS[i];
      if (NO_RESOURCE == res) {
        continue;
      }
      QIcon icon = QIcon(QPixmap::fromImage(oria::qt::loadImageResource(res).scaled(QSize(128, 128))));
      iconCache[QUrl("preset://cube/" + i)] = icon;
      iconCache[QUrl(QString().sprintf("preset://cube/%02d", i))] = icon;
    }
    iconCache[QUrl()] = QIcon();
  }

  QIcon loadIcon(const QUrl & url) {
    qDebug() << "Looking for icon for URL " << url;
    if (!iconCache.count(url)) {
      qDebug() << "Existing icon not found, opening URL to create a new one";
      QImage image;
      image.load(url.toLocalFile());
      iconCache[url] = QIcon(QPixmap::fromImage(image.scaled(QSize(128, 128))));
    }
    return iconCache[url];
  }

  QToolButton * makeImageButton(const QUrl & url = QUrl(), const QSize & size = QSize(128, 128)) {
    QToolButton  * button = new QToolButton();
    button->resize(size);
    button->setAutoFillBackground(true);
    if (iconCache.count(url)) {
      button->setIcon(iconCache[url]);
    }
    button->setIconSize(size);
    return button;
  }

  void setUiState(UiState state) {
    uiChannelDialog->hide();
    uiEditDialog->hide();
    uiLoadDialog->hide();
    uiSaveDialog->hide();

    switch (state) {
    case EDIT:
      uiEditDialog->show();
      shaderTextWidget.setFocus();
      break;
    case CHANNEL:
      uiChannelDialog->show();
      break;
    case LOAD:
      emit refreshUserShaders();
      uiLoadDialog->show();
      break;
    case SAVE:
      uiSaveDialog->show();
      filenameTextWidget.setText(activeShader.name);
      filenameTextWidget.setFocus();
      break;
    }
  }

  // FIXME add status like texture res scale, eye pos scale and ... stuff to the edit window
  // oh yeah, 'eye per frame' mode indicator
  void setupShaderEditor() {
    QWidget & shaderEditor = *(new QWidget());
    shaderEditor.resize(UI_SIZE.x, UI_SIZE.y);
    shaderEditor.setLayout(new QHBoxLayout());
    {
      QWidget * pButtonCol = new QWidget();
      pButtonCol->setLayout(new QVBoxLayout());

      for (int i = 0; i < shadertoy::MAX_CHANNELS; ++i) {
        QToolButton  * button = makeImageButton();
        connect(button, &QToolButton::clicked, this, [&, i] {
          // Start the channel selection dialog, record what channel we're modifying
          activeChannelIndex = i;
          setUiState(CHANNEL);
        });
        connect(this, &ShadertoyApp::channelTextureChanged, button, [=](int channel, shadertoy::ChannelInputType type, const QUrl & textureSource) {
          if (i == channel) {
            button->setIcon(loadIcon(textureSource));
            activeShader.channelTypes[channel] = type;
            activeShader.channelTextures[channel] = textureSource;
          }
        });
        connect(this, &ShadertoyApp::shaderLoaded, button, [=](const shadertoy::Shader & shader) {
          button->setIcon(loadIcon(shader.channelTextures[i]));
        });
        pButtonCol->layout()->addWidget(button);
      }
      shaderEditor.layout()->addWidget(pButtonCol);
    }

    {
      QWidget * pEditColumn = new QWidget();
      pEditColumn->setLayout(new QVBoxLayout());
      pEditColumn->layout()->addWidget(&shaderTextWidget);

      {
        QTextEdit * pStatus = new QTextEdit();
        pStatus->setMaximumHeight(48);
        pStatus->setFont(QFont("Courier", 18));
        pStatus->setEnabled(false);

        // Signals emitted from the Rift widget rendering thread are processed 
        // on that thread, so we need to queue them for processing on the main 
        // thread
        connect(&riftRenderWidget, &ShadertoyRiftWidget::compileError, this, [=](QString string) {
          tasks.queueTask([=] {
            pStatus->setText(string);
          });
        }, Qt::DirectConnection);
        connect(&riftRenderWidget, &ShadertoyRiftWidget::compileSuccess, this, [=]() {
          tasks.queueTask([=] {
            pStatus->setText("Success");
          });
        }, Qt::DirectConnection);

        pEditColumn->layout()->addWidget(pStatus);
      }


      {
        QWidget * pButtonRow = new QWidget();
        QHBoxLayout * pButtonLayout = new QHBoxLayout();
        pButtonRow->setLayout(pButtonLayout);
        {
          QPushButton * pRunButton = new QPushButton("Run");
          pRunButton->setFont(defaultFont);
          connect(pRunButton, &QPushButton::clicked, this, [&] {
            emit shaderSourceChanged(shaderTextWidget.toPlainText());
          });
          pButtonLayout->addWidget(pRunButton);
        }
        pButtonLayout->addStretch();

        {
          QPushButton * pLoadButton = new QPushButton("Load");
          pLoadButton->setFont(defaultFont);
          connect(pLoadButton, &QPushButton::clicked, this, [&] {
            setUiState(LOAD);
          });
          pButtonLayout->addWidget(pLoadButton);
          pButtonLayout->setAlignment(pLoadButton, Qt::AlignRight);
        }

        {
          QPushButton * pSaveButton = new QPushButton("Save");
          pSaveButton->setFont(defaultFont);
          connect(pSaveButton, &QPushButton::clicked, this, [&] {
            setUiState(SAVE);
          });
          pButtonLayout->addWidget(pSaveButton);
          pButtonLayout->setAlignment(pSaveButton, Qt::AlignRight);
        }

        //{
        //  QPushButton * pLoadButton = new QPushButton("Toggle Persistence");
        //  pLoadButton->setFont(defaultFont);
        //  connect(pLoadButton, &QPushButton::clicked, this, [&] {
        //    emit toggleOvrFlag(ovrHmdCap_LowPersistence);
        //  });
        //  pButtonRow->layout()->addWidget(pLoadButton);
        //  pButtonRow->layout()->setAlignment(pLoadButton, Qt::AlignRight);
        //}

        //{
        //  QPushButton * pLoadButton = new QPushButton("Toggle V-Sync");
        //  pLoadButton->setFont(defaultFont);
        //  connect(pLoadButton, &QPushButton::clicked, this, [&] {
        //    emit toggleOvrFlag(ovrHmdCap_NoVSync);
        //  });
        //  pButtonRow->layout()->addWidget(pLoadButton);
        //  pButtonRow->layout()->setAlignment(pLoadButton, Qt::AlignRight);
        //}

        pEditColumn->layout()->addWidget(pButtonRow);
      }

      shaderEditor.layout()->addWidget(pEditColumn);
    }
    uiEditDialog = uiScene.addWidget(&shaderEditor);
  }

  void setupChannelSelector() {
    QWidget & channelSelector = *(new QWidget());
    channelSelector.resize(UI_SIZE.x, UI_SIZE.y);
    channelSelector.setLayout(new QVBoxLayout());
    {
      QLabel * label = new QLabel("Textures");
      label->setFont(defaultFont);
      channelSelector.layout()->addWidget(label);

      QWidget * pButtonRow = new QWidget();
      pButtonRow->setLayout(new QHBoxLayout());
      pButtonRow->layout()->setAlignment(Qt::AlignLeft);

      int buttonCount = 0;
      for (int i = 0; i < shadertoy::MAX_TEXTURES; ++i) {
        if (shadertoy::TEXTURES[i] == NO_RESOURCE) {
          continue;
        }

        if (++buttonCount > 8) {
          channelSelector.layout()->addWidget(pButtonRow);
          pButtonRow = new QWidget();
          pButtonRow->setLayout(new QHBoxLayout());
          pButtonRow->layout()->setAlignment(Qt::AlignLeft);
          buttonCount = 0;
        };

        QUrl url = QUrl("preset://tex/" + i);
        QToolButton  * button = makeImageButton(url);
        connect(button, &QToolButton::clicked, this, [&, url] {
          emit channelTextureChanged(activeChannelIndex, shadertoy::ChannelInputType::TEXTURE, url);
          setUiState(EDIT);
        });
        pButtonRow->layout()->addWidget(button);
      }
      channelSelector.layout()->addWidget(pButtonRow);
    }

    {
      QLabel * label = new QLabel("Cubemaps");
      label->setFont(defaultFont);
      channelSelector.layout()->addWidget(label);

      QWidget * pButtonRow = new QWidget();
      pButtonRow->setLayout(new QHBoxLayout());
      pButtonRow->layout()->setAlignment(Qt::AlignLeft);

      int buttonCount = 0;
      for (int i = 0; i < shadertoy::MAX_CUBEMAPS; ++i) {
        if (shadertoy::CUBEMAPS[i] == NO_RESOURCE) {
          continue;
        }

        if (++buttonCount > 8) {
          channelSelector.layout()->addWidget(pButtonRow);
          pButtonRow = new QWidget();
          pButtonRow->setLayout(new QHBoxLayout());
          buttonCount = 0;
        };

        QUrl url = QUrl("preset://cube/" + i);
        QToolButton  * button = makeImageButton(url);
        connect(button, &QToolButton::clicked, this, [&, url] {
          emit channelTextureChanged(activeChannelIndex, shadertoy::ChannelInputType::CUBEMAP, url);
          setUiState(EDIT);
        });
        pButtonRow->layout()->addWidget(button);
      }
      channelSelector.layout()->addWidget(pButtonRow);
    }

    {
      QWidget * pButtonRow = new QWidget();
      pButtonRow->setLayout(new QHBoxLayout());
      pButtonRow->layout()->setAlignment(Qt::AlignLeft);
      {
        QPushButton * pSaveButton = new QPushButton("Load");
        pSaveButton->setFont(defaultFont);
        connect(pSaveButton, &QPushButton::clicked, this, [&, this] {
          QFileDialog * fileDialog = new QFileDialog();
          fileDialog->setNameFilters(QStringList() << "Image files (*.png *.jpg)");
          fileDialog->show();
          fileDialog->move(0, 0);
          fileDialog->resize(UI_SIZE.x, UI_SIZE.y);
          fileDialog->hide();
          fileDialog->setDirectory(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
          QGraphicsProxyWidget * uiOpenDialog = uiScene.addWidget(fileDialog);
          uiChannelDialog->hide();
          uiOpenDialog->show();
          connect(fileDialog, &QFileDialog::fileSelected, this, [&, this](const QString& file) {
            QUrl fileUrl = QUrl::fromLocalFile(file);
            // FIXME
            emit channelTextureChanged(activeChannelIndex, shadertoy::ChannelInputType::TEXTURE, fileUrl);
            setUiState(EDIT);
          });
        });
        pButtonRow->layout()->addWidget(pSaveButton);
        pButtonRow->layout()->setAlignment(pSaveButton, Qt::AlignRight);
      }
      channelSelector.layout()->addWidget(pButtonRow);
    }

    uiChannelDialog = uiScene.addWidget(&channelSelector);
    uiChannelDialog->hide();

  }

  QLabel * makeLabel(const QString & label) {
    QLabel * pLabel = new QLabel(label);
    pLabel->setFont(defaultFont);
    pLabel->setMaximumHeight(48);
    return pLabel;
  }

  void setupLoad() {
    QWidget & loadDialog = *(new QWidget());
    loadDialog.resize(UI_SIZE.x, UI_SIZE.y);
    loadDialog.setLayout(new QHBoxLayout());
    {
      QWidget * pPresetHolder = new QWidget();
      loadDialog.layout()->addWidget(pPresetHolder);

      pPresetHolder->setLayout(new QVBoxLayout());
      pPresetHolder->layout()->addWidget(makeLabel("Presets"));

      QListWidget * pPresets = new QListWidget();
      pPresets->setFont(defaultFont);

      for (int i = 0; i < shadertoy::MAX_PRESETS; ++i) {
        shadertoy::Preset & preset = shadertoy::PRESETS[i];
        QListWidgetItem * pItem = new QListWidgetItem(preset.name);
        pItem->setData(Qt::UserRole, (int)i);
        pPresets->addItem(pItem);
      }

      connect(pPresets, &QListWidget::itemClicked, this, [&](QListWidgetItem* item) {
        int presetIndex = item->data(Qt::UserRole).toInt();
        loadPreset(shadertoy::PRESETS[presetIndex]);
        setUiState(EDIT);
      });
      pPresetHolder->layout()->addWidget(pPresets);
    }
    {
      QWidget * pUserShaderHolder = new QWidget();
      pUserShaderHolder->setLayout(new QVBoxLayout());
      pUserShaderHolder->layout()->addWidget(makeLabel("User Shaders"));
      loadDialog.layout()->addWidget(pUserShaderHolder);

      QListWidget * pUserShaders = new QListWidget();
      pUserShaders->setFont(defaultFont);
      connect(this, &ShadertoyApp::refreshUserShaders, pUserShaders, [=]() {
        pUserShaders->clear();
        QDir shaderDir = QDir(configPath.absoluteFilePath("shaders"));
        auto files = shaderDir.entryList(QStringList("*.xml"), QDir::Files | QDir::NoSymLinks);
        qDebug() << files.count();
        foreach(QString file, files) {
          QRegExp re(".*([^/]+).xml");
          if (!re.exactMatch(file)) {
            continue;
          }
          QString name = re.cap(1);
          QListWidgetItem * pItem = new QListWidgetItem(name);
          pItem->setData(Qt::UserRole, shaderDir.absoluteFilePath(file));
          pUserShaders->addItem(pItem);
        }
      });
      connect(pUserShaders, &QListWidget::itemClicked, this, [&](QListWidgetItem* item) {
        QString file = item->data(Qt::UserRole).toString();
        qDebug() << "Loading file: " << file;
        loadFile(file);
        setUiState(EDIT);
      });
      pUserShaderHolder->layout()->addWidget(pUserShaders);
    }
    uiLoadDialog = uiScene.addWidget(&loadDialog);
    uiLoadDialog->hide();
  }

  void setupSave() {
    QWidget & saveDialog = *(new QWidget());
    saveDialog.resize(UI_SIZE.x, UI_SIZE.y);
    QFormLayout * formLayout = new QFormLayout();
    saveDialog.setLayout(formLayout);
    filenameTextWidget.setFont(defaultFont);
    filenameTextWidget.setMaximumHeight(48);
    formLayout->addRow("Name", &filenameTextWidget);
    formLayout->labelForField(&filenameTextWidget)->setFont(defaultFont);

    QTextEdit * pStatus = new QTextEdit();
    pStatus->setMaximumHeight(64);
    pStatus->setFont(defaultFont);
    pStatus->setEnabled(false);
    formLayout->addRow(pStatus);

    {
      QWidget * pButtonRow = new QWidget();
      QHBoxLayout * pButtonRowLayout = new QHBoxLayout();
      pButtonRow->setLayout(pButtonRowLayout);
      {
        QPushButton * pButton = new QPushButton("Cancel");
        pButton->setFont(defaultFont);
        connect(pButton, &QPushButton::clicked, this, [&] {
          setUiState(EDIT);
        });
        pButtonRow->layout()->addWidget(pButton);
        pButtonRow->layout()->setAlignment(pButton, Qt::AlignCenter);
      }
      {
        QPushButton * pButton = new QPushButton("Save");
        pButton->setFont(defaultFont);
        connect(pButton, &QPushButton::clicked, this, [&, pStatus] {
          activeShader.name = filenameTextWidget.toPlainText();
          activeShader.fragmentSource = shaderTextWidget.toPlainText();
          activeShader.saveFile(
            configPath.absoluteFilePath(QString("shaders/") 
              + activeShader.name
              + ".xml"));
          setUiState(EDIT);
        });
        pButtonRow->layout()->addWidget(pButton);
        pButtonRow->layout()->setAlignment(pButton, Qt::AlignCenter);
      }
      formLayout->addRow(pButtonRow);
    }


    uiSaveDialog = uiScene.addWidget(&saveDialog);
    uiSaveDialog->hide();
  }

  void setupDesktopWindow() {
    desktopWindow.setLayout(new QFormLayout());
    QLabel * label = new QLabel("Your Oculus Rift is now active.  Please put on your headset.  Share and enjoy");
    desktopWindow.layout()->addWidget(label);
    desktopWindow.show();
  }
  
  void loadPreset(shadertoy::Preset & preset) {
    for (int i = 0; i < shadertoy::MAX_PRESETS; ++i) {
      if (shadertoy::PRESETS[i].res == preset.res) {
        activePresetIndex = i;
      }
    }

    activeShader = shadertoy::Shader(preset.res);
    assert(!activeShader.fragmentSource.isEmpty());
    shaderTextWidget.setPlainText(activeShader.fragmentSource);
    emit shaderLoaded(activeShader);
  }

  void loadFile(const QString & file) {
    activeShader = shadertoy::Shader(file);
    assert(!activeShader.fragmentSource.isEmpty());
    shaderTextWidget.setPlainText(activeShader.fragmentSource);
    emit shaderLoaded(activeShader);
  }

public:
  ShadertoyApp(int argc, char ** argv) :
    QApplication(argc, argv),
    riftRenderWidget(QRiftWidget::getFormat()),
    uiGlWidget(QRiftWidget::getFormat(), 0, &riftRenderWidget),
    timer(this) { 

    QCoreApplication::setOrganizationName(ORG_NAME);
    QCoreApplication::setOrganizationDomain(ORG_DOMAIN);
    QCoreApplication::setApplicationName(APP_NAME);

    QString configLocation = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    configPath = QDir(configLocation);
    configPath.mkpath("shaders");

    initIconCache();

    // Set up the UI renderer
    uiView.setViewport(new QWidget());
    uiView.setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    uiView.setScene(&uiScene);
    uiView.resize(UI_SIZE.x, UI_SIZE.y);

    // Set up the various UI dialogs shown in the VR view
    setupChannelSelector();
    setupShaderEditor();
    setupLoad();
    setupSave();

    // Setup the desktop UI window
    setupDesktopWindow();

    // UI toggle needs to intercept keyboard events before anything else.
    riftRenderWidget.installEventFilter(this);

    // When we get the 'recompile key' do the same thing as the 'Run' button in the shader editor
    connect(&riftRenderWidget, &ShadertoyRiftWidget::recompileShader, this, [&]() {
      emit shaderSourceChanged(shaderTextWidget.toPlainText());
    });
    // Both the rift render widget and the app need to change behaviour based on whether the UI is visible
    connect(&riftRenderWidget, &ShadertoyRiftWidget::uiVisibleChanged, this, [&](bool uiVisible) {
      toggleUi(uiVisible);
    });

    // Hotkeys for next/previous shader
    // FIXME add confirmation for when the user might lose edits.
    connect(&riftRenderWidget, &ShadertoyRiftWidget::loadNextPreset, this, [&]() {
      int newPreset = (activePresetIndex + 1) % shadertoy::MAX_PRESETS;
      loadPreset(shadertoy::PRESETS[newPreset]);
    });
    connect(&riftRenderWidget, &ShadertoyRiftWidget::loadPreviousPreset, this, [&]() {
      int newPreset = (activePresetIndex + shadertoy::MAX_PRESETS - 1) % shadertoy::MAX_PRESETS;
      loadPreset(shadertoy::PRESETS[newPreset]);
    });
    connect(&riftRenderWidget, &ShadertoyRiftWidget::shutdown, this, [&]() {
      riftRenderWidget.stop();
      QApplication::instance()->quit();
    });
    connect(&uiScene, &QGraphicsScene::changed, this, [&](const QList<QRectF> &region) {
      currentWindowImage = uiView.grab();
      uiTextureRefresh();
    });
    connect(&riftRenderWidget, &ShadertoyRiftWidget::mouseMoved, this, [&](QPointF pos) {
      uiTextureRefresh();
    });

    

    // Connect the UI to the rendering system so that when the a channel texture or the shader source is changed 
    // we reflect it in rendering
    connect(this, &ShadertoyApp::channelTextureChanged, &riftRenderWidget, &ShadertoyRiftWidget::setChannelTexture);
    connect(this, &ShadertoyApp::shaderSourceChanged, &riftRenderWidget, &ShadertoyRiftWidget::setShaderSource);
    connect(this, &ShadertoyApp::shaderLoaded, &riftRenderWidget, &ShadertoyRiftWidget::setShader);

    riftRenderWidget.start();

    loadPreset(shadertoy::PRESETS[0]);

    // We need mouse tracking because the GL window is a proxies mouse 
    // events for actual UI objects that respond to mouse move / hover
    riftRenderWidget.setMouseTracking(true);
    shaderTextWidget.setFocus();

    foreach(QGraphicsItem *item, uiScene.items()) {
      item->setFlag(QGraphicsItem::ItemIsMovable);
      item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }

    
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    timer.start(150);
  }

  virtual ~ShadertoyApp() {
  }


private slots:
  void toggleUi(bool uiVisible) {
    this->uiVisible = uiVisible;
    if (uiVisible) {
      uiView.install(&riftRenderWidget);
      uiTextureRefresh();
    } else {
      uiChannelDialog->hide();
      uiEditDialog->show();
      uiView.remove(&riftRenderWidget);
    }
  }

  void mouseRefresh(QPixmap & currentWindowWithMouseImage) {
    QCursor cursor = riftRenderWidget.cursor();
    QPoint position = cursor.pos();
    // Fetching the cursor pixmap isn't working... 
    static QPixmap cursorPm = oria::qt::loadXpmResource(Resource::MISC_CURSOR_XPM);

    QPointF relative = riftRenderWidget.mapFromGlobal(position);
    relative.rx() /= riftRenderWidget.size().width();
    relative.ry() /= riftRenderWidget.size().height();
    relative.rx() *= uiView.size().width();
    relative.ry() *= uiView.size().height();
    currentWindowWithMouseImage = currentWindowImage;
    // Draw the mouse pointer on the window
    QPainter qp;
    qp.begin(&currentWindowWithMouseImage);
    qp.drawPixmap(relative, cursorPm);
    qp.end();
  }

  void uiTextureRefresh() {
    if (!uiVisible) {
      return;
    }

    uiGlWidget.makeCurrent();
    Texture::Active(0);
    QPixmap currentWindowWithMouseImage; 
    mouseRefresh(currentWindowWithMouseImage);
    GLuint newTexture = uiGlWidget.bindTexture(currentWindowWithMouseImage);
    if (newTexture) {
      // Is this needed?
      glBindTexture(GL_TEXTURE_2D, newTexture);
      GLenum err = glGetError();
      Texture::MagFilter(Texture::Target::_2D, TextureMagFilter::Nearest);
      Texture::MinFilter(Texture::Target::_2D, TextureMinFilter::Nearest);
      glBindTexture(GL_TEXTURE_2D, 0);
      glFlush();

      GLuint oldTexture = riftRenderWidget.exchangeUiTexture(newTexture);
      // If we get a non-0 value back it means the drawing thread never 
      // used this texture, so we can delete it right away
      if (oldTexture) {
        glDeleteTextures(1, &oldTexture);
      }
    }
    uiGlWidget.doneCurrent();
  }

  void onTimer() {
    tasks.drainTaskQueue();
    if (!uiVisible) {
      return;
    }


//    currentWindowImage = uiView.grab();
//    uiMouseOnlyRefresh();
  }

signals:
  void shaderSourceChanged(QString str);
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
void onChannelTextureChanged(int channel, shadertoy::ChannelInputType type, int index) {
  tasks.queueTask([&, channel, type, index] {
    SAY("Emitting CHANNEL TEXTURE CHANGED %d %d %d", channel, type, index);
    emit channelTextureChanged(channel, type, index);
  });
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
