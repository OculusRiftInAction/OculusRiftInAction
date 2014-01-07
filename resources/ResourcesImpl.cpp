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
#include <stdexcept>

#ifdef HAVE_BOOST
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;
using namespace boost::filesystem;
#endif

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

// Linux implmentation
#if RIFT_DEBUG || (!defined(WIN32) && !defined(__APPLE__))
#define FILE_LOADING 1
#endif


#if defined(WIN32)

HMODULE module;

BOOL WINAPI DllMain(
  HINSTANCE hinstDLL,  // handle to DLL module
  DWORD fdwReason,     // reason for calling function
  LPVOID lpReserved)  // reserved
{
  switch (fdwReason)
  {
  case DLL_PROCESS_ATTACH:
    module = hinstDLL;
    break;
  }
  return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

std::string loadResource(const std::string& in) {
  HRSRC res = FindResourceA(module, in.c_str(), "TEXTFILE");
  assert(res);
  HGLOBAL mem = LoadResource(module, res);
  assert(mem);
  DWORD size = SizeofResource(module, res);
  LPVOID data = LockResource(mem);
  assert(data);
  std::string result((const char*)data, size);
  FreeResource(mem);
  return result;
}

#elif defined(__APPLE__)
std::string loadResource(const std::string& in) {
  static CFBundleRef mainBundle = CFBundleGetMainBundle();
  assert(mainBundle);
  static char PATH_BUFFER[PATH_MAX];
  CFStringRef stringRef = CFStringCreateWithCString(NULL, in.c_str(), kCFStringEncodingASCII);
  assert(stringRef);
  CFURLRef resourceURL = CFBundleCopyResourceURL(mainBundle, stringRef, NULL, NULL);
  assert(resourceURL);
  auto result = CFURLGetFileSystemRepresentation(resourceURL, true, (UInt8*)PATH_BUFFER, PATH_MAX);
  assert(result);
  return readFile(std::string(PATH_BUFFER));
}
#endif // WIN32

size_t Resources::getResourceSize(Resource resource) {
  std::string data, path = getResourcePath(resource);
#ifdef FILE_LOADING
  data = readFile(path);
#else
  data = loadResource(path);
#endif
  return data.length();
}


void Resources::getResourceData(Resource resource, void * out) {
  std::string data, path = getResourcePath(resource);
#ifdef FILE_LOADING
  data = readFile(path);
#else
  data = loadResource(path);
#endif
  memcpy(out, data.data(), data.length());
}

time_t Resources::getResourceModified(Resource resource) {
#if FILE_LOADING && HAVE_BOOST
  return last_write_time(getResourcePath(resource));
#else
  return 0;
#endif
}

