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


class GlUtils {
public:
  static void checkError() {
    GLenum error = glGetError();
    if (error != 0) {
      FAIL("GL error set %d", error);
    }
  }

  static void clearError() {
    glGetError();
  }

  //static void drawColorCube(bool lit = false);
  //static void drawQuad(
  //    const glm::vec2 & min = glm::vec2(-1),
  //    const glm::vec2 & max = glm::vec2(1));
  //static void drawAngleTicks();
  //static void draw3dGrid();
  //static void draw3dVector(glm::vec3 vec, const glm::vec3 & col);

  //static gl::GeometryPtr getColorCubeGeometry();
  //static gl::GeometryPtr getCubeGeometry();
  //static gl::GeometryPtr getWireCubeGeometry();

  //static gl::GeometryPtr getQuadGeometry(
  //  float aspect, float size = 2.0f
  //);

  //static gl::GeometryPtr getQuadGeometry(
  //    const glm::vec2 & min = glm::vec2(-1),
  //    const glm::vec2 & max = glm::vec2(1),
  //    const glm::vec2 & texMin = glm::vec2(0),
  //    const glm::vec2 & texMax = glm::vec2(1)
  //);

  //static gl::GeometryPtr getSphereGeometry(float radius = 1, int du = 20, int dv = 10);

  //static gl::TextureCubeMapPtr getCubemapTextures(Resource resource);

//  static void getImageData(
//    const std::vector<unsigned char> & indata,
//    glm::uvec2 & outSize,
//    std::vector<unsigned char> & outData,
//    bool flip = true
//  );
//
//  static void getImageData(
//    Resource resource,
//    glm::uvec2 & outSize,
//    std::vector<unsigned char> & outData,
//    bool flip = true
//    );
//
//  template<GLenum TextureType>
//  static void getImageAsTexture(
//    std::shared_ptr<gl::Texture<TextureType> > & texture,
//    Resource resource,
//    glm::uvec2 & outSize,
//    GLenum target = TextureType) {
//    typedef gl::Texture<TextureType>
//      Texture;
//    typedef std::shared_ptr<gl::Texture<TextureType> >
//      TexturePtr;
//    typedef std::vector<unsigned char>
//      Vector;
//    Vector imageData;
//    getImageData(resource, outSize, imageData);
//    texture = TexturePtr(new Texture());
//    texture->bind();
//#ifdef HAVE_OPENCV
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 0);
//#endif
//    texture->image2d(outSize, &imageData[0]);
//    texture->parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    texture->parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    Texture::unbind();
//  }

  ///**
  //* A convenience method for loading images into textures
  //* when you don't care about the dimensions of the image
  //*/
  //template<
  //  GLenum TextureType = GL_TEXTURE_2D, 
  //  GLenum TextureFomat = GL_RGBA8
  //>
  //static typename gl::Texture<TextureType, TextureFomat>::Ptr  
  //  getImageAsTexture(
  //    Resource resource,
  //    GLenum target = TextureType) {
  //  glm::uvec2 imageSize;
  //  std::shared_ptr<gl::Texture<TextureType, TextureFomat> > texture;
  //  getImageAsTexture(texture, resource, target);
  //  return texture;
  //}

  ///**
  // * A convenience method for loading images into textures
  // * when you don't care about the dimensions of the image
  // */
  //template<
  //  GLenum TextureType = GL_TEXTURE_2D,
  //  GLenum TextureFomat = GL_RGBA8
  //>
  //static void getImageAsTexture(
  //  std::shared_ptr<gl::Texture<TextureType, TextureFomat> > & texture,
  //  Resource resource,
  //  GLenum target = TextureType) {
  //  glm::uvec2 imageSize;
  //  getImageAsTexture(texture, resource, imageSize, target);
  //}

  ///**
  //* A convenience method for loading unsigned char (RGB) pixels into textures.
  //*/
  //static gl::TexturePtr getImageAsTexture(const glm::uvec2 &dimensions, unsigned char *pixels = nullptr) {
  //  gl::TexturePtr texture = gl::TexturePtr(new gl::Texture2d());
  //  texture->bind();
  //  texture->image2d(dimensions, pixels);
  //  texture->parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  //  texture->parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  //  texture->unbind();
  //  return texture;
  //}

  //template<GLenum TextureType = GL_TEXTURE_2D>
  //static void getImageAsGeometryAndTexture(
  //  Resource resource,
  //  gl::GeometryPtr & geometry,
  //  std::shared_ptr<gl::Texture<TextureType> > & texture) {

  //  glm::uvec2 imageSize;
  //  GlUtils::getImageAsTexture(texture, resource, imageSize);
  //  float imageAspectRatio = glm::aspect(imageSize);
  //  glm::vec2 geometryMax(1.0f, 1.0f / imageAspectRatio);
  //  glm::vec2 geometryMin = geometryMax * -1.0f;
  //  geometry = GlUtils::getQuadGeometry(geometryMin, geometryMax);
  //}

//  static const Mesh & getMesh(Resource resource);
  //static Text::FontPtr getFont(Resource resource);
  //static Text::FontPtr getDefaultFont();
  //static void renderGeometry(
  //    const gl::GeometryPtr & geometry,
  //    gl::ProgramPtr program);

  //static void renderSkybox(Resource firstResource);
  //static void renderBunny();
  //static void renderArtificialHorizon( float alpha = 1.0f );
  //static void renderRift();

  //static void renderParagraph(const std::string & str);

  //static void renderString(const std::string & str, glm::vec2 & cursor,
  //    float fontSize = 12.0f, Resource font =
  //        Resource::FONTS_INCONSOLATA_MEDIUM_SDFF);

  //static void renderString(const std::string & str, glm::vec3 & cursor,
  //    float fontSize = 12.0f, Resource font =
  //        Resource::FONTS_INCONSOLATA_MEDIUM_SDFF);

  //static void tumble(const glm::vec3 & camera = glm::vec3(0, 0, 1));

  //static void renderFloor();
  //static void renderManikin();
  //static void renderCubeScene(float ipd = OVR_DEFAULT_IPD, float eyeHeight = OVR_DEFAULT_EYE_HEIGHT);

  //static void cubeRecurse(int depth = 6, float elapsed = Platform::elapsedSeconds());
  //static void dancingCubes(int elements = 8, float elapsed = Platform::elapsedSeconds());

};
