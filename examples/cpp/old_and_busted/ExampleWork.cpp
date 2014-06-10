#include "Common.h"
//#include "pxccapture.h"
//#include "pxcsmartptr.h"

class Example05 : public GlfwApp{
protected:
  glm::mat4 player;
  gl::GeometryPtr headGeometry;
  float playerHeight = 1.2f;

public:

  Example05() {
    player = glm::inverse(glm::lookAt(glm::vec3(0.75, playerHeight, -0.5),
        GlUtils::ORIGIN, GlUtils::UP));
  }

  void initGl() {
    GlfwApp::initGl();
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_CHECK_ERROR;

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    GL_CHECK_ERROR;
    headGeometry = GlUtils::getMesh(Resource::MESHES_HEAD_CTM).getGeometry();
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    if (CameraControl::instance().onKey(player, key, scancode, action, mods)) {
      return;
    }
  }

  virtual void update() {
    CameraControl::instance().applyInteraction(player);

    // The rift orientation impacts the view, but does not alter the camera
    // orientation itself.  Movement is independent of which direction
    // you're looking, only depends on the camera orientation
    riftOrientation = Rift::getQuaternion(sensorFusion);

    gl::Stacks::modelview().top() = glm::mat4_cast(riftOrientation) * glm::inverse(player);
  }

  virtual void renderScene(const PerEyeArg & eyeArgs) {
    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::MatrixStack & pr = gl::Stacks::projection();
    pr.push().top() = eyeArgs.getProjection();
    mv.push().preMultiply(eyeArgs.modelviewOffset).preMultiply(eyeArgs.strabsimusCorrection);

    glEnable(GL_DEPTH_TEST);
    gl::clearColor(glm::vec3(0.05f, 0.05f, 0.05f));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //GlUtils::draw3dGrid();
    GlUtils::drawAngleTicks();
    mv.rotate(-HALF_PI, GlUtils::X_AXIS).scale(1.0);
    //GlUtils::drawColorCube();
//    glPointSize(2.0);
    gl::ProgramPtr program = GlUtils::getProgram(
        Resource::SHADERS_LIT_VS,
        Resource::SHADERS_LITCOLORED_FS);
    program->use();
    program->setUniform("ForceAlpha", 0.4f);
//    glCullFace(GL_FRONT);
//    glPolygonMode(GL_BACK, GL_LINE);
//    glCullFace(GL_BACK);
//    glPolygonMode(GL_BACK, GL_LINE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    GlUtils::renderGeometry(treeGeometry, program);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    GlUtils::draw3dVector(glm::vec3(0, 1, 0), Colors::red);
    mv.pop();
    pr.pop();
  }

};

RUN_OVR_APP(Example05);
