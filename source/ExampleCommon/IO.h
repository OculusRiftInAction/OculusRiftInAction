/*
 * IO.h
 *
 *  Created on: Oct 26, 2013
 *      Author: bdavis
 */

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

template <size_t Size, typename T> std::istream &
readStream(std::istream & in, T (&array)[Size]) {
    return in.read(read_cast(array), Size * sizeof(T));
}

template <typename T> std::istream &
readStream(std::istream & in, T & t) {
    return in.read(read_cast(&t), sizeof(T));
}

template <typename T> std::istream &
readStream(std::istream & in, T * t, size_t size) {
    return in.read(read_cast(t), sizeof(T) * size);
}
