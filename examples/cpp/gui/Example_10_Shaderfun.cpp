#include "Common.h"
#include <oglplus/shapes/plane.hpp>

using namespace oglplus;

#define UI_X 1280
#define UI_Y 720

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

typedef std::shared_ptr<oglplus::VertexShader> VertexShaderPtr;
typedef std::shared_ptr<oglplus::FragmentShader> FragmentShaderPtr;

struct Channel {
  TexturePtr texture;
  oglplus::Texture::Target target;
  vec3 resolution;
};

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
      start = ovr_GetTimeInSeconds();
    } else {
      ++count;
    }
  }

  float getRate() const {
    float elapsed = ovr_GetTimeInSeconds() - start;
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

  static const char * UNIFORM_RESOLUTION;
  static const char * UNIFORM_GLOBALTIME;
  static const char * UNIFORM_CHANNEL_TIME;
  static const char * UNIFORM_CHANNEL_RESOLUTION;
  static const char * UNIFORM_MOUSE_COORDS; 
  static const char * UNIFORM_DATE; 
  static const char * UNIFORM_SAMPLE_RATE;
  static const char * UNIFORM_POSITION;
  static const char * UNIFORM_CHANNELS[4];
  

  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_PLAYER_HEIGHT };
#ifdef RIFT
  float texRes{ 1.0f };
