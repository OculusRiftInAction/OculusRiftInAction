#include <OVR.h>
#include "Common.h"

class SensorFusionExample : public GlfwApp {
  OVR::Ptr<OVR::DeviceManager> ovrManager;
  OVR::Ptr<OVR::SensorDevice> ovrSensor;
  OVR::SensorFusion sensorFusion;
  glm::quat currentOrientation;
  bool renderSensors;

public:

  SensorFusionExample() {
    ovrManager = *OVR::DeviceManager::Create();
    renderSensors = false;
    if (!ovrManager) {
      FAIL("Unable to create device manager");
    }

    ovrSensor =
      *ovrManager->EnumerateDevices<OVR::SensorDevice>().CreateDevice();
    if (!ovrSensor) {
      FAIL("Unable to locate Rift sensor device");
    }

    sensorFusion.SetGravityEnabled(false);
    sensorFusion.SetPredictionEnabled(false);
    sensorFusion.SetYawCorrectionEnabled(false);
    sensorFusion.AttachToSensor(ovrSensor);
  }

  void createRenderingTarget() {
    createWindow(glm::ivec2(1280, 800), glm::ivec2(100, 100));
  }

  void initGl() {
    GlfwApp::initGl();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    gl::Stacks::projection().top() = glm::perspective(
      60.0f, glm::aspect(windowSize),
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
      sensorFusion.SetGravityEnabled(
          !sensorFusion.IsGravityEnabled());
      sensorFusion.Reset();
      return;

    case GLFW_KEY_M:
      sensorFusion.SetYawCorrectionEnabled(
          !sensorFusion.IsYawCorrectionEnabled());
      sensorFusion.Reset();
      return;

    case GLFW_KEY_P:
      sensorFusion.SetPredictionEnabled(
          !sensorFusion.IsPredictionEnabled());
      sensorFusion.Reset();
      return;

    case GLFW_KEY_S:
      renderSensors = !renderSensors;
      return;

    case GLFW_KEY_UP:
      if (sensorFusion.IsPredictionEnabled()) {
        sensorFusion.SetPrediction(
            sensorFusion.GetPredictionDelta() + 0.010f);
      }
      return;

    case GLFW_KEY_DOWN:
      if (sensorFusion.IsPredictionEnabled()) {
        sensorFusion.SetPrediction(
            sensorFusion.GetPredictionDelta() - 0.010f);
      }
      return;

    case GLFW_KEY_R:
      sensorFusion.Reset();
      return;
    }

    GlfwApp::onKey(key, scancode, action, mods);
  }

  void update() {
    currentOrientation = Rift::fromOvr(sensorFusion.GetPredictedOrientation());
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::MatrixStack & pr = gl::Stacks::projection();

    mv.push().rotate(glm::inverse(currentOrientation));
    GlUtils::renderRift();
    mv.pop();

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
        sensorFusion.IsGravityEnabled() ? "Enabled" : "Disabled",
        sensorFusion.IsYawCorrectionEnabled() ? "Enabled" : "Disabled",
        sensorFusion.IsPredictionEnabled() ?
            sensorFusion.GetPredictionDelta() * 1000.0f : 0.0f );
    GlUtils::renderString(message, cursor, 18.0f);

    if (renderSensors) {
      mv.top() = glm::lookAt(
        glm::vec3(3.0f, 1.0f, 3.0f),
        GlUtils::ORIGIN, GlUtils::UP);
      mv.translate(glm::vec3(0.75f, -0.3f, 0.0f));
      mv.scale(0.2f);

      glm::vec3 acc = Rift::fromOvr(sensorFusion.GetAcceleration());
      acc /= 9.8f;
      glm::vec3 rot = Rift::fromOvr(sensorFusion.GetAngularVelocity());
      glm::vec3 mag = Rift::fromOvr(sensorFusion.GetMagnetometer());
      GlUtils::draw3dGrid();
      GlUtils::draw3dVector(acc, Colors::white);
      GlUtils::draw3dVector(rot, Colors::yellow);
      GlUtils::draw3dVector(mag, Colors::cyan);
    }


    pr.pop();
    mv.pop();
  }
};

RUN_OVR_APP(SensorFusionExample)

