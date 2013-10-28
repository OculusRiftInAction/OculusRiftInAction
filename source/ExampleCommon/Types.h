/*
 Copyright (c) 2010-2012, Paul Houx - All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <glm/glm.hpp>

namespace RiftExamples {


template <typename T, glm::precision P> struct trect {
    typedef glm::detail::tvec2<T, P> vec2;
    vec2 max;
    vec2 min;

    trect() {};
    trect(vec2 min, vec2 max) : min(min), max(max) {
        fix();
    };
    trect(T minx, T miny, T maxx, T maxy) : min(vec2(minx, miny)), max(vec2(maxx, maxy)) {
        fix();
    };
    trect & include(const trect & in) {
        include(in.min);
        include(in.max);
        return *this;
    }

    trect & include(const vec2 & in) {
        min = glm::min(min, in);
        max = glm::max(max, in);
        return *this;
    }

    trect & scale(T scale) {
        min *= scale;
        max *= scale;
        return *this;
    }

    trect scaled(T scale) const {
        return trect(*this).scale(scale);
    }

    const vec2 & getUpperRight() const {
        return max;
    }

    const vec2 & getLowerLeft() const {
        return min;
    }

    vec2 getLowerRight() const {
        return vec2(max.x, min.y);
    }

    vec2 getUpperLeft() const {
        return vec2(min.x, max.y);
    }

    T width() const {
        return max.x - min.x;
    }

    T height() const {
        return max.y - min.y;
    }

    T area() const {
        return width() * height();
    }
private:
    void fix() {
//        if (max.x < min.x) {
//            std::swap(max.x, min.x);
//        }
//        if (max.y < min.y) {
//            std::swap(max.y, min.y);
//        }
    }
};

typedef trect<float,glm::precision::highp> rectf;


}
