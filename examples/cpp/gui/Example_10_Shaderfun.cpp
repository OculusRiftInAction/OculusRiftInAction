#include "Common.h"
#include <oglplus/shapes/plane.hpp>

using namespace oglplus;

#define UI_X 1280
#define UI_Y 720
#define ASPECT (float)((float)UI_X/(float)UI_Y)

static uvec2 UI_SIZE(UI_X, UI_Y);

#define RIFT
#ifdef RIFT
#define PARENT_CLASS RiftApp
#else
#define PARENT_CLASS GlfwApp
glm::mat4 player;
#endif

enum ChannelInputType {
  TEXTURE,
  CUBEMAP,
  AUDIO,
  VIDEO,
};

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

template <typename F>
void withContext(GLFWwindow * newContext, F f) {
  GLFWwindow * save = glfwGetCurrentContext();
  glfwMakeContextCurrent(newContext);
  f();
  glfwMakeContextCurrent(save);
}



struct UiContainer {
  ProgramPtr program;
  ShapeWrapperPtr shape;
  FramebufferWrapper fbo;

  GLFWwindow * context;

  template <typename F>
  void init(F f) {
    glfwWindowHint(GLFW_VISIBLE, 0);
    context = glfwCreateWindow(100, 100, "", nullptr, glfwGetCurrentContext());
    glfwWindowHint(GLFW_VISIBLE, 1);
    withContext(context, [&]{
      Context::Disable(Capability::CullFace);
      fbo.init(UI_SIZE);
      fbo.Bound([&]{
        Context::Enable(Capability::Blend);
        Context::Disable(Capability::DepthTest);
        ui::initWindow(UI_SIZE);
        f();
      });
    });
    using namespace oglplus;
    program = oria::loadProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_TEXTURED_FS);
    shape = ShapeWrapperPtr(new shapes::ShapeWrapper({ "Position", "TexCoord" }, shapes::Plane(
      Vec3f(1, 0, 0),
      Vec3f(0, 1 / ASPECT, 0)
      ), *program));
  }

  void update() {
    withContext(context, [&]{
      fbo.Bound([]{
        glViewport(0, 0, UI_X, UI_Y);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
        glActiveTexture(GL_TEXTURE0);
        Context::ClearColor(0, 0, 0, 1);
        Context::Clear().ColorBuffer().DepthBuffer();
        Context::Enable(Capability::Blend);
        CEGUI::System::getSingleton().renderAllGUIContexts();
      });
    });
    glGetError();
  }

  void render() {
    Context::Enable(Capability::Blend);
    fbo.color->Bind(oglplus::Texture::Target::_2D);
    oria::renderGeometry(shape, program);
  }};







typedef std::shared_ptr<oglplus::VertexShader> VertexShaderPtr;
typedef std::shared_ptr<oglplus::FragmentShader> FragmentShaderPtr;

struct Channel {
  TexturePtr texture;
  oglplus::Texture::Target target;
  vec3 resolution;
};

class DynamicFramebufferScaleExample : public PARENT_CLASS {
  const char * SHADER_HEADER = "#version 330\n"
    "uniform vec3      iResolution;           // viewport resolution (in pixels)\n"
    "uniform float     iGlobalTime;           // shader playback time (in seconds)\n"
    "uniform float     iChannelTime[4];       // channel playback time (in seconds)\n"
    "uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)\n"
    "uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click\n"
    "uniform vec4      iDate;                 // (year, month, day, time in seconds)\n"
    "uniform float     iSampleRate;           // sound sample rate (i.e., 44100)\n"
    "uniform vec3      iPos; // Head position\n"
    "uniform sampler2D iChannel0;\n"
    "uniform sampler2D iChannel1;\n"
    "uniform sampler2D iChannel2;\n"
    "uniform sampler2D iChannel3;\n"
    "\n"
    "in vec3 iDir; // Direction from viewer\n"
    "out vec4 FragColor;\n";

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
  

  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_PLAYER_HEIGHT };
