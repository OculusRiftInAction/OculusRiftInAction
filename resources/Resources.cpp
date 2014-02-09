#include "Resources.h"

#ifdef WIN32
#include <Windows.h>
#endif

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <fstream>
#include <iostream>
#include <cassert>
#include <memory.h>
#include <sstream>
#include <functional>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <sys/types.h>
#include <sys/stat.h>

#undef HAVE_BOOST
#ifdef HAVE_BOOST
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;
using namespace boost::filesystem;
#endif

#if defined(RIFT_DEBUG)
#define FILE_LOADING 1
#endif


const std::string & Resources::getResourcePath(Resource resource) {
  typedef std::unordered_map<Resource, std::string> Map;
  static bool init = false;
  static Map resources;

  if (!init) {
    init = true;
    int i = 0;
#ifdef __APPLE__
    static CFBundleRef mainBundle = CFBundleGetMainBundle();
    assert(mainBundle);
#endif

    while (RESOURCE_MAP_VALUES[i].first != NO_RESOURCE) {
      Resource res = RESOURCE_MAP_VALUES[i].first;
      std::string path = RESOURCE_MAP_VALUES[i].second;
#if FILE_LOADING
      path = RESOURCE_ROOT + "/" + path;
#elif defined(WIN32)
      // Win32 resource identifiers can't have spaces
      for (int j = 0; j < path.size(); ++j) {
        if (path.at(j) == ' ') {
          path[j] = '_';
        }
      }
#elif defined(__APPLE__)
      static char PATH_BUFFER[PATH_MAX];
      CFStringRef stringRef = CFStringCreateWithCString(NULL, path.c_str(), kCFStringEncodingASCII);
      assert(stringRef);
      CFURLRef resourceURL = CFBundleCopyResourceURL(mainBundle, stringRef, NULL, NULL);
      assert(resourceURL);
      auto result = CFURLGetFileSystemRepresentation(resourceURL, true, (UInt8*)PATH_BUFFER, PATH_MAX);
      assert(result);
      path = std::string(PATH_BUFFER);
#endif
      resources[res] = path;
      ++i;
    }
  }
  return resources[resource];
}

// Everything except for windows resources ultimately boils down to a file path
// and file loading
#if defined(WIN32) && !defined(FILE_LOADING)

HMODULE module;
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
  switch (fdwReason)
  {
  case DLL_PROCESS_ATTACH:
    module = hinstDLL;
    break;
  }
  return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

size_t Resources::getResourceSize(Resource resource) {
  const std::string & path = getResourcePath(resource);
  HRSRC res = FindResourceA(module, path.c_str(), "TEXTFILE");
  assert(res);
  DWORD size = SizeofResource(module, res);
  return size;
}

void Resources::getResourceData(Resource resource, void * out) {
  const std::string & path = getResourcePath(resource);
  HRSRC res = FindResourceA(module, path.c_str(), "TEXTFILE");
  assert(res);
  HGLOBAL mem = LoadResource(module, res);
  assert(mem);
  DWORD size = SizeofResource(module, res);
  LPVOID data = LockResource(mem);
  assert(data);
  memcpy(out, data, size);
  FreeResource(mem);
}

time_t Resources::getResourceModified(Resource resource) {
  return 0;
}

#else

std::string slurpStream(std::istream& in) {
  std::stringstream sstr;
  sstr << in.rdbuf();
  return sstr.str();
}

std::string readFile(const std::string & filename) {
  std::ifstream ins(filename, std::ios::binary);
  assert(ins);
  if (!ins) {
    throw std::runtime_error("Failed to load file " + filename);
  }
  return slurpStream(ins);
}

size_t Resources::getResourceSize(Resource resource) {
  const std::string & filename = getResourcePath(resource);
  struct stat st;
  if (0 == stat(filename.c_str(), &st))
    return st.st_size;
  return -1;
}

void Resources::getResourceData(Resource resource, void * out) {
  const std::string & path = getResourcePath(resource);
  std::string data = readFile(path);
  memcpy(out, data.data(), data.length());
}

time_t Resources::getResourceModified(Resource resource) {
  const std::string & filename = getResourcePath(resource);
  struct stat st;
  if (0 == stat(filename.c_str(), &st))
    return st.st_mtime;
  return 0;
}

#endif



