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

#ifndef GL_ZERO
#error "You must include the GL headers before including this file"
#endif

#include "GlShaders.h"
#include <glm/glm.hpp>
#include <vector>

namespace GL {

struct Light {
    glm::vec4 position;
    glm::vec4 color;

    Light(const glm::vec4 & position = glm::vec4(1), const glm::vec4 & color = glm::vec4(1)) {
        this->position = position;
        this->color = color;
    }
};

class Lights {
    std::vector<glm::vec4> lightPositions;
    std::vector<glm::vec4> lightColors;
    glm::vec4 ambient;

public:
    // Singleton class
    Lights() : ambient(glm::vec4(0.2, 0.2, 0.2, 1.0)) {
        addLight();
    }

    void apply(Program & renderProgram)  {
        renderProgram.setUniform4f("Ambient", ambient);
        renderProgram.setUniform1i("LightCount", lightPositions.size());
        if (!lightPositions.empty()) {
            renderProgram.setUniform4fv("LightPosition[0]", lightPositions);
            renderProgram.setUniform4fv("LightColor[0]", lightColors);
        }
    }

    void apply(ProgramPtr & program) {
        apply(*program);
    }

    void addLight(const glm::vec4 & position = glm::vec4(1), const glm::vec4 & color = glm::vec4(1)) {
        lightPositions.push_back(position);
        lightColors.push_back(color);
    }

    void addLight(const Light & light) {
        addLight(light.position, light.color);
    }

    void setAmbient(const glm::vec4 & ambient) {
        this->ambient = ambient;
    }
};

} // GL
