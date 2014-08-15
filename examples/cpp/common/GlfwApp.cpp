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
#include "GlfwApp.h"
#include <regex>
#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

#ifdef __APPLE__
#include <CoreGraphics/CGDirectDisplay.h>
#include <CoreGraphics/CGDisplayConfiguration.h>
#endif

glm::uvec2 getSize(GLFWmonitor * monitor) {
  const GLFWvidmode * mode = glfwGetVideoMode(monitor);
  return glm::uvec2(mode->width, mode->height);
}

glm::ivec2 getPosition(GLFWmonitor * monitor) {
  glm::ivec2 result;
  glfwGetMonitorPos(monitor, &result.x, &result.y);
  return result;
}

void GlfwApp::createSecondaryScreenWindow(const glm::uvec2 & size) {
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
  createWindow(size, pos);
}

void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action,
    int mods) {
  GlfwApp * instance = (GlfwApp *) glfwGetWindowUserPointer(window);
  instance->onKey(key, scancode, action, mods);
}

void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  GlfwApp * instance = (GlfwApp *)glfwGetWindowUserPointer(window);
  instance->onMouseButton(button, action, mods);
}

void glfwErrorCallback(int error, const char* description) {
  FAIL(description);
}

GlfwApp::GlfwApp()
    : window(0) {
  // Initialize the GLFW system for creating and positioning windows
  if (!glfwInit()) {
    FAIL("Failed to initialize GLFW");
  }
  glfwSetErrorCallback(glfwErrorCallback);
}

void compileAllShaders(const Resource * shaders,
    GLenum shaderType) {
  int i = 0;
  while (shaders[i] != Resource::NO_RESOURCE) {
    SAY("Compiling %s", Resources::getResourcePath(shaders[i]).c_str());
    std::string shaderSource = Platform::getResourceData(shaders[i]);
    gl::Shader s(shaderType, shaderSource);
    ++i;
  }
}

void APIENTRY debugCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar * message,
    void * userParam) {
  const char * typeStr = "?";
  switch (type) {
  case GL_DEBUG_TYPE_ERROR:
    typeStr = "ERROR";
    break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
    typeStr = "DEPRECATED_BEHAVIOR";
    break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
    typeStr = "UNDEFINED_BEHAVIOR";
    break;
  case GL_DEBUG_TYPE_PORTABILITY:
    typeStr = "PORTABILITY";
    break;
  case GL_DEBUG_TYPE_PERFORMANCE:
    typeStr = "PERFORMANCE";
    break;
  case GL_DEBUG_TYPE_OTHER:
    typeStr = "OTHER";
    break;
  }

  const char * severityStr = "?";
  switch (severity) {
  case GL_DEBUG_SEVERITY_LOW:
    severityStr = "LOW";
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    severityStr = "MEDIUM";
    break;
  case GL_DEBUG_SEVERITY_HIGH:
    severityStr = "HIGH";
    break;
  }
  SAY("--- OpenGL Callback Message ---");
  SAY("type: %s\nseverity: %-8s\nid: %d\nmsg: %s", typeStr, severityStr, id,
      message);
  SAY("--- OpenGL Callback Message ---");
}

void GlfwApp::onCreate() {
  windowAspect = glm::aspect(windowSize);
  windowAspectInverse = 1.0f / windowAspect;
  glfwSetWindowUserPointer(window, this);
  glfwSetKeyCallback(window, glfwKeyCallback);
  glfwSetMouseButtonCallback(window, glfwMouseButtonCallback);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

// Initialize the OpenGL bindings
// For some reason we have to set this experminetal flag to properly
// init GLEW if we use a core context.
  glewExperimental = GL_TRUE;
  if (0 != glewInit()) {
    FAIL("Failed to initialize GL3W");
  }
  glGetError();
#ifdef RIFT_DEBUG
  GL_CHECK_ERROR;
  glEnable (GL_DEBUG_OUTPUT_SYNCHRONOUS);
  GL_CHECK_ERROR;
  GLuint unusedIds = 0;
  if (glDebugMessageCallback) {
    glDebugMessageCallback(debugCallback, this);
    GL_CHECK_ERROR;
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
        0, &unusedIds, true);
    GL_CHECK_ERROR;

  } else if (glDebugMessageCallbackARB) {
    glDebugMessageCallbackARB(debugCallback, this);
    GL_CHECK_ERROR;
    glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
        0, &unusedIds, true);
    GL_CHECK_ERROR;
  }
#endif
  GL_CHECK_ERROR;
/*
  if (glNamedStringARB) {
    for (int i = 0;
        Resources::LIB_SHADERS[i] != Resource::NO_SHADER; ++i) {

      std::string shaderFile = getShaderPath(
          Resources::LIB_SHADERS[i]);
      std::string shaderSrc = Files::read(shaderFile);
      size_t lastSlash = shaderFile.rfind('/');
      std::string name = shaderFile.substr(lastSlash);
      glNamedStringARB(GL_SHADER_INCLUDE_ARB,
          name.length(), name.c_str(),
          shaderSrc.length(), shaderSrc.c_str());
      GL_CHECK_ERROR;
    }
  }
*/
  compileAllShaders(Resources::VERTEX_SHADERS, GL_VERTEX_SHADER);
  compileAllShaders(Resources::FRAGMENT_SHADERS, GL_FRAGMENT_SHADER);
}

