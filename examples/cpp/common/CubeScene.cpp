#include "Common.h"
#include "CubeScene.h"

CubeScene::CubeScene() {
  ipd = ovrHmd_GetFloat(hmd, OVR_KEY_IPD, OVR_DEFAULT_IPD);
  eyeHeight = ovrHmd_GetFloat(hmd,
      OVR_KEY_PLAYER_HEIGHT,
      OVR_DEFAULT_PLAYER_HEIGHT);
  resetCamera();
}

void CubeScene::initGl() {
  RiftGlfwApp::initGl();

  glDisable(GL_BLEND);
  glClearColor(0.65f, 0.65f, 0.65f, 1);
  glEnable(GL_DEPTH_TEST);
  gl::Stacks::modelview().push().identity();

  program = GlUtils::getProgram(Resource::SHADERS_CHAPTER5_VS, Resource::SHADERS_CHAPTER5_FS);
  cube = GlUtils::getColorCubeGeometry();

  std::vector<glm::mat4> cubeTransforms;
  cubeTransforms.push_back(glm::scale(glm::translate(glm::mat4(), glm::vec3(0, eyeHeight, 0)), glm::vec3(ipd, ipd, ipd)));
  cubeTransforms.push_back(glm::scale(glm::translate(glm::mat4(), glm::vec3(0, eyeHeight / 2, 0)), glm::vec3(ipd / 2, eyeHeight, ipd / 2)));
  (new gl::VertexBuffer(cubeTransforms))->bind();
  cube->addInstanceVertexArray();
  gl::VertexArray::unbind();
}

void CubeScene::onKey(int key, int scancode, int action, int mods) {
  if (CameraControl::instance().onKey(key, scancode, action, mods)) {
    return;
  }

  if (GLFW_PRESS == action) {
    switch (key) {
    case GLFW_KEY_R:
      resetCamera();
      ovrHmd_RecenterPose(hmd);
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

void CubeScene::drawCubeScene() {
  GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
  GlUtils::renderFloor();

  program->use();

  gl::Stacks::lights().apply(*program);
  gl::Stacks::projection().apply(*program);
  gl::Stacks::modelview().apply(*program);

  cube->bindVertexArray();
  cube->drawInstanced(2);

  gl::VertexArray::unbind();
  gl::Program::clear();
}
