#include "Common.h"

Resource SCENE_IMAGES[2] = {
  Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_PNG,
  Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_PNG
};

class DistortedExample : public RiftGlfwApp {
protected:
  gl::Texture2dPtr sceneTextures[2];
  ovrTexture eyeTextures[2];
  ovrEyeRenderDesc eyeRenderDescs[2];

public:
  virtual ~DistortedExample() {
  }

  void initGl() {
    RiftGlfwApp::initGl();
    glDisable(GL_BLEND);

    for_each_eye([&](ovrEyeType eye){
      glm::uvec2 textureSize;
      GlUtils::getImageAsTexture(sceneTextures[eye],
        SCENE_IMAGES[eye], textureSize);

      memset(eyeTextures + eye, 0,
        sizeof(eyeTextures[eye]));

      ovrTextureHeader & eyeTextureHeader =
        eyeTextures[eye].Header;

      eyeTextureHeader.TextureSize = Rift::toOvr(textureSize);
      eyeTextureHeader.RenderViewport.Size =
        eyeTextureHeader.TextureSize;

      eyeTextureHeader.API = ovrRenderAPI_OpenGL;

      ((ovrGLTextureData&)eyeTextures[eye]).TexId =
        sceneTextures[eye]->texture;
    });

    ovrRenderAPIConfig config;
    //ovrRenderAPIConfig cfg;
    memset(&config, 0, sizeof(config));
    config.Header.API = ovrRenderAPI_OpenGL;
    config.Header.RTSize = Rift::toOvr(windowSize);
    config.Header.Multisample = 1;
#if defined(OVR_OS_WIN32)
    ((ovrGLConfigData&)config).Window = 0;
#elif defined(OVR_OS_LINUX)
    ((ovrGLConfigData&)config).Win = 0;
    ((ovrGLConfigData&)config).Disp = 0;
#endif

    int distortionCaps = 
      ovrDistortionCap_Vignette
      | ovrDistortionCap_Chromatic
      | ovrDistortionCap_TimeWarp;

    int configResult = ovrHmd_ConfigureRendering(hmd, &config,
      distortionCaps, hmd->DefaultEyeFov, eyeRenderDescs);
    if (0 == configResult) {
      FAIL("Unable to configure rendering");
    }
  }


  void onKey(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
      static ovrHSWDisplayState hswDisplayState;
      ovrHmd_GetHSWDisplayState(hmd, &hswDisplayState);
      if (hswDisplayState.Displayed) {
        ovrHmd_DismissHSWDisplay(hmd);
        return;
      }
    }
    RiftGlfwApp::onKey(key, scancode, action, mods);
  }

  virtual void finishFrame() {
  }

  void draw() {
    static int frameIndex = 0;
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ovrHmd_BeginFrame(hmd, frameIndex++);
    static ovrPosef poses[2];
    for (int i = 0; i < 2; ++i) {
      ovrEyeType eye = hmd->EyeRenderOrder[i];
      poses[eye] = ovrHmd_GetEyePose(hmd, eye);
    }
    ovrHmd_EndFrame(hmd, poses, eyeTextures);
  }
};

RUN_OVR_APP(DistortedExample)
