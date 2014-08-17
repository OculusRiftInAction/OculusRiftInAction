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

#include "GlBuffers.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "Font.h"

class GlUtils {
public:
  static void drawColorCube(bool lit = false);
  static void drawQuad(
      const glm::vec2 & min = glm::vec2(-1),
      const glm::vec2 & max = glm::vec2(1));
  static void drawAngleTicks();
  static void draw3dGrid();
  static void draw3dVector(glm::vec3 vec, const glm::vec3 & col);

  static gl::GeometryPtr getColorCubeGeometry();
  static gl::GeometryPtr getCubeGeometry();
  static gl::GeometryPtr getWireCubeGeometry();

  static gl::GeometryPtr getQuadGeometry(
    float aspect, float size = 2.0f
  );

  static gl::GeometryPtr getQuadGeometry(
      const glm::vec2 & min = glm::vec2(-1),
      const glm::vec2 & max = glm::vec2(1),
      const glm::vec2 & texMin = glm::vec2(0),
      const glm::vec2 & texMax = glm::vec2(1)
  );

  static gl::TextureCubeMapPtr getCubemapTextures(Resource resource);

  static void getImageData(
    const std::vector<unsigned char> & indata,
    glm::uvec2 & outSize,
    std::vector<unsigned char> & outData,
    bool flip = true
  );

  static void getImageData(
    Resource resource,
    glm::uvec2 & outSize,
    std::vector<unsigned char> & outData,
    bool flip = true
    );

  template<GLenum TextureType>
  static void getImageAsTexture(
    std::shared_ptr<gl::Texture<TextureType> > & texture,
    Resource resource,
    glm::uvec2 & outSize,
    GLenum target = TextureType) {
    typedef gl::Texture<TextureType>
      Texture;
    typedef std::shared_ptr<gl::Texture<TextureType> >
      TexturePtr;
    typedef std::vector<unsigned char>
      Vector;
    Vector imageData;
    getImageData(resource, outSize, imageData);
    texture = TexturePtr(new Texture());
    texture->bind();
#ifdef HAVE_OPENCV
    glPixelStorei(GL_UNPACK_ALIGNMENT, 0);
#endif
    texture->image2d(outSize, &imageData[0]);
    texture->parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    texture->parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    Texture::unbind();
  }

  /**
  * A convenience method for loading images into textures
  * when you don't care about the dimensions of the image
  */
  template<
    GLenum TextureType = GL_TEXTURE_2D, 
    GLenum TextureFomat = GL_RGBA8
  >
  static typename gl::Texture<TextureType, TextureFomat>::Ptr  
    getImageAsTexture(
      Resource resource,
      GLenum target = TextureType) {
    glm::uvec2 imageSize;
    std::shared_ptr<gl::Texture<TextureType, TextureFomat> > texture;
    getImageAsTexture(texture, resource, target);
    return texture;
  }


  /**
   * A convenience method for loading images into textures
   * when you don't care about the dimensions of the image
   */
  template<
    GLenum TextureType = GL_TEXTURE_2D,
    GLenum TextureFomat = GL_RGBA8
  >
  static void getImageAsTexture(
    std::shared_ptr<gl::Texture<TextureType, TextureFomat> > & texture,
    Resource resource,
    GLenum target = TextureType) {
    glm::uvec2 imageSize;
    getImageAsTexture(texture, resource, imageSize, target);
  }

  template<GLenum TextureType = GL_TEXTURE_2D>
  static void getImageAsGeometryAndTexture(
    Resource resource,
    gl::GeometryPtr & geometry,
    std::shared_ptr<gl::Texture<TextureType> > & texture) {

    glm::uvec2 imageSize;
    GlUtils::getImageAsTexture(texture, resource, imageSize);
    float imageAspectRatio = glm::aspect(imageSize);
    glm::vec2 geometryMax(1.0f, 1.0f / imageAspectRatio);
    glm::vec2 geometryMin = geometryMax * -1.0f;
    geometry = GlUtils::getQuadGeometry(geometryMin, geometryMax);
  }

