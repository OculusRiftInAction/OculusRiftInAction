#include "Common.h"

using namespace OVR;
using namespace glm;

class DisplayTargeting : public GlfwApp {
protected:
  Ptr<DeviceManager> ovrManager;
  HMDInfo hmdInfo;
  ivec2 eyeSize;
  GLFWmonitor * monitor;

public:
  DisplayTargeting() {
    ovrManager = *DeviceManager::Create();
    if (!ovrManager) {
      FAIL("Unable to create Rift device manager");
    }

    Ptr<HMDDevice> ovrHmd = *ovrManager->EnumerateDevices<HMDDevice>().CreateDevice();
    if (!ovrHmd) {
      FAIL("Unable to detect Rift display");
    }
    ovrHmd->GetDeviceInfo(&hmdInfo);
    ovrHmd = nullptr;
    monitor = GlfwApp::getMonitorAtPosition(glm::ivec2(hmdInfo.DesktopX, hmdInfo.DesktopY));
  }

  ~DisplayTargeting() {
    ovrManager = nullptr;
  }

  void initGl() {
    GlfwApp::initGl();
    glEnable(GL_SCISSOR_TEST);
  }

  void draw() {
    const GLFWvidmode * videoMode = glfwGetVideoMode(monitor);
    glm::ivec2 pos;
    glfwGetMonitorPos(monitor, &pos.x, &pos.y);

    glm::ivec2 position(0, 0);
    gl::scissor(position, eyeSize);
    gl::clearColor(Colors::red);
    glClear(GL_COLOR_BUFFER_BIT);

    position = ivec2(eyeSize.x, 0);
    gl::scissor(position, eyeSize);
    gl::clearColor(Colors::blue);
    glClear(GL_COLOR_BUFFER_BIT);
  }
};

class Windowed : public DisplayTargeting {
public:
  virtual void createRenderingTarget() {
    const GLFWvidmode * videoMode = glfwGetVideoMode(monitor);
    glfwWindowHint(GLFW_DECORATED, 0);
    createWindow(
      videoMode->width, videoMode->height, 
      hmdInfo.DesktopX, hmdInfo.DesktopY);
    if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
      FAIL("Unable to create undecorated window");
    }
    eyeSize = ivec2(videoMode->width / 2, videoMode->height);
  }
};

class Fullscreen : public DisplayTargeting {
public:
  virtual void createRenderingTarget() {
    createFullscreenWindow(glm::uvec2(hmdInfo.HResolution, hmdInfo.VResolution), monitor);
    eyeSize = ivec2(hmdInfo.HResolution / 2, hmdInfo.VResolution);
  }
};

RUN_OVR_APP(Windowed);
