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

#include "Common.h"
#include "RiftGlfwApp.h"
#include <OVR_CAPI_GL.h>

RiftGlfwApp::RiftGlfwApp() {
}

RiftGlfwApp::~RiftGlfwApp() {
}

GLFWwindow * RiftGlfwApp::createRenderingTarget(glm::uvec2 & outSize, glm::ivec2 & outPosition) {
  return ovr::createRiftRenderingWindow(hmd, outSize, outPosition);
}

void RiftGlfwApp::viewport(ovrEyeType eye) {
  const glm::uvec2 & windowSize = getSize();
  glm::ivec2 viewportPosition(eye == ovrEye_Left ? 0 : windowSize.x / 2, 0);
  GlfwApp::viewport(glm::uvec2(windowSize.x / 2, windowSize.y), viewportPosition);
}

void RiftGlfwApp::onKey(int key, int scancode, int action, int mods) {
  if (!oria::clearHSW(hmd)) {

    if (GLFW_PRESS == action) {
      switch (key) {
      case GLFW_KEY_R:
        ovrHmd_RecenterPose(hmd);
        return;
      }
    }

    GlfwApp::onKey(key, scancode, action, mods);
  }
}
