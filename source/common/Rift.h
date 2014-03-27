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

//#define RIFT_MULTISAMPLE 1

typedef OVR::Util::Render::StereoEye StereoEye;
#define LEFT StereoEye::StereoEye_Left
#define RIGHT StereoEye::StereoEye_Right

class Rift {
public:
  static void getDefaultDk1HmdValues(OVR::HMDInfo & hmdInfo);
  static void getRiftPositionAndSize(const OVR::HMDInfo & ovrHmdInfo,
      glm::ivec2 & windowPosition, glm::uvec2 & windowSize);

  static glm::quat getStrabismusCorrection();
  static void setStrabismusCorrection(const glm::quat & q);

  static void getHmdInfo(
    const OVR::Ptr<OVR::DeviceManager> & ovrManager,
    OVR::HMDInfo & ovrHmdInfoOut);

  // Fetch a glm style quaternion from an OVR sensor fusion object
  static glm::quat getQuaternion(OVR::SensorFusion & sensorFusion);

  // Fetch a glm style quaternion from an OVR sensor fusion object
  static glm::mat4 getMat4(OVR::SensorFusion & sensorFusion);


  // Fetch a glm vector containing Euler angles from an OVR sensor fusion object
  static glm::vec3 getEulerAngles(OVR::SensorFusion & sensorFusion);
  static glm::vec3 getEulerAngles(const OVR::Quatf & in);

  static glm::vec4 fromOvr(const OVR::Color & in);
  static glm::vec3 fromOvr(const OVR::Vector3f & in);
  static glm::quat fromOvr(const OVR::Quatf & in);

  static const float MONO_FOV;
  static const float FRAMEBUFFER_OBJECT_SCALE;
  static const float ZFAR;
  static const float ZNEAR;
};

typedef gl::Texture<GL_TEXTURE_2D, GL_RG16F> RiftLookupTexture;
typedef RiftLookupTexture::Ptr RiftLookupTexturePtr;

class RiftDistortionHelper {
  glm::dvec4 K{ 1, 0, 0, 0 };
  glm::dvec4 chromaK{ 1, 0, 1, 0 };
  double lensOffset{ 0 };
  double eyeAspect{ 0.06 };

  double getLensOffset(StereoEye eye) const;
  static glm::dvec2 screenToTexture(const glm::dvec2 & v);
  static glm::dvec2 textureToScreen(const glm::dvec2 & v);
  glm::dvec2 screenToRift(const glm::dvec2 & v, StereoEye eye) const;
  glm::dvec2 riftToScreen(const glm::dvec2 & v, StereoEye eye) const;
  glm::dvec2 textureToRift(const glm::dvec2 & v, StereoEye eye) const;
  glm::dvec2 riftToTexture(const glm::dvec2 & v, StereoEye eye) const;
  double getUndistortionScaleForRadiusSquared(double rSq) const;
  double getUndistortionScale(const glm::dvec2 & v) const;
  double getUndistortionScaleForRadius(double r) const;
  glm::dvec2 getUndistortedPosition(const glm::dvec2 & v) const;
  glm::dvec2 getTextureLookupValue(const glm::dvec2 & texCoord, StereoEye eye) const;
  double getDistortionScaleForRadius(double rTarget) const;
  glm::dvec2 findDistortedVertexPosition(const glm::dvec2 & source, StereoEye eye) const;

public:
  RiftDistortionHelper(const OVR::HMDInfo & hmdInfo);
  RiftLookupTexturePtr createLookupTexture(const glm::uvec2 & lookupTextureSize, StereoEye eye) const;
  gl::GeometryPtr createDistortionMesh(const glm::uvec2 & distortionMeshResolution, StereoEye eye) const;
};


class RiftManagerApp {
protected:
  OVR::Ptr<OVR::DeviceManager> ovrManager;
  OVR::HMDInfo ovrHmdInfo;
  glm::uvec2 hmdNativeResolution;
  glm::ivec2 hmdDesktopPosition;
  glm::uvec2 eyeSize;
  float eyeAspect{1};
  float eyeAspectInverse{1};

public:
  RiftManagerApp() {
    ovrManager = *OVR::DeviceManager::Create();
    Rift::getHmdInfo(ovrManager, ovrHmdInfo);
    hmdNativeResolution = glm::ivec2(ovrHmdInfo.HResolution, ovrHmdInfo.VResolution);
    eyeAspect = glm::aspect(hmdNativeResolution) / 2.0f;
    eyeAspectInverse = 1.0f / eyeAspect;
    hmdDesktopPosition = glm::ivec2(ovrHmdInfo.DesktopX, ovrHmdInfo.DesktopY);
    eyeSize = hmdNativeResolution;
    eyeSize.x /= 2;
  }
};

