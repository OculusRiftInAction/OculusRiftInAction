#include "Common.h"

using namespace std;
using namespace gl;
using namespace OVR;

class PostProcessBlur : public RiftGlfwApp {
protected:
  Texture2dPtr texture;
  GeometryPtr quadGeometry;
  ProgramPtr program;
  int eyeWidth;

public:

  PostProcessBlur() {
  }

  void initGl() {
    RiftGlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    glm::uvec2 imageSize;
    GlUtils::getImageAsTexture(
      texture,
      Resource::IMAGES_SHOULDER_CAT_PNG,
      imageSize);

    float imageAspectRatio = (float) imageSize.x / (float) imageSize.y;
    glm::vec2 geometryMax(1.0f, 1.0f / imageAspectRatio);
    if (imageAspectRatio < eyeAspect) {
      float scale = imageAspectRatio / eyeAspect;
      geometryMax *= scale;
    }

    glm::vec2 geometryMin = geometryMax * -1.0f;
    quadGeometry = GlUtils::getQuadGeometry(
        geometryMin, geometryMax);

    program = GlUtils::getProgram(
      Resource::SHADERS_EXAMPLE_5_1_1_VS,
      Resource::SHADERS_EXAMPLE_5_1_1_FS);
    program->use();
    program->setUniform("ViewportAspectRatio", eyeAspect);
    Program::clear();
  }

  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    program->use();
    texture->bind();
    quadGeometry->bindVertexArray();

    for (int i = 0; i < 2; ++i) {
      viewport(i);
      quadGeometry->draw();
    }

    VertexArray::unbind();
    Texture2d::unbind();
    Program::clear();
  }
};

RUN_OVR_APP(PostProcessBlur)
