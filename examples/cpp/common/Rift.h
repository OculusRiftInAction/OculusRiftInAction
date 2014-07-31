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

class Rift {
public:
//  static void getDefaultDk1HmdValues(ovrHmd hmd, ovrHmdDesc & ovrHmdInfo);
  static void getRiftPositionAndSize(ovrHmd hmd,
      glm::ivec2 & windowPosition, glm::uvec2 & windowSize);
  static glm::quat getStrabismusCorrection();
  static void setStrabismusCorrection(const glm::quat & q);
  static void getHmdInfo(ovrHmd hmd, ovrHmdDesc & ovrHmdInfo);
  static glm::mat4 getMat4(ovrHmd hmd);

  static inline glm::mat4 fromOvr(const ovrFovPort & fovport, float nearPlane = 0.01f, float farPlane = 10000.0f) {
    return fromOvr(ovrMatrix4f_Projection(fovport, nearPlane, farPlane, true));
  }

  static inline glm::mat4 fromOvr(const ovrMatrix4f & om) {
    return glm::transpose(glm::make_mat4(&om.M[0][0]));
  }

  static inline glm::vec3 fromOvr(const ovrVector3f & ov) {
    return glm::make_vec3(&ov.x);
  }

  static inline glm::vec2 fromOvr(const ovrVector2f & ov) {
    return glm::make_vec2(&ov.x);
  }

  static inline glm::uvec2 fromOvr(const ovrSizei & ov) {
    return glm::uvec2(ov.w, ov.h);
  }

  static inline glm::quat fromOvr(const ovrQuatf & oq) {
    return glm::make_quat(&oq.x);
  }

  static inline glm::mat4 fromOvr(const ovrPosef & op) {
    glm::mat4 orientation = glm::mat4_cast(fromOvr(op.Orientation));
    glm::mat4 translation = glm::translate(glm::mat4(), Rift::fromOvr(op.Position));
    return translation * orientation;
    //  return glm::mat4_cast(fromOvr(op.Orientation)) * glm::translate(glm::mat4(), Rift::fromOvr(op.Position));
  }

  static inline ovrMatrix4f toOvr(const glm::mat4 & m) {
    ovrMatrix4f result;
    glm::mat4 transposed(glm::transpose(m));
    memcpy(result.M, &(transposed[0][0]), sizeof(float) * 16);
    return result;
  }

  static inline ovrVector3f toOvr(const glm::vec3 & v) {
    ovrVector3f result;
    result.x = v.x;
    result.y = v.y;
    result.z = v.z;
    return result;
  }

  static inline ovrVector2f toOvr(const glm::vec2 & v) {
    ovrVector2f result;
    result.x = v.x;
    result.y = v.y;
    return result;
  }

  static inline ovrSizei toOvr(const glm::uvec2 & v) {
    ovrSizei result;
    result.w = v.x;
    result.h = v.y;
    return result;
  }

  static inline ovrQuatf toOvr(const glm::quat & q) {
    ovrQuatf result;
    result.x = q.x;
    result.y = q.y;
    result.z = q.z;
    result.w = q.w;
    return result;
  }

};

typedef gl::Texture<GL_TEXTURE_2D, GL_RG16F> RiftLookupTexture;
typedef RiftLookupTexture::Ptr RiftLookupTexturePtr;

class RiftManagerApp {
protected:
  ovrHmd hmd;

  glm::uvec2 hmdNativeResolution;
  glm::ivec2 hmdDesktopPosition;

public:
  RiftManagerApp(ovrHmdType defaultHmdType = ovrHmd_DK1) {
    hmd = ovrHmd_Create(0);
    if (NULL == hmd) {
      hmd = ovrHmd_CreateDebug(defaultHmdType);
    }
    hmdNativeResolution = glm::ivec2(hmd->Resolution.w, hmd->Resolution.h);
    hmdDesktopPosition = glm::ivec2(hmd->WindowsPos.x, hmd->WindowsPos.y);
  }

  virtual ~RiftManagerApp() {
    ovrHmd_Destroy(hmd);
    hmd = nullptr;
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
      if (videoMode->width > (int)windowSize.x) {
        hmdDesktopPosition.x += (videoMode->width - windowSize.x) / 2;
      }
      if (videoMode->height > (int)windowSize.y) {
        hmdDesktopPosition.y += (videoMode->height - windowSize.y) / 2;
      }
    }
  }

  virtual void createRenderingTarget() {

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

  virtual ~RiftGlfwApp() {
  }

  virtual void viewport(ovrEyeType eye) {
    glm::uvec2 viewportPosition(eye == ovrEye_Left ? 0 : windowSize.x / 2, 0);
    gl::viewport(viewportPosition, glm::uvec2(windowSize.x / 2, windowSize.y));
  }


  void leftEyeViewport() {
    viewport(ovrEye_Left);
  }

  void rightEyeViewport() {
    viewport(ovrEye_Right);
  }
};

class RiftApp : public RiftGlfwApp {
public:

protected:
  glm::mat4 player;
  ovrPosef  headPose;
  ovrTexture eyeTextures[2];

private:
  ovrEyeRenderDesc eyeRenderDescs[2];
  gl::FrameBufferWrapper frameBuffers[2];
  glm::mat4 projections[2];
  glm::mat4 orthoProjections[2];
  ovrEyeType currentEye;

protected:
  void renderStringAt(const std::string & str, float x, float y);
  virtual void initGl();
  virtual void finishFrame();
  virtual void onKey(int key, int scancode, int action, int mods);
  virtual void draw() final;
  virtual void postDraw() {};
  virtual void update();
  virtual void renderScene() = 0;



  inline ovrEyeType getCurrentEye() const {
    return currentEye;
  }

  const ovrEyeRenderDesc & getEyeRenderDesc(ovrEyeType eye) const {
    return eyeRenderDescs[eye];
  }

  const ovrFovPort & getFov(ovrEyeType eye) const {
    return eyeRenderDescs[eye].Fov;
  }

  const glm::mat4 & getPerspectiveProjection(ovrEyeType eye) const {
    return projections[eye];
  }

  const glm::mat4 & getOrthographicProjection(ovrEyeType eye) const {
    return orthoProjections[eye];
  }

  const ovrFovPort & getFov() const {
    return getFov(getCurrentEye());
  }

  const ovrEyeRenderDesc & getEyeRenderDesc() const {
    return getEyeRenderDesc(getCurrentEye());
  }

  const glm::mat4 & getPerspectiveProjection() const {
    return getPerspectiveProjection(getCurrentEye());
  }

  const glm::mat4 & getOrthographicProjection() const {
    return getOrthographicProjection(getCurrentEye());
  }

public:
  RiftApp(bool fullscreen = false);
  virtual ~RiftApp();
};

template <typename Function>
void for_each_eye(Function function) {
  for (ovrEyeType eye = ovrEyeType::ovrEye_Left;
      eye < ovrEyeType::ovrEye_Count;
      eye = static_cast<ovrEyeType>(eye + 1)) {
    function(eye);
  }
}

// Combine some macros together to create a single macro
// to launch a class containing a run method
#define RUN_OVR_APP(AppClass) \
MAIN_DECL { \
  if (!ovr_Initialize()) { \
      SAY_ERR("Failed to initialize the Oculus SDK"); \
      return -1; \
  } \
  int result = -1; \
  try { \
    result = AppClass().run(); \
  } catch (std::exception & error) { \
    SAY_ERR(error.what()); \
  } catch (std::string & error) { \
    SAY_ERR(error.c_str()); \
  } \
  ovr_Shutdown(); \
  return result; \
}

