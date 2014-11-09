#include "Common.h"


class RiftDisplay : public GlfwApp {
  bool directMode{ false };
  void * nativeWindow{ nullptr };
  ovrHmd hmd;

public:

  RiftDisplay() {
    hmd = ovrHmd_Create(0);
    if (!hmd) {
      FAIL("Unable to detect Rift display");
    }
  }

  virtual GLFWwindow * createRenderingTarget(glm::uvec2 & outSize, glm::ivec2 & outPosition) {
    GLFWwindow * window = nullptr;
    bool directHmdMode = false;

    outPosition = glm::ivec2(hmd->WindowsPos.x, hmd->WindowsPos.y);
    outSize = glm::uvec2(hmd->Resolution.w, hmd->Resolution.h);

    // The ovrHmdCap_ExtendDesktop only reliably reports on Windows currently
    ON_WINDOWS([&]{
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
    // The SDK currently allows no mechanism to test if this is the case.  I could query
    // GLFW for the current resolution of the Rift, but that sounds too much like actual
    // work.
    ON_LINUX([&]{
      std::swap(outSize.x, outSize.y);
    });

    if (directHmdMode) {
      // In direct mode, try to put the output window on a secondary screen
      // (for easier debugging, assuming your dev environment is on the primary)
      window = glfw::createSecondaryScreenWindow(outSize);
    } else {
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

  void draw() {
    glm::ivec2 position = glm::ivec2(0, 0);
    glm::vec4 color = glm::vec4(1, 0, 0, 1);
    glm::uvec2 eyeSize = getSize();
    eyeSize.x /= 2;

    using namespace oglplus;

    Context::Enable(Capability::ScissorTest);
    Context::Scissor(position.x, position.y, eyeSize.x, eyeSize.y);
    Context::ClearColor(color.r, color.g, color.b, color.a);
    Context::Clear().ColorBuffer();

    position = glm::ivec2(eyeSize.x, 0);
    color = glm::vec4(0, 0, 1, 1);

    Context::Scissor(position.x, position.y, eyeSize.x, eyeSize.y);
    Context::ClearColor(color.r, color.g, color.b, color.a);
    Context::Clear().ColorBuffer();
    Context::Disable(Capability::ScissorTest);
  }
};

RUN_OVR_APP(RiftDisplay);
