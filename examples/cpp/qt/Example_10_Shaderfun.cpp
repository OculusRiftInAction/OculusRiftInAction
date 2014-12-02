#include "Common.h"

#include <QtOpenGL/QGLContext>
#include <QtOpenGL/QGLFunctions>

#include <QtWidgets>
#include <QQuickView>
#include <QQuickItem>
#include <QQuickImageProvider>
#include <QtQuickWidgets/QQuickWidget>

#include <oglplus/error/basic.hpp>
#include <oglplus/shapes/plane.hpp>
#include <regex>
#include <queue>
#include <mutex>
#include <set>
#include <atomic>         // std::atomic
#include <thread>

#include "Shadertoy.h"
#include "Cursor.xpm"

using namespace oglplus;

#define UI_X 1280
#define UI_Y 720

#ifdef _DEBUG
#define BRAD_DEBUG 1
#endif

static const float ROOT_2 = sqrt(2.0f);
static const float INV_ROOT_2 = 1.0f / ROOT_2;
static uvec2 UI_SIZE(UI_X, UI_Y);

static QImage loadImageResource(Resource res) {
  std::vector<uint8_t> data = Platform::getResourceByteVector(res);
  QImage image;
  image.loadFromData(data.data(), data.size());
  return image;
}

typedef std::map<Resource, QIcon> IconMap;

IconMap iconCache;

static void initIconCache() {
  using namespace shadertoy;
  for (int i = 0; i < MAX_TEXTURES; ++i) {
    Resource res = TEXTURES[i];
    if (NO_RESOURCE == res) {
      continue;
    }
    iconCache[res] = QIcon(QPixmap::fromImage(loadImageResource(res).scaled(QSize(128, 128))));
  }
  for (int i = 0; i < MAX_CUBEMAPS; ++i) {
    Resource res = CUBEMAPS[i];
    if (NO_RESOURCE == res) {
      continue;
    }
    iconCache[res] = QIcon(QPixmap::fromImage(loadImageResource(res).scaled(QSize(128, 128))));
  }
  iconCache[NO_RESOURCE] = QIcon();
}


static QToolButton * makeImageButton(Resource res = NO_RESOURCE, const QSize & size = QSize(128, 128)) {
  if (!iconCache.size()) {
    initIconCache();
  }
  QToolButton  * button = new QToolButton();
  button->resize(size);
  button->setAutoFillBackground(true);
  if (res != NO_RESOURCE) {
    button->setIcon(iconCache[res]);
  }
  button->setIconSize(size);
  return button;
}

void viewport(const uvec2 & size) {
  oglplus::Context::Viewport(0, 0, size.x, size.y);
}

std::string replaceAll(const std::string & input, const std::string & find, const std::string & replace) {
  std::string result = input;
  std::string::size_type find_size = find.size();
  std::string::size_type n = 0;
  while (std::string::npos != (n = result.find(find, n))) {
    result.replace(n, find_size, replace);
  }
  return result;
}

std::vector<std::string> splitLines(const std::string & str) {
  std::stringstream ss(str);
  std::string line;
  std::vector<std::string> result;
  while (std::getline(ss, line, '\n')) {
    result.push_back(line);
  }
  return result;
}

struct RateCounter {
  unsigned int count{ 0 };
  float start{ -1 };

  void reset() {
    count = 0;
    start = -1;
  }

  unsigned int getCount() const {
    return count;
  }

  void increment() {
    if (start < 0) {
      start = Platform::elapsedSeconds();
    } else {
      ++count;
    }
  }

  float getRate() const {
    float elapsed = Platform::elapsedSeconds() - start;
    return (float)count / elapsed;
  }
};

std::map<std::string, GLuint> getActiveUniforms(ProgramPtr & program) {
  std::map<std::string, GLuint> activeUniforms;
  size_t uniformCount = program->ActiveUniforms().Size();
  for (size_t i = 0; i < uniformCount; ++i) {
    std::string name = program->ActiveUniforms().At(i).Name().c_str();
    activeUniforms[name] = program->ActiveUniforms().At(i).Index();
  }
  return activeUniforms;
}

class TaskQueueWrapper {
  typedef std::mutex Mutex;
  typedef std::unique_lock<Mutex> Locker;
  typedef std::function<void()> Functor;
  typedef std::queue<Functor> TaskQueue;

  TaskQueue queue;
  Mutex mutex;

public:
  void drainTaskQueue() {
    TaskQueue copy;
    {
      Locker lock(mutex);
      std::swap(copy, queue);
    }
    while (!copy.empty()) {
      copy.front()();
      copy.pop();
    }
  }