#ifdef RIFT
  float texRes{ 1.0f };
#endif
  ProgramPtr cubeProgram;
  ShapeWrapperPtr cube;
  VertexShaderPtr vertexShader;
  FragmentShaderPtr fragmentShader;
  UiContainer ui;
  std::string fragmentSource;
  std::function<void()> uniformLambda;
  Channel channels[4];
  CEGUI::FrameWindow * rootWindow;
  bool uiVisible{ false };
  std::set<std::string> activeUniforms;

public:
  DynamicFramebufferScaleExample() {
#ifdef RIFT
    ipd = ovrHmd_GetFloat(hmd, 
      OVR_KEY_IPD, 
      OVR_DEFAULT_IPD);

    eyeHeight = ovrHmd_GetFloat(hmd, 
      OVR_KEY_PLAYER_HEIGHT, 
      OVR_DEFAULT_PLAYER_HEIGHT);
#endif
    resetCamera();
  }

  virtual ~DynamicFramebufferScaleExample() {
  }

  uvec2 renderSize() {
    return uvec2(texRes * vec2(ovr::toGlm(eyeTextures[0].Header.TextureSize)));
  }

  virtual void onMouseEnter(int entered) {
    if (entered){
      glfwSetInputMode(getWindow(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    } else {
      glfwSetInputMode(getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
  }

  virtual void initGl() {
    setFragmentSource(Platform::getResourceData(Resource::SHADERS_SHADERTOY_FS));
    assert(cubeProgram);
    cube = oria::loadSkybox(cubeProgram);
    Platform::addShutdownHook([&]{
      cubeProgram.reset();
      cube.reset();
    });


    ui.init([&]{
      using namespace CEGUI;
      WindowManager & wmgr = WindowManager::getSingleton();
      rootWindow = dynamic_cast<FrameWindow *>(wmgr.createWindow("TaharezLook/FrameWindow", "root"));
      rootWindow->setFrameEnabled(false);
      rootWindow->setTitleBarEnabled(false);
      rootWindow->setAlpha(0.5f);
      rootWindow->setSize(CEGUI::USize(cegui_absdim(UI_X), cegui_absdim(UI_Y)));
      {
        rootWindow->addChild(wmgr.loadLayoutFromFile("ShaderEdit.layout"));
        Window * pShaderEdit = rootWindow->getChild("ShaderEdit/ShaderText");
        FontManager::getSingleton().createFromFile("Inconsolata-14.font");

        pShaderEdit->setFont("Inconsolata-14");
        pShaderEdit->setText(fragmentSource);
        Window * pWnd = rootWindow->getChild("ShaderEdit/ButtonRun");
        pWnd->subscribeEvent(PushButton::EventClicked, [=](const EventArgs& e) -> bool {
          setFragmentSource(pShaderEdit->getText().c_str());
          return true;
        });
      }

      {
        rootWindow->addChild(wmgr.loadLayoutFromFile("ShaderChannels.layout"));
        ImageManager & im = CEGUI::ImageManager::getSingleton();
        for (int i = 0; i < 17; ++i) {
          std::string str = Platform::format("Tex%02d", i);
          std::string buttonName = "ShaderChannels/" + str;
          if (rootWindow->isChild(buttonName)) {
            std::string imageName = "Image" + str;
            Resource res = TEXTURES[i];
            Window * pWnd = rootWindow->getChild(buttonName);
            std::string file = Resources::getResourcePath(res);
            im.addFromImageFile(imageName, file, "resources");
            pWnd->setProperty("NormalImage", imageName);
            pWnd->setProperty("PushedImage", imageName);
            pWnd->setProperty("HoverImage", imageName);
            pWnd->subscribeEvent(PushButton::EventClicked, [=](const EventArgs& e) -> bool {
              setChannelInput(0, TEXTURE, res);
              return true;
            });
          }
        }
        //rootWindow->getChild("ShaderChannels")->hide();
        rootWindow->getChild("ShaderEdit")->hide();
      }
      System::getSingleton().getDefaultGUIContext().setRootWindow(rootWindow);
    });

    PARENT_CLASS::initGl();
  }

  static std::string replaceAll(const std::string & input, const std::string & find, const std::string & replace) {
    std::string result = input;
    std::string::size_type find_size = find.size();
    std::string::size_type n = 0;
    while (std::string::npos != (n = result.find(find, n))) {
      result.replace(n, find_size, replace);
    }
    return result;
  }
           
  virtual void setFragmentSource(const std::string & source) {
    using namespace oglplus;
    fragmentSource = source;
    try {
      if (!vertexShader) {
        vertexShader = VertexShaderPtr(new VertexShader());
        vertexShader->Source(Platform::getResourceData(Resource::SHADERS_SHADERTOY_VS));
        vertexShader->Compile();
      }
      
      FragmentShaderPtr newFragmentShader(new FragmentShader());
      std::string newSource = replaceAll(source, "gl_FragColor", "FragColor");
      newFragmentShader->Source(GLSLSource(std::string(SHADER_HEADER) + newSource));
      newFragmentShader->Compile();
      ProgramPtr result(new Program());
      result->AttachShader(*vertexShader);
      result->AttachShader(*newFragmentShader);
      result->Link();
      cubeProgram.swap(result);
      fragmentShader.swap(newFragmentShader);
      size_t uniformCount = cubeProgram->ActiveUniforms().Size();
      for (int i = 0; i < uniformCount; ++i) {
        std::string name = cubeProgram->ActiveUniforms().At(i).Name().c_str();
        activeUniforms.insert(name);
      }
    }
    catch (ProgramBuildError & err) {
      SAY_ERR((const char*)err.Message);
    }
    updateUniforms();
  }

//public static Texture getTexture(Resource resource) {
//  Texture texture = new Texture(GL_TEXTURE_2D);
//  texture.bind();
//  texture.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//  texture.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//  texture.parameter(GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
//  texture.parameter(GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
//  texture.parameter(GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);
//  texture.loadImageData(Images.load(resource), GL_TEXTURE_2D);
//  texture.unbind();
//  return texture;
//}
//
//public void setTextureSource(Resource res, int index) {
//  assert(index >= 0);
//  assert(index < channels.length);
//  channels[index] = getTexture(res);
//  assert(0 == glGetError());
//  updateUniforms();
//}

  virtual void setChannelInput(int channel, ChannelInputType type, Resource res) {
    using namespace oglplus;
    Channel newChannel;
    uvec2 size;
    switch (type) {
    case TEXTURE:
      newChannel.texture = oria::load2dTexture(res, size);
      newChannel.target = Texture::Target::_2D;
      newChannel.resolution = vec3(size, 0);
      break;
    case CUBEMAP:
      newChannel.texture = oria::loadCubemapTexture(res);
      newChannel.target = Texture::Target::CubeMap;
      break;
    case VIDEO:
      break;
    case AUDIO:
      break;
    }
    
    channels[channel] = newChannel;
    updateUniforms();
  }

  void updateUniforms() {
    if (activeUniforms.count(UNIFORM_RESOLUTION)) {
      Uniform<vec3>(*cubeProgram, UNIFORM_RESOLUTION).Set(vec3(getSize(), 0));
    }
//    UNIFORM_DATE;
    for (int i = 0; i < 4; ++i) {
      std::string uniform =Platform::format("iChannel%d",i);
      if (activeUniforms.count(uniform)) {
        Uniform<GLuint>(*cubeProgram, uniform).Set(i);
      }
    }
    uniformLambda = [&]{
      if (activeUniforms.count(UNIFORM_GLOBALTIME)) {
        Uniform<GLfloat>(*cubeProgram, UNIFORM_GLOBALTIME).Set(Platform::elapsedSeconds());
      }
      if (activeUniforms.count(UNIFORM_POSITION)) {
        Uniform<vec3>(*cubeProgram, UNIFORM_POSITION).Set(ovr::toGlm(getEyePose().Position));
      }
      for (int i = 0; i < 4; ++i) {
        if (activeUniforms.count(UNIFORM_CHANNELS[i]) && channels[i].texture) {
          Texture::Active(i);
          channels[i].texture->Bind(channels[i].target);
        }
      }
    };
  }

  virtual void onMouseButton(int button, int action, int mods) { 
    if (uiVisible) {
      ui::handleGlfwMouseButtonEvent(button, action, mods);
    }
  }

  virtual void onMouseMove(double x, double y) {
    if (uiVisible) {
      vec2 mousePosition(x, y);
      vec2 windowSize(getSize());
      mousePosition /= windowSize;
      mousePosition *= UI_SIZE;
      ui::handleGlfwMouseMoveEvent(mousePosition.x, mousePosition.y);
    }
  }

  virtual void onScroll(double x, double y) {
    if (uiVisible) {
      ui::handleGlfwMouseScrollEvent(x, y);
    }
  }

  virtual void onCharacter(unsigned int codepoint) {
    if (uiVisible) {
      ui::handleGlfwCharacterEvent(codepoint);
    }
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    if (uiVisible) {
      if (GLFW_PRESS == action && GLFW_KEY_ESCAPE == key) {
        uiVisible = false;
        return;
      } else {
        ui::handleGlfwKeyboardEvent(key, scancode, action, mods);
        return;
      }
    }

    static const float ROOT_2 = sqrt(2.0f);
    static const float INV_ROOT_2 = 1.0f / ROOT_2;
    if (action == GLFW_PRESS) {
      switch (key) {
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
      case GLFW_KEY_R:
        resetCamera();
        break;

      case GLFW_KEY_SPACE:
        uiVisible = true;
        break;
      }
    } else {
      PARENT_CLASS::onKey(key, scancode, action, mods);
    }
  }

  void resetCamera() {
    player = glm::inverse(glm::lookAt(
      glm::vec3(0, eyeHeight, ipd * 2),  // Position of the camera
      glm::vec3(0, eyeHeight, 0),  // Where the camera is looking
      Vectors::Y_AXIS));           // Camera up axis
#ifdef RIFT
    ovrHmd_RecenterPose(hmd);
#endif
  }

  void update() {
    using namespace oglplus;
    PARENT_CLASS::update();
    ui.update();
  }

  void renderSkybox() {
    using namespace oglplus;
    cubeProgram->Bind();
    MatrixStack & mv = Stacks::modelview();
    mv.withPush([&]{
      mv.untranslate();
      Context::Disable(Capability::DepthTest);
      Context::Disable(Capability::CullFace);
      oria::renderGeometry(cube, cubeProgram, { uniformLambda });
      Context::Enable(Capability::CullFace);
      Context::Enable(Capability::DepthTest);
    });
    DefaultTexture().Bind(TextureTarget::CubeMap);
  }

  void renderScene() {
    Context::Clear().DepthBuffer();
    Context::Enable(Capability::Blend);
    Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);
    MatrixStack & mv = Stacks::modelview();
    mv.withPush([&]{
      mv.postMultiply(glm::inverse(player));
      renderSkybox();
      if (uiVisible) {
        mv.translate(vec3(0, OVR_DEFAULT_EYE_HEIGHT, -1));
        ui.render();
      }
    });
  }

#ifndef RIFT
  void draw() {
    Context::Viewport(0, 0, UI_X, UI_Y);
    Context::ClearColor(0, 0, 1, 1);
    Context::Clear().ColorBuffer().DepthBuffer();
    Stacks::projection().top() = glm::perspective(1.6f, ASPECT, 0.01f, 100.0f);
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
