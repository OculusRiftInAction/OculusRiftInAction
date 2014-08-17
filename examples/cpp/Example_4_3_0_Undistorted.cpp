#include "Common.h"

Resource SCENE_IMAGES_DK1[2] = {
  Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_DK1_PNG,
  Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_DK1_PNG
};

Resource SCENE_IMAGES_DK2[2] = {
  Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_DK2_PNG,
  Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_DK2_PNG
};

class UndistortedExample : public RiftGlfwApp {

protected:
  gl::Texture2dPtr textures[2];
  gl::GeometryPtr quadGeometry;
  gl::ProgramPtr program;

public:
  virtual ~UndistortedExample() {
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

    Resource * sceneImages = SCENE_IMAGES_DK2;
    if (hmd->Type == ovrHmd_DK1) {
      sceneImages = SCENE_IMAGES_DK1;
    }

    for_each_eye([&](ovrEyeType eye) {
      GlUtils::getImageAsTexture(textures[eye], sceneImages[eye]);
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
