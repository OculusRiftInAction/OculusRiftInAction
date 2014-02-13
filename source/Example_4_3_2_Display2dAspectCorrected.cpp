#include "Common.h"

using namespace std;
using namespace gl;
using namespace OVR;

class Display2dAspectCorrected : public GlfwApp {
protected:
  Texture2dPtr texture;
  GeometryPtr quadGeometry;
  ProgramPtr program;
  glm::uvec2 eyeSize;
  float eyeAspect;

public:

  Display2dAspectCorrected() {
    OVR::Ptr<OVR::DeviceManager> ovrManager;
    ovrManager = *OVR::DeviceManager::Create();
    HMDInfo hmdInfo;
    Rift::getHmdInfo(ovrManager, hmdInfo);
    Rift::getRiftPositionAndSize(hmdInfo, windowPosition, windowSize);
    eyeSize = windowSize;
    eyeSize.x /= 2;
    eyeAspect = ((float)hmdInfo.HResolution / 2.0f) / (float)hmdInfo.VResolution;
  }

  void createRenderingTarget() {
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
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

    glm::uvec2 imageSize;
    GlUtils::getImageAsTexture(
      texture,
      Resource::IMAGES_SHOULDER_CAT_PNG,
      imageSize);

    float viewportAspectRatio = eyeAspect;
    float imageAspectRatio = (float) imageSize.x / (float) imageSize.y;
    glm::vec2 geometryMax(1.0f, 1.0f / imageAspectRatio);

    if (imageAspectRatio < viewportAspectRatio) {
      float scale = imageAspectRatio / viewportAspectRatio;
      geometryMax *= scale;
    }

    glm::vec2 geometryMin = geometryMax * -1.0f;
    quadGeometry = GlUtils::getQuadGeometry(
      geometryMin, geometryMax);

    program = GlUtils::getProgram(
		  Resource::SHADERS_TEXTURED_VS,
		  Resource::SHADERS_TEXTURED_FS);
    program->use();
    program->setUniform("ViewportAspectRatio", viewportAspectRatio);
    Program::clear();
  }

  void draw() {
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

RUN_OVR_APP(Display2dAspectCorrected)
