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
      if (best == nullptr || (bestSize.x < curSize.x && bestSize.y < curSize.y)) {
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

  inline void * getNativeWindowHandle(GLFWwindow * window) {
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

class GlfwApp {
private:
  GLFWwindow *  window{ nullptr };
  glm::uvec2    windowSize;
  glm::ivec2    windowPosition;
  int           frame{ 0 };

protected:
  float         windowAspect{ 1.0f };
  float         windowAspectInverse{ 1.0f };
  float         fps{ 0.0 };

public:
  GlfwApp() {
    // Initialize the GLFW system for creating and positioning windows
    if (!glfwInit()) {
      FAIL("Failed to initialize GLFW");
    }
    glfwSetErrorCallback(ErrorCallback);
  }

  virtual ~GlfwApp() {
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  virtual int run() {
    try {
      preCreate();
      window = createRenderingTarget(windowSize, windowPosition);
      postCreate();

      if (!window) {
        FAIL("Unable to create OpenGL window");
      }

      initGl();
      // Ensure we shutdown the GL resources even if we throw an exception
      Finally f([&]{
        shutdownGl();
      });

      int framecount = 0;
      long start = Platform::elapsedMillis();
      while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ++frame;
        update();
        draw();
        finishFrame();
        long now = Platform::elapsedMillis();
        ++framecount;
        if ((now - start) >= 2000) {
          float elapsed = (now - start) / 1000.f;
          fps = (float)framecount / elapsed;
          SAY("FPS: %0.2f", fps);
          start = now;
          framecount = 0;
        }
      }
    }
    catch (std::runtime_error & err) {
      SAY(err.what());
    }
    return 0;
  }


protected:
  virtual GLFWwindow * createRenderingTarget(glm::uvec2 & outSize, glm::ivec2 & outPosition) = 0;
  virtual void draw() = 0;

  int getFrame() const {
    return this->frame;
  }

  void preCreate() {
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Without this line we get
    // FATAL (86): NSGL: The targeted version of OS X only supports OpenGL 3.2 and later versions if they are forward-compatible
    ON_MAC([]{
      glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    });
#ifdef DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
  }

  void postCreate()  {
    glfwMakeContextCurrent(window);
    windowAspect = aspect(windowSize);
    windowAspectInverse = 1.0f / windowAspect;
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSwapInterval(1);

    // Initialize the OpenGL bindings
    // For some reason we have to set this experminetal flag to properly
    // init GLEW if we use a core context.
    glewExperimental = GL_TRUE;
    if (0 != glewInit()) {
      FAIL("Failed to initialize GLEW");
    }
    glGetError();

#if DEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    GLuint unusedIds = 0;
    if (glDebugMessageCallback) {
      glDebugMessageCallback(glfw::debugCallback, this);
      glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
        0, &unusedIds, true);
    }
    else if (glDebugMessageCallbackARB) {
      glDebugMessageCallbackARB(glfw::debugCallback, this);
      glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
        0, &unusedIds, true);
    }
#endif
  }

  const glm::uvec2 & getSize() const {
    return windowSize;
  }

  const glm::ivec2 & getPosition() const {
    return windowPosition;
  }

  GLFWwindow * getWindow() {
    return window;
  }

  virtual void initGl() {
    using namespace oglplus;
    DefaultFramebuffer().Bind(Framebuffer::Target::Draw);
    DefaultFramebuffer().Bind(Framebuffer::Target::Read);
    Context::Enable(Capability::CullFace);
    Context::Enable(Capability::DepthTest);
    Context::Disable(Capability::Dither);
  }

  virtual void shutdownGl() {
    Platform::runShutdownHooks();
  }
  
  virtual void finishFrame() {
    glfwSwapBuffers(window);
  }

  virtual void destroyWindow() {
    glfwSetKeyCallback(window, nullptr);
    glfwDestroyWindow(window);
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action) {
      return;
    }

    switch (key) {
    case GLFW_KEY_ESCAPE:
      glfwSetWindowShouldClose(window, 1);
      return;

    case GLFW_KEY_S:
      if (mods & GLFW_MOD_SHIFT) {
        screenshot();
      }
      return;
    }
  }

  virtual void onMouseButton(int button, int action, int mods) { }
  virtual void update() {  }


protected:
  virtual void viewport(const glm::ivec2 & pos, const glm::uvec2 & size) {
    oglplus::Context::Viewport(pos.x, pos.y, size.x, size.y);
  }

  virtual void renderStringAt(const std::string & string, float x, float y) {
    renderStringAt(string, glm::vec2(x, y));
  }

  virtual void renderStringAt(const std::string & string, const glm::vec2 & position);


private:

  static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    GlfwApp * instance = (GlfwApp *)glfwGetWindowUserPointer(window);
    instance->onKey(key, scancode, action, mods);
  }

  static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    GlfwApp * instance = (GlfwApp *)glfwGetWindowUserPointer(window);
    instance->onMouseButton(button, action, mods);
  }

  static void ErrorCallback(int error, const char* description) {
    FAIL(description);
  }



  virtual void screenshot() {
//#ifdef HAVE_OPENCV
//    //use fast 4-byte alignment (default anyway) if possible
//    glFlush();
//    cv::Mat img(windowSize.y, windowSize.x, CV_8UC3);
//    glPixelStorei(GL_PACK_ALIGNMENT, (img.step & 3) ? 1 : 4);
//    glPixelStorei(GL_PACK_ROW_LENGTH, img.step / img.elemSize());
//    glReadPixels(0, 0, img.cols, img.rows, GL_BGR, GL_UNSIGNED_BYTE, img.data);
//    cv::flip(img, img, 0);
//
//    static int counter = 0;
//    static char buffer[128];
//    sprintf(buffer, "screenshot%05i.png", counter++);
//    bool success = cv::imwrite(buffer, img);
//    if (!success) {
//      throw std::runtime_error("Failed to write image");
//    }
//#endif
  }
};

