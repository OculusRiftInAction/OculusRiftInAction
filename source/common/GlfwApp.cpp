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

void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action,
    int mods) {
  GlfwApp * instance = (GlfwApp *) glfwGetWindowUserPointer(window);
  instance->onKey(key, scancode, action, mods);
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

void compileAllShaders(ShaderResource * shaders,
    GLenum shaderType) {
  int i = 0;
  while (shaders[i] != ShaderResource::NO_SHADER) {
    std::string shaderFile = getShaderPath(shaders[i]);
    SAY("Compiling %s", shaderFile.c_str());
    std::string shaderSource = Files::read(shaderFile);
    gl::Shader s(shaderType, shaderSource);
    ++i;
  }
}

void GlfwApp::onCreate() {
  glfwSetWindowUserPointer(window, this);
  glfwSetKeyCallback(window, glfwKeyCallback);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  GL_CHECK_ERROR;
#ifndef __APPLE__
  // Initialize the OpenGL bindings
  if (0 != glewInit()) {
    FAIL("Failed to initialize GL3W");
  }
  glGetError();
  GL_CHECK_ERROR;
#endif
  if (glNamedStringARB) {
    for (int i = 0; ShaderResources::LIB_SHADERS[i] != ShaderResource::NO_SHADER; ++i) {
      std::string shaderFile = getShaderPath(ShaderResources::LIB_SHADERS[i]);
      std::string shaderSrc = Files::read(shaderFile);
      size_t lastSlash = shaderFile.rfind('/');
      std::string name = shaderFile.substr(lastSlash);
//      glNamedStringARB(GL_SHADER_INCLUDE_ARB,
//          name.length(), name.c_str(),
//          shaderSrc.length(), shaderSrc.c_str());
      GL_CHECK_ERROR;
    }
  }

  compileAllShaders(ShaderResources::VERTEX_SHADERS, GL_VERTEX_SHADER);
  compileAllShaders(ShaderResources::FRAGMENT_SHADERS, GL_FRAGMENT_SHADER);
}

void GlfwApp::createWindow(const glm::ivec2 & size, const glm::ivec2 & position) {
  this->size = size;
  glfwWindowHint(GLFW_DEPTH_BITS, 16);

#ifdef __APPLE__
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

  window = glfwCreateWindow(size.x, size.y, "glfw", NULL, NULL);
  if (!window) {
    FAIL("Unable to create rendering window");
  }
  if (position.x > INT_MIN && position.y > INT_MIN) {
    glfwSetWindowPos(window, position.x, position.y);
  }
  onCreate();
}

void GlfwApp::fullscreen(const glm::ivec2 & size, const char * display) {
  this->size = size;
  int count;
  GLFWmonitor* target = nullptr;
  GLFWmonitor** monitors = glfwGetMonitors(&count);
  for (int i = 0; i < count; ++i) {
    GLFWmonitor * monitor = monitors[i];
    int x, y;
    glfwGetMonitorPhysicalSize(monitor, &x, &y);
    const GLFWvidmode * mode = glfwGetVideoMode(monitor);
    const char * monitorName = glfwGetMonitorName(monitor);
    if (0 == strcmp(display, monitorName)) {
      target = monitor;
      break;
    }
  }
  if (nullptr == target) {
    FAIL("Unable to find Rift target monitor for fullscreen output");
  }
  window = glfwCreateWindow(size.x, size.y, "glfw", target, NULL);
  assert(window != 0);
  onCreate();
}

void GlfwApp::initGl() {
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glDisable(GL_DITHER);
  glEnable(GL_DEPTH_TEST);
  GL_CHECK_ERROR;
}

void GlfwApp::destroyWindow() {
  glfwSetKeyCallback(window, nullptr);
  glfwDestroyWindow(window);
}

GlfwApp::~GlfwApp() {
  glfwTerminate();
}

void GlfwApp::screenshot() {

#ifdef HAVE_OPENCV
  //use fast 4-byte alignment (default anyway) if possible
  glFlush();
  cv::Mat img(size.x, size.y, CV_8UC3);
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
    return -1;
  }

  initGl();

  int framecount = 0;
  long start = Platform::elapsedMillis();
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    update();
    draw();
    glfwSwapBuffers(window);
    long now = Platform::elapsedMillis();
    ++framecount;
    if ((now - start) >= 2000) {
      float elapsed = (now - start) / 1000.f;
      float fps = (float) framecount / elapsed;
      SAY("FPS: %0.2f\n", fps);
      start = now;
      framecount = 0;
    }
  }
  return 0;
}

void GlfwApp::onKey(int key, int scancode, int action, int mods) {
  if (GLFW_PRESS != action) {
    return;
  }

  switch (key) {
  case GLFW_KEY_ESCAPE:
    exit(0);
#ifdef HAVE_OPENCV
    case GLFW_KEY_S:
    if (mods & GLFW_MOD_SHIFT) {
      screenshot();
    }
#endif
  }
}

void GlfwApp::draw() {
}

void GlfwApp::update() {
}

