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