#endif

  // Renders the shader to a skybox
  ProgramPtr skyboxProgram;
  // Skybox geometry
  ShapeWrapperPtr skybox;

  ProgramPtr planeProgram;
  ShapeWrapperPtr plane;
  
  // Measure the FPS for use in dynamic scaling
  RateCounter fps;
  
  // The vertex shader, constant
  VertexShaderPtr vertexShader;

  // The fragment shader, which can be edited
  FragmentShaderPtr fragmentShader;
  
  // We actually render the shader to one FBO for dynamic framebuffer scaling,
  // while leaving the actual texture we pass to the Oculus SDK fixed.
  // This allows us to have a clear UI regardless of the shader performance
  FramebufferWrapper shaderFramebuffer;

  // UI for editing the shader
  ui::Wrapper uiWrapper;
  
  // The current fragment source
  std::string fragmentSource;
  std::list<std::function<void()>> uniformLambdas;
  Channel channels[4];
  bool uiVisible{ false };
  

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

  vec2 textureSize() {
    return vec2(ovr::toGlm(eyeTextures[0].Header.TextureSize));
  }
  
  uvec2 renderSize() {
    return uvec2(texRes * textureSize());
  }

  virtual void onMouseEnter(int entered) {
    if (entered){
      glfwSetInputMode(getWindow(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    } else {
      glfwSetInputMode(getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
  }

  virtual void initUi() {
    std::function<void()> initFunction([&]{
      using namespace CEGUI;
      WindowManager & wmgr = WindowManager::getSingleton();
      CEGUI::FrameWindow * rootWindow;
      rootWindow = dynamic_cast<FrameWindow *>(wmgr.createWindow("TaharezLook/FrameWindow", "root"));
      rootWindow->setFrameEnabled(false);
      rootWindow->setTitleBarEnabled(false);
      rootWindow->setAlpha(0.5f);
      rootWindow->setCloseButtonEnabled(false);
      rootWindow->setSize(CEGUI::USize(cegui_absdim(UI_X), cegui_absdim(UI_Y)));
      rootWindow->addChild(wmgr.loadLayoutFromFile("ShaderChannels.layout"));
      rootWindow->addChild(wmgr.loadLayoutFromFile("ShaderEdit.layout"));
      Window * pShaderEditor = rootWindow->getChild("ShaderEdit");
      Window * pShaderChannels = rootWindow->getChild("ShaderChannels");
      {
        Window * pShaderText = pShaderEditor->getChild("ShaderText");
        FontManager::getSingleton().createFromFile("Inconsolata-14.font");
        pShaderText->setFont("Inconsolata-14");
        pShaderText->setText(fragmentSource);
        Window * pRunButton = pShaderEditor->getChild("ButtonRun");
        pRunButton->subscribeEvent(PushButton::EventClicked, [=](const EventArgs& e) -> bool {
          setFragmentSource(pShaderText->getText().c_str());
          return true;
        });
        for (int i = 0; i < 4; ++i) {
          std::string controlName = Platform::format("ButtonC%d", i);
          Window * pButton = pShaderEditor->getChild(controlName);
          pButton->subscribeEvent(PushButton::EventClicked, [=](const EventArgs& e) -> bool {
            pShaderChannels->setUserData((void*)i);
            pShaderEditor->hide();
            pShaderChannels->show();
            return true;
          });
        }
      }
      
      {
        ImageManager & im = CEGUI::ImageManager::getSingleton();
        for (int i = 0; i < 17; ++i) {
          std::string str = Platform::format("Tex%02d", i);
          if (pShaderChannels->isChild(str)) {
            std::string imageName = "Image" + str;
            Resource res = TEXTURES[i];
            Window * pChannelButton = pShaderChannels->getChild(str);
            std::string file = Resources::getResourcePath(res);
            im.addFromImageFile(imageName, file, "resources");
            pChannelButton->setProperty("NormalImage", imageName);
            pChannelButton->setProperty("PushedImage", imageName);
            pChannelButton->setProperty("HoverImage", imageName);
            pChannelButton->subscribeEvent(PushButton::EventClicked, [=](const EventArgs& e) -> bool {
              size_t channel = (size_t)pShaderChannels->getUserData();
              std::string controlName = Platform::format("ButtonC%d", channel);
              Window * pButton = pShaderEditor->getChild(controlName);
              pButton->setProperty("NormalImage", imageName);
              pButton->setProperty("PushedImage", imageName);
              pButton->setProperty("HoverImage", imageName);
              setChannelInput(channel, TEXTURE, res);
              pShaderChannels->hide();
              pShaderEditor->show();
              return true;
            });
          }
        }
        pShaderChannels->hide();
      }
      System::getSingleton().getDefaultGUIContext().setRootWindow(rootWindow);
    });
    uiWrapper.init(UI_SIZE, initFunction);
  }
  
  virtual void initGl() {
    initUi();
    
    planeProgram = oria::loadProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);
    plane = oria::loadPlane(planeProgram, 1.0f);

    setFragmentSource(Platform::getResourceData(Resource::SHADERS_SHADERTOY_FS));
    assert(skyboxProgram);

    skybox = oria::loadSkybox(skyboxProgram);
    Platform::addShutdownHook([&]{
      planeProgram.reset();
      plane.reset();
      skyboxProgram.reset();
      skybox.reset();
    });
    
    shaderFramebuffer.init(ovr::toGlm(eyeTextures[0].Header.TextureSize));

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
      skyboxProgram.swap(result);
      fragmentShader.swap(newFragmentShader);
    }
    catch (ProgramBuildError & err) {
      SAY_ERR((const char*)err.Message);
    }
    updateUniforms();
  }

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

    std::set<std::string> activeUniforms = getActiveUniforms(skyboxProgram);
    skyboxProgram->Bind();
    //    UNIFORM_DATE;
    for (int i = 0; i < 4; ++i) {
      const char * uniformName = UNIFORM_CHANNELS[i];
      if (activeUniforms.count(uniformName)) {
        // setting the active texture slot for the channels
//        Uniform<GLuint>(*cubeProgram, uniformName).Set(i);
      }
    }
    if (activeUniforms.count(UNIFORM_RESOLUTION)) {
      vec3 textureSize = vec3(ovr::toGlm(this->eyeTextures[0].Header.TextureSize), 0);
      Uniform<vec3>(*skyboxProgram, UNIFORM_RESOLUTION).Set(textureSize);
    }
    NoProgram().Bind();

    uniformLambdas.clear();
    if (activeUniforms.count(UNIFORM_GLOBALTIME)) {
      uniformLambdas.push_back([&]{
        Uniform<GLfloat>(*skyboxProgram, UNIFORM_GLOBALTIME).Set(Platform::elapsedSeconds());
      });
    }
    if (activeUniforms.count(UNIFORM_POSITION)) {
      uniformLambdas.push_back([&]{
        Uniform<vec3>(*skyboxProgram, UNIFORM_POSITION).Set(ovr::toGlm(getEyePose().Position));
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
        if (GLFW_MOD_CONTROL == mods) {
          switch (key) {
          case GLFW_KEY_C:
            CEGUI::System::getSingleton().getDefaultGUIContext().injectCopyRequest();
            return;
          case GLFW_KEY_V:
            CEGUI::System::getSingleton().getDefaultGUIContext().injectPasteRequest();
            return;
          case GLFW_KEY_X:
            CEGUI::System::getSingleton().getDefaultGUIContext().injectCutRequest();
            return;
          case GLFW_KEY_A:
            // CEGUI::System::getSingleton().getDefaultGUIContext().injectinjectCutRequest();
            return;
          default: break;
          }
        }
        if (!ui::handleGlfwKeyboardEvent(key, scancode, action, mods)) {
          if (GLFW_RELEASE == action) {
            SAY("Key not accepted");
          }
        }
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
    uiWrapper.update();
    if (fps.getCount() > 50) {
      float rate = fps.getRate();
      if (rate < 55) {
        SAY("%f %f", rate, texRes);
        texRes *= 0.9f;
      }
      fps.reset();
    }
    fps.increment();
  }

  void renderSkybox() {
    using namespace oglplus;
    shaderFramebuffer.Bound([&]{
      viewport(renderSize());
      skyboxProgram->Bind();
      MatrixStack & mv = Stacks::modelview();
      mv.withPush([&]{
        mv.untranslate();
        oria::renderGeometry(skybox, skyboxProgram, uniformLambdas );
      });
      DefaultTexture().Bind(TextureTarget::CubeMap);
    });
    viewport(textureSize());
  }

  void renderScene() {
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
      }} );
    });
    
    mv.withPush([&]{
      mv.postMultiply(glm::inverse(player));
      if (uiVisible) {
        mv.translate(vec3(0, OVR_DEFAULT_EYE_HEIGHT, -1));
        uiWrapper.render();
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

const char * DynamicFramebufferScaleExample::UNIFORM_RESOLUTION = "iResolution";
const char * DynamicFramebufferScaleExample::UNIFORM_GLOBALTIME = "iGlobalTime";
const char * DynamicFramebufferScaleExample::UNIFORM_CHANNEL_TIME = "iChannelTime";
const char * DynamicFramebufferScaleExample::UNIFORM_CHANNEL_RESOLUTION = "iChannelResolution";
const char * DynamicFramebufferScaleExample::UNIFORM_MOUSE_COORDS = "iMouse";
const char * DynamicFramebufferScaleExample::UNIFORM_DATE = "iDate";
const char * DynamicFramebufferScaleExample::UNIFORM_SAMPLE_RATE = "iSampleRate";
const char * DynamicFramebufferScaleExample::UNIFORM_POSITION = "iPos";
const char * DynamicFramebufferScaleExample::UNIFORM_CHANNELS[4] = {
  "iChannel0",
  "iChannel1",
  "iChannel2",
  "iChannel3",
};


#ifdef RIFT
RUN_OVR_APP(DynamicFramebufferScaleExample);
#else
RUN_APP(DynamicFramebufferScaleExample);
#endif
