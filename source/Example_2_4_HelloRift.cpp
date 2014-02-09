#include "Common.h"

using namespace OVR::Util::Render;

class HelloRift : public GlfwApp {
protected:
  OVR::Ptr<OVR::DeviceManager>  ovrManager;
  OVR::Ptr<OVR::SensorDevice>   ovrSensor;
  OVR::SensorFusion             sensorFusion;
  bool                          useTracker;

  gl::FrameBufferWrapper        frameBuffer;
  gl::GeometryPtr               quadGeometry;

  gl::ProgramPtr                distortProgram;
  glm::uvec2                    eyeSize;
  float                         eyeAspect;
  float                         ipd;
  struct EyeArg {
    RiftLookupTexturePtr        lookupTexture;
    glm::uvec2                  viewportLocation;
    glm::mat4                   projectionOffset;
    glm::mat4                   modelviewOffset;
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

    windowPosition = glm::ivec2(hmdInfo.DesktopX, hmdInfo.DesktopY);
    // The HMDInfo gives us the position of the Rift in desktop 
    // coordinates as well as the native resolution of the Rift 
    // display panel, but NOT the current resolution of the signal
    // being sent to the Rift.  
    GLFWmonitor * hmdMonitor = 
      GlfwApp::getMonitorAtPosition(windowPosition);
    if (!hmdMonitor) {
      FAIL("Unable to find Rift display");
    }

    // For the current resoltuion we must find the appropriate GLFW monitor
    const GLFWvidmode * videoMode = 
      glfwGetVideoMode(hmdMonitor);
    windowSize = glm::ivec2(videoMode->width, videoMode->height);

    // The eyeSize is used to help us set the viewport when rendering to 
    // each eye.  This should be based off the video mode that is / will 
    // be sent to the Rift
    // We also use the eyeSize to set up the framebuffer which will be 
    // used to render the scene to a texture for distortion and display 
    // on the Rift.  The Framebuffer resolution does not have to match 
    // the Physical display resolution in either aspect ratio or 
    // resolution, but using a resolution less than the native pixels can
    // negatively impact image quality.
    eyeSize = windowSize;
    eyeSize.x /= 2;

    eyeArgs[1].viewportLocation = glm::ivec2(eyeSize.x, 0);
    eyeArgs[0].viewportLocation = glm::ivec2(0, 0);

    // Notice that the eyeAspect we calculate is based on the physical 
    // display resolution, regardless of the current resolution being 
    // sent to the Rift.  The Rift scales the image sent to it to fit
    // the display panel, so a 1920x1080 image (with an aspect ratio of 
    // 16:9 will be displayed with the aspect ratio of the Rift display
    // (16:10 for the DK1).  This means that if you're cloning a 
    // 1920x1080 output to the rift and an conventional monitor of those 
    // dimensions the conventional monitor's image will appear a bit 
    // squished.  This is expected and correct.
    eyeAspect = (float)(hmdInfo.HResolution / 2) /
      (float)hmdInfo.VResolution;

    // Some of the values needed by the rendering or distortion need some 
    // calculation to find, but the OVR SDK includes a utility class to
    // do them, so we use it here to get the ProjectionOffset and the 
    // post distortion scale.
    OVR::Util::Render::StereoConfig stereoConfig;
    stereoConfig.SetHMDInfo(hmdInfo);

    // The projection offset and lens offset are both in normalized 
    // device coordinates, i.e. [-1, 1] on both the X and Y axis
    glm::vec3 projectionOffsetVector =
        glm::vec3(stereoConfig.GetProjectionCenterOffset() / 2.0f, 0, 0);
    eyeArgs[0].projectionOffset =
        glm::translate(glm::mat4(), projectionOffsetVector);
    eyeArgs[1].projectionOffset =
        glm::translate(glm::mat4(), -projectionOffsetVector);

    ipd = stereoConfig.GetIPD();
    // The IPD and the modelview offset are in meters.  If you wish to have a 
    // different unit for  the scale of your world coordinates, you would need 
    // to apply the conversion factor here.
    glm::vec3 modelviewOffsetVector =
        glm::vec3(ipd / 2.0f, 0, 0);
    eyeArgs[0].modelviewOffset =
        glm::translate(glm::mat4(), modelviewOffsetVector);
    eyeArgs[1].modelviewOffset =
        glm::translate(glm::mat4(), -modelviewOffsetVector);

    gl::Stacks::projection().top() = glm::perspective(
        stereoConfig.GetYFOVDegrees() * DEGREES_TO_RADIANS,
        eyeAspect,
        Rift::ZNEAR, Rift::ZFAR);
  }

  ~HelloRift() {
    sensorFusion.AttachToSensor(nullptr);
    ovrSensor = nullptr;
    ovrManager = nullptr;
  }

  virtual void createRenderingTarget() {
    glfwWindowHint(GLFW_DECORATED, 0);
    createWindow(windowSize, windowPosition);
    if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
      FAIL("Unable to create undecorated window");
    }
  }

  void initGl() {
    GlfwApp::initGl();
    frameBuffer.init(eyeSize);
    quadGeometry = GlUtils::getQuadGeometry();
    distortProgram = GlUtils::getProgram(
        Resource::SHADERS_TEXTURED_VS,
        Resource::SHADERS_RIFTWARP_FS);
    gl::Program::clear();

    OVR::HMDInfo hmdInfo;
    Rift::getHmdInfo(ovrManager, hmdInfo);
    RiftDistortionHelper helper(hmdInfo);
    for (int i = 0; i < 2; ++i) {
      eyeArgs[i].lookupTexture =
        helper.createLookupTexture(glm::uvec2(512, 512), i);
    }

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
        glm::vec3(0.0f, 0.0f, 0.2f);

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

      distortProgram->setUniform("Scene", 1);
      glActiveTexture(GL_TEXTURE1);
      frameBuffer.color->bind();

      distortProgram->setUniform("OffsetMap", 0);
      glActiveTexture(GL_TEXTURE0);
      eyeArg.lookupTexture->bind();

      quadGeometry->bindVertexArray();
      quadGeometry->draw();

      gl::Texture2d::unbind();
      gl::Geometry::unbindVertexArray();
      gl::Program::clear();
    }
  }

  virtual void renderScene(const EyeArg & eyeArg) {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::MatrixStack & pm = gl::Stacks::projection();
    gl::MatrixStack & mv = gl::Stacks::modelview();
    pm.push().preMultiply(eyeArg.projectionOffset);
    mv.push().preMultiply(eyeArg.modelviewOffset);
    {
      mv.push().translate(glm::vec3(0, 0, -1.5f)).
        rotate(glm::angleAxis(QUARTER_TAU, GlUtils::X_AXIS));
        GlUtils::draw3dGrid();
      mv.pop();
      mv.scale(ipd);
      GlUtils::drawColorCube();
    }
    mv.pop();
    pm.pop();
  }
};

RUN_OVR_APP(HelloRift);

