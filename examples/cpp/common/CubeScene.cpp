#include "Common.h"
#include "CubeScene.h"
#include <glm/gtx/string_cast.hpp>

CubeScene::CubeScene() : cubeCount(0) {
  ipd = ovrHmd_GetFloat(hmd, OVR_KEY_IPD, OVR_DEFAULT_IPD);
  eyeHeight = ovrHmd_GetFloat(hmd,
      OVR_KEY_PLAYER_HEIGHT,
      OVR_DEFAULT_PLAYER_HEIGHT);
  resetCamera();
}

void CubeScene::initGl() {
  RiftGlfwApp::initGl();

  program = GlUtils::getProgram(
      Resource::SHADERS_CHAPTER5_VS,
      Resource::SHADERS_CHAPTER5_FS);
  cube = GlUtils::getColorCubeGeometry();
  pedestal = GlUtils::getColorCubeGeometry();

  glDisable(GL_BLEND);
  glDisable(GL_LINE_SMOOTH);
  glClearColor(0.65f, 0.65f, 0.65f, 1);
  gl::Stacks::lights().addLight(glm::vec4(50, 50, 50, 1));
  gl::Stacks::lights().addLight(glm::vec4(50, 50, -50, 1));
  gl::Stacks::modelview().push().identity();
}

void CubeScene::onKey(int key, int scancode, int action, int mods) {
  if (CameraControl::instance().onKey(key, scancode, action, mods)) {
    return;
  }

  if (GLFW_PRESS == action) {
    switch (key) {
    case GLFW_KEY_R:
      resetCamera();
      ovrHmd_ResetSensor(hmd);
      return;
    case GLFW_KEY_LEFT_BRACKET:
      SAY("%s", glm::to_string(camera).c_str());
      return;
    case GLFW_KEY_RIGHT_BRACKET:
      camera = glm::mat4x4(
          1.000000, -0.000000, 0.000000, -0.000000,
          -0.000000, 0.998896, -0.046983, 0.000000,
          0.000000, 0.046983, 0.998896, -0.000000,
          -0.000000, -2.200359, -15.852616, 1.000000);
      return;
    }
  }

  RiftGlfwApp::onKey(key, scancode, action, mods);
}

void CubeScene::update() {

  // We invert the camera matrix to generate a "player" matrix,
  // because our interaction code works from the player's point
  // of view instead of the world's view of the camera.
  glm::mat4 player = glm::inverse(camera);
  CameraControl::instance().applyInteraction(player);
  camera = glm::inverse(player);

  gl::Stacks::modelview().top() = camera;
}

void CubeScene::resetCamera() {
  camera = glm::lookAt(
      glm::vec3(0, eyeHeight, 0.5f),  // Position of the camera
      glm::vec3(0, eyeHeight, 0),     // Where the camera is looking
      GlUtils::Y_AXIS);               // Camera up axis
}

typedef std::vector<glm::mat4> VecXfm; 
VecXfm buildRoom() {
  VecXfm transforms;

  // Draw arches made of cubes.
  for (int x = -10; x <= 10; x++) {
    for (int y = 0; y <= 10; y++) {
      for (int z = -10; z <= 10; z++) {
        glm::vec3 pos(x, y, z);
        float len = glm::length(pos);
        if (len > 11 && len < 12) {
          transforms.push_back(glm::translate(glm::mat4(), pos));
        }
      }
    }
  }

  // Draw a floor made of cubes.
  for (int x = -12; x <= 12; x++) {
    for (int z = -12; z <= 12; z++) {
      transforms.push_back(glm::translate(glm::mat4(), glm::vec3(x, -0.5f, z)));
    }
  }

  return transforms;
}

VecXfm buildPedestal(float ipd, float eyeHeight) {
  VecXfm transforms;

  // Draw a single cube at the center of the room, at eye height,
  // and put it on a pedestal.
  transforms.push_back(glm::scale(glm::translate(glm::mat4(), glm::vec3(0, eyeHeight, 0)), glm::vec3(ipd)));
  transforms.push_back(glm::scale(glm::translate(glm::mat4(), glm::vec3(0, eyeHeight / 2, 0)), glm::vec3(ipd / 2, eyeHeight, ipd / 2)));
  return transforms;
}

void CubeScene::drawCubeScene() {
  if (!cubeCount) {
    VecXfm scene = buildRoom();
    cubeCount = scene.size();

    (new gl::VertexBuffer(scene))->bind();
    cube->bindVertexArray();
    cube->addInstanceVertexArray();

    (new gl::VertexBuffer(buildPedestal(ipd, eyeHeight)))->bind();
    pedestal->bindVertexArray();
    pedestal->addInstanceVertexArray();

    gl::VertexArray::unbind();
  }

  program->use();
  program->setUniform("FadeToWhite", 1);

  gl::Stacks::lights().apply(*program);
  gl::Stacks::projection().apply(*program);
  gl::Stacks::modelview().apply(*program);

  cube->bindVertexArray();
  cube->drawInstanced(cubeCount);

  program->setUniform("FadeToWhite", 0);

  pedestal->bindVertexArray();
  pedestal->drawInstanced(2);

  gl::VertexArray::unbind();
  gl::Program::clear();
}