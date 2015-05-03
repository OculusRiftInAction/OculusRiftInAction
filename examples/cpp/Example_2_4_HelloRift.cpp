#include "Common.h"

class HelloRift : public GlfwApp {
protected:
  ovrHmd                  hmd{ 0 };
  bool                    debugDevice{ false };
  ovrVector3f             eyeOffsets[2];
  ovr::SwapTexFboPtr      eyeFramebuffers[2];
  glm::mat4               eyeProjections[2];
  ovrLayerEye_Union       submitLayers[1];

  float                   eyeHeight{ OVR_DEFAULT_EYE_HEIGHT };
  float                   ipd{ OVR_DEFAULT_IPD };
  glm::mat4               player;

public:
  HelloRift() {
    if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
      FAIL("Could not initialize Oculus SDK");
    }
    if (!OVR_SUCCESS(ovrHmd_Create(0, &hmd))) {
      SAY("Could not open HMD, attempting debug");
    }
    if (nullptr == hmd) {
      debugDevice = true;
      if (!OVR_SUCCESS(ovrHmd_CreateDebug(ovrHmd_DK2, &hmd))) {
        FAIL("Could not create debug HMD");
      }
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
    ovrLayerEyeFov & layer = submitLayers[0].EyeFov;
    layer.Header.Type = ovrLayerType_EyeFov;
    layer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
    for_each_eye([&](ovrEyeType eye){
      ovrFovPort & fov = layer.Fov[eye];
      fov = hmd->MaxEyeFov[eye];

      ovrRecti & vp = layer.Viewport[eye];
      vp.Pos = { 0, 0 };
      vp.Size = ovrHmd_GetFovTextureSize(hmd, eye, fov, 1.0f);

      eyeFramebuffers[eye] = ovr::SwapTexFboPtr(new ovr::SwapTextureFramebufferWrapper(hmd));
      eyeFramebuffers[eye]->Init(ovr::toGlm(vp.Size));
      layer.ColorTexture[eye] = eyeFramebuffers[eye]->textureSet;

      auto erd = ovrHmd_GetRenderDesc(hmd, eye, fov);
      eyeOffsets[eye] = erd.HmdToEyeViewOffset;

      ovrMatrix4f ovrPerspectiveProjection = ovrMatrix4f_Projection(fov, OVR_DEFAULT_IPD * 4, 100000.0f, true);
      eyeProjections[eye] = ovr::toGlm(ovrPerspectiveProjection);
    });
  }

  void onKey(int key, int scancode, int action, int mods) {
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
    // auto frameTiming = ovrHmd_GetFrameTiming(hmd, frameIndex);
    // auto trackingState = ovrHmd_GetTrackingState(hmd, ovr_GetTimeInSeconds());
    // ovr_CalcEyePoses(trackingState.HeadPose.ThePose, eyeOffsets, eyePoses);

    ovrHmd_GetEyePoses(hmd, frameIndex, eyeOffsets, eyePoses, nullptr);
    glEnable(GL_DEPTH_TEST);

    for (int i = 0; i < 2; ++i) {
      ovrEyeType eye = hmd->EyeRenderOrder[i];
      Stacks::projection().top() = eyeProjections[eye];
      MatrixStack & mv = Stacks::modelview();
      mv.withPush([&]{
        // Apply the per-eye offset & the head pose
        mv.top() = glm::inverse(ovr::toGlm(eyePoses[eye])) * mv.top();
        eyeFramebuffers[eye]->Pushed([&] {
            eyeFramebuffers[eye]->Viewport();
            renderScene();
        });
      });
    };
    oglplus::DefaultFramebuffer().Bind(oglplus::Framebuffer::Target::Draw);
    ovrLayerEyeFov & layer = submitLayers[0].EyeFov;
    for_each_eye([&](ovrEyeType eye) {
      layer.RenderPose[eye] = eyePoses[eye];
    });
    ovrLayerHeader* firstHeader = &submitLayers[0].Header;
    ovrHmd_SubmitFrame(hmd, frameIndex, nullptr, &firstHeader, 1);
    for_each_eye([&](ovrEyeType eye) {
      eyeFramebuffers[eye]->Increment();
    });
    ++frameIndex;
  }

  virtual void renderScene() {
    oglplus::Context::Clear().DepthBuffer().ColorBuffer();
    oria::renderManikinScene(ipd, eyeHeight);
  }
};

RUN_APP(HelloRift);
