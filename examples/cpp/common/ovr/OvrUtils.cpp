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

#ifdef _DEBUG
#define BRAD_DEBUG 1
#endif

namespace ovr {

  GLFWwindow * createRiftRenderingWindow(ovrHmd hmd, glm::uvec2 & outSize, glm::ivec2 & outPosition) {
    GLFWwindow * window = nullptr;
    bool directHmdMode = false;

    outPosition = glm::ivec2(hmd->WindowsPos.x, hmd->WindowsPos.y);
    outSize = glm::uvec2(hmd->Resolution.w, hmd->Resolution.h);

    // The ovrHmdCap_ExtendDesktop only reliably reports on Windows currently
    ON_WINDOWS([&] {
      directHmdMode = (0 == (ovrHmdCap_ExtendDesktop & hmd->HmdCaps));
    });

    // In direct HMD mode, we always use the native resolution, because the
    // user has no control over it.
    // In legacy mode, we should be using the current resolution of the Rift
    // (unless we're creating a 'fullscreen' window)
    if (!directHmdMode) {
      GLFWmonitor * monitor = glfw::getMonitorAtPosition(outPosition);
      if (nullptr != monitor) {
        auto mode = glfwGetVideoMode(monitor);
        outSize = glm::uvec2(mode->width, mode->height);
      }
    }

    // On linux it's recommended to leave the screen in it's default portrait orientation.
    // The SDK currently allows no mechanism to test if this is the case.
    ON_LINUX([&] {
      // If we're running in extended mode, then we have the size of the screen from the GLFW call
      // But in direct mode we need to swap the x and y value
      if (directHmdMode) {
        std::swap(outSize.x, outSize.y);
      }
    });

    if (directHmdMode) {
      // In direct mode, try to put the output window on a secondary screen
      // (for easier debugging, assuming your dev environment is on the primary)
      window = glfw::createSecondaryScreenWindow(outSize);
    }
    else {
      // If we're creating a desktop window, we should strip off any window decorations
      // which might change the location of the rendered contents relative to the lenses.
      //
      // Another alternative would be to create the window in fullscreen mode, on
      // platforms that support such a thing.
      glfwWindowHint(GLFW_DECORATED, 0);
      window = glfw::createWindow(outSize, outPosition);
    }

    // If we're in direct mode, attach to the window
    if (directHmdMode) {
      void * nativeWindowHandle = glfw::getNativeWindowHandle(window);
      if (nullptr != nativeWindowHandle) {
        ovrHmd_AttachToWindow(hmd, nativeWindowHandle, nullptr, nullptr);
      }
    }

    return window;
  }

}

namespace oria {

  // Returns true if the HSW needed to be removed from display, else false.
  bool clearHSW(ovrHmd hmd) {
    static bool dismissedHsw = false;

    if (!dismissedHsw) {
      ovrHSWDisplayState hswDisplayState;
      ovrHmd_GetHSWDisplayState(hmd, &hswDisplayState);
      if (hswDisplayState.Displayed) {
        ovrHmd_DismissHSWDisplay(hmd);
        return true;
      }
      else {
        dismissedHsw = true;
      }
    }
    return !dismissedHsw;
  }

}
