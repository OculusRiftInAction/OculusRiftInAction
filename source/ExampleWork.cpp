#include "Common.h"

using namespace OVR;
using namespace glm;
using namespace gl;

class Example05 : public RiftApp {
protected:
  glm::mat4 player;
  TextureCubeMapPtr skyboxTexture;
  GeometryPtr treeGeometry;
  int activeBase = -1;
  int activeControllers = -1;

public:

  Example05() {
//    sensorFusion.SetGravityEnabled(false);
  }

  void initGl() {
    RiftApp::initGl();
    Stacks::projection().top() = perspective(60.0f, 4.0f / 3.0f, 0.001f,
        1000.0f);

    skyboxTexture = GlUtils::getCubemapTextures(
        Resource::IMAGES_SKY_CITY_XNEG_PNG);
    {
      Mesh mesh;
      MatrixStack & m = mesh.getModel();
      m.push().identity();
      {
        m.push();
        m.scale(glm::vec3(0.10, 0.25f, 0.10));
        m.translate(glm::vec3(0, 0.5f, 0));
        m.rotate(90, GlUtils::X_AXIS);
        mesh.getColor() = Colors::brown;
        mesh.addMesh(GlUtils::getMesh(Resource::MESHES_CYLINDER_CTM));
        mesh.fillColors(true);
        m.pop();

        m.scale(vec3(0.15));
        m.translate(vec3(0, 2, 0));
        mesh.getColor() = Colors::green;
        mesh.addMesh(GlUtils::getMesh(Resource::MESHES_ICO_SPHERE_CTM));
        mesh.fillColors(true);
//      for (int i = 0; i < 4; ++i) {
//        m.push();
//        glm::vec2 noiseParam = glm::vec2((float)i, 0);
//        float noise = glm::noise1(noiseParam);
//        m.rotate(noise * 360.0f, GlUtils::Y_AXIS);
//        m.translate(vec3(1, 0, 0));
//        m.scale(vec3(0.5));
//        mesh.getColor() = Colors::forestGreen;
//        mesh.addMesh(GlUtils::getMesh(Resource::MESHES_ICO_SPHERE_CTM));
//        mesh.fillColors(true);
//        m.pop();
//      }
      }
      mesh.fillNormals(true);
      m.pop();
      treeGeometry = mesh.getGeometry();
      GL_CHECK_ERROR;
      GlUtils::getCubeGeometry();

    }
    static const vec3 POSITION = vec3(1, 0.5, 2.5);
    static const vec3 ORIGIN = vec3(0.0f, 0.5f, 0.0f);
    static const vec3 UP = GlUtils::Y_AXIS;
    player = glm::inverse(glm::lookAt(POSITION, ORIGIN, UP));
  }

  virtual void update() {
    CameraControl::instance().applyInteraction(player);

    // The rift orientation impact the view, but does not alter the camera itself
    // Movement is independent of which direction you're looking, only depends on
    // the camera orientation
    glm::mat4 riftOrientation = mat4_cast(
        Rift::getQuaternion(sensorFusion));

    Stacks::modelview().top() = riftOrientation * glm::inverse(player);
  }

  virtual void renderScene(const glm::mat4 & modelviewOffset =
      glm::mat4()) {
    glEnable(GL_DEPTH_TEST);
    clearColor(vec3(0.05f, 0.05f, 0.05f));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    MatrixStack & mv = Stacks::modelview();

    // Skybox
    {
      mv.push().untranslate();
      skyboxTexture->bind();
      GL_CHECK_ERROR;
      glDisable(GL_DEPTH_TEST);
      glCullFace(GL_FRONT);
      GL_CHECK_ERROR;
      GlUtils::renderGeometry(
          GlUtils::getCubeGeometry(),
          ShaderResource::SHADERS_CUBEMAP_VS,
          ShaderResource::SHADERS_CUBEMAP_FS
          );
      GL_CHECK_ERROR;
      glCullFace(GL_BACK);
      glEnable(GL_DEPTH_TEST);
      GL_CHECK_ERROR;
      skyboxTexture->unbind();
      mv.pop();
      GL_CHECK_ERROR;
    }

    GlUtils::draw3dGrid();
//  GlUtils::drawColorCube();

    // Set up camera position
    mv.push();
    {
      mv.translate(vec3(0, 0, -0.5));
      GlUtils::renderGeometry(treeGeometry,
          ShaderResource::SHADERS_LITCOLORED_VS,
          ShaderResource::SHADERS_LITCOLORED_FS);
    }
    mv.pop();
    mv.push();
    {
//    mv.translate(curPos / 1000.0f);
//    mv.rotate(curOrient);
      mv.translate(GlUtils::Y_AXIS * -0.3f);
      mv.scale(glm::vec3(4));
//    GlUtils::renderBunny();
      mv.push();
      {
        mv.identity();
        glm::vec2 cursor;
//        GlUtils::renderString("Hello", cursor);
      }
      mv.pop();
    }
    mv.pop();
//    GlUtils::draw3dVector(mag, Colors::cyan);
  }

  virtual int run() {
    int result = GlfwApp::run();
    return result;
  }
};

RUN_OVR_APP(Example05);

