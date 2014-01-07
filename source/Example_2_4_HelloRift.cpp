#include "Common.h"

using namespace OVR::Util::Render;

class HelloRift : public GlfwApp {
protected:
  OVR::Ptr<OVR::DeviceManager>  ovrManager;
  OVR::Ptr<OVR::SensorDevice>   ovrSensor;
  OVR::SensorFusion             sensorFusion;
  bool                          useTracker;

//  OVR::HMDInfo                  hmdInfo;
//  StereoMode                    renderMode;

  glm::ivec2                    screenPosition;
  glm::ivec2                    screenSize;

  gl::FrameBufferWrapper        frameBuffer;
  gl::GeometryPtr               quadGeometry;

  gl::ProgramPtr                distortProgram;
  glm::ivec2                    eyeSize;
  float                         eyeAspect;
  glm::vec4                     distortionCoefficients;
  float                         postDistortionScale;

  struct EyeArg {
    glm::ivec2                  viewportLocation;
    glm::mat4                   projectionOffset;
    glm::mat4                   modelviewOffset;
    float                       lensOffset;
  } eyeArgs[2];

public:
  HelloRift() : useTracker(false) {
    ovrManager = *OVR::DeviceManager::Create();
    if (!ovrManager) {
      FAIL("Unable to initialize OVR Device Manager");
    }

    OVR::Ptr<OVR::HMDDevice> ovrHmd =
        *ovrManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
    OVR::HMDInfo hmdInfo;
    if (ovrHmd) {
      ovrHmd->GetDeviceInfo(&hmdInfo);
      ovrSensor = *ovrHmd->GetSensor();
    } else {
      Rift::getDk1HmdValues(hmdInfo);
    }
    ovrHmd.Clear();

    if (!ovrSensor) {
      ovrSensor =
          *ovrManager->EnumerateDevices<OVR::SensorDevice>().CreateDevice();
    }

    if (ovrSensor) {
      sensorFusion.AttachToSensor(ovrSensor);
      useTracker = sensorFusion.IsAttachedToSensor();
    }

    screenSize = glm::ivec2(
        hmdInfo.HResolution,
        hmdInfo.VResolution);
    screenPosition = glm::ivec2(
      hmdInfo.DesktopX,
      hmdInfo.DesktopY);
    eyeSize = glm::ivec2(
        hmdInfo.HResolution / 2.0f,
        hmdInfo.VResolution);

    OVR::Util::Render::StereoConfig stereoConfig;
    stereoConfig.SetHMDInfo(hmdInfo);
    postDistortionScale = 1.0f / stereoConfig.GetDistortionScale();
    distortionCoefficients = glm::vec4(
        hmdInfo.DistortionK[0], hmdInfo.DistortionK[1],
        hmdInfo.DistortionK[2], hmdInfo.DistortionK[3]);


    eyeArgs[0].viewportLocation = glm::ivec2(0, 0);
    eyeArgs[1].viewportLocation = glm::ivec2(eyeSize.x, 0);

    glm::vec3 projectionOffsetVector =
        glm::vec3(stereoConfig.GetProjectionCenterOffset() / 2.0f, 0, 0);
    eyeArgs[0].projectionOffset =
        glm::translate(glm::mat4(), projectionOffsetVector);
    eyeArgs[1].projectionOffset =
        glm::translate(glm::mat4(), -projectionOffsetVector);

    glm::vec3 modelviewOffsetVector =
        glm::vec3(stereoConfig.GetIPD() / 2.0f, 0, 0);
    eyeArgs[0].modelviewOffset =
        glm::translate(glm::mat4(), modelviewOffsetVector);
    eyeArgs[1].modelviewOffset =
        glm::translate(glm::mat4(), -modelviewOffsetVector);

    eyeArgs[0].lensOffset =
        1.0f - (2.0f * hmdInfo.LensSeparationDistance / hmdInfo.HScreenSize);
    eyeArgs[1].lensOffset = -eyeArgs[0].lensOffset;

    eyeAspect = (float) eyeSize.x / (float) eyeSize.y;

    gl::Stacks::projection().top() = glm::perspective(
        stereoConfig.GetYFOVDegrees(),
        (float) eyeSize.x / (float) eyeSize.y,
        Rift::ZNEAR, Rift::ZFAR);
  }

