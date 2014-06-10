#include "Common.h"

static const glm::uvec2 WINDOW_SIZE(1280, 800);
static const glm::ivec2 WINDOW_POS(100, 100);

class SimpleScene: public RiftGlfwApp {
  glm::mat4 player;
  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_PLAYER_HEIGHT };

public:
  SimpleScene() {
    eyeHeight = ovrHmd_GetFloat(hmd, 
        OVR_KEY_PLAYER_HEIGHT, 
        OVR_DEFAULT_PLAYER_HEIGHT);
    ipd = ovrHmd_GetFloat(hmd, OVR_KEY_IPD, OVR_DEFAULT_IPD);
    resetCamera();

    gl::Stacks::projection().top() = glm::perspective(
        PI / 2.0f, glm::aspect(WINDOW_SIZE), 0.01f, 1000.0f);
  }

  virtual ~SimpleScene() {
  }

  void createRenderingTarget() {
    createWindow(WINDOW_SIZE, WINDOW_POS);
  }

  void initGl() {
    GlfwApp::initGl();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    gl::clearColor(Colors::darkGrey);
  }

  virtual void update() {
    CameraControl::instance().applyInteraction(player);
    gl::Stacks::modelview().top() = glm::inverse(player);
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    if (!CameraControl::instance().onKey(player, key, scancode, action, mods)) {
      if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_R:
          resetCamera();
          break;
        }
      } else {
        GlfwApp::onKey(key, scancode, action, mods);
      }
    }
  }

  void resetCamera() {
    player = glm::inverse(glm::lookAt(
        glm::vec3(0, eyeHeight, 1),  // Position of the camera
        glm::vec3(0, eyeHeight, 0),  // Where the camera is looking
        GlUtils::Y_AXIS));           // Camera up axis
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::MatrixStack & pr = gl::Stacks::projection();
    gl::Stacks::with_push(mv, pr, [&]{
      renderScene();
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
