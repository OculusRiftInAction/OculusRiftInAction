#include "Common.h"
#include <thread>
#include <mutex>

class AsyncTimewarpExample : public RiftGlfwApp {
  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_PLAYER_HEIGHT };
  glm::mat4 player;
  
  ovrTexture eyeTextures[2];
  ovrPosef eyePoses[2];
  ovrVector3f hmdToEyeOffsets[2];
  glm::mat4 eyeProjections[2];

  int perEyeDelay = 0;
  // Offscreen rendering targets: two for each eye.
  // One is used for rendering (writing) while the other 
  // is  used for distortion (reading)
  FramebufferWrapperPtr  eyeFramebuffers[2][2];
  // Keep track of the index of the buffer currently being written to for each eye
  unsigned int writeFramebuffersIndex{ 0 };
  unsigned int distortionFrameIndex{ 0 };

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
      ;

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
    cfg.OGL.Header.BackBufferSize = ovr::fromGlm(getSize());
    cfg.OGL.Header.Multisample = 1;

    int distortionCaps = 0
      | ovrDistortionCap_Vignette
      | ovrDistortionCap_Chromatic
      | ovrDistortionCap_TimeWarp;

    ovrEyeRenderDesc eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg.Config,
      distortionCaps, hmd->MaxEyeFov, eyeRenderDescs);

    for_each_eye([&](ovrEyeType eye){
      const ovrEyeRenderDesc & erd = eyeRenderDescs[eye];
      ovrMatrix4f ovrPerspectiveProjection =
        ovrMatrix4f_Projection(erd.Fov, 0.01f, 100000.0f, true);
      eyeProjections[eye] =
        ovr::toGlm(ovrPerspectiveProjection);
      hmdToEyeOffsets[eye] = erd.HmdToEyeViewOffset;
    });

    //////////////////////////////////////////////////////
    // Initialize OpenGL settings and variables
    glfwWindowHint(GLFW_VISIBLE, 0);
    renderWindow = glfwCreateWindow(100, 100, "Offscreen", nullptr, getWindow());
    glfwMakeContextCurrent(renderWindow);
    // Some bug in the SDK causes the render window to trigger a 
    // framebuffer error when it's made active, so clear that out
    GLenum err = glGetError();
    oglplus::Context::Enable(oglplus::Capability::Blend);

    for_each_eye([&](ovrEyeType eye){
      glm::uvec2 frameBufferSize =
        ovr::toGlm(eyeTextures[eye].Header.TextureSize);
      for (int i = 0; i < 2; ++i) {
        eyeFramebuffers[eye][i] =  FramebufferWrapperPtr(new FramebufferWrapper());
        eyeFramebuffers[eye][i]->init(frameBufferSize);
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
      Vectors::UP));           // Camera up axis
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
    Platform::setThreadPriority(Platform::HIGH);
    // Make the shared context current
    glfwMakeContextCurrent(getWindow());

    // Each thread requires it's own glewInit call.
    glewInit();
    while (running) {
      ++distortionFrameIndex;
      ovrFrameTiming frameTime = ovrHmd_BeginFrame(hmd, distortionFrameIndex);

      // The Endframe call will block until right before the v-sync.  So we
      // give the render thread as much time as possible to update the frame
      // before starting the EndFrame call
      if (0 != frameTime.TimewarpPointSeconds) {
        ovr_WaitTillTime(frameTime.TimewarpPointSeconds - 0.003);
      }

      ovrLock.lock();
      ovrHmd_EndFrame(hmd, eyePoses, eyeTextures);
      ovrLock.unlock();
    }
  }

  void draw() {
    // The pose for each rendered framebuffer
    ovrPosef renderPoses[2];
    ovrHmd_GetEyePoses(hmd, distortionFrameIndex, hmdToEyeOffsets, renderPoses, nullptr);

    for (int i = 0; i < 2; ++i) {
      ovrEyeType eye = hmd->EyeRenderOrder[i];
      MatrixStack & mv = Stacks::modelview();
      Stacks::projection().top() = eyeProjections[eye];
      Stacks::withPush(mv, [&]{
        // Apply the head pose
        glm::mat4 m = ovr::toGlm(renderPoses[eye]);
        mv.preMultiply(glm::inverse(m));
        FramebufferWrapperPtr & frameBuffer =
          eyeFramebuffers[eye][writeFramebuffersIndex];
        // Render the scene to an offscreen buffer
        frameBuffer->Bind();
        renderScene();
      });
    } // for each eye

    // Ensure all scene rendering commands have been completed
    glFinish();

    ovrLock.lock();
    for_each_eye([&](ovrEyeType eye) {
      int renderBufferIndex = writeFramebuffersIndex;
      ((ovrGLTexture&)(eyeTextures[eye])).OGL.TexId =
        oglplus::GetName(eyeFramebuffers[eye][renderBufferIndex]->color);
      eyePoses[eye] = renderPoses[eye];
    });
    writeFramebuffersIndex = writeFramebuffersIndex ? 0 : 1;
    ovrLock.unlock();
  }

  void renderScene() {
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    MatrixStack & mv = Stacks::modelview();
    mv.withPush([&]{
      mv.postMultiply(glm::inverse(player));
      oria::renderManikinScene(ipd, eyeHeight);
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
