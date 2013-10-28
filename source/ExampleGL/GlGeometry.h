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

#include "GlBuffers.h"
#include "GlVertexArrays.h"

namespace GL {

class Geometry;

typedef std::shared_ptr<Geometry> GeometryPtr;

class Geometry {
public:
    static const int VERTEX_ATTRIBUTE_SIZE = 4;
    static const int BYTES_PER_ATTRIBUTE = (sizeof(float) * VERTEX_ATTRIBUTE_SIZE);
    enum Flag {
        HAS_NORMAL = 0x01,
        HAS_COLOR = 0x02,
        HAS_TEXTURE = 0x04,
    };

    VertexBufferPtr vertices;
    IndexBufferPtr indices;
    VertexArrayPtr vertexArray;

    unsigned int elements;
    unsigned int verticesPerElement;
    unsigned int flags;
    GLenum elementType;

    void bindVertexArray() {
        vertexArray->bind();
    }

    static void unbindVertexArray() {
        GL::VertexArray::unbind();
    }


    static unsigned int getStride(unsigned int flags) {
        int result = VERTEX_ATTRIBUTE_SIZE;
        if (flags & Geometry::Flag::HAS_NORMAL) {
            result += VERTEX_ATTRIBUTE_SIZE;
        }
        if (flags & Geometry::Flag::HAS_COLOR) {
            result += VERTEX_ATTRIBUTE_SIZE;
        }
        if (flags & Geometry::Flag::HAS_TEXTURE) {
            result += VERTEX_ATTRIBUTE_SIZE;
        }
        return result * sizeof(float);
    }

    unsigned int getStride() {
        return getStride(flags);
    }

    void draw() {
        glDrawElements(elementType, elements * verticesPerElement, GL_UNSIGNED_INT, (void*) 0);
    }

    Geometry(const GL::VertexBufferPtr in_vertices, const GL::IndexBufferPtr in_indices, unsigned int elements = 0,
            unsigned int flags = 0, GLenum elementType = GL_TRIANGLES, unsigned int verticesPerElement = 3)
            : vertices(in_vertices), indices(in_indices), elements(elements), flags(flags), vertexArray(
                    new GL::VertexArray()), elementType(elementType), verticesPerElement(verticesPerElement) {
        buildVertexArray();
    }

    void buildVertexArray() {
        vertexArray->bind();
        {
            indices->bind();
            vertices->bind();
            unsigned int stride = getStride(flags);
            GLfloat * offset = 0;
            glEnableVertexAttribArray(GL::Attribute::Position);
            glVertexAttribPointer(GL::Attribute::Position, 3, GL_FLOAT, GL_FALSE, stride, offset);

            if (flags & HAS_NORMAL) {
                offset += VERTEX_ATTRIBUTE_SIZE;
                glEnableVertexAttribArray(GL::Attribute::Normal);
                glVertexAttribPointer(GL::Attribute::Normal, 3, GL_FLOAT, GL_FALSE, stride, offset);
            }

            if (flags & HAS_COLOR) {
                offset += VERTEX_ATTRIBUTE_SIZE;
                glEnableVertexAttribArray(GL::Attribute::Color);
                glVertexAttribPointer(GL::Attribute::Color, 3, GL_FLOAT, GL_FALSE, stride, offset);
            }

            if (flags & HAS_TEXTURE) {
                offset += VERTEX_ATTRIBUTE_SIZE;
                glEnableVertexAttribArray(GL::Attribute::TexCoord0);
                glVertexAttribPointer(GL::Attribute::TexCoord0, 2, GL_FLOAT, GL_FALSE, stride, offset);
            }
        }
        VertexArray::unbind();
        IndexBuffer::unbind();
        VertexBuffer::unbind();
    }


};

} // GL

