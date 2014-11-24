#include "Common.h"
#include <oglplus/shapes/plane.hpp>
#include <regex>

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

namespace shadertoy {
  const int MAX_CHANNELS = 4;

  enum class ChannelInputType {
    TEXTURE, CUBEMAP, AUDIO, VIDEO,
  };

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

  static std::string getChannelInputName(ChannelInputType type, int index) {
    switch (type) {
    case ChannelInputType::TEXTURE:
      return Platform::format("Tex%02d", index);
    case ChannelInputType::CUBEMAP:
      return Platform::format("Cube%02d", index);
    default:
      return "";
    }
  }

  static Resource getChannelInputResource(ChannelInputType type, int index) {
    switch (type) {
    case ChannelInputType::TEXTURE:
      return TEXTURES[index];
    case ChannelInputType::CUBEMAP:
      return CUBEMAPS[index];
    default:
      return NO_RESOURCE;
    }
  }

  enum UiState {
    INACTIVE,
    EDIT,
    CHANNEL,
    LOAD,
    SAVE,
  };

  static void setButtonImage(CEGUI::Window * pButton, const std::string & imageName) {
    pButton->setProperty("NormalImage", imageName);
    pButton->setProperty("PushedImage", imageName);
    pButton->setProperty("HoverImage", imageName);
  }

  class EditUi : public ui::Wrapper {
  protected:
    UiState uiState{ INACTIVE };
    static const char * WINDOW_EDIT;
    static const char * WINDOW_CHANNELS;
    static const char * WINDOW_LOAD;
    static const char * WINDOW_SAVE;

    std::function<void(int, ChannelInputType, int)> channelFunction;
    std::function<bool(const std::string & source)> sourceFunction;
    std::function<void(Resource res)> loadPresetFunction;

    struct EditWindow {
      CEGUI::Window * pWindow;
      EditUi & parent;
      EditWindow(EditUi & parent) : parent(parent) {
      }

      void init(CEGUI::Window * rootWindow) {
        using namespace CEGUI;
        WindowManager & wmgr = WindowManager::getSingleton();
        rootWindow->addChild(wmgr.loadLayoutFromFile("ShaderEdit.layout"));
        pWindow = rootWindow->getChild(WINDOW_EDIT);
        Window * pShaderText = pWindow->getChild("ShaderText");
        FontManager::getSingleton().createFromFile("Inconsolata-14.font");
        pShaderText->setFont("Inconsolata-14");
        pShaderText->setText(parent.fragmentSource);
        Window * pStatus = pWindow->getChild("Status");
        pStatus->setFont("Inconsolata-14");
        pStatus->setTextParsingEnabled(true);

        // Run button
        Window * pRunButton = pWindow->getChild("ButtonRun");
        pRunButton->subscribeEvent(PushButton::EventClicked, [=](const EventArgs& e) -> bool {
          if (!parent.sourceFunction(pShaderText->getText().c_str())) {
            //            pStatus->setText(lastError.c_str());
            pStatus->setText("FIXME error handling");
          }
          else {
            pStatus->setText("Success");
          }
          return true;
        });

        // Load button
        Window * pLoadButton = pWindow->getChild("ButtonLoad");
        pLoadButton->subscribeEvent(PushButton::EventClicked, [=](const EventArgs& e) -> bool {
          parent.setUiState(LOAD);
          return true;
        });

        // Save button
        Window * pSaveButton = pWindow->getChild("ButtonSave");
        pSaveButton->subscribeEvent(PushButton::EventClicked, [=](const EventArgs& e) -> bool {
          parent.setUiState(SAVE);
          return true;
        });

        // Channel buttons
        for (int i = 0; i < MAX_CHANNELS; ++i) {
          std::string controlName = Platform::format("ButtonC%d", i);
          Window * pButton = pWindow->getChild(controlName);
          pButton->subscribeEvent(PushButton::EventClicked, [=](const EventArgs& e) -> bool {
            pWindow->setUserData((void*)i);
            parent.setUiState(CHANNEL);
            return true;
          });
        }
      }
    };

    struct ChannelWindow {
      CEGUI::Window * pWindow;
      EditUi & parent;
      ChannelWindow(EditUi & parent) : parent(parent) {
      }