/**
A class that takes care of the basic duties of putting an OpenGL
window on the desktop in the correct position so that it's visible
through the Rift.
*/
class RiftGlfwApp : public GlfwApp, public RiftManagerApp {
protected:
  GLFWmonitor * hmdMonitor;
  const bool fullscreen;
  bool fakeRiftMonitor{ false };

public:
  RiftGlfwApp(bool fullscreen = false) : fullscreen(fullscreen)
  {
    // Attempt to find the Rift monitor.
    hmdMonitor = GlfwApp::getMonitorAtPosition(hmdDesktopPosition);
    if (!hmdMonitor) {
      SAY_ERR("No Rift display found.  Looking for alternate display");
      fakeRiftMonitor = true;
      // Try to find the best monitor that isn't the primary display.
      GLFWmonitor * primaryMonitor = glfwGetPrimaryMonitor();
      int monitorCount;
      GLFWmonitor ** monitors = glfwGetMonitors(&monitorCount);
      for (int i = 0; i < monitorCount; ++i) {
        GLFWmonitor * monitor = monitors[i];
        if (monitors[i] != primaryMonitor) {
          hmdMonitor = monitors[i];
          break;
        }
      }
      // No joy, use the primary monitor
      if (!hmdMonitor) {
        hmdMonitor = primaryMonitor;
      }
    }

    if (!hmdMonitor) {
      FAIL("Somehow failed to find any output display ");
    }

    const GLFWvidmode * videoMode = glfwGetVideoMode(hmdMonitor);
    if (!fakeRiftMonitor || fullscreen) {
      // if we've got a real rift monitor, OR we're doing fullscreen with 
      // a fake Rift, use the resolution of the monitor
      windowSize = glm::uvec2(videoMode->width, videoMode->height);
    } else {
      // If we've got a fake rift and we're NOT fullscreen, 
      // use the DK1 resolution
      windowSize = glm::uvec2(1280, 800);
    }

    // if we're using a fake rift 
    if (fakeRiftMonitor) {
      int fakex, fakey;
      // Reset the desktop display's position to the target monitor
      glfwGetMonitorPos(hmdMonitor, &fakex, &fakey);
      hmdDesktopPosition = glm::ivec2(fakex, fakey);
      // on a large display, try to center the fake Rift display.
      if (videoMode->width > windowSize.x) {
        hmdDesktopPosition.x += (videoMode->width - windowSize.x) / 2;
      }
      if (videoMode->height > windowSize.y) {
        hmdDesktopPosition.y += (videoMode->height - windowSize.y) / 2;
      }
    }


    eyeSize = windowSize;
    eyeSize.x /= 2;
  }

  virtual void createRenderingTarget() {
#ifdef RIFT_MULTISAMPLE
    glfwWindowHint(GLFW_SAMPLES, 4);
#endif

    if (fullscreen) {
      // Fullscreen apps should use the native resolution of the Rift
      this->createFullscreenWindow(hmdNativeResolution, hmdMonitor);
    } else {
      glfwWindowHint(GLFW_DECORATED, 0);
      createWindow(windowSize, hmdDesktopPosition);
      if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
        FAIL("Unable to create undecorated window");
      }
    }
  }

  virtual void viewport(StereoEye eye) {
    glm::uvec2 viewportPosition(eye == LEFT ? 0 : eyeSize.x, 0);
    gl::viewport(viewportPosition, eyeSize);
  }


  void leftEyeViewport() {
    viewport(LEFT);
  }

  void rightEyeViewport() {
    viewport(RIGHT);
  }
};

struct RiftPerEyeArg {
  StereoEye eye;
  glm::mat4 strabsimusCorrection;
  glm::mat4 projectionOffset;
  glm::mat4 modelviewOffset;
  glm::uvec2 viewportPosition;
  RiftLookupTexturePtr distortionTexture;

  RiftPerEyeArg(StereoEye eye)
    : eye(eye) { }
};

class RiftApp : public RiftGlfwApp {
public:

protected:
  OVR::SensorFusion sensorFusion;
  glm::mat4 player;
  glm::mat4 riftOrientation;

private:
  float distortionScale;
  OVR::Ptr<OVR::SensorDevice> ovrSensor;
  gl::GeometryPtr quadGeometry;

#ifdef RIFT_MULTISAMPLE
  gl::MultisampleFrameBufferWrapper frameBuffer;
#else
  gl::FrameBufferWrapper frameBuffer;
#endif

  gl::ProgramPtr distortProgram;
  std::array<RiftPerEyeArg, 2> eyes{ { RiftPerEyeArg(LEFT), RiftPerEyeArg(RIGHT) } };

protected:
  void renderStringAt(const std::string & str, float x, float y);

  virtual void createRenderingTarget();
  virtual void initGl();
  virtual void onKey(int key, int scancode, int action, int mods);
  virtual void draw() final;
  virtual void update();
  virtual void renderScene() = 0;

public:
  RiftApp(bool fullscreen = false);
  virtual ~RiftApp();
};

template <typename Function>
void for_each_eye(Function function) {
  for (StereoEye eye = StereoEye::StereoEye_Left;
      eye <= StereoEye::StereoEye_Right;
      eye = static_cast<StereoEye>(eye + 1)) {
    function(eye);
  }
}

// Combine some macros together to create a single macro
// to launch a class containing a run method
#define RUN_OVR_APP(AppClass) \
    MAIN_DECL { \
        OVR::System::Init(); \
        try { \
            return AppClass().run(); \
        } catch (std::exception & error) { \
            SAY_ERR(error.what()); \
        } catch (std::string & error) { \
            SAY_ERR(error.c_str()); \
        } \
        OVR::System::Destroy(); \
        return -1; \
    }

