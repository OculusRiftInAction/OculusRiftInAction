/************************************************************************************

 Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
 Copyright   :   Copyright Brad Davis. All Rights reserved.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 ************************************************************************************/

#pragma once

class RiftRenderingApp : public RiftManagerApp {

protected:
  ovrTexture eyeTextures[2];
  ovrVector3f eyeOffsets[2];

private:
  ovrEyeRenderDesc eyeRenderDescs[2];
  ovrPosef eyePoses[2];
  ovrEyeType currentEye;
  glm::mat4 projections[2];
  FramebufferWrapper eyeFramebuffers[2];
  unsigned int frameCount = 0;


protected:
  virtual void onFrameStart() {
  }

  virtual void onFrameEnd() {

  }

  virtual void * getRenderWindow() = 0;

  virtual void initGl() {
    glewExperimental = true;
    glewInit();
    glGetError();
    oglplus::DefaultFramebuffer().Bind(oglplus::Framebuffer::Target::Draw);

    ovrGLConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.RTSize = ovr::fromGlm(hmdNativeResolution);
    cfg.OGL.Header.Multisample = 1;
    ON_WINDOWS([&]{
      cfg.OGL.Window = (HWND)getRenderWindow();
    });

    int distortionCaps =
      ovrDistortionCap_Chromatic
      | ovrDistortionCap_Vignette
      | ovrDistortionCap_Overdrive
      | ovrDistortionCap_TimeWarp;

    ON_LINUX([&]{
      distortionCaps |= ovrDistortionCap_LinuxDevFullscreen;
    });

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
      eyeFramebuffers[eye].init(frameBufferSize);
      ((ovrGLTexture&)(eyeTextures[eye])).OGL.TexId =
        oglplus::GetName(*eyeFramebuffers[eye].color);
    });
  }

  virtual void draw() {
    ++frameCount;
    onFrameStart();
    ovrHmd_BeginFrame(hmd, frameCount);
    MatrixStack & mv = Stacks::modelview();
    MatrixStack & pr = Stacks::projection();

    ovrHmd_GetEyePoses(hmd, frameCount, eyeOffsets, eyePoses, nullptr);
    for (int i = 0; i < 2; ++i) {
      ovrEyeType eye = currentEye = hmd->EyeRenderOrder[i];
      Stacks::withPush(pr, mv, [&]{
        // Set up the per-eye projection matrix
        pr.top() = projections[eye];

        // Set up the per-eye modelview matrix
        // Apply the head pose
        glm::mat4 eyePose = ovr::toGlm(eyePoses[eye]);
        mv.preMultiply(glm::inverse(eyePose));

        // Render the scene to an offscreen buffer
        eyeFramebuffers[eye].Bind();
        renderScene();
      });
    }
    // Restore the default framebuffer
    //oglplus::DefaultFramebuffer().Bind(oglplus::Framebuffer::Target::Draw);
    ovrHmd_EndFrame(hmd, eyePoses, eyeTextures);
    onFrameEnd();
  }

  virtual void renderScene() = 0;

  inline ovrEyeType getCurrentEye() const {
    return currentEye;
  }

  const ovrEyeRenderDesc & getEyeRenderDesc(ovrEyeType eye) const {
    return eyeRenderDescs[eye];
  }

  const ovrFovPort & getFov(ovrEyeType eye) const {
    return eyeRenderDescs[eye].Fov;
  }

  const glm::mat4 & getPerspectiveProjection(ovrEyeType eye) const {
    return projections[eye];
  }

  const ovrPosef & getEyePose(ovrEyeType eye) const {
    return eyePoses[eye];
  }

  const ovrPosef & getEyePose() const {
    return getEyePose(getCurrentEye());
  }

  const ovrFovPort & getFov() const {
    return getFov(getCurrentEye());
  }

  const ovrEyeRenderDesc & getEyeRenderDesc() const {
    return getEyeRenderDesc(getCurrentEye());
  }

  const glm::mat4 & getPerspectiveProjection() const {
    return getPerspectiveProjection(getCurrentEye());
  }

public:
  RiftRenderingApp() {
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

  virtual ~RiftRenderingApp() {

  }
};


