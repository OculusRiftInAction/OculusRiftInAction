#include "Common.h"

using namespace std;
using namespace gl;
using namespace OVR;

class Display2dRiftTargeted : public GlfwApp {
protected:
  Texture2dPtr texture;
  GeometryPtr quadGeometries[2];
  ProgramPtr program;
  glm::uvec2 eyeSize;
  float eyeAspect;

public:

  Display2dRiftTargeted() {
    OVR::Ptr<OVR::DeviceManager> ovrManager;
    ovrManager = *OVR::DeviceManager::Create();
    HMDInfo hmdInfo;
    Rift::getHmdInfo(ovrManager, hmdInfo);
    Rift::getRiftPositionAndSize(hmdInfo, windowPosition, windowSize);
    eyeSize = windowSize;
    eyeSize.x /= 2;
    eyeAspect = ((float)hmdInfo.HResolution / 2.0f) / (float)hmdInfo.VResolution;
  }

  virtual void createRenderingTarget() {
    glfwWindowHint(GLFW_DECORATED, 0);
    createWindow(windowSize, windowPosition);
    if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
      FAIL("Unable to create undecorated window");
    }
  }

  void initGl() {
    GlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    glm::uvec2 imageSize;
    GlUtils::getImageAsTexture(
        texture,
        Resource::IMAGES_TUSCANY_UNDISTORTED_PNG,
        imageSize);
    quadGeometries[0] =
      GlUtils::getQuadGeometry(
          glm::vec2(-1.0), glm::vec2(1.0),
          glm::vec2(0.0f, 0.0f), glm::vec2(0.5f, 1.0f));
    quadGeometries[1] =
      GlUtils::getQuadGeometry(
          glm::vec2(-1.0), glm::vec2(1.0),
          glm::vec2(0.5f, 0.0f), glm::vec2(1.0f, 1.0f));

    program = GlUtils::getProgram(
      Resource::SHADERS_TEXTURE_VS,
      Resource::SHADERS_TEXTURE_FS);

    program->use();
    program->setUniform("ViewportAspectRatio", eyeAspect);
    Program::clear();
  }

  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    program->use();
    texture->bind();

    glm::uvec2 position(0, 0);
    gl::viewport(position, eyeSize);
    quadGeometries[0]->bindVertexArray();
    quadGeometries[0]->draw();

    position.x = eyeSize.x;
    gl::viewport(position, eyeSize);
    quadGeometries[1]->bindVertexArray();
    quadGeometries[1]->draw();

    VertexArray::unbind();
    Texture2d::unbind();
    Program::clear();
  }
};

RUN_OVR_APP(Display2dRiftTargeted)
