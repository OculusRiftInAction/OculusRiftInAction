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

  /**
   * Build an extended mode window positioned to exactly overlay the OS desktop monitor
   * which corresponds to the Rift.
   */
  GLFWwindow * createExtendedModeWindow(glm::uvec2 & outSize, glm::ivec2 & outPosition) {
    // In extended desktop mode, we should be using the current resolution of the Rift.
    GLFWmonitor * monitor = glfw::getMonitorAtPosition(outPosition);
    if (nullptr != monitor) {
      auto mode = glfwGetVideoMode(monitor);
      outSize = glm::uvec2(mode->width, mode->height);
    }

    // If we're creating a desktop window, we strip off any window decorations
    // which might change the location of the rendered contents relative to the lenses.
    glfwWindowHint(GLFW_DECORATED, 0);
    return glfw::createWindow(outSize, outPosition);
  }

  /**
   * Build a Direct HMD mode window, binding the OVR SDK to the native window object.
   */
  GLFWwindow * createDirectHmdModeWindow(ovrHmd hmd, glm::uvec2 & outSize) {

    // On linux it's recommended to leave the screen in its default portrait orientation.
    // The SDK currently allows no mechanism to test if this is the case.
    // So in direct mode, we need to swap the x and y value.
    ON_LINUX([&] {
      std::swap(outSize.x, outSize.y);
    });

    // In direct HMD mode, we always use the native resolution, because the
    // user has no control over it.
    // In direct mode, try to put the output window on a secondary screen
    // (for easier debugging, assuming your dev environment is on the primary)
    GLFWwindow * window = glfw::createSecondaryScreenWindow(outSize);

    // Attach the OVR SDK to the native window
    void * nativeWindowHandle = glfw::getNativeWindowHandle(window);
    if (nullptr != nativeWindowHandle) {
      ovrHmd_AttachToWindow(hmd, nativeWindowHandle, nullptr, nullptr);
    }

    // A bug in some versions of the SDK (0.4.x) prevents Direct Mode from 
    // engaging properly unless you call the GetEyePoses function.
    {
      static ovrVector3f offsets[2];
      static ovrPosef poses[2];
      ovrHmd_GetEyePoses(hmd, 0, offsets, poses, nullptr);
    }

    return window;
  }

  /**
   * Build an OpenGL window, respecting the Rift's current display mode choice of
   * extended or direct HMD.
   */
  GLFWwindow * createRiftRenderingWindow(ovrHmd hmd, glm::uvec2 & outSize, glm::ivec2 & outPosition) {
    bool extendedMode = true;

    outPosition = glm::ivec2(hmd->WindowsPos.x, hmd->WindowsPos.y);
    outSize = glm::uvec2(hmd->Resolution.w, hmd->Resolution.h);

    // The ovrHmdCap_ExtendDesktop is currently only reported reliably on Windows
    ON_WINDOWS([&] {
      extendedMode = (ovrHmdCap_ExtendDesktop & hmd->HmdCaps);
    });

    return extendedMode
      ? createExtendedModeWindow(outSize, outPosition)
      : createDirectHmdModeWindow(hmd, outSize);
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
