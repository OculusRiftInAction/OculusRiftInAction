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
    if (!CameraControl::instance().onKey(player, key, scancode, action, mods)) {
      if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_HOME:
          if (texRes < 0.95f) {
            texRes *= 1.1f;
          }
          break;
        case GLFW_KEY_END:
          if (texRes > 0.05f) {
            texRes *= (1.0f / 1.1f);
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
    ovrGLTexture & eyeTex = eyeTextures[currentEye];
    const ovrSizei & texSize = eyeTex.Texture.Header.TextureSize;
    ovrSizei & renderSize = eyeTex.Texture.Header.RenderViewport.Size;
    renderSize.w = texSize.w * texRes;
    renderSize.h = texSize.h * texRes;
    glViewport(
      0, 
      texSize.h - renderSize.h,
      renderSize.w, renderSize.h);

    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.withPush([&]{
      mv.postMultiply(glm::inverse(player));
      GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
      GlUtils::renderFloorGrid(player);
      gl::Stacks::with_push(mv, [&]{
        mv.translate(glm::vec3(0, eyeHeight, 0)).scale(ipd);
        GlUtils::drawColorCube(true);
      });
    });
  }
};

RUN_OVR_APP(SimpleScene);
