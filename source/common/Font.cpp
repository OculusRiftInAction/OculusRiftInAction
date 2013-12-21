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

#include "Common.h"
#include "IO.h"

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#else
#include <png.h>
#endif

namespace Text {

using namespace std;

// 1 point = 1pt = 1/72in (cala) = 0.3528 mm
const float Font::DTP_TO_METERS = 0.003528f;
const float Font::METERS_TO_DTP = 1.0f / Font::DTP_TO_METERS;

Font::Font(void)
    : mFamily("Unknown"), mFontSize(12.0f), mLeading(0.0f), mAscent(0.0f), mDescent(
        0.0f), mSpaceWidth(0.0f) {
}

Font::~Font(void) {
}

struct TextureVertex {
  glm::vec4 pos;
  glm::vec4 tex;
  TextureVertex(const glm::vec2 & pos, const glm::vec2 & tex)
      : pos(pos, 0, 0), tex(tex, 0, 0) {
  }
};

#ifdef HAVE_OPENCV

template<GLenum TargetType = GL_TEXTURE_2D>
void readPngToTexture(const void * data, size_t size,
    std::shared_ptr<gl::Texture<TargetType> > & texture, glm::vec2 & textureSize) {
  std::vector<unsigned char> v(size);
  memcpy(&(v[0]), data, size);
  cv::Mat image = cv::imdecode(v, CV_LOAD_IMAGE_ANYDEPTH);
  texture = std::shared_ptr<gl::Texture<TargetType> >(new gl::Texture<TargetType>());
  GL_CHECK_ERROR;
  texture->bind();
  GL_CHECK_ERROR;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, image.cols, image.rows, 0, GL_RGB,
  GL_UNSIGNED_BYTE, image.data);
  GL_CHECK_ERROR;
  gl::Texture<TargetType>::unbind();
  GL_CHECK_ERROR;
  }

#else

void PngDataCallback(png_structp png_ptr, png_bytep outBytes,
    png_size_t byteCountToRead) {
  istream * pIn = (istream*) png_get_io_ptr(png_ptr);
  if (pIn == nullptr)
  throw runtime_error("PNG: missing stream pointer");

  istream & in = *pIn; // png_ptr->io_ptr;
  const ios::pos_type start = in.tellg();
  readStream(in, outBytes, byteCountToRead);
  const ios::pos_type bytesRead = in.tellg() - start;
  if ((png_size_t) bytesRead != byteCountToRead)
  throw runtime_error("PNG: short read");
}

void readPngToTexture(const void * data, size_t size,
    gl::TexturePtr & texture,
    glm::vec2 & textureSize) {
  std::istringstream in(std::string(static_cast<const char*>(data), size));
  enum {
    kPngSignatureLength = 8
  };
  uint8_t pngSignature[kPngSignatureLength];
  readStream(in, pngSignature);
  if (!png_check_sig(pngSignature, kPngSignatureLength))
    throw runtime_error("bad png sig");

  // get PNG file info struct (memory is allocated by libpng)
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
  NULL, NULL, NULL);
  if (png_ptr == nullptr)
    throw runtime_error("PNG: bad signature");

  // get PNG image data info struct (memory is allocated by libpng)
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == nullptr) {
    // libpng must free file info struct memory before we bail
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    throw runtime_error("PNG: failed to allocate header");
  }

  png_set_read_fn(png_ptr, &in, PngDataCallback);
  // tell libpng we already read the signature
  png_set_sig_bytes(png_ptr, kPngSignatureLength);

  png_read_info(png_ptr, info_ptr);

  png_uint_32 width = 0;
  png_uint_32 height = 0;
  int colorType = -1;
  int bitDepth = 0;
  uint32_t retval = png_get_IHDR(png_ptr, info_ptr, &width, &height,
      &bitDepth, &colorType, nullptr, nullptr, nullptr);

  if (retval != 1) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    throw runtime_error("PNG: failed to read header");
  }

  textureSize = glm::vec2(width, height);
  const size_t bytesPerRow = png_get_rowbytes(png_ptr, info_ptr);
  uint8_t * textureData = new uint8_t[bytesPerRow * height];

  for (uint32_t rowIdx = 0; rowIdx < height; ++rowIdx) {
    size_t offset = height - (rowIdx + 1);
    offset *= bytesPerRow;
    png_read_row(png_ptr, (png_bytep) textureData + offset, NULL);
  }

  texture = gl::TexturePtr(new gl::Texture2d());
  texture->bind();
  texture->parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  texture->parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  texture->image2d(glm::ivec2(width, height), textureData, 0, GL_RED);
  texture->generateMipmap();
  gl::Texture2d::unbind();
  delete[] textureData;
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
}
#endif
void Font::read(const void * data, size_t size) {
  std::istringstream in(std::string(static_cast<const char*>(data), size));
  // read header
  uint8_t header;
  readStream(in, header);
  if (header != 'S')
    FAIL("Bad font file");
  readStream(in, header);
  if (header != 'D')
    FAIL("Bad font file");
  readStream(in, header);
  if (header != 'F')
    FAIL("Bad font file");
  readStream(in, header);
  if (header != 'F')
    FAIL("Bad font file");

  uint16_t version;
  readStream(in, version);

  // read font name
  if (version > 0x0001) {
    char c;
    readStream(in, c);
    while (c) {
      mFamily += c;
      readStream(in, c);
    }
  }

  // read font data
  readStream(in, mLeading);
  readStream(in, mAscent);
  readStream(in, mDescent);
  readStream(in, mSpaceWidth);
  mFontSize = mAscent + mDescent;

  // read metrics data
  mMetrics.clear();

  uint16_t count;
  readStream(in, count);

  for (int i = 0; i < count; ++i) {
    uint16_t charcode;
    readStream(in, charcode);

    Metrics m;
    readStream(in, m.x1);
    readStream(in, m.y1);
    readStream(in, m.w);
    readStream(in, m.h);

    readStream(in, m.dx);
    readStream(in, m.dy);
    readStream(in, m.d);

    m.x2 = m.x1 + m.w;
    m.y2 = m.y1 + m.h;
    std::swap(m.y2, m.y1);
    m.y1 = -m.y1;
    m.y2 = -m.y2;
    mMetrics[charcode] = m;
  }

  // read image data
  readPngToTexture((const char *) data + in.tellg(), size - in.tellg(),
      mTexture, mTextureSize);

  std::vector < TextureVertex > vertexData;
  std::vector < GLuint > indexData;
  float texH = 0, texW = 0, texA = 0;
  int characters = 0;
  std::for_each(mMetrics.begin(), mMetrics.end(),
      [&] ( MetricsData::reference & md ) {
        uint16_t id = md.first;
        Font::Metrics & m = md.second;
        ++characters;

		GLuint index = (GLuint)vertexData.size();

        rectf bounds = getBounds(m, mFontSize);
        rectf texBounds = getTexCoords(m);
        TextureVertex vertex[4] = {
          TextureVertex(bounds.getLowerLeft(), texBounds.getLowerLeft()),
          TextureVertex(bounds.getLowerRight(), texBounds.getLowerRight()),
          TextureVertex(bounds.getUpperRight(), texBounds.getUpperRight()),
          TextureVertex(bounds.getUpperLeft(), texBounds.getUpperLeft())
        };

        vertexData.push_back(TextureVertex(bounds.getLowerLeft(), texBounds.getLowerLeft()));
        vertexData.push_back(TextureVertex(bounds.getLowerRight(), texBounds.getLowerRight()));
        vertexData.push_back(TextureVertex(bounds.getUpperRight(), texBounds.getUpperRight()));
        vertexData.push_back(TextureVertex(bounds.getUpperLeft(), texBounds.getUpperLeft()));

        m.indexOffset = indexData.size();
        indexData.push_back(index + 0);
        indexData.push_back(index + 1);
        indexData.push_back(index + 2);
        indexData.push_back(index + 0);
        indexData.push_back(index + 2);
        indexData.push_back(index + 3);
      });

  gl::VertexBufferPtr vertexBuffer(new gl::VertexBuffer(vertexData));
  gl::IndexBufferPtr indexBuffer(new gl::IndexBuffer(indexData));
  mGeometry = gl::GeometryPtr(
      new gl::Geometry(vertexBuffer, indexBuffer, characters * 2,
          gl::Geometry::Flag::HAS_TEXTURE));
  mGeometry->buildVertexArray();
}

