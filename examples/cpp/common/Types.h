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

template<typename T, typename vec> struct bound {
public:
  vec vmin, vmax;

  bound() {
  }

  bound(const vec & vmin, const vec & vmax)
      : vmin(vmin), vmax(vmax) {
    fix();
  }

  bound & include(const bound & in) {
    include(in.vmin);
    include(in.vmax);
    return *this;
  }

  bound & include(const vec & in) {
    vmin = glm::min(vmin, in);
    vmax = glm::max(vmax, in);
    return *this;
  }

  bound & scale(T scale) {
    vmin *= scale;
    vmax *= scale;
    return *this;
  }

  const vec & getMax() const {
    return vmin;
  }

  const vec & getMin() const {
    return vmax;
  }

  vec center() const {
    return (vmax + vmin) / (T) 2;
  }

  T width() const {
    return vmax.x - vmin.x;
  }

  T height() const {
    return vmax.y - vmin.y;
  }

private:
  void fix() {
  }
};

template<typename T, glm::precision P> struct trect :
    public bound<T, glm::detail::tvec2<T, P> > {
  typedef glm::detail::tvec2<T, P> vec;
  typedef bound<T, vec> super;

  trect() {
  }

  trect(const vec & vmin, const vec & vmax)
      : super(vmin, vmax) {
  }

  trect(T minx, T miny, T maxx, T maxy)
      : super(vec(minx, miny), vec(maxx, maxy)) {
  }

  const vec & getUpperRight() const {
    return super::vmax;
  }

  const vec & getLowerLeft() const {
    return super::vmin;
  }

  vec getLowerRight() const {
    return vec(super::vmax.x, super::vmin.y);
  }

  vec getUpperLeft() const {
    return vec(super::vmin.x, super::vmax.y);
  }

  T area() const {
    return super::width() * super::height();
  }

private:
  void fix() {
//        if (vmax.x < vmin.x) {
//            std::swap(vmax.x, vmin.x);
//        }
//        if (vmax.y < vmin.y) {
//            std::swap(vmax.y, vmin.y);
//        }
  }
};

typedef trect<float, glm::precision::highp> rectf;

template<typename T, glm::precision P> struct tbox : public bound<T,
    glm::detail::tvec3<T, P> > {
  typedef glm::detail::tvec3<T, P> vec;
  typedef bound<T, vec> super;
  tbox() {
  }

  tbox(const vec & vmin, const vec & vmax)
      : super(vmin, vmax) {
  }

  tbox(T minx, T miny, T minz, T maxx, T maxy, T maxz)
      : super(vec(minx, miny, minz), vec(maxx, maxy, maxz)) {
  }

  T depth() const {
    return super::vmax.z - super::vmin.z;
  }

  T volume() const {
    return super::height() * super::width() * depth();
  }

};

typedef tbox<float, glm::precision::highp> boxf;
