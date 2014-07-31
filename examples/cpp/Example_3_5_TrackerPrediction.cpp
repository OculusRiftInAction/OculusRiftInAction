#include "Common.h"

class SensorFusionPredictionExample : public GlfwApp {
  ovrHmd hmd;
  float predictionValue{ 0.030 };
  bool renderSensors{ false };

  glm::mat4 actual;
  glm::mat4 predicted;

public:

  SensorFusionPredictionExample() {
    hmd = ovrHmd_Create(0);
    if (!hmd) {
      FAIL("Unable to open HMD");
    }

    if (!ovrHmd_ConfigureTracking(hmd, ovrTrackingCap_Orientation, 0)) {
      FAIL("Unable to locate Rift sensor device");
    }
  }

  virtual ~SensorFusionPredictionExample() {
    ovrHmd_Destroy(hmd);
    hmd = nullptr;
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
      0.01f, 10000.0f);
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
      ovrHmd_RecenterPose(hmd);
      return;
    case GLFW_KEY_UP:
      predictionValue *= 1.5f;
      return;
    case GLFW_KEY_DOWN:
      predictionValue /= 1.5f;
      return;
    }
    GlfwApp::onKey(key, scancode, action, mods);
  }

  void update() {
    ovrTrackingState predictedState = ovrHmd_GetTrackingState(hmd, ovr_GetTimeInSeconds() + predictionValue);
    ovrTrackingState recordedState = ovrHmd_GetTrackingState(hmd, ovr_GetTimeInSeconds());
    // Update the modelview to reflect the orientation of the Rift
    actual = glm::mat4_cast(Rift::fromOvr(recordedState.HeadPose.ThePose.Orientation));
    predicted = glm::mat4_cast(Rift::fromOvr(predictedState.HeadPose.ThePose.Orientation));
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    std::string message = Platform::format(
        "Prediction Delta: %0.2f ms\n",
        predictionValue * 1000.0f);
    renderStringAt(message, -0.9f, 0.9f);

    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.with_push([&]{
      mv.transform(predicted);
      GlUtils::renderArtificialHorizon();
    });
    mv.with_push([&]{
      mv.transform(actual).scale(1.25f);
      GlUtils::renderArtificialHorizon(0.3f);
    });
  }

};

RUN_OVR_APP(SensorFusionPredictionExample)

