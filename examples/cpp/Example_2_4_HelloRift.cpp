#include "Common.h"
#include <OVR_CAPI_GL.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL

#include <GLFW/glfw3native.h>
#define DISTORT

struct EyeArgs {
  glm::mat4               projection;
  glm::mat4               viewOffset;
  gl::FrameBufferWrapper  framebuffer;
};

void swapBufferCallback(void * userData) {
  glfwSwapBuffers((GLFWwindow*)userData);
}

class HelloRift : public GlfwApp {
protected:
  ovrHmd                  hmd{ 0 };
  EyeArgs                 perEyeArgs[2];
  ovrTexture              textures[2];
  float                   eyeHeight{ OVR_DEFAULT_EYE_HEIGHT };
  float                   ipd{ OVR_DEFAULT_IPD };

public:
  HelloRift() {
    windowPosition = glm::ivec2(0, 0);
    windowSize = glm::uvec2(100, 100);
  }

  ~HelloRift() {
    ovrHmd_Destroy(hmd);
    hmd = 0;
  }

  virtual void finishFrame() {

  }


  virtual void createRenderingTarget() {
    glfwWindowHint(GLFW_DECORATED, 0);

    createWindow(windowSize, windowPosition);
    ovr_Initialize();
    hmd = ovrHmd_Create(0);
    ovrHmd_ConfigureTracking(hmd,
      ovrTrackingCap_Orientation |
      ovrTrackingCap_MagYawCorrection |
      ovrTrackingCap_Position, 0);
    windowPosition = glm::ivec2(hmd->WindowsPos.x, hmd->WindowsPos.y);
    windowSize = glm::uvec2(hmd->Resolution.w, hmd->Resolution.h);
    glfwSetWindowPos(window, windowPosition.x, windowPosition.y);
    glfwSetWindowSize(window, windowSize.x, windowSize.y);
    
    ovrHmd_AttachToWindow(hmd, glfwGetWin32Window(window), nullptr, nullptr);

    if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
      FAIL("Unable to create undecorated window");
    }
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

      eyeArgs.framebuffer.init(Rift::fromOvr(eyeTextureHeader.TextureSize));
      ((ovrGLTexture&)textures[eye]).OGL.TexId = eyeArgs.framebuffer.color->texture;
    });

    ovrGLConfig cfg; memset(&cfg, 0, sizeof(ovrGLConfig));
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.RTSize = hmd->Resolution;
    cfg.OGL.Header.Multisample = 1;

#ifdef OVR_OS_WINDOWS
    cfg.OGL.Window = 0;
#elif defined(OVR_OS_LINUX)
    cfg.OGL.Disp = 0;
    cfg.OGL.Win = 0;
#endif

    int distortionCaps = ovrDistortionCap_TimeWarp | ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette;

    ovrEyeRenderDesc              eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg.Config,
      distortionCaps, eyeFovPorts, eyeRenderDescs);
    //ovrHmd_SetSwapBuffersCallback(hmd, swapBufferCallback, window);

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
      ovrHmd_RecenterPose(hmd);
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
    static ovrPosef eyePoses[2];
    ovrHmd_BeginFrame(hmd, frameIndex++);
    glEnable(GL_DEPTH_TEST);
    for( int i = 0; i < 2; ++i) {
      ovrEyeType eye = hmd->EyeRenderOrder[i];
      EyeArgs & eyeArgs = perEyeArgs[eye];
      gl::Stacks::projection().top() = eyeArgs.projection;
      gl::MatrixStack & mv = gl::Stacks::modelview();

      eyeArgs.framebuffer.activate();
      eyePoses[eye] = ovrHmd_GetEyePose(hmd, eye);
      mv.withPush([&]{
        // Apply the per-eye offset & the head pose
        mv.top() = eyeArgs.viewOffset * glm::inverse(Rift::fromOvr(eyePoses[eye])) * mv.top();
        renderScene();
      });
      //ovrHmd_EndEyeRender(hmd, eye, renderPose, &eyeArgs.textures.Texture);
      eyeArgs.framebuffer.deactivate();
    };
    GLenum err = glGetError();
    ovrHmd_EndFrame(hmd, eyePoses, textures);
//    glfwSwapBuffers(window);
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

RUN_APP(HelloRift);

