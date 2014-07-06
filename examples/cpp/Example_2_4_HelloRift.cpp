#include "Common.h"
#include <OVR_CAPI_GL.h>

#define DISTORT

struct EyeArgs {
  glm::mat4               projection;
  glm::mat4               viewOffset;
  gl::FrameBufferWrapper  framebuffer;
  ovrGLTexture            textures;
};

class HelloRift : public GlfwApp {
protected:
  ovrHmd                  hmd;
  ovrHmdDesc              hmdDesc;
  EyeArgs                 perEyeArgs[2];
  float                   eyeHeight{ OVR_DEFAULT_EYE_HEIGHT };
  float                   ipd{ OVR_DEFAULT_IPD };

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

  virtual void createRenderingTarget() {
    glfwWindowHint(GLFW_DECORATED, 0);

    createWindow(windowSize, windowPosition);

    if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
      FAIL("Unable to create undecorated window");
    }
  }

  void initGl() {
    GlfwApp::initGl();
    ovrFovPort eyeFovPorts[2];
    for_each_eye([&](ovrEyeType eye){
      EyeArgs & eyeArgs = perEyeArgs[eye];
      ovrTextureHeader & eyeTextureHeader = eyeArgs.textures.OGL.Header;
      eyeFovPorts[eye] = hmdDesc.DefaultEyeFov[eye];
      eyeTextureHeader.TextureSize = ovrHmd_GetFovTextureSize(hmd, eye, hmdDesc.DefaultEyeFov[eye], 1.0f);
      eyeTextureHeader.RenderViewport.Size = eyeTextureHeader.TextureSize;
      eyeTextureHeader.RenderViewport.Pos.x = 0;
      eyeTextureHeader.RenderViewport.Pos.y = 0;
      eyeTextureHeader.API = ovrRenderAPI_OpenGL;

      eyeArgs.framebuffer.init(Rift::fromOvr(eyeTextureHeader.TextureSize));
      eyeArgs.textures.OGL.TexId = eyeArgs.framebuffer.color->texture;
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

    ovrEyeRenderDesc              eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg.Config,
      distortionCaps, eyeFovPorts, eyeRenderDescs);

    for_each_eye([&](ovrEyeType eye){
      EyeArgs & eyeArgs = perEyeArgs[eye];
      eyeArgs.projection = Rift::fromOvr(
        ovrMatrix4f_Projection(eyeFovPorts[eye], 0.01, 100, true));
      eyeArgs.viewOffset = glm::translate(glm::mat4(), 
        Rift::fromOvr(eyeRenderDescs[eye].ViewAdjust));
    });
  }

  void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action) {
      return;
    }

    switch (key) {
    case GLFW_KEY_R:
      ovrHmd_ResetSensor(hmd);
      break;

    default:
      GlfwApp::onKey(key, scancode, action, mods);
      break;
    }
  }

  virtual void update() {
    static const glm::vec3 EYE = glm::vec3(0, eyeHeight, ipd * 5);
    static const glm::vec3 LOOKAT = glm::vec3(0, eyeHeight, 0);
    gl::Stacks::modelview().top() = glm::lookAt(EYE, LOOKAT, GlUtils::UP);
  }

  void draw() {
    static int frameIndex = 0;
    ovrHmd_BeginFrame(hmd, frameIndex++);
    glEnable(GL_DEPTH_TEST);
    for( int i = 0; i < 2; ++i) {
      ovrEyeType eye = hmdDesc.EyeRenderOrder[i];
      EyeArgs & eyeArgs = perEyeArgs[eye];
      gl::Stacks::projection().top() = eyeArgs.projection;
      gl::MatrixStack & mv = gl::Stacks::modelview();

      eyeArgs.framebuffer.activate();
      ovrPosef renderPose = ovrHmd_BeginEyeRender(hmd, eye);
      mv.withPush([&]{
        // Apply the per-eye offset & the head pose
        mv.top() = eyeArgs.viewOffset * glm::inverse(Rift::fromOvr(renderPose)) * mv.top();
        renderScene();
      });
      ovrHmd_EndEyeRender(hmd, eye, renderPose, &eyeArgs.textures.Texture);
      eyeArgs.framebuffer.deactivate();
    };
    ovrHmd_EndFrame(hmd);
  }

  virtual void renderScene() {
    glClear(GL_DEPTH_BUFFER_BIT);
    GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    GlUtils::renderFloorGrid(glm::mat4());

    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.with_push([&]{
      mv.translate(glm::vec3(0, eyeHeight, 0));
      mv.scale(ipd);
      GlUtils::drawColorCube();
    });
  }
};

RUN_OVR_APP(HelloRift);

