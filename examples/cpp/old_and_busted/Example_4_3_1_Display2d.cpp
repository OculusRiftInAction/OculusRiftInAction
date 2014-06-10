#include "Common.h"

class Display2d : public RiftGlfwApp {
protected:
  gl::Texture2dPtr texture;
  gl::GeometryPtr quadGeometry;
  gl::ProgramPtr program;

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
    quadGeometry = GlUtils::getQuadGeometry(
        glm::vec2(-1.0f, -1.0f),
        glm::vec2(1.0f, 1.0f));

    program = GlUtils::getProgram(
        Resource::SHADERS_TEXTURED_VS,
        Resource::SHADERS_TEXTURED_FS);
  }

  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    program->use();
    texture->bind();
    quadGeometry->bindVertexArray();

    for_each_eye([&](StereoEye eye) {
      viewport(eye);
      quadGeometry->draw();
    });

    gl::VertexArray::unbind();
    gl::Texture2d::unbind();
    gl::Program::clear();
  }
};

RUN_OVR_APP(Display2d)
