#include "Common.h"

struct PerEyeArg {
  StereoEye eye;
  glm::uvec2 viewportPosition;
  glm::mat4 projectionOffset;
  glm::mat4 modelviewOffset;
  RiftLookupTexturePtr distortionTexture;

  PerEyeArg(StereoEye eye) : eye(eye) { }
};

class SimpleScene : public RiftGlfwApp {
  glm::mat4 player;
  float ipd{ 0.06f };
  float eyeHeight{ 1.5f };

  std::array<PerEyeArg, 2> eyes{ { PerEyeArg(LEFT), PerEyeArg(RIGHT) } };

  float distortionScale{ 1.0f };

  gl::FrameBufferWrapper frameBuffer;
  gl::ProgramPtr distortProgram;
  gl::GeometryPtr quadGeometry;

  OVR::SensorFusion sensorFusion;
  OVR::Ptr<OVR::SensorDevice> ovrSensor;
  glm::mat4 riftOrientation;

public:
  SimpleScene() {
    OVR::Ptr<OVR::ProfileManager> profileManager =
      *OVR::ProfileManager::Create();
    OVR::Ptr<OVR::Profile> profile =
      *(profileManager->GetDeviceDefaultProfile(
      OVR::ProfileType::Profile_RiftDK1));
    ipd = profile->GetIPD();
    eyeHeight = profile->GetEyeHeight();

    // setup the initial player location
    player = glm::inverse(glm::lookAt(
      glm::vec3(0, eyeHeight, ipd * 4.0f),
      glm::vec3(0, eyeHeight, 0),
      GlUtils::Y_AXIS));

    OVR::Util::Render::StereoConfig ovrStereoConfig;
    ovrStereoConfig.SetHMDInfo(ovrHmdInfo);

    gl::Stacks::projection().top() =
      glm::perspective(ovrStereoConfig.GetYFOVRadians(),
      glm::aspect(eyeSize), 0.01f, 1000.0f);

    eyes[LEFT].viewportPosition =
      glm::uvec2(0, 0);
    eyes[LEFT].modelviewOffset = glm::translate(glm::mat4(),
      glm::vec3(ipd / 2.0f, 0, 0));
    eyes[LEFT].projectionOffset = glm::translate(glm::mat4(),
      glm::vec3(ovrStereoConfig.GetProjectionCenterOffset(), 0, 0));

    eyes[RIGHT].viewportPosition =
      glm::uvec2(hmdNativeResolution.x / 2, 0);
    eyes[RIGHT].modelviewOffset = glm::translate(glm::mat4(),
      glm::vec3(-ipd / 2.0f, 0, 0));
    eyes[RIGHT].projectionOffset = glm::translate(glm::mat4(),
      glm::vec3(-ovrStereoConfig.GetProjectionCenterOffset(), 0, 0));

    distortionScale = ovrStereoConfig.GetDistortionScale();

    ovrSensor =
      *ovrManager->EnumerateDevices<OVR::SensorDevice>().
      CreateDevice();
    if (ovrSensor) {
      sensorFusion.AttachToSensor(ovrSensor);
    }

    if (!sensorFusion.IsAttachedToSensor()) {
      SAY_ERR("Could not attach to sensor device");
    }
  }

  virtual ~SimpleScene() {
  }

  void initGl() {
    RiftGlfwApp::initGl();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    gl::clearColor(Colors::darkGrey);

    distortProgram = GlUtils::getProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_RIFTWARP_FS);
    distortProgram->use();
    distortProgram->setUniform1i("OffsetMap", 1);
    distortProgram->setUniform1i("Scene", 0);
    gl::Program::clear();

    quadGeometry = GlUtils::getQuadGeometry();
    frameBuffer.init(glm::uvec2(glm::vec2(eyeSize) * 2.0f));

    RiftDistortionHelper distortionHelper(ovrHmdInfo);
    for_each_eye([&](StereoEye eye) {
      eyes[eye].distortionTexture =
          distortionHelper.createLookupTexture(glm::uvec2(512, 512), eye);
    });
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

  virtual void update() {
    CameraControl::instance().applyInteraction(player);
    riftOrientation = Rift::getMat4(sensorFusion);

    static const glm::vec4 EYE_ROTATION_OFFSET(0, 0.15f, -0.09f, 1);
    glm::vec3 riftImposedTranslation =
      glm::vec3(glm::inverse(riftOrientation) * EYE_ROTATION_OFFSET);
    riftImposedTranslation -= glm::vec3(EYE_ROTATION_OFFSET);
    glm::mat4 playerRiftTranslation =
      glm::translate(glm::mat4(), riftImposedTranslation);
    gl::Stacks::modelview().top() = riftOrientation *
      glm::inverse(playerRiftTranslation * player);
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::MatrixStack & pr = gl::Stacks::projection();

    for_each_eye([&](StereoEye eye) {
      const PerEyeArg & eyeArgs = eyes[eye];
      frameBuffer.activate();
      glEnable(GL_DEPTH_TEST);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      gl::Stacks::with_push(pr, mv, [&]{
        mv.preMultiply(eyeArgs.modelviewOffset);
        pr.preMultiply(eyeArgs.projectionOffset);
        renderScene();
      });

      frameBuffer.deactivate();
      glDisable(GL_DEPTH_TEST);

      viewport(eye);
      distortProgram->use();
      glActiveTexture(GL_TEXTURE1);
      eyeArgs.distortionTexture->bind();
      glActiveTexture(GL_TEXTURE0);
      frameBuffer.color->bind();
      quadGeometry->bindVertexArray();
      quadGeometry->draw();
      gl::VertexArray::unbind();
      gl::Program::clear();
    });
  }

  void renderScene() {
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
