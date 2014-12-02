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

struct Light {
  vec3 position;
  vec4 color;

  Light(const vec3 & position = vec3(1), const vec4 & color = vec4(1)) {
    this->position = position;
    this->color = color;
  }
};

class Lights {
public:
  std::vector<vec4> lightPositions;
  std::vector<vec4> lightColors;
  vec4 ambient;

  // Singleton class
  Lights()
      : ambient(glm::vec4(0.2, 0.2, 0.2, 1.0)) {
    addLight(vec3(1, 0.5, 2.0));
  }

  void addLight(const glm::vec3 & position = vec3(1),
      const vec4 & color = glm::vec4(1)) {
    lightPositions.push_back(glm::vec4(position, 1));
    lightColors.push_back(color);
  }

  void addLight(const Light & light) {
    addLight(light.position, light.color);
  }

  void setAmbient(const glm::vec4 & ambient) {
    this->ambient = ambient;
  }
};