      void setupButton(ChannelInputType type, int index) {
        using namespace CEGUI;
        std::string str = getChannelInputName(type, index);
        ImageManager & im = ImageManager::getSingleton();
        if (pWindow->isChild(str)) {
          std::string imageName = "Image" + str;
          Resource res = getChannelInputResource(type, index);
          Window * pChannelButton = pWindow->getChild(str);
          std::string file = Resources::getResourcePath(res);
          im.addFromImageFile(imageName, file, "resources");
          setButtonImage(pChannelButton, imageName);
          pChannelButton->subscribeEvent(PushButton::EventClicked, [=](const EventArgs& e) -> bool {
            size_t channel = (size_t)pWindow->getUserData();
            parent.channelFunction(channel, type, index);
            parent.setUiState(EDIT);
            return true;
          });
        }
      }

      void init(CEGUI::Window * rootWindow) {
        using namespace CEGUI;
        WindowManager & wmgr = WindowManager::getSingleton();
        rootWindow->addChild(wmgr.loadLayoutFromFile("ShaderChannels.layout"));
        pWindow = rootWindow->getChild(WINDOW_CHANNELS);
        ImageManager & im = ImageManager::getSingleton();

        for (int i = 0; i < MAX_TEXTURES; ++i) {
          setupButton(ChannelInputType::TEXTURE, i);
        }

        for (int i = 0; i < MAX_CUBEMAPS; ++i) {
          setupButton(ChannelInputType::CUBEMAP, i);
        }
      }
    };

    struct LoadWindow {
      CEGUI::Window * pWindow;
      EditUi & parent;
      LoadWindow(EditUi & parent) : parent(parent) {
      }

      void init(CEGUI::Window * rootWindow) {
        using namespace CEGUI;
        WindowManager & wmgr = WindowManager::getSingleton();
        rootWindow->addChild(wmgr.loadLayoutFromFile("ShaderLoad.layout"));
        pWindow = rootWindow->getChild(WINDOW_LOAD);
        ImageManager & im = ImageManager::getSingleton();
        Listbox * lb = (Listbox*)pWindow->getChild("ListboxPresets");
        const CEGUI::Image* sel_img = &ImageManager::getSingleton().get("TaharezLook/MultiListSelectionBrush");
        for (int i = 0; Resource::NO_RESOURCE != shadertoy::PRESETS[i].res; ++i) {
          ListboxTextItem * itm = new ListboxTextItem(shadertoy::PRESETS[i].name, i, (void*)PRESETS[i].res, false);
          itm->setSelectionBrushImage(sel_img);
          lb->addItem(itm);
        }
        lb->subscribeEvent(Listbox::EventSelectionChanged, [=](const EventArgs& e) -> bool {
          ListboxItem * itm = lb->getFirstSelectedItem();
          if (nullptr != itm) {
            pWindow->setUserData(itm->getUserData());
          }
          else {
            pWindow->setUserData(nullptr);
          }
          return true;
        });

        Window * pButtonOk = pWindow->getChild("ButtonOk");
        pButtonOk->subscribeEvent(PushButton::EventClicked, [=](const EventArgs& e) -> bool {
          void * selected = pWindow->getUserData();
          if (nullptr != selected) {
            Resource res = (Resource)(int)selected;
            // FIXME, figure out a good dialog return value mechanism
            pWindow->getParent()->getChild(WINDOW_EDIT)->getChild("ShaderText")->setText(Platform::getResourceData(res).c_str());
            parent.loadPresetFunction(res);
          }
          pWindow->setUserData(nullptr);
          parent.setUiState(EDIT);
          return true;
        });
        Window * pButtonCancel = pWindow->getChild("ButtonCancel");
        pButtonOk->subscribeEvent(PushButton::EventClicked, [=](const EventArgs& e) -> bool {
          pWindow->setUserData(nullptr);
          parent.setUiState(EDIT);
          return true;
        });

      }
    };

    EditWindow editWindow;
    ChannelWindow channelWindow;
    LoadWindow loadWindow;

  public:
    std::string fragmentSource;

    EditUi() : editWindow(*this), channelWindow(*this), loadWindow(*this) {}

    void init(
      const std::string & fragmentSource, 
      std::function<void(int, ChannelInputType, int)> channelFunction, 
      std::function<bool(const std::string & source)> sourceFunction,
      std::function<void(Resource res)> loadPresetFunction) {
      this->channelFunction = channelFunction;
      this->sourceFunction = sourceFunction;
      this->loadPresetFunction = loadPresetFunction;
      ui::Wrapper::init(UI_SIZE, std::function<void()>([&]{
        using namespace CEGUI;
        WindowManager & wmgr = WindowManager::getSingleton();
        auto rootWindow = getWindow();
        // Set up the editor window
        editWindow.init(rootWindow);
        editWindow.pWindow->getChild("ShaderText")->setText(fragmentSource.c_str());
        // Set up the channel texture selection dialog
        channelWindow.init(rootWindow);
        // Set up the preset / user shader loading window
        loadWindow.init(rootWindow);

        setUiState(INACTIVE);
        System::getSingleton().getDefaultGUIContext().setRootWindow(rootWindow);
      }));
    }

