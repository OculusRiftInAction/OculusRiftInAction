#include "Common.h"

#if defined(OVR_OS_WINDOWS)
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(OVR_OS_LINUX)
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#endif
#include <GLFW/glfw3native.h>

class SimpleScene : public RiftGlfwApp {
  glm::mat4 player;
  float ipd{ 0.064f };
  float eyeHeight{ 1.5f };
  gl::FrameBufferWrapper frameBuffers[2];
  ovrGLTexture eyeTextures[2];
  ovrEyeRenderDesc eyeRenderDescs[2];

public:
  SimpleScene() {
    ipd = ovrHmd_GetFloat(hmd, OVR_KEY_IPD, ipd);
    eyeHeight = ovrHmd_GetFloat(hmd, OVR_KEY_EYE_HEIGHT, eyeHeight);
    player = glm::inverse(glm::lookAt(
        glm::vec3(0, eyeHeight, ipd * 4.0f),
        glm::vec3(0, eyeHeight, 0),
        GlUtils::Y_AXIS));
  }

  void initGl() {
    RiftGlfwApp::initGl();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    gl::clearColor(Colors::darkGrey);

    ovrEyeDesc eyeDescs[2];
    for_each_eye([&](ovrEyeType eye){
      ovrEyeDesc & eyeDesc = eyeDescs[eye];
      eyeDesc.Eye = eye;
      eyeDesc.Fov = hmdDesc.DefaultEyeFov[eye];
      eyeDesc.TextureSize = ovrHmd_GetFovTextureSize(hmd, eye, hmdDesc.DefaultEyeFov[eye], 1.0f);
      eyeDesc.RenderViewport.Size = eyeDesc.TextureSize;
      eyeDesc.RenderViewport.Pos.x = 0;
      eyeDesc.RenderViewport.Pos.y = 0;
      frameBuffers[eye].init(Rift::fromOvr(eyeDesc.TextureSize));

      ovrTextureHeader & eyeTextureHeader = eyeTextures[eye].OGL.Header;
      eyeTextureHeader.API = ovrRenderAPI_OpenGL;
      eyeTextureHeader.TextureSize = eyeDesc.TextureSize;
      eyeTextureHeader.RenderViewport = eyeDesc.RenderViewport;
      eyeTextures[eye].OGL.TexId = frameBuffers[eye].color->texture;
    });

    ovrGLConfig cfg;
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.RTSize = hmdDesc.Resolution;
    cfg.OGL.Header.Multisample = 1;

    int distortionCaps = ovrDistortion_Chromatic | ovrDistortion_TimeWarp;
    int renderCaps = 0;
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg.Config,
      renderCaps, distortionCaps, eyeDescs, eyeRenderDescs);
  }

  virtual void update() {
    CameraControl::instance().applyInteraction(player);
    gl::Stacks::modelview().top() = glm::inverse(player);
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    if (CameraControl::instance().onKey(player, key, scancode, action, mods)) {
      return;
    }

    if (action == GLFW_PRESS) switch (key) {
    case GLFW_KEY_R:
      player = glm::inverse(glm::lookAt(
        glm::vec3(0, eyeHeight, ipd * 4.0f),
        glm::vec3(0, eyeHeight, 0),
        GlUtils::Y_AXIS));
      return;
    }

    RiftGlfwApp::onKey(key, scancode, action, mods);
  }

  void draw() {
    static int frameIndex = 0;
    ovrHmd_BeginFrame(hmd, frameIndex++);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::MatrixStack & pr = gl::Stacks::projection();
    for_each_eye([&](ovrEyeType eye) {
      const ovrEyeRenderDesc & erd = eyeRenderDescs[eye];

      frameBuffers[eye].activate();
      ovrPosef renderPose = ovrHmd_BeginEyeRender(hmd, eye);
      gl::Stacks::with_push([&]{
        // Set up the per-eye projection matrix
        pr.top() = Rift::fromOvr(erd.Desc.Fov);
        // Set up the per-eye modelview matrix
        // Apply the head pose
        mv.postMultiply(Rift::fromOvr(renderPose));
        // Apply the per-eye offset
        mv.preMultiply(glm::translate(glm::mat4(), Rift::fromOvr(erd.ViewAdjust)));

        renderScene();
      });
      frameBuffers[eye].deactivate();
      ovrHmd_EndEyeRender(hmd, eye, renderPose, &(eyeTextures[eye].Texture));
    });
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    ovrHmd_EndFrame(hmd);
//    glEnable(GL_CULL_FACE);
//    glEnable(GL_DEPTH_TEST);
  }

  void renderScene() {
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    GlUtils::renderFloorGrid(player);

    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::Stacks::with_push(mv, [&]{
      mv.translate(glm::vec3(0, eyeHeight, 0)).scale(ipd);
//      GlUtils::drawColorCube();
//      GlUtils::dancingCubes();
      GlUtils::cubeRecurse();
    });
  }
};

RUN_OVR_APP(SimpleScene);
