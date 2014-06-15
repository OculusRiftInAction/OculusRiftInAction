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
  gl::clearColor(Colors::darkGrey);
}

void Chapter_5::onKey(int key, int scancode, int action, int mods) {
  if (CameraControl::instance().onKey(camera, key, scancode, action, mods)) {
    return;
  }

  switch (key) {
  case GLFW_KEY_R:
    if (GLFW_PRESS == action) {
      resetCamera();
      if (NULL != hmd) {
        ovrHmd_ResetSensor(hmd);
      }
    }
    return;

  default:
    RiftGlfwApp::onKey(key, scancode, action, mods);
    return;
  }
}

void Chapter_5::update() {
  // We invert the camera matrix to generate a player matrix,
  // because our interaction code works from the player's point
  // of view instead of the world's point of view.
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
  glm::vec3 scale = glm::vec3(0.975f, 0.975f, 0.975f);
  gl::MatrixStack & mv = gl::Stacks::modelview();
  const gl::ProgramPtr & program = GlUtils::getProgram(
      Resource::SHADERS_COLORED_VS, Resource::SHADERS_COLORED_FS);
  const gl::GeometryPtr & geometry = GlUtils::getColorCubeGeometry();

  program->use();
  geometry->bindVertexArray();
  gl::Stacks::lights().apply(*program);
  gl::Stacks::projection().apply(*program);

  // Draw arches made of cubes.
  for (int x = -10; x <= 10; x++) {
    for (int y = 0; y <= 10; y++) {
      for (int z = -10; z <= 10; z++) {
        glm::vec3 pos(x, y, z);
        float len = glm::length(pos);
        if (len > 11 && len < 12) {
          gl::Stacks::with_push(mv, [&]{
            mv.translate(pos);
            mv.apply(*program);
            geometry->draw();
          });
        }
      }
    }
  }

  // Draw a floor made of cubes.
  for (int x = -10; x <= 10; x++) {
    for (int z = -10; z <= 10; z++) {
      glm::vec3 pos(x, -0.5f, z);
      gl::Stacks::with_push(mv, [&]{
        mv.translate(pos);
        mv.scale(scale);
        mv.apply(*program);
        geometry->draw();
      });
    }
  }

  // Draw a single cube at the center of the room,
  // at eye height.
  gl::Stacks::with_push(mv, [&]{
    mv.translate(glm::vec3(0, eyeHeight, 0));
    mv.scale(ipd);
    mv.apply(*program);
    geometry->draw();
  });

  // Put the cube on a pedestal.
  gl::Stacks::with_push(mv, [&]{
    mv.translate(glm::vec3(0, eyeHeight / 2, 0));
    mv.scale(glm::vec3(ipd / 2, eyeHeight, ipd / 2));
    mv.apply(*program);
    geometry->draw();
  });

  gl::VertexArray::unbind();
  gl::Program::clear();
}
