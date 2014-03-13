#include "Common.h"

class Display2d : public RiftGlfwApp {
protected:
  gl::Texture2dPtr texture;
  gl::GeometryPtr quadGeometry;
  gl::ProgramPtr program;
  glm::mat4 projection;

public:

  void initGl() {
    RiftGlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glm::uvec2 imageSize;
    GlUtils::getImageAsTexture(texture,
        Resource::IMAGES_SHOULDER_CAT_PNG,
        imageSize);

    float imageAspect = (float) imageSize.x / (float) imageSize.y;
    if (imageAspect > eyeAspect) {
      quadGeometry = GlUtils::getQuadGeometry(
          glm::vec2(-1.0f, -1.0f / imageAspect),
          glm::vec2( 1.0f,  1.0f / imageAspect));
    } else {
      quadGeometry = GlUtils::getQuadGeometry(
          glm::vec2(-imageAspect / eyeAspect, -1.0f / eyeAspect),
          glm::vec2( imageAspect / eyeAspect,  1.0f / eyeAspect));
    }

    projection = glm::ortho(
        -1.0f, 1.0f,
        -1.0f / eyeAspect, 1.0f / eyeAspect);

    program = GlUtils::getProgram(
        Resource::SHADERS_TEXTURED_VS,
        Resource::SHADERS_TEXTURED_FS);
  }

  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    program->use();
    program->setUniform("Projection", projection);

    texture->bind();
    quadGeometry->bindVertexArray();

    for (int eye = 0; eye <= 1; eye++) {
      viewport(eye);
      quadGeometry->draw();
    }

    gl::VertexArray::unbind();
    gl::Texture2d::unbind();
    gl::Program::clear();
  }
};

RUN_OVR_APP(Display2d)
