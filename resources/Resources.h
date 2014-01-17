#pragma once
#include <string>
#include <unordered_map>
#include "Resources_Export.h"

#include "ResourceEnums.h"

class Resources {
public:
  typedef std::unordered_map<Resource, std::string> Map;
public:
  static Resources_EXPORT const Resource VERTEX_SHADERS[];
  static Resources_EXPORT const Resource FRAGMENT_SHADERS[];
  static Resources_EXPORT const Resource LIB_SHADERS[];
  static Resources_EXPORT const std::string & getResourcePath(Resource resource);
  static Resources_EXPORT time_t getResourceModified(Resource resource);
  static Resources_EXPORT size_t getResourceSize(Resource resource);
  static Resources_EXPORT void getResourceData(Resource resource, void * out);
private:
  static const std::string RESOURCE_ROOT;
  static const Map::value_type RESOURCE_MAP_VALUES[];
};
