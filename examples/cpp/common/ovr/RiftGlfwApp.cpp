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
void RiftGlfwApp::initGl() {
    GlfwApp::initGl();
    initializeRiftRendering();
    setupMirror(getSize());
    glfwSwapInterval(0);
}

GLFWwindow * RiftGlfwApp::createRenderingTarget(glm::uvec2 & outSize, glm::ivec2 & outPosition) {
    return ovr::createRiftRenderingWindow(hmdDesc, outSize, outPosition);
}

void RiftGlfwApp::draw() {
    drawRiftFrame();
}


void RiftGlfwApp::onKey(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_R:
            ovr_RecenterPose(hmd);
            return;
        }
    }
    GlfwApp::onKey(key, scancode, action, mods);
}
