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
  gl::TimeQueryPtr query;
  GLFWwindow *  window;
  glm::uvec2    windowSize;
  glm::ivec2    windowPosition;
  float         windowAspect;
  float         windowAspectInverse;
  float         fps;

public:
  GlfwApp();
  virtual ~GlfwApp();

  virtual void createRenderingTarget() = 0;
  virtual void initGl();
  virtual int run();

  virtual void screenshot();
  virtual void finishFrame();
  virtual void createWindow(const glm::uvec2 & size, const glm::ivec2 & position = glm::ivec2(INT_MIN));
  virtual void createWindow(int w, int h, int x = INT_MIN, int y = INT_MIN) {
    createWindow(glm::uvec2(w, h), glm::ivec2(x, y));
  }
  virtual void createFullscreenWindow(const glm::uvec2 & size, GLFWmonitor * targetMonitor);
  virtual void destroyWindow();
  virtual void onKey(int key, int scancode, int action, int mods);
  virtual void onMouseButton(int button, int action, int mods);
  virtual void draw();
  virtual void update();
  virtual void renderStringAt(const std::string & string, float x, float y) {
    renderStringAt(string, glm::vec2(x, y));
  }
  virtual void renderStringAt(const std::string & string, const glm::vec2 & position);

  static GLFWmonitor * getMonitorAtPosition(const glm::ivec2 & position);
  void createSecondaryScreenWindow(const glm::uvec2 & size);

private:
  void onCreate();
  void preCreate();
};
