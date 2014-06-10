#include "Common.h"
#include <sixense.h>
#include <sixense_math.hpp>
#ifdef WIN32
#include <sixense_utils/mouse_pointer.hpp>
#endif
#include <sixense_utils/derivatives.hpp>
#include <sixense_utils/button_states.hpp>
#include <sixense_utils/event_triggers.hpp>
#include <sixense_utils/controller_manager/controller_manager.hpp>


class Example05 : public RiftApp {
private:
  gl::TextureCubeMapPtr skyboxTexture;
  glm::mat4 player;
  gl::GeometryPtr hydraController;
  sixenseAllControllerData acd;
  sixenseUtils::ButtonStates buttonStates[2];

public:
  Example05() {
    CameraControl::instance().enableHydra(true);
  }

private:

  void initGl() {
    RiftApp::initGl();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    gl::clearColor(glm::vec3(0.05f, 0.05f, 0.05f));
    player = glm::inverse(
        glm::lookAt(glm::vec3(0, 0.4f, 1), glm::vec3(0, 0.4f, 0),
            GlUtils::UP));
    skyboxTexture = GlUtils::getCubemapTextures(
        Resource::IMAGES_SKY_CITY_XNEG_PNG);
    const Mesh & mesh = GlUtils::getMesh(Resource::MESHES_HYDRA_FROZEN_HIGH_CTM);
    Mesh xfmMesh;

    xfmMesh.model.rotate(TWO_PI / 2, GlUtils::Z_AXIS).rotate(TWO_PI / 4, GlUtils::Y_AXIS).scale(1.0f/10.0f);
    xfmMesh.addMesh(mesh);
    hydraController = xfmMesh.getGeometry();
  }

  virtual void update() {
    CameraControl::instance().applyInteraction(player);

    gl::Stacks::modelview().top() = glm::mat4_cast(
        Rift::getQuaternion(sensorFusion)) *
        glm::inverse(player);

    // update the controller manager with the latest controller data here
    sixenseSetActiveBase(0);
    sixenseGetAllNewestData(&acd);
    sixenseUtils::getTheControllerManager()->update(&acd);
    for (int i = 0; i < 2; ++i) {
      buttonStates[i].update(&acd.controllers[i]);
    }


    // Get the latest data for the left controller
    // int left_index = sixenseUtils::getTheControllerManager()->getIndex(sixenseUtils::ControllerManager::P1L);
    // sixenseControllerData cd;
    // sixenseGetNewestData(left_index, &cd);

    // Use a sixenseUtils::Derivatives object to compute velocity and acceleration from the position
    // static sixenseUtils::Derivatives derivs;
    // update the derivative object
    // derivs.update(&cd);
  }

  glm::vec2 quantize(const glm::vec2 & t, float scale) {
    float width = scale * 2.0f;
    glm::vec2 t2 = t + scale;
    glm::vec2 t3 = glm::floor(t2 / width) * width;
    return t3;
  }

  void scaleRenderGrid(float scale, const glm::vec2 & p) {
    glm::vec2 p3 = quantize(p, scale);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.push().translate(glm::vec3(p3.x - scale, 0, p3.y - scale)).scale(
        scale);
    GlUtils::draw3dGrid();
    mv.pop();

    mv.push().translate(glm::vec3(p3.x - scale, 0, p3.y + scale)).scale(
        scale);
    GlUtils::draw3dGrid();
    mv.pop();

    mv.push().translate(glm::vec3(p3.x + scale, 0, p3.y - scale)).scale(
        scale);
    GlUtils::draw3dGrid();
    mv.pop();

    mv.push().translate(glm::vec3(p3.x + scale, 0, p3.y + scale)).scale(
        scale);
    GlUtils::draw3dGrid();
    mv.pop();
  }

  virtual void renderScene() {
    GL_CHECK_ERROR;
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    GlUtils::renderFloorGrid(player);

    gl::MatrixStack & mv = gl::Stacks::modelview();

    for (int i = 0; i < 2; ++i) {
      const auto & c = acd.controllers[i];
      glm::vec3 pos(c.pos[0], c.pos[1], c.pos[2]);
      pos /= 1000.0f;
      glm::quat rot(-c.rot_quat[2], c.rot_quat[1], -c.rot_quat[0], c.rot_quat[3]);
      mv.with_push([&]{
        mv.translate(pos).rotate(rot).scale(0.05f);
        if (c.buttons & SIXENSE_BUTTON_START) {
          mv.with_push([&]{
            mv.translate(GlUtils::Y_AXIS);
            GlUtils::drawColorCube();
          });
          mv.with_push([&]{
            mv.translate(-GlUtils::Y_AXIS);
            GlUtils::drawColorCube();
          });
          mv.with_push([&]{
            mv.translate(GlUtils::Z_AXIS);
            GlUtils::drawColorCube();
          });
          mv.with_push([&]{
            mv.translate(-GlUtils::Z_AXIS);
            GlUtils::drawColorCube();
          });
        } else {
          gl::ProgramPtr program = GlUtils::getProgram(
            Resource::SHADERS_LIT_VS,
            Resource::SHADERS_LITCOLORED_FS);
          GlUtils::renderGeometry(hydraController, program);
          //        GlUtils::drawColorCube();
        }
      });
    }
  }
};

RUN_OVR_APP(Example05);
