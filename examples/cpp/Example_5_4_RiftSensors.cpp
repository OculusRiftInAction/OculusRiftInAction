#include "Common.h"
#include "CubeScene.h"

static const glm::uvec2 WINDOW_SIZE(1280, 800);

struct PerEyeArg {
  glm::mat4                     modelviewOffset;
  glm::mat4                     projection;
  gl::FrameBufferWrapper        frameBuffer;
  ovrFovPort                    fovPort;
  ovrGLTexture                  texture;
  ovrEyeRenderDesc              renderDesc;
};

class CubeScene_RiftSensors: public CubeScene {
  PerEyeArg eyes[2];
  int frameIndex{ 0 };

public:
  CubeScene_RiftSensors() {
    eyes[ovrEye_Left].modelviewOffset = glm::translate(glm::mat4(),
        glm::vec3(ipd / 2.0f, 0, 0));
    eyes[ovrEye_Right].modelviewOffset = glm::translate(glm::mat4(),
        glm::vec3(-ipd / 2.0f, 0, 0));
    windowSize = WINDOW_SIZE;

    if (!ovrHmd_StartSensor(hmd, ovrSensorCap_Orientation, 0)) {
      SAY("Warning: Unable to locate Rift sensor device.  This demo is boring now.");
    }
  }

  virtual ~CubeScene_RiftSensors() {
    ovrHmd_StopSensor(hmd);
  }

  virtual void initGl() {
    CubeScene::initGl();

    for_each_eye([&](ovrEyeType eye){
      PerEyeArg & eyeArg = eyes[eye];
      eyeArg.fovPort = hmdDesc.DefaultEyeFov[eye];
      ovrTextureHeader & textureHeader = eyeArg.texture.Texture.Header;
      ovrSizei texSize = ovrHmd_GetFovTextureSize(hmd, eye, eyeArg.fovPort, 1.0f);
      textureHeader.API = ovrRenderAPI_OpenGL;
      textureHeader.TextureSize = texSize;
      textureHeader.RenderViewport.Size = texSize;
      textureHeader.RenderViewport.Pos.x = 0;
      textureHeader.RenderViewport.Pos.y = 0;
      eyeArg.frameBuffer.init(Rift::fromOvr(texSize));
      eyeArg.texture.OGL.TexId = eyeArg.frameBuffer.color->texture;

      ovrMatrix4f projection = ovrMatrix4f_Projection(eyeArg.fovPort, 0.01f, 100, true);
      eyeArg.projection = Rift::fromOvr(projection);
    });

    ovrRenderAPIConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.Header.API = ovrRenderAPI_OpenGL;
    cfg.Header.RTSize = hmdDesc.Resolution;
    cfg.Header.Multisample = 1;

    int distortionCaps = ovrDistortionCap_Chromatic;
    int renderCaps = 0;
    ovrFovPort eyePorts[] = { eyes[0].fovPort, eyes[1].fovPort };
    ovrEyeRenderDesc eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg,
        distortionCaps, eyePorts, eyeRenderDescs);
  }

  virtual void finishFrame() {
  }

  virtual void draw() {
    ovrHmd_BeginFrame(hmd, frameIndex++);

    ovrSensorState sensorState = ovrHmd_GetSensorState(hmd, 0);
    ovrPoseStatef & poseState = sensorState.Recorded;
    glm::mat4 orientation = glm::inverse(Rift::fromOvr(poseState.Pose));

    gl::MatrixStack & mv = gl::Stacks::modelview();
    for (int i = 0; i < ovrEye_Count; ++i) {
      ovrEyeType eye = hmdDesc.EyeRenderOrder[i];
      PerEyeArg & eyeArgs = eyes[eye];
      ovrPosef renderPose = ovrHmd_BeginEyeRender(hmd, eye);

      gl::Stacks::projection().top() = eyeArgs.projection;

      eyeArgs.frameBuffer.withFramebufferActive([&]{
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl::Stacks::with_push(mv, [&]{
          mv.preMultiply(eyeArgs.modelviewOffset);
          mv.preMultiply(orientation);
          drawCubeScene();
        });
      });

      ovrHmd_EndEyeRender(hmd, eye, renderPose, &eyeArgs.texture.Texture);
    }

    ovrHmd_EndFrame(hmd);
  }
};

RUN_OVR_APP(CubeScene_RiftSensors);