  void queueTask(Functor task) {
    Locker locker(mutex);
    queue.push(task);
  }

};

struct Channel {
  TexturePtr texture;
  oglplus::Texture::Target target;
  vec3 resolution;
  Resource resource{ NO_RESOURCE };
};

enum UiState {
  INACTIVE,
  EDIT,
  CHANNEL,
  LOAD,
  SAVE,
};

UiState activeState = INACTIVE;
bool uiVisible{ false };
std::string currentFragmentSource;

typedef std::atomic<GLuint> AtomicGlTexture;
AtomicGlTexture uiTexture{ 0 };

class LambdaThread : public QThread {
  std::function<void()> f;

  void run() {
    f();
  }

public:
  template <typename F>
  LambdaThread(F f) : f(f) {
  }
};

class ShadertoyRiftWidget : public QRiftWidget {
  Q_OBJECT
  typedef std::shared_ptr<oglplus::VertexShader> VertexShaderPtr;
  typedef std::shared_ptr<oglplus::FragmentShader> FragmentShaderPtr;
  typedef std::pair<shadertoy::ChannelInputType, int> ChannelPair;
  typedef std::vector<ChannelPair> ChannelVector;

  struct TextureData {
    TexturePtr tex;
    uvec2 size;
  };
  typedef std::map<Resource, TextureData> TextureMap;

  bool shuttingDown{ false };
  LambdaThread renderThread;

  TextureMap textureCache;

  // GLSL and geometry for the shader skybox
  ProgramPtr skyboxProgram;
  ShapeWrapperPtr skybox;
  VertexShaderPtr vertexShader;
  FragmentShaderPtr fragmentShader;
  // We actually render the shader to one FBO for dynamic framebuffer scaling,
  // while leaving the actual texture we pass to the Oculus SDK fixed.
  // This allows us to have a clear UI regardless of the shader performance
  FramebufferWrapperPtr shaderFramebuffer;

  Channel channels[4];
  float texRes{ 1.0f };

  TaskQueueWrapper tasks;
  ProgramPtr uiProgram;
  ShapeWrapperPtr uiShape;

  // GLSL and geometry for the UI
  ProgramPtr planeProgram;
  ShapeWrapperPtr plane;

  // Measure the FPS for use in dynamic scaling
  RateCounter fps;

  // The current fragment source
  std::list<std::function<void()>> uniformLambdas;


  void initTextureCache() {
    using namespace shadertoy;
    for (int i = 0; i < MAX_TEXTURES; ++i) {
      Resource res = TEXTURES[i];
      if (NO_RESOURCE == res) {
        continue;
      }
      TextureData & tex = textureCache[res];
      tex.tex = oria::load2dTexture(res, tex.size);
    }
    for (int i = 0; i < MAX_CUBEMAPS; ++i) {
      Resource res = CUBEMAPS[i];
      if (NO_RESOURCE == res) {
        continue;
      }
      TextureData & tex = textureCache[res];
      static int resourceOrder[] = {
        0, 1, 2, 3, 4, 5
      };
      tex.tex = oria::loadCubemapTexture(res, resourceOrder, false);
    }
  }

  void run() {
    glewExperimental = true;
    glewInit();
    context()->makeCurrent();
    setupRiftRendering();
    initTextureCache();

    QCoreApplication* app = QCoreApplication::instance();
    while (!shuttingDown) {
      // Process the Qt message pump to run the standard window controls
      if (app->hasPendingEvents())
        app->processEvents();
      tasks.drainTaskQueue();
      if (isRenderingConfigured()) {
        draw();
      } else {
        QThread::msleep(4);
      }
    }
    context()->doneCurrent();
    context()->moveToThread(QApplication::instance()->thread());
  }
public:
  explicit ShadertoyRiftWidget(QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0)
    : QRiftWidget(parent, shareWidget, f), renderThread([&]{run();}) {
  }

  explicit ShadertoyRiftWidget(QGLContext *context, QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0)
    : QRiftWidget(context, parent, shareWidget, f), renderThread([&] {run(); }) {
  }

  explicit ShadertoyRiftWidget(const QGLFormat& format, QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0)
    : QRiftWidget(format, parent, shareWidget, f), renderThread([&] {run(); }) {
  }

  QThread & getRenderThread() {
    return renderThread;
  }

