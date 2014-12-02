#include "Common.h"


typedef std::shared_ptr<oglplus::Buffer> BufferPtr;

class SensorFusionPredictionExample : public GlfwApp {
  ovrHmd hmd;
  float predictionValue{ 0.030 };
  bool renderSensors{ false };

  glm::mat4 actual;
  glm::mat4 predicted;

public:

  SensorFusionPredictionExample() {
  }

  virtual ~SensorFusionPredictionExample() {
    ovrHmd_Destroy(hmd);
    hmd = nullptr;
    ovr_Shutdown();
  }


  virtual GLFWwindow * createRenderingTarget(glm::uvec2 & outSize, glm::ivec2 & outPosition) {
    outSize = glm::uvec2(800, 600);
    outPosition = glm::ivec2(100, 100);
    Stacks::projection().top() = glm::perspective(
      PI / 3.0f, aspect(outSize),
      0.01f, 10000.0f);
    Stacks::modelview().top() = glm::lookAt(
      glm::vec3(0.0f, 0.0f, 3.5f),
      Vectors::ORIGIN, Vectors::UP);

    return glfw::createWindow(outSize, outPosition);
  }

  void initGl() {
    GlfwApp::initGl();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    Stacks::projection().top() = glm::perspective(
      PI / 3.0f, aspect(getSize()),
      0.01f, 10000.0f);
    Stacks::modelview().top() = glm::lookAt(
      glm::vec3(0.0f, 0.0f, 3.5f),
      Vectors::ORIGIN, Vectors::UP);

    ovr_Initialize();
    hmd = ovrHmd_Create(0);
    if (!hmd) {
      FAIL("Unable to open HMD");
    }

    if (!ovrHmd_ConfigureTracking(hmd, ovrTrackingCap_Orientation, 0)) {
      FAIL("Unable to locate Rift sensor device");
    }

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
    actual = glm::mat4_cast(ovr::toGlm(recordedState.HeadPose.ThePose.Orientation));
    predicted = glm::mat4_cast(ovr::toGlm(predictedState.HeadPose.ThePose.Orientation));
  }

  void draw() {
    oglplus::Context::Clear().ColorBuffer().DepthBuffer();
    
    std::string message = Platform::format(
        "Prediction Delta: %0.2f ms\n",
        predictionValue * 1000.0f);
    renderStringAt(message, -0.9f, 0.9f);

    MatrixStack & mv = Stacks::modelview();
    mv.withPush([&]{
      mv.transform(predicted);
      oria::renderArtificialHorizon();
    });
    mv.withPush([&]{
      mv.transform(actual).scale(1.25f);
      oria::renderArtificialHorizon(0.3f);
    });
  }

};

RUN_APP(SensorFusionPredictionExample)