    CEGUI::Window * getEditWindow() {
      return getWindow()->getChild(WINDOW_EDIT);
    }

    bool uiVisible() {
      return uiState != INACTIVE;
    }

    void setUiState(UiState newState) {
      auto rootWindow = getWindow();
      int childCount = rootWindow->getChildCount();
      for (int i = 0; i < childCount; ++i) {
        rootWindow->getChildAtIdx(i)->hide();
      }

      switch (newState) {
      case INACTIVE:
        break;
      case EDIT:
        rootWindow->getChild(WINDOW_EDIT)->show();
        break;
      case CHANNEL:
        rootWindow->getChild(WINDOW_CHANNELS)->show();
        break;
      case LOAD:
        rootWindow->getChild(WINDOW_LOAD)->show();
        break;
      case SAVE:
        break;
      }
      uiState = newState;
    }

    void setChannelButton(int channel, ChannelInputType type, int index) {
      std::string imageName = "Image" + getChannelInputName(type, index);
      std::string controlName = Platform::format("ButtonC%d", channel);
      CEGUI::Window * pButton = getWindow()->getChild(WINDOW_EDIT)->getChild(controlName);
      pButton->setProperty("NormalImage", imageName);
      pButton->setProperty("PushedImage", imageName);
      pButton->setProperty("HoverImage", imageName);
      Resource res = getChannelInputResource(type, index);

    }

    void clearChannelButton(int channel) {
      std::string controlName = Platform::format("ButtonC%d", channel);
      CEGUI::Window * pButton = getWindow()->getChild(WINDOW_EDIT)->getChild(controlName);
      pButton->setProperty("NormalImage", "");
      pButton->setProperty("PushedImage", "");
      pButton->setProperty("HoverImage", "");
    }
  };

  const char * EditUi::WINDOW_EDIT = "ShaderEdit";
  const char * EditUi::WINDOW_CHANNELS = "ShaderChannels";
  const char * EditUi::WINDOW_LOAD = "ShaderLoad";
  const char * EditUi::WINDOW_SAVE = "ShaderSave";
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
    "in vec3 iDir; // Direction from viewer\n"
    "out vec4 FragColor;\n";

  const char * CHANNEL_HEADER =
    "uniform sampler2D iChannel0;\n"
    "uniform sampler2D iChannel1;\n"
    "uniform sampler2D iChannel2;\n"
    "uniform sampler2D iChannel3;\n";

  const char * LINE_NUMBER_HEADER = 
    "#line 1\n";

  static const char * UNIFORM_RESOLUTION;
  static const char * UNIFORM_GLOBALTIME;
  static const char * UNIFORM_CHANNEL_TIME;
  static const char * UNIFORM_CHANNEL_RESOLUTION;
  static const char * UNIFORM_MOUSE_COORDS; 
  static const char * UNIFORM_DATE; 
  static const char * UNIFORM_SAMPLE_RATE;
  static const char * UNIFORM_POSITION;
  static const char * UNIFORM_CHANNELS[4];
  

  enum UiState {
    INACTIVE,
    EDIT,
    CHANNEL,
    LOAD,
    SAVE,
  };

  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_PLAYER_HEIGHT };
#ifdef RIFT
  float texRes{ 1.0f };
