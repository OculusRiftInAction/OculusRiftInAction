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
    glm::ivec2 viewportPosition(eye == ovrEye_Left ? 0 : windowSize.x / 2, 0);
    GlfwApp::viewport(glm::uvec2(windowSize.x / 2, windowSize.y), viewportPosition);
  }
};
