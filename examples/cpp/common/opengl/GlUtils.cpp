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

#include "Common.h"

#include "Font.h"
#include <openctmpp.h>
#pragma warning( disable : 4068 4244 4267 4065 4101 4244)
#include <oglplus/bound/buffer.hpp>
#include <oglplus/shapes/cube.hpp>
#include <oglplus/shapes/sphere.hpp>
#include <oglplus/images/image.hpp>
#include <oglplus/shapes/wicker_torus.hpp>
#include <oglplus/shapes/sky_box.hpp>
#include <oglplus/shapes/plane.hpp>
#include <oglplus/shapes/grid.hpp>
#include <oglplus/shapes/vector.hpp>
#include <oglplus/opt/list_init.hpp>
#include <oglplus/shapes/obj_mesh.hpp>

namespace oglplus {
  namespace shapes {

    /// Class providing attributes and instructions for drawing of mesh loaded from obj
    class CtmMesh
      : public DrawingInstructionWriter
      , public DrawMode
    {
    public:
      typedef std::vector<GLuint> IndexArray;

    private:
      struct _loading_options
      {
        bool load_normals;
        bool load_tangents;
        bool load_bitangents;
        bool load_texcoords;
        bool load_materials;

        _loading_options(bool load_all = true)
        {
          All(load_all);
        }

        _loading_options& All(bool load_all = true)
        {
          load_normals = load_all;
          load_tangents = load_all;
          load_bitangents = load_all;
          load_texcoords = load_all;
          load_materials = load_all;
          return *this;
        }

        _loading_options& Nothing(void)
        {
          return All(false);
        }

        _loading_options& Normals(bool load = true)
        {
          load_normals = load;
          return *this;
        }

        _loading_options& Tangents(bool load = true)
        {
          load_tangents = load;
          return *this;
        }

        _loading_options& Bitangents(bool load = true)
        {
          load_bitangents = load;
          return *this;
        }

        _loading_options& TexCoords(bool load = true)
        {
          load_texcoords = load;
          return *this;
        }

        _loading_options& Materials(bool load = true)
        {
          load_materials = load;
          return *this;
        }
      };

      /// The type of the index container returned by Indices()
      // vertex positions
      std::vector<float> _pos_data;
      // vertex normals
      std::vector<float> _nml_data;
      // vertex tex coords
      std::vector<float> _tex_data;
      IndexArray _idx_data;
      unsigned int _prim_count;


      struct _vert_indices
      {
        GLuint _pos;
        GLuint _nml;
        GLuint _tex;
        GLuint _mtl;

        _vert_indices(void)
          : _pos(0)
          , _nml(0)
          , _tex(0)
          , _mtl(0)
        { }
      };

      bool _load_index(
        GLuint& value,
        GLuint count,
        std::string::const_iterator& i,
        std::string::const_iterator& e
        );

      void _call_load_meshes(
        Resource resource,
        aux::AnyInputIter<const char*> names_begin,
        aux::AnyInputIter<const char*> names_end,
        _loading_options opts
        ) {

        CTMimporter importer;
        importer.LoadData(Platform::getResourceString(resource));
        int vertexCount = importer.GetInteger(CTM_VERTEX_COUNT);
        {
          const float * ctmData = importer.GetFloatArray(CTM_VERTICES);
          _pos_data = std::vector<float>(ctmData, ctmData + (vertexCount * 3));
        }

        if (opts.load_texcoords && importer.GetInteger(CTM_UV_MAP_COUNT)) {
          const float * ctmData = importer.GetFloatArray(CTM_UV_MAP_1);
          _tex_data = std::vector<float>(ctmData, ctmData + (vertexCount * 2));
        }

        if (opts.load_normals && importer.GetInteger(CTM_HAS_NORMALS)) {
          const float * ctmData = importer.GetFloatArray(CTM_NORMALS);
          _nml_data = std::vector<float>(ctmData, ctmData + (vertexCount * 3));
        }

        {
          _prim_count = importer.GetInteger(CTM_TRIANGLE_COUNT);
          int indexCount = 3 * _prim_count;
          const CTMuint * ctmIntData = importer.GetIntegerArray(CTM_INDICES);
          _idx_data = IndexArray(ctmIntData, ctmIntData + indexCount);
        }
      }

    public:
      typedef _loading_options LoadingOptions;

      CtmMesh(
        Resource resource,
        LoadingOptions opts = LoadingOptions()
        )
      {
        const char** p = nullptr;
        _call_load_meshes(resource, p, p, opts);
      }

      /// Returns the winding direction of faces
      FaceOrientation FaceWinding(void) const
      {
        return FaceOrientation::CCW;
      }

      typedef GLuint(CtmMesh::*VertexAttribFunc)(std::vector<GLfloat>&) const;

      /// Makes the vertex positions and returns the number of values per vertex
      template <typename T>
      GLuint Positions(std::vector<T>& dest) const
      {
        dest.clear();
        dest.insert(dest.begin(), _pos_data.begin(), _pos_data.end());
        return 3;
      }

      /// Makes the vertex normals and returns the number of values per vertex
      template <typename T>
      GLuint Normals(std::vector<T>& dest) const
      {
        dest.clear();
        dest.insert(dest.begin(), _nml_data.begin(), _nml_data.end());
        return 3;
      }

      /// Makes the vertex tangents and returns the number of values per vertex
      template <typename T>
      GLuint Tangents(std::vector<T>& dest) const
      {
        dest.clear();
        return 3;
      }

      /// Makes the vertex bi-tangents and returns the number of values per vertex
      template <typename T>
      GLuint Bitangents(std::vector<T>& dest) const
      {
        dest.clear();
        return 3;
      }

      /// Makes the texture coordinates returns the number of values per vertex
      template <typename T>
      GLuint TexCoordinates(std::vector<T>& dest) const
      {
        dest.clear();
        dest.insert(dest.begin(), _tex_data.begin(), _tex_data.end());
        return 2;
      }

      typedef VertexAttribsInfo<
        CtmMesh,
        std::tuple<
        VertexPositionsTag,
        VertexNormalsTag,
        VertexTangentsTag,
        VertexBitangentsTag,
        VertexTexCoordinatesTag
        >
      > VertexAttribs;

      Spheref MakeBoundingSphere(void) const {
          GLfloat min_x = _pos_data[3], max_x = _pos_data[3];
          GLfloat min_y = _pos_data[4], max_y = _pos_data[4];
          GLfloat min_z = _pos_data[5], max_z = _pos_data[5];
          for (std::size_t v = 0, vn = _pos_data.size() / 3; v != vn; ++v)
          {
            GLfloat x = _pos_data[v * 3 + 0];
            GLfloat y = _pos_data[v * 3 + 1];
            GLfloat z = _pos_data[v * 3 + 2];

            if (min_x > x) min_x = x;
            if (min_y > y) min_y = y;
            if (min_z > z) min_z = z;
            if (max_x < x) max_x = x;
            if (max_y < y) max_y = y;
            if (max_z < z) max_z = z;
          }

          Vec3f c(
            (min_x + max_x) * 0.5f,
            (min_y + max_y) * 0.5f,
            (min_z + max_z) * 0.5f
            );

          return Spheref(
            c.x(), c.y(), c.z(),
            Distance(c, Vec3f(min_x, min_y, min_z))
            );
      }

      /// Queries the bounding sphere coordinates and dimensions
      template <typename T>
      void BoundingSphere(oglplus::Sphere<T>& bounding_sphere) const
      {
        bounding_sphere = oglplus::Sphere<T>(MakeBoundingSphere());
      }


      /// Returns element indices that are used with the drawing instructions
      const IndexArray & Indices(Default = Default()) const
      {
        return _idx_data;
      }

      /// Returns the instructions for rendering of faces
      DrawingInstructions Instructions(PrimitiveType primitive) const {
        DrawingInstructions instr = this->MakeInstructions();
        DrawOperation operation;
        operation.method = DrawOperation::Method::DrawElements;
        operation.mode = primitive;
        operation.first = 0;
        operation.count = _prim_count * 3;
        operation.restart_index = DrawOperation::NoRestartIndex();
        operation.phase = 0;
        this->AddInstruction(instr, operation);
        return std::move(instr);
      }

      /// Returns the instructions for rendering of faces
      DrawingInstructions Instructions(Default = Default()) const
      {
        return Instructions(PrimitiveType::Triangles);
      }
    };
  } // shapes
} // oglplus

