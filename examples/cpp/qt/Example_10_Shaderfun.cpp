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

static const float ROOT_2 = sqrt(2.0f);
static const float INV_ROOT_2 = 1.0f / ROOT_2;
static uvec2 UI_SIZE(UI_X, UI_Y);

static QImage loadImageResource(Resource res) {
  std::vector<uint8_t> data = Platform::getResourceByteVector(res);
  QImage image;
  image.loadFromData(data.data(), data.size());
  return image;
}

static QToolButton * makeImageButton(Resource res = NO_RESOURCE, const QSize & size = QSize(128, 128)) {
  QToolButton  * button = new QToolButton();
  button->resize(size);
  button->setAutoFillBackground(true);
  if (res != NO_RESOURCE) {
    button->setIcon(QIcon(QPixmap::fromImage(loadImageResource(res).scaled(size))));
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

typedef std::shared_ptr<oglplus::VertexShader> VertexShaderPtr;
typedef std::shared_ptr<oglplus::FragmentShader> FragmentShaderPtr;
typedef std::atomic<GLuint> AtomicGlTexture;
typedef std::mutex Mutex;
typedef std::unique_lock<Mutex> Locker;
typedef std::function<void()> Functor;
typedef std::queue<Functor> TaskQueue;
// typedef std::queue<std::function<void()>> TaskQueue;

class TaskQueueWrapper {
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
AtomicGlTexture uiTexture{ 0 };
std::string currentFragmentSource;

class ShadertoyRiftWidget : public QRiftWidget {
  Q_OBJECT
  friend class RendeThread;

  class RenderThread : public QThread {
    ShadertoyRiftWidget & parent;

    void run() {
      glewExperimental = true;
      glewInit();
      parent.context()->makeCurrent();
      parent.setupRiftRendering();

      QCoreApplication* app = QCoreApplication::instance();
      while (!parent.shuttingDown) {
        // Process the Qt message pump to run the standard window controls
        if (app->hasPendingEvents())
          app->processEvents();

        parent.tasks.drainTaskQueue();
        if (parent.isRenderingConfigured()) {
          parent.draw();
        } else {
          QThread::msleep(4);
        }
      }
      parent.context()->doneCurrent();
      parent.context()->moveToThread(QApplication::instance()->thread());
    }

  public:
    RenderThread(ShadertoyRiftWidget & parent) : parent(parent) {
    }
  };

  RenderThread renderThread;
  bool shuttingDown{ false };

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

  float ipd{ OVR_DEFAULT_IPD };
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
  std::string lastError;
  std::list<std::function<void()>> uniformLambdas;
public:


  explicit ShadertoyRiftWidget(QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0)
    : QRiftWidget(parent, shareWidget, f), renderThread(*this) { }

  explicit ShadertoyRiftWidget(QGLContext *context, QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0)
    : QRiftWidget(context, parent, shareWidget, f), renderThread(*this) { }

  explicit ShadertoyRiftWidget(const QGLFormat& format, QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0)
    : QRiftWidget(format, parent, shareWidget, f), renderThread(*this) { }

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
  }

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

    setFragmentSource(Platform::getResourceData(Resource::SHADERTOY_SHADERS_DEFAULT_FS));
    assert(skyboxProgram);

    skybox = oria::loadSkybox(skyboxProgram);
    shaderFramebuffer = FramebufferWrapperPtr(new FramebufferWrapper());
    shaderFramebuffer->init(ovr::toGlm(eyeTextures[0].Header.TextureSize));
    DefaultFramebuffer().Bind(Framebuffer::Target::Draw);
  }

  void loadShaderResource(Resource resource) {
    loadShader(Platform::getResourceData(resource));
  }

  void loadShaderFile(const std::string & file) {
    if (std::string::npos == file.find("/")) {
      // FIXME Get the local files path
      loadShader(Platform::readFile(file));
    } else {
      loadShader(Platform::readFile(file));
    }
  }

  void loadShader(const std::string & shader) {
    using std::regex;
    using std::string;
    using std::sregex_token_iterator;

    for (int i = 0; i < 4; ++i) {
      setChannelInput(i, shadertoy::ChannelInputType::TEXTURE, -1);
    }
    std::vector<std::string> lines = splitLines(shader);
    // Delimiters are spaces (\s) and/or commas
    static std::regex re("\\@channel(\\d+)\\s+(\\w+)(\\d{2})\\s*"); // (\\@\\a + \\d + )");
    std::for_each(lines.begin(), lines.end(), [&](const std::string & line) {
      std::smatch m;
      std::regex_search(line, m, re);
      if (m.size() >= 4) {
        int channel = atoi(m[1].str().c_str());
        if (channel > 3) {
          return;
        }
        std::string typeStr = m[2].str();
        int index = atoi(m[3].str().c_str());
        shadertoy::ChannelInputType type;
        if (std::string("tex") == typeStr) {
          type = shadertoy::ChannelInputType::TEXTURE;
        } else if (std::string("cube") == typeStr) {
          type = shadertoy::ChannelInputType::CUBEMAP;
        } else {
          return;
        }
        setChannelInput(channel, type, index);
      }
    });
    setFragmentSource(shader);
  }
public:

  virtual bool setFragmentSource(const std::string & source) {
    using namespace oglplus;
    try {
      if (!vertexShader) {
        vertexShader = VertexShaderPtr(new VertexShader());
        vertexShader->Source(Platform::getResourceData(Resource::SHADERTOY_SHADERS_DEFAULT_VS));
        vertexShader->Compile();
      }

      std::string header = shadertoy::SHADER_HEADER;
      for (int i = 0; i < 4; ++i) {
        const Channel & channel = channels[i];
        // "uniform sampler2D iChannel0;\n"
        std::string line = Platform::format("uniform sampler%s iChannel%d;\n",
          channel.target == Texture::Target::CubeMap ? "Cube" : "2D", i);
        header += line;
      }
      header += shadertoy::LINE_NUMBER_HEADER;
      FragmentShaderPtr newFragmentShader(new FragmentShader());
      std::string newSource = replaceAll(source, "gl_FragColor", "FragColor");
      newSource = replaceAll(newSource, "texture2D", "texture");
      newSource = header + newSource;
      SAY(newSource.c_str());
      newFragmentShader->Source(GLSLSource(newSource));
      newFragmentShader->Compile();
      ProgramPtr result(new Program());
      result->AttachShader(*vertexShader);
      result->AttachShader(*newFragmentShader);

      result->Link();
      skyboxProgram.swap(result);
      fragmentShader.swap(newFragmentShader);
      updateUniforms();
      return true;
    } catch (ProgramBuildError & err) {
      lastError = err.Log().c_str();
      SAY_ERR((const char*)err.Log().c_str());
    }
    return false;
  }

  virtual void setChannelInput(int channel, shadertoy::ChannelInputType type, int index) {
    using namespace oglplus;
    Channel & channelRef = channels[channel];
    Resource res = shadertoy::getChannelInputResource(type, index);
    if (res == channelRef.resource) {
      return;
    }

    emit channelTextureChanged(channel, type, index);

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
      newChannel.texture = oria::load2dTexture(res, size);
      newChannel.target = Texture::Target::_2D;
      newChannel.resolution = vec3(size, 0);
      break;
    case shadertoy::ChannelInputType::CUBEMAP:
    {
      static int resourceOrder[] = {
        0, 1, 2, 3, 4, 5
      };
      newChannel.texture = oria::loadCubemapTexture(res, resourceOrder, false);
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

  virtual void resizeEvent(QResizeEvent *e) {

  }

  virtual void paintEvent(QPaintEvent *) {
  }

public slots:
  void onSourceChanged(QString source) {
    currentFragmentSource = source.toStdString();
    tasks.queueTask([&, source] {
      setFragmentSource(currentFragmentSource);
    });
  }

  void onShaderLoaded(QString source) {
    currentFragmentSource = source.toStdString();
    tasks.queueTask([&, source] {
      loadShader(currentFragmentSource);
    });
  }

  void setChannelTexture(int channel, shadertoy::ChannelInputType type, int index) {
    tasks.queueTask([&, channel, type, index] {
      setChannelInput(channel, type, index);
    });
  }

signals:
  void channelTextureChanged(int channel, shadertoy::ChannelInputType type, int index);
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
  QPlainTextEdit shaderTextWidget;
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
        button->setProperty("channel", i);
        connect(this, &ShadertoyApp::channelTextureChanged, button, [=](int channel, shadertoy::ChannelInputType type, int index) {
          int buttonChannel = button->property("channel").toInt();
          if (buttonChannel == channel) {
            Resource res = shadertoy::getChannelInputResource(type, index);
            if (NO_RESOURCE == res) {
              button->setIcon(QIcon());
            } else {
              button->setIcon(QIcon(QPixmap::fromImage(loadImageResource(res).scaled(QSize(128, 128)))));
            }
          }
        });
        pButtonCol->layout()->addWidget(button);
      }
      shaderEditor.layout()->addWidget(pButtonCol);
    }

    {
      QWidget * pEditColumn = new QWidget();
      pEditColumn->setLayout(new QVBoxLayout());
      pEditColumn->layout()->addWidget(&shaderTextWidget);
      std::vector<uint8_t> v = Platform::getResourceByteVector(Resource::FONTS_INCONSOLATA_REGULAR_TTF);
      QFontDatabase::addApplicationFontFromData(QByteArray((char*)&v[0], (int)v.size()));
      QFont f("Inconsolata", 18);
      shaderTextWidget.setFont(f);

      {
        QWidget * pButtonRow = new QWidget();
        pButtonRow->setLayout(new QHBoxLayout());
        {
          QPushButton * pRunButton = new QPushButton("Run");
          pRunButton->setFont(QFont("Arial", 24, QFont::Bold));
          connect(pRunButton, &QPushButton::clicked, this, [&] {
            emit sourceChanged(shaderTextWidget.toPlainText());
          });
          pButtonRow->layout()->addWidget(pRunButton);
          pButtonRow->layout()->setAlignment(pRunButton, Qt::AlignLeft);
        }

        {
          QPushButton * pLoadButton = new QPushButton("Load");
          pLoadButton->setFont(QFont("Arial", 24, QFont::Bold));
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
        connect(this, &ShadertoyApp::channelTextureChanged, &riftRenderWidget, &ShadertoyRiftWidget::setChannelTexture);
        connect(button, &QToolButton::clicked, this, [&, i] {
          emit setChannelTexture(activeChannelIndex, shadertoy::ChannelInputType::TEXTURE, i);
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
          emit setChannelTexture(activeChannelIndex, shadertoy::ChannelInputType::CUBEMAP, i);
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
        pItem->setData(Qt::UserRole, (int)preset.res);
        pPresets->addItem(pItem);
      }
      connect(pPresets, &QListWidget::itemClicked, this, [&](QListWidgetItem* item) {
        Resource shader = (Resource)item->data(Qt::UserRole).toInt();
        QString shaderString = Platform::getResourceData(shader).c_str();
        shaderTextWidget.setPlainText(shaderString);
        emit shaderLoaded(shaderString);
        setUiState(EDIT);
      });
      loadDialog.layout()->addWidget(pPresets);
    }
    uiLoadDialog = uiScene.addWidget(&loadDialog);
    uiLoadDialog->hide();
  }

  TaskQueueWrapper tasks;

public:
  ShadertoyApp(int argc, char ** argv) :
    QApplication(argc, argv),
    riftRenderWidget(QRiftWidget::getFormat()),
    uiGlWidget(QRiftWidget::getFormat(), 0, &riftRenderWidget),
    timer(this) { 

    // UI toggle needs to intercept keyboard events before anything else.
    connect(&uiToggle, SIGNAL(uiToggled(bool)), this, SLOT(toggleUi(bool)));
    connect(&uiToggle, SIGNAL(mouseMoved()), this, SLOT(uiRefresh()));
    connect(this, SIGNAL(sourceChanged(QString)), &riftRenderWidget, SLOT(onSourceChanged(QString)));
    connect(this, SIGNAL(shaderLoaded(QString)), &riftRenderWidget, SLOT(onShaderLoaded(QString)));
    riftRenderWidget.installEventFilter(&uiToggle);

    // Cross connect the UI and the rendering system so that when a channel texture is changed in one
    // we reflect it in the other

    connect(&riftRenderWidget, &ShadertoyRiftWidget::channelTextureChanged, this, &ShadertoyApp::onChannelTextureChanged, Qt::DirectConnection);
    connect(this, &ShadertoyApp::setChannelTexture, &riftRenderWidget, &ShadertoyRiftWidget::setChannelTexture);

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

    shaderTextWidget.setPlainText(Platform::getResourceData(SHADERTOY_SHADERS_DEFAULT_FS).c_str());


    foreach(QGraphicsItem *item, uiScene.items()) {
      item->setFlag(QGraphicsItem::ItemIsMovable);
      item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }

    connect(&timer, SIGNAL(timeout()), this, SLOT(uiRefresh()));
    timer.start(150);
  }

  virtual ~ShadertoyApp() {
  }

private slots:


  void onChannelTextureChanged(int channel, shadertoy::ChannelInputType type, int index) {
    tasks.queueTask([&, channel, type, index] {
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

  void uiRefresh() {
    tasks.drainTaskQueue();
    if (!uiVisible) {
      return;
    }
    uiGlWidget.makeCurrent();
    Texture::Active(0);
    QPixmap windowImage = uiView.grab();
    QCursor cursor = riftRenderWidget.cursor();
    QPoint position = cursor.pos();
    // Fetching the cursor pixmap isn't working... 
    static QPixmap cursorPm = QPixmap(Normal); //cursor.pixmap();
    QPointF relative = riftRenderWidget.mapFromGlobal(position);
    relative.rx() /= riftRenderWidget.size().width();
    relative.ry() /= riftRenderWidget.size().height();
    relative.rx() *= uiView.size().width();
    relative.ry() *= uiView.size().height();

    // Draw the mouse pointer on the window
    QPainter qp;
    qp.begin(&windowImage);
    qp.drawPixmap(relative, cursorPm);
    qp.end();

    GLuint newTexture = uiGlWidget.bindTexture(windowImage);
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

signals:
  void sourceChanged(QString str);
  void shaderLoaded(QString str);
  void channelTextureChanged(int channelIndex, shadertoy::ChannelInputType, int index);
  void setChannelTexture(int channel, shadertoy::ChannelInputType type, int index);
};

//struct PreInit{
//  PreInit() {
////    SetCurrentDirectoryA("F:\\shadertoy");
////    ovr_Initialize();
////    QCoreApplication::addLibraryPath("F:/shadertoy");
//  }
//};


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

#include "Example_10_Shaderfun.moc"


//virtual void onMouseButton(int button, int action, int mods) {
//  if (uiVisible()) {
//    ui::handleGlfwMouseButtonEvent(button, action, mods);
//  }
//}

//virtual void onMouseMove(double x, double y) {
//  if (uiVisible()) {
//    vec2 mousePosition(x, y);
//    vec2 windowSize(getSize());
//    mousePosition /= windowSize;
//    mousePosition *= UI_SIZE;
//    ui::handleGlfwMouseMoveEvent(mousePosition.x, mousePosition.y);
//  }
//}

//virtual void onScroll(double x, double y) {
//  if (uiVisible()) {
//    ui::handleGlfwMouseScrollEvent(x, y);
//  }
//}

//virtual void onCharacter(unsigned int codepoint) {
//  if (editUi.uiVisible()) {
//    ui::handleGlfwCharacterEvent(codepoint);
//  }
//}

//  virtual void onKey(int key, int scancode, int action, int mods) {
//    //if (editUi.uiVisible()) {
//    //  if (GLFW_PRESS == action && GLFW_KEY_ESCAPE == key) {
//    //    editUi.setUiState(shadertoy::UiState::INACTIVE);
//    //  }
//    //  if (GLFW_PRESS == action && GLFW_MOD_CONTROL == mods) {
//    //    switch (key) {
//    //    case GLFW_KEY_C:
//    //      CEGUI::System::getSingleton().getDefaultGUIContext().injectCopyRequest();
//    //      return;
//    //    case GLFW_KEY_V:
//    //      CEGUI::System::getSingleton().getDefaultGUIContext().injectPasteRequest();
//    //      return;
//    //    case GLFW_KEY_X:
//    //      CEGUI::System::getSingleton().getDefaultGUIContext().injectCutRequest();
//    //      return;
//    //    case GLFW_KEY_A:
//    //      // CEGUI::System::getSingleton().getDefaultGUIContext().injectinjectCutRequest();
//    //      // return;
//    //    default:
//    //      break;
//    //    }
//    //  }
//    //  ui::handleGlfwKeyboardEvent(key, scancode, action, mods);
//    //  return;
//    //}
//
//    if (action == GLFW_PRESS) {
//      
//      if (key >= GLFW_KEY_0 && key < (GLFW_KEY_0 + shadertoy::MAX_PRESETS)) {
//        int index = key - GLFW_KEY_0;
//        Resource res = shadertoy::PRESETS[index].res;
//        loadShaderResource(res);
//        return;
//      }
//      
//      switch (key) {
//#ifdef RIFT
//      case GLFW_KEY_HOME:
//        if (texRes < 0.95f) {
//          texRes = std::min(texRes * ROOT_2, 1.0f);
//        }
//        break;
//      case GLFW_KEY_END:
//        if (texRes > 0.05f) {
//          texRes *= INV_ROOT_2;
//        }
//        break;
//#endif
//      case GLFW_KEY_R:
//        resetCamera();
//        break;
//          
//      case GLFW_KEY_ESCAPE:
//        //editUi.setUiState(shadertoy::UiState::EDIT);
//        break;
//      }
//    }
//    else {
//      QRiftWidget::onKey(key, scancode, action, mods);
//    }
//  }
//  void resetCamera() {
//    player = glm::inverse(glm::lookAt(
//      glm::vec3(0, 0, ipd * 2),  // Position of the camera
//      glm::vec3(0, 0, 0),  // Where the camera is looking
//      Vectors::Y_AXIS));           // Camera up axis
//#ifdef RIFT
//    ovrHmd_RecenterPose(hmd);
//#endif
//  }
//#ifndef RIFT
//  void draw() {
//    glGetError();
//    DefaultFramebuffer().Bind(Framebuffer::Target::Draw);
//    Context::Viewport(0, 0, UI_X, UI_Y);
//    Context::ClearColor(0, 0, 1, 1);
//    Context::Clear().ColorBuffer().DepthBuffer();
//    Stacks::projection().top() = glm::perspective(1.6f, 128.0f/72.0f, 0.01f, 100.0f);
//    renderScene();
//  }
//
//  GLFWwindow * createRenderingTarget(uvec2 & outSize, ivec2 & outPosition) {
//    outSize = uvec2(UI_X, UI_Y);
//    outPosition = ivec2(100, -800);
//    return glfw::createWindow(outSize, outPosition);
//  }
//#endif





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
