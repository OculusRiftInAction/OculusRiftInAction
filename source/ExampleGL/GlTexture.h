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

#include <glm/glm.hpp>
#include <memory>

namespace GL {

class Texture {
public:
    GLuint texture;
    Texture() : texture(0) {
    }
    ~Texture() {
        if (texture) {
            glDeleteTextures(1, &texture);
        }
    }

    void init() {
        glGenTextures(1, &texture);
    }

    void bind(GLenum target = GL_TEXTURE_2D) {
        glBindTexture(target, texture);
    }

    static void unbind(GLenum target = GL_TEXTURE_2D) {
        glBindTexture(target, 0);
    }

    operator GLuint() {
        return texture;
    }
};

typedef std::shared_ptr<Texture> TexturePtr;

} // GL
