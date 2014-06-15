#include "Common.h"
#include "Chapter_5.h"

static const glm::uvec2 WINDOW_SIZE(1280, 800);
static const glm::ivec2 WINDOW_POS(100, 100);

class SimpleScene: public Chapter_5 {

public:
  SimpleScene() {
    gl::Stacks::projection().top() = glm::perspective(
        PI / 2.0f, glm::aspect(WINDOW_SIZE), 0.01f, 100.0f);
  }

  virtual void createRenderingTarget() {
    createWindow(WINDOW_SIZE, WINDOW_POS);
  }

  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawChapter5Scene();
  }
};

RUN_OVR_APP(SimpleScene);
