#include "Common.h"

using namespace std;
using namespace gl;
using namespace OVR;

class Example_5_3_5 : public GlfwApp {
protected:
  HMDInfo hmdInfo;
  Texture2dPtr texture;
  GeometryPtr quadGeometries[2];
  ProgramPtr program;
  int eyeWidth;

public:

  Example_5_3_5() {
    Rift::getHmdInfo(hmdInfo);
    eyeWidth = hmdInfo.HResolution / 2;
  }

  virtual void createRenderingTarget() {
    glfwWindowHint(GLFW_DECORATED, 0);
    createWindow(
        hmdInfo.HResolution, hmdInfo.VResolution,
        hmdInfo.DesktopX, hmdInfo.DesktopY
        );
    if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
      FAIL("Unable to create undecorated window");
    }
  }

  // This simple program only uses a single program, texture and set of
  // geometry, so we can bind all of them here at the start, rather than
  // every frame
  void initGl() {
    GlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    glm::ivec2 imageSize;
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
      ShaderResource::SHADERS_TEXTURE_VS,
      ShaderResource::SHADERS_TEXTURE_FS);
  }

  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    program->use();
    texture->bind();

    glViewport(0, 0, eyeWidth, hmdInfo.VResolution);
    quadGeometries[0]->bindVertexArray();
    quadGeometries[0]->draw();

    glViewport(eyeWidth, 0, eyeWidth, hmdInfo.VResolution);
    quadGeometries[1]->bindVertexArray();
    quadGeometries[1]->draw();

    VertexArray::unbind();
    Texture2d::unbind();
    Program::clear();
  }
};

RUN_OVR_APP(Example_5_3_5)
