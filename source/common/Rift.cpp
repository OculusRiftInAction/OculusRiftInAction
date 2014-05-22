#include "Common.h"
#include <OVR_CAPI_GL.h>

RiftApp::RiftApp(bool fullscreen) :  RiftGlfwApp(fullscreen) {
  if (!ovrHmd_StartSensor(hmd, 0, 0)) {
    SAY_ERR("Could not attach to sensor device");
  }

  float eyeHeight = 1.5f;
  player = glm::inverse(glm::lookAt(
    glm::vec3(0, eyeHeight, 4),
    glm::vec3(0, eyeHeight, 0),
    glm::vec3(0, 1, 0)));
}

RiftApp::~RiftApp() {
  ovrHmd_StopSensor(hmd);
  ovrHmd_Destroy(hmd);
  hmd = nullptr;
}

void RiftApp::finishFrame() {

}

void RiftApp::initGl() {
  RiftGlfwApp::initGl();
  query = gl::TimeQueryPtr(new gl::TimeQuery());
  GL_CHECK_ERROR;

  ovrEyeDesc eyeDescs[2];
  for_each_eye([&](ovrEyeType eye){
    ovrEyeDesc & eyeDesc = eyeDescs[eye];
    eyeDesc.Eye = eye;
    eyeDesc.Fov = hmdDesc.DefaultEyeFov[eye];
    eyeDesc.TextureSize = ovrHmd_GetFovTextureSize(hmd, eye, hmdDesc.DefaultEyeFov[eye], 1.0f);
    eyeDesc.RenderViewport.Size = eyeDesc.TextureSize;
    eyeDesc.RenderViewport.Pos.x = 0;
    eyeDesc.RenderViewport.Pos.y = 0;

    ovrTextureHeader & eyeTextureHeader = eyeTextures[eye].Header;
    eyeTextureHeader.TextureSize = eyeDesc.TextureSize;
    eyeTextureHeader.RenderViewport = eyeDesc.RenderViewport;
    eyeTextureHeader.API = ovrRenderAPI_OpenGL;
  });

  ovrGLConfig cfg;
  cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
  cfg.OGL.Header.RTSize = hmdDesc.Resolution;
  cfg.OGL.Header.Multisample = 1;

  int distortionCaps = ovrDistortion_Chromatic | ovrDistortion_TimeWarp;
  int renderCaps = 0;

  void * foo = glDrawElements;
  void * bar = glBindBuffer;
  int configResult = ovrHmd_ConfigureRendering(hmd, &cfg.Config,
      renderCaps, distortionCaps, eyeDescs, eyeRenderDescs);


  ///////////////////////////////////////////////////////////////////////////
  // Initialize OpenGL settings and variables
  // Anti-alias lines (hopefully)
  glEnable(GL_BLEND);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
  GL_CHECK_ERROR;
  
  // Allocate the frameBuffer that will hold the scene, and then be
  // re-rendered to the screen with distortion
  glm::uvec2 frameBufferSize = glm::uvec2(
    glm::vec2(eyeDescs[0].TextureSize.w, eyeDescs[0].TextureSize.h));
  for_each_eye([&](ovrEyeType eye) {
    frameBuffers[eye].init(frameBufferSize);
    eyeTextures[eye].Header.API = ovrRenderAPI_OpenGL;
    eyeTextures[eye].Header.TextureSize = eyeDescs[0].TextureSize;
    eyeTextures[eye].Header.RenderViewport = eyeDescs[0].RenderViewport;
    ((ovrGLTexture&)eyeTextures[eye]).OGL.TexId = frameBuffers[eye].color->texture;
  });
  GL_CHECK_ERROR;
}

void RiftApp::onKey(int key, int scancode, int action, int mods) {
  if (GLFW_PRESS == action) switch (key) {
  case GLFW_KEY_R:
    ovrHmd_ResetSensor(hmd);
    return;
  }

  // Allow the camera controller to intercept the input
  if (CameraControl::instance().onKey(player, key, scancode, action, mods)) {
    return;
  }
  RiftGlfwApp::onKey(key, scancode, action, mods);
}


void RiftApp::update() {
  RiftGlfwApp::update();
  CameraControl::instance().applyInteraction(player);
//  gl::Stacks::modelview().top() = glm::lookAt(glm::vec3(0, 0, 0.4f), glm::vec3(0), glm::vec3(0, 1, 0));
}

void RiftApp::draw() {
  static int frameIndex = 0;
  ovrHmd_BeginFrame(hmd, frameIndex++);
  gl::MatrixStack & mv = gl::Stacks::modelview();
  gl::MatrixStack & pr = gl::Stacks::projection();
  for_each_eye([&](ovrEyeType eye) {
    gl::Stacks::with_push(pr, mv, [&]{
      const ovrEyeRenderDesc & erd = eyeRenderDescs[eye];
      // Set up the per-eye projection matrix
      {
        ovrMatrix4f eyeProjection = ovrMatrix4f_Projection(erd.Desc.Fov, 0.01f, 100000.0f, true);
        glm::mat4 ovrProj = Rift::fromOvr(eyeProjection);
        pr.top() = ovrProj;
      }

      ovrPosef renderPose = ovrHmd_BeginEyeRender(hmd, eye);
      // Set up the per-eye modelview matrix
      {
        // Apply the head pose
        glm::mat4 m = Rift::fromOvr(renderPose);
        mv.preMultiply(glm::inverse(m));
        // Apply the per-eye offset
        glm::vec3 eyeOffset = Rift::fromOvr(erd.ViewAdjust);
        mv.preMultiply(glm::translate(glm::mat4(), eyeOffset));
      }

      // Render the scene to an offscreen buffer
      frameBuffers[eye].activate();
      //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      //glEnable(GL_DEPTH_TEST);
      renderScene();
      frameBuffers[eye].deactivate();

      ovrHmd_EndEyeRender(hmd, eye, renderPose, &eyeTextures[eye]);
    });
    GL_CHECK_ERROR;
  });
  query->begin();
#if 1
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  ovrHmd_EndFrame(hmd);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
#else
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  static gl::GeometryPtr geometry = GlUtils::getQuadGeometry(1.0, 1.5f);
  static gl::ProgramPtr program = GlUtils::getProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);
  program->use();
  geometry->bindVertexArray();
  gl::Stacks::with_push(pr, mv, [&]{
    pr.identity(); mv.identity();
    frameBuffers[0].color->bind();
    glViewport(0, 0, 640, 800);
    geometry->draw();
    frameBuffers[1].color->bind();
    glViewport(640, 0, 640, 800);
    geometry->draw();
  });
  gl::Program::clear();
  gl::VertexArray::unbind();
  glfwSwapBuffers(window);
#endif
  query->end();
  int result = query->getResult();
  GL_CHECK_ERROR;
}

void RiftApp::renderStringAt(const std::string & str, float x, float y) {
  gl::MatrixStack & mv = gl::Stacks::modelview();
  gl::MatrixStack & pr = gl::Stacks::projection();
  gl::Stacks::with_push(mv, pr, [&]{
    mv.identity();
    pr.top() = 1.0f * glm::ortho(
      -1.0f, 1.0f,
      -windowAspectInverse * 2.0f, windowAspectInverse * 2.0f,
      -100.0f, 100.0f);
    glm::vec2 cursor(x, windowAspectInverse * y);
    GlUtils::renderString(str, cursor, 18.0f);
  });
}
