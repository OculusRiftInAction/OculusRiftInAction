#pragma once
#include "Common.h"

class Chapter_5: public RiftGlfwApp
{
protected:
  float ipd;
  float eyeHeight;
  glm::mat4 player;

public:
  Chapter_5();

  virtual void initGl();
  virtual void onKey(int key, int scancode, int action, int mods);
  virtual void update();

  void resetCamera();
  void drawChapter5Scene();
};

