#include "Common.h"
struct PerEyeArg {
  glm::mat4                     modelviewOffset;
  glm::mat4                     projection;
  gl::FrameBufferWrapper        frameBuffer;
  ovrFovPort                    fovPort;
  ovrGLTexture                  texture;
  ovrEyeRenderDesc              renderDesc;
};

class SimpleScene: public RiftGlfwApp {
  glm::mat4 player;
  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_PLAYER_HEIGHT };

  PerEyeArg              eyes[2];
  int                    frameIndex{ 0 };

public:
  SimpleScene() {
    ipd = ovrHmd_GetFloat(hmd, OVR_KEY_IPD, OVR_DEFAULT_IPD);
    eyeHeight = ovrHmd_GetFloat(hmd, OVR_KEY_PLAYER_HEIGHT, OVR_DEFAULT_PLAYER_HEIGHT);

    // setup the initial player location
    player = glm::inverse(glm::lookAt(
        glm::vec3(0, eyeHeight, ipd * 4.0f),
        glm::vec3(0, eyeHeight, 0),
        GlUtils::Y_AXIS));
    eyes[ovrEye_Left].modelviewOffset = glm::translate(glm::mat4(),
        glm::vec3(ipd / 2.0f, 0, 0));
    eyes[ovrEye_Right].modelviewOffset = glm::translate(glm::mat4(),
        glm::vec3(-ipd / 2.0f, 0, 0));
    windowSize = glm::uvec2(1280, 800);
  }

  ~SimpleScene() {
    ovrHmd_Destroy(hmd);
  }

  void initGl() {
    RiftGlfwApp::initGl();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    gl::clearColor(Colors::darkGrey);

    for_each_eye([&](ovrEyeType eye){
      PerEyeArg & eyeArg = eyes[eye];
      eyeArg.fovPort = hmdDesc.DefaultEyeFov[eye];
      ovrTextureHeader & textureHeader = eyeArg.texture.Texture.Header;
      textureHeader.API = ovrRenderAPI_OpenGL;
      ovrSizei texSize = ovrHmd_GetFovTextureSize(hmd, eye, eyeArg.fovPort, 1.0f);
      textureHeader.TextureSize = texSize;
      textureHeader.RenderViewport.Size = texSize;
      textureHeader.RenderViewport.Pos.x = 0;
      textureHeader.RenderViewport.Pos.y = 0;
      eyeArg.frameBuffer.init(Rift::fromOvr(texSize));
      eyeArg.texture.OGL.TexId = eyeArg.frameBuffer.color->texture;

      ovrMatrix4f projection = ovrMatrix4f_Projection(eyeArg.fovPort, 0.01, 100000, true);
      eyeArg.projection = Rift::fromOvr(projection);
    });

    ovrRenderAPIConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.Header.API = ovrRenderAPI_OpenGL;
    cfg.Header.RTSize = hmdDesc.Resolution;
    cfg.Header.Multisample = 1;

    int distortionCaps = ovrDistortionCap_Chromatic;
    int renderCaps = 0;
    ovrFovPort eyePorts[] = { eyes[0].fovPort, eyes[1].fovPort };
    ovrEyeRenderDesc eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg,
      distortionCaps, eyePorts, eyeRenderDescs);
  }

  virtual void update() {
    CameraControl::instance().applyInteraction(player);
    gl::Stacks::modelview().top() = glm::inverse(player);
  }

  virtual void finishFrame() {

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
    ovrHmd_BeginFrame(hmd, frameIndex++);

    gl::MatrixStack & mv = gl::Stacks::modelview();
    for (int i = 0; i < ovrEye_Count; ++i) {
      ovrEyeType eye = hmdDesc.EyeRenderOrder[i];
      PerEyeArg & eyeArgs = eyes[eye];
      gl::Stacks::projection().top() = eyeArgs.projection;

      ovrPosef renderPose = ovrHmd_BeginEyeRender(hmd, eye);

      eyeArgs.frameBuffer.withFramebufferActive([&]{
        gl::Stacks::with_push(mv, [&]{
          mv.preMultiply(eyeArgs.modelviewOffset);
          renderScene();
        });
      });

      ovrHmd_EndEyeRender(hmd, eye, renderPose, &eyeArgs.texture.Texture);
    }

    ovrHmd_EndFrame(hmd);
  }

  void renderScene() {
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    GlUtils::renderFloorGrid(player);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::Stacks::with_push(mv, [&]{
      mv.translate(glm::vec3(0, eyeHeight, 0)).scale(ipd);
      GlUtils::drawColorCube(true);
    });
  }
};

RUN_OVR_APP(SimpleScene);
