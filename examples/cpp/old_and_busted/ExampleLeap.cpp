#include <Leap.h>
#include "Common.h"

static glm::vec3 toGlm(const Leap::Vector & v) {
  return glm::vec3(v.x, v.y, v.z) / 1000.0f;
}

static glm::vec3 toRiftFrame(const glm::vec3 & v) {
  glm::vec4 relative = glm::vec4(v.z, -v.x, -v.y, 1);
  return glm::vec3(relative.x, relative.y, relative.z);
}

class Example05 : public RiftApp, Leap::Listener {
  gl::TextureCubeMapPtr skyboxTexture;
  glm::mat4 player;
  Leap::Controller controller;
  Leap::Frame frame;

  virtual void onFrame(const Leap::Controller& c) {
    frame = c.frame();
  }

  void initGl() {
    RiftApp::initGl();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    gl::clearColor(glm::vec3(0.05f, 0.05f, 0.05f));
    player = glm::inverse(
        glm::lookAt(glm::vec3(0, 0.4f, 1), glm::vec3(0, 0.4f, 0),
            GlUtils::UP));
    skyboxTexture = GlUtils::getCubemapTextures(
        Resource::IMAGES_SKY_CITY_XNEG_PNG);
    controller.addListener(*this);
  }
  
  virtual void update() {
    CameraControl::instance().applyInteraction(player);

    gl::Stacks::modelview().top() = glm::mat4_cast(
        Rift::getQuaternion(sensorFusion)) *
        glm::inverse(player);
    // gl::Stacks::modelview().top() = glm::inverse(player);
  }

  glm::vec2 quantize(const glm::vec2 & t, float scale) {
    float width = scale * 2.0f;
    glm::vec2 t2 = t + scale;
    glm::vec2 t3 = glm::floor(t2 / width) * width;
    return t3;
  }

  void scaleRenderGrid(float scale, const glm::vec2 & p) {
    glm::vec2 p3 = quantize(p, scale);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.push().translate(glm::vec3(p3.x - scale, 0, p3.y - scale)).scale(
        scale);
    GlUtils::draw3dGrid();
    mv.pop();

    mv.push().translate(glm::vec3(p3.x - scale, 0, p3.y + scale)).scale(
        scale);
    GlUtils::draw3dGrid();
    mv.pop();

    mv.push().translate(glm::vec3(p3.x + scale, 0, p3.y - scale)).scale(
        scale);
    GlUtils::draw3dGrid();
    mv.pop();

    mv.push().translate(glm::vec3(p3.x + scale, 0, p3.y + scale)).scale(
        scale);
    GlUtils::draw3dGrid();
    mv.pop();
  }

  virtual void renderScene() {
    GL_CHECK_ERROR;
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);

    glm::vec2 position = glm::vec2(player[3].x, player[3].z);
    scaleRenderGrid(1.0, position);
    scaleRenderGrid(10.0, position);
    scaleRenderGrid(100.0, position);

    gl::MatrixStack & mv = gl::Stacks::modelview();
    for (int i = 0; i < frame.hands().count(); ++i) {
      Leap::Hand hand = frame.hands()[i];
      if (hand.fingers().count() >= 4) {
        renderHand(hand);
      }
    }

    glDisable(GL_DEPTH_TEST);
    std::string message = Platform::format(
        "Frame valid %d\n"
            "Hands %d\n"
            "Fingers %d\n"
            "Gestures %d\n",
        frame.isValid() ? 1 : 0,
        frame.hands().count(),
        frame.fingers().count(),
        frame.gestures().count()
        );
    renderStringAt(message, -0.5, 0.5);
  }


  void renderHand(const Leap::Hand & hand) {
    gl::MatrixStack & mv = gl::Stacks::modelview();

    glm::vec3 hp = toRiftFrame(toGlm(hand.palmPosition()));

    std::string message = Platform::format(
        "Position \n%f\n%f\n%f\n",
        hp.x, hp.y, hp.z
        );
    renderStringAt(message, 0.2f, -0.4f);

    mv.push().identity().translate(glm::vec3(0.02f, 0.0f, -0.06f));
    {
      mv.push().translate(hp).scale(0.02f);
      GlUtils::drawColorCube();
      mv.pop();

      for (int i = 0; i < hand.fingers().count(); ++i) {
        Leap::Finger finger = hand.fingers()[i];
        mv.push().translate(toRiftFrame(toGlm(finger.tipPosition()))).scale(0.01f);
        GlUtils::drawColorCube();
        mv.pop();
      }
    }
    mv.pop();
  }
};

RUN_OVR_APP(Example05);