#endif

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
  

  // UI for editing the shader
  shadertoy::EditUi editUi;
  
  // The current fragment source
  std::string lastError;
  std::list<std::function<void()>> uniformLambdas;
  Channel channels[4];
  UiState uiState{ INACTIVE };

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
    editUi.init(Platform::getResourceData(Resource::SHADERTOY_SHADERS_DEFAULT_FS),
      [&](int channel, shadertoy::ChannelInputType type, int index) { setChannelInput(channel, type, index);  },
      [&](const std::string & source) -> bool { return setFragmentSource(source);  },
      [&](Resource res){ loadShaderResource(res); }
    );
  }


  virtual void initGl() {
    initUi();


    planeProgram = oria::loadProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);
    plane = oria::loadPlane(planeProgram, 1.0f);

    setFragmentSource(editUi.fragmentSource);
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

    shaderFramebuffer.init(ovr::toGlm(eyeTextures[0].Header.TextureSize));

    loadShaderResource(Resource::SHADERTOY_SHADERS_DEFAULT_FS);
    PARENT_CLASS::initGl();
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

         
  virtual bool setFragmentSource(const std::string & source) {
    using namespace oglplus;
    editUi.fragmentSource = source;
    try {
      if (!vertexShader) {
        vertexShader = VertexShaderPtr(new VertexShader());
        vertexShader->Source(Platform::getResourceData(Resource::SHADERTOY_SHADERS_DEFAULT_VS));
        vertexShader->Compile();
      }
      
      std::string header = SHADER_HEADER;
      for (int i = 0; i < 4; ++i) {
        const Channel & channel = channels[i];
        // "uniform sampler2D iChannel0;\n"
        std::string line = Platform::format("uniform sampler%s iChannel%d;\n",
          channel.target == Texture::Target::CubeMap ? "Cube" : "2D", i);
        header += line;
      }
      header += LINE_NUMBER_HEADER;
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
      lastError  = err.Log().c_str();
      SAY_ERR((const char*)err.Log().c_str());
    }
    return false;
  }

  virtual void setChannelInput(int channel, shadertoy::ChannelInputType type, int index) {
    using namespace oglplus;
    if (index < 0) {
      editUi.clearChannelButton(channel);
      channels[channel].texture.reset();
      channels[channel].target = Texture::Target::_2D;
      return;
    }

    editUi.setChannelButton(channel, type, index);
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
    std::set<std::string> activeUniforms = getActiveUniforms(skyboxProgram);
    skyboxProgram->Bind();
    //    UNIFORM_DATE;
    for (int i = 0; i < 4; ++i) {
      const char * uniformName = UNIFORM_CHANNELS[i];
      if (activeUniforms.count(uniformName)) {
        int loc = glGetUniformLocation(oglplus::GetName(*skyboxProgram), uniformName);
        glUniform1i(loc, i);
        // setting the active texture slot for the channels
        // Uniform<GLuint>(*cubeProgram, uniformName).Set(i);
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
        if (editUi.uiVisible()) {
          Uniform<vec3>(*skyboxProgram, UNIFORM_POSITION).Set(vec3(0));
        } else {
          Uniform<vec3>(*skyboxProgram, UNIFORM_POSITION).Set(ovr::toGlm(getEyePose().Position));
        }
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
    if (editUi.uiVisible()) {
      ui::handleGlfwMouseButtonEvent(button, action, mods);
    }
  }

  virtual void onMouseMove(double x, double y) {
    if (editUi.uiVisible()) {
      vec2 mousePosition(x, y);
      vec2 windowSize(getSize());
      mousePosition /= windowSize;
      mousePosition *= UI_SIZE;
      ui::handleGlfwMouseMoveEvent(mousePosition.x, mousePosition.y);
    }
  }

  virtual void onScroll(double x, double y) {
    if (editUi.uiVisible()) {
      ui::handleGlfwMouseScrollEvent(x, y);
    }
  }

  virtual void onCharacter(unsigned int codepoint) {
    if (editUi.uiVisible()) {
      ui::handleGlfwCharacterEvent(codepoint);
    }
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    if (editUi.uiVisible()) {
      if (GLFW_PRESS == action && GLFW_KEY_ESCAPE == key) {
        editUi.setUiState(shadertoy::UiState::INACTIVE);
      } 
      if (GLFW_PRESS == action && GLFW_MOD_CONTROL == mods) {
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
          // return;
        default: 
          break;
        }
      }
      ui::handleGlfwKeyboardEvent(key, scancode, action, mods);
      return;
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

      case GLFW_KEY_ESCAPE:
        editUi.setUiState(shadertoy::UiState::EDIT);
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
    editUi.update();
    if (fps.getCount() > 50) {
      float rate = fps.getRate();
      if (rate < 55) {
        SAY("%f %f", rate, texRes);
        texRes *= 0.9f;
      }
      editUi.getWindow()->getChild("ShaderEdit/TextFps")->setText(Platform::format("%2.1f", rate));
      editUi.getWindow()->getChild("ShaderEdit/TextRez")->setText(Platform::format("%2.1f", texRes));
      float mpx = renderSize().x * renderSize().y;
      mpx /= 1e6f;
      editUi.getWindow()->getChild("ShaderEdit/TextMpx")->setText(Platform::format("%02.2f", mpx));

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
      for (int i = 0; i < 4; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        DefaultTexture().Bind(Texture::Target::_2D);
        DefaultTexture().Bind(Texture::Target::CubeMap);
      }
      glActiveTexture(GL_TEXTURE0);
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
      //mv.top() = glm::inverse(player);
      if (editUi.uiVisible()) {
        mv.translate(vec3(0, OVR_DEFAULT_EYE_HEIGHT, -1));
        editUi.render();
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
