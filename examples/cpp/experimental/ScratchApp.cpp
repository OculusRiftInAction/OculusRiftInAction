#include "Common.h"
#include <oglplus/shapes/plane.hpp>

static const glm::uvec2 WINDOW_SIZE(1280, 800);
static const glm::ivec2 WINDOW_POS(100, 100);
static const glm::uvec2 RES(WINDOW_SIZE.x / 8, WINDOW_SIZE.y / 8);
static const glm::uvec2 RASTER_RES(RES.x * 2, RES.y * 2);

using namespace oglplus;

typedef std::vector<vec3> vv3;
void rasterise(vec3 vs[3], vv3 & out) {
}


uvec2 mapNdcToPixel(const vec3 & v, const uvec2 & res) {
  vec2(v.x, v.y)
}

uvec2 mapNdcToPixel(const vec3 & v, const uvec2 & res) {
  vec2(v.x, v.y)
}


struct RasterizerExample : public GlfwApp {
  ProgramPtr program;
  ShapeWrapperPtr shape;
  TexturePtr tex;
  BufferPtr edge;

  virtual GLFWwindow * createRenderingTarget(glm::uvec2 & outSize, glm::ivec2 & outPosition) {
    outSize = WINDOW_SIZE;
    outPosition = WINDOW_POS;
    return glfw::createWindow(outSize, outPosition);
  }

  virtual void initGl() {
    Context::ClearColor(0.1f, 0.1f, 0.1f, 1);

    program = oria::loadProgram(
      Resource::SHADERS_RASTERIZER_VS,
      Resource::SHADERS_RASTERIZER_FS);
    
    shapes::Plane plane(Vec3f(1, 0, 0), Vec3f(0, 1, 0)); 
    shape = ShapeWrapperPtr(new shapes::ShapeWrapper({ "Position", "TexCoord" }, plane, *program));
    {
      std::vector<GLfloat> posv;
      plane.Positions(posv);
      std::vector<GLfloat> edgev;
      for (int i = 0; i < posv.size(); i += 3) {
        float x = posv[i] * RES.x / 2;
        edgev.push_back(x);
        float y = posv[i + 1] * RES.y / 2;
        edgev.push_back(y);
      }
      edge = BufferPtr(new Buffer());
      edge->Bind(Buffer::Target::Array);
      Buffer::Data(Buffer::Target::Array, edgev);
    }

    shape->Use();
    VertexArrayAttrib(*program, 4)
      .Setup<GLfloat>(2)
      .Enable();
    NoVertexArray().Bind();
    buildTextureData();
  }
  
  void buildTextureData() {
    std::vector<uint8_t> data;
    data.resize(RES.x * RES.y * 3);
    memset(&data[0], 255, data.size());
    for (size_t i = 0; i < data.size(); ++i) {
      if (0 == i % 2) data[i] = 0;
    }
    
    tex = TexturePtr(new Texture());
    Context::Bound(Texture::Target::_2D, *tex)
      .MinFilter(TextureMinFilter::Nearest)
      .MagFilter(TextureMagFilter::Nearest)
      .WrapS(TextureWrap::MirroredRepeat)
      .WrapT(TextureWrap::MirroredRepeat)
      .Image2D(0, PixelDataInternalFormat::RGBA, RES.x, RES.y, 0, PixelDataFormat::RGB, PixelDataType::UnsignedByte, &data[0]);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, RES.X, RES.y, 0, GL_RGB, GL_UNSIGNED_BYTE, )
  }

  virtual void draw() {
    Context::Clear().ColorBuffer().DepthBuffer();
    oria::renderGeometry(shape, program, { [&]{
      glActiveTexture(GL_TEXTURE0);
      tex->Bind(Texture::Target::_2D);
    } });
  }
};

RUN_APP(RasterizerExample);

