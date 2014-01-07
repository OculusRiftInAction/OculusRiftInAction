#include "Common.h"

const float Rift::MONO_FOV = 65.0;
const float Rift::FRAMEBUFFER_OBJECT_SCALE = 1;;
const float DISPLACEMENT_MAP_SCALE = 0.02f;
const float Rift::ZFAR = 10000.0f;
const float Rift::ZNEAR = 0.01f;

//#define DISPLACEMENT_MAP_DISTORT 1
//#define DISTORTION_TIMING 1

#ifdef DISPLACEMENT_MAP_DISTORT
const Resource RiftApp::DISTORTION_VERTEX_SHADER =
    Resource::SHADERS_VERTEXTOTEXTURE_VS;
const Resource RiftApp::DISTORTION_FRAGMENT_SHADER =
   Resource::SHADERS_DISPLACEMENTMAPDISTORT_FS;
#else
const Resource RiftApp::DISTORTION_VERTEX_SHADER =
    Resource::SHADERS_VERTEXTORIFT_VS;
const Resource RiftApp::DISTORTION_FRAGMENT_SHADER =
    Resource::SHADERS_DIRECTDISTORT_FS;
#endif

using namespace OVR;


RiftApp::RiftApp()
    /// : renderMode(OVR::Util::Render::Stereo_None) {
    : renderMode(OVR::Util::Render::Stereo_LeftRight_Multipass) {
  ///////////////////////////////////////////////////////////////////////////
  // Initialize Oculus VR SDK and hardware
  bool attached = false;
  if (ovrManager) {
    ovrSensor =
        *ovrManager->EnumerateDevices<OVR::SensorDevice>().CreateDevice();
    if (ovrSensor) {
      sensorFusion.AttachToSensor(ovrSensor);
      attached = sensorFusion.IsAttachedToSensor();
    }
  }
}

void RiftApp::createRenderingTarget() {
//  glfwWindowHint(GLFW_SAMPLES, 4);
  RiftRenderApp::createRenderingTarget();
}

RiftApp::~RiftApp() {
  sensorFusion.AttachToSensor(nullptr);
  ovrSensor.Clear();
  ovrManager.Clear();
}

void RiftApp::initGl() {
  RiftRenderApp::initGl();
  query = gl::TimeQueryPtr(new gl::TimeQuery());
  query->begin();
  query->end();
  GL_CHECK_ERROR;

  ///////////////////////////////////////////////////////////////////////////
  // Initialize OpenGL settings and variables
  // Anti-alias lines (hopefully)
  glEnable(GL_BLEND);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
  GL_CHECK_ERROR;

  // Allocate the frameBuffer that will hold the scene, and then be
  // re-rendered to the screen with distortion
  glm::ivec2 frameBufferSize = glm::ivec2(glm::vec2(eyeSize) *
      Rift::FRAMEBUFFER_OBJECT_SCALE);
  frameBuffer.init(frameBufferSize);
  GL_CHECK_ERROR;

  // Create the buffers for the texture quad we will draw
  quadGeometry = GlUtils::getQuadGeometry();

#ifdef DISPLACEMENT_MAP_DISTORT
  // Create the rendering displacement map
  generateDisplacementTexture(offsetTexture, DISPLACEMENT_MAP_SCALE);
  GL_CHECK_ERROR;
#endif

  // Create the rendering shaders
  distortProgram = GlUtils::getProgram(
      DISTORTION_VERTEX_SHADER,
      DISTORTION_FRAGMENT_SHADER);
  GL_CHECK_ERROR;
  distortProgram->use();
  GL_CHECK_ERROR;
  bindUniforms(distortProgram);
#ifdef DISPLACEMENT_MAP_DISTORT
  distortProgram->setUniform1i("OffsetMap", 1);
#endif
  distortProgram->setUniform1i("Scene", 0);
  GL_CHECK_ERROR;
  gl::Program::clear();
  GL_CHECK_ERROR;
}

void RiftApp::onKey(int key, int scancode, int action, int mods) {
  if (GLFW_PRESS != action) {
    return;
  }

  switch (key) {
  case GLFW_KEY_R:
    sensorFusion.Reset();
    return;

  case GLFW_KEY_P:
    if (renderMode == OVR::Util::Render::Stereo_None) {
      renderMode = OVR::Util::Render::Stereo_LeftRight_Multipass;
    } else {
      renderMode = OVR::Util::Render::Stereo_None;
    }
    return;
  }
  RiftRenderApp::onKey(key, scancode, action, mods);
}


void RiftApp::draw() {
  GL_CHECK_ERROR;
  if (renderMode == OVR::Util::Render::Stereo_None) {
    gl::Stacks::projection().top() = glm::perspective(
        60.0f, glm::aspect(hmdSize),
        Rift::ZNEAR, Rift::ZFAR);
    // If we're not working stereo, we're just going to render the
    // scene once, from a single position, directly to the back buffer
    gl::viewport(hmdSize);
    renderScene();
    GL_CHECK_ERROR;
  } else {
    // If we get here, we're rendering in stereo, so we have to render our output twice

    // We have to explicitly clear the screen here.  the Clear command doesn't object the viewport
    // and the clear command inside renderScene will only target the active framebuffer object.
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GL_CHECK_ERROR;
    using namespace OVR::Util::Render;
    for (int i = 0; i < 2; ++i) {
      const PerEyeArgs & eyeArgs = eye[i];
      // Compute the modelview and projection offsets for the rendered scene based on the eye and
      // whether or not we're doing side by side or rift rendering
      frameBuffer.activate();
      GL_CHECK_ERROR;

      renderScene(eyeArgs);
      GL_CHECK_ERROR;
      frameBuffer.deactivate();
      GL_CHECK_ERROR;

      eyeArgs.viewport();
      static long accumulator = 0;
      static long count = 0;
#ifdef DISTORTION_TIMING
      query->begin();
#endif
      distortProgram = GlUtils::getProgram(
          DISTORTION_VERTEX_SHADER,
          DISTORTION_FRAGMENT_SHADER);
      distortProgram->use();
      bindUniforms(distortProgram);
      eyeArgs.bindUniforms(distortProgram);
      glActiveTexture(GL_TEXTURE1);
#ifdef DISPLACEMENT_MAP_DISTORT
      offsetTexture->bind();
#endif
      glActiveTexture(GL_TEXTURE0);
      frameBuffer.color->bind();
//      frameBuffer.color->generateMipmap();
      quadGeometry->bindVertexArray();
      quadGeometry->draw();
      gl::VertexArray::unbind();
      gl::Program::clear();
#ifdef DISTORTION_TIMING
      query->end();
      accumulator += query->getReult();
      count++;
      SAY("%d ns", accumulator / count);
#endif
    } // for
  } // if
}

void RiftApp::renderScene(const glm::mat4 & modelviewOffset) {
}
