#include <OVR.h>
#include "Common.h"

using namespace gl;

class SensorFusionPredictionExample : public GlfwApp {
  OVR::Ptr<OVR::DeviceManager> ovrManager;
  OVR::Ptr<OVR::SensorDevice> ovrSensor;
  OVR::Ptr<OVR::LatencyTestDevice> ovrLatencyTester;
  OVR::SensorFusion sensorFusion;
  OVR::Util::LatencyTest latencyTest;

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

    ovrLatencyTester = *ovrManager->EnumerateDevices<OVR::LatencyTestDevice>().CreateDevice();
    if (ovrLatencyTester) {
      latencyTest.SetDevice(ovrLatencyTester);
    }
    sensorFusion.SetGravityEnabled(true);
    sensorFusion.SetPredictionEnabled(true);
    sensorFusion.SetYawCorrectionEnabled(true);
    sensorFusion.AttachToSensor(ovrSensor);
  }

  void createRenderingTarget() {
    OVR::HMDInfo hmdInfo;
    Rift::getHmdInfo(ovrManager, hmdInfo);
    createWindow(glm::ivec2(800, 600), glm::ivec2(100, 100));
  }

  void initGl() {
    GlfwApp::initGl();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    gl::Stacks::projection().top() = glm::perspective(
        60.0f, glm::aspect(windowSize),
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
    /*
     * The SensorFusion object is continuously being updated in the
     * background, so we want to ensure that we only ever fetch information
     * from it once per frame (meaning one fetch for both eyes).
     *
     * If we were to fetch it repeatedly over the course of the render, the
     * value might change, which would lead to undesirable artifacts in
     * rendering
     */
    currentOrientation = Rift::fromOvr(sensorFusion.GetOrientation());
    predictedOrientation = Rift::fromOvr(sensorFusion.GetPredictedOrientation());
    // >>> THIS MUST BE PLACED AS CLOSE AS POSSIBLE TO WHERE THE HMD ORIENTATION IS READ <<<
    latencyTest.ProcessInputs();
    const char * results = latencyTest.GetResultsString();
    if (results) {
      SAY(results);
    }
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.push().rotate(predictedOrientation);
    GlUtils::renderArtificialHorizon();
    mv.pop();
    mv.push().rotate(currentOrientation);
    mv.scale(1.25f);
    GlUtils::renderArtificialHorizon(0.3f);
    mv.pop();


    // Display colored quad if we're doing a latency test.
    OVR::Color colorToDisplay;
    if (latencyTest.DisplayScreenColor(colorToDisplay)) {
      static gl::GeometryPtr geometry = GlUtils::getQuadGeometry();
      static gl::ProgramPtr program = GlUtils::getProgram(
          Resource::SHADERS_SIMPLE_VS,
          Resource::SHADERS_SIMPLE_FS);
      static glm::vec4 color;
      gl::Stacks::modelview().push().identity();
      gl::Stacks::projection().push().identity();
      colorToDisplay.GetRGBA(&color.r, &color.g, &color.b, &color.a);
      program->use();
      program->setUniform("Color", color);
      GlUtils::renderGeometry(geometry, program);
      gl::Stacks::modelview().pop();
      gl::Stacks::projection().pop();
    }

  }

};

RUN_OVR_APP(SensorFusionPredictionExample)

