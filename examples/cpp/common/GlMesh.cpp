/*
 * GlMesh.h
 *
 *  Created on: Nov 9, 2013
 *      Author: bdavis
 */

#include "Common.h"
#include <openctmpp.h>

void Mesh::clear() {
  positions.clear();
  normals.clear();
  colors.clear();
  texCoords.clear();
  indices.clear();
}

void Mesh::loadCtm(const std::string & data) {
  clear();
  CTMimporter importer;
    importer.LoadData(data);

  int vertexCount = importer.GetInteger(CTM_VERTEX_COUNT);
  positions.resize(vertexCount);
  const float * ctmData = importer.GetFloatArray(CTM_VERTICES);
  for (int i = 0; i < vertexCount; ++i) {
    positions[i] = glm::vec4(glm::make_vec3(ctmData + (i * 3)), 1);
  }

  if (importer.GetInteger(CTM_UV_MAP_COUNT) > 0) {
    const float * ctmData = importer.GetFloatArray(CTM_UV_MAP_1);
    texCoords.resize(vertexCount);
    for (int i = 0; i < vertexCount; ++i) {
      texCoords[i] = glm::make_vec2(ctmData + (i * 2));
    }
  }

  bool hasNormals = importer.GetInteger(CTM_HAS_NORMALS) ? true : false;
  if (hasNormals) {
    normals.resize(vertexCount);
    ctmData = importer.GetFloatArray(CTM_NORMALS);
    for (int i = 0; i < vertexCount; ++i) {
      normals[i] = glm::vec4(glm::make_vec3(ctmData + (i * 3)), 1);
    }
  }

  int indexCount = 3 * importer.GetInteger(CTM_TRIANGLE_COUNT);
  const CTMuint * ctmIntData = importer.GetIntegerArray(CTM_INDICES);
  indices.resize(indexCount);
  for (int i = 0; i < indexCount; ++i) {
    indices[i] = *(ctmIntData + i);
  }
}

glm::vec3 transform(const glm::mat4 & mat, const glm::vec3 & vec) {
  glm::vec4 result = mat * glm::vec4(vec, 1);
  return glm::vec3(result.x, result.y, result.z);
}

template<typename T>
void add_all(T & dest, const T & src) {
  dest.reserve(dest.size() + src.size());
  dest.insert(dest.end(), src.begin(), src.end());
}

template<typename T>
void add_all_transformed(const glm::mat4 & xfm, T & dest, const T & src) {
  int destSize = dest.size();
  dest.reserve(dest.size() + src.size());
  for (size_t i = 0; i < src.size(); ++i) {
    dest.push_back(xfm * src[i]);
  }
}

template<typename T>
void add_all_incremented(const size_t increment, T & dest, const T & src) {
  dest.reserve(dest.size() + src.size());
  for (size_t i = 0; i < src.size(); ++i) {
    dest.push_back(src[i] + increment);
  }
}

void Mesh::fillColors(bool force) {
  if (force || colors.size()) {
    if (colors.size() != positions.size()) {
      add_all(colors, VVec3(positions.size() - colors.size(), color));
    }
  }
}

void Mesh::fillNormals(bool force) {
  if (force || normals.size()) {
    while (normals.size() != positions.size()) {
      normals.push_back(positions[normals.size()]);
    }
  }
}

void Mesh::addVertex(const glm::vec4 & vertex) {
  positions.push_back(model.top() * vertex);
  indices.push_back((GLuint)indices.size());
}

void Mesh::addVertex(const glm::vec3 & vertex) {
  positions.push_back(glm::vec4(transform(model.top(), vertex), 1));
  indices.push_back((GLuint)indices.size());
}

void Mesh::addTexCoord(const glm::vec2 & texCoord) {
  texCoords.push_back(texCoord);
}


