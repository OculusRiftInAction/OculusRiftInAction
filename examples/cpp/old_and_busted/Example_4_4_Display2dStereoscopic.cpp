#include "Common.h"

const Resource STEREO_IMAGES[] = {
  IMAGES_STEREO_VLCSNAP_2_13_11_16_12H41M46S16__PNG,
  IMAGES_STEREO_VLCSNAP_2_13_11_16_19H47M52S211_PNG,
  IMAGES_STEREO_VLCSNAP_2_13_11_16_2_H_8M17S111_PNG,
  IMAGES_STEREO_VLCSNAP_2_13_11_16_2_H_9M17S243_PNG,
  IMAGES_STEREO_VLCSNAP_2_13_11_16_2_H_9M43S38_PNG,
};

const int STEREO_IMAGE_COUNT = 5;

class StereoscopicImages : public RiftGlfwApp {
protected:
  gl::ProgramPtr program;
  gl::Texture2dPtr texture;
  std::map<StereoEye, gl::GeometryPtr> quadGeometries;
  std::map<StereoEye, glm::mat4> projections;
  int imageIndex{0};
  bool stereo{true};

public:

  StereoscopicImages() {
    float lensDistance = ovrHmdInfo.LensSeparationDistance / ovrHmdInfo.HScreenSize;
    float lensOffset = 1.0f - (2.0f * lensDistance);
    for_each_eye([&](StereoEye eye) {
      float eyeLensOffset = (eye == LEFT) ? -lensOffset : lensOffset;
      projections[eye] = glm::ortho(
          -1.0f + eyeLensOffset, 1.0f + eyeLensOffset,
          -1.0f / eyeAspect, 1.0f / eyeAspect);
    });
  }

  void initGl() {
    GlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    program = GlUtils::getProgram(
        Resource::SHADERS_TEXTURED_VS,
        Resource::SHADERS_TEXTURED_FS);

    glm::uvec2 imageSize;
    GlUtils::getImageAsTexture(texture, STEREO_IMAGES[imageIndex], imageSize);

    float imageAspectRatio = (float)imageSize.x / (float)imageSize.y;
    glm::vec2 geometryMax(1.0f, 1.0f / imageAspectRatio);
    glm::vec2 geometryMin = geometryMax * -1.0f;

    glm::vec2 textureMin = glm::vec2(0, 0);
    glm::vec2 textureMax = glm::vec2(0.5, 1.0);
    quadGeometries[LEFT] = GlUtils::getQuadGeometry(
        geometryMin, geometryMax,
        textureMin, textureMax);

    textureMin = glm::vec2(0.5, 0.0);
    textureMax = glm::vec2(1.0, 1.0);
    quadGeometries[RIGHT] = GlUtils::getQuadGeometry(
        geometryMin, geometryMax,
        textureMin, textureMax);
  }

  void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action) {
      return;
    }

    switch (key) {
    case GLFW_KEY_O:
      nextImage(1);
      break;

    case GLFW_KEY_P:
      nextImage(-1);
      break;

    case GLFW_KEY_S:
      stereo = !stereo;
      break;

    default:
      GlfwApp::onKey(key, scancode, action, mods);
      break;
    }
  }

  void nextImage(int increment) {
    imageIndex = (imageIndex + STEREO_IMAGE_COUNT + increment) % STEREO_IMAGE_COUNT;
    glm::uvec2 imageSize;
    GlUtils::getImageAsTexture(texture, STEREO_IMAGES[imageIndex], imageSize);
  }

  virtual void draw() {
    glClear(GL_COLOR_BUFFER_BIT);
    program->use();
    texture->bind();

    for_each_eye([&](StereoEye eye) {
      viewport(eye);
      program->setUniform("Projection", projections[eye]);
      StereoEye renderEye = stereo ? eye : LEFT;
      quadGeometries[renderEye]->bindVertexArray();
      quadGeometries[renderEye]->draw();
    });
    gl::VertexArray::unbind();
    gl::Texture2d::unbind();
    gl::Program::clear();
  }
};

RUN_OVR_APP(StereoscopicImages)
