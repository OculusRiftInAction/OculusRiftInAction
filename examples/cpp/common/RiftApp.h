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

class RiftManagerApp {
protected:
  ovrHmd hmd;

  glm::uvec2 hmdNativeResolution;
  glm::ivec2 hmdDesktopPosition;

public:
  RiftManagerApp(ovrHmdType defaultHmdType = ovrHmd_DK2) {
    hmd = ovrHmd_Create(0);
    if (nullptr == hmd) {
      hmd = ovrHmd_CreateDebug(defaultHmdType);
      hmdDesktopPosition = glm::ivec2(100, 100);
    }
    else {
      hmdDesktopPosition = glm::ivec2(hmd->WindowsPos.x, hmd->WindowsPos.y);
    }
    hmdNativeResolution = glm::ivec2(hmd->Resolution.w, hmd->Resolution.h);
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
  RiftGlfwApp(bool fullscreen = false) : fullscreen(fullscreen) {
  }

  virtual ~RiftGlfwApp() {
  }

  virtual GLFWwindow * createRenderingTarget(glm::uvec2 & outSize, glm::ivec2 & outPosition) {
    return ovr::createRiftRenderingWindow(hmd, outSize, outPosition);
  }

  using GlfwApp::viewport;
  virtual void viewport(ovrEyeType eye) {
    const glm::uvec2 & windowSize = getSize();
    glm::uvec2 viewportPosition(eye == ovrEye_Left ? 0 : windowSize.x / 2, 0);
    GlfwApp::viewport(viewportPosition,  glm::uvec2(windowSize.x / 2, windowSize.y));
  }
    
  int getEnabledCaps() {
    return ovrHmd_GetEnabledCaps(hmd);
  }

  void enableCaps(int caps) {
    ovrHmd_SetEnabledCaps(hmd, getEnabledCaps() | caps);
  }

  void toggleCaps(ovrHmdCaps cap) {
    if (cap & getEnabledCaps()) {
      disableCaps(cap);
    } else {
      enableCaps(cap);
    }
  }

  void disableCaps(int caps) {
    ovrHmd_SetEnabledCaps(hmd, getEnabledCaps() & ~caps);
  }
};

class RiftApp : public RiftGlfwApp {
public:

protected:
  glm::mat4 player;
  ovrTexture eyeTextures[2];
  ovrVector3f eyeOffsets[2];

private:
  ovrEyeRenderDesc eyeRenderDescs[2];
  ovrPosef eyePoses[2];
  ovrEyeType currentEye;

  glm::mat4 projections[2];
  FramebufferWrapper eyeFramebuffers[2];

protected:
  using RiftGlfwApp::renderStringAt;
  void renderStringAt(const std::string & str, float x, float y, float size = 18.0f);
  virtual void initGl();
  virtual void finishFrame();
  virtual void onKey(int key, int scancode, int action, int mods);
  virtual void draw() final;
  virtual void postDraw() {};
  virtual void update();
  virtual void renderScene() = 0;

  virtual void applyEyePoseAndOffset(const glm::mat4 & eyePose, const glm::vec3 & eyeOffset);

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

  const ovrPosef & getEyePose(ovrEyeType eye) const {
    return eyePoses[eye];
  }

  const ovrPosef & getEyePose() const {
    return getEyePose(getCurrentEye());
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