void GlfwApp::preCreate() {
  glfwWindowHint(GLFW_DEPTH_BITS, 16);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // Without this line we get
  // FATAL (86): NSGL: The targeted version of OS X only supports OpenGL 3.2 and later versions if they are forward-compatible
  ON_MAC([]{
      glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  });
#ifdef RIFT_DEBUG
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
}

void GlfwApp::createWindow(const glm::uvec2 & size, const glm::ivec2 & position) {
  windowSize = size;
  windowPosition = position;
  preCreate();
  window = glfwCreateWindow(size.x, size.y, "glfw", nullptr, nullptr);
  if (!window) {
    FAIL("Unable to create rendering window");
  }
  if ((position.x > INT_MIN) && (position.y > INT_MIN)) {
    glfwSetWindowPos(window, position.x, position.y);
  }
  onCreate();
}

void GlfwApp::createFullscreenWindow(const glm::uvec2 & size, GLFWmonitor * monitor) {
  windowSize = size;
  preCreate();
  const GLFWvidmode * currentMode = glfwGetVideoMode(monitor);
  window = glfwCreateWindow(windowSize.x, windowSize.y, "glfw", monitor, nullptr);
  assert(window != 0);
  onCreate();
}

void GlfwApp::initGl() {
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glDisable(GL_DITHER);
  glEnable(GL_DEPTH_TEST);
  query = gl::TimeQueryPtr(new gl::TimeQuery());
  GL_CHECK_ERROR;
}

void GlfwApp::finishFrame() {
  glfwSwapBuffers(window);
}

void GlfwApp::destroyWindow() {
  glfwSetKeyCallback(window, nullptr);
  glfwDestroyWindow(window);
}

GlfwApp::~GlfwApp() {
  glfwTerminate();
}

void GlfwApp::renderStringAt(const std::string & str, const glm::vec2 & pos) {
  gl::MatrixStack & mv = gl::Stacks::modelview();
  gl::MatrixStack & pr = gl::Stacks::projection();
  mv.push().identity();
  pr.push().top() = glm::ortho(
    -1.0f, 1.0f,
    -windowAspectInverse, windowAspectInverse,
    -100.0f, 100.0f);
  glm::vec2 cursor(pos.x, windowAspectInverse * pos.y);
  GlUtils::renderString(str, cursor, 18.0f);
  pr.pop();
  mv.pop();
}

void GlfwApp::screenshot() {

#ifdef HAVE_OPENCV
  //use fast 4-byte alignment (default anyway) if possible
  glFlush();
  cv::Mat img(windowSize.y, windowSize.x, CV_8UC3);
  glPixelStorei(GL_PACK_ALIGNMENT, (img.step & 3) ? 1 : 4);
  glPixelStorei(GL_PACK_ROW_LENGTH, img.step / img.elemSize());
  glReadPixels(0, 0, img.cols, img.rows, GL_BGR, GL_UNSIGNED_BYTE, img.data);
  cv::flip(img, img, 0);

  static int counter = 0;
  static char buffer[128];
  sprintf(buffer, "screenshot%05i.png", counter++);
  bool success = cv::imwrite(buffer, img);
  if (!success) {
    throw std::runtime_error("Failed to write image");
  }
#endif
}

int GlfwApp::run() {
  createRenderingTarget();

  if (!window) {
    FAIL("Unable to create OpenGL window");
  }

  initGl();

  int framecount = 0;
  long start = Platform::elapsedMillis();
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    update();
    draw();
    finishFrame();
    long now = Platform::elapsedMillis();
    ++framecount;
    if ((now - start) >= 2000) {
      float elapsed = (now - start) / 1000.f;
      fps = (float) framecount / elapsed;
      SAY("FPS: %0.2f", fps);
      start = now;
      framecount = 0;
    }
  }
  glfwDestroyWindow(window);
  return 0;
}

void GlfwApp::onKey(int key, int scancode, int action, int mods) {
  if (GLFW_PRESS != action) {
    return;
  }

  switch (key) {
  case GLFW_KEY_ESCAPE:
    glfwSetWindowShouldClose(window, 1);
    return;

#ifdef HAVE_OPENCV
    case GLFW_KEY_S:
    if (mods & GLFW_MOD_SHIFT) {
      screenshot();
    }
    return;
#endif
  }
}

void GlfwApp::onMouseButton(int button, int action, int mods) {

}


void GlfwApp::draw() {
}

void GlfwApp::update() {
}

GLFWmonitor * GlfwApp::getMonitorAtPosition(const glm::ivec2 & position) {
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
