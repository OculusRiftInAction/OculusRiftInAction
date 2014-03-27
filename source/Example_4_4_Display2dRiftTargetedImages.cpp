#include "Common.h"

class Display2dRiftTargeted : public RiftGlfwApp {
protected:

  std::map<StereoEye, gl::Texture2dPtr> texture;
  gl::GeometryPtr quadGeometry;
  gl::ProgramPtr program;

public:

  void initGl() {
    GlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    program = GlUtils::getProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_TEXTURED_FS);

    GlUtils::getImageAsTexture( texture[LEFT],
        Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_PNG);
    GlUtils::getImageAsTexture( texture[RIGHT],
        Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_PNG);
    quadGeometry = GlUtils::getQuadGeometry();
  }

  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    program->use();
    quadGeometry->bindVertexArray();

    for_each_eye([&](OVR::Util::Render::StereoEye eye){
      viewport(eye);
      texture[eye]->bind();
      quadGeometry->draw();
    });

    gl::VertexArray::unbind();
    gl::Texture2d::unbind();
    gl::Program::clear();
  }
};

RUN_OVR_APP(Display2dRiftTargeted)
