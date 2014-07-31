#include "Common.h"

class RiftDisplay : public GlfwApp {
glm::uvec2 eyeSize;
ovrHmd hmd;

public:
RiftDisplay() {
  hmd = ovrHmd_Create(0);
  if (!hmd) {
    FAIL("Unable to detect Rift display");
  }

  windowPosition = glm::ivec2(
      hmd->WindowsPos.x,
      hmd->WindowsPos.y);

  GLFWmonitor * hmdMonitor =
      GlfwApp::getMonitorAtPosition(windowPosition);
  const GLFWvidmode * videoMode =
      glfwGetVideoMode(hmdMonitor);
  windowSize = glm::uvec2(
      videoMode->width, videoMode->height);

  eyeSize = windowSize;
  eyeSize.x /= 2;
}

void createRenderingTarget() {
  glfwWindowHint(GLFW_DECORATED, 0);
  createWindow(windowSize, windowPosition);
  if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
    FAIL("Unable to create undecorated window");
  }
}

void initGl() {
  GlfwApp::initGl();
  glEnable(GL_SCISSOR_TEST);
}

void draw() {
  glm::ivec2 position(0, 0);
  gl::scissor(position, eyeSize);
  gl::clearColor(Colors::red);
  glClear(GL_COLOR_BUFFER_BIT);

  position = glm::ivec2(eyeSize.x, 0);
  gl::scissor(position, eyeSize);
  gl::clearColor(Colors::blue);
  glClear(GL_COLOR_BUFFER_BIT);
}
};

RUN_OVR_APP(RiftDisplay);
