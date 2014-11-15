#include "Common.h"


class CubeScene_Rift: public RiftGlfwApp {
  FramebufferWrapper  eyeFramebuffers[2];
  ovrTexture eyeTextures[2];
  ovrVector3f eyeOffsets[2];
  glm::mat4 eyeProjections[2];

public:
  CubeScene_Rift() {
    Stacks::modelview().top() = glm::lookAt(
      vec3(0, OVR_DEFAULT_EYE_HEIGHT, 5 * OVR_DEFAULT_IPD),
      vec3(0, OVR_DEFAULT_EYE_HEIGHT, 0),
      Vectors::UP);
  }

  virtual void initGl() {
    GlfwApp::initGl();

    ovrRenderAPIConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.Header.API = ovrRenderAPI_OpenGL;
    cfg.Header.BackBufferSize = ovr::fromGlm(getSize());
    cfg.Header.Multisample = 1;

    int distortionCaps = 
      ovrDistortionCap_Chromatic |
      ovrDistortionCap_Vignette;
    ovrEyeRenderDesc eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg,
        distortionCaps, hmd->DefaultEyeFov, eyeRenderDescs);

    for_each_eye([&](ovrEyeType eye){
      ovrFovPort fov = hmd->DefaultEyeFov[eye];
      ovrSizei texSize = ovrHmd_GetFovTextureSize(hmd, eye, fov, 1.0f);
      eyeFramebuffers[eye].init(ovr::toGlm(texSize));

      ovrTextureHeader & textureHeader = eyeTextures[eye].Header;
      textureHeader.API = ovrRenderAPI_OpenGL;
      textureHeader.TextureSize = texSize;
      textureHeader.RenderViewport.Size = texSize;
      textureHeader.RenderViewport.Pos.x = 0;
      textureHeader.RenderViewport.Pos.y = 0;
      ((ovrGLTexture&)eyeTextures[eye]).OGL.TexId = oglplus::GetName(*eyeFramebuffers[eye].color);

      eyeOffsets[eye] = eyeRenderDescs[eye].HmdToEyeViewOffset;

      ovrMatrix4f projection = ovrMatrix4f_Projection(fov, 0.01f, 100, true);
      eyeProjections[eye] = ovr::toGlm(projection);
    });
  }

  virtual void finishFrame() {
  }

  virtual void draw() {
    ovrPosef eyePoses[2];
    // Bug in SDK prevents direct mode from activating unless I call this
    ovrHmd_GetEyePoses(hmd, getFrame(), eyeOffsets, eyePoses, nullptr);

    ovrHmd_BeginFrame(hmd, getFrame());
    MatrixStack & mv = Stacks::modelview();
    for (int i = 0; i < ovrEye_Count; ++i) {
      ovrEyeType eye = hmd->EyeRenderOrder[i];
      Stacks::projection().top() = eyeProjections[eye];

      eyeFramebuffers[eye].Bind();
      oglplus::Context::Clear().DepthBuffer();
      Stacks::withPush(mv, [&]{
        mv.preMultiply(glm::translate(glm::mat4(), ovr::toGlm(eyeOffsets[eye])));
        oria::renderCubeScene(OVR_DEFAULT_IPD, OVR_DEFAULT_EYE_HEIGHT);
      });
    }
    oglplus::DefaultFramebuffer().Bind(oglplus::Framebuffer::Target::Draw);
    ovrHmd_EndFrame(hmd, eyePoses, eyeTextures);
  }
};

RUN_OVR_APP(CubeScene_Rift);
