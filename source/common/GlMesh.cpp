/*
 * GlMesh.h
 *
 *  Created on: Nov 9, 2013
 *      Author: bdavis
 */

#include "Common.h"
#include <openctmpp.h>

using namespace glm;
using namespace gl;
using namespace std;

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
    positions[i] = glm::vec4(make_vec3(ctmData + (i * 3)), 1);
  }

  bool hasNormals = importer.GetInteger(CTM_HAS_NORMALS) ? true : false;
  if (hasNormals) {
    normals.resize(vertexCount);
    ctmData = importer.GetFloatArray(CTM_NORMALS);
    for (int i = 0; i < vertexCount; ++i) {
      normals[i] = glm::vec4(make_vec3(ctmData + (i * 3)), 1);
    }
  }

  int indexCount = 3 * importer.GetInteger(CTM_TRIANGLE_COUNT);
  const CTMuint * ctmIntData = importer.GetIntegerArray(CTM_INDICES);
  indices.resize(indexCount);
  for (int i = 0; i < indexCount; ++i) {
    indices[i] = *(ctmIntData + i);
  }
}

vec3 transform(const mat4 & mat, const vec3 & vec) {
  vec4 result = mat * vec4(vec, 1);
  return vec3(result.x, result.y, result.z);
}

template<typename T>
void add_all(T & dest, const T & src) {
  dest.reserve(dest.size() + src.size());
  dest.insert(dest.end(), src.begin(), src.end());
}

template<typename T>
void add_all_transformed(const mat4 & xfm, T & dest, const T & src) {
  int destSize = dest.size();
  dest.reserve(dest.size() + src.size());
  for (int i = 0; i < src.size(); ++i) {
    dest.push_back(xfm * src[i]);
  }
}

template<typename T>
void add_all_incremented(const size_t increment, T & dest, const T & src) {
  dest.reserve(dest.size() + src.size());
  for (int i = 0; i < src.size(); ++i) {
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
    add_all(texCoords, VVec2(positions.size() - texCoords.size(), vec2()));
  }

  // indices are copied and incremented
  add_all_incremented(indexOffset, indices, mesh.indices);
}

//void Mesh::addMesh(const Mesh & mesh, const glm::vec3 & color) {
//  if (colors.empty()) {
//    colors.resize(positions.size());
//    colors = VVec3(positions.size(), getColor());
//  }
//  addMesh(mesh);
//}

void Mesh::addQuad(float width, float height) {
  int indexOffset = positions.size();
  float x = width / 2.0f;
  float y = height / 2.0f;

  VVec4 quad;
  // C++11      { vec3(-x, -y, 0), vec3(x, -y, 0), vec3(x, y, 0), vec3(-x, y, 0),  });
  quad.push_back(vec4(-x, -y, 0, 1));
  quad.push_back(vec4(x, -y, 0, 1));
  quad.push_back(vec4(x, y, 0, 1));
  quad.push_back(vec4(-x, y, 0, 1));
  // Positions are transformed
  add_all_transformed(model.top(), positions, quad);
  if (normals.size()) {
    // normals are transformed with only the rotation, not the translation
    model.push().untranslate();
    add_all_transformed(model.top(), normals, quad);
    model.pop();
  }

  // indices are copied and incremented
  VS quadIndices;
  // C++11 VS( { 0, 1, 2, 0, 2, 3 } );
  quadIndices.push_back(0);
  quadIndices.push_back(1);
  quadIndices.push_back(2);
  quadIndices.push_back(0);
  quadIndices.push_back(2);
  quadIndices.push_back(3);
  add_all_incremented(indexOffset, indices, quadIndices);

  fillColors();
  fillNormals();
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
  vector<vec4> vertices;
  vertices.reserve(vertexCount * attributeCount);
  for (int i = 0; i < vertexCount; ++i) {
    vertices.push_back(positions[i]);
    if (flags & Geometry::Flag::HAS_NORMAL) {
      vertices.push_back(normals[i]);
    }
    if (flags & Geometry::Flag::HAS_COLOR) {
      vertices.push_back(vec4(colors[i], 1));
    }
    if (flags & Geometry::Flag::HAS_TEXTURE) {
      vertices.push_back(vec4(texCoords[i], 1, 1));
    }
  }
  return vertices;
}

GeometryPtr Mesh::getGeometry(GLenum elementType) const {
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
  case GL_POINTS:
    elements = (unsigned int)indices.size();
    verticesPerElement = 1;
    break;
  default:
	  throw runtime_error("unsupported geometry type");
  }
  return GeometryPtr(new Geometry(vertices, indices, elements, flags, elementType, verticesPerElement));
}
