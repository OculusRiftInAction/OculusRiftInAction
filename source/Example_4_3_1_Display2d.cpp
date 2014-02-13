#include "Common.h"

using namespace std;
using namespace gl;
using namespace OVR;

class Display2d : public GlfwApp {
protected:
  Texture2dPtr texture;
  GeometryPtr quadGeometry;
  ProgramPtr program;
  glm::uvec2 eyeSize;

public:

  Display2d() {
    OVR::Ptr<OVR::DeviceManager> ovrManager;
    ovrManager = *OVR::DeviceManager::Create();
    HMDInfo hmdInfo;
    Rift::getHmdInfo(ovrManager, hmdInfo);
    Rift::getRiftPositionAndSize(hmdInfo, windowPosition, windowSize);
    eyeSize = windowSize;
    eyeSize.x /= 2;
  }

  virtual void createRenderingTarget() {
    glfwWindowHint(GLFW_DECORATED, 0);
    createWindow(windowSize,windowPosition);
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
    GlUtils::getImageAsTexture(texture,
        Resource::IMAGES_SHOULDER_CAT_PNG,
        imageSize);

    quadGeometry = GlUtils::getQuadGeometry(
        glm::vec2(-1.0f, -1.0f),
        glm::vec2(1.0f, 1.0f));
    program = GlUtils::getProgram(
		Resource::SHADERS_TEXTURED_VS,
		Resource::SHADERS_TEXTURED_FS);

  }

  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    program->use();
    texture->bind();

    quadGeometry->bindVertexArray();

    glm::uvec2 position(0, 0);
    gl::viewport(position, eyeSize);
    quadGeometry->draw();

    position.x = eyeSize.x;
    gl::viewport(position, eyeSize);
    quadGeometry->draw();

    VertexArray::unbind();

    Texture2d::unbind();
    Program::clear();
  }
};

RUN_OVR_APP(Display2d)
