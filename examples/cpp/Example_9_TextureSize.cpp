#include "Common.h"

class SimpleScene: public RiftApp {
  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_PLAYER_HEIGHT };
  float texRes{ 1.0f };

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
      static const float ROOT_2 = sqrt(2.0f);
      static const float INV_ROOT_2 = 1.0f / ROOT_2;
      if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_HOME:
          if (texRes < 0.95f) {
            texRes *= ROOT_2;
          }
          break;
        case GLFW_KEY_END:
          if (texRes > 0.05f) {
            texRes *= INV_ROOT_2;
          }
          break;
        case GLFW_KEY_R:
          resetCamera();
          break;
        }
      } else {
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


  void renderScene() {
    int currentEye = getCurrentEye();
    ovrTexture & eyeTex = eyeTextures[currentEye];
    const ovrSizei & texSize = eyeTex.Header.TextureSize;
    ovrRecti & rvp = eyeTex.Header.RenderViewport;
    rvp.Size.w = texSize.w * texRes;
    rvp.Size.h = texSize.h * texRes;
    glViewport(
      rvp.Pos.x, rvp.Pos.y,
      rvp.Size.w, rvp.Size.h);

    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.withPush([&]{
      mv.postMultiply(glm::inverse(player));
      GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
      GlUtils::renderFloor();
      gl::Stacks::with_push(mv, [&]{
        mv.translate(glm::vec3(0, eyeHeight, 0)).scale(ipd);
        GlUtils::drawColorCube(true);
      });
    });
  }
};

RUN_OVR_APP(SimpleScene);
