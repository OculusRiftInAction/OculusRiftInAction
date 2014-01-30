#include "Common.h"

#ifdef HAVE_JSON
#include <json/json.h>
#include <fstream>
#endif

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

#if HAVE_JSON
glm::quat parseQuaternion(const Json::Value & node) {
  glm::quat out;
  out.x = node.get("x", 0).asFloat();
  out.y = node.get("y", 0).asFloat();
  out.z = node.get("z", 0).asFloat();
  out.w = node.get("w", 1).asFloat();
  return out;
}

std::string getConfigFile() {
  static OVR::String ovrBasePath = OVR::GetBaseOVRPath(false);
  static OVR::String ovrConfigFile = ovrBasePath + OVR::String("/profile2.json");
  static std::string configFile = (const char*)ovrConfigFile;
  return configFile;
}

Json::Value readConfig() {
  std::string file = getConfigFile();
  Json::Value result;
  if (Files::exists(file)) {
    Json::Reader reader;
    if (!reader.parse(Files::read(file), result)) {
      SAY_ERR("Failed to parse config %s", reader.getFormattedErrorMessages().c_str());
    }
  }
  return result;
}

void writeConfig(const Json::Value & value) {
  std::string file = getConfigFile();
  Json::StyledWriter writer;
  std::string json = writer.write(value);
  std::ofstream out(file);
  if (out) {
    out << json << std::endl;
    out.close();
  }
}

glm::quat Rift::getStrabismusCorrection() {
  Json::Value config = readConfig();
  Json::Value sc = config.get("StrabismusCorrection", Json::Value::null);
  if (sc.isNull()) {
    return glm::quat();
  }
  return parseQuaternion(sc);
}

void Rift::setStrabismusCorrection(const glm::quat & q) {
  Json::Value config = readConfig();
  config["StrabismusCorrection"]["x"] = q.x;
  config["StrabismusCorrection"]["y"] = q.y;
  config["StrabismusCorrection"]["z"] = q.z;
  config["StrabismusCorrection"]["w"] = q.w;
  writeConfig(config);
}

#else

glm::quat Rift::getStrabismusCorrection() {
  return glm::quat();
}

void Rift::setStrabismusCorrection(const glm::quat & q) {
}

#endif

void Rift::getDk1HmdValues(OVR::HMDInfo & hmdInfo) {
  hmdInfo.HResolution = 1280;
  hmdInfo.VResolution = 800;
  hmdInfo.HScreenSize = 0.14976f;
  hmdInfo.VScreenSize = 0.09360f;
  hmdInfo.VScreenCenter = 0.04680f;
  hmdInfo.EyeToScreenDistance = 0.04100f;
  hmdInfo.LensSeparationDistance = 0.06350f;
  hmdInfo.InterpupillaryDistance = 0.06400f;
  hmdInfo.DistortionK[0] = 1;
  // hmdInfo.DistortionK[1] = 0.18007f;
  hmdInfo.DistortionK[1] = 0.22f;
  // hmdInfo.DistortionK[2] = 0.11502f;
  hmdInfo.DistortionK[2] = 0.24f;
  hmdInfo.DistortionK[3] = 0;
  hmdInfo.DesktopX = 100;
  hmdInfo.DesktopY = 100;
  hmdInfo.ChromaAbCorrection[0] = 0.99600f;
  hmdInfo.ChromaAbCorrection[1] = -0.00400f;
  hmdInfo.ChromaAbCorrection[2] = 1.01400f;
  hmdInfo.ChromaAbCorrection[3] = 0;
}

void Rift::getHmdInfo(
  const OVR::Ptr<OVR::DeviceManager> & ovrManager,
  OVR::HMDInfo & out) {
  if (!ovrManager) {
    FAIL("Unable to create Rift device manager");
  }
  OVR::Ptr<OVR::HMDDevice> ovrHmd = *ovrManager->
      EnumerateDevices<OVR::HMDDevice>().CreateDevice();
  if (!ovrHmd) {
    getDk1HmdValues(out);
    return;
  }
  ovrHmd->GetDeviceInfo(&out);
  ovrHmd = nullptr;
}

// Fetch a glm style quaternion from an OVR sensor fusion object
glm::quat Rift::getQuaternion(OVR::SensorFusion & sensorFusion) {
  return glm::quat(getEulerAngles(sensorFusion));
}

// Fetch a glm vector containing Euler angles from an OVR sensor fusion object
glm::vec3 Rift::getEulerAngles(OVR::SensorFusion & sensorFusion) {
  return getEulerAngles(sensorFusion.GetPredictedOrientation());
}
/*
std::string Rift::formatOvrVector(const OVR::Vector3f & vector) {
  static char BUFFER[128];
  sprintf(BUFFER, "%+03.2f %+03.2f %+03.2f", vector.x, vector.y, vector.z);
  return std::string(BUFFER);
}
*/

