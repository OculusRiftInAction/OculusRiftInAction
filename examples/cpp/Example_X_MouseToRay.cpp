#include "Common.h"

class PickingExample : public RiftApp {
  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_PLAYER_HEIGHT };


public:
  PickingExample() {
    ipd = ovrHmd_GetFloat(hmd, OVR_KEY_IPD, OVR_DEFAULT_IPD);
    eyeHeight = ovrHmd_GetFloat(hmd, OVR_KEY_PLAYER_HEIGHT, OVR_DEFAULT_PLAYER_HEIGHT);
    resetCamera();
  }

  ~PickingExample() {
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    if (CameraControl::instance().onKey(key, scancode, action, mods)) {
      return;
    }

    if (GLFW_PRESS != action) {
      GlfwApp::onKey(key, scancode, action, mods);
      return;
    }

    int caps = ovrHmd_GetEnabledCaps(hmd);
    switch (key) {
    case GLFW_KEY_R:
      resetCamera();
      break;

    default:
      GlfwApp::onKey(key, scancode, action, mods);
      break;
    }
  }

  void onMouseButton(int button, int action, int mods) {
    if (GLFW_RELEASE == action) {
      glm::dvec2 pos;
      glfwGetCursorPos(window, &pos.x, &pos.y);
      SAY("Button %d %d", (int)pos.x, (int)pos.y);
    }
  }

  virtual void update() {
    CameraControl::instance().applyInteraction(player);
    gl::Stacks::modelview().top() = glm::inverse(player);
  }


  void resetCamera() {
    player = glm::inverse(glm::lookAt(
      glm::vec3(0, eyeHeight, 1),  // Position of the camera
      glm::vec3(0, eyeHeight, 0),  // Where the camera is looking
      GlUtils::Y_AXIS));           // Camera up axis
    ovrHmd_RecenterPose(hmd);
  }

  void renderScene() {
    int currentEye = getCurrentEye();
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    GlUtils::renderFloor();

    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::MatrixStack & pr = gl::Stacks::projection();
    mv.withPush([&]{
      mv.translate(glm::vec3(0, eyeHeight, 0)).scale(ipd);
      GlUtils::drawColorCube(true);
    });
  }
};

RUN_OVR_APP(PickingExample);
