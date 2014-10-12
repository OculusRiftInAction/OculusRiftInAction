#include "Common.h"
#include <thread>
#include <mutex>

template <typename Function>
void withLock(std::mutex & m, Function f) {
  m.lock();
  f();
  m.unlock();
}

struct PerEyeArgs {
  glm::mat4 projection;
  glm::mat4 translation;
};

class AsyncTimewarpExample : public RiftGlfwApp {
  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_PLAYER_HEIGHT };
  glm::mat4 player;

  int perEyeDelay = 0;

  PerEyeArgs perEyeArgs[2];
  ovrTexture eyeTextures[2];
  ovrPosef eyePoses[2];

  int frameIndex{ 0 };

  // Offscreen rendering targets.  two for each eye.
  // One is used for rendering while the other is used for distortion
  gl::FrameBufferWrapper frameBuffers[2][2];

  std::unique_ptr<std::thread> threadPtr;
  std::mutex ovrLock;

  GLFWwindow * renderWindow;
  bool running{ true };

public:
  AsyncTimewarpExample() {
    ipd = ovrHmd_GetFloat(hmd, 
      OVR_KEY_IPD, OVR_DEFAULT_IPD);
    eyeHeight = ovrHmd_GetFloat(hmd, 
      OVR_KEY_PLAYER_HEIGHT, 
      OVR_DEFAULT_PLAYER_HEIGHT);

    int trackingCaps = 0
      | ovrTrackingCap_Orientation
      | ovrTrackingCap_Position
      | ovrTrackingCap_MagYawCorrection;

    if (!ovrHmd_ConfigureTracking(hmd, trackingCaps, 0)) {
      SAY_ERR("Could not attach to sensor device");
    }
  }

  ~AsyncTimewarpExample() {
    running = false;
    threadPtr->join();
    glfwDestroyWindow(renderWindow);
  }

  void initGl() {
    RiftGlfwApp::initGl();

    memset(eyeTextures, 0, 2 * sizeof(ovrGLTexture));
    float eyeHeight = 1.5f;
    player = glm::inverse(glm::lookAt(
      glm::vec3(0, eyeHeight, 4),
      glm::vec3(0, eyeHeight, 0),
      glm::vec3(0, 1, 0)));

    for_each_eye([&](ovrEyeType eye){
      ovrSizei eyeTextureSize = 
        ovrHmd_GetFovTextureSize(hmd, eye, hmd->MaxEyeFov[eye], 1.0f);
      ovrTextureHeader & eyeTextureHeader = eyeTextures[eye].Header;
      eyeTextureHeader.TextureSize = eyeTextureSize;
      eyeTextureHeader.RenderViewport.Size = eyeTextureSize;
      eyeTextureHeader.API = ovrRenderAPI_OpenGL;
    });

    ovrGLConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.RTSize = Rift::toOvr(windowSize);
    cfg.OGL.Header.Multisample = 1;

    int distortionCaps = 0
      | ovrDistortionCap_Vignette
      | ovrDistortionCap_Chromatic
      | ovrDistortionCap_TimeWarp;

    ovrEyeRenderDesc eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg.Config,
      distortionCaps, hmd->MaxEyeFov, eyeRenderDescs);

#ifdef _DEBUG
    ovrhmd_EnableHSWDisplaySDKRender(hmd, false);
