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

#include "Common.h"
#include "RiftApp.h"
#include <OVR_CAPI_GL.h>

RiftApp::RiftApp() :  RiftGlfwApp() {
  Platform::sleepMillis(200);
  if (!ovrHmd_ConfigureTracking(hmd,
    ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection, 0)) {
    SAY_ERR("Could not attach to sensor device");
  }

  memset(eyeTextures, 0, 2 * sizeof(ovrGLTexture));
  float eyeHeight = 1.5f;
  player = glm::inverse(glm::lookAt(
    glm::vec3(0, eyeHeight, 4),
    glm::vec3(0, eyeHeight, 0),
    glm::vec3(0, 1, 0)));

  for_each_eye([&](ovrEyeType eye){
    ovrSizei eyeTextureSize = ovrHmd_GetFovTextureSize(hmd, eye, hmd->MaxEyeFov[eye], 1.0f);

    ovrTextureHeader & eyeTextureHeader = eyeTextures[eye].Header;
    eyeTextureHeader.TextureSize = eyeTextureSize;
    eyeTextureHeader.RenderViewport.Size = eyeTextureSize;
    eyeTextureHeader.API = ovrRenderAPI_OpenGL;
  });
}

RiftApp::~RiftApp() {
}

void RiftApp::finishFrame() {
}

void RiftApp::initGl() {
  RiftGlfwApp::initGl();

  int samples;
  glGetIntegerv(GL_SAMPLES, &samples);

  ovrGLConfig cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
  cfg.OGL.Header.BackBufferSize = ovr::fromGlm(getSize());
  cfg.OGL.Header.Multisample = 1;

  int distortionCaps = 0
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
    eyeFramebuffers[eye] = FramebufferWrapperPtr(new FramebufferWrapper());
    eyeFramebuffers[eye]->init(frameBufferSize);
    ((ovrGLTexture&)(eyeTextures[eye])).OGL.TexId = 
        oglplus::GetName(eyeFramebuffers[eye]->color);
  });
}

void RiftApp::update() {
  RiftGlfwApp::update();
//  CameraControl::instance().applyInteraction(player);
//  Stacks::modelview().top() = glm::lookAt(glm::vec3(0, 0, 0.4f), glm::vec3(0), glm::vec3(0, 1, 0));
}

void RiftApp::applyEyePoseAndOffset(const glm::mat4 & eyePose, const glm::vec3 & eyeOffset) {
  MatrixStack & mv = Stacks::modelview();
  mv.preMultiply(glm::inverse(eyePose));
  // Apply the per-eye offset
  mv.preMultiply(glm::translate(glm::mat4(), eyeOffset));
}

void RiftApp::draw() {
  ovrHmd_BeginFrame(hmd, getFrame());
  MatrixStack & mv = Stacks::modelview();
  MatrixStack & pr = Stacks::projection();
  
  ovrHmd_GetEyePoses(hmd, getFrame(), eyeOffsets, eyePoses, nullptr);
  for (int i = 0; i < 2; ++i) {
    ovrEyeType eye = currentEye = hmd->EyeRenderOrder[i];
    Stacks::withPush(pr, mv, [&]{
      const ovrEyeRenderDesc & erd = eyeRenderDescs[eye];
      // Set up the per-eye projection matrix
      {
        ovrMatrix4f eyeProjection = ovrMatrix4f_Projection(erd.Fov, 0.01f, 100000.0f, true);
        glm::mat4 ovrProj = ovr::toGlm(eyeProjection);
        pr.top() = ovrProj;
      }

      // Set up the per-eye modelview matrix
      {
        // Apply the head pose
        glm::mat4 eyePose = ovr::toGlm(eyePoses[eye]);
        applyEyePoseAndOffset(eyePose, glm::vec3(0));
      }

      // Render the scene to an offscreen buffer
      eyeFramebuffers[eye]->Bind();
      renderScene();
    });
  }
  // Restore the default framebuffer
  oglplus::DefaultFramebuffer().Bind(oglplus::Framebuffer::Target::Draw);

#if 1
  ovrHmd_EndFrame(hmd, eyePoses, eyeTextures);
#else
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  static gl::GeometryPtr geometry = GlUtils::getQuadGeometry(1.0, 1.5f);
  static gl::ProgramPtr program = GlUtils::getProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);
  program->use();
  geometry->bindVertexArray();
  Stacks::with_push(pr, mv, [&]{
    pr.identity(); mv.identity();
    frameBuffers[0].color->bind();
    viewport(ovrEye_Left);
    geometry->draw();
    frameBuffers[1].color->bind();
    viewport(ovrEye_Right);
    geometry->draw();
  });
  gl::Program::clear();
  gl::VertexArray::unbind();
  glfwSwapBuffers(window);
#endif
}

void RiftApp::renderStringAt(const std::string & str, float x, float y, float size) {
  MatrixStack & mv = Stacks::modelview();
  MatrixStack & pr = Stacks::projection();
  Stacks::withPush(mv, pr, [&]{
    mv.identity();
    pr.top() = 1.0f * glm::ortho(
      -1.0f, 1.0f,
      -windowAspectInverse * 2.0f, windowAspectInverse * 2.0f,
      -100.0f, 100.0f);
    glm::vec2 cursor(x, windowAspectInverse * y);
    oria::renderString(str, cursor, size);
  });
}
