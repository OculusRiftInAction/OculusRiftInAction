#pragma once
#include "Common.h"

class CubeScene: public RiftGlfwApp
{
protected:
  gl::ProgramPtr program;
  gl::GeometryPtr cube;
  gl::GeometryPtr wireCube;
  glm::mat4 camera;
  float ipd, eyeHeight;
  int cubeCount;

public:
  CubeScene();

  virtual void initGl();
  virtual void onKey(int key, int scancode, int action, int mods);
  virtual void update();

  void resetCamera();
  void drawCubeScene();
};

