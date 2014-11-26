#include "Common.h"
#include <QtGui/QGuiApplication>
#include <QtWidgets/QGraphicsView>
#include <QtGui/QResizeEvent>
#include <QGLWidget>
#include <QtWidgets>
#include <QPixmap>

#include <regex>
#include <mutex>
#include <thread>
#include <set>
#if 0

#define RIFT
using namespace oglplus;

static uvec2 UI_SIZE(1280, 720);

#define RIFT
#ifdef RIFT
#define PARENT_CLASS RiftApp
#else
#define PARENT_CLASS GlfwApp
glm::mat4 player;
#endif

class OffscreenUi {
  typedef std::mutex Mutex;
  typedef std::unique_lock<Mutex> Lock;
  typedef std::function<void(QWidget &, QOpenGLContext &)> InitFunction;
  typedef std::thread Thread;
  typedef std::unique_ptr<Thread> ThreadPtr;

  Mutex mutex;
  ThreadPtr thread;
  InitFunction initFunction;
  QImage image;
  QOpenGLContext context;

  void run() {
    static int argc = 0;
    static char ** argv = nullptr;
    QApplication app(argc, argv);
    QWidget window;
//    initFunction(window, context);
    while (true) {
      Platform::sleepMillis(100);
      // Process the Qt message pump to run the standard window controls
      //      if (app.hasPendingEvents())
      //        app.processEvents();
      //      app.postEvent();
//        Lock lock(mutex);
//        image = QPixmap::grabWidget(&window).toImage();
//          // If the system depth is 16 and the pixmap doesn't have an alpha channel
//          // then we convert it to RGB16 in the hope that it gets uploaded as a 16
//          // bit texture which is much faster to access than a 32-bit one.
//          if (pixmap.depth() == 16 && !image.hasAlphaChannel())
//            image = image.convertToFormat(QImage::Format_RGB16);
//          texture = bindTexture(image, target, format, key, options);
//        }
//        // NOTE: bindTexture(const QImage&, GLenum, GLint, const qint64, bool) should never return null
//        Q_ASSERT(texture);

//        if (texture->id > 0)
//          QImagePixmapCleanupHooks::enableCleanupHooks(pixmap);
//
//        return texture;
    }
  }

public:

  void start(InitFunction init = [](QWidget &, QOpenGLContext &){}) {
    initFunction = init;
    thread = ThreadPtr(new Thread([&]{
      run();
    }));
  }
};

namespace shadertoy {
  const int MAX_CHANNELS = 4;

  enum class ChannelInputType {
    TEXTURE, CUBEMAP, AUDIO, VIDEO,
  };

  const char * UNIFORM_RESOLUTION = "iResolution";
  const char * UNIFORM_GLOBALTIME = "iGlobalTime";
  const char * UNIFORM_CHANNEL_TIME = "iChannelTime";
  const char * UNIFORM_CHANNEL_RESOLUTION = "iChannelResolution";
  const char * UNIFORM_MOUSE_COORDS = "iMouse";
  const char * UNIFORM_DATE = "iDate";
  const char * UNIFORM_SAMPLE_RATE = "iSampleRate";
  const char * UNIFORM_POSITION = "iPos";
  const char * UNIFORM_CHANNELS[4] = {
    "iChannel0",
    "iChannel1",
    "iChannel2",
    "iChannel3",
  };

  const char * SHADER_HEADER = "#version 330\n"
    "uniform vec3      iResolution;           // viewport resolution (in pixels)\n"
    "uniform float     iGlobalTime;           // shader playback time (in seconds)\n"
    "uniform float     iChannelTime[4];       // channel playback time (in seconds)\n"
    "uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)\n"
    "uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click\n"
    "uniform vec4      iDate;                 // (year, month, day, time in seconds)\n"
    "uniform float     iSampleRate;           // sound sample rate (i.e., 44100)\n"
    "uniform vec3      iPos; // Head position\n"
    "in vec3 iDir; // Direction from viewer\n"
    "out vec4 FragColor;\n";

  const char * LINE_NUMBER_HEADER =
    "#line 1\n";