void Mesh::addMesh(const Mesh & mesh, bool forceColor) {
  int indexOffset = positions.size();

  // Positions are transformed
  add_all_transformed(model.top(), positions, mesh.positions);

  // normals are transformed with only the rotation, not the translation
  model.push().untranslate();
  add_all_transformed(model.top(), normals, mesh.normals);
  model.pop();

  // colors and tex coords are simply copied
  add_all(colors, mesh.colors);
  fillColors(forceColor);

  add_all(texCoords, mesh.texCoords);
  if (texCoords.size() && texCoords.size() != positions.size()) {
    add_all(texCoords, VVec2(positions.size() - texCoords.size(), glm::vec2()));
  }

  // indices are copied and incremented
  add_all_incremented(indexOffset, indices, mesh.indices);
}

void Mesh::addQuad(float width, float height) {
  int indexOffset = positions.size();
  float x = width / 2.0f;
  float y = height / 2.0f;

  // Positions are transformed
  VVec4 quad;
  quad.push_back(glm::vec4(-x, -y, 0, 1));
  quad.push_back(glm::vec4(x, -y, 0, 1));
  quad.push_back(glm::vec4(x, y, 0, 1));
  quad.push_back(glm::vec4(-x, y, 0, 1));
  add_all_transformed(model.top(), positions, quad);

  // normals are transformed only by rotation, not translation
  VVec4 norm;
  for (int i = 0; i < 4; i++) {
    norm.push_back(glm::vec4(0, 0, 1, 1));
  }
  model.push().untranslate();
  add_all_transformed(model.top(), normals, norm);
  model.pop();

  // indices are copied and incremented
  VS quadIndices;
  quadIndices.push_back(0);
  quadIndices.push_back(1);
  quadIndices.push_back(2);
  quadIndices.push_back(0);
  quadIndices.push_back(2);
  quadIndices.push_back(3);
  add_all_incremented(indexOffset, indices, quadIndices);

  fillColors();
}

void Mesh::validate() const {
  size_t vertexCount = positions.size();
  if (!normals.empty() && normals.size() != vertexCount) {
    throw new std::runtime_error("Incorrect number of normals");
  }
  if (!colors.empty() && colors.size() != vertexCount) {
    throw new std::runtime_error("Incorrect number of colors");
  }
  if (!texCoords.empty() && texCoords.size() != vertexCount) {
    throw new std::runtime_error("Incorrect number of texture coordinates");
  }
}

std::vector<glm::vec4> Mesh::buildVertices() const {
  int flags = getFlags();
  size_t attributeCount = getAttributeCount();
  size_t vertexCount = positions.size();
  std::vector<glm::vec4> vertices;
  vertices.reserve(vertexCount * attributeCount);
  for (size_t i = 0; i < vertexCount; ++i) {
    vertices.push_back(positions[i]);
    if (flags & gl::Geometry::Flag::HAS_NORMAL) {
      vertices.push_back(normals[i]);
    }
    if (flags & gl::Geometry::Flag::HAS_COLOR) {
      vertices.push_back(glm::vec4(colors[i], 1));
    }
    if (flags & gl::Geometry::Flag::HAS_TEXTURE) {
      vertices.push_back(glm::vec4(texCoords[i], 1, 1));
    }
  }
  return vertices;
}

gl::GeometryPtr Mesh::getGeometry(GLenum elementType) const {
  validate();
  int flags = getFlags();
  size_t attributeCount = getAttributeCount();
  std::vector<glm::vec4> vertices = buildVertices();
  // TODO add triangle stripping algorithm?
  int elements;
  int verticesPerElement;
  switch (elementType) {
  case GL_LINES:
    elements = (unsigned int)indices.size() / 2;
      verticesPerElement = 2;
      break;
  case GL_TRIANGLES:
    elements = (unsigned int)indices.size() / 3;
      verticesPerElement = 3;
      break;
  case GL_TRIANGLE_STRIP:
    elements = (unsigned int)indices.size();
    verticesPerElement = 1;
    break;
  case GL_POINTS:
    elements = (unsigned int)indices.size();
    verticesPerElement = 1;
    break;
  default:
      throw std::runtime_error("unsupported geometry type");
  }
  return gl::GeometryPtr(new gl::Geometry(vertices, indices, elements, flags, elementType, verticesPerElement));
}
