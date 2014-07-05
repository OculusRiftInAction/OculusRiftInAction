#include "Common.h"
#include "OVR_CAPI_GL.h"

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
    ovrHmd_Destroy(hmd);
  }

  void initGl() {
    RiftGlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 1.1f, 0.1f, 1.0f);

    ovrFovPort eyeFovPorts[2];
    for_each_eye([&](ovrEyeType eye){
      ovrTextureHeader & eyeTextureHeader = eyeTextures[eye].Header;
      memset(eyeTextures + eye, 0, sizeof(eyeTextures[eye]));
          eyeFovPorts[eye] = hmdDesc.DefaultEyeFov[eye];
      eyeTextureHeader.TextureSize =
          ovrHmd_GetFovTextureSize(hmd, eye, hmdDesc.DefaultEyeFov[eye], 1.0f);
      eyeTextureHeader.RenderViewport.Size = eyeTextureHeader.TextureSize;
      eyeTextureHeader.RenderViewport.Pos.x = 0;
      eyeTextureHeader.RenderViewport.Pos.y = 0;
      eyeTextureHeader.API = ovrRenderAPI_OpenGL;
      sceneTextures[eye] =
          GlUtils::getImageAsTexture(SCENE_IMAGES[eye]);
      ((ovrGLTextureData&)eyeTextures[eye]).TexId =
          sceneTextures[eye]->texture;
    });

    ovrRenderAPIConfig cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.Header.API = ovrRenderAPI_OpenGL;
    cfg.Header.RTSize = hmdDesc.Resolution;
    cfg.Header.Multisample = 1;

    int distortionCaps = ovrDistortionCap_Chromatic
        | ovrDistortionCap_TimeWarp
        | ovrDistortionCap_NoSwapBuffers;
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg,
      distortionCaps, eyeFovPorts, eyeRenderDescs);
    if (0 == configResult) {
      FAIL("Unable to configure rendering");
    }
  }

  void draw() {
    static int frameIndex = 0;
    ovrHmd_BeginFrame(hmd, frameIndex++);
    glClear(GL_COLOR_BUFFER_BIT);
    for_each_eye([&](ovrEyeType eye) {
      ovrPosef renderPose = ovrHmd_BeginEyeRender(hmd, eye);
      ovrHmd_EndEyeRender(hmd, eye, renderPose, &eyeTextures[eye]);
    });
    ovrHmd_EndFrame(hmd);
  }
};

RUN_OVR_APP(DistortedExample)
