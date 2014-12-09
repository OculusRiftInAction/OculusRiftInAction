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

typedef std::shared_ptr<oglplus::shapes::ShapeWrapper> ShapeWrapperPtr;
typedef std::shared_ptr<oglplus::Buffer> BufferPtr;
typedef std::shared_ptr<oglplus::VertexArray> VertexArrayPtr;

namespace oria {

  ShapeWrapperPtr loadShape(const std::initializer_list<const GLchar*>& names, Resource resource, ProgramPtr program);
  ShapeWrapperPtr loadSphere(const std::initializer_list<const GLchar*>& names, ProgramPtr program);
  ShapeWrapperPtr loadSkybox(ProgramPtr program);
  ShapeWrapperPtr loadPlane(ProgramPtr program, float aspect);
  void bindLights(ProgramPtr & program);

  void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program);
  void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program, std::initializer_list<std::function<void()>> list);
  void renderCube(const glm::vec3 & color = Colors::white);
  void renderColorCube();
  void renderSkybox(Resource firstImageResource);
  void renderFloor();
  void renderManikin();
  void renderRift();
  void renderArtificialHorizon(float alpha = 0.0f);
  void renderCubeScene(float ipd, float eyeHeight);

  void renderString(const std::string & str, glm::vec2 & cursor,
      float fontSize = 12.0f, Resource font =
          Resource::FONTS_INCONSOLATA_MEDIUM_SDFF);

  void renderString(const std::string & str, glm::vec3 & cursor,
      float fontSize = 12.0f, Resource font =
          Resource::FONTS_INCONSOLATA_MEDIUM_SDFF);


  void APIENTRY debugCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar * message,
    void * userParam);
}

template <typename F>
void withContext(GLFWwindow * newContext, F f) {
  GLFWwindow * save = glfwGetCurrentContext();
  glfwMakeContextCurrent(newContext);
  f();
  glfwMakeContextCurrent(save);
}

