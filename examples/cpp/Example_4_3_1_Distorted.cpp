#include "Common.h"

Resource SCENE_IMAGES_DK1[2] = {
  Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_DK1_PNG,
  Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_DK1_PNG
};

Resource SCENE_IMAGES_DK2[2] = {
  Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_DK2_PNG,
  Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_DK2_PNG
};

class DistortedExample : public RiftGlfwApp {
protected:
  TexturePtr sceneTextures[2];
  ovrTexture eyeTextures[2];

public:
  
  void initGl() {
    RiftGlfwApp::initGl();

    Resource * sceneImages = SCENE_IMAGES_DK2;
    if (hmd->Type == ovrHmd_DK1) {
      sceneImages = SCENE_IMAGES_DK1;
    }

    for_each_eye([&](ovrEyeType eye){
      glm::uvec2 textureSize;
      sceneTextures[eye] = oria::load2dTexture(sceneImages[eye], textureSize);

      memset(eyeTextures + eye, 0, sizeof(eyeTextures[eye]));
      ovrTextureHeader & eyeTextureHeader = eyeTextures[eye].Header;
      eyeTextureHeader.TextureSize = ovr::fromGlm(textureSize);
      eyeTextureHeader.RenderViewport.Size = eyeTextureHeader.TextureSize;
      eyeTextureHeader.API = ovrRenderAPI_OpenGL;

      ((ovrGLTextureData&)eyeTextures[eye]).TexId = oglplus::GetName(*sceneTextures[eye]);
    });

    ovrRenderAPIConfig config;
    memset(&config, 0, sizeof(config));
    config.Header.API = ovrRenderAPI_OpenGL;
    config.Header.BackBufferSize = ovr::fromGlm(getSize());
    config.Header.Multisample = 1;
#if defined(OVR_OS_WIN32)
    ((ovrGLConfigData&)config).Window = 0;
#elif defined(OVR_OS_LINUX)
    ((ovrGLConfigData&)config).Disp = 0;
#endif

    int distortionCaps = ovrDistortionCap_Vignette;

    ovrEyeRenderDesc eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(hmd, &config,
        distortionCaps, hmd->DefaultEyeFov, eyeRenderDescs);
    if (0 == configResult) {
      FAIL("Unable to configure rendering");
    }
  }

  virtual void finishFrame() {
  }

  void draw() {
    static ovrPosef eyePoses[2];
    ovrHmd_BeginFrame(hmd, getFrame());
    ovrHmd_EndFrame(hmd, eyePoses, eyeTextures);
  }
};

RUN_OVR_APP(DistortedExample)
