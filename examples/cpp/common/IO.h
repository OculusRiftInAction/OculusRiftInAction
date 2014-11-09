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
#include <iostream>

template<class To, class From> To hard_cast(From v) {
  return static_cast<To>(static_cast<void*>(v));
}

template<class To, class From> To hard_const_cast(const From v) {
  return static_cast<To>(static_cast<const void*>(v));
}

template<class From> char * read_cast(From v) {
  return hard_cast<char *, From>(v);
}

template<size_t Size, typename T> std::istream &
readStream(std::istream & in, T (&array)[Size]) {
    size_t s = Size;
    size_t ts = sizeof(T);

  return in.read(read_cast(array), s * ts);
}

template<typename T> std::istream &
readStream(std::istream & in, T & t) {
  return in.read(read_cast(&t), sizeof(T));
}

template<typename T> std::istream &
readStream(std::istream & in, T * t, size_t size) {
  return in.read(read_cast(t), sizeof(T) * size);
}

std::istream &
readStream(std::istream & in, std::string & out);
