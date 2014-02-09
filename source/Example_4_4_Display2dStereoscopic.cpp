#include "Common.h"

using namespace std;
using namespace gl;
using namespace OVR;

class Example_5_3_4 : public GlfwApp {
protected:
  Texture2dPtr texture;
  GeometryPtr quadGeometries[2];
  ProgramPtr program;
  glm::uvec2 eyeSize;
  float eyeAspect;
  float lensOffset;
  Resource imageResource;
  bool stereo;

public:

  Example_5_3_4() {
    OVR::Ptr<OVR::DeviceManager> ovrManager;
    ovrManager = *OVR::DeviceManager::Create();
    HMDInfo hmdInfo;
    Rift::getHmdInfo(ovrManager, hmdInfo);
    Rift::getRiftPositionAndSize(hmdInfo, windowPosition, windowSize);
    eyeSize = windowSize;
    eyeSize.x /= 2;
    eyeAspect = ((float)hmdInfo.HResolution / 2.0f) / (float)hmdInfo.VResolution;
    float lensDistance =
      hmdInfo.LensSeparationDistance /
      hmdInfo.HScreenSize;
    lensOffset =
      1.0f - (2.0f * lensDistance);


    imageResource = Resource::IMAGES_STEREO_VLCSNAP_2_13_11_16_12H41M46S16__PNG;
    stereo = true;
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

    float viewportAspectRatio = eyeAspect;
    glm::vec2 projectionMax(1.0f, 1.0f / viewportAspectRatio);
    glm::vec2 projectionMin = projectionMax * -1.0f;
    glm::mat4 projection = glm::ortho(
        projectionMin.x, projectionMax.x,
        projectionMin.y, projectionMax.y);

    loadImageAndGeometry();

    program = GlUtils::getProgram(
        Resource::SHADERS_TEXTURERIFT_VS,
        Resource::SHADERS_TEXTURE_FS);
    program->use();
    program->setUniform("Projection", projection);
    Program::clear();
  }

  void loadImageAndGeometry() {
    glm::uvec2 imageSize;
    GlUtils::getImageAsTexture(texture, imageResource, imageSize);
    if (!(imageResource >= IMAGES_STEREO_VLCSNAP_2_13_11_16_12H41M46S16__PNG &&
        imageResource <= IMAGES_STEREO_VLCSNAP_2_13_11_16_2_H_9M43S38_PNG)) {
      imageSize.x /= 2;
    }

    float viewportAspectRatio = eyeAspect;
    float imageAspectRatio = (float) imageSize.x / (float) imageSize.y;
    glm::vec2 geometryMax(1.0f, 1.0f / imageAspectRatio);

    if (imageAspectRatio < viewportAspectRatio) {
      float scale = imageAspectRatio / viewportAspectRatio;
      geometryMax *= scale;
    }

    glm::vec2 geometryMin = geometryMax * -1.0f;
    quadGeometries[0] =
        GlUtils::getQuadGeometry(
            geometryMin, geometryMax,
            glm::vec2(0.0f, 0.0f), glm::vec2(0.5f, 1.0f));


    quadGeometries[1] =
        GlUtils::getQuadGeometry(
            geometryMin, geometryMax,
            glm::vec2(0.5f, 0.0f), glm::vec2(1.0f, 1.0f));
  }

  void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action) {
      return;
    }

    int newResource = imageResource;
    switch (key) {
    case GLFW_KEY_O:
      newResource++;
      break;

    case GLFW_KEY_P:
      newResource--;
      break;

    case GLFW_KEY_S:
      stereo = !stereo;
      break;

    default:
      GlfwApp::onKey(key, scancode, action, mods);
      break;
    }
    if (newResource != imageResource) {
      if (newResource > IMAGES_STEREO_VLCSNAP_2_13_11_16_2_H_9M43S38_PNG) {
        newResource = IMAGES_STEREO_VLCSNAP_2_13_11_16_12H41M46S16__PNG;
      }
      if (newResource < IMAGES_STEREO_VLCSNAP_2_13_11_16_12H41M46S16__PNG) {
        newResource = IMAGES_STEREO_VLCSNAP_2_13_11_16_2_H_9M43S38_PNG;
      }
      imageResource = static_cast<Resource>(newResource);
      loadImageAndGeometry();
    }
  }


  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    program->use();
    texture->bind();

    glm::uvec2 position(0, 0);
    gl::viewport(position, eyeSize);
    program->setUniform("LensOffset", lensOffset);
    quadGeometries[0]->bindVertexArray();
    quadGeometries[0]->draw();

    position.x = eyeSize.x;
    gl::viewport(position, eyeSize);
    program->setUniform("LensOffset", -lensOffset);
    quadGeometries[stereo ? 1 : 0]->bindVertexArray();
    quadGeometries[stereo ? 1 : 0]->draw();

    VertexArray::unbind();
    Texture2d::unbind();
    Program::clear();
  }
};

RUN_OVR_APP(Example_5_3_4)
