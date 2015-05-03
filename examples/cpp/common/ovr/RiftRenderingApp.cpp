#include "Common.h"

void RiftRenderingApp::initializeRiftRendering() {
  ovrLayerEyeFov & layer = layers[0].EyeFov;
  layer.Header.Type = ovrLayerType_EyeFov;
  layer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;

  for_each_eye([&](ovrEyeType eye) {
    EyeParams & ep = eyesParams[eye];
    ep.renderDesc = ovrHmd_GetRenderDesc(hmd, eye, hmd->MaxEyeFov[eye]);
    const ovrEyeRenderDesc & erd = ep.renderDesc;
    eyeOffsets[eye] = erd.HmdToEyeViewOffset;

    const ovrFovPort & fov = erd.Fov;
    layer.Fov[eye] = fov;

    ovrRecti & viewport = layer.Viewport[eye];
    viewport.Pos = { 0, 0 };
    viewport.Size = ovrHmd_GetFovTextureSize(hmd, eye, fov, 1.0f);
    ep.size = ovr::toGlm(viewport.Size);
    ovrMatrix4f ovrPerspectiveProjection = ovrMatrix4f_Projection(fov, 0.01f, 100000.0f, true);
    ep.projection = ovr::toGlm(ovrPerspectiveProjection);

    // Allocate the frameBuffer that will hold the scene, and then be
    // submitted to the SDk for distortion and display on screen
    ep.fbo = ovr::SwapTexFboPtr(new ovr::SwapTextureFramebufferWrapper(hmd, ovr::toGlm(viewport.Size)));
    layer.ColorTexture[eye] = ep.fbo->textureSet;
  });
}

void RiftRenderingApp::setupMirror(const glm::uvec2 & size) {
  if (mirrorEnabled) {
      if (!mirrorFbo) {
        mirrorFbo = ovr::MirrorFboPtr(new ovr::MirrorFramebufferWrapper(hmd));
      }
    mirrorFbo->Init(size);
  }
}


RiftRenderingApp::RiftRenderingApp() : layers(1) {
  Platform::sleepMillis(200);
  if (!ovrHmd_ConfigureTracking(hmd,
    ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection, 0)) {
    SAY_ERR("Could not attach to sensor device");
  }
}

RiftRenderingApp::~RiftRenderingApp() {
}

static RateCounter rateCounter;

void RiftRenderingApp::drawRiftFrame() {
  MatrixStack & mv = Stacks::modelview();
  MatrixStack & pr = Stacks::projection();

  perFrameRender();

  ovrPosef fetchPoses[2];
  ovrLayerEyeFov & layer = layers[0].EyeFov;
  ovrHmd_GetEyePoses(hmd, frameCount, eyeOffsets, fetchPoses, nullptr);
  for (int i = 0; i < 2; ++i) {
    ovrEyeType eye = currentEye = hmd->EyeRenderOrder[i];
    EyeParams & eyeParams = eyesParams[eye];
    // Force us to alternate eyes if we aren't keeping up with the required framerate
    if (eye == lastEyeRendered) {
      continue;
    }
    ovrPosef & pose = fetchPoses[eye];

    // We want to ensure that we only update the pose we 
    // send to the SDK if we actually render this eye.
    layer.RenderPose[eye] = pose;
    lastEyeRendered = eye;
    Stacks::withPush(pr, mv, [&] {
      // Set up the per-eye projection matrix
      pr.top() = eyeParams.projection;

      // Set up the per-eye modelview matrix
      // Apply the head pose
      glm::mat4 eyePose = ovr::toGlm(pose);
      mv.preMultiply(glm::inverse(eyePose));

      // Render the scene to an offscreen buffer
      eyeParams.fbo->Pushed([&]{
        eyeParams.fbo->Viewport();
        perEyeRender();
      });
    });

    if (eyePerFrameMode) {
      break;
    }
  }

  if (endFrameLock) {
    endFrameLock->lock();
  }

  ovrLayerHeader* layers = &layer.Header;
  ovrHmd_SubmitFrame(hmd, frameCount, nullptr, &layers, 1);
  if (endFrameLock) {
    endFrameLock->unlock();
  }
  for_each_eye([&](ovrEyeType eye) {
    if (eyePerFrameMode && lastEyeRendered == eye) {
      // Only increment the eye we're going to change next
      return;
    }
    eyesParams[eye].fbo->Increment();
  });

  if (mirrorEnabled && mirrorFbo) {
      mirrorFbo->Pushed(oglplus::Framebuffer::Target::Read, [&] {
          // Copy the mirror buffer, flipping the Y axis, because reasons.
          glBlitFramebuffer(
              0, mirrorFbo->size.y, mirrorFbo->size.x, 0,
              0, 0, mirrorFbo->size.x, mirrorFbo->size.y,
              GL_COLOR_BUFFER_BIT, GL_NEAREST
          );
      });
  }
  


  rateCounter.increment();
  if (rateCounter.elapsed() > 2.0f) {
    float fps = rateCounter.getRate();
    updateFps(fps);
    rateCounter.reset();
  }
  ++frameCount;
}