  const Resource TEXTURES[] = {
    Resource::SHADERTOY_TEXTURES_TEX00_JPG,
    Resource::SHADERTOY_TEXTURES_TEX01_JPG,
    Resource::SHADERTOY_TEXTURES_TEX02_JPG,
    Resource::SHADERTOY_TEXTURES_TEX03_JPG,
    Resource::SHADERTOY_TEXTURES_TEX04_JPG,
    Resource::SHADERTOY_TEXTURES_TEX05_JPG,
    Resource::SHADERTOY_TEXTURES_TEX06_JPG,
    Resource::SHADERTOY_TEXTURES_TEX07_JPG,
    Resource::SHADERTOY_TEXTURES_TEX08_JPG,
    Resource::SHADERTOY_TEXTURES_TEX09_JPG,
    Resource::SHADERTOY_TEXTURES_TEX10_PNG,
    Resource::SHADERTOY_TEXTURES_TEX11_PNG,
    Resource::SHADERTOY_TEXTURES_TEX12_PNG,
    NO_RESOURCE,
    Resource::SHADERTOY_TEXTURES_TEX14_PNG,
    Resource::SHADERTOY_TEXTURES_TEX15_PNG,
    Resource::SHADERTOY_TEXTURES_TEX16_PNG,
  };
  const int MAX_TEXTURES = 17;

  const Resource CUBEMAPS[] = {
    Resource::SHADERTOY_CUBEMAPS_CUBE00_0_JPG,
    Resource::SHADERTOY_CUBEMAPS_CUBE01_0_PNG,
    Resource::SHADERTOY_CUBEMAPS_CUBE02_0_JPG,
    Resource::SHADERTOY_CUBEMAPS_CUBE03_0_PNG,
    Resource::SHADERTOY_CUBEMAPS_CUBE04_0_PNG,
    Resource::SHADERTOY_CUBEMAPS_CUBE05_0_PNG,
  };
  const int MAX_CUBEMAPS = 6;

  std::string getChannelInputName(ChannelInputType type, int index) {
    switch (type) {
    case ChannelInputType::TEXTURE:
      return Platform::format("Tex%02d", index);
    case ChannelInputType::CUBEMAP:
      return Platform::format("Cube%02d", index);
    default:
      return "";
    }
  }

  Resource getChannelInputResource(ChannelInputType type, int index) {
    switch (type) {
    case ChannelInputType::TEXTURE:
      return TEXTURES[index];
    case ChannelInputType::CUBEMAP:
      return CUBEMAPS[index];
    default:
      return NO_RESOURCE;
    }
  }

  struct Preset {
    const Resource res;
    const char * name;
    Preset(Resource res, const char * name) : res(res), name(name) {};
  };

  Preset PRESETS[] {
    Preset(Resource::SHADERTOY_SHADERS_DEFAULT_FS, "Default"),
      Preset(Resource::SHADERTOY_SHADERS_4DXGRM_FLYING_STEEL_CUBES_FS, "Steel Cubes"),
      Preset(Resource::SHADERTOY_SHADERS_4DF3DS_INFINITE_CITY_FS, "Infinite City"),
      //    Preset(Resource::SHADERTOY_SHADERS_4DFGZS_VOXEL_EDGES_FS, "Voxel Edges"),
      Preset(Resource::SHADERTOY_SHADERS_4DJGWR_ROUNDED_VOXELS_FS, "Rounded Voxels"),
      Preset(Resource::SHADERTOY_SHADERS_4SBGD1_FAST_BALLS_FS, "Fast Balls"),
      Preset(Resource::SHADERTOY_SHADERS_4SXGRM_OCEANIC_FS, "Oceanic"),
      Preset(Resource::SHADERTOY_SHADERS_MDX3RR_ELEVATED_FS, "Elevated"),
      //    Preset(Resource::SHADERTOY_SHADERS_MSSGD1_HAND_DRAWN_SKETCH_FS, "Hand Drawn"),
      Preset(Resource::SHADERTOY_SHADERS_MSXGZM_VORONOI_ROCKS_FS, "Voronoi Rocks"),
      Preset(Resource::SHADERTOY_SHADERS_XSBSRG_MORNING_CITY_FS, "Morning City"),
      Preset(Resource::SHADERTOY_SHADERS_XSSSRW_ABANDONED_BASE_FS, "Abandoned Base"),
      Preset(Resource::SHADERTOY_SHADERS_MSXGZ4_CUBEMAP_FS, "Cubemap"),
      Preset(Resource::SHADERTOY_SHADERS_LSS3WS_RELENTLESS_FS, "Relentless"),
      Preset(Resource::NO_RESOURCE, nullptr),
  };