#pragma warning( default : 4068 4244 4267 4065 4101)


namespace oria {

  std::wstring toUtf16(const std::string & text) {
    //    wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide(text.begin(), text.end()); //= converter.from_bytes(narrow.c_str());
    return wide;
  }

  Text::FontPtr getFont(Resource fontName) {
    static std::map<Resource, Text::FontPtr> fonts;
    if (fonts.find(fontName) == fonts.end()) {
      std::string fontData = Platform::getResourceString(fontName);
      Text::FontPtr result(new Text::Font());
      result->read((const void*)fontData.data(), fontData.size());
      fonts[fontName] = result;
    }
    return fonts[fontName];
  }

  Text::FontPtr getDefaultFont() {
    return getFont(Resource::FONTS_INCONSOLATA_MEDIUM_SDFF);
  }


  void renderString(const std::string & cstr, glm::vec2 & cursor,
    float fontSize, Resource fontResource) {
    getFont(fontResource)->renderString(toUtf16(cstr), cursor, fontSize);
  }

  void renderString(const std::string & str, glm::vec3 & cursor3d,
    float fontSize, Resource fontResource) {
    glm::vec4 target = glm::vec4(cursor3d, 0);
    target = Stacks::projection().top() * Stacks::modelview().top() * target;
    glm::vec2 newCursor(target.x, target.y);
    renderString(str, newCursor, fontSize, fontResource);
  }

