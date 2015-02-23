#include "Common.h"

class DynamicFramebufferScaleExample : public RiftApp {
  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_PLAYER_HEIGHT };
  float texRes{ 1.0f };

public:
  DynamicFramebufferScaleExample() {
    ipd = ovrHmd_GetFloat(hmd, 
      OVR_KEY_IPD, 
      OVR_DEFAULT_IPD);

    eyeHeight = ovrHmd_GetFloat(hmd, 
      OVR_KEY_PLAYER_HEIGHT, 
      OVR_DEFAULT_PLAYER_HEIGHT);

    resetCamera();
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
//    if (CameraControl::instance().onKey(key, scancode, action, mods)) {
//      return;
//    }
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
      RiftApp::onKey(key, scancode, action, mods);
  }

  void resetCamera() {
    player = glm::inverse(glm::lookAt(
      glm::vec3(0, eyeHeight, 0.4),  // Position of the camera
      glm::vec3(0, eyeHeight, 0),  // Where the camera is looking
      Vectors::Y_AXIS));           // Camera up axis
    ovrHmd_RecenterPose(hmd);
  }


  void renderScene() {
    int currentEye = getCurrentEye();
    ovrTexture & eyeTex = eyeTextures[currentEye];
    ovrRecti & rvp = eyeTex.Header.RenderViewport;
    const ovrSizei & texSize = eyeTex.Header.TextureSize;
    rvp.Size.w = texSize.w * texRes;
    rvp.Size.h = texSize.h * texRes;
    glViewport(
      rvp.Pos.x, rvp.Pos.y,
      rvp.Size.w, rvp.Size.h);

    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    MatrixStack & mv = Stacks::modelview();
    mv.withPush([&]{
      mv.postMultiply(glm::inverse(player));
      oria::renderManikinScene(ipd, eyeHeight);
    });

    std::string message = Platform::format(
      "Texture Scale %0.2f\nMegapixels per eye: %0.2f", texRes, 
      (rvp.Size.w * rvp.Size.h) / 1000000.0f);
    GlfwApp::renderStringAt(message, glm::vec2(-0.5f, 0.5f));
  }
};

RUN_OVR_APP(DynamicFramebufferScaleExample);
