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

#if defined(OS_LINUX)
#include <X11/Xlib.h>
#endif

namespace glfw {
  inline glm::uvec2 getSize(GLFWmonitor * monitor) {
    const GLFWvidmode * mode = glfwGetVideoMode(monitor);
    return glm::uvec2(mode->width, mode->height);
  }

  inline glm::ivec2 getPosition(GLFWmonitor * monitor) {
    glm::ivec2 result;
    glfwGetMonitorPos(monitor, &result.x, &result.y);
    return result;
  }

  inline glm::ivec2 getSecondaryScreenPosition(const glm::uvec2 & size) {
    GLFWmonitor * primary = glfwGetPrimaryMonitor();
    int monitorCount;
    GLFWmonitor ** monitors = glfwGetMonitors(&monitorCount);
    GLFWmonitor * best = nullptr;
    glm::uvec2 bestSize;
    for (int i = 0; i < monitorCount; ++i) {
      GLFWmonitor * cur = monitors[i];
      if (cur == primary) {
        continue;
      }
      glm::uvec2 curSize = getSize(cur);
      if (best == nullptr || (bestSize.x < curSize.x || bestSize.y < curSize.y)) {
        best = cur;
        bestSize = curSize;
      }
    }
    if (nullptr == best) {
      best = primary;
      bestSize = getSize(best);
    }
    glm::ivec2 pos = getPosition(best);
    if (bestSize.x > size.x) {
      pos.x += (bestSize.x - size.x) / 2;
    }

    if (bestSize.y > size.y) {
      pos.y += (bestSize.y - size.y) / 2;
    }

    return pos;
  }

  inline GLFWmonitor * getMonitorAtPosition(const glm::ivec2 & position) {
    int count;
    GLFWmonitor ** monitors = glfwGetMonitors(&count);
    for (int i = 0; i < count; ++i) {
      glm::ivec2 candidatePosition;
      glfwGetMonitorPos(monitors[i], &candidatePosition.x, &candidatePosition.y);
      if (candidatePosition == position) {
        return monitors[i];
      }
    }
    return nullptr;
  }

  void * getNativeWindowHandle(GLFWwindow * window);
  void * getNativeDisplay(GLFWwindow * window);

  inline GLFWwindow * createWindow(const glm::uvec2 & size, const glm::ivec2 & position = glm::ivec2(INT_MIN)) {
    GLFWwindow * window = glfwCreateWindow(size.x, size.y, "glfw", nullptr, nullptr);
    if (!window) {
      FAIL("Unable to create rendering window");
    }
    if ((position.x > INT_MIN) && (position.y > INT_MIN)) {
      glfwSetWindowPos(window, position.x, position.y);
    }
    return window;
  }

  inline GLFWwindow * createWindow(int w, int h, int x = INT_MIN, int y = INT_MIN) {
    return createWindow(glm::uvec2(w, h), glm::ivec2(x, y));
  }

  inline GLFWwindow * createFullscreenWindow(const glm::uvec2 & size, GLFWmonitor * targetMonitor) {
    GLFWwindow * window = glfwCreateWindow(size.x, size.y, "glfw", targetMonitor, nullptr);
    assert(window != 0);
    return window;
  }

  inline GLFWwindow * createSecondaryScreenWindow(const glm::uvec2 & size) {
    return createWindow(size, getSecondaryScreenPosition(size));
  }
}
