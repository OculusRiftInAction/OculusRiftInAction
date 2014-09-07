#include "Common.h"
#include <thread>
#include <mutex>

template <typename Function>
void withLock(std::mutex & m, Function f) {
  m.lock();
  f();
  m.unlock();
}

class SimpleScene : public RiftGlfwApp {
  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_PLAYER_HEIGHT };
  ovrEyeRenderDesc eyeRenderDescs[2];
  glm::mat4 projections[2];
  ovrTexture eyeTextures[2];
  ovrPosef eyePoses[2];
  glm::mat4 player;
  volatile int perEyeDelay = 0;
  int frameIndex{ 0 };
  volatile int textureIds[2];
  ovrEyeType currentEye;
  std::unique_ptr<std::thread> threadPtr;
  std::mutex ovrLock;
  GLFWwindow * renderWindow;
  volatile bool running{ true };

public:
  SimpleScene() {
    ipd = ovrHmd_GetFloat(hmd, OVR_KEY_IPD, OVR_DEFAULT_IPD);
    eyeHeight = ovrHmd_GetFloat(hmd, OVR_KEY_PLAYER_HEIGHT, OVR_DEFAULT_PLAYER_HEIGHT);
    if (!ovrHmd_ConfigureTracking(hmd,
      ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection, 0)) {
      SAY_ERR("Could not attach to sensor device");
    }
    for_each_eye([&](ovrEyeType eye){
      textureIds[eye] = 0;
    });

    memset(eyeTextures, 0, 2 * sizeof(ovrGLTexture));
    float eyeHeight = 1.5f;
    player = glm::inverse(glm::lookAt(
      glm::vec3(0, eyeHeight, 4),
      glm::vec3(0, eyeHeight, 0),
      glm::vec3(0, 1, 0)));

    for_each_eye([&](ovrEyeType eye){
      ovrSizei eyeTextureSize = ovrHmd_GetFovTextureSize(hmd, eye, hmd->MaxEyeFov[eye], 1.0f);

      ovrTextureHeader & eyeTextureHeader = eyeTextures[eye].Header;
      eyeTextureHeader.TextureSize = eyeTextureSize;
      eyeTextureHeader.RenderViewport.Size = eyeTextureSize;
      eyeTextureHeader.API = ovrRenderAPI_OpenGL;
    });

    resetCamera();
  }

  ~SimpleScene() {
    // Release the distortion lock
    running = false;
    // Hack find a cleaner way of shutting down the rendering thread
    Platform::sleepMillis(10);
    ovrLock.unlock();
    threadPtr->join();
  }


  virtual void onKey(int key, int scancode, int action, int mods) {
    if (!CameraControl::instance().onKey(key, scancode, action, mods)) {
      static const float ROOT_2 = sqrt(2.0f);
      static const float INV_ROOT_2 = 1.0f / ROOT_2;
      if (action == GLFW_PRESS) {
        switch (key) {
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
        case GLFW_KEY_R:
          resetCamera();
          return;
        }
      }
      RiftGlfwApp::onKey(key, scancode, action, mods);
    }
  }

  void resetCamera() {
    player = glm::inverse(glm::lookAt(
      glm::vec3(0, eyeHeight, 0.5f),  // Position of the camera
      glm::vec3(0, eyeHeight, 0),  // Where the camera is looking
      GlUtils::Y_AXIS));           // Camera up axis
    ovrHmd_RecenterPose(hmd);
  }

  void finishFrame() {

  }

  void initGl() {
    RiftGlfwApp::initGl();

    ovrGLConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.RTSize = Rift::toOvr(windowSize);
    cfg.OGL.Header.Multisample = 1;

    int distortionCaps = 0
      | ovrDistortionCap_Vignette
      | ovrDistortionCap_Chromatic
      | ovrDistortionCap_TimeWarp
      ;

    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg.Config,
      distortionCaps, hmd->MaxEyeFov, eyeRenderDescs);

#ifdef _DEBUG
    ovrhmd_EnableHSWDisplaySDKRender(hmd, false);
