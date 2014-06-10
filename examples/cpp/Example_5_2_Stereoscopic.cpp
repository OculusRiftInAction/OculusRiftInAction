#include "Common.h"

static const glm::uvec2 WINDOW_SIZE(1280, 800);
static const glm::ivec2 WINDOW_POS(100, 100);

static const glm::uvec2 EYE_SIZE(WINDOW_SIZE.x / 2, WINDOW_SIZE.y);
static const float EYE_ASPECT = glm::aspect(EYE_SIZE);

struct PerEyeArg {
  glm::uvec2 viewportPosition;
  glm::mat4 modelviewOffset;
};

class SimpleScene : public RiftGlfwApp {
  glm::mat4 player;
  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_PLAYER_HEIGHT };

  std::map<ovrEyeType, PerEyeArg> eyes;

  bool applyModelviewOffset{ true };

public:
  SimpleScene() {
    ipd = ovrHmd_GetFloat(hmd, OVR_KEY_IPD, OVR_DEFAULT_IPD);
    eyeHeight = ovrHmd_GetFloat(hmd, OVR_KEY_PLAYER_HEIGHT, OVR_DEFAULT_PLAYER_HEIGHT);

    player = glm::inverse(glm::lookAt(
        glm::vec3(0, eyeHeight, ipd * 4.0f),
        glm::vec3(0, eyeHeight, 0),
        GlUtils::Y_AXIS));

    gl::Stacks::projection().top() = glm::perspective(
        PI / 2.0f, EYE_ASPECT, 0.01f, 1000.0f);

    eyes[ovrEye_Left].viewportPosition = glm::uvec2(0, 0);
    eyes[ovrEye_Left].modelviewOffset = glm::translate(glm::mat4(),
        glm::vec3(ipd / 2.0f, 0, 0));

    eyes[ovrEye_Right].viewportPosition = glm::uvec2(WINDOW_SIZE.x / 2, 0);
    eyes[ovrEye_Right].modelviewOffset = glm::translate(glm::mat4(),
        glm::vec3(-ipd / 2.0f, 0, 0));
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
    if (CameraControl::instance().onKey(player, key, scancode, action, mods)) {
      return;
    }

    if (action == GLFW_PRESS) switch (key) {
    case GLFW_KEY_R:
      player = glm::inverse(glm::lookAt(
          glm::vec3(0, eyeHeight, ipd * 4.0f),
          glm::vec3(0, eyeHeight, 0),
          GlUtils::Y_AXIS));
      return;

    case GLFW_KEY_M:
      applyModelviewOffset = !applyModelviewOffset;
      return;
    }

    GlfwApp::onKey(key, scancode, action, mods);
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::MatrixStack & mv = gl::Stacks::modelview();

    for_each_eye([&](ovrEyeType eye) {
      const PerEyeArg & eyeArgs = eyes[eye];
      gl::viewport(eyeArgs.viewportPosition, EYE_SIZE);

      gl::MatrixStack & mv = gl::Stacks::modelview();
      gl::Stacks::with_push(mv, [&]{
        if (applyModelviewOffset) {
          mv.preMultiply(eyeArgs.modelviewOffset);
        }
        renderScene();
      });
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
