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

namespace GL {
    inline void vertex(const glm::vec4 & v) {
        glVertex4f(v.x, v.y, v.z, v.w);
    }

    inline void vertex(const glm::vec3 & v) {
        glVertex3f(v.x, v.y, v.z);
    }

    inline void vertex(const glm::vec2 & v) {
        glVertex2f(v.x, v.y);
    }

    inline void texCoord(const glm::vec2 & v) {
        glTexCoord2f(v.x, v.y);
    }

    inline void color(const glm::vec3 & v) {
        glColor3f(v.r, v.g, v.b);
    }

    inline void color(const glm::vec4 & v) {
        glColor4f(v.r, v.g, v.b, v.a);
    }

    inline void uniform(GLint location, GLfloat a) {
        glUniform1f(location, a);
    }

    inline void uniform(GLint location, GLint a) {
        glUniform1i(location, a);
    }

    inline void uniform(GLint location, const glm::vec2 & a) {
        glUniform2f(location, a.x, a.y);
    }

    inline void uniform(GLint location, const glm::vec4 & a) {
        glUniform4f(location, a.x, a.y, a.z, a.w);
    }

    inline void uniform(GLint location, const std::vector<glm::vec4> & a) {
        glUniform4fv(location, (GLsizei) a.size(), &(a[0][0]));
    }

    inline void uniform(GLint location, const glm::vec3 & a) {
        glUniform3f(location, a.x, a.y, a.z);
    }

    inline void uniform(GLint location, const glm::mat4 & a) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(a));
    }
}



