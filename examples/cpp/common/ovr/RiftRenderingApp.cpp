#include "Common.h"

void RiftRenderingApp::initializeRiftRendering() {
    ovrGLConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.BackBufferSize = ovr::fromGlm(hmdNativeResolution);
    cfg.OGL.Header.Multisample = 1;

    ON_WINDOWS([&]{
      cfg.OGL.Window = (HWND)getNativeWindow();
    });

    int distortionCaps = 0
      | ovrDistortionCap_Vignette
      | ovrDistortionCap_Overdrive
      | ovrDistortionCap_TimeWarp;

    ON_LINUX([&]{
      distortionCaps |= ovrDistortionCap_LinuxDevFullscreen;
    });

    ovrEyeRenderDesc eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg.Config,
      distortionCaps, hmd->MaxEyeFov, eyeRenderDescs);
    assert(configResult);

    for_each_eye([&](ovrEyeType eye){
      const ovrEyeRenderDesc & erd = eyeRenderDescs[eye];
      ovrMatrix4f ovrPerspectiveProjection = ovrMatrix4f_Projection(erd.Fov, 0.01f, 100000.0f, true);
      projections[eye] = ovr::toGlm(ovrPerspectiveProjection);
      eyeOffsets[eye] = erd.HmdToEyeViewOffset;
    });

    // Allocate the frameBuffer that will hold the scene, and then be
    // re-rendered to the screen with distortion
    glm::uvec2 frameBufferSize = ovr::toGlm(eyeTextures[0].Header.TextureSize);
    for_each_eye([&](ovrEyeType eye) {
      eyeFramebuffers[eye] = FramebufferWrapperPtr(new FramebufferWrapper());
      eyeFramebuffers[eye]->init(frameBufferSize);
      ((ovrGLTexture&)(eyeTextures[eye])).OGL.TexId =
        oglplus::GetName(eyeFramebuffers[eye]->color);
    });
  }

RiftRenderingApp::RiftRenderingApp() {
    Platform::sleepMillis(200);
    if (!ovrHmd_ConfigureTracking(hmd,
      ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection, 0)) {
      SAY_ERR("Could not attach to sensor device");
    }

    memset(eyeTextures, 0, 2 * sizeof(ovrGLTexture));

    for_each_eye([&](ovrEyeType eye){
      ovrSizei eyeTextureSize = ovrHmd_GetFovTextureSize(hmd, eye, hmd->MaxEyeFov[eye], 1.0f);
      ovrTextureHeader & eyeTextureHeader = eyeTextures[eye].Header;
      eyeTextureHeader.TextureSize = eyeTextureSize;
      eyeTextureHeader.RenderViewport.Size = eyeTextureSize;
      eyeTextureHeader.API = ovrRenderAPI_OpenGL;
    });
  }

RiftRenderingApp::~RiftRenderingApp() {
}

static RateCounter rateCounter;

void RiftRenderingApp::drawRiftFrame() {
  ++frameCount;
  ovrHmd_BeginFrame(hmd, frameCount);
  MatrixStack & mv = Stacks::modelview();
  MatrixStack & pr = Stacks::projection();

  perFrameRender();
  
  ovrPosef fetchPoses[2];
  ovrHmd_GetEyePoses(hmd, frameCount, eyeOffsets, fetchPoses, nullptr);
  for (int i = 0; i < 2; ++i) {
    ovrEyeType eye = currentEye = hmd->EyeRenderOrder[i];
    // Force us to alternate eyes if we aren't keeping up with the required framerate
    if (eye == lastEyeRendered) {
      continue;
    }
    // We want to ensure that we only update the pose we 
    // send to the SDK if we actually render this eye.
    eyePoses[eye] = fetchPoses[eye];

    lastEyeRendered = eye;
    Stacks::withPush(pr, mv, [&] {
      // Set up the per-eye projection matrix
      pr.top() = projections[eye];

      // Set up the per-eye modelview matrix
      // Apply the head pose
      glm::mat4 eyePose = ovr::toGlm(eyePoses[eye]);
      mv.preMultiply(glm::inverse(eyePose));

      // Render the scene to an offscreen buffer
      eyeFramebuffers[eye]->Bind();
      perEyeRender();
    });
    
    if (eyePerFrameMode) {
      break;
    }
  }

  if (endFrameLock) {
    endFrameLock->lock();
  }
  ovrHmd_EndFrame(hmd, eyePoses, eyeTextures);
  if (endFrameLock) {
    endFrameLock->unlock();
  }
  rateCounter.increment();
  if (rateCounter.elapsed() > 2.0f) {
    float fps = rateCounter.getRate();
    updateFps(fps);
    rateCounter.reset();
  }
}


