#include "Common.h"

const Resource SCENE_IMAGES[2] = {
  Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_PNG,
  Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_PNG
};

class ShaderDirectDistortionExample : public RiftGlfwApp {

protected:
  gl::Texture2dPtr textures[2];
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

    for (int eyeIndex = 0; eyeIndex < 2; ++eyeIndex) {
      GlUtils::getImageAsTexture(textures[eyeIndex], SCENE_IMAGES[eyeIndex]);
    }
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    FOR_EACH_EYE(eye) {
      renderEye(eye);
    }
  }

  void renderEye(int eyeIndex) {
    viewport(eyeIndex);
    program->setUniform("LensOffset", (eyeIndex == 0) ? -lensOffset : lensOffset);
    textures[eyeIndex]->bind();
    quadGeometry->draw();
  }
};


RUN_OVR_APP(ShaderDirectDistortionExample)
