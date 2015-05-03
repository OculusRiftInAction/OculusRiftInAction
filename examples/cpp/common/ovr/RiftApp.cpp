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

RiftApp::RiftApp() {
  float eyeHeight = 1.5f;
  player = glm::inverse(glm::lookAt(
    glm::vec3(0, eyeHeight, 4),
    glm::vec3(0, eyeHeight, 0),
    glm::vec3(0, 1, 0)));

}

RiftApp::~RiftApp() {
}

void RiftApp::finishFrame() {
}

void RiftApp::initGl() {
  GlfwApp::initGl();
  RiftRenderingApp::initializeRiftRendering();
}

void RiftApp::draw() {
  drawRiftFrame();
}

void RiftApp::renderStringAt(const std::string & str, float x, float y, float size) {
  MatrixStack & mv = Stacks::modelview();
  MatrixStack & pr = Stacks::projection();
  Stacks::withPush(mv, pr, [&]{
    mv.identity();
    pr.top() = 1.0f * glm::ortho(
      -1.0f, 1.0f,
      -windowAspectInverse * 2.0f, windowAspectInverse * 2.0f,
      -100.0f, 100.0f);
    glm::vec2 cursor(x, windowAspectInverse * y);
    oria::renderString(str, cursor, size);
  });
}