#endif

    for_each_eye([&](ovrEyeType eye){
      const ovrEyeRenderDesc & erd = eyeRenderDescs[eye];
      ovrMatrix4f ovrPerspectiveProjection = ovrMatrix4f_Projection(erd.Fov, 0.01f, 100000.0f, true);
      projections[eye] = Rift::fromOvr(ovrPerspectiveProjection);
    });

    ///////////////////////////////////////////////////////////////////////////
    // Initialize OpenGL settings and variables
    glEnable(GL_BLEND);

    ovrLock.lock();
    renderWindow = glfwCreateWindow(100, 100, "Ofscreen", nullptr, window);

    threadPtr = std::unique_ptr<std::thread>(new std::thread(&SimpleScene::runOvrThread, this));
    glfwMakeContextCurrent(window);
  }


  void renderScene() {
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.withPush([&]{
      mv.postMultiply(glm::inverse(player));
      GlUtils::renderCubeScene(ipd, eyeHeight);
    });
    renderStringAt(Platform::format("Per Eye Delay %dms", perEyeDelay), glm::vec2(-0.5, 0.5));
    // Simulate some really slow rendering
    if (0 != perEyeDelay) {
      Platform::sleepMillis(perEyeDelay);
    }
  }

  void runOvrThread() {
    // Make the shared context current
    glfwMakeContextCurrent(renderWindow);
    // Each thread requires it's own glewInit call.
    glewInit();

    // Synchronization to determine when a given eye's render commands have completed
    GLsync eyeFences[2]{0, 0};
    // The index of the current rendering target framebuffer.  
    int backBuffers[2]{0, 0};
    // The pose for each rendered framebuffer
    ovrPosef backPoses[2];

    // Offscreen rendering targets.  two for each eye.
    // One is used for rendering while the other is used for distortion
    gl::FrameBufferWrapper frameBuffers[2][2];
    for_each_eye([&](ovrEyeType eye) {
      glm::uvec2 frameBufferSize = Rift::fromOvr(eyeTextures[0].Header.TextureSize);
      for (int i = 0; i < 2; ++i) {
        frameBuffers[i][eye].init(frameBufferSize);
      }
    });

    while (running) {
      for (int i = 0; i < 2; ++i) {
        for_each_eye([&](ovrEyeType eye) {
          if (0 != eyeFences[eye]) {
            GLenum result = glClientWaitSync(eyeFences[eye], GL_SYNC_FLUSH_COMMANDS_BIT, 0);
            switch (result) {
            case GL_ALREADY_SIGNALED:
            case GL_CONDITION_SATISFIED:
              withLock(ovrLock, [&]{
                eyeFences[eye] = 0;
                int bufferIndex = backBuffers[eye];
                textureIds[eye] = frameBuffers[bufferIndex][eye].color->texture;
                backBuffers[eye] = (bufferIndex + 1) % 2;
                eyePoses[eye] = backPoses[eye];
              });
              break;
            }
          }
        });


        ovrEyeType eye = hmd->EyeRenderOrder[i];
        if (0 != eyeFences[eye]) {
          continue;
        }

        gl::MatrixStack & mv = gl::Stacks::modelview();
        gl::Stacks::projection().top() = projections[eye];
        gl::Stacks::with_push(mv, [&]{
          const ovrEyeRenderDesc & erd = eyeRenderDescs[eye];

          // We can only acquire an eye pose between beginframe and endframe.
          // So we've arranged for the lock to be only open at those points.  
          // The main thread will spend most of it's time in the wait.
          ::withLock(ovrLock, [&]{
            if (running) {
              backPoses[eye] = ovrHmd_GetEyePose(hmd, eye);
            }
          });

          {
            // Apply the head pose
            glm::mat4 m = Rift::fromOvr(backPoses[eye]);
            mv.preMultiply(glm::inverse(m));
            // Apply the per-eye offset
            glm::vec3 eyeOffset = Rift::fromOvr(erd.ViewAdjust);
            mv.preMultiply(glm::translate(glm::mat4(), eyeOffset));
          }

          int bufferIndex = backBuffers[eye];
          gl::FrameBufferWrapper & frameBuffer = frameBuffers[bufferIndex][eye];
          // Render the scene to an offscreen buffer
          frameBuffer.activate();
          renderScene();
          frameBuffer.deactivate();
          eyeFences[eye] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        });
      }
    }
  }


  void draw() {
    auto frameTime = ovrHmd_BeginFrame(hmd, frameIndex++);
    ovrLock.unlock();

    if (0 == frameTime.TimewarpPointSeconds) {
      ovr_WaitTillTime(frameTime.TimewarpPointSeconds - 0.002);
    } else {
      ovr_WaitTillTime(frameTime.NextFrameSeconds - 0.008);
    }

    // Grab the most recent textures
    for_each_eye([&](ovrEyeType eye) {
      ((ovrGLTexture&)(eyeTextures[eye])).OGL.TexId =
        textureIds[eye];
    });

    ovrLock.lock();
    ovrHmd_EndFrame(hmd, eyePoses, eyeTextures);
  }
};

RUN_OVR_APP(SimpleScene);
