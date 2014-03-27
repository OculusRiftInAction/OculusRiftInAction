#include "Common.h"

std::map<StereoEye, Resource> SCENE_IMAGES = {
  { LEFT, Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_PNG},
  { RIGHT, Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_PNG }
};

class UndistortedExample : public RiftGlfwApp {

protected:
  std::map<StereoEye, gl::Texture2dPtr> textures;
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

    for_each_eye([&](StereoEye eye){
      Resource image = SCENE_IMAGES[eye];
      GlUtils::getImageAsTexture(textures[eye], image);
    });
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    for_each_eye([&](StereoEye eye){
      renderEye(eye);
    });
  }

  void renderEye(StereoEye eye) {
    viewport(eye);
    textures[eye]->bind();
    quadGeometry->draw();
  }
};

RUN_OVR_APP(UndistortedExample)
