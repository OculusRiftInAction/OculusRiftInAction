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

namespace GL {

class VertexArray {
    GLuint object;
public:
    VertexArray() : object(0) {
    }
    ~VertexArray() {
        if (object) {
        	glDeleteVertexArrays(1, &object);
        }
    }

    void bind() {
        if (object == 0) {
            glGenVertexArrays(1, &object);
        }
    	glBindVertexArray(object);
    }

    static void draw(GLuint vertexCount, GLenum type = GL_TRIANGLES, GLsizeiptr offset = 0, GLenum indexType = GL_UNSIGNED_INT) {
        glDrawElements(type, vertexCount, indexType, (GLvoid*) offset);
    }

    void bindAndDraw(GLuint vertexCount, GLenum type = GL_TRIANGLES, GLsizeiptr offset = 0, GLenum indexType = GL_UNSIGNED_INT) {
        bind();
        draw(vertexCount, type, offset, indexType);
        unbind();
    }

    static void unbind() {
    	glBindVertexArray(0);
    }



    operator GLuint() {
        return object;
    }
};

typedef std::shared_ptr<VertexArray> VertexArrayPtr;

} // GL