  enum UiState {
    INACTIVE,
    EDIT,
    CHANNEL,
    LOAD,
    SAVE,
  };
}

typedef std::shared_ptr<oglplus::VertexShader> VertexShaderPtr;
typedef std::shared_ptr<oglplus::FragmentShader> FragmentShaderPtr;

struct Channel {
  TexturePtr texture;
  oglplus::Texture::Target target;
  vec3 resolution;
};

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
  while (std::getline(ss, line, '\n')){
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
    }
    else {
      ++count;
    }
  }

  float getRate() const {
    float elapsed = Platform::elapsedSeconds() - start;
    return (float)count / elapsed;
  }
};

std::set<std::string> getActiveUniforms(ProgramPtr & program) {
  std::set<std::string> activeUniforms;
  activeUniforms.clear();
  size_t uniformCount = program->ActiveUniforms().Size();
  for (int i = 0; i < uniformCount; ++i) {
    std::string name = program->ActiveUniforms().At(i).Name().c_str();
    activeUniforms.insert(name);
  }
  return activeUniforms;
}


class DynamicFramebufferScaleExample : public PARENT_CLASS {

  enum UiState {
    INACTIVE,
    EDIT,
    CHANNEL,
    LOAD,
    SAVE,
  };

  float ipd{ OVR_DEFAULT_IPD };
  float texRes{ 1.0f };

  // GLSL and geometry for the shader skybox
  ProgramPtr skyboxProgram;
  ShapeWrapperPtr skybox;
  VertexShaderPtr vertexShader;
  FragmentShaderPtr fragmentShader;
  // We actually render the shader to one FBO for dynamic framebuffer scaling,
  // while leaving the actual texture we pass to the Oculus SDK fixed.
  // This allows us to have a clear UI regardless of the shader performance
  FramebufferWrapper shaderFramebuffer;


  // GLSL and geometry for the UI
  ProgramPtr planeProgram;
  ShapeWrapperPtr plane;

  // Measure the FPS for use in dynamic scaling
  RateCounter fps;

  OffscreenUi ui;

  // The current fragment source
  std::string lastError;
  std::list<std::function<void()>> uniformLambdas;
  Channel channels[4];
  UiState uiState{ INACTIVE };

public:
  DynamicFramebufferScaleExample() {
#ifdef RIFT
    ipd = ovrHmd_GetFloat(hmd, OVR_KEY_IPD, OVR_DEFAULT_IPD);
#endif
    resetCamera();
  }

  virtual ~DynamicFramebufferScaleExample() {
  }

#ifdef RIFT
  vec2 textureSize() {
    return vec2(ovr::toGlm(eyeTextures[0].Header.TextureSize));
  }
  uvec2 renderSize() {
    return uvec2(texRes * textureSize());
  }
#else
  vec2 textureSize() {
    return vec2(UI_SIZE);
  }
  uvec2 renderSize() {
    return UI_SIZE;
  }
#endif

