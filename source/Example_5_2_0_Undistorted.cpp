#include "Common.h"

const Resource SCENE_IMAGES[2] = {
  Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_PNG,
  Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_PNG
};

class UndistortedExample : public RiftGlfwApp {

protected:
  gl::Texture2dPtr textures[2];
  gl::GeometryPtr quadGeometry;
  gl::ProgramPtr program;

public:

  void initGl() {
    RiftGlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    program = GlUtils::getProgram(
        Resource::SHADERS_TEXTURED_VS,
        Resource::SHADERS_TEXTURED_FS);
    program->use();

    quadGeometry = GlUtils::getQuadGeometry();
    quadGeometry->bindVertexArray();

    for (int eyeIndex = 0; eyeIndex < 2; ++eyeIndex) {
      GlUtils::getImageAsTexture(textures[eyeIndex], SCENE_IMAGES[eyeIndex]);
    }
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    for (int eyeIndex = 0; eyeIndex < 2; ++eyeIndex) {
      renderEye(eyeIndex);
    }
  }

  void renderEye(int eyeIndex) {
    viewport(eyeIndex);
    textures[eyeIndex]->bind();
    quadGeometry->draw();
  }
};

RUN_OVR_APP(UndistortedExample)
