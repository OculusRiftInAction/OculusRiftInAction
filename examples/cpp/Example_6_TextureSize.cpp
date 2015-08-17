#include "Common.h"

class DynamicFramebufferScaleExample : public RiftGlfwApp {
  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_PLAYER_HEIGHT };
  float texRes{ 1.0f };
  mat4 player;

public:
  DynamicFramebufferScaleExample() {
    ipd = ovr_GetFloat(hmd, OVR_KEY_IPD, ipd);
    eyeHeight = ovr_GetFloat(hmd, OVR_KEY_PLAYER_HEIGHT, eyeHeight);
    resetCamera();
  }

  virtual void onKey(int key, int scancode, int action, int mods) override {
      static const float ROOT_2 = sqrt(2.0f);
      static const float INV_ROOT_2 = 1.0f / ROOT_2;
      if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_HOME:
          if (texRes < 0.95f) {
            texRes = std::min(texRes * ROOT_2, 1.0f);
          }
          return;

        case GLFW_KEY_END:
          if (texRes > 0.05f) {
            texRes *= INV_ROOT_2;
          }
          return;

        case GLFW_KEY_R:
          resetCamera();
          return;
        }
      } 
      RiftGlfwApp::onKey(key, scancode, action, mods);
  }

  void resetCamera() {
    player = glm::inverse(glm::lookAt(
      glm::vec3(0, eyeHeight, 0.4),  // Position of the camera
      glm::vec3(0, eyeHeight, 0),  // Where the camera is looking
      Vectors::Y_AXIS));           // Camera up axis
    Stacks::modelview().top() = glm::inverse(player);
    ovr_RecenterPose(hmd);
  }


  virtual void perEyeRender() override {
    int currentEye = getCurrentEye();
    ovrLayerEyeFov & layer = layers[0].EyeFov;
    auto swapTextures = layer.ColorTexture[currentEye];
    ovrTexture & eyeTex = swapTextures->Textures[swapTextures->CurrentIndex];
    ovrRecti & rvp = layer.Viewport[currentEye];
    const ovrSizei & texSize = eyeTex.Header.TextureSize;
    rvp.Size.w = texSize.w * texRes;
    rvp.Size.h = texSize.h * texRes;
    // The fuck?  Bug from 0.4 re-appears.  reported here https://forums.oculus.com/viewtopic.php?f=69&t=25648
    glViewport(
      rvp.Pos.x, texSize.h - rvp.Size.h,
      rvp.Size.w, rvp.Size.h);
    oglplus::Context::Clear().ColorBuffer().DepthBuffer();
    oria::renderManikinScene(ipd, eyeHeight);
    std::string message = Platform::format(
      "Texture Scale %0.2f\nMegapixels per eye: %0.2f", texRes, 
      (rvp.Size.w * rvp.Size.h) / 1000000.0f);
    GlfwApp::renderStringAt(message, glm::vec2(-0.5f, 0.5f));
  }
};

RUN_OVR_APP(DynamicFramebufferScaleExample);