  virtual void onMouseEnter(int entered) {
    if (entered){
      glfwSetInputMode(getWindow(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    } else {
      glfwSetInputMode(getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
  }

  void initUi() {
    ui.start([&](QWidget& widget, QOpenGLContext & context){
      widget.resize(UI_SIZE.x, UI_SIZE.y);
      //! [create, position and show]
      QPushButton *button = new QPushButton(
        QApplication::translate("childwidget", "Press me"), &widget);
      button->move(100, 100);
      button->show();
    });
  }
  
  virtual void initGl() {
    initUi();

    PARENT_CLASS::initGl();
    program = oria::loadProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_TEXTURED_FS);
    float aspect = (float)UI_SIZE.x / (float)UI_SIZE.y;
    shape = oria::loadPlane(program, aspect);

    planeProgram = oria::loadProgram(
      Resource::SHADERS_TEXTURED_VS, 
      Resource::SHADERS_TEXTURED_FS);
    plane = oria::loadPlane(planeProgram, 1.0);

    setFragmentSource(Platform::getResourceData(Resource::SHADERTOY_SHADERS_DEFAULT_FS));
    assert(skyboxProgram);

    skybox = oria::loadSkybox(skyboxProgram);
    Platform::addShutdownHook([&]{
      shaderFramebuffer.color.reset();
      shaderFramebuffer.depth.reset();
      shaderFramebuffer.fbo.reset();
      vertexShader.reset();
      fragmentShader.reset();
      planeProgram.reset();
      plane.reset();
      skyboxProgram.reset();
      skybox.reset();
    });


#ifdef RIFT
    shaderFramebuffer.init(ovr::toGlm(eyeTextures[0].Header.TextureSize));
#else
    shaderFramebuffer.init(UI_SIZE);
#endif
    DefaultFramebuffer().Bind(Framebuffer::Target::Draw);

    loadShaderResource(Resource::SHADERTOY_SHADERS_DEFAULT_FS);
  }

  void loadShaderResource(Resource resource) {
    loadShader(Platform::getResourceData(resource));
  }

  void loadShaderFile(const std::string & file) {
    if (std::string::npos == file.find("/")) {
      // FIXME Get the local files path
      loadShader(Platform::readFile(file));
    }
    else {
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
    std::for_each(lines.begin(), lines.end(), [&](const std::string & line){
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
        }
        else if (std::string("cube") == typeStr) {
          type = shadertoy::ChannelInputType::CUBEMAP;
        }
        else {
          return;
        }
        setChannelInput(channel, type, index);
      }
    });
    setFragmentSource(shader);
  }

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
    }
    catch (ProgramBuildError & err) {
      lastError = err.Log().c_str();
      SAY_ERR((const char*)err.Log().c_str());
    }
    return false;
  }

  virtual void setChannelInput(int channel, shadertoy::ChannelInputType type, int index) {
    using namespace oglplus;
    if (index < 0) {
      channels[channel].texture.reset();
      channels[channel].target = Texture::Target::_2D;
      return;
    }

    Resource res = getChannelInputResource(type, index);
    Channel newChannel;
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
    updateUniforms();
  }

  void updateUniforms() {
    using namespace shadertoy;
    std::set<std::string> activeUniforms = getActiveUniforms(skyboxProgram);
    skyboxProgram->Bind();
    //    UNIFORM_DATE;
    for (int i = 0; i < 4; ++i) {
      const char * uniformName = shadertoy::UNIFORM_CHANNELS[i];
      if (activeUniforms.count(uniformName)) {
        Uniform<GLuint>(*skyboxProgram, uniformName).Set(i);
      }
    }
    if (activeUniforms.count(UNIFORM_RESOLUTION)) {
#ifdef RIFT
      vec3 textureSize = vec3(ovr::toGlm(this->eyeTextures[0].Header.TextureSize), 0);
      Uniform<vec3>(*skyboxProgram, UNIFORM_RESOLUTION).Set(textureSize);
#else
      Uniform<vec3>(*skyboxProgram, UNIFORM_RESOLUTION).Set(vec3(1920, 1080, 0));
#endif
    }
    NoProgram().Bind();

    uniformLambdas.clear();
    if (activeUniforms.count(UNIFORM_GLOBALTIME)) {
      uniformLambdas.push_back([&]{
        Uniform<GLfloat>(*skyboxProgram, UNIFORM_GLOBALTIME).Set(Platform::elapsedSeconds());
      });
    }

    if (activeUniforms.count(shadertoy::UNIFORM_POSITION)) {
      uniformLambdas.push_back([&]{
#ifdef RIFT
        Uniform<vec3>(*skyboxProgram, shadertoy::UNIFORM_POSITION).Set(ovr::toGlm(getEyePose().Position));
#endif
      });
    }
    for (int i = 0; i < 4; ++i) {
      if (activeUniforms.count(UNIFORM_CHANNELS[i]) && channels[i].texture) {
        uniformLambdas.push_back([=]{
          Texture::Active(i);
          channels[i].texture->Bind(channels[i].target);
        });
      }
    }
  }


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

