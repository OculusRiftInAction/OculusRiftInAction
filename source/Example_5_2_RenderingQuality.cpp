#include <OVR.h>
#include "Common.h"

using namespace glm;
using namespace OVR;
using namespace OVR::Util::Render;

class RenderingQuality : public GlfwApp {
protected:
  Ptr<DeviceManager> ovrManager;
  Ptr<SensorDevice> ovrSensor;
  // Provides the resolution and location of the Rift
  HMDInfo hmdInfo;

  // Oculus utility class for calculating some values
  StereoConfig stereoConfig;
  // Are we rendering in stereo or mono?
  StereoMode renderMode;
  // Class that attaches to the head tracker sensor object and
  // turns the stream of acceleration and rotation information
  // into a single quaternion.  Includes features like magnetic
  // yaw correction, gravity based pitch correction and prediction
  SensorFusion * sensorFusion;

  // Calculated width and height of the per-eye rendering area used
  ivec2 eyeSize;

  // A wrapper class containing a GL framebuffer (an offscreen
  // target to which OpenGL can render just like it can to a
  // screen).
  gl::FrameBufferWrapper frameBuffer;

  // OpenGL buffers to store geometry for the rectangle we will use
  // to render the distorted view of the scene, as required by the
  // Rift
  gl::GeometryPtr quadGeometry;

  gl::ProgramPtr distortProgram;

  // Compute the modelview and projection matrices for the rendered scene based on the eye and
  // whether or not we're doing side by side or rift rendering
  vec3 eyeProjectionOffset;
  vec3 eyeModelviewOffset;

  bool useTracker;

public:
  RenderingQuality()
      :
          renderMode(Util::Render::Stereo_LeftRight_Multipass),
          useTracker(false)  {
    sensorFusion = new SensorFusion();
    sensorFusion->SetGravityEnabled(false);
    sensorFusion->SetPredictionEnabled(false);
    sensorFusion->SetYawCorrectionEnabled(false);

    ///////////////////////////////////////////////////////////////////////////
    // Initialize Oculus VR SDK and hardware
    ovrManager = *DeviceManager::Create();
    if (ovrManager) {
      ovrSensor =
          *ovrManager->EnumerateDevices<SensorDevice>().CreateDevice();

      if (ovrSensor) {
        sensorFusion->AttachToSensor(ovrSensor);
      }

      Ptr<HMDDevice> ovrHmd = *ovrManager->EnumerateDevices<
          HMDDevice>().CreateDevice();
      if (ovrHmd) {
        ovrHmd->GetDeviceInfo(&hmdInfo);
      } else {
        Rift::getDk1HmdValues(hmdInfo);
      }
      // The HMDInfo structure contains everything we need for now, so no
      // need to keep the device handle around
      ovrHmd.Clear();
    }

    useTracker = sensorFusion->IsAttachedToSensor();

    stereoConfig.SetHMDInfo(hmdInfo);
    eyeSize = ivec2(hmdInfo.HResolution / 2, hmdInfo.VResolution);

    // Compute the offsets for the modelview and projection matrices
    eyeProjectionOffset = vec3(
        stereoConfig.GetProjectionCenterOffset() / 2.0f, 0, 0);
    eyeModelviewOffset = vec3(stereoConfig.GetIPD() / 2.0f, 0, 0);

    gl::Stacks::projection().top() = perspective(
        stereoConfig.GetYFOVDegrees(),
        (float) eyeSize.x / (float) eyeSize.y,
        Rift::ZNEAR, Rift::ZFAR);
  }

  ~RenderingQuality() {
    sensorFusion->AttachToSensor(nullptr);
    ovrSensor = nullptr;
    ovrManager = nullptr;
  }

