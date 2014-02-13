#include "Common.h"

using namespace std;
using namespace gl;
using namespace OVR;

class Display2d : public RiftGlfwApp {
protected:
  Texture2dPtr texture;
  GeometryPtr quadGeometry;
  ProgramPtr program;
  glm::mat4 projections[2];

public:

  void initGl() {
    GlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glm::uvec2 imageSize;
    GlUtils::getImageAsTexture(texture,
        Resource::IMAGES_SHOULDER_CAT_PNG,
        imageSize);

    float imageAspect = (float)imageSize.x / (float)imageSize.y;
    quadGeometry = GlUtils::getQuadGeometry(
      glm::vec2(-1.0f, -1.0f / imageAspect),
      glm::vec2(1.0f, 1.0f / imageAspect));

    program = GlUtils::getProgram(
		  Resource::SHADERS_TEXTURED_VS,
		  Resource::SHADERS_TEXTURED_FS);

    OVR::HMDInfo hmdInfo;
    Rift::getHmdInfo(ovrManager, hmdInfo);
    float lensOffset = 1.0f
      - (2.0f * hmdInfo.LensSeparationDistance / hmdInfo.HScreenSize);

    for (int i = 0; i < 2; ++i) {
      float eyeLensOffset = i == 0 ? -lensOffset : lensOffset;
      projections[i] = glm::ortho(
        -1.0f + eyeLensOffset, 1.0f + eyeLensOffset, 
        -1.0f / eyeAspect, 1.0f / eyeAspect);
    }
  }

  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    program->use();
    texture->bind();
    quadGeometry->bindVertexArray();

    for (int i = 0; i < 2; ++i) {
      viewport(i);
      program->setUniform("Projection", projections[i]);
      quadGeometry->draw();
    }

    VertexArray::unbind();
    Texture2d::unbind();
    Program::clear();
  }
};

RUN_OVR_APP(Display2d)