glm::vec3 Rift::fromOvr(const OVR::Vector3f & in) {
  return glm::vec3(in.x, in.y, in.z);
}

glm::quat Rift::fromOvr(const OVR::Quatf & in) {
  return glm::quat(getEulerAngles(in));
}

glm::vec3 Rift::getEulerAngles(const OVR::Quatf & in) {
  glm::vec3 eulerAngles;
  in.GetEulerAngles<
      OVR::Axis_X, OVR::Axis_Y, OVR::Axis_Z,
      OVR::Rotate_CW, OVR::Handed_R
      >(&eulerAngles.x, &eulerAngles.y, &eulerAngles.z);
  return eulerAngles;
}


void Rift::getRiftPositionAndSize(
    const OVR::HMDInfo & hmdInfo,
    glm::ivec2 & windowPosition,
    glm::ivec2 & windowSize) {

  windowPosition = glm::ivec2(hmdInfo.DesktopX, hmdInfo.DesktopY);
  GLFWmonitor * hmdMonitor =
    GlfwApp::getMonitorAtPosition(windowPosition);

  if (!hmdMonitor) {
    FAIL("Unable to find Rift display");
  }

  const GLFWvidmode * videoMode =
    glfwGetVideoMode(hmdMonitor);
  windowSize = glm::ivec2(videoMode->width, videoMode->height);
}

RiftRenderApp::RiftRenderApp(bool fullscreen) :
fullscreen(fullscreen),
eyeAspect(1),
fov(60),
postDistortionScale(1),
eye({ { PerEyeArgs(*this, true), PerEyeArgs(*this, false) } })
{
  OVR::HMDInfo hmdInfo;
  Rift::getHmdInfo(ovrManager, hmdInfo);
  hmdNativeResolution = glm::ivec2(hmdInfo.HResolution, hmdInfo.VResolution);
  eyeAspect = glm::aspect(hmdNativeResolution) / 2.0f;
  eyeAspectInverse = 1.0f / eyeAspect;

  hmdDesktopPosition = glm::ivec2(hmdInfo.DesktopX, hmdInfo.DesktopY);
  hmdMonitor = GlfwApp::getMonitorAtPosition(hmdDesktopPosition);
  float lensDistance =
    hmdInfo.LensSeparationDistance /
    hmdInfo.HScreenSize;
  eye[0].lensOffset = 1.0f - (2.0f * lensDistance);
  eye[1].lensOffset = -eye[0].lensOffset;

  distortion = glm::vec4(
    hmdInfo.DistortionK[0],
    hmdInfo.DistortionK[1],
    hmdInfo.DistortionK[2],
    hmdInfo.DistortionK[3]);

  OVR::Util::Render::StereoConfig stereoConfig;
  stereoConfig.SetHMDInfo(hmdInfo);
  fov = stereoConfig.GetYFOVDegrees() * DEGREES_TO_RADIANS;
  postDistortionScale = 1.0f / stereoConfig.GetDistortionScale();

  {
    float projectionOffset = stereoConfig.GetProjectionCenterOffset();
    glm::vec3 v = glm::vec3(projectionOffset, 0, 0);
    eye[0].projectionOffset = glm::translate(glm::mat4(), v);
    eye[1].projectionOffset = glm::translate(glm::mat4(), -v);
  }

  {
    float modelviewOffset = stereoConfig.GetIPD() / 2.0f;
    glm::vec3 v = glm::vec3(modelviewOffset, 0, 0);
    eye[0].modelviewOffset = glm::translate(glm::mat4(), v);
    eye[1].modelviewOffset = glm::translate(glm::mat4(), -v);
  }

  {
    glm::quat strabismusCorrection = Rift::getStrabismusCorrection();
    eye[0].strabsimusCorrection = glm::mat4_cast(strabismusCorrection);
    eye[1].strabsimusCorrection = glm::mat4_cast(glm::inverse(strabismusCorrection));
  }
}


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
        60.0f, glm::aspect(windowSize),
        Rift::ZNEAR, Rift::ZFAR);
    // If we're not working stereo, we're just going to render the
    // scene once, from a single position, directly to the back buffer
    gl::viewport(windowSize);
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

void RiftApp::renderStringAt(const std::string & str, float x, float y) {
   gl::MatrixStack & mv = gl::Stacks::modelview();
  gl::MatrixStack & pr = gl::Stacks::projection();
  mv.push().identity();
  pr.push().top() = glm::ortho(
    -1.0f, 1.0f,
    -windowAspectInverse * 2.0f, windowAspectInverse * 2.0f,
    -100.0f, 100.0f);
  glm::vec2 cursor(x, windowAspectInverse * y);
  GlUtils::renderString(str, cursor, 18.0f);
  pr.pop();
  mv.pop();

}

void RiftApp::renderScene(const glm::mat4 & modelviewOffset) {
}
