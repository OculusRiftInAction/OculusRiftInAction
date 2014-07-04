#pragma once
#include "Common.h"

class CubeScene: public RiftGlfwApp
{
protected:
  glm::mat4 camera;
  float ipd, eyeHeight;
  gl::GeometryPtr cube;
  gl::ProgramPtr program;

public:
  CubeScene();

  virtual void initGl();
  virtual void onKey(int key, int scancode, int action, int mods);
  virtual void update();

  void resetCamera();
  void drawCubeScene();
};

