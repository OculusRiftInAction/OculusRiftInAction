#include "Common.h"

using namespace std;
using namespace gl;
using namespace OVR;

#define DISTORTION_TIMING 1

class PostProcessDistortRift : public RiftGlfwApp {
protected:
  Texture2dPtr textures[2];
  GeometryPtr quadGeometry;
  ProgramPtr program;

public:

  void initGl() {
    RiftGlfwApp::initGl();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    GlUtils::getImageAsTexture(textures[0],
        Resource::IMAGES_TUSCANY_UNDISTORTED_LEFT_PNG);
    GlUtils::getImageAsTexture(textures[1],
        Resource::IMAGES_TUSCANY_UNDISTORTED_RIGHT_PNG);

    quadGeometry = GlUtils::getQuadGeometry();
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < 2; ++i) {
      const gl::TexturePtr & sceneTexture = textures[i];
      viewport(i);

      renderEye(sceneTexture);
    }
  }

  void renderEye(const gl::TexturePtr & sceneTexture) {

    GlUtils::getProgram(
      Resource::SHADERS_TEXTURED_VS,
      Resource::SHADERS_TEXTURE_FS
    )->use();

    sceneTexture->bind();

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