  static const Mesh & getMesh(Resource resource);

  static const gl::ProgramPtr & getProgram(
    Resource vertexResource,
    Resource fragmentResource);

  static Text::FontPtr getFont(Resource resource);
  static Text::FontPtr getDefaultFont();

  static void renderGeometry(
      const gl::GeometryPtr & geometry,
      gl::ProgramPtr program);

  static void renderSkybox(Resource firstResource);
  static void renderBunny();
  static void renderArtificialHorizon( float alpha = 1.0f );
  static void renderRift();

  static void renderParagraph(const std::string & str);

  static void renderString(const std::string & str, glm::vec2 & cursor,
      float fontSize = 12.0f, Resource font =
          Resource::FONTS_INCONSOLATA_MEDIUM_SDFF);

  static void renderString(const std::string & str, glm::vec3 & cursor,
      float fontSize = 12.0f, Resource font =
          Resource::FONTS_INCONSOLATA_MEDIUM_SDFF);

  static void tumble(const glm::vec3 & camera = glm::vec3(0, 0, 1));

  static void renderFloor();
  static void renderManikin();

  static void cubeRecurse(int depth = 6, float elapsed = Platform::elapsedSeconds());
  static void dancingCubes(int elements = 8, float elapsed = Platform::elapsedSeconds());


  static const glm::vec3 X_AXIS;
  static const glm::vec3 Y_AXIS;
  static const glm::vec3 Z_AXIS;
  static const glm::vec3 ORIGIN;
  static const glm::vec3 ONE;
  static const glm::vec3 UP;
};