  ~HelloRift() {
    sensorFusion.AttachToSensor(nullptr);
    ovrSensor = nullptr;
    ovrManager = nullptr;
  }

  virtual void createRenderingTarget() {
    glfwWindowHint(GLFW_DECORATED, 0);
    createWindow(screenSize, screenPosition);
    if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
      FAIL("Unable to create undecorated window");
    }
  }

  void initGl() {
    GlfwApp::initGl();
    frameBuffer.init(eyeSize);
    quadGeometry = GlUtils::getQuadGeometry();

    distortProgram = GlUtils::getProgram(
        Resource::SHADERS_VERTEXTORIFT_VS,
        Resource::SHADERS_DIRECTDISTORT_FS);
    distortProgram->use();
    distortProgram->setUniform("Aspect", eyeAspect);
    distortProgram->setUniform("PostDistortionScale", postDistortionScale);
    distortProgram->setUniform("K", distortionCoefficients);
    gl::Program::clear();
  }

  void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action) {
      return;
    }

    switch (key) {
    case GLFW_KEY_R:
      sensorFusion.Reset();
      break;

    case GLFW_KEY_T:
      useTracker = !useTracker;
      break;

    default:
      GlfwApp::onKey(key, scancode, action, mods);
      break;
    }
  }

  virtual void update() {
    static const glm::vec3 CAMERA =
        glm::vec3(0.0f, 0.0f, 0.4f);

    glm::mat4 & modelview = gl::Stacks::modelview().top();
    modelview = glm::lookAt(CAMERA, GlUtils::ORIGIN, GlUtils::Y_AXIS);

    if (useTracker) {
      glm::quat riftOrientation = Rift::getQuaternion(sensorFusion);
      modelview = modelview * glm::mat4_cast(riftOrientation);
    } else {
      static const float Y_ROTATION_RATE = 0.01f;
      static const float Z_ROTATION_RATE = 0.05f;
      time_t now = Platform::elapsedMillis();
      modelview = glm::rotate(modelview, now * Y_ROTATION_RATE,
          GlUtils::Y_AXIS);
      modelview = glm::rotate(modelview, now * Z_ROTATION_RATE,
          GlUtils::Z_AXIS);
    }
  }

  void draw() {
    glClearColor(0, 1, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    GL_CHECK_ERROR;

    for (int i = 0; i < 2; ++i) {
      const EyeArg & eyeArg = eyeArgs[i];

      frameBuffer.activate();
      renderScene(eyeArg);
      frameBuffer.deactivate();

      glDisable(GL_DEPTH_TEST);
      glDisable(GL_BLEND);

      gl::viewport(eyeArg.viewportLocation, eyeSize);
      distortProgram->use();
      distortProgram->setUniform("LensOffset", eyeArg.lensOffset);
      frameBuffer.color->bind();
      quadGeometry->bindVertexArray();
      quadGeometry->draw();
      gl::Geometry::unbindVertexArray();
      gl::Program::clear();
    }
  }

  virtual void renderScene(const EyeArg & eyeArg) {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::MatrixStack & pm = gl::Stacks::projection();
    pm.push();
      pm.preMultiply(eyeArg.projectionOffset);
      gl::MatrixStack & mv = gl::Stacks::modelview();
      mv.push();
        mv.preMultiply(eyeArg.modelviewOffset);
        mv.scale(0.15f);
        GlUtils::drawColorCube();
      mv.pop();
    pm.pop();
  }
};

RUN_OVR_APP(HelloRift);

