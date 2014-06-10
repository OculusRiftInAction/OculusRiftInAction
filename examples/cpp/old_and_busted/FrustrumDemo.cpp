#include "Common.h"

static const glm::uvec2 WINDOW_SIZE(1280, 800);
static const glm::ivec2 WINDOW_POS(100, 100);

class SimpleScene: public GlfwApp {
  glm::mat4 player;
  float ipd{ 0.06f };
  float eyeHeight{ 0.09f };
  
public:
  SimpleScene() {
    OVR::Ptr<OVR::ProfileManager> profileManager = 
      *OVR::ProfileManager::Create();
    OVR::Ptr<OVR::Profile> profile = 
      *(profileManager->GetDeviceDefaultProfile(
        OVR::ProfileType::Profile_RiftDK1));
    ipd = profile->GetIPD();

    player = glm::inverse(glm::lookAt(
      glm::vec3(0, eyeHeight, ipd * 4.0f),
      glm::vec3(0, eyeHeight, 0),
      GlUtils::Y_AXIS));
    gl::Stacks::projection().top() = 
      glm::perspective(PI / 3.0f, 
      glm::aspect(WINDOW_SIZE), 
      0.01f, 1000.0f);
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
    gl::Stacks::modelview().top() =
      glm::lookAt(glm::vec3(-1, eyeHeight * 2, 1), glm::vec3(0, eyeHeight, 0), GlUtils::UP);
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
    }

    GlfwApp::onKey(key, scancode, action, mods);
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
    static gl::GeometryPtr head = GlUtils::getMesh(Resource::MESHES_HEAD_CTM).getGeometry();
    static gl::ProgramPtr program = GlUtils::getProgram(Resource::SHADERS_LIT_VS, Resource::SHADERS_LITCOLORED_FS);
    mv.with_push([&]{
      mv.with_push([&]{
        mv.translate(glm::vec3(0, eyeHeight, ipd * 6.0f));
        mv.rotate(PI, GlUtils::Y_AXIS);
        mv.rotate(-PI / 2.0f, GlUtils::X_AXIS);
        GlUtils::renderGeometry(head, program);
      });
//      GlUtils::renderFrustum();
      mv.translate(glm::vec3(0, eyeHeight, 0)).scale(ipd);
      GlUtils::drawColorCube(true);
    });
  }
};

RUN_OVR_APP(SimpleScene);
