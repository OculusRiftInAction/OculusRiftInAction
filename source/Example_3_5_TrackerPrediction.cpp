#include "Common.h"

class SensorFusionPredictionExample : public GlfwApp {
  OVR::Ptr<OVR::DeviceManager> ovrManager;
  OVR::Ptr<OVR::SensorDevice> ovrSensor;
  OVR::SensorFusion sensorFusion;

  glm::quat currentOrientation;
  glm::quat predictedOrientation;

public:

  SensorFusionPredictionExample() {
    ovrManager = *OVR::DeviceManager::Create();
    if (!ovrManager) {
      FAIL("Unable to create device manager");
    }

    ovrSensor =
        *ovrManager->EnumerateDevices<OVR::SensorDevice>().CreateDevice();
    if (!ovrSensor) {
      FAIL("Unable to locate Rift sensor device");
    }

    sensorFusion.SetGravityEnabled(true);
    sensorFusion.SetPredictionEnabled(true);
    sensorFusion.SetYawCorrectionEnabled(true);
    sensorFusion.AttachToSensor(ovrSensor);
  }

  void createRenderingTarget() {
    createWindow(glm::ivec2(1280, 800), glm::ivec2(100, 100));
  }

  void initGl() {
    GlfwApp::initGl();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    gl::Stacks::projection().top() = glm::perspective(
        TAU / 6.0f, glm::aspect(windowSize),
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
      sensorFusion.Reset();
      return;
    case GLFW_KEY_UP:
      sensorFusion.SetPrediction(sensorFusion.GetPredictionDelta() * 1.5f, true);
      return;
    case GLFW_KEY_DOWN:
      sensorFusion.SetPrediction(sensorFusion.GetPredictionDelta() / 1.5f, true);
      return;
    }
    GlfwApp::onKey(key, scancode, action, mods);
  }

  void update() {
    currentOrientation = Rift::fromOvr(sensorFusion.GetOrientation());
    predictedOrientation = Rift::fromOvr(sensorFusion.GetPredictedOrientation());
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    std::string message = Platform::format(
      "Prediction Delta: %0.2f ms\n",
      sensorFusion.GetPredictionDelta() * 1000.0f);
    renderStringAt(message, -0.9f, 0.9f);

    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::MatrixStack & pr = gl::Stacks::projection();

    mv.push().rotate(predictedOrientation);
    GlUtils::renderArtificialHorizon();
    mv.pop();

    mv.push().rotate(currentOrientation);
    mv.scale(1.25f);
    GlUtils::renderArtificialHorizon(0.3f);
    mv.pop();
  }

};

RUN_OVR_APP(SensorFusionPredictionExample)

