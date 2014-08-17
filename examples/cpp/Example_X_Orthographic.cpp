#include "Common.h"

class SimpleScene : public RiftApp {
  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_PLAYER_HEIGHT };
  float scaleFactor = 1;


public:
  SimpleScene() {
    ipd = ovrHmd_GetFloat(hmd, OVR_KEY_IPD, OVR_DEFAULT_IPD);
    eyeHeight = ovrHmd_GetFloat(hmd, OVR_KEY_PLAYER_HEIGHT, OVR_DEFAULT_PLAYER_HEIGHT);
    resetCamera();
  }

  ~SimpleScene() {
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    if (!CameraControl::instance().onKey(key, scancode, action, mods)) {
      static const float GROW = 10;
      static const float SHRINK = 1.0f / GROW;
      if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_HOME:
          scaleFactor *= GROW;
          break;
        case GLFW_KEY_END:
          scaleFactor *= SHRINK;
          break;
        case GLFW_KEY_R:
          resetCamera();
          break;
        }
      }
      else {
        RiftGlfwApp::onKey(key, scancode, action, mods);
      }
    }
  }

  void resetCamera() {
    player = glm::inverse(glm::lookAt(
      glm::vec3(0, eyeHeight, 1),  // Position of the camera
      glm::vec3(0, eyeHeight, 0),  // Where the camera is looking
      GlUtils::Y_AXIS));           // Camera up axis
  }

  glm::mat4 getOrthographic() {
    const ovrEyeRenderDesc & erd = getEyeRenderDesc();
    ovrMatrix4f ovrPerspectiveProjection = ovrMatrix4f_Projection(erd.Fov, 0.01f, 100000.0f, true);
    ovrVector2f scale; scale.x = scaleFactor; scale.y = scaleFactor;
    return Rift::fromOvr(ovrMatrix4f_OrthoSubProjection(ovrPerspectiveProjection, scale, 100.8f, erd.ViewAdjust.x));
  }

#define QUAD_SIZE (1)

  void renderScene() {
    int currentEye = getCurrentEye();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::MatrixStack & pr = gl::Stacks::projection();
    glUseProgram(0);
    gl::Stacks::withPush([&]{
      pr.top() = getOrthographic();
      pr.top()[1][1] = -pr.top()[1][1];
      //float tr = pr.top()[3].x;
      //pr.identity().translate(tr);

      //glDisable(GL_CULL_FACE);
//      glMatrixMode(GL_PROJECTION);
//      glLoadMatrixf(glm::value_ptr(pr.top()));

//      glMatrixMode(GL_MODELVIEW);
      mv.identity();
//      glLoadMatrixf(glm::value_ptr(mv.top()));
      glm::vec2 cursor = glm::vec2(-0.5, -0.5);
      static Text::FontPtr font =
        GlUtils::getFont(Resource::FONTS_INCONSOLATA_MEDIUM_SDFF);
      font->renderString("World", cursor );
      mv.scale(0.5f);
      cursor = glm::vec2(0);
      GlUtils::drawQuad(glm::vec2(-QUAD_SIZE), glm::vec2(QUAD_SIZE));
      GlUtils::getFont(Resource::FONTS_INCONSOLATA_MEDIUM_SDFF)->
        renderString("Hello", cursor , 12, 100);

    });

    GlUtils::drawAngleTicks();
    /*
    mv.withPush([&]{
      mv.postMultiply(glm::inverse(player));
      GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
      GlUtils::renderFloor();
      gl::Stacks::with_push(mv, [&]{
        mv.translate(glm::vec3(0, eyeHeight, 0)).scale(ipd);
        GlUtils::drawColorCube(true);
      });
    });
    */
  }
};

RUN_OVR_APP(SimpleScene);