  virtual ~ShadertoyRiftWidget() {
    shuttingDown = true;
    renderThread.exit();
    renderThread.wait();
    context()->makeCurrent();
  }
  void startRenderThread() {
    context()->doneCurrent();
    context()->moveToThread(&renderThread);
    renderThread.start();
    renderThread.setPriority(QThread::HighestPriority);
    QThread::currentThread()->setPriority(QThread::LowPriority);
  }

  int currentPresetIndex = 0;

protected:
  vec2 textureSize() {
    return vec2(ovr::toGlm(eyeTextures[0].Header.TextureSize));
  }

  uvec2 renderSize() {
    return uvec2(texRes * textureSize());
  }

  void renderSkybox() {
    using namespace oglplus;
    shaderFramebuffer->Bound([&] {
      viewport(renderSize());
      skyboxProgram->Bind();
      MatrixStack & mv = Stacks::modelview();
      mv.withPush([&] {
        mv.untranslate();
        oria::renderGeometry(skybox, skyboxProgram, uniformLambdas);
      });
      for (int i = 0; i < 4; ++i) {
        oglplus::DefaultTexture().Active(0);
        DefaultTexture().Bind(Texture::Target::_2D);
        DefaultTexture().Bind(Texture::Target::CubeMap);
      }
      oglplus::Texture::Active(0);
    });
    viewport(textureSize());
  }

  void renderScene() {
    glGetError();
    viewport(textureSize());
    Context::Disable(Capability::Blend);
    Context::Disable(Capability::ScissorTest);
    Context::Disable(Capability::DepthTest);
    Context::Disable(Capability::CullFace);
    Context::Clear().DepthBuffer().ColorBuffer();
    renderSkybox();

    MatrixStack & mv = Stacks::modelview();
    MatrixStack & pr = Stacks::projection();
    Stacks::withPush([&] {
      pr.identity();
      mv.identity();
      shaderFramebuffer->color.Bind(Texture::Target::_2D);
      oria::renderGeometry(plane, planeProgram, { [&] {
        Uniform<vec2>(*planeProgram, "UvMultiplier").Set(vec2(texRes));
      } });
    });

    typedef std::pair<GLuint, GLsync> SyncPair;
    typedef std::queue<SyncPair> Trashcan;
    static Trashcan uiTextureTrash;

    if (uiVisible) {
      static SyncPair lastUiTextureData;
      GLuint currentUiTexture = uiTexture.exchange(0);
      if (0 == currentUiTexture) {
        currentUiTexture = lastUiTextureData.first;
      } else {
        // If the texture has changed, push it into the trash bin for 
        // deletion once it's finished rendering
        if (lastUiTextureData.first) {
          uiTextureTrash.push(lastUiTextureData);
        }
        lastUiTextureData.first = currentUiTexture;
      }

      if (currentUiTexture) {
        mv.withPush([&] {
          mv.translate(vec3(0, 0, -1));
          Texture::Active(0);
          glBindTexture(GL_TEXTURE_2D, currentUiTexture);
          oria::renderGeometry(uiShape, uiProgram);
          lastUiTextureData.second = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        });
      }
    }

    // Clean up used textures that have completed rendering
    while (uiTextureTrash.size()) {
      SyncPair syncPair = uiTextureTrash.front();
      GLsizei outLength;
      GLint outResult;
      glGetSynciv(syncPair.second, GL_SYNC_STATUS, 1, &outLength, &outResult);
      if (outResult != GL_SIGNALED) {
        break;
      }
      glDeleteTextures(1, &syncPair.first);
      uiTextureTrash.pop();
    }
  }

  void keyPressEvent(QKeyEvent * event) {
    int key = event->key();

    ovrHSWDisplayState hswState;
    ovrHmd_GetHSWDisplayState(hmd, &hswState);
    if (hswState.Displayed) {
      ovrHmd_DismissHSWDisplay(hmd);
      return;
    }

    // These functions have to be done on the Rift rendering thread
    tasks.queueTask([&, key] {
      switch (key) {
      case Qt::Key_F2:
        ovrHmd_RecenterPose(hmd);
        break;
      case Qt::Key_F5:
        texRes = std::max(texRes * INV_ROOT_2, 0.05f);
        break;
      case Qt::Key_F6:
        texRes = std::max(texRes * 0.95f, 0.05f);
        break;
      case Qt::Key_F7:
        texRes = std::min(texRes * 1.05f, 1.0f);
        break;
      case Qt::Key_F8:
        texRes = std::min(texRes * ROOT_2, 1.0f);
        break;
      //case Qt::Key_F9:
      //  onPreviousPreset();
      //  break;
      //case Qt::Key_F10:
      //  onNextPreset();
      //  break;
      }
    });
  }

