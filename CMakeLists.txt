# Required for statically linking the MSVC runtime
set(CMAKE_USER_MAKE_RULES_OVERRIDE
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)

project(OculusRiftExamples)

cmake_minimum_required(VERSION 2.8)
include(GenerateExportHeader)
include(FindPackageHandleStandardArgs)
include(cmake/defaults.cmake)

###############################################################################
# OpenCTM - a 3D mesh (de)compression library

add_subdirectory(libraries/OpenCTM)
set_target_properties(OpenCTM PROPERTIES FOLDER "3rdparty")
list(APPEND EXAMPLE_LIBS OpenCTM)
include_directories(${CMAKE_SOURCE_DIR}/libraries/OpenCTM)

###############################################################################
# OpenCV - a Computer vision library with advanced image loading and 
# manipulation functionality, including a simple API for accessing cameras  
#
# OpenCV is optional, but required to build the chapter 14 examples.
#  
# OpenCV will be found if you set an environment variable OpenCV_DIR 
# pointing to the location of the installed OpenCVConfig.cmake file

find_package(OpenCV QUIET)

if (OpenCV_FOUND) 
    message(STATUS "OpenCV Found")
    include_directories(${OpenCV_INCLUDE_DIRS})
    list(APPEND EXAMPLE_LIBS ${OpenCV_LIBS})
    set(HAVE_OPENCV 1)
    add_paths_to_fixup_libs("${OpenCV_LIB_PATH}/../bin")
    include_directories(${OpenCV_INCLUDE_DIRS})
else()
    message(STATUS "OpenCV NOT found")
    
    # Without OpenCV, we have to fall back on libpng
    # which requires zlib.  For windows and OSX we
    # build the library.  For Unix systems we locate 
    # the native package
    
    if((WIN32 OR APPLE))
        # zlib
        add_subdirectory(libraries/zlib)
        set(ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libraries/zlib)
        include_directories(${CMAKE_BINARY_DIR}/libraries/zlib)
        include_directories(${CMAKE_SOURCE_DIR}/libraries/zlib)
        list(APPEND EXAMPLE_LIBS zlib)
    
        # png
        set(PNG_STATIC ON CACHE BOOL "Build PNG static library")
        set(PNG_TESTS OFF CACHE BOOL "Build PNG tests")
        set(PNG_SHARED OFF CACHE BOOL "Build PNG shared library")
        add_subdirectory(libraries/libpng)
        list(APPEND EXAMPLE_LIBS png)
        set_target_properties(zlib PROPERTIES FOLDER "3rdparty")
        set_target_properties(png PROPERTIES FOLDER "3rdparty")
        include_directories(${CMAKE_SOURCE_DIR}/libraries/libpng)
        include_directories(${CMAKE_BINARY_DIR}/libraries/libpng)
    else()
        find_package(PNG REQUIRED)
        list(APPEND EXAMPLE_LIBS ${PNG_LIBRARIES})
        include_directories(${PNG_INCLUDE_DIR})
    endif()
endif()

###############################################################################
# Leap Motion SDK - Required for building the chapter 14 examples that use 
# the leap motion controller

find_package(LeapMotion QUIET)
if (LeapMotion_FOUND)
    message(STATUS "Found Leap Motion SDK")
    SET(HAVE_LEAP 1) 
    include_directories(${LeapMotion_INCLUDE_DIR})
    list(APPEND EXAMPLE_LIBS ${LeapMotion_LIBRARY})
    add_paths_to_fixup_libs("${LeapMotion_BINARY}")
endif()


find_package(Threads)
list(APPEND EXAMPLE_LIBS ${CMAKE_THREAD_LIBS_INIT})

###############################################################################
#
# Non-C++ resources that are shared by the examples
# (shaders, meshes, images, fonts, etc)
#
# On Windows these will be pulled from a common resource DLL.  
#
# On OSX, they will be embedded in each application bundle, owing to CMakes
# lack of support for shared frameworks
# 
# On Linux or if you enable the debug build, they will be pulled from the
# source location at runtime.  
#
add_subdirectory(resources)

set_target_properties(ExampleResources PROPERTIES FOLDER "Examples/Shared")
include_directories(resources/cpp)
include_directories(${CMAKE_BINARY_DIR}/resources)
list(APPEND EXAMPLE_LIBS ExampleResources)

###############################################################################
#
# All our includes for the used libraries
#

include_directories(${BOOSTCONFIG_INCLUDE_DIRS})
include_directories(${GLM_INCLUDE_DIRS})
include_directories(${GLEW_INCLUDE_DIRS})
include_directories(${GLFW_INCLUDE_DIRS})
include_directories(${OGLPLUS_INCLUDE_DIRS})
include_directories(${LIBOVR_INCLUDE_DIRS})

###############################################################################
#
# The examples themselves
#
add_subdirectory(examples/cpp)

