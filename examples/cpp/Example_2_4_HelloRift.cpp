#include "Common.h"


class HelloRift : public GlfwApp {
protected:
  ovrHmd                  hmd{ 0 };
  bool                    debugDevice{ false };
  ovrTexture              textures[2];
  ovrVector3f             eyeOffsets[2];
  FramebufferWrapperPtr   eyeFramebuffers[2];
  glm::mat4               eyeProjections[2];

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
    static const glm::vec3 EYE = glm::vec3(0, eyeHeight, ipd * 5.0f);
    static const glm::vec3 LOOKAT = glm::vec3(0, eyeHeight, 0);
    player = glm::inverse(glm::lookAt(EYE, LOOKAT, Vectors::UP));
    ovrHmd_RecenterPose(hmd);
  }

  virtual void finishFrame() {
    /**
     * The parent class calls glfwSwapBuffers in finishFrame,
     * but with the Oculus SDK, the SDK it responsible for buffer
     * swapping, so we have to override the method and ensure it
     * does nothing, otherwise the dual buffer swaps will
     * cause a constant flickering of the display.
     */
  }

  virtual GLFWwindow * createRenderingTarget(glm::uvec2 & outSize, glm::ivec2 & outPosition) {
    /**
     * In the Direct3D examples in the Oculus SDK, they make the point that the
     * onscreen window size does not need to match the Rift resolution.  However
     * this doesn't currently work in OpenGL, so we have to create the window at
     * the full resolution of the BackBufferSize.
     */
    return ovr::createRiftRenderingWindow(hmd, outSize, outPosition);
  }

  void initGl() {
    GlfwApp::initGl();

    ovrFovPort eyeFovPorts[2];
    for_each_eye([&](ovrEyeType eye){
      ovrTextureHeader & eyeTextureHeader = textures[eye].Header;
      eyeFovPorts[eye] = hmd->DefaultEyeFov[eye];
      eyeTextureHeader.TextureSize = ovrHmd_GetFovTextureSize(hmd, eye, hmd->DefaultEyeFov[eye], 1.0f);
      eyeTextureHeader.RenderViewport.Size = eyeTextureHeader.TextureSize;
      eyeTextureHeader.RenderViewport.Pos.x = 0;
      eyeTextureHeader.RenderViewport.Pos.y = 0;
      eyeTextureHeader.API = ovrRenderAPI_OpenGL;

      eyeFramebuffers[eye] = FramebufferWrapperPtr(new FramebufferWrapper());
      eyeFramebuffers[eye]->init(ovr::toGlm(eyeTextureHeader.TextureSize));
      ((ovrGLTexture&)textures[eye]).OGL.TexId = oglplus::GetName(eyeFramebuffers[eye]->color);
    });

    ovrGLConfig cfg;
    memset(&cfg, 0, sizeof(ovrGLConfig));
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.Multisample = 1;

    /**
     * In the Direct3D examples in the Oculus SDK, they make the point that the
     * onscreen window size does not need to match the Rift resolution.  However
     * this doesn't currently work in OpenGL, so we have to create the window at
     * the full resolution of the Rift and ensure that we use the same
     * size here when setting the BackBufferSize.
     */
    cfg.OGL.Header.BackBufferSize = ovr::fromGlm(getSize());

    ON_LINUX([&]{
      cfg.OGL.Disp = (Display*)glfw::getNativeDisplay(getWindow());
    });

    int distortionCaps = 0
        | ovrDistortionCap_TimeWarp
        | ovrDistortionCap_Vignette;

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
      eyeOffsets[eye] = eyeRenderDescs[eye].HmdToEyeViewOffset;
      eyeProjections[eye] = ovr::toGlm(
          ovrMatrix4f_Projection(eyeFovPorts[eye], 0.01f, 1000.0f, true));
    });
  }

  void onKey(int key, int scancode, int action, int mods) {
    if (oria::clearHSW(hmd)) {
      return;
    }

    if (CameraControl::instance().onKey(key, scancode, action, mods)) {
      return;
    }

    if (GLFW_PRESS != action) {
      int caps = ovrHmd_GetEnabledCaps(hmd);
      switch (key) {
      case GLFW_KEY_V:
        if (caps & ovrHmdCap_NoVSync) {
          ovrHmd_SetEnabledCaps(hmd, caps & ~ovrHmdCap_NoVSync);
        } else {
          ovrHmd_SetEnabledCaps(hmd, caps | ovrHmdCap_NoVSync);
        }
        return;
      case GLFW_KEY_P:
        if (caps & ovrHmdCap_LowPersistence) {
          ovrHmd_SetEnabledCaps(hmd, caps & ~ovrHmdCap_LowPersistence);
        } else {
          ovrHmd_SetEnabledCaps(hmd, caps | ovrHmdCap_LowPersistence);
        }
        return;
      case GLFW_KEY_R:
        resetPosition();
        return;
      }
    }

    GlfwApp::onKey(key, scancode, action, mods);
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

    for (int i = 0; i < 2; ++i) {
      ovrEyeType eye = hmd->EyeRenderOrder[i];

      const ovrRecti & vp = textures[eye].Header.RenderViewport;
      eyeFramebuffers[eye]->Bind();
      oglplus::Context::Viewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
      Stacks::projection().top() = eyeProjections[eye];

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
    oria::renderManikinScene(ipd, eyeHeight);
  }
};

RUN_APP(HelloRift);