  virtual void onKey(int key, int scancode, int action, int mods) {
    //if (editUi.uiVisible()) {
    //  if (GLFW_PRESS == action && GLFW_KEY_ESCAPE == key) {
    //    editUi.setUiState(shadertoy::UiState::INACTIVE);
    //  }
    //  if (GLFW_PRESS == action && GLFW_MOD_CONTROL == mods) {
    //    switch (key) {
    //    case GLFW_KEY_C:
    //      CEGUI::System::getSingleton().getDefaultGUIContext().injectCopyRequest();
    //      return;
    //    case GLFW_KEY_V:
    //      CEGUI::System::getSingleton().getDefaultGUIContext().injectPasteRequest();
    //      return;
    //    case GLFW_KEY_X:
    //      CEGUI::System::getSingleton().getDefaultGUIContext().injectCutRequest();
    //      return;
    //    case GLFW_KEY_A:
    //      // CEGUI::System::getSingleton().getDefaultGUIContext().injectinjectCutRequest();
    //      // return;
    //    default:
    //      break;
    //    }
    //  }
    //  ui::handleGlfwKeyboardEvent(key, scancode, action, mods);
    //  return;
    //}

    static const float ROOT_2 = sqrt(2.0f);
    static const float INV_ROOT_2 = 1.0f / ROOT_2;
    if (action == GLFW_PRESS) {
      switch (key) {
#ifdef RIFT
      case GLFW_KEY_HOME:
        if (texRes < 0.95f) {
          texRes = std::min(texRes * ROOT_2, 1.0f);
        }
        break;
      case GLFW_KEY_END:
        if (texRes > 0.05f) {
          texRes *= INV_ROOT_2;
        }
        break;
#endif
      case GLFW_KEY_R:
        resetCamera();
        break;

      case GLFW_KEY_ESCAPE:
        //editUi.setUiState(shadertoy::UiState::EDIT);
        break;
      }
    }
    else {
      PARENT_CLASS::onKey(key, scancode, action, mods);
    }
  }

  void resetCamera() {
    player = glm::inverse(glm::lookAt(
      glm::vec3(0, 0, ipd * 2),  // Position of the camera
      glm::vec3(0, 0, 0),  // Where the camera is looking
      Vectors::Y_AXIS));           // Camera up axis
#ifdef RIFT
    ovrHmd_RecenterPose(hmd);
#endif
  }

  void update() {
    PARENT_CLASS::update();
    //editUi.update([&]{
    //  if (fps.getCount() > 50) {
    //    float rate = fps.getRate();
    //    if (rate < 55) {
    //      SAY("%f %f", rate, texRes);
    //      texRes *= 0.9f;
    //    }
    //    editUi.getWindow()->getChild("ShaderEdit/TextFps")->setText(Platform::format("%2.1f", rate));
    //    editUi.getWindow()->getChild("ShaderEdit/TextRez")->setText(Platform::format("%2.1f", texRes));
    //    float mpx = renderSize().x * renderSize().y;
    //    mpx /= 1e6f;
    //    editUi.getWindow()->getChild("ShaderEdit/TextMpx")->setText(Platform::format("%02.2f", mpx));

    //    fps.reset();
    //  }
    //});
    fps.increment();
  }

  ProgramPtr program;
  ShapeWrapperPtr shape;

  void renderSkybox() {
    using namespace oglplus;
    shaderFramebuffer.Bound([&]{
      viewport(renderSize());
      skyboxProgram->Bind();
      MatrixStack & mv = Stacks::modelview();
      mv.withPush([&]{
        mv.untranslate();
        oria::renderGeometry(skybox, skyboxProgram, uniformLambdas);
      });
      for (int i = 0; i < 4; ++i) {
        Texture::Active(i);
        DefaultTexture().Bind(Texture::Target::_2D);
        DefaultTexture().Bind(Texture::Target::CubeMap);
      }
      Texture::Active(0);
    });
    viewport(textureSize());
  }

  void renderScene() {
    glGetError();
    viewport(textureSize());
    //Context::Disable(Capability::Blend);
    //Context::Disable(Capability::ScissorTest);
    Context::Disable(Capability::DepthTest);
    Context::Disable(Capability::CullFace);
    Context::Clear().DepthBuffer().ColorBuffer();
    MatrixStack & mv = Stacks::modelview();
    MatrixStack & pr = Stacks::projection();
    mv.withPush([&]{
      mv.postMultiply(glm::inverse(player));
      renderSkybox();
    });

    Stacks::withPush([&]{
      pr.identity();
      mv.identity();
      Stacks::projection().identity();
      shaderFramebuffer.color->Bind(Texture::Target::_2D);
      oria::renderGeometry(plane, planeProgram, { [&]{
        Uniform<vec2>(*planeProgram, "UvMultiplier").Set(vec2(texRes));
      } });
    });
    //Context::Clear().DepthBuffer().ColorBuffer();

    //    if (editUi.uiVisible()) {
    //      mv.withPush([&]{
    //        mv.translate(vec3(0, 0, -1));
    //        mv.postMultiply(glm::inverse(player));
    ////        editUi.render();
    //        editUi.bindTexture();
    //        oria::renderGeometry(shape, program);
    //      });
    //    }
  }

#ifndef RIFT
  void draw() {
    glGetError();
    DefaultFramebuffer().Bind(Framebuffer::Target::Draw);
    Context::Viewport(0, 0, UI_X, UI_Y);
    Context::ClearColor(0, 0, 1, 1);
    Context::Clear().ColorBuffer().DepthBuffer();
    Stacks::projection().top() = glm::perspective(1.6f, 128.0f/72.0f, 0.01f, 100.0f);
    renderScene();
  }

