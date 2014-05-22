#include "Common.h"
#include "OVR_CAPI_GL.h"

Resource SCENE_IMAGES[2] = {
  Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_PNG,
  Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_PNG
};


class ShaderLookupDistortionExample : public RiftGlfwApp {
protected:
  gl::Texture2dPtr sceneTextures[2];
  ovrTexture eyeTextures[2];
  ovrEyeRenderDesc eyeRenderDescs[2];

public:

  void initGl() {
    RiftGlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 1.1f, 0.1f, 1.0f);

    ovrEyeDesc eyeDescs[2];
    for_each_eye([&](ovrEyeType eye){
      ovrEyeDesc & eyeDesc = eyeDescs[eye];
      eyeDesc.Eye = eye;
      eyeDesc.Fov = hmdDesc.DefaultEyeFov[eye];
      eyeDesc.TextureSize = ovrHmd_GetFovTextureSize(hmd, eye, hmdDesc.DefaultEyeFov[eye], 1.0f);
      eyeDesc.RenderViewport.Size = eyeDesc.TextureSize;
      eyeDesc.RenderViewport.Pos.x = 0;
      eyeDesc.RenderViewport.Pos.y = 0;
      sceneTextures[eye] = GlUtils::getImageAsTexture(SCENE_IMAGES[eye]);

      ovrTextureHeader & eyeTextureHeader = eyeTextures[eye].Header;
      eyeTextureHeader.API = ovrRenderAPI_OpenGL;
      eyeTextureHeader.TextureSize = eyeDesc.TextureSize;
      eyeTextureHeader.RenderViewport = eyeDesc.RenderViewport;
      ovrGLTexture * glTexture = (ovrGLTexture *)(&eyeTextures[eye]);
      glTexture->OGL.TexId = sceneTextures[eye]->texture;
    });

    ovrGLConfig cfg;
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.RTSize = hmdDesc.Resolution;
    cfg.OGL.Header.Multisample = 1;

    int distortionCaps = ovrDistortion_Chromatic | ovrDistortion_TimeWarp;
    int renderCaps = 0;

    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg.Config,
      renderCaps, distortionCaps, eyeDescs, eyeRenderDescs);
  }

  void draw() {
    static int frameIndex = 0;
    ovrHmd_BeginFrame(hmd, frameIndex++);
    glClear(GL_COLOR_BUFFER_BIT);
    for_each_eye([&](ovrEyeType eye) {
      ovrPosef renderPose = ovrHmd_BeginEyeRender(hmd, eye);
      ovrHmd_EndEyeRender(hmd, eye, renderPose, &eyeTextures[eye]);
    });
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    ovrHmd_EndFrame(hmd);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
  }
};

RUN_OVR_APP(ShaderLookupDistortionExample)
