#include "Common.h"

class DisplayTargetingExample : public RiftGlfwApp {
public:
  DisplayTargetingExample(bool fullscreen) : RiftGlfwApp(fullscreen) {
  }

  void initGl() {
    RiftGlfwApp::initGl();
    glEnable(GL_SCISSOR_TEST);
  }

  void draw() {
    glm::ivec2 position(0, 0);
    glm::uvec2 eyeSize(hmd->Resolution.w / 2, hmd->Resolution.h);
    gl::scissor(position, eyeSize);
    gl::clearColor(Colors::red);
    glClear(GL_COLOR_BUFFER_BIT);

    position = glm::ivec2(eyeSize.x, 0);
    gl::scissor(position, eyeSize);
    gl::clearColor(Colors::blue);
    glClear(GL_COLOR_BUFFER_BIT);
  }
};

class Windowed : public DisplayTargetingExample {
public:
  Windowed() : DisplayTargetingExample(false) {}
};

class Fullscreen : public DisplayTargetingExample {
public:
  Fullscreen() : DisplayTargetingExample(true) {}
};

RUN_OVR_APP(Windowed);