  GLFWwindow * createRenderingTarget(uvec2 & outSize, ivec2 & outPosition) {
    outSize = uvec2(UI_X, UI_Y);
    outPosition = ivec2(100, -800);
    return glfw::createWindow(outSize, outPosition);
  }
#endif

};



#ifdef RIFT
RUN_OVR_APP(DynamicFramebufferScaleExample);
#else
RUN_APP(DynamicFramebufferScaleExample);
#endif

#else


//
//
//using namespace oglplus;
//class UiWidget : public QWidget {
//public:
//  UiWidget(QWidget * widget, int windowFlags = 0) {
//    widget->installEventFilter(this);
//  }
//
//  bool eventFilter(QObject *object, QEvent *event) {
//    if (QEvent::MouseButtonPress == event->type()) {
//      this->event(event);
//      return true;
//    }
//    return false;
//  }
//};

using namespace oglplus;

class OculusWidget : public QGLWidget
{
  ShapeWrapperPtr shape;
  ProgramPtr program;
public:

  OculusWidget(QGLFormat & format)
  : QGLWidget(format) { }

  virtual ~OculusWidget() {
  }

private:
  void initializeGL() {
    SAY("INIT");
    glewExperimental = true;
    glewInit();
    glGetError();
    DefaultFramebuffer().Bind(Framebuffer::Target::Draw);

    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    GLuint unusedIds = 0;
    if (glDebugMessageCallback) {
      glDebugMessageCallback(oria::debugCallback, this);
      glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
        0, &unusedIds, true);
    }
    glGetError();

    program = oria::loadProgram(
      Resource::SHADERS_TEXTURED_VS, 
      Resource::SHADERS_TEXTURED_FS);
    shape = oria::loadPlane(program, 1.0f);
  }

  void resizeGL(int w, int h) {
    glGetError();
    SAY("Resize %d %d", w, h);
    Context::Viewport(0, 0, w, h);
  }

  void paintGL() {
    Context::ClearColor(0, 1, 1, 1);
    Context::Clear().ColorBuffer();
    oria::renderGeometry(shape, program);
    update();
  }
};

QGLWidget * glWidget = nullptr;

class MyView : public QGraphicsView {
public:
  static MyView * INSTANCE;

  MyView() {
    INSTANCE = this;
  }
  
  void resizeEvent(QResizeEvent *event) {
    if (scene())
      scene()->setSceneRect(
                            QRect(QPoint(0, 0), event->size()));
    QGraphicsView::resizeEvent(event);
  }
  
  friend class MyOtherWidget;
};

MyView * MyView::INSTANCE = nullptr;


