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

  ovrFovPort eyeFovPorts[2];
  for_each_eye([&](ovrEyeType eye){
    ovrTextureHeader & eyeTextureHeader = eyeTextures[eye].Header;
    eyeFovPorts[eye] = hmdDesc.DefaultEyeFov[eye];
    ovrSizei eyeTextureSize = ovrHmd_GetFovTextureSize(hmd, eye, hmdDesc.DefaultEyeFov[eye], 1.0f);
    frameBuffers[eye].init(Rift::fromOvr(eyeTextureSize));

    eyeTextureHeader.RenderViewport.Size = eyeTextureSize;
    eyeTextureHeader.RenderViewport.Pos.x = 0;
    eyeTextureHeader.RenderViewport.Pos.y = 0;
    eyeTextureHeader.API = ovrRenderAPI_OpenGL;
    eyeTextureHeader.TextureSize = eyeTextureSize;
  });

  ovrGLConfig cfg;
  cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
  cfg.OGL.Header.RTSize = hmdDesc.Resolution;
  cfg.OGL.Header.Multisample = 1;

  int distortionCaps = ovrDistortionCap_Chromatic | ovrDistortionCap_TimeWarp;
  int renderCaps = 0;

  int configResult = ovrHmd_ConfigureRendering(hmd, &cfg.Config,
    renderCaps, eyeFovPorts, eyeRenderDescs);

  float    orthoDistance = 0.8f; // 2D is 0.8 meter from camera
  for_each_eye([&](ovrEyeType eye){
    const ovrEyeRenderDesc & erd = eyeRenderDescs[eye];
    ovrMatrix4f ovrPerspectiveProjection = ovrMatrix4f_Projection(erd.Fov, 0.01f, 100000.0f, true);
    projections[eye] = Rift::fromOvr(ovrPerspectiveProjection);
    glm::vec2 orthoScale = glm::vec2(1.0f) / Rift::fromOvr(erd.PixelsPerTanAngleAtCenter);
    orthoProjections[eye] = Rift::fromOvr(
        ovrMatrix4f_OrthoSubProjection(
            ovrPerspectiveProjection, Rift::toOvr(orthoScale), orthoDistance, erd.ViewAdjust.x));
  });

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
    glm::vec2(eyeTextures[0].Header.RenderViewport.Size.w, eyeTextures[0].Header.RenderViewport.Size.h));
  for_each_eye([&](ovrEyeType eye) {
    frameBuffers[eye].init(frameBufferSize);
    eyeTextures[eye].Header.API = ovrRenderAPI_OpenGL;
    eyeTextures[eye].Header.TextureSize = eyeTextures[0].Header.RenderViewport.Size;
    eyeTextures[eye].Header.RenderViewport = eyeTextures[0].Header.RenderViewport;
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
        ovrMatrix4f eyeProjection = ovrMatrix4f_Projection(erd.Fov, 0.01f, 100000.0f, true);
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
