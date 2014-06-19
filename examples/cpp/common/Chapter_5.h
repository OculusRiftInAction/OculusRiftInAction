#pragma once
#include "Common.h"

class Chapter_5: public RiftGlfwApp
{
protected:
  gl::ProgramPtr program;
  gl::GeometryPtr cube;
  gl::GeometryPtr wireCube;

  float ipd;
  float eyeHeight;
  glm::mat4 camera;

  void drawCube(const glm::vec3 & translate, const glm::vec3 & scale = GlUtils::ONE);

public:
  Chapter_5();

  virtual void initGl();
  virtual void onKey(int key, int scancode, int action, int mods);
  virtual void update();

  void resetCamera();
  void drawChapter5Scene();
};

