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

#include <vector>
#include <cassert>
#include <memory>

namespace GL {

class GlBufferLoader {
public:
    virtual ~GlBufferLoader() { }
    virtual const GLvoid* getData() = 0;
    virtual GLsizeiptr getSize() = 0;
};

template <class T> class VectorLoader : public GlBufferLoader {
public:
    const std::vector<T> & data;
    VectorLoader(const std::vector<T> & data) : data(data) {
    }

    const GLvoid* getData() {
        const T * ptr = &(this->data[0]);
        return ptr;
    }

    GLsizeiptr getSize() {
        return sizeof(T) * data.size();
    }

    operator GlBufferLoader & () {
        return *this;
    }

};

template <size_t Size, typename  T> class ArrayLoader :  public GlBufferLoader {
public:
    const T * data;

    ArrayLoader(T * data) : data(data) {
    }

    const GLvoid* getData() {
        return (GLvoid*)data;
    }

    GLsizeiptr getSize() {
        return sizeof(T) * Size;
    }
};

template <size_t Size, typename T> ArrayLoader<Size, T> makeArrayLoader(T * pointer) {
    return ArrayLoader<Size, T>(pointer);
}


template <size_t Size, typename T> ArrayLoader<Size, T> makeArrayLoader(T (&array)[Size]) {
    return ArrayLoader<Size, T>(array);
}

template <
    GLenum GlBufferType,
    GLenum GlUsageType = GL_STATIC_DRAW
>
class GlBuffer {
    GLuint buffer;
public:
    GlBuffer() : buffer(0) {
    }

    template <typename T> GlBuffer(std::vector<T> & data) : buffer(0) {
        *this << GL::VectorLoader<T>(data);
    }

    GlBuffer(GlBuffer && other) : buffer(0) {
        std::swap(other.buffer, buffer);
    }

    virtual ~GlBuffer() {
        glDeleteBuffers(1, &buffer);
    }

    void bind() const {
        glBindBuffer(GlBufferType, buffer);
    }

    static void unbind() {
        glBindBuffer(GlBufferType, 0);
    }

    void init() {
        glGenBuffers(1, &buffer);
        assert(buffer != 0);
    }

    void operator << (GlBufferLoader & loader) {
        load(loader);
    }

    void operator << (GlBufferLoader && loader) {
        load(loader);
    }

    void load(GlBufferLoader & loader) {
        if (0 == buffer) {
            init();
        }
        bind();
        glBufferData(GlBufferType, loader.getSize(), loader.getData(), GlUsageType);
    }
};

typedef GlBuffer<GL_ELEMENT_ARRAY_BUFFER> IndexBuffer;
typedef std::shared_ptr<IndexBuffer> IndexBufferPtr;

typedef GlBuffer<GL_ARRAY_BUFFER> VertexBuffer;
typedef std::shared_ptr<VertexBuffer> VertexBufferPtr;

} // GL