class MyScene : public QGraphicsScene {
  Q_OBJECT
public:
  static MyScene * INSTANCE;
  QWidget * uiWidget;
  ShapeWrapperPtr shape;
  ProgramPtr program;

  
  static QDialog * createDialog() {
    QDialog * dialog = new QDialog(0, Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    dialog->setWindowOpacity(0.0);
    dialog->setWindowTitle("Test");
    dialog->setLayout(new QVBoxLayout());
    return dialog;
  }

  MyScene() {
    INSTANCE = this;
    uiWidget = createDialog();
    for (int i = 0; i < 20; ++i) {
      QPushButton * button = new QPushButton("Press me");
      uiWidget->layout()->addWidget(button);
      connect(button, SIGNAL(pressed()), this, SLOT(customSlot()));
    }
    uiWidget->resize(640, 480);
    addWidget(uiWidget)->setPos(0, 0);

    //    dialog = createDialog();
    //    dialog->layout()->addWidget(new QLabel("Use mouse wheel to zoom model, and click and drag to rotate model"));
    //    dialog->layout()->addWidget(new QLabel("Move the sun around to change the light position"));
    //    addWidget(dialog);

    foreach (QGraphicsItem *item, items()) {
      item->setFlag(QGraphicsItem::ItemIsMovable);
      item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }
    
  }
  
  void drawBackground(QPainter *painter, const QRectF & rect) {
    
    auto type = painter->paintEngine()->type();
    if (type != QPaintEngine::OpenGL && type != QPaintEngine::OpenGL2) {
      qWarning("OpenGLScene: drawBackground needs a "
               "QGLWidget to be set as viewport on the "
               "graphics view");
      return;
    }

    static bool init = false;
    if (!init) {
      init = true;
      glewExperimental = true;
      glewInit();
      glGetError();
      DefaultFramebuffer().Bind(Framebuffer::Target::Draw);
      
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
      GLuint unusedIds = 0;
      if (glDebugMessageCallback) {
        glDebugMessageCallback(oria::debugCallback, this);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, true);
      }
      glGetError();
      
//      program = oria::loadProgram(
//                                  Resource::SHADERS_TEXTURED_VS,
//                                  Resource::SHADERS_TEXTURED_FS);
//      shape = oria::loadPlane(program, 1.0f);
    }

    Context::Viewport(0, 0, 640, 480);
    Context::ClearColor(1, 0, 1, 1);
    Context::Clear().ColorBuffer();

//    uiWidget->setWindowOpacity(1.0);
//    QPixmap pixmap = uiWidget->grab();
//    glWidget->context()->bindTexture(pixmap);
//    uiWidget->setWindowOpacity(0.0);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
#define S 0.5f
    glTexCoord2f(0, 0);
    glVertex3f(-S, -S, 0);
    glTexCoord2f(1, 0);
    glVertex3f(S, -S, 0);
    glTexCoord2f(1, 1);
    glVertex3f(S, S, 0);
    glTexCoord2f(0, 1);
    glVertex3f(-S, S, 0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    
    QTimer::singleShot(20, this, SLOT(update()));
  }
  
  private slots:
  void customSlot() {
    SAY("Click");
  }
};

MyScene * MyScene::INSTANCE = nullptr;

//class MyOtherWidget : public QWidget {
//  
//public:
//  bool event(QEvent * event) {
//    auto t = event->type();
//    MyView * view = MyView::INSTANCE;
//    QMouseEvent * me = (QMouseEvent *)event;
//    switch (t) {
//      case QEvent::MouseButtonPress:
//      case QEvent::MouseButtonRelease:
//        QMouseEvent ne(t, me->localPos(), view->mapToGlobal(me->localPos().toPoint()), me->button(), me->buttons(), me->modifiers());
//        QApplication::sendEvent(view, &ne);
//        return true;
//        
//
////      default:
////        return QWidget::event(event);
//    }
//    return QWidget::event(event);
//  }
//};

class MyApp {
public:

  int run() {
    static int argc = 0;
    static char ** argv = nullptr;
    QApplication app(argc, argv);
    
    QGLFormat glFormat;
//    glFormat.setVersion( 3, 2 );
//    glFormat.setProfile( QGLFormat::CoreProfile );
    glFormat.setSampleBuffers( true );

    glWidget = new OculusWidget(glFormat);
    
    MyView view;
    view.setViewport(glWidget);
    view.setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view.setScene(new MyScene());
    view.show();
    view.resize(640, 480);
    
//    MyOtherWidget other;
//    other.resize(640, 480);
//    other.show();
    
    
    return app.exec();


    
    
    
//    QWidget window;
//    window.resize(640, 480);
//    window.setWindowTitle("Child widget");
//    {
//      //! [create, position and show]
//      QPushButton *button = new QPushButton(
//        QApplication::translate("childwidget", "Press me"), &window);
//      button->move(100, 100);
//      button->show();
//    }

//    OculusWidget glWidget(glFormat, &window);
//    glWidget.resize(640, 480);
//    glWidget.setFocusPolicy(Qt::NoFocus);
//    glWidget.setAttribute(Qt::WA_ShowWithoutActivating);
//    glWidget.show();

    return app.exec();
  }
};

RUN_APP(MyApp);

#include "Example_Qt.moc"

#endif