#endif

    for_each_eye([&](ovrEyeType eye){
      const ovrEyeRenderDesc & erd = eyeRenderDescs[eye];
      ovrMatrix4f ovrPerspectiveProjection =
        ovrMatrix4f_Projection(erd.Fov, 0.01f, 100000.0f, true);
      perEyeArgs[eye].projection =
        Rift::fromOvr(ovrPerspectiveProjection);
      glm::vec3 eyeOffset =
        Rift::fromOvr(erd.ViewAdjust);
      perEyeArgs[eye].translation =
        glm::translate(glm::mat4(), eyeOffset);
    });

    //////////////////////////////////////////////////////
    // Initialize OpenGL settings and variables
    glfwWindowHint(GLFW_VISIBLE, 0);
    renderWindow = glfwCreateWindow(100, 100, "Offscreen", nullptr, window);
    glfwMakeContextCurrent(renderWindow);
    glEnable(GL_BLEND);
    for_each_eye([&](ovrEyeType eye){
      glm::uvec2 frameBufferSize =
        Rift::fromOvr(eyeTextures[0].Header.TextureSize);
      for (int i = 0; i < 2; ++i) {
        frameBuffers[eye][i].init(frameBufferSize);
      }
    });

    // Launch the thread that will perform distortion and display content to the screen
    threadPtr = std::unique_ptr<std::thread>(
      new std::thread(&AsyncTimewarpExample::distortionThread, this)
    );
    // Wait on the main thread long enough to ensure that the distortion thread has begun it's loop
    Platform::sleepMillis(500);
    resetCamera();
  }

  void resetCamera() {
    player = glm::inverse(glm::lookAt(
      glm::vec3(0, eyeHeight, 0.3f),  // Position of the camera
      glm::vec3(0, eyeHeight, 0),  // Where the camera is looking
      GlUtils::Y_AXIS));           // Camera up axis
    ovrHmd_RecenterPose(hmd);
  }

  void finishFrame() {

  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    if (CameraControl::instance().onKey(key, scancode, action, mods)) {
      return;
    }

    static const float ROOT_2 = sqrt(2.0f);
    static const float INV_ROOT_2 = 1.0f / ROOT_2;

    if (action == GLFW_PRESS) {
      switch (key) {
      case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, 1);
        return;

      case GLFW_KEY_R:
        resetCamera();
        return;

      case GLFW_KEY_HOME:
        if (0 == perEyeDelay) {
          perEyeDelay = 1;
        } else {
          perEyeDelay <<= 1;
        }
        return;

      case GLFW_KEY_END:
        perEyeDelay >>= 1;
        return;
      }
    }

    RiftGlfwApp::onKey(key, scancode, action, mods);
  }


  void distortionThread() {
    ovrLock.lock();
    // Make the shared context current
    glfwMakeContextCurrent(window);
    // Each thread requires it's own glewInit call.
    glewInit();
    while (running) {
      ovrFrameTiming frameTime = ovrHmd_BeginFrame(hmd, frameIndex++);
      ovrLock.unlock();

      if (0 != frameTime.TimewarpPointSeconds) {
        ovr_WaitTillTime(frameTime.TimewarpPointSeconds - 0.002);
      }

      ovrLock.lock();
      ovrHmd_EndFrame(hmd, eyePoses, eyeTextures);
    }
    ovrLock.unlock();
  }

  void draw() {
    // Synchronization to determine when a given eye's render commands have completed
    // The index of the current rendering target framebuffer.  
    static int renderBuffers[2]{0, 0};
    // The pose for each rendered framebuffer
    static ovrPosef renderPoses[2];

    for (int i = 0; i < 2; ++i) {
      ovrEyeType eye = hmd->EyeRenderOrder[i];
      // We can only acquire an eye pose between beginframe and endframe.
      // So we've arranged for the lock to be only open at those points.  
      // The main thread will spend most of it's time in the wait.
      ::withLock(ovrLock, [&]{
          renderPoses[eye] = ovrHmd_GetEyePose(hmd, eye);
      });

      const PerEyeArgs & eyeArgs = perEyeArgs[eye];
      gl::MatrixStack & mv = gl::Stacks::modelview();
      gl::Stacks::projection().top() = eyeArgs.projection;
      gl::Stacks::with_push(mv, [&]{
        // Apply the head pose
        glm::mat4 m = Rift::fromOvr(renderPoses[eye]);
        mv.preMultiply(glm::inverse(m));
        // Apply the per-eye offset
        mv.preMultiply(eyeArgs.translation);

        int renderBufferIndex = renderBuffers[eye];
        gl::FrameBufferWrapper & frameBuffer = 
          frameBuffers[eye][renderBufferIndex];
        // Render the scene to an offscreen buffer
        frameBuffer.activate();
        renderScene();
        frameBuffer.deactivate();
      });
    } // for each eye

    // Ensure all scene rendering commands have been completed
    glFinish();


    withLock(ovrLock, [&]{
      for_each_eye([&](ovrEyeType eye) {
        int renderBufferIndex = renderBuffers[eye];
        renderBuffers[eye] = renderBufferIndex ? 0 : 1;
        ((ovrGLTexture&)(eyeTextures[eye])).OGL.TexId =
          frameBuffers[eye][renderBufferIndex].color->texture;
        eyePoses[eye] = renderPoses[eye];
      });
    });
  }

  void renderScene() {
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.withPush([&]{
      mv.postMultiply(glm::inverse(player));
      GlUtils::renderCubeScene(ipd, eyeHeight);
    });

    std::string maxfps = perEyeDelay ? 
      Platform::format("%0.2f", 500.0f / perEyeDelay) : "N/A";
    std::string message =
      Platform::format("Per Eye Delay %dms\nMax FPS %s",
      perEyeDelay, maxfps.c_str());
    renderStringAt(message, glm::vec2(-0.5, 0.5));

    // Simulate some really slow rendering
    if (0 != perEyeDelay) {
      ovr_WaitTillTime(ovr_GetTimeInSeconds() + 
        (float)perEyeDelay / 1000.0f);
    }
  }
};

RUN_OVR_APP(AsyncTimewarpExample);
