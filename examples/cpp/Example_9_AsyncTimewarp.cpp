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
  gl::FrameBufferWrapper frameBuffers[2][2];
  glm::mat4 projections[2];
  ovrTexture eyeTextures[2];
  ovrPosef eyePoses[2];
  glm::mat4 player;
  ovrEyeType currentEye;
  std::unique_ptr<std::thread> threadPtr;
  std::mutex ovrLock;
  GLFWwindow * renderWindow;
  GLsync eyeFences[2];
  int backBuffers[2];

public:
  SimpleScene() {
    ipd = ovrHmd_GetFloat(hmd, OVR_KEY_IPD, OVR_DEFAULT_IPD);
    eyeHeight = ovrHmd_GetFloat(hmd, OVR_KEY_PLAYER_HEIGHT, OVR_DEFAULT_PLAYER_HEIGHT);
    if (!ovrHmd_ConfigureTracking(hmd,
      ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection, 0)) {
      SAY_ERR("Could not attach to sensor device");
    }
    for_each_eye([&](ovrEyeType eye){
      eyeFences[eye] = 0;
      backBuffers[eye] = 0;
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
  }


  virtual void onKey(int key, int scancode, int action, int mods) {
    if (!CameraControl::instance().onKey(key, scancode, action, mods)) {
      static const float ROOT_2 = sqrt(2.0f);
      static const float INV_ROOT_2 = 1.0f / ROOT_2;
      if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_HOME:
          break;
        case GLFW_KEY_END:
          break;
        case GLFW_KEY_R:
          resetCamera();
          break;
        }
      }
      else {
        RiftGlfwApp::onKey(key, scancode, action, mods);
      }
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
//    threadPtr->join();
    glfwMakeContextCurrent(window);
  }

  int frameIndex{ 0 };
  volatile int textureIds[2];

  void runOvrThread() {
    glfwMakeContextCurrent(renderWindow);
    glewInit();

    // Allocate the frameBuffer that will hold the scene, and then be
    // re-rendered to the screen with distortion
    for_each_eye([&](ovrEyeType eye) {
      glm::uvec2 frameBufferSize = Rift::fromOvr(eyeTextures[0].Header.TextureSize);
      for (int i = 0; i < 2; ++i) {
        frameBuffers[i][eye].init(frameBufferSize);
      }
    });

    while (true) {
      for_each_eye([&](ovrEyeType eye) {
        if (0 != eyeFences[eye]) {
          GLenum result = glClientWaitSync(eyeFences[eye], GL_SYNC_FLUSH_COMMANDS_BIT, 0);
          switch (result) {
          case GL_ALREADY_SIGNALED:
          case GL_CONDITION_SATISFIED:
            eyeFences[eye] = 0;
            int bufferIndex = backBuffers[eye];
            textureIds[eye] = frameBuffers[bufferIndex][eye].color->texture;
            backBuffers[eye] = (bufferIndex + 1) % 2;
            break;
          }
        }
      });

      for (int i = 0; i < 2; ++i) {
        ovrEyeType eye = hmd->EyeRenderOrder[i];
        if (0 != eyeFences[eye]) {
          continue;
        }

        gl::MatrixStack & mv = gl::Stacks::modelview();
        gl::Stacks::projection().top() = projections[eye];
        gl::Stacks::with_push(mv, [&]{
          const ovrEyeRenderDesc & erd = eyeRenderDescs[eye];

          ::withLock(ovrLock, [&]{
            eyePoses[eye] = ovrHmd_GetEyePose(hmd, eye);
          });

          {
            // Apply the head pose
            glm::mat4 m = Rift::fromOvr(eyePoses[eye]);
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
          glFlush();
          textureIds[eye] = frameBuffer.color->texture;
        });
      }
    }

  }


  void draw() {
    auto frameTime = ovrHmd_BeginFrame(hmd, frameIndex++);
    ovrLock.unlock();

    ovr_WaitTillTime(frameTime.TimewarpPointSeconds - 0.002);

    // Grab the most recent textures
    for_each_eye([&](ovrEyeType eye) {
      ((ovrGLTexture&)(eyeTextures[eye])).OGL.TexId =
        textureIds[eye];
    });

    ovrLock.lock();
    ovrHmd_EndFrame(hmd, eyePoses, eyeTextures);
  }

  void renderScene() {
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.withPush([&]{
      mv.postMultiply(glm::inverse(player));
      GlUtils::renderCubeScene(ipd, eyeHeight);
      GlUtils::cubeRecurse(10);
    });
  }
};

RUN_OVR_APP(SimpleScene);
