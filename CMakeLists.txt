project(OculusRiftExamples)

cmake_minimum_required(VERSION 2.8)

# Comment out these two lines to disable the debug build
set(CMAKE_BUILD_TYPE "Debug")

# Verbosity is good, especially if a command fails
set(CMAKE_VERBOSE_MAKEFILE ON)

# Add debug output from the Oculus SDK
add_definitions(-DOVR_BUILD_DEBUG)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-std=gnu++0x)
    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC"  )
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC"  )    
    endif()
endif()

if(APPLE)
	set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -stdlib=libc++ -g -Wall")
endif()

if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

###############################################################################
#
# Sample code support
#

# The Linux version of the community SDK requires an addition EDID handling library
if(UNIX AND NOT(APPLE) AND NOT(CYGWIN))
    add_subdirectory(libraries/OculusSDK/3rdParty/EDID)
endif()

# The primary SDK artifact, a static library for Oculus access 
add_subdirectory(libraries/OculusSDK/LibOVR)
# Make sure to set up the OVR include paths
include_directories(${CORE_OculusVR_SOURCE_DIR}/Include)
include_directories(${CORE_OculusVR_SOURCE_DIR}/Src)
include_directories(${CORE_OculusVR_SOURCE_DIR}/Src/Kernel)
include_directories(${CORE_OculusVR_SOURCE_DIR}/Src/Util)

# Cross platform window creation
add_subdirectory(libraries/glfw)
include_directories(${CMAKE_SOURCE_DIR}/libraries/glfw/include)

# Cross platform access to shader functionality
add_subdirectory(libraries/gl3w)
include_directories(${CMAKE_SOURCE_DIR}/libraries/gl3w)

# OpenGL vector / matrix header only math library
include_directories(${CMAKE_SOURCE_DIR}/libraries/glm)

add_subdirectory(source/Example00)

