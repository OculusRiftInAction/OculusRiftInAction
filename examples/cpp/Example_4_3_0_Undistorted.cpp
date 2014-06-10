#include "Common.h"
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

Resource SCENE_IMAGES[2] = {
  Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_PNG,
  Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_PNG
};

class UndistortedExample : public RiftGlfwApp {

protected:
  gl::Texture2dPtr textures[2];
  gl::GeometryPtr quadGeometry;
  gl::ProgramPtr program;

public:
  virtual ~UndistortedExample() {
    ovrHmd_Destroy(hmd);
  }

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

    for_each_eye([&](ovrEyeType eye) {
      GlUtils::getImageAsTexture(textures[eye], SCENE_IMAGES[eye]);
    });
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    for_each_eye([&](ovrEyeType eye) {
      renderEye(eye);
    });
  }

  void renderEye(ovrEyeType eye) {
    viewport(eye);
    textures[eye]->bind();
    quadGeometry->draw();
  }
};

RUN_OVR_APP(UndistortedExample)
