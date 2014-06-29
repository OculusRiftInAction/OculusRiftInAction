#include "Common.h"
#include "CubeScene.h"

static const glm::uvec2 WINDOW_SIZE(1280, 800);
static const glm::ivec2 WINDOW_POS(100, 100);

class CubeScene_Mono : public CubeScene {
public:
  CubeScene_Mono() {
    gl::Stacks::projection().top() = glm::perspective(
        PI / 2.0f, glm::aspect(WINDOW_SIZE), 0.01f, 100.0f);
  }

  virtual void createRenderingTarget() {
    createWindow(WINDOW_SIZE, WINDOW_POS);
  }

  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawCubeScene();
  }
};

RUN_OVR_APP(CubeScene_Mono);