  virtual void createRenderingTarget() {
    glfwWindowHint(GLFW_DECORATED, 0);
    createWindow(hmdInfo.HResolution, hmdInfo.VResolution, hmdInfo.DesktopX,
        hmdInfo.DesktopY);
    if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
      FAIL("Unable to create undecorated window");
    }
    int samples;
    glGetIntegerv(GL_SAMPLES, &samples);
    SAY("%d", samples);
  }

  void initGl() {
    ///////////////////////////////////////////////////////////////////////////
    // Initialize OpenGL settings and variables
    // Anti-alias lines (hopefully)
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // Allocate the frameBuffer that will hold the scene, and then be
    // re-rendered to the screen with distortion
    frameBuffer.init(eyeSize * 2);

    // Create the rendering shaders
    distortProgram = GlUtils::getProgram(Resource::SHADERS_VERTEXTORIFT_VS,
        Resource::SHADERS_DIRECTDISTORT_FS);

    // If we get here, we're rendering in stereo, so we have to render our output twice

    // Create the buffers for the texture quad we will draw
    quadGeometry = GlUtils::getQuadGeometry();

    // The distortion shader uses a number of input values that depend on
    // the exact Rift hardware, but which don't change from frame to frame.
    // It's sufficient therefore to compute them once here, and set them
    // in the GL shader program and then leave them alone.

    // The (per-eye) aspect ratio of the GL viewport
    float aspect = glm::aspect(eyeSize);
    // The offset of the lens from the center of the GL viewport,
    // expressed in  the default OpenGL coordinate system.  This value
    // Is for the left eye, since the offset is negative for the right
    // eye.  However, the shader is passed a flag that lets it know to
    // flip the value when rendering for the right eye.  This works as
    // long as the physical is centered laterally
    vec2 lensOffset = vec2(
        1.0f - (2.0 * hmdInfo.LensSeparationDistance / hmdInfo.HScreenSize),
        0);

    float distortionPostScale = stereoConfig.GetDistortionScale();
    vec4 K = vec4(hmdInfo.DistortionK[0], hmdInfo.DistortionK[1],
        hmdInfo.DistortionK[2], hmdInfo.DistortionK[3]);

    distortProgram->use();
    distortProgram->setUniform2f("LensCenter", lensOffset);
    distortProgram->setUniform("Aspect", aspect);
    distortProgram->setUniform("DistortionScale", distortionPostScale);
    distortProgram->setUniform4f("K", K);
    gl::Program::clear();
  }

  void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action) {
      return;
    }

    switch (key) {
    case GLFW_KEY_R:
      sensorFusion->Reset();
      break;

    case GLFW_KEY_P:
      if (renderMode == Util::Render::Stereo_None) {
        renderMode = Util::Render::Stereo_LeftRight_Multipass;
        gl::Stacks::projection().top() = perspective(
            stereoConfig.GetYFOVDegrees(),
            (float) eyeSize.x / (float) eyeSize.y, Rift::ZNEAR, Rift::ZFAR);

      } else {
        renderMode = Util::Render::Stereo_None;
        gl::Stacks::projection().top() = perspective(Rift::MONO_FOV,
            (float) hmdInfo.HResolution / (float) hmdInfo.VResolution,
            Rift::ZNEAR, Rift::ZFAR);
      }
      break;

    case GLFW_KEY_T:
      useTracker = !useTracker;
      break;

    default:
      GlfwApp::onKey(key, scancode, action, mods);
      break;
    }
  }

  void draw() {
    if (renderMode == Util::Render::Stereo_None) {
      // If we're not using stereo, we're just going to render the
      // scene once, from a single position, directly to the back buffer
      glViewport(0, 0, hmdInfo.HResolution, hmdInfo.VResolution);
      renderScene();
    } else {
      // If we get here, we're rendering in stereo, so we have to render our output twice

      // We have to explicitly clear the screen here.  the Clear command doesn't object the viewport
      // and the clear command inside renderScene will only target the active framebuffer object.
      glClearColor(0, 1, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);

      for (StereoEye eye = StereoEye_Left; eye <= StereoEye_Right; eye =
          static_cast<StereoEye>((eye + 1))) {
        bool leftEye = (eye == StereoEye_Left);

        // Update the matrix stacks to account for the different points of view
        // and the different projections for each eye.
        {
          float transform = (eye == StereoEye_Left ? 1.0f : -1.0f);

          // Activate the frame buffer so that the results of our
          // renderScene call will be sent to the texture attached
          // to the buffer
          frameBuffer.activate();
          renderScene(eyeProjectionOffset * transform,
              eyeModelviewOffset * transform);
          frameBuffer.deactivate();
        }

        ///////////////////////////////////////////////////////////////////////////
        //
        // Now render the texture containing the scene to the physical display
        //

        // We're not doing anything but rendering a single quad to the full viewport
        // so we don't need any depth or alpha testing.  If we didn't care about
        // being able to switch to non-stereo mode, we could create our primary
        // context without any depth buffer.
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        // Setup the viewport for the eye to which we're rendering.  This code
        // Adds a small frame that lets you see the background clear color.
        glViewport(1 + (leftEye ? 0 : eyeSize.x), 1, eyeSize.x - 2,
            eyeSize.y - 2);
        // Now activate the distortion program and render the textured quad
        {
          // Enable the Rift distortion shader
          distortProgram->use();
          // Tell it which eye we're using
          distortProgram->setUniform1i("Mirror", leftEye ? 0 : 1);
          // Bind the framebuffer texture containing the scene
          frameBuffer.color->bind();
          quadGeometry->bindVertexArray();
          quadGeometry->draw();
          gl::Geometry::unbindVertexArray();
          gl::Program::clear();
        } // distortion program use
      } // for each eye
    } // if stereo
  }

  virtual void update() {
    static const vec3 CAMERA = vec3(0.0f, 0.0f, 0.4f);
    static const vec3 ORIGIN = vec3(0.0f, 0.0f, 0.0f);
    static const vec3 UP = GlUtils::Y_AXIS;
    mat4 & modelview = gl::Stacks::modelview().top();
    time_t now = Platform::elapsedMillis();
    if (useTracker) {
      modelview = lookAt(CAMERA, ORIGIN, UP)
          * mat4_cast(Rift::getQuaternion(*sensorFusion));
    } else {
      static const float Y_ROTATION_RATE = 0.01f;
      static const float Z_ROTATION_RATE = 0.05f;
      modelview = lookAt(CAMERA, ORIGIN, UP);
      modelview = rotate(modelview, now * Y_ROTATION_RATE,
          GlUtils::Y_AXIS);
      modelview = rotate(modelview, now * Z_ROTATION_RATE,
          GlUtils::Z_AXIS);
    }
  }

  virtual void renderScene(const vec3 & projectionOffset = vec3(),
      const vec3 & modelviewOffset = vec3()) {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::Stacks::projection().push().preMultiply(
        translate(mat4(), projectionOffset));
    gl::Stacks::modelview().push().preMultiply(
        translate(mat4(), projectionOffset));

    gl::Stacks::modelview().push().scale(0.15f);
    GlUtils::drawColorCube();
    gl::Stacks::modelview().pop();

    gl::Stacks::modelview().pop();
    gl::Stacks::projection().pop();
    assert(gl::Stacks::modelview().size() == 1);
    assert(gl::Stacks::projection().size() == 1);
  }
};

RUN_OVR_APP(RenderingQuality);

