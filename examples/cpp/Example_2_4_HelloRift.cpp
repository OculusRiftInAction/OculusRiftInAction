#include "Common.h"

struct EyeArgs {
  glm::mat4                 projection;
  FramebufferWrapper        framebuffer;
};

class HelloRift : public GlfwApp {
protected:
  ovrHmd                  hmd{ 0 };
  bool                    debugDevice{ false };
  EyeArgs                 perEyeArgs[2];
  ovrTexture              textures[2];
  ovrVector3f             eyeOffsets[2];

  float                   eyeHeight{ OVR_DEFAULT_EYE_HEIGHT };
  float                   ipd{ OVR_DEFAULT_IPD };
  glm::mat4               player;

public:
  HelloRift() {
    ovr_Initialize();
    hmd = ovrHmd_Create(0);
    if (nullptr == hmd) {
      debugDevice = true;
      hmd = ovrHmd_CreateDebug(ovrHmd_DK2);
    }

    ovrHmd_ConfigureTracking(hmd,
      ovrTrackingCap_Orientation |
      ovrTrackingCap_Position, 0);
    resetPosition();
  }

  ~HelloRift() {
    ovrHmd_Destroy(hmd);
    hmd = 0;
  }

  virtual void resetPosition() {
    //eyeHeight = 0;
    static const glm::vec3 EYE = glm::vec3(0, eyeHeight, ipd * 5.0f);
    static const glm::vec3 LOOKAT = glm::vec3(0, eyeHeight, 0);
    player = glm::inverse(glm::lookAt(EYE, LOOKAT, Vectors::UP));
    ovrHmd_RecenterPose(hmd);
  }

  virtual void finishFrame() {
    /*
     * The parent class calls glfwSwapBuffers in finishFrame,
     * but with the Oculus SDK, the SDK it responsible for buffer
     * swapping, so we have to override the method and ensure it
     * does nothing, otherwise the dual buffer swaps will
     * cause a constant flickering of the display.
     */
  }

  virtual GLFWwindow * createRenderingTarget(glm::uvec2 & outSize, glm::ivec2 & outPosition) {
    GLFWwindow * window = nullptr;
    bool directHmdMode = false;
    
    outPosition = glm::ivec2(hmd->WindowsPos.x, hmd->WindowsPos.y);
    outSize = glm::uvec2(hmd->Resolution.w, hmd->Resolution.h);
    
    // The ovrHmdCap_ExtendDesktop only reliably reports on Windows currently
    ON_WINDOWS([&]{
      directHmdMode = (0 == (ovrHmdCap_ExtendDesktop & hmd->HmdCaps));
    });

    // In direct HMD mode, we always use the native resolution, because the
    // user has no control over it.
    // In legacy mode, we should be using the current resolution of the Rift
    // (unless we're creating a 'fullscreen' window)
    if (!directHmdMode) {
      GLFWmonitor * monitor = glfw::getMonitorAtPosition(outPosition);
      if (nullptr != monitor) {
        auto mode = glfwGetVideoMode(monitor);
        outSize = glm::uvec2(mode->width, mode->height);
      }
    }

    // On linux it's recommended to leave the screen in it's default portrait orientation.
    // The SDK currently allows no mechanism to test if this is the case.  I could query
    // GLFW for the current resolution of the Rift, but that sounds too much like actual
    // work.
    ON_LINUX([&]{
      std::swap(outSize.x, outSize.y);
    });

    if (directHmdMode && !debugDevice) {
      // In direct mode, try to put the output window on a secondary screen
      // (for easier debugging, assuming your dev environment is on the primary)
      window = glfw::createSecondaryScreenWindow(outSize);
    } else {
      // If we're creating a desktop window, we should strip off any window decorations
      // which might change the location of the rendered contents relative to the lenses.
      //
      // Another alternative would be to create the window in fullscreen mode, on
      // platforms that support such a thing.
      glfwWindowHint(GLFW_DECORATED, 0);
      window = glfw::createWindow(outSize, outPosition);
    }

    // If we're in direct mode, attach to the window
    if (directHmdMode) {
      void * nativeWindowHandle = glfw::getNativeWindowHandle(window);
      if (nullptr != nativeWindowHandle) {
        ovrHmd_AttachToWindow(hmd, nativeWindowHandle, nullptr, nullptr);
      }
    }

    return window;
  }

