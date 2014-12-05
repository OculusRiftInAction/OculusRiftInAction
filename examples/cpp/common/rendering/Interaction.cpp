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
#include "Interaction.h"

CameraControl::CameraControl() {
}

CameraControl & CameraControl::instance() {
  static CameraControl instance_;
  return instance_;
}

void translateCamera(glm::mat4 & camera, const glm::vec3 & delta) {
  // SAY("Translating %01.3f %01.3f %01.3f", delta.x, delta.y, delta.z);
  // Bring the vector into camera space coordinates
  glm::vec3 eyeDelta = glm::quat(camera) * delta;
  camera = glm::translate(glm::mat4(), eyeDelta) * camera;
}

void rotateCamera(glm::mat4 & camera, const glm::quat & rot) {
  camera = camera * glm::mat4_cast(rot);
}

void recompose(glm::mat4 & camera) {
  glm::vec4 t4 = camera[3];
  t4 /= t4.w;
  camera = glm::mat4_cast(glm::normalize(glm::quat(camera)));
  camera[3] = t4;
}

bool CameraControl::onKey(int key, int scancode, int action, int mods) {
  if (GLFW_PRESS != action && GLFW_RELEASE != action) {
    return false;
  }
  SAY("KEY %s: %c", (GLFW_PRESS == action) ? "pressed" : "released", key);
  int update = (GLFW_PRESS == action) ? 1 : 0;
  switch (key) {
  case GLFW_KEY_A:
    keyboardTranslate.x = -update; return true;
  case GLFW_KEY_D:
    keyboardTranslate.x = update; return true;
  case GLFW_KEY_W:
    keyboardTranslate.z = -update; return true;
  case GLFW_KEY_S:
    keyboardTranslate.z = update; return true;
  case GLFW_KEY_C:
    keyboardTranslate.y = -update; return true;
  case GLFW_KEY_F:
  case GLFW_KEY_SPACE:
    keyboardTranslate.y = update; return true;
  case GLFW_KEY_RIGHT:
    keyboardRotate.y = -update; return true;
  case GLFW_KEY_LEFT:
    keyboardRotate.y = update; return true;
  }
  return false;
}

void CameraControl::applyInteraction(glm::mat4 & camera) {
  static uint32_t lastKeyboardUpdateTick = 0;
  uint32_t now = Platform::elapsedMillis();
  if (0 != lastKeyboardUpdateTick) {
    float dt = (now - lastKeyboardUpdateTick) / 1000.0f;
    if (keyboardRotate.x || keyboardRotate.y || keyboardRotate.z) {
      const glm::quat delta = glm::quat(glm::vec3(keyboardRotate) * dt);
      rotateCamera(camera, delta);
    }
    if (keyboardTranslate.x || keyboardTranslate.y || keyboardTranslate.z) {
      const glm::vec3 delta = glm::vec3(keyboardTranslate) * dt;
      translateCamera(camera, delta);
    }
  }
  lastKeyboardUpdateTick = now;
}