  void bindLights(ProgramPtr & program) {
    using namespace oglplus;
    Lights & lights = Stacks::lights();
    int count = (int)lights.lightPositions.size();
    Uniform<vec4>(*program, "Ambient").Set(lights.ambient);
    Uniform<int>(*program, "LightCount").Set(count);
    if (count) {
      Uniform<Vec4f>(*program, "LightColor[0]").SetValues(count, (Vec4f*)&lights.lightColors.at(0).r);
      Uniform<Vec4f>(*program, "LightPosition[0]").SetValues(count, (Vec4f*)&lights.lightPositions.at(0).x);
    }
  }

  typedef std::function<void()> Lambda;
  typedef std::list<Lambda> LambdaList;
  template <typename Iter>
  void renderGeometryWithLambdas(ShapeWrapperPtr & shape, ProgramPtr & program, Iter begin, const Iter & end) {
    program->Use();

    Mat4Uniform(*program, "ModelView").Set(Stacks::modelview().top());
    Mat4Uniform(*program, "Projection").Set(Stacks::projection().top());

    std::for_each(begin, end, [&](const std::function<void()>&f){
      f();
    });

    shape->Use();
    shape->Draw();

    oglplus::NoProgram().Bind();
    oglplus::NoVertexArray().Bind();
  }

