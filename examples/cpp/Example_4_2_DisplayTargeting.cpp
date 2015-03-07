#include "Common.h"

class DisplayTargetingExample : public RiftGlfwApp {
public:
  DisplayTargetingExample(bool fullscreen) : RiftGlfwApp(fullscreen) {
  }

  void initGl() {
    RiftGlfwApp::initGl();
  }

  void draw() {
    // A bug in some versions of the SDK prevents Direct Mode from engaging properly unless you call the GetEyePoses function
    {
      static ovrVector3f offsets[2];
      static ovrPosef poses[2];
      ovrHmd_GetEyePoses(hmd, 0, offsets, poses, nullptr);
    }
    glm::uvec2 eyeSize(hmd->Resolution.w / 2, hmd->Resolution.h);
    glm::ivec2 position = glm::ivec2(0, 0);
    glm::vec4 color = glm::vec4(1, 0, 0, 1);

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

class Windowed : public DisplayTargetingExample {
public:
  Windowed() : DisplayTargetingExample(false) {}
};

class Fullscreen : public DisplayTargetingExample {
public:
  Fullscreen() : DisplayTargetingExample(true) {}
};

RUN_OVR_APP(Windowed);
