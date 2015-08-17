#include "Common.h"

class CubeScene_Rift: public RiftGlfwApp {

  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_EYE_HEIGHT };

public:
  CubeScene_Rift() {
    eyeHeight = ovr_GetFloat(hmd, OVR_KEY_EYE_HEIGHT, eyeHeight);
    ipd = ovr_GetFloat(hmd, OVR_KEY_IPD, ipd);

    Stacks::modelview().top() = glm::lookAt(
      vec3(0, eyeHeight, 0.5f),
      vec3(0, eyeHeight, 0),
      Vectors::UP);
  }

  virtual void perEyeRender() {
    oglplus::Context::Clear().DepthBuffer();
    oria::renderExampleScene(ipd, eyeHeight);
  }
};

RUN_OVR_APP(CubeScene_Rift);
