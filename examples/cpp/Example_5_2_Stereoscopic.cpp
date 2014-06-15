#include "Common.h"
#include "Chapter_5.h"

static const glm::uvec2 WINDOW_SIZE(1280, 800);
static const glm::ivec2 WINDOW_POS(100, 100);

static const glm::uvec2 EYE_SIZE(WINDOW_SIZE.x / 2, WINDOW_SIZE.y);
static const float EYE_ASPECT = glm::aspect(EYE_SIZE);

struct PerEyeArg {
  glm::uvec2 viewportPosition;
  glm::mat4 modelviewOffset;
};

class SimpleScene : public Chapter_5 {
  PerEyeArg       eyes[2];

public:
  SimpleScene() {
    gl::Stacks::projection().top() = glm::perspective(
        PI / 2.0f, EYE_ASPECT, 0.01f, 100.0f);

    eyes[ovrEye_Left].viewportPosition = glm::uvec2(0, 0);
    eyes[ovrEye_Left].modelviewOffset = glm::translate(glm::mat4(),
        glm::vec3(ipd / 2.0f, 0, 0));

    eyes[ovrEye_Right].viewportPosition = glm::uvec2(WINDOW_SIZE.x / 2, 0);
    eyes[ovrEye_Right].modelviewOffset = glm::translate(glm::mat4(),
        glm::vec3(-ipd / 2.0f, 0, 0));
  }

  virtual void createRenderingTarget() {
    createWindow(WINDOW_SIZE, WINDOW_POS);
  }

  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::MatrixStack & mv = gl::Stacks::modelview();

    for (int i = 0; i < ovrEye_Count; ++i) {
      ovrEyeType eye = hmdDesc.EyeRenderOrder[i];
      PerEyeArg & eyeArgs = eyes[eye];
      gl::viewport(eyeArgs.viewportPosition, EYE_SIZE);
      gl::Stacks::with_push(mv, [&]{
        mv.preMultiply(eyeArgs.modelviewOffset);
        drawChapter5Scene();
      });
    };
  }
};

RUN_OVR_APP(SimpleScene);