  void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program, std::function<void()> lambda) {
    renderGeometry(shape, program, LambdaList({ lambda }) );
  }

  void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program, const std::list<std::function<void()>> & list) {
    renderGeometryWithLambdas(shape, program, list.begin(), list.end());
  }

  void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program) {
    static const std::list<std::function<void()>> EMPTY_LIST;
    renderGeometryWithLambdas(shape, program, EMPTY_LIST.begin(), EMPTY_LIST.end());
  }


  void renderCube(const glm::vec3 & color) {
    using namespace oglplus;

    static ProgramPtr program;
    static ShapeWrapperPtr shape;
    if (!program) {
      program = loadProgram(Resource::SHADERS_SIMPLE_VS, Resource::SHADERS_COLORED_FS);
      shape = ShapeWrapperPtr(new shapes::ShapeWrapper(List("Position").Get(), shapes::Cube(), *program));
      Platform::addShutdownHook([&]{
        program.reset();
        shape.reset();
      });
    }
    program->Use();
    Uniform<vec4>(*program, "Color").Set(vec4(color, 1));
    renderGeometry(shape, program);
  }

  void renderColorCube() {
    using namespace oglplus;

    static ProgramPtr program;
    static ShapeWrapperPtr shape;
    if (!program) {
      program = loadProgram(Resource::SHADERS_COLORCUBE_VS, Resource::SHADERS_COLORCUBE_FS);
      shape = ShapeWrapperPtr(new shapes::ShapeWrapper(List("Position")("Normal").Get(), shapes::Cube(), *program));;
      Platform::addShutdownHook([&]{
        program.reset();
        shape.reset();
      });
    }

    renderGeometry(shape, program);
  }

  ShapeWrapperPtr loadSkybox(ProgramPtr program) {
    using namespace oglplus;
    ShapeWrapperPtr shape = ShapeWrapperPtr(new shapes::ShapeWrapper(List("Position").Get(), shapes::SkyBox(), *program));
    return shape;
  }


  ShapeWrapperPtr loadPlane(ProgramPtr program, float aspect) {
    using namespace oglplus;
    Vec3f a(1, 0, 0);
    Vec3f b(0, 1, 0);
    if (aspect > 1) {
      b[1] /= aspect;
    } else {
      a[0] *= aspect;
    }
    return ShapeWrapperPtr(
      new shapes::ShapeWrapper(
        { "Position", "TexCoord" }, 
        shapes::Plane(a, b), 
        *program
      )
    );
  }

  void renderSkybox(Resource firstImageResource) {
    using namespace oglplus;

    static ProgramPtr program;
    static ShapeWrapperPtr shape;
    if (!program) {
      program = loadProgram(Resource::SHADERS_CUBEMAP_VS, Resource::SHADERS_CUBEMAP_FS);
      shape = ShapeWrapperPtr(new shapes::ShapeWrapper(List("Position").Get(), shapes::SkyBox(), *program));
      Platform::addShutdownHook([&]{
        program.reset();
        shape.reset();
      });
    }

    TexturePtr texture = loadCubemapTexture(firstImageResource);
    texture->Bind(TextureTarget::CubeMap);
    MatrixStack & mv = Stacks::modelview();
    mv.withPush([&]{
      Context::Disable(Capability::DepthTest);
      Context::Disable(Capability::CullFace);
      renderGeometry(shape, program);
      Context::Enable(Capability::CullFace);
      Context::Enable(Capability::DepthTest);
    });
    DefaultTexture().Bind(TextureTarget::CubeMap);
  }

  void renderFloor() {
    using namespace oglplus;
    const float SIZE = 100;
    static ProgramPtr program;
    static ShapeWrapperPtr shape;
    static TexturePtr texture;
    if (!program) {
      program = loadProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);
      shape = ShapeWrapperPtr(new shapes::ShapeWrapper(List("Position")("TexCoord").Get(), shapes::Plane(), *program));
      texture = load2dTexture(Resource::IMAGES_FLOOR_PNG);
      Context::Bound(TextureTarget::_2D, *texture).MinFilter(TextureMinFilter::LinearMipmapNearest).GenerateMipmap();
      Platform::addShutdownHook([&]{
        program.reset();
        shape.reset();
        texture.reset();
      });
    }

    texture->Bind(TextureTarget::_2D);
    MatrixStack & mv = Stacks::modelview();
    mv.withPush([&]{
      mv.scale(vec3(SIZE));
      renderGeometry(shape, program, [&]{
        oglplus::Uniform<vec2>(*program, "UvMultiplier").Set(vec2(SIZE * 2.0f));
      });
    });

    DefaultTexture().Bind(TextureTarget::_2D);
  }

  ShapeWrapperPtr loadShape(const std::initializer_list<const GLchar*>& names, Resource resource) {
    using namespace oglplus;
    return ShapeWrapperPtr(new shapes::ShapeWrapper(names, shapes::CtmMesh(resource)));
  }

  ShapeWrapperPtr loadShape(const std::initializer_list<const GLchar*>& names, Resource resource, ProgramPtr program) {
    using namespace oglplus;
    return ShapeWrapperPtr(new shapes::ShapeWrapper(names, shapes::CtmMesh(resource), *program));
  }

  void renderManikin() {
    static ProgramPtr program;
    static ShapeWrapperPtr shape;

    if (!program) {
      program = loadProgram(Resource::SHADERS_LIT_VS, Resource::SHADERS_LITCOLORED_FS);
      shape = loadShape({ "Position", "Normal" }, Resource::MESHES_MANIKIN_CTM, program);
      Platform::addShutdownHook([&]{
        program.reset();
        shape.reset();
      });
    }


    renderGeometry(shape, program, [&]{
      bindLights(program);
    });
  }

  void renderRift(float alpha) {
    using namespace oglplus;
    static ProgramPtr program;
    static ShapeWrapperPtr shape;
    if (!program) {
      Platform::addShutdownHook([&]{
        program.reset();
        shape.reset();
      });

      program = loadProgram(Resource::SHADERS_LIT_VS, Resource::SHADERS_LITCOLORED_FS);
      shape = ::oria::loadShape({ "Position", "Normal" }, Resource::MESHES_RIFT_CTM, program);
    }

    auto & mv = Stacks::modelview();
    mv.withPush([&]{
      mv.rotate(-HALF_PI - 0.22f, Vectors::X_AXIS).scale(0.5f);
      renderGeometry(shape, program, [&] {
        Uniform<float>(*program, "ForceAlpha").Set(alpha);
        oria::bindLights(program);
      });
    });
  }

  void renderArtificialHorizon(float alpha) {
    using namespace oglplus;
    static ProgramPtr program;
    static ShapeWrapperPtr shape;
    static std::vector<Vec4f> materials = {
        Vec4f(0.351366f, 0.665379f, 0.800000f, 1),
        Vec4f(0.640000f, 0.179600f, 0.000000f, 1),
        Vec4f(0.000000f, 0.000000f, 0.000000f, 1),
        Vec4f(0.171229f, 0.171229f, 0.171229f, 1),
        Vec4f(0.640000f, 0.640000f, 0.640000f, 1)
    };

    if (!program) {
      Platform::addShutdownHook([&]{
        program.reset();
        shape.reset();
      });

      program = loadProgram(Resource::SHADERS_LITMATERIALS_VS, Resource::SHADERS_LITCOLORED_FS);
      std::stringstream && stream = Platform::getResourceStream(Resource::MESHES_ARTIFICIAL_HORIZON_OBJ);
      shapes::ObjMesh mesh(stream);
      shape = ShapeWrapperPtr(new shapes::ShapeWrapper({ "Position", "Normal", "Material" }, mesh, *program));
      Uniform<Vec4f>(*program, "Materials[0]").Set(materials);
    }

    auto & mv = Stacks::modelview();
    mv.withPush([&]{
      renderGeometry(shape, program, [&]{
        Uniform<float>(*program, "ForceAlpha").Set(alpha);
        oria::bindLights(program);
      });
    });

  }
  
  void renderManikinScene(float ipd, float eyeHeight) {
    oria::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    oria::renderFloor();
    
    // Scale the size of the cube to the distance between the eyes
    MatrixStack & mv = Stacks::modelview();
    
    mv.withPush([&]{
      mv.translate(glm::vec3(0, eyeHeight, 0)).scale(glm::vec3(ipd));
      oria::renderColorCube();
    });
    
    mv.withPush([&]{
      mv.translate(glm::vec3(0, 0, ipd * -5.0));
      oglplus::Context::Disable(oglplus::Capability::CullFace);
      oria::renderManikin();
    });
  }

  void renderExampleScene(float ipd, float eyeHeight) {
    oria::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    oria::renderFloor();

    MatrixStack & mv = Stacks::modelview();
    for (int j = -1; j <= 1; j++) {
      for (int k = -1; k <= 1; k++) {
        mv.withPush([&]{
          mv.translate(glm::vec3(0, 0.01, 0));
          mv.scale(glm::vec3(4));
          mv.translate(glm::vec3(j, 0, k));
          oria::draw3dGrid();
        });
      }
    }
    mv.withPush([&]{
      mv.translate(glm::vec3(0, eyeHeight, 0)).scale(glm::vec3(ipd));
      oria::renderColorCube();
    });
    mv.withPush([&]{
      mv.translate(glm::vec3(0, eyeHeight / 2, 0)).scale(glm::vec3(ipd / 2, eyeHeight, ipd / 2));
      oria::renderColorCube();
    });
  }

  void GL_CALLBACK debugCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar * message,
    void * userParam) {
    const char * typeStr = "?";
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
      typeStr = "ERROR";
      break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      typeStr = "DEPRECATED_BEHAVIOR";
      break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      typeStr = "UNDEFINED_BEHAVIOR";
      break;
    case GL_DEBUG_TYPE_PORTABILITY:
      typeStr = "PORTABILITY";
      break;
    case GL_DEBUG_TYPE_PERFORMANCE:
      typeStr = "PERFORMANCE";
      break;
    case GL_DEBUG_TYPE_OTHER:
      typeStr = "OTHER";
      break;
    }

    const char * severityStr = "?";
    switch (severity) {
    case GL_DEBUG_SEVERITY_LOW:
      severityStr = "LOW";
      break;
    case GL_DEBUG_SEVERITY_MEDIUM:
      severityStr = "MEDIUM";
      break;
    case GL_DEBUG_SEVERITY_HIGH:
      severityStr = "HIGH";
      break;
    }
    SAY("--- OpenGL Callback Message ---");
    SAY("type: %s\nseverity: %-8s\nid: %d\nmsg: %s", typeStr, severityStr, id,
      message);
    SAY("--- OpenGL Callback Message ---");
  }

  ShapeWrapperPtr loadSphere(const std::initializer_list<const GLchar*>& names, ProgramPtr program) {
    using namespace oglplus;
    return ShapeWrapperPtr(new shapes::ShapeWrapper(names, shapes::Sphere(), *program));
  }

  ShapeWrapperPtr loadGrid(ProgramPtr program) {
    using namespace oglplus;
    return ShapeWrapperPtr(new shapes::ShapeWrapper(std::initializer_list<const GLchar*>({ "Position" }), shapes::Grid(
      Vec3f(0.0f, 0.0f, 0.0f),
      Vec3f(1.0f, 0.0f, 0.0f),
      Vec3f(0.0f, 0.0f, -1.0f),
      8,
      8), *program));
  }

  void draw3dGrid() {
    static ProgramPtr program;
    static ShapeWrapperPtr grid;
    if (!program) {
      program = loadProgram(Resource::SHADERS_SIMPLE_VS, Resource::SHADERS_COLORED_FS);
      grid = loadGrid(program);
      Platform::addShutdownHook([&] {
        program.reset();
        grid.reset();
      });
    }
    renderGeometry(grid, program);
  }

  /*
   For the sin of writing compat mode OpenGL, I will go to the special hell.
  */
  void draw3dVector(const glm::vec3 & end, const glm::vec3 & col) {
    oglplus::NoProgram().Bind();
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(Stacks::projection().top()));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(Stacks::modelview().top()));
    glColor3f(col.r, col.g, col.b);
    glBegin(GL_LINES);
    glVertex3f(0, 0, 0);
    glVertex3f(end.x, end.y, end.z);
    glEnd();
    GLenum err = glGetError();
  }

}

