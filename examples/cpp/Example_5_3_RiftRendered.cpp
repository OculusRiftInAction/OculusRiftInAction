#include "Common.h"
#include "CubeScene.h"

struct PerEyeArg {
  glm::mat4                     modelviewOffset;
  glm::mat4                     projection;
  gl::FrameBufferWrapper        frameBuffer;
};

class CubeScene_Rift: public CubeScene {
  PerEyeArg eyes[2];
  int frameIndex{ 0 };
  ovrTexture textures[2];

public:
  CubeScene_Rift() {
  }

  virtual void initGl() {
    CubeScene::initGl();

    ovrRenderAPIConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.Header.API = ovrRenderAPI_OpenGL;
    cfg.Header.RTSize = hmd->Resolution;
    cfg.Header.Multisample = 1;

    int distortionCaps = 
      ovrDistortionCap_Chromatic |
      ovrDistortionCap_Vignette;
    ovrEyeRenderDesc eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg,
        distortionCaps, hmd->DefaultEyeFov, eyeRenderDescs);

    for_each_eye([&](ovrEyeType eye){
      PerEyeArg & eyeArg = eyes[eye];
      ovrFovPort fov = hmd->DefaultEyeFov[eye];
      ovrTextureHeader & textureHeader = textures[eye].Header;
      ovrSizei texSize = ovrHmd_GetFovTextureSize(hmd, eye, fov, 1.0f);
      eyeArg.frameBuffer.init(Rift::fromOvr(texSize));

      textureHeader.API = ovrRenderAPI_OpenGL;
      textureHeader.TextureSize = texSize;
      textureHeader.RenderViewport.Size = texSize;
      textureHeader.RenderViewport.Pos.x = 0;
      textureHeader.RenderViewport.Pos.y = 0;
      ((ovrGLTexture&)textures[eye]).OGL.TexId = eyeArg.frameBuffer.color->texture;

      ovrVector3f offset = eyeRenderDescs[eye].ViewAdjust;
      ovrMatrix4f projection = ovrMatrix4f_Projection(fov, 0.01f, 100, true);

      eyeArg.projection = Rift::fromOvr(projection);
      eyeArg.modelviewOffset = glm::translate(glm::mat4(), Rift::fromOvr(offset));
    });
  }

  virtual void finishFrame() {
  }

  virtual void draw() {
    ovrHmd_BeginFrame(hmd, frameIndex++);
    static ovrPosef eyePoses[2];
    gl::MatrixStack & mv = gl::Stacks::modelview();
    for (int i = 0; i < ovrEye_Count; ++i) {
      ovrEyeType eye = hmd->EyeRenderOrder[i];
      PerEyeArg & eyeArgs = eyes[eye];
      gl::Stacks::projection().top() = eyeArgs.projection;

      eyeArgs.frameBuffer.withFramebufferActive([&]{
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl::Stacks::with_push(mv, [&]{
          mv.preMultiply(eyeArgs.modelviewOffset);
          drawCubeScene();
        });
      });

    }

    ovrHmd_EndFrame(hmd, eyePoses, textures);
  }
};

RUN_OVR_APP(CubeScene_Rift);
