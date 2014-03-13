#include "Common.h"

class SensorFusionPredictionExample : public GlfwApp {
  OVR::Ptr<OVR::DeviceManager> ovrManager;
  OVR::Ptr<OVR::SensorDevice> ovrSensor;
  OVR::SensorFusion ovrSensorFusion;

  OVR::Matrix4f actual;
  OVR::Matrix4f predicted;

public:

  SensorFusionPredictionExample() {
    ovrManager = *OVR::DeviceManager::Create();
    if (!ovrManager) {
      FAIL("Unable to create device manager");
    }

    ovrSensor = *ovrManager->EnumerateDevices<OVR::SensorDevice>().CreateDevice();
    if (!ovrSensor) {
      FAIL("Unable to locate Rift sensor device");
    }

    ovrSensorFusion.SetGravityEnabled(true);
    ovrSensorFusion.SetPredictionEnabled(true);
    ovrSensorFusion.SetYawCorrectionEnabled(true);
    ovrSensorFusion.AttachToSensor(ovrSensor);
  }

  void createRenderingTarget() {
    createWindow(glm::uvec2(1280, 800), glm::ivec2(100, 100));
  }

  void initGl() {
    GlfwApp::initGl();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    gl::Stacks::projection().top() = glm::perspective(
        PI / 3.0f, glm::aspect(windowSize),
        Rift::ZNEAR, Rift::ZFAR);
    gl::Stacks::modelview().top() = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.5f),
        GlUtils::ORIGIN, GlUtils::UP);
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action) {
      return;
    }

    switch (key) {
    case GLFW_KEY_R:
      ovrSensorFusion.Reset();
      return;
    case GLFW_KEY_UP:
      ovrSensorFusion.SetPrediction(ovrSensorFusion.GetPredictionDelta() * 1.5f, true);
      return;
    case GLFW_KEY_DOWN:
      ovrSensorFusion.SetPrediction(ovrSensorFusion.GetPredictionDelta() / 1.5f, true);
      return;
    }
    GlfwApp::onKey(key, scancode, action, mods);
  }

  void update() {
    actual = ovrSensorFusion.GetOrientation();
    predicted = ovrSensorFusion.GetPredictedOrientation();
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    std::string message = Platform::format(
        "Prediction Delta: %0.2f ms\n",
        ovrSensorFusion.GetPredictionDelta() * 1000.0f);
    renderStringAt(message, -0.9f, 0.9f);

    gl::MatrixStack & mv = gl::Stacks::modelview();
    glm::mat4 glm_mat;

    glm_mat = glm::make_mat4((float*) predicted.M);
    mv.push().transform(glm_mat);
    GlUtils::renderArtificialHorizon();
    mv.pop();

    glm_mat = glm::make_mat4((float*) actual.M);
    mv.push().transform(glm_mat);
    mv.scale(1.25f);
    GlUtils::renderArtificialHorizon(0.3f);
    mv.pop();
  }

};

RUN_OVR_APP(SensorFusionPredictionExample)