struct Colors {
  static const glm::vec3 gray;
  static const glm::vec3 white;
  static const glm::vec3 red;
  static const glm::vec3 green;
  static const glm::vec3 blue;
  static const glm::vec3 cyan;
  static const glm::vec3 magenta;
  static const glm::vec3 yellow;
  static const glm::vec3 black;
  static const glm::vec3 aliceBlue;
  static const glm::vec3 antiqueWhite;
  static const glm::vec3 aqua;
  static const glm::vec3 aquamarine;
  static const glm::vec3 azure;
  static const glm::vec3 beige;
  static const glm::vec3 bisque;
  static const glm::vec3 blanchedAlmond;
  static const glm::vec3 blueViolet;
  static const glm::vec3 brown;
  static const glm::vec3 burlyWood;
  static const glm::vec3 cadetBlue;
  static const glm::vec3 chartreuse;
  static const glm::vec3 chocolate;
  static const glm::vec3 coral;
  static const glm::vec3 cornflowerBlue;
  static const glm::vec3 cornsilk;
  static const glm::vec3 crimson;
  static const glm::vec3 darkBlue;
  static const glm::vec3 darkCyan;
  static const glm::vec3 darkGoldenRod;
  static const glm::vec3 darkGray;
  static const glm::vec3 darkGrey;
  static const glm::vec3 darkGreen;
  static const glm::vec3 darkKhaki;
  static const glm::vec3 darkMagenta;
  static const glm::vec3 darkOliveGreen;
  static const glm::vec3 darkorange;
  static const glm::vec3 darkOrchid;
  static const glm::vec3 darkRed;
  static const glm::vec3 darkSalmon;
  static const glm::vec3 darkSeaGreen;
  static const glm::vec3 darkSlateBlue;
  static const glm::vec3 darkSlateGray;
  static const glm::vec3 darkSlateGrey;
  static const glm::vec3 darkTurquoise;
  static const glm::vec3 darkViolet;
  static const glm::vec3 deepPink;
  static const glm::vec3 deepSkyBlue;
  static const glm::vec3 dimGray;
  static const glm::vec3 dimGrey;
  static const glm::vec3 dodgerBlue;
  static const glm::vec3 fireBrick;
  static const glm::vec3 floralWhite;
  static const glm::vec3 forestGreen;
  static const glm::vec3 fuchsia;
  static const glm::vec3 gainsboro;
  static const glm::vec3 ghostWhite;
  static const glm::vec3 gold;
  static const glm::vec3 goldenRod;
  static const glm::vec3 grey;
  static const glm::vec3 greenYellow;
  static const glm::vec3 honeyDew;
  static const glm::vec3 hotPink;
  static const glm::vec3 indianRed;
  static const glm::vec3 indigo;
  static const glm::vec3 ivory;
  static const glm::vec3 khaki;
  static const glm::vec3 lavender;
  static const glm::vec3 lavenderBlush;
  static const glm::vec3 lawnGreen;
  static const glm::vec3 lemonChiffon;
  static const glm::vec3 lightBlue;
  static const glm::vec3 lightCoral;
  static const glm::vec3 lightCyan;
  static const glm::vec3 lightGoldenRodYellow;
  static const glm::vec3 lightGray;
  static const glm::vec3 lightGrey;
  static const glm::vec3 lightGreen;
  static const glm::vec3 lightPink;
  static const glm::vec3 lightSalmon;
  static const glm::vec3 lightSeaGreen;
  static const glm::vec3 lightSkyBlue;
  static const glm::vec3 lightSlateGray;
  static const glm::vec3 lightSlateGrey;
  static const glm::vec3 lightSteelBlue;
  static const glm::vec3 lightYellow;
  static const glm::vec3 lime;
  static const glm::vec3 limeGreen;
  static const glm::vec3 linen;
  static const glm::vec3 maroon;
  static const glm::vec3 mediumAquaMarine;
  static const glm::vec3 mediumBlue;
  static const glm::vec3 mediumOrchid;
  static const glm::vec3 mediumPurple;
  static const glm::vec3 mediumSeaGreen;
  static const glm::vec3 mediumSlateBlue;
  static const glm::vec3 mediumSpringGreen;
  static const glm::vec3 mediumTurquoise;
  static const glm::vec3 mediumVioletRed;
  static const glm::vec3 midnightBlue;
  static const glm::vec3 mintCream;
  static const glm::vec3 mistyRose;
  static const glm::vec3 moccasin;
  static const glm::vec3 navajoWhite;
  static const glm::vec3 navy;
  static const glm::vec3 oldLace;
  static const glm::vec3 olive;
  static const glm::vec3 oliveDrab;
  static const glm::vec3 orange;
  static const glm::vec3 orangeRed;
  static const glm::vec3 orchid;
  static const glm::vec3 paleGoldenRod;
  static const glm::vec3 paleGreen;
  static const glm::vec3 paleTurquoise;
  static const glm::vec3 paleVioletRed;
  static const glm::vec3 papayaWhip;
  static const glm::vec3 peachPuff;
  static const glm::vec3 peru;
  static const glm::vec3 pink;
  static const glm::vec3 plum;
  static const glm::vec3 powderBlue;
  static const glm::vec3 purple;
  static const glm::vec3 rosyBrown;
  static const glm::vec3 royalBlue;
  static const glm::vec3 saddleBrown;
  static const glm::vec3 salmon;
  static const glm::vec3 sandyBrown;
  static const glm::vec3 seaGreen;
  static const glm::vec3 seaShell;
  static const glm::vec3 sienna;
  static const glm::vec3 silver;
  static const glm::vec3 skyBlue;
  static const glm::vec3 slateBlue;
  static const glm::vec3 slateGray;
  static const glm::vec3 slateGrey;
  static const glm::vec3 snow;
  static const glm::vec3 springGreen;
  static const glm::vec3 steelBlue;
  static const glm::vec3 blueSteel;
  static const glm::vec3 tan;
  static const glm::vec3 teal;
  static const glm::vec3 thistle;
  static const glm::vec3 tomato;
  static const glm::vec3 turquoise;
  static const glm::vec3 violet;
  static const glm::vec3 wheat;
  static const glm::vec3 whiteSmoke;
  static const glm::vec3 yellowGreen;
};
