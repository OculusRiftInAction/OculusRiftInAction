#include "Common.h"

using namespace std;
using namespace gl;
using namespace OVR;

class PostProcessDistort : public RiftRenderApp {
protected:
  Texture2dPtr texture;
  GeometryPtr geometry;
  ProgramPtr program;

public:

  void initGl() {
    GlfwApp::initGl();
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
    bindUniforms(program);
    Program::clear();
  }

  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    program->use();
    texture->bind();
    geometry->bindVertexArray();

    eye[0].viewport();
    eye[0].bindUniforms(program);
    geometry->draw();

    eye[1].viewport();
    eye[1].bindUniforms(program);
    geometry->draw();

    VertexArray::unbind();
    texture->unbind();
    Program::clear();
  }
};

RUN_OVR_APP(PostProcessDistort)
