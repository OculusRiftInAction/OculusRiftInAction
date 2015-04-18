#include "Common.h"

// For some interaction with the Oculus SDK we'll need the native window
// handle from GLFW.  To get it we need to define a couple of macros
// (that depend on OS) and include an additional header
#if defined(OS_WIN)
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#elif defined(OS_OSX)
#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_NSGL
#elif defined(OS_LINUX)
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#endif
#include <GLFW/glfw3native.h>

namespace glfw {
  void * getNativeWindowHandle(GLFWwindow * window) {
    void * nativeWindowHandle = nullptr;
    ON_WINDOWS([&]{
      nativeWindowHandle = (void*)glfwGetWin32Window(window);
    });
    ON_LINUX([&]{
      nativeWindowHandle = (void*)glfwGetX11Window(window);
    });
    ON_MAC([&]{
      nativeWindowHandle = (void*)glfwGetCocoaWindow(window);
    });
    return nativeWindowHandle;
  }

  void * getNativeDisplay(GLFWwindow * window) {
    void * nativeDisplay = nullptr;
    ON_LINUX([&]{
      nativeDisplay = (void*)glfwGetX11Display();
    });
    return nativeDisplay;
  }


}
