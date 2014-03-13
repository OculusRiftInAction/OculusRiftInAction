#include "Common.h"

class SensorFusionExample : public GlfwApp {
  OVR::Ptr<OVR::DeviceManager> ovrManager;
  OVR::Ptr<OVR::SensorDevice> ovrSensor;
  OVR::SensorFusion ovrSensorFusion;
  OVR::Matrix4f currentOrientation;

  bool renderSensors{ false };

public:

  SensorFusionExample() {
    ovrManager = *OVR::DeviceManager::Create();
    if (!ovrManager) {
      FAIL("Unable to create device manager");
    }

    ovrSensor = *ovrManager->EnumerateDevices<OVR::SensorDevice>().CreateDevice();
    if (!ovrSensor) {
      FAIL("Unable to locate Rift sensor device");
    }

    ovrSensorFusion.SetGravityEnabled(false);
    ovrSensorFusion.SetPredictionEnabled(false);
    ovrSensorFusion.SetYawCorrectionEnabled(false);
    ovrSensorFusion.AttachToSensor(ovrSensor);
  }

  void createRenderingTarget() {
    createWindow(glm::uvec2(1280, 800), glm::ivec2(100, 100));
  }

  void initGl() {
    GlfwApp::initGl();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    gl::Stacks::projection().top() = glm::perspective(
        PI / 3.0f, glm::aspect(windowSize),
        Rift::ZNEAR, Rift::ZFAR);
    gl::Stacks::modelview().top() = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.5f),
        GlUtils::ORIGIN, GlUtils::UP);
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action && GLFW_REPEAT != action) {
      return;
    }

    switch (key) {
    case GLFW_KEY_G:
      ovrSensorFusion.SetGravityEnabled(
          !ovrSensorFusion.IsGravityEnabled());
      ovrSensorFusion.Reset();
      return;

    case GLFW_KEY_M:
      ovrSensorFusion.SetYawCorrectionEnabled(
          !ovrSensorFusion.IsYawCorrectionEnabled());
      ovrSensorFusion.Reset();
      return;

    case GLFW_KEY_P:
      ovrSensorFusion.SetPredictionEnabled(
          !ovrSensorFusion.IsPredictionEnabled());
      ovrSensorFusion.Reset();
      return;

    case GLFW_KEY_S:
      renderSensors = !renderSensors;
      return;

    case GLFW_KEY_UP:
      if (ovrSensorFusion.IsPredictionEnabled()) {
        ovrSensorFusion.SetPrediction(
            ovrSensorFusion.GetPredictionDelta() + 0.010f);
      }
      return;

    case GLFW_KEY_DOWN:
      if (ovrSensorFusion.IsPredictionEnabled()) {
        ovrSensorFusion.SetPrediction(
            ovrSensorFusion.GetPredictionDelta() - 0.010f);
      }
      return;

    case GLFW_KEY_R:
      ovrSensorFusion.Reset();
      return;
    }

    GlfwApp::onKey(key, scancode, action, mods);
  }

  void update() {
    currentOrientation = ovrSensorFusion.GetPredictedOrientation();
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::MatrixStack & pr = gl::Stacks::projection();
    
    // Update the modelview to reflect the orientation of the Rift
    glm::mat4 glm_mat = glm::make_mat4((float*)currentOrientation.M);
    mv.push().transform(glm::inverse(glm_mat));
    GlUtils::renderRift();
    mv.pop();

    // Text display of our current sensor settings
    mv.push().identity();
    pr.push().top() = glm::ortho(
        -1.0f, 1.0f,
        -windowAspectInverse, windowAspectInverse,
        -100.0f, 100.0f);
    glm::vec2 cursor(-0.9, windowAspectInverse * 0.9);
    std::string message = Platform::format(
        "Gravity Correction: %s\n"
        "Magnetic Correction: %s\n"
        "Prediction Delta: %0.2f ms\n",
        ovrSensorFusion.IsGravityEnabled() ? "Enabled" : "Disabled",
        ovrSensorFusion.IsYawCorrectionEnabled() ? "Enabled" : "Disabled",
        ovrSensorFusion.IsPredictionEnabled() ?
            ovrSensorFusion.GetPredictionDelta() * 1000.0f : 0.0f );
    GlUtils::renderString(message, cursor, 18.0f);

    // Render a set of axes and local basis to show the Rift's sensor output
    if (renderSensors) {
      mv.top() = glm::lookAt(
        glm::vec3(3.0f, 1.0f, 3.0f),
        GlUtils::ORIGIN, GlUtils::UP);
      mv.translate(glm::vec3(0.75f, -0.3f, 0.0f));
      mv.scale(0.2f);

      glm::vec3 acc = Rift::fromOvr(ovrSensorFusion.GetAcceleration());
      glm::vec3 rot = Rift::fromOvr(ovrSensorFusion.GetAngularVelocity());
      glm::vec3 mag = Rift::fromOvr(ovrSensorFusion.GetMagnetometer());
      GlUtils::draw3dGrid();
      GlUtils::draw3dVector(acc / 9.8f, Colors::white);
      GlUtils::draw3dVector(rot, Colors::yellow);
      GlUtils::draw3dVector(mag, Colors::cyan);
    }

    mv.pop(); 
    pr.pop();
  }
};

RUN_OVR_APP(SensorFusionExample)

