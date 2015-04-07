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

class GlfwApp {
protected:
  GLFWwindow *  window{ nullptr };
  glm::uvec2    windowSize;
  glm::ivec2    windowPosition;
  int           frame{ 0 };
  RateCounter   fpsCounter;

protected:
  float         windowAspect{ 1.0f };
  float         windowAspectInverse{ 1.0f };
  float         fps{ 0 };

public:
  GlfwApp();
  virtual ~GlfwApp();
  virtual int run();

protected:
  virtual GLFWwindow * createRenderingTarget(glm::uvec2 & outSize, glm::ivec2 & outPosition) = 0;
  virtual void draw() = 0;

  int getFrame() const;
  const glm::uvec2 & getSize() const;
  const glm::ivec2 & getPosition() const;
  GLFWwindow * getWindow();

  virtual void preCreate();
  virtual void postCreate();
  virtual void initGl();
  virtual void shutdownGl();
  virtual void finishFrame();
  virtual void destroyWindow();
  virtual void onKey(int key, int scancode, int action, int mods);
  virtual void onCharacter(unsigned int code_point);
  virtual void onScroll(double x, double y);
  virtual void onMouseButton(int button, int action, int mods);
  virtual void onMouseMove(double x, double y);
  virtual void onMouseEnter(int entered);
  virtual void update();
  virtual void viewport(const glm::uvec2 & size, const glm::ivec2 & pos = ivec2(0));
  virtual void viewport(const glm::vec2 & size, const glm::vec2 & pos = vec2(0));
  virtual void renderStringAt(const std::string & string, float x, float y);
  virtual void renderStringAt(const std::string & string, const glm::vec2 & position);

private:

  friend void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
  friend void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
  friend void CursorEnterCallback(GLFWwindow* window, int enter);
  friend void MouseMoveCallback(GLFWwindow* window, double x, double y);
  friend void CharacterCallback(GLFWwindow* window, unsigned int codepoint);
  friend void ScrollCallback(GLFWwindow * window, double x, double y);
};

