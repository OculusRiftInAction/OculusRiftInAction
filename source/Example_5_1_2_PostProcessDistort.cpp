#include "Common.h"

using namespace std;
using namespace gl;
using namespace OVR;

class PostProcessDistort : public RiftGlfwApp {
protected:
  Texture2dPtr texture;
  GeometryPtr geometry;
  ProgramPtr program;

public:

  void initGl() {
    RiftGlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    GlUtils::getImageAsGeometryAndTexture(
        Resource::IMAGES_SHOULDER_CAT_PNG,
        geometry, texture);

    program = GlUtils::getProgram(
      Resource::SHADERS_EXAMPLE_5_1_2_VS,
      Resource::SHADERS_EXAMPLE_5_1_2_FS);

    program->use();
    Program::clear();
  }

  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    program->use();
    texture->bind();
    geometry->bindVertexArray();

    for (int i = 0; i < 2; ++i) {
      viewport(i);
      geometry->draw();
    }

    VertexArray::unbind();
    texture->unbind();
    Program::clear();
  }
};

RUN_OVR_APP(PostProcessDistort)
