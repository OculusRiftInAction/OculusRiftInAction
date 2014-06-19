#include "Common.h"
#include "Chapter_5.h"

Chapter_5::Chapter_5() {
  ipd = ovrHmd_GetFloat(hmd, OVR_KEY_IPD, OVR_DEFAULT_IPD);
  eyeHeight = ovrHmd_GetFloat(hmd,
      OVR_KEY_PLAYER_HEIGHT,
      OVR_DEFAULT_PLAYER_HEIGHT);
  resetCamera();
}

void Chapter_5::initGl() {
  RiftGlfwApp::initGl();

  program = GlUtils::getProgram(
      Resource::SHADERS_LITCOLORED_VS,
      Resource::SHADERS_LITCOLORED_FS);
  cube = GlUtils::getColorCubeGeometry();
  wireCube = GlUtils::getWireCubeGeometry();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glLineWidth(4);
  glClearColor(0.65f, 0.65f, 0.65f, 1);

  gl::Stacks::lights().addLight(glm::vec4(50, 50, 50, 1));
}

void Chapter_5::onKey(int key, int scancode, int action, int mods) {
  if (CameraControl::instance().onKey(key, scancode, action, mods)) {
    return;
  }

  switch (key) {
  case GLFW_KEY_R:
    if (GLFW_PRESS == action) {
      resetCamera();
      ovrHmd_ResetSensor(hmd);
    }
    return;

  default:
    RiftGlfwApp::onKey(key, scancode, action, mods);
    return;
  }
}

void Chapter_5::update() {

  // We invert the camera matrix to generate a "player" matrix,
  // because our interaction code works from the player's point
  // of view instead of the world's view of the camera.
  glm::mat4 player = glm::inverse(camera);
  CameraControl::instance().applyInteraction(player);
  camera = glm::inverse(player);

  gl::Stacks::modelview().top() = camera;
}

void Chapter_5::resetCamera() {
  camera = glm::lookAt(
      glm::vec3(0, eyeHeight, 0.5f),  // Position of the camera
      glm::vec3(0, eyeHeight, 0),     // Where the camera is looking
      GlUtils::Y_AXIS);               // Camera up axis
}

void Chapter_5::drawChapter5Scene() {

  program->use();
  gl::Stacks::lights().apply(*program);
  gl::Stacks::projection().apply(*program);

  // Draw arches made of cubes.
  for (int x = -10; x <= 10; x++) {
    for (int y = 0; y <= 10; y++) {
      for (int z = -10; z <= 10; z++) {
        glm::vec3 pos(x, y, z);
        float len = glm::length(pos);
        if (len > 11 && len < 12) {
          drawCube(pos);
        }
      }
    }
  }

  // Draw a floor made of cubes.
  for (int x = -12; x <= 12; x++) {
    for (int z = -12; z <= 12; z++) {
      drawCube(glm::vec3(x, -0.5f, z));
    }
  }

  // Draw a single cube at the center of the room, at eye height,
  // and put it on a pedestal.
  drawCube(glm::vec3(0, eyeHeight, 0), glm::vec3(ipd));
  drawCube(glm::vec3(0, eyeHeight / 2, 0), glm::vec3(ipd / 2, eyeHeight, ipd / 2));

  gl::VertexArray::unbind();
  gl::Program::clear();
}

void Chapter_5::drawCube(const glm::vec3 & translate, const glm::vec3 & scale) {
  gl::MatrixStack & mv = gl::Stacks::modelview();
  gl::Stacks::with_push(mv, [&]{
    mv.translate(translate);
    mv.scale(scale);

    // Solid cube
    mv.apply(*program);
    cube->bindVertexArray();
    cube->draw();
    cube->unbindVertexArray();

    // Wire frame
// gl::Stacks::projection().apply(*program);
    mv.scale(1.001);
    mv.apply(*program);
    wireCube->bindVertexArray();
    wireCube->draw();
    wireCube->unbindVertexArray();
  });
}
