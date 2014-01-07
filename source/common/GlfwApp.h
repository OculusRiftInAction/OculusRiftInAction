/************************************************************************************

 Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
 Copyright   :   Copyright Brad Davis. All Rights reserved.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 ************************************************************************************/

#pragma once
#include <GLFW/glfw3.h>
#include <climits>
#include <iostream>
#include <string>

class GlfwApp {
protected:
  GLFWwindow *  window;
  glm::ivec2    windowSize;
  float         windowAspect;
  float         windowAspectInverse;

public:
  GlfwApp();
  virtual ~GlfwApp();

  virtual void createRenderingTarget() = 0;
  virtual void initGl();
  virtual int run();

  virtual void screenshot();
  virtual void createWindow(const glm::ivec2 & size, const glm::ivec2 & position = glm::ivec2(INT_MIN));
  virtual void createWindow(int w, int h, int x = INT_MIN, int y = INT_MIN) {
    createWindow(glm::ivec2(w, h), glm::ivec2(x, y));
  }
  virtual void fullscreen(const glm::ivec2 & size, const char * target);
  virtual void destroyWindow();
  virtual void onKey(int key, int scancode, int action, int mods);
  virtual void draw();
  virtual void update();
private:
  void onCreate();
  void preCreate();
};
