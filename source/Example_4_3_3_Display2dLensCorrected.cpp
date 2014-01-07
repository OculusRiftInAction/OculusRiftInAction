#include "Common.h"

using namespace std;
using namespace gl;
using namespace OVR;

class Display2dLensCorrected : public GlfwApp {
protected:
  HMDInfo hmdInfo;
  Texture2dPtr texture;
  GeometryPtr quadGeometry;
  ProgramPtr program;
  int eyeWidth;

public:

  Display2dLensCorrected() {
    OVR::Ptr<OVR::DeviceManager> ovrManager;
    ovrManager = *OVR::DeviceManager::Create();
    Rift::getHmdInfo(ovrManager, hmdInfo);
    eyeWidth = hmdInfo.HResolution / 2;
  }

  void createRenderingTarget() {
    glfwWindowHint(GLFW_DECORATED, 0);
    createWindow(
        hmdInfo.HResolution, hmdInfo.VResolution,
        hmdInfo.DesktopX, hmdInfo.DesktopY
    );
    if (glfwGetWindowAttrib(window, GLFW_DECORATED)) {
      FAIL("Unable to create undecorated window");
    }
  }

  void initGl() {
    GlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

    float viewportAspectRatio = (float) eyeWidth / (float) hmdInfo.VResolution;

    glm::ivec2 imageSize;
    GlUtils::getImageAsTexture(texture,
        // Resource::IMAGES_SPACE_NEEDLE_PNG,
        // Resource::IMAGES_TALLMAN_CARGO_PNG,
        Resource::IMAGES_SHOULDER_CAT_PNG,
        imageSize);

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
		Resource::SHADERS_TEXTURERIFT_VS,
		Resource::SHADERS_TEXTURE_FS);
    program->use();
    program->setUniform("ViewportAspectRatio", viewportAspectRatio);

    float lensDistance =
        hmdInfo.LensSeparationDistance /
        hmdInfo.HScreenSize;
    float lensOffset =
        1.0f - (2.0f * lensDistance);

    program->setUniform("LensOffset", lensOffset);
    Program::clear();
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    program->use();
    texture->bind();

    quadGeometry->bindVertexArray();

    glViewport(0, 0, eyeWidth, hmdInfo.VResolution);
    program->setUniform("Mirror", 0);
    quadGeometry->draw();

    glViewport(eyeWidth, 0, eyeWidth, hmdInfo.VResolution);
    program->setUniform("Mirror", 1);
    quadGeometry->draw();

    VertexArray::unbind();
    Texture2d::unbind();
    Program::clear();
  }
};

RUN_OVR_APP(Display2dLensCorrected)

