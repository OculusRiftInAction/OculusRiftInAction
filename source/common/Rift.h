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

enum Eye {
  LEFT, RIGHT
};

class Rift {
public:
  static Eye EYES[];
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

  static glm::vec4 Rift::fromOvr(const OVR::Color & in);
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

  double getLensOffset(Eye eye) const;
  static glm::dvec2 screenToTexture(const glm::dvec2 & v);
  static glm::dvec2 textureToScreen(const glm::dvec2 & v);
  glm::dvec2 screenToRift(const glm::dvec2 & v, Eye eye) const;
  glm::dvec2 riftToScreen(const glm::dvec2 & v, Eye eye) const;
  glm::dvec2 textureToRift(const glm::dvec2 & v, Eye eye) const;
  glm::dvec2 riftToTexture(const glm::dvec2 & v, Eye eye) const;
  double getUndistortionScaleForRadiusSquared(double rSq) const;
  double getUndistortionScale(const glm::dvec2 & v) const;
  double getUndistortionScaleForRadius(double r) const;
  glm::dvec2 getUndistortedPosition(const glm::dvec2 & v) const;
  glm::dvec2 getTextureLookupValue(const glm::dvec2 & texCoord, Eye eye) const;
  double getDistortionScaleForRadius(double rTarget) const;
  glm::dvec2 findDistortedVertexPosition(const glm::dvec2 & source, Eye eye) const;

public:
  RiftDistortionHelper(const OVR::HMDInfo & hmdInfo);
  RiftLookupTexturePtr createLookupTexture(const glm::uvec2 & lookupTextureSize, Eye eye) const;
  gl::GeometryPtr createDistortionMesh(const glm::uvec2 & distortionMeshResolution, Eye eye) const;
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

public:
  RiftGlfwApp(bool fullscreen = false) : fullscreen(fullscreen)
  {
    hmdMonitor = GlfwApp::getMonitorAtPosition(hmdDesktopPosition);
    if (hmdMonitor && !fullscreen) {
      const GLFWvidmode * videoMode = glfwGetVideoMode(hmdMonitor);
      eyeSize = glm::uvec2(videoMode->width, videoMode->height);
      eyeSize.x /= 2;
    }
  }

  virtual void createRenderingTarget() {
#ifdef RIFT_MULTISAMPLE
    glfwWindowHint(GLFW_SAMPLES, 4);
#endif

    if (fullscreen) {
      // Fullscreen apps should use the native resolution of the Rift
      this->createFullscreenWindow(hmdNativeResolution, hmdMonitor);
    } else {
      const GLFWvidmode * videoMode = glfwGetVideoMode(hmdMonitor);
      glfwWindowHint(GLFW_DECORATED, 0);
      createWindow(glm::uvec2(videoMode->width, videoMode->height), hmdDesktopPosition);
      if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
        FAIL("Unable to create undecorated window");
      }
    }
  }

  virtual void viewport(int eyeIndex) {
    glm::uvec2 viewportPosition(eyeIndex == 0 ? 0 : eyeSize.x, 0);
    gl::viewport(viewportPosition, eyeSize);
  }

  virtual void viewport(Eye eye) {
    viewport((int)eye);
  }

  void leftEyeViewport() {
    viewport(LEFT);
  }

  void rightEyeViewport() {
    viewport(RIGHT);
  }
};

struct RiftPerEyeArg {
  Eye eye;
  glm::mat4 strabsimusCorrection;
  glm::mat4 projectionOffset;
  glm::mat4 modelviewOffset;
  glm::uvec2 viewportPosition;
  RiftLookupTexturePtr distortionTexture;

  RiftPerEyeArg(Eye eye)
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

#define FOR_EACH_EYE(eye) \
for (Eye eye = LEFT; eye <= RIGHT; \
  eye = static_cast<Eye>(eye + 1))

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

