#include "Common.h"

static const glm::mat4 STARTING_POINT = glm::inverse(glm::lookAt(
  glm::vec3(0, 0, 2),
  glm::vec3(0, 0, 0),
  glm::vec3(0, 1, 0)));

class LatencyTesterExample : public RiftApp {
  OVR::Ptr<OVR::LatencyTestDevice> ovrLatencyTester;
  OVR::Util::LatencyTest ovrLatencyTest;
  gl::GeometryPtr latencyTestQuad;
  gl::ProgramPtr latencyTestProgram;

public:
  LatencyTesterExample() {
    ovrLatencyTester = *ovrManager->EnumerateDevices<OVR::LatencyTestDevice>().CreateDevice();
    if (ovrLatencyTester) {
      ovrLatencyTest.SetDevice(ovrLatencyTester);
    }
  }

  virtual ~LatencyTesterExample() {
  }

  void initGl() {
    RiftApp::initGl();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    gl::clearColor(Colors::darkGrey);
    latencyTestQuad = GlUtils::getQuadGeometry();
    latencyTestProgram = GlUtils::getProgram(
      Resource::SHADERS_SIMPLE_VS, 
      Resource::SHADERS_COLORED_FS);
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) switch (key) {
    // This lets you begin a latency test from the keyboard, 
    // just the same as if you pressed the button on the Rift.  
    // Say for instance if you'd slept wrong the previous night 
    // and had an annoying crick in your neck, and so didn't 
    // want to keep leaning over to where the Rift was sitting 
    // on a shelf.  Seriously... it hurts...  I'm putting ice 
    // on it and everything (my neck, not the Rift).
    case GLFW_KEY_T:
      ovrLatencyTest.BeginTest();
      return;
    }
    
    RiftGlfwApp::onKey(key, scancode, action, mods);
  }

  virtual void update() {
    RiftApp::update();
    // Bypass head tracking, set the viewpoint explicitly based on our needs
    gl::Stacks::modelview().top() = glm::inverse(STARTING_POINT);
    ovrLatencyTest.ProcessInputs();
  }

  void renderScene() {
    GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    GlUtils::renderFloorGrid(player);
    // Draws an animated structure of 1 + 2^N cubes, without using 
    // instancing.  

    // 16 cubes
    GlUtils::cubeRecurse(4);

    // 4096 cubes
    //GlUtils::cubeRecurse(12);

    // Outputs the image required by the latency tester.  Does nothing
    // if the test has not been triggered;
    renderLatencyTestSquare();
  }

  void renderLatencyTestSquare() {
    OVR::Color colorToDisplay;

    // If this returns a non-0 value, we're performing a latency
    // test and must render a colored square underneath the tester 
    // (On the lens axis)
    if (ovrLatencyTest.DisplayScreenColor(colorToDisplay)) {
      latencyTestProgram->use();
      latencyTestProgram->setUniform("Color", Rift::fromOvr(colorToDisplay));

      gl::MatrixStack & mv = gl::Stacks::modelview();
      glDisable(GL_DEPTH_TEST);
      mv.with_push([&]{
        mv.identity().translate(glm::vec3(0, 0, -0.1f)).scale(0.005f);
        GlUtils::renderGeometry(latencyTestQuad, latencyTestProgram);
      });
      glEnable(GL_DEPTH_TEST);
    }
  }

};

RUN_OVR_APP(LatencyTesterExample);