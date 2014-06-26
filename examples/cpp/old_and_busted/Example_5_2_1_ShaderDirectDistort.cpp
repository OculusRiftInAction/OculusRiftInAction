#include "Common.h"

std::map<StereoEye, Resource> SCENE_IMAGES = {
  { LEFT, Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_PNG},
  { RIGHT, Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_PNG }
};

class ShaderDirectDistortionExample : public RiftGlfwApp {

protected:
  std::map<StereoEye, gl::Texture2dPtr> textures;
  gl::GeometryPtr quadGeometry;
  glm::vec4 K;
  float lensOffset;
  gl::ProgramPtr program;

public:
  ShaderDirectDistortionExample() {
    OVR::Util::Render::StereoConfig stereoConfig;
    stereoConfig.SetHMDInfo(ovrHmdInfo);
    const OVR::Util::Render::DistortionConfig & distortion =
        stereoConfig.GetDistortionConfig();

    float postDistortionScale = 1.0f / stereoConfig.GetDistortionScale();
    for (int i = 0; i < 4; ++i) {
      K[i] = distortion.K[i] * postDistortionScale;
    }
    lensOffset = distortion.XCenterOffset;
  }

  void initGl() {
    RiftGlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    program = GlUtils::getProgram(
        Resource::SHADERS_TEXTURED_VS,
        Resource::SHADERS_RIFTWARPDIRECT_FS);
    program->use();
    program->setUniform("Aspect", eyeAspect);
    program->setUniform("K", K);

    quadGeometry = GlUtils::getQuadGeometry();
    quadGeometry->bindVertexArray();

    for_each_eye([&](StereoEye eye) {
      GlUtils::getImageAsTexture(textures[eye], SCENE_IMAGES[eye]);
    });
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    for_each_eye([&](StereoEye eye ){
      renderEye(eye);
    });
  }

  void renderEye(StereoEye eye) {
    viewport(eye);
    program->setUniform("LensOffset", (eye == LEFT) ? -lensOffset : lensOffset);
    textures[eye]->bind();
    quadGeometry->draw();
  }
};


RUN_OVR_APP(ShaderDirectDistortionExample)
