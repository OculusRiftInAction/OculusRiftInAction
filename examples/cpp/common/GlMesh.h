/*
 * GlMesh.h
 *
 *  Created on: Nov 9, 2013
 *      Author: bdavis
 */

#pragma once

struct Mesh {
  typedef std::vector<glm::vec4> VVec4;
  typedef std::vector<glm::vec3> VVec3;
  typedef std::vector<glm::vec2> VVec2;
  typedef std::vector<GLuint> VS;

  gl::MatrixStack model;
  glm::vec3 color{1, 1, 1};

  VVec4 positions;
  VVec4 normals;
  VVec3 colors;
  VVec2 texCoords;
  VS indices;
  public:
  Mesh() {
  }

  Mesh(const Mesh & other)
    : model(other.model), color(other.color),
      positions(other.positions), normals(other.normals), colors(other.colors), texCoords(other.texCoords), indices(other.indices)
  {
  }

  Mesh(const std::string & file) {
    loadCtm(file);
  }

  void loadCtm(const std::string & file);
  void clear();
  void validate() const;

  gl::MatrixStack & getModel() {
    return model;
  }

  glm::vec3 & getColor() {
    return color;
  }

  void compare(const Mesh & mesh) const {
    size_t size = positions.size();
    size_t osize = mesh.positions.size();
    assert(size == osize);
    for (size_t i = 0; i < size; ++i) {
      const glm::vec4 & pos = positions[i];
      const glm::vec4 & opos = mesh.positions[i];
      assert(pos == opos);
    }
  }

  void fillColors(bool force = false);
  void fillNormals(bool force = false);
  void addVertex(const glm::vec4 & vertex);
  void addVertex(const glm::vec3 & vertex);
  void addTexCoord(const glm::vec2 & texCoord);
  void addMesh(const Mesh & mesh, bool populateColor = false);
  void addQuad(float width = 1.0f, float height = 1.0f);
  void addQuad(const glm::vec2 & size) {
    addQuad(size.x, size.y);
  }

  size_t getAttributeCount() const {
    size_t attributeCount = 1;
    if (!normals.empty()) {
      attributeCount++;
    }
    if (!colors.empty()) {
      attributeCount++;
    }
    if (!texCoords.empty()) {
      attributeCount++;
    }
    return attributeCount;
  }

  int getFlags() const {
    int flags = 0;
    if (!normals.empty()) {
      flags |= gl::Geometry::Flag::HAS_NORMAL;
    }
    if (!colors.empty()) {
      flags |= gl::Geometry::Flag::HAS_COLOR;
    }
    if (!texCoords.empty()) {
      flags |= gl::Geometry::Flag::HAS_TEXTURE;
    }
    return flags;
  }

  std::vector<glm::vec4> buildVertices() const;

  // Converts an in CPU memory mesh to an in GPU memory
  // geometry object (represented as some gl buffers and
  // a vertex array object
  gl::GeometryPtr getGeometry(GLenum elementType = GL_TRIANGLES) const;
};

typedef std::shared_ptr<Mesh> MeshPtr;