  virtual void initGl() {
    QRiftWidget::initGl();
    uiProgram = oria::loadProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_TEXTURED_FS);
    uiShape = oria::loadPlane(uiProgram, (float)UI_X / (float)UI_Y);

    planeProgram = oria::loadProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_TEXTURED_FS);
    plane = oria::loadPlane(planeProgram, 1.0);

    setShaderSourceInternal(qt::toString(Resource::SHADERTOY_SHADERS_DEFAULT_FS));
    assert(skyboxProgram);

    skybox = oria::loadSkybox(skyboxProgram);
    shaderFramebuffer = FramebufferWrapperPtr(new FramebufferWrapper());
    shaderFramebuffer->init(ovr::toGlm(eyeTextures[0].Header.TextureSize));
    DefaultFramebuffer().Bind(Framebuffer::Target::Draw);
  }

  virtual void setShaderSourceInternal(QString source) {
    try {
      if (!vertexShader) {
        vertexShader = VertexShaderPtr(new VertexShader());
        vertexShader->Source(Platform::getResourceData(Resource::SHADERTOY_SHADERS_DEFAULT_VS));
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
      newFragmentShader->Source(GLSLSource(source.toLocal8Bit().data()));
      newFragmentShader->Compile();
      ProgramPtr result(new Program());
      result->AttachShader(*vertexShader);
      result->AttachShader(*newFragmentShader);

      result->Link();
      skyboxProgram.swap(result);
      fragmentShader.swap(newFragmentShader);
      updateUniforms();
      emit compileSuccess();
    } catch (ProgramBuildError & err) {
      emit compileError(QString(err.Log().c_str()));
    }
  }

  virtual void setChannelTextureInternal(int channel, shadertoy::ChannelInputType type, int index) {
    using namespace oglplus;
    Channel & channelRef = channels[channel];
    Resource res = shadertoy::getChannelInputResource(type, index);
    if (res == channelRef.resource) {
      return;
    }

    if (index < 0) {
      channels[channel].texture.reset();
      channels[channel].target = Texture::Target::_2D;
      channels[channel].resource = res;
      return;
    }
    
    Channel newChannel;
    newChannel.resource = res;
    uvec2 size;
    switch (type) {
    case shadertoy::ChannelInputType::TEXTURE:
      newChannel.texture = textureCache[res].tex;
      newChannel.target = Texture::Target::_2D;
      newChannel.resolution = vec3(textureCache[res].size, 0);
      break;

    case shadertoy::ChannelInputType::CUBEMAP:
    {
      newChannel.texture = textureCache[res].tex;
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

  virtual void setShaderAndChannelsInternal(QString source, const ChannelVector & channelVector) {
    for (int i = 0; i < 4; ++i) {
      const ChannelPair & cp = channelVector[i];
      setChannelTextureInternal(i, cp.first, cp.second);
    }
    setShaderSourceInternal(source);
  }

  void updateUniforms() {
    using namespace shadertoy;
    std::map<std::string, GLuint> activeUniforms = getActiveUniforms(skyboxProgram);
    skyboxProgram->Bind();
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
      Uniform<vec3>(*skyboxProgram, UNIFORM_RESOLUTION).Set(textureSize);
    }
    NoProgram().Bind();

    uniformLambdas.clear();
    if (activeUniforms.count(UNIFORM_GLOBALTIME)) {
      uniformLambdas.push_back([&] {
        Uniform<GLfloat>(*skyboxProgram, UNIFORM_GLOBALTIME).Set(Platform::elapsedSeconds());
      });
    }

    if (activeUniforms.count(shadertoy::UNIFORM_POSITION)) {
      uniformLambdas.push_back([&] {
        if (!uiVisible) {
          Uniform<vec3>(*skyboxProgram, shadertoy::UNIFORM_POSITION).Set(ovr::toGlm(getEyePose().Position));
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
  //void onSourceChanged(QString source) {
  //  currentFragmentSource = source.toStdString();
  //  tasks.queueTask([&, source] {
  //    setFragmentSource(currentFragmentSource);
  //  });
  //}

  //void onShaderLoaded(QString source) {
  //  currentFragmentSource = source.toStdString();
  //  emit sourceChanged(currentFragmentSource.c_str());
  //  tasks.queueTask([&] {
  //    loadShader(currentFragmentSource);
  //  });
  //}

  //void onShaderLoaded(int presetIndex) {
  //  currentPresetIndex = presetIndex;
  //  currentFragmentSource = Platform::getResourceData(shadertoy::PRESETS[presetIndex].res);
  //  emit sourceChanged(currentFragmentSource.c_str());
  //  tasks.queueTask([&] {
  //    loadShader(currentFragmentSource);
  //  });
  //}

  //void onNextPreset() {
  //  int newPresetIndex = currentPresetIndex;
  //  ++newPresetIndex;
  //  if (newPresetIndex >= shadertoy::MAX_PRESETS) {
  //    newPresetIndex =  0;
  //  }
  //  onShaderLoaded(newPresetIndex);
  //}

  //void onPreviousPreset() {
  //  int newPresetIndex = currentPresetIndex;
  //  --newPresetIndex;
  //  if (newPresetIndex < 0) {
  //    newPresetIndex = shadertoy::MAX_PRESETS - 1;
  //  }
  //  onShaderLoaded(newPresetIndex);
  //}

  void setChannelTexture(int channel, shadertoy::ChannelInputType type, int index) {
    tasks.queueTask([&, channel, type, index] {
      setChannelTextureInternal(channel, type, index);
      updateUniforms();
    });
  }

  void setShaderSource(QString source) {
    tasks.queueTask([&, source] {
      setShaderSourceInternal(source);
      updateUniforms();
    });
  }

  void setShaderAndChannels(QString source, shadertoy::ChannelInputType types[4], int indices[4]) {
    ChannelVector v; v.resize(4);
    for (int i = 0; i < 4; ++i) {
      v[i].first = types[i];
      v[i].second = indices[i];
    }
    tasks.queueTask([&, source, v] {
      setShaderAndChannelsInternal(source, v);
      updateUniforms();
    });
  }

  void setOffscreenScale(float scale) {
    tasks.queueTask([&, scale] {
      texRes = scale;
    });
  }

signals:
  void compileError(QString source);
  void compileSuccess();
};

class UiToggle : public QObject {
  Q_OBJECT
  bool eventFilter(QObject *object, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {

      QKeyEvent * ke = (QKeyEvent*)event;
      if (Qt::Key_Q == ke->key() && Qt::ControlModifier == ke->modifiers()) {
        QApplication::instance()->quit();
        return true;
      }
      if (Qt::Key_Escape == ke->key() || Qt::Key_F1 == ke->key()) {
        uiVisible = !uiVisible;
        emit uiToggled(uiVisible);
        return true;
      }
    }

    if (event->type() == QEvent::MouseMove) {
      emit mouseMoved();
    }
    return false;
  }

signals:
  void uiToggled(bool newValue);
  void mouseMoved();
};

class ShadertoyApp : public QApplication {
  Q_OBJECT

  ShadertoyRiftWidget riftRenderWidget;
  QGLWidget uiGlWidget;
  UiToggle uiToggle;
  ForwardingGraphicsView uiView;
  QGraphicsScene uiScene;
  GlslEditor shaderTextWidget;
  QTimer timer;
  QGraphicsProxyWidget * uiEditDialog;
  QGraphicsProxyWidget * uiChannelDialog;
  QGraphicsProxyWidget * uiLoadDialog;


  int activeChannelIndex = 0;

  void setUiState(UiState state) {
    uiChannelDialog->hide();
    uiEditDialog->hide();
    uiLoadDialog->hide();

    switch (state) {
    case EDIT:
      uiEditDialog->show();
      break;
    case CHANNEL:
      uiChannelDialog->show();
      break;
    case LOAD:
      uiLoadDialog->show();
      break;
    }
  }


  void setupShaderEditor() {
    initIconCache();
    QWidget & shaderEditor = *(new QWidget());
    shaderEditor.resize(UI_SIZE.x, UI_SIZE.y);
    shaderEditor.setLayout(new QHBoxLayout());
    {
      QWidget * pButtonCol = new QWidget();
      pButtonCol->setLayout(new QVBoxLayout());

      for (int i = 0; i < shadertoy::MAX_CHANNELS; ++i) {
        QToolButton  * button = makeImageButton();
        connect(button, &QToolButton::clicked, this, [&, i] {
          // Start the channel selection dialog
          activeChannelIndex = i;
          setUiState(CHANNEL);
        });
        connect(this, &ShadertoyApp::channelTextureChanged, button, [=](int channel, shadertoy::ChannelInputType type, int index) {
          if (i == channel) {
            assert(channel == i);
            Resource res = shadertoy::getChannelInputResource(type, index);
            button->setIcon(iconCache[res]);
          }
        });
        connect(this, &ShadertoyApp::shaderPresetLoaded, button, [=](QString source, shadertoy::ChannelInputType types[4], int indices[4]) {
          Resource res = shadertoy::getChannelInputResource(types[i], indices[i]);
          button->setIcon(iconCache[res]);
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
        pButtonRow->setLayout(new QHBoxLayout());
        {
          QPushButton * pRunButton = new QPushButton("Run");
          pRunButton->setFont(QFont("Arial", 20, QFont::Bold));
          connect(pRunButton, &QPushButton::clicked, this, [&] {
            emit shaderSourceChanged(shaderTextWidget.toPlainText());
          });
          pButtonRow->layout()->addWidget(pRunButton);
          pButtonRow->layout()->setAlignment(pRunButton, Qt::AlignLeft);
        }

        {
          QPushButton * pLoadButton = new QPushButton("Load");
          pLoadButton->setFont(QFont("Arial", 20, QFont::Bold));
          connect(pLoadButton, &QPushButton::clicked, this, [&] {
            setUiState(LOAD);
          });
          pButtonRow->layout()->addWidget(pLoadButton);
          pButtonRow->layout()->setAlignment(pLoadButton, Qt::AlignRight);
        }

        {
          QPushButton * pSaveButton = new QPushButton("Save");
          pSaveButton->setFont(QFont("Arial", 24, QFont::Bold));
          connect(pSaveButton, &QPushButton::clicked, this, [&] {
            SAY("Start the save dialog");
          });
          pButtonRow->layout()->addWidget(pSaveButton);
          pButtonRow->layout()->setAlignment(pSaveButton, Qt::AlignRight);
        }
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
      QFont f("Arial", 24, QFont::Bold);
      label->setFont(f);
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

        Resource res = shadertoy::TEXTURES[i];
        QToolButton  * button = makeImageButton(res);
        connect(button, &QToolButton::clicked, this, [&, i] {
          emit channelTextureChanged(activeChannelIndex, shadertoy::ChannelInputType::TEXTURE, i);
          setUiState(EDIT);
        });
        pButtonRow->layout()->addWidget(button);
      }
      channelSelector.layout()->addWidget(pButtonRow);
    }

    {
      QLabel * label = new QLabel("Cubemaps");
      QFont f("Arial", 24, QFont::Bold);
      label->setFont(f);
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

        Resource res = shadertoy::CUBEMAPS[i];
        QToolButton  * button = makeImageButton(res);
        connect(button, &QToolButton::clicked, this, [&, i, button] {
          SAY("Emitting CHANNEL TEXTURE SET %d %d %d", activeChannelIndex, shadertoy::ChannelInputType::TEXTURE, i);
          emit channelTextureChanged(activeChannelIndex, shadertoy::ChannelInputType::CUBEMAP, i);
          setUiState(EDIT);
        });
        pButtonRow->layout()->addWidget(button);
      }
      channelSelector.layout()->addWidget(pButtonRow);

      QWidget * pCubemapRow = new QWidget();
      pCubemapRow->setLayout(new QHBoxLayout());
      channelSelector.layout()->addWidget(pCubemapRow);
    }
    uiChannelDialog = uiScene.addWidget(&channelSelector);
    uiChannelDialog->hide();
  }

  void setupLoad() {
    QWidget & loadDialog = *(new QWidget());
    loadDialog.resize(UI_SIZE.x, UI_SIZE.y);
    loadDialog.setLayout(new QHBoxLayout());

    {
      QListWidget * pPresets = new QListWidget();
      pPresets->setFont(QFont("Arial", 24, QFont::Bold));

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

      loadDialog.layout()->addWidget(pPresets);
    }
    uiLoadDialog = uiScene.addWidget(&loadDialog);
    uiLoadDialog->hide();
  }

  TaskQueueWrapper tasks;

  void loadPreset(shadertoy::Preset & preset) {
    QString shaderString = qt::toString(preset.res);

    shaderTextWidget.setPlainText(shaderString);

    shadertoy::ChannelInputType channelTypes[4];
    int channelIndices[4] = { -1, -1, -1, -1 };
    // Parse out any built in indicators for the textures
    {
      foreach(QString line, shaderString.split(QRegExp("\n|\r\n|\r"))) {
        QRegExp re("\\@channel(\\d+)\\s+(\\w+)(\\d{2})\\s*");
        if (re.indexIn(line) > 0) {
          int channelIndex = re.cap(1).toInt();
          if (channelIndex < 0 || channelIndex >= shadertoy::MAX_CHANNELS) {
            continue;
          }
          QString channelTypeStr = re.cap(2);
          int texIndex = re.cap(3).toInt();
          shadertoy::ChannelInputType channelType;
          if (channelTypeStr == "tex") {
            channelType = shadertoy::ChannelInputType::TEXTURE;
          } else if (channelTypeStr == "cube") {
            channelType = shadertoy::ChannelInputType::CUBEMAP;
          } else {
            continue;
          }
          channelTypes[channelIndex] = channelType;
          channelIndices[channelIndex] = texIndex;
        }
      }
    }
    emit shaderPresetLoaded(shaderString, channelTypes, channelIndices);
  }

public:
  ShadertoyApp(int argc, char ** argv) :
    QApplication(argc, argv),
    riftRenderWidget(QRiftWidget::getFormat()),
    uiGlWidget(QRiftWidget::getFormat(), 0, &riftRenderWidget),
    timer(this) { 

    // UI toggle needs to intercept keyboard events before anything else.
    connect(&uiToggle, SIGNAL(uiToggled(bool)), this, SLOT(toggleUi(bool)));
    connect(&uiToggle, SIGNAL(mouseMoved()), this, SLOT(uiMouseOnlyRefresh()));
//    connect(this, SIGNAL(sourceChanged(QString)), &riftRenderWidget, SLOT(onSourceChanged(QString)));
//    connect(this, SIGNAL(shaderLoaded(QString)), &riftRenderWidget, SLOT(onShaderLoaded(QString)));
    riftRenderWidget.installEventFilter(&uiToggle);

    // Cross connect the UI and the rendering system so that when a channel texture is changed in one
    // we reflect it in the other

    connect(this, &ShadertoyApp::channelTextureChanged, &riftRenderWidget, &ShadertoyRiftWidget::setChannelTexture);
    connect(this, &ShadertoyApp::shaderSourceChanged, &riftRenderWidget, &ShadertoyRiftWidget::setShaderSource);
    connect(this, &ShadertoyApp::shaderPresetLoaded, &riftRenderWidget, &ShadertoyRiftWidget::setShaderAndChannels);
//    connect(&riftRenderWidget, &ShadertoyRiftWidget::channelTextureChanged, this, &ShadertoyApp::onChannelTextureChanged, Qt::DirectConnection);
//    connect(&riftRenderWidget, &ShadertoyRiftWidget::channelTextureChanged, this, &ShadertoyApp::onChannelTextureChanged, Qt::DirectConnection);
//    connect(this, &ShadertoyApp::setChannelTexture, &riftRenderWidget, &ShadertoyRiftWidget::setChannelTexture);

    riftRenderWidget.show();
    riftRenderWidget.startRenderThread();
    // We need mouse tracking because the GL window is a proxies mouse 
    // events for actual UI objects that respond to mouse move / hover
    riftRenderWidget.setMouseTracking(true);

    // Set up the UI renderer
    uiView.setViewport(new QWidget());
    uiView.setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    uiView.setScene(&uiScene);
    uiView.resize(UI_SIZE.x, UI_SIZE.y);

    // Set up the channel selection screen
    setupChannelSelector();
    setupShaderEditor();
    setupLoad();

    shaderTextWidget.setFocus();
    shaderTextWidget.setPlainText(qt::toString(SHADERTOY_SHADERS_DEFAULT_FS));
    foreach(QGraphicsItem *item, uiScene.items()) {
      item->setFlag(QGraphicsItem::ItemIsMovable);
      item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }

    connect(&timer, SIGNAL(timeout()), this, SLOT(uiRefresh()));
    timer.start(150);
  }

  virtual ~ShadertoyApp() {
  }

  QPixmap currentWindowImage;
  QPixmap currentWindowWithMouseImage;

private slots:

  void onChannelTextureChanged(int channel, shadertoy::ChannelInputType type, int index) {
    tasks.queueTask([&, channel, type, index] {
      SAY("Emitting CHANNEL TEXTURE CHANGED %d %d %d", channel, type, index);
      emit channelTextureChanged(channel, type, index);
    });
  }

  void toggleUi(bool uiVisible) {
    if (uiVisible) {
      uiView.install(&riftRenderWidget);
    } else {
      uiChannelDialog->hide();
      uiEditDialog->show();
      uiView.remove(&riftRenderWidget);
    }
  }

  void mouseRefresh() {
    QCursor cursor = riftRenderWidget.cursor();
    QPoint position = cursor.pos();
    // Fetching the cursor pixmap isn't working... 
    static QPixmap cursorPm = QPixmap(Normal); //cursor.pixmap();
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

  void uiMouseOnlyRefresh() {
    if (!uiVisible) {
      return;
    }

    mouseRefresh();
    GLuint newTexture = uiGlWidget.bindTexture(currentWindowWithMouseImage);
    if (newTexture) {
      // Is this needed?

      glBindTexture(GL_TEXTURE_2D, newTexture);
      GLenum err = glGetError();
      Texture::MagFilter(Texture::Target::_2D, TextureMagFilter::Nearest);
      Texture::MinFilter(Texture::Target::_2D, TextureMinFilter::Nearest);
      Texture::MagFilter(Texture::Target::_2D, TextureMagFilter::Nearest);
      Texture::MinFilter(Texture::Target::_2D, TextureMinFilter::Nearest);
      glBindTexture(GL_TEXTURE_2D, 0);
      glFlush();

      GLuint oldTexture = uiTexture.exchange(newTexture);
      // If we get a non-0 value back it means the drawing thread never 
      // used this texture, so we can delete it right away
      if (oldTexture) {
        glDeleteTextures(1, &oldTexture);
      }
    }
  }

  void uiRefresh() {
    tasks.drainTaskQueue();
    if (!uiVisible) {
      return;
    }
    uiGlWidget.makeCurrent();
    Texture::Active(0);
    currentWindowImage = uiView.grab();

    uiMouseOnlyRefresh();
  }

signals:
  void shaderSourceChanged(QString str);
  void channelTextureChanged(int channelIndex, shadertoy::ChannelInputType, int index);
  void shaderPresetLoaded(QString source, shadertoy::ChannelInputType types[4], int indices[4]);
};

#define RUN_QT_OVR_APP(AppClass) \
MAIN_DECL { \
  try { \
    ovr_Initialize(); \
    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", "."); \
    QT_APP_WITH_ARGS(AppClass); \
    return app.exec(); \
  } catch (std::exception & error) { \
    SAY_ERR(error.what()); \
  } catch (const std::string & error) { \
    SAY_ERR(error.c_str()); \
  } \
  return -1; \
} 



//static PreInit init;
RUN_QT_OVR_APP(ShadertoyApp)


//class MyImageProvider : public QQuickImageProvider {
//public:
//  MyImageProvider() : QQuickImageProvider(QQmlImageProviderBase::Image) {}
//
//  virtual QImage requestImage(const QString & id, QSize * size, const QSize & requestedSize) {
//    qDebug() << "Id: " << id;
//    Resource res = resourceFromQmlId(id);
//    if (NO_RESOURCE == res) {
//      qWarning() << "Unable to find resource for image ID " << id;
//      return QQuickImageProvider::requestImage(id, size, requestedSize);
//    }
//    return loadImageResource(res);
//  }
//
//  static Resource resourceFromQmlId(const QString & id) {
//    static QRegExp re("(cube|tex)/(\\d+)");
//    static const QString CUBE("cube");
//    static const QString TEX("tex");
//    if (!re.exactMatch(id)) {
//      return NO_RESOURCE;
//    }
//    QString type = re.cap(1);
//    int index = re.cap(2).toInt();
//    qDebug() << "Type " << type << " Index " << index;
//    if (CUBE == type) {
//      return shadertoy::CUBEMAPS[index];
//    } else if (TEX == type) {
//      return shadertoy::TEXTURES[index];
//    }
//    return NO_RESOURCE;
//  }
//
//  static QImage loadImageResource(Resource res) {
//    std::vector<uint8_t> data = Platform::getResourceByteVector(res);
//    QImage image;
//    image.loadFromData(data.data(), data.size());
//    return image;
//  }
//};

//int newChannelData = ((int)type << 16);
//newChannelData |= index;
//emit channelChanged(newChannelData);
//setUiState(EDIT);
//riftRenderWidget.queueTask([&, type, index] {
//  riftRenderWidget.setChannelInput(activeChannelIndex, type, index);
//  riftRenderWidget.updateUniforms();
//});

#include "Example_10_Shaderfun.moc"

