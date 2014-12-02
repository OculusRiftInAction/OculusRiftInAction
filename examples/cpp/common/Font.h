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

#include <unordered_map>
#include <memory>
#include <string>
#include <cstdint>
#include "Types.h"

namespace Text {

class Font {
public:
  static const float DTP_TO_METERS; // = 0.003528f;
  static const float METERS_TO_DTP; // = 1.0 / DTP_TO_METERS;

public:
  // stores the font metrics for a single character
  struct Metrics {
    glm::vec2 ul;
    glm::vec2 lr;
    glm::vec2 size;
    glm::vec2 offset;
    float d;  // xadvance - adjusts character positioning
    size_t indexOffset;
  };

  typedef std::unordered_map<uint16_t, Metrics> MetricsData;
  public:
  Font();
  virtual ~Font();

  //! reads a binary font file created using 'writeBinary'
  void read(const void * data, size_t size);

  //!
  const std::string & getFamily() const {
    return mFamily;
  }
  //!
  float getAscent(float fontSize = 12.0f) const {
    return mAscent * (fontSize / mFontSize);
  }
  //!
  float getDescent(float fontSize = 12.0f) const {
    return mDescent * (fontSize / mFontSize);
  }
  //!
  float getLeading(float fontSize = 12.0f) const {
    return mLeading * (fontSize / mFontSize);
  }
  //!
  float getSpaceWidth(float fontSize = 12.0f) const {
    return mSpaceWidth * (fontSize / mFontSize);
  }

  //!
  bool contains(uint16_t charcode) const {
    return (mMetrics.find(charcode) != mMetrics.end());
  }
  //!
  rectf getBounds(uint16_t charcode, float fontSize = 12.0f) const;
  //!
  rectf getTexCoords(uint16_t charcode) const;
  //!
  float getAdvance(uint16_t charcode, float fontSize = 12.0f) const;
  //!
  rectf measure(const std::string &text, float fontSize = 12.0f) const;
  //!
  rectf measure(const std::wstring &text, float fontSize = 12.0f) const;

  //!
  float measureWidth(const std::string &text, float fontSize = 12.0f,
      bool precise = true) const;

  //!
  float measureWidth(
      const std::wstring &text,
      float fontSize = 12.0f,
      bool precise = true) const;

  float measureWidth(
      const std::wstring &text,
      size_t start,
      size_t end,
      float fontSize = 12.0f,
      bool precise = true) const;

public:
  //!
  inline rectf getBounds(const Metrics &metrics,
      float fontSize = 12.0f) const;
  //!
  inline rectf getTexCoords(const Metrics &metrics) const;
  //!
  inline float getAdvance(const Metrics &metrics,
      float fontSize = 12.0f) const;
  //!
  Metrics getMetrics(uint16_t charcode) const;

  rectf getDimensions(const std::wstring & str, float fontSize);

  void renderString(
      const std::string & str,
      glm::vec2 & cursor,
      float fontSize = 12.0f,
      float maxWidth = NAN);

  void renderString(
      const std::wstring & str,
      glm::vec2 & cursor,
      float fontSize = 12.0f,
      float maxWidth = NAN);

public:
  std::string mFamily;

  //! calculated by the 'measure' function
  float mFontSize;
  float mLeading;
  float mAscent;
  float mDescent;
  float mSpaceWidth;

  TexturePtr mTexture;
  VertexArrayPtr mVao;
  glm::vec2 mTextureSize;

  MetricsData mMetrics;
};

typedef std::shared_ptr<Font> FontPtr;

}