  void initGl() {
    GlfwApp::initGl();
    
    ovrFovPort eyeFovPorts[2];
    for_each_eye([&](ovrEyeType eye){
      EyeArgs & eyeArgs = perEyeArgs[eye];
      ovrTextureHeader & eyeTextureHeader = textures[eye].Header;
      eyeFovPorts[eye] = hmd->DefaultEyeFov[eye];
      eyeTextureHeader.TextureSize = ovrHmd_GetFovTextureSize(hmd, eye, hmd->DefaultEyeFov[eye], 1.0f);
      eyeTextureHeader.RenderViewport.Size = eyeTextureHeader.TextureSize;
      eyeTextureHeader.RenderViewport.Pos.x = 0;
      eyeTextureHeader.RenderViewport.Pos.y = 0;
      eyeTextureHeader.API = ovrRenderAPI_OpenGL;

      eyeArgs.framebuffer.init(ovr::toGlm(eyeTextureHeader.TextureSize));
      ((ovrGLTexture&)textures[eye]).OGL.TexId = oglplus::GetName(*(eyeArgs.framebuffer.color));
    });

    ovrGLConfig cfg;
    memset(&cfg, 0, sizeof(ovrGLConfig));
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.Multisample = 1;
    cfg.OGL.Header.RTSize = ovr::fromGlm(getSize());

    // FIXME Doesn't work as expected in OpenGL
    //if (0 == (ovrHmd_GetEnabledCaps(hmd) & ovrHmdCap_ExtendDesktop)) {
    //  cfg.OGL.Header.RTSize.w /= 4;
    //  cfg.OGL.Header.RTSize.h /= 4;
    //}

    ON_LINUX([&]{
      cfg.OGL.Disp = glfwGetX11Display();
      cfg.OGL.Win = glfwGetX11Window(window);
    });

    int distortionCaps =
      ovrDistortionCap_TimeWarp |
      ovrDistortionCap_Chromatic |
      ovrDistortionCap_Vignette;

    ON_LINUX([&]{
      distortionCaps |= ovrDistortionCap_LinuxDevFullscreen;
    });

    ovrEyeRenderDesc              eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg.Config,
      distortionCaps, eyeFovPorts, eyeRenderDescs);
    if (!configResult) {
      FAIL("Unable to configure SDK based distortion rendering");
    }

    for_each_eye([&](ovrEyeType eye){
      EyeArgs & eyeArgs = perEyeArgs[eye];
      eyeOffsets[eye] = eyeRenderDescs[eye].HmdToEyeViewOffset;
      eyeArgs.projection = ovr::toGlm(
        ovrMatrix4f_Projection(eyeFovPorts[eye], 0.01f, 1000.0f, true));
    });
  }

  void onKey(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
      static ovrHSWDisplayState hswDisplayState;
      ovrHmd_GetHSWDisplayState(hmd, &hswDisplayState);
      if (hswDisplayState.Displayed) {
        ovrHmd_DismissHSWDisplay(hmd);
        return;
      }
    }

    if (CameraControl::instance().onKey(key, scancode, action, mods)) {
      return;
    }

    if (GLFW_PRESS != action) {
      GlfwApp::onKey(key, scancode, action, mods);
      return;
    }

    int caps = ovrHmd_GetEnabledCaps(hmd);
    switch (key) {
    case GLFW_KEY_V:
      if (caps & ovrHmdCap_NoVSync) {
        ovrHmd_SetEnabledCaps(hmd, caps & ~ovrHmdCap_NoVSync);
      } else {
        ovrHmd_SetEnabledCaps(hmd, caps | ovrHmdCap_NoVSync);
      }
      break;

    case GLFW_KEY_P:
      if (caps & ovrHmdCap_LowPersistence) {
        ovrHmd_SetEnabledCaps(hmd, caps & ~ovrHmdCap_LowPersistence);
      }
      else {
        ovrHmd_SetEnabledCaps(hmd, caps | ovrHmdCap_LowPersistence);
      }
      break;

    case GLFW_KEY_R:
      resetPosition();
      break;

    default:
      GlfwApp::onKey(key, scancode, action, mods);
      break;
    }
  }

  virtual void update() {
    CameraControl::instance().applyInteraction(player);
    Stacks::modelview().top() = glm::inverse(player);
  }

  void draw() {
    static int frameIndex = 0;
    static ovrPosef eyePoses[2];
    ++frameIndex;
    ovrHmd_GetEyePoses(hmd, frameIndex, eyeOffsets, eyePoses, nullptr);

    ovrHmd_BeginFrame(hmd, frameIndex);
    glEnable(GL_DEPTH_TEST);

    for( int i = 0; i < 2; ++i) {
      ovrEyeType eye = hmd->EyeRenderOrder[i];
      EyeArgs & eyeArgs = perEyeArgs[eye];

      const ovrRecti & vp = textures[eye].Header.RenderViewport;
      eyeArgs.framebuffer.Bind();
      oglplus::Context::Viewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
      
      Stacks::projection().top() = eyeArgs.projection;
      MatrixStack & mv = Stacks::modelview();
      mv.withPush([&]{
        // Apply the per-eye offset & the head pose
        mv.top() = glm::inverse(ovr::toGlm(eyePoses[eye])) * mv.top();
        renderScene();
      });
    };
    oglplus::DefaultFramebuffer().Bind(oglplus::Framebuffer::Target::Draw);

    ovrHmd_EndFrame(hmd, eyePoses, textures);
  }

  virtual void renderScene() {
    oglplus::Context::Clear().DepthBuffer().ColorBuffer();
    oria::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    oria::renderFloor();

    // Scale the size of the cube to the distance between the eyes
    MatrixStack & mv = Stacks::modelview();

    mv.withPush([&]{
      mv.translate(glm::vec3(0, eyeHeight, 0)).scale(glm::vec3(ipd));
      oria::renderColorCube();
    });

    mv.withPush([&]{
      mv.translate(glm::vec3(0, 0, ipd * -5.0));

      oglplus::Context::Disable(oglplus::Capability::CullFace);
      oria::renderManikin();
    });

    //MatrixStack & mv = Stacks::modelview();
    //mv.with_push([&]{
    //  mv.translate(glm::vec3(0, 0, ipd * -5));
    //  GlUtils::renderManikin();
    //});
  }
};

RUN_APP(HelloRift);

