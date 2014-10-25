#include "Common.h"
#include "CubeScene.h"

struct PerEyeArg {
  glm::mat4                     projection;
  gl::FrameBufferWrapper        frameBuffer;
};

class CubeScene_RiftTimewarp: public CubeScene {
  PerEyeArg   eyes[2];
  ovrTexture  textures[2];
  ovrVector3f hmdToEyeOffsets[2];
  int         frameIndex{ 0 };

public:
  CubeScene_RiftTimewarp() {
    if (!ovrHmd_ConfigureTracking(hmd, 
        ovrTrackingCap_Orientation |
        ovrTrackingCap_Position, 0)) {
      SAY("Warning: Unable to locate Rift sensor device.  This demo is boring now.");
    }
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
      ovrDistortionCap_Vignette |
      ovrDistortionCap_TimeWarp;
    ovrEyeRenderDesc eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg,
      distortionCaps, hmd->DefaultEyeFov, eyeRenderDescs);

    for_each_eye([&](ovrEyeType eye){
      PerEyeArg & eyeArg = eyes[eye];
      ovrFovPort fov = hmd->DefaultEyeFov[eye];
      ovrTextureHeader & textureHeader = textures[eye].Header;
      ovrSizei texSize = ovrHmd_GetFovTextureSize(hmd, eye, fov, 1.0f);
      textureHeader.API = ovrRenderAPI_OpenGL;
      textureHeader.TextureSize = texSize;
      textureHeader.RenderViewport.Size = texSize;
      textureHeader.RenderViewport.Pos.x = 0;
      textureHeader.RenderViewport.Pos.y = 0;
      eyeArg.frameBuffer.init(Rift::fromOvr(texSize));
      ((ovrGLTexture&)textures[eye]).OGL.TexId = eyeArg.frameBuffer.color->texture;

      hmdToEyeOffsets[eye] = eyeRenderDescs[eye].HmdToEyeViewOffset;
      ovrMatrix4f projection = ovrMatrix4f_Projection(fov, 0.01f, 100, true);

      eyeArg.projection = Rift::fromOvr(projection);
    });
  }

  virtual void finishFrame() {
  }

  ovrPosef eyePoses[2];

  virtual void draw() {
    ++frameIndex;
    ovrPosef eyePoses[2];
    ovrHmd_BeginFrame(hmd, frameIndex);
    ovrHmd_GetEyePoses(hmd, frameIndex, hmdToEyeOffsets, eyePoses, nullptr);

    gl::MatrixStack & mv = gl::Stacks::modelview();
    for (int i = 0; i < ovrEye_Count; ++i) {
      ovrEyeType eye = hmd->EyeRenderOrder[i];
      PerEyeArg & eyeArgs = eyes[eye];
      gl::Stacks::projection().top() = eyeArgs.projection;

      eyeArgs.frameBuffer.withFramebufferActive([&]{
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl::Stacks::with_push(mv, [&]{
          mv.preMultiply(glm::inverse(Rift::fromOvr(eyePoses[eye])));
          drawCubeScene();
        });
      });
    }

    ovrHmd_EndFrame(hmd, eyePoses, textures);
  }

  void onKey(int key, int scancode, int action, int mods) {
    ovrHSWDisplayState hswState;
    if (GLFW_PRESS == action) {
      ovrHmd_GetHSWDisplayState(hmd, &hswState);
      if (hswState.Displayed) {
        ovrHmd_DismissHSWDisplay(hmd);
        return;
      }
    }

    if (CameraControl::instance().onKey(key, scancode, action, mods)) {
      return;
    }

    if (GLFW_PRESS == action) {
      switch (key) {
      case GLFW_KEY_P:
        toggleCap(ovrHmdCap_LowPersistence);
        return;
      }
    }

    CubeScene::onKey(key, scancode, action, mods);
  }

  

};

RUN_OVR_APP(CubeScene_RiftTimewarp);
