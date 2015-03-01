#include "Common.h"

class SensorFusionExample : public GlfwApp {
  ovrHmd hmd;
  glm::quat orientation;
  glm::vec3 linearA;
  glm::vec3 angularV;

  bool renderSensors{ false };

public:

  SensorFusionExample() {
  }

  virtual ~SensorFusionExample() {
    ovrHmd_Destroy(hmd);
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

    GLFWwindow * result = glfw::createWindow(outSize, outPosition);

    ovr_Initialize();
    hmd = ovrHmd_Create(0);

    if (!hmd || !ovrHmd_ConfigureTracking(hmd, ovrTrackingCap_Orientation, 0)) {
      FAIL("Unable to locate Rift sensors");
    }
    return result;
  }

  void initGl() {
    GlfwApp::initGl();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action) {
      return;
    }

    switch (key) {
    case GLFW_KEY_S:
      renderSensors = !renderSensors;
      return;
    case GLFW_KEY_R:
      ovrHmd_RecenterPose(hmd);
      return;
    }

    GlfwApp::onKey(key, scancode, action, mods);
  }

  void update() {
    ovrTrackingState trackingState = ovrHmd_GetTrackingState(hmd, 0);
    ovrPoseStatef & poseState = trackingState.HeadPose;

    orientation = ovr::toGlm(poseState.ThePose.Orientation);
    linearA = ovr::toGlm(poseState.LinearAcceleration);
    angularV = ovr::toGlm(poseState.AngularVelocity);
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    MatrixStack & mv = Stacks::modelview();
    mv.withPush([&]{
      mv.rotate(orientation); 
      oria::renderRift();
    });

    MatrixStack & pr = Stacks::projection();
    pr.withPush([&]{
      pr.top() = glm::ortho(
          -1.0f, 1.0f,
          -windowAspectInverse, windowAspectInverse,
          -100.0f, 100.0f);

      // Text display of our current sensor settings
      mv.withPush([&]{
        mv.identity();
        glm::vec3 euler = glm::eulerAngles(orientation);
        glm::vec2 cursor(-0.9, windowAspectInverse * 0.9);
        std::string message = Platform::format(
            "Current orientation\n"
            "roll  %0.2f\n"
            "pitch %0.2f\n"
            "yaw   %0.2f",
            euler.z * RADIANS_TO_DEGREES,
            euler.x * RADIANS_TO_DEGREES,
            euler.y * RADIANS_TO_DEGREES);
        oria::renderString(message, cursor, 18.0f);
      });

      if (renderSensors) {
        mv.withPush([&]{
          mv.top() = glm::lookAt(
              glm::vec3(3.0f, 1.0f, 3.0f),
              Vectors::ORIGIN, Vectors::UP);
          mv.translate(glm::vec3(0.75f, -0.3f, 0.0f));
          mv.scale(0.2f);

          oria::draw3dGrid();
          oria::draw3dVector(linearA, Colors::green);
          oria::draw3dVector(angularV, Colors::yellow);
        });
      }
    });
  }
};

RUN_APP(SensorFusionExample)

