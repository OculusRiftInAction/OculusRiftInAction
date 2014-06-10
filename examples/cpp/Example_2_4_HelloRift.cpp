#include "Common.h"
#include <OVR_CAPI_GL.h>

#define DISTORT
class HelloRift : public GlfwApp {
protected:
  ovrHmd                        hmd;
  ovrHmdDesc                    hmdDesc;

  ovrEyeRenderDesc              eyes[2];
  gl::FrameBufferWrapper        frameBuffers[2];

  bool                          useTracker{false};

  float                         ipd{ OVR_DEFAULT_IPD };
  ovrEyeRenderDesc              eyeRenderDescs[2];
  ovrGLTexture                  eyeTextures[2];

public:
  HelloRift() {
    hmd = ovrHmd_Create(0);
    ovrHmd_GetDesc(hmd, &hmdDesc);
    ovrHmd_StartSensor(hmd, 0, 0);
    windowPosition = glm::ivec2(hmdDesc.WindowsPos.x, hmdDesc.WindowsPos.y);
    // The HMDInfo gives us the position of the Rift in desktop
    // coordinates as well as the native resolution of the Rift
    // display panel, but NOT the current resolution of the signal
    // being sent to the Rift.
    GLFWmonitor * hmdMonitor =
        GlfwApp::getMonitorAtPosition(windowPosition);
    if (hmdMonitor) {
      // For the current resoltuion we must find the appropriate GLFW monitor
      const GLFWvidmode * videoMode = glfwGetVideoMode(hmdMonitor);
      windowSize = glm::ivec2(videoMode->width, videoMode->height);
    } else {
      windowSize = glm::ivec2(1280, 800);
    }
  }

  ~HelloRift() {
    ovrHmd_Destroy(hmd);
    hmd = 0;
  }
#define SAMPLE_COUNT 4
#define TEXTURE_MULTIPLIER 1.4f


  virtual void createRenderingTarget() {
    glfwWindowHint(GLFW_SAMPLES, SAMPLE_COUNT);
    glfwWindowHint(GLFW_DECORATED, 0);
    createWindow(windowSize, windowPosition);
    GLint sampleCount;
    glGetIntegerv(GL_SAMPLES, &sampleCount);

    if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
      FAIL("Unable to create undecorated window");
    }
  }

  void initGl() {
    GlfwApp::initGl();
    ovrFovPort eyeFovPorts[2];
    for_each_eye([&](ovrEyeType eye){
      ovrTextureHeader & eyeTextureHeader = eyeTextures[eye].OGL.Header;
      eyeFovPorts[eye] = hmdDesc.DefaultEyeFov[eye];
      eyeTextureHeader.TextureSize = ovrHmd_GetFovTextureSize(hmd, eye, hmdDesc.DefaultEyeFov[eye], 1.0f);
      eyeTextureHeader.TextureSize.w *= TEXTURE_MULTIPLIER;
      eyeTextureHeader.TextureSize.h *= TEXTURE_MULTIPLIER;
      eyeTextureHeader.RenderViewport.Size = eyeTextureHeader.TextureSize;
      eyeTextureHeader.RenderViewport.Pos.x = 0;
      eyeTextureHeader.RenderViewport.Pos.y = 0;
      eyeTextureHeader.API = ovrRenderAPI_OpenGL;

      frameBuffers[eye].init(Rift::fromOvr(eyeTextureHeader.TextureSize));
      eyeTextures[eye].OGL.TexId = frameBuffers[eye].color->texture;
    });

    ovrGLConfig cfg;
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.RTSize = hmdDesc.Resolution;
    cfg.OGL.Header.Multisample = 1;
#ifdef OVR_OS_WINDOWS
    cfg.OGL.Window = 0;
#elif defined(OVR_OS_LINUX)
    cfg.OGL.Disp = 0;
    cfg.OGL.Win = 0;
#endif

    int distortionCaps = ovrDistortionCap_TimeWarp | ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette | ovrDistortionCap_NoSwapBuffers;

    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg.Config,
      distortionCaps, eyeFovPorts, eyeRenderDescs);
  }

  void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action) {
      return;
    }

    switch (key) {
    case GLFW_KEY_R:
      ovrHmd_ResetSensor(hmd);
      break;

    case GLFW_KEY_T:
      useTracker = !useTracker;
      break;

    default:
      GlfwApp::onKey(key, scancode, action, mods);
      break;
    }
  }

  virtual void update() {
    static const glm::vec3 CAMERA = glm::vec3(0.0f, 0.0f, ipd * 5);
    gl::Stacks::modelview().top() = glm::lookAt(CAMERA, GlUtils::ORIGIN, GlUtils::Y_AXIS);
  }

  void draw() {
    static int frameIndex = 0;
    ovrHmd_BeginFrame(hmd, frameIndex++);
    glClearColor(0, 1, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    GL_CHECK_ERROR;

    for_each_eye([&](ovrEyeType eye) {
      frameBuffers[eye].activate();
      renderScene(eye);
      frameBuffers[eye].deactivate();

      glDisable(GL_DEPTH_TEST);
      glDisable(GL_BLEND);
    });

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    ovrHmd_EndFrame(hmd);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
  }


  virtual void renderScene(ovrEyeType eye) {
    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::MatrixStack & pr = gl::Stacks::projection();
    ovrPosef renderPose = ovrHmd_BeginEyeRender(hmd, eye);
    const ovrEyeRenderDesc & erd = eyeRenderDescs[eye];
    gl::Stacks::with_push([&]{
      // Set up the per-eye projection matrix
      pr.top() = Rift::fromOvr(ovrMatrix4f_Projection(erd.Fov, 0.01, 100000, true));
      // Set up the per-eye modelview matrix
      mv.preMultiply(glm::translate(glm::mat4(), Rift::fromOvr(renderPose.Position) * -1.0f));
      // Apply the head pose
      mv.postMultiply(glm::mat4_cast(glm::inverse(Rift::fromOvr(renderPose.Orientation))));
      // Apply the per-eye offset
      mv.preMultiply(glm::translate(glm::mat4(), Rift::fromOvr(erd.ViewAdjust)));

      glEnable(GL_DEPTH_TEST);
      glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      mv.with_push([&]{
        mv.translate(glm::vec3(0, 0, -1.5f))
          .rotate(glm::angleAxis(PI / 2.0f, GlUtils::X_AXIS));
        GlUtils::draw3dGrid();
      });
      mv.with_push([&]{
        mv.scale(ipd);
        GlUtils::drawColorCube();
      });

      ovrHmd_EndEyeRender(hmd, eye, renderPose, &eyeTextures[eye].Texture);
    });
  }
};

RUN_OVR_APP(HelloRift);

