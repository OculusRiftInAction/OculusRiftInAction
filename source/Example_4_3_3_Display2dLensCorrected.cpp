#include "Common.h"

class Display2d : public RiftGlfwApp {
protected:
  gl::Texture2dPtr texture;
  gl::GeometryPtr quadGeometry;
  gl::ProgramPtr program;
  glm::mat4 projections[2];

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

    program = GlUtils::getProgram(
        Resource::SHADERS_TEXTURED_VS,
        Resource::SHADERS_TEXTURED_FS);

    float lensOffset = 1.0f - (2.0f * ovrHmdInfo.LensSeparationDistance / ovrHmdInfo.HScreenSize);

    for (int eye = 0; eye <= 1; eye++) {
      float eyeLensOffset = (eye == LEFT ? -lensOffset : lensOffset);
      projections[eye] = glm::ortho(
          -1.0f + eyeLensOffset, 1.0f + eyeLensOffset,
          -1.0f / eyeAspect, 1.0f / eyeAspect);
    }
  }

  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    program->use();
    texture->bind();
    quadGeometry->bindVertexArray();

    for (int eye = 0; eye <= 1; eye++) {
      viewport(eye);
      program->setUniform("Projection", projections[eye]);
      quadGeometry->draw();
    }

    gl::VertexArray::unbind();
    gl::Texture2d::unbind();
    gl::Program::clear();
  }
};

RUN_OVR_APP(Display2d)
