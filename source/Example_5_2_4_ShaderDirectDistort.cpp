#include "Common.h"

using namespace std;
using namespace gl;
using namespace OVR;

const Resource SCENE_IMAGES[2] = {
  Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_PNG,
  Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_PNG
};

#define DISTORTION_TIMING 1
class PostProcessDistortRift : public RiftGlfwApp {
protected:
  Texture2dPtr sceneTextures[2];
  GeometryPtr quadGeometry;
  glm::vec4 K;
  float lensOffset;

public:
  PostProcessDistortRift() {
    OVR::HMDInfo hmdInfo;
    Rift::getHmdInfo(ovrManager, hmdInfo);
    OVR::Util::Render::DistortionConfig Distortion;
    OVR::Util::Render::StereoConfig stereoConfig;
    stereoConfig.SetHMDInfo(hmdInfo);
    Distortion = stereoConfig.GetDistortionConfig();

    float postDistortionScale = 1.0 / stereoConfig.GetDistortionScale();
    for (int i = 0; i < 4; ++i) {
      K[i] = Distortion.K[i] * postDistortionScale;
    }
    lensOffset = 1.0f - (2.0f *
      hmdInfo.LensSeparationDistance /
      hmdInfo.HScreenSize);
  }

  void initGl() {
    RiftGlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    quadGeometry = GlUtils::getQuadGeometry();

    for (int eyeIndex = 0; eyeIndex < 2; ++eyeIndex) {
      // load the scene textures
      GlUtils::getImageAsTexture(sceneTextures[eyeIndex], SCENE_IMAGES[eyeIndex]);
    }
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    for (int eyeIndex = 0; eyeIndex < 2; ++eyeIndex) {
      renderEye(eyeIndex);
    }
    GL_CHECK_ERROR;
  }

  void renderEye(int eyeIndex) {
    viewport(eyeIndex);
    ProgramPtr distortProgram = GlUtils::getProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_EXAMPLE_5_2_4_WARP_FS);

    distortProgram->use();
    sceneTextures[eyeIndex]->bind();
    distortProgram->setUniform("LensOffset", (eyeIndex == 0) ? -lensOffset : lensOffset);
    distortProgram->setUniform("Aspect", eyeAspect);
    distortProgram->setUniform("K", K);

    quadGeometry->bindVertexArray();
#ifdef DISTORTION_TIMING
    query->begin();
#endif

    quadGeometry->draw();

#ifdef DISTORTION_TIMING
    query->end();
    static long accumulator = 0;
    static long count = 0;
    accumulator += query->getReult();
    count++;
    SAY("%d ns", accumulator / count);
#endif

    VertexArray::unbind();
    Texture2d::unbind();
    Program::clear();
  }
};


RUN_OVR_APP(PostProcessDistortRift)