Font::Metrics Font::getMetrics(uint16_t charcode) const {
  MetricsData::const_iterator itr = mMetrics.find(charcode);
  if (itr == mMetrics.end())
    return Metrics();

  return itr->second;
}

rectf Font::getBounds(uint16_t charcode, float fontSize) const {
  MetricsData::const_iterator itr = mMetrics.find(charcode);
  if (itr != mMetrics.end())
    return getBounds(itr->second, fontSize);
  else
    return rectf();
}

rectf Font::getBounds(const Metrics &metrics, float fontSize) const {
  float scale = (fontSize / mFontSize);

  return rectf(glm::vec2(metrics.dx, -metrics.dy) * scale,
      glm::vec2(metrics.dx + metrics.w, metrics.h - metrics.dy) * scale);
}

rectf Font::getTexCoords(const Metrics &metrics) const {
  return rectf(glm::vec2(metrics.x1, metrics.y1) / mTextureSize,
      glm::vec2(metrics.x2, metrics.y2) / mTextureSize);
}

float Font::getAdvance(uint16_t charcode, float fontSize) const {
  MetricsData::const_iterator itr = mMetrics.find(charcode);
  if (itr != mMetrics.end())
    return getAdvance(itr->second, fontSize);

  return 0.0f;
}

float Font::getAdvance(const Metrics &metrics, float fontSize) const {
  return metrics.d * fontSize / mFontSize;
}

rectf Font::measure(const std::wstring &text, float fontSize) const {
  float offset = 0.0f;
  rectf result(0.0f, 0.0f, 0.0f, 0.0f);

  std::wstring::const_iterator citr;
  for (citr = text.begin(); citr != text.end(); ++citr) {
    uint16_t charcode = (uint16_t) * citr;

    // TODO: handle special chars like /t
    MetricsData::const_iterator itr = mMetrics.find(charcode);
    if (itr != mMetrics.end()) {
      result.include(
          rectf(offset + itr->second.dx, -itr->second.dy,
              offset + itr->second.dx + itr->second.w,
              itr->second.h - itr->second.dy));
      offset += itr->second.d;
    }
  }

  // return
  result.scale(fontSize / mFontSize);
  return result;
}

float Font::measureWidth(const std::wstring &text, float fontSize,
    bool precise) const {
  float offset = 0.0f;
  float adjust = 0.0f;

  std::wstring::const_iterator citr;
  for (citr = text.begin(); citr != text.end(); ++citr) {
    uint16_t charcode = (uint16_t) * citr;

    // TODO: handle special chars like /t
    MetricsData::const_iterator itr = mMetrics.find(charcode);
    if (itr != mMetrics.end()) {
      offset += itr->second.d;

      // precise measurement takes into account that the last character
      // contributes to the total width only by its own width, not its advance
      if (precise)
        adjust = itr->second.dx + itr->second.w - itr->second.d;
    }
  }

  return (offset + adjust) * (fontSize / mFontSize);
}

}
