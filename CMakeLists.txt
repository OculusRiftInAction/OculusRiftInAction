# Required for statically linking the MSVC runtime
set(CMAKE_USER_MAKE_RULES_OVERRIDE
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)

project(OculusRiftExamples)
cmake_minimum_required(VERSION 2.8)
include(GenerateExportHeader)
include(cmake/defaults.cmake)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(RIFT_BUILD_SCRATCH_EXAMPLES OFF)
string( TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER )
if(BUILD_TYPE_LOWER STREQUAL "debug")
   set(RIFT_DEBUG 1)
endif()

if(RIFT_DEBUG)
   message("Debug build, resources will be loaded from disk")
   set(OVR_DEBUG_BUILD 1)
else() 
   message("Release build, resources will be loaded from executable/bundle")
endif()


if(WIN32)
elseif(APPLE)
else()
endif()


###############################################################################
#
# Oculus VR SDK dependency 
#
set(OVR_DIR ${CMAKE_SOURCE_DIR}/libraries/OculusSDK/LibOVR)
# Check for the existence of the LibOVR project.  If it's missing, the 
# submodule probably hasn't been checked out.
if (NOT(EXISTS ${OVR_DIR} AND IS_DIRECTORY ${OVR_DIR}))
    message(FATAL_ERROR "Submodules seem to be missing.  Please make sure you check out with 'git clone --recursive' OR run 'git submodule init' and 'git submodule update' after checkout") 
endif()

# Add debug output from the Oculus SDK

# The primary SDK artifact, a static library for Oculus access 
add_subdirectory(libraries/OculusSDK/LibOVR)
set_target_properties(OculusVR PROPERTIES FOLDER "3rdparty")
list(APPEND EXAMPLE_LIBS OculusVR)

###############################################################################
#
# Non-Oculus third party dependencies 
#

###############################################################################
#
# GLEW - Cross platform access to OpenGL extensions and > 1.x functions 
#

add_definitions(-DGLEW_STATIC)
add_subdirectory(libraries/glew)
set_target_properties(glew PROPERTIES FOLDER "3rdparty")
list(APPEND EXAMPLE_LIBS glew)

# Cross platform access to shader functionality

###############################################################################
#
# GLFW - Cross platform OpenGL window creation and input handling
#
# We need to set a bunch of values so we build only the library and not the 
# install targets, documentation or examples
#
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
add_subdirectory(libraries/glfw)
set_target_properties(glfw PROPERTIES FOLDER "3rdparty")
list(APPEND EXAMPLE_LIBS glfw ${GLFW_LIBRARIES})

#
# OpenCTM - a 3D mesh compression
#
add_subdirectory(libraries/OpenCTM)
set_target_properties(OpenCTM PROPERTIES FOLDER "3rdparty")
list(APPEND EXAMPLE_LIBS OpenCTM)

###############################################################################
#
# OpenCV is a Computer vision library with advanced image loading and 
# manipulation functionality, including a simple API for accessing cameras  
#
# OpenCV is optional, but required to build the chapter 14 examples.
#  
# OpenCV will be found if you set an environment variable OpenCV_DIR 
# pointing to the location of the installed OpenCVConfig.cmake file

find_package(OpenCV QUIET)

if (OpenCV_FOUND) 
    include_directories(${OpenCV_INCLUDE_DIRS})
    list(APPEND EXAMPLE_LIBS ${OpenCV_LIBS})
    set(HAVE_OPENCV 1)
    message(STATUS "Found OpenCV")
else()
    message(STATUS "No OpenCV found")
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
    else()
        find_package(PNG REQUIRED)
        list(APPEND EXAMPLE_LIBS ${PNG_LIBRARIES})
    endif()
endif()


set(LeapMotion_DIR "C:/dev/LeapDeveloperKit_2.1.5+22699_win/LeapSDK")
find_package(LeapMotion QUIET)
if (LeapMotion_FOUND)
    SET(HAVE_LEAP 1) 
    include_directories(${LeapMotion_INCLUDE_DIR})
    list(APPEND EXAMPLE_LIBS ${LeapMotion_LIBRARY})
	message(STATUS "Found Leap Motion SDK")
endif()

find_package(Threads)
list(APPEND EXAMPLE_LIBS ${CMAKE_THREAD_LIBS_INIT} )

###############################################################################
#
# Qt is a cross platform framework for just about everything.
#
# Qt can be found by setting a QT5_ROOT to the root of your 
#
# Qt is optional, but required to build the chapter 10 examples

set(QT_COMPONENTS Core Widgets Gui QuickWidgets OpenGL Xml)

# Qt emits a ton of warnings which we're not interested in
if(NOT DEFINED CMAKE_SUPPRESS_DEVELOPER_WARNINGS)
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "No dev warnings")
endif()

find_package(Qt5 REQUIRED ${QT_COMPONENTS})
if (Qt5_FOUND)
    message(STATUS "Found QT")
    set(HAVE_QT 1)
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
	foreach(qtc ${QT_COMPONENTS})
    	list(APPEND EXAMPLE_LIBS ${Qt5${qtc}_LIBRARIES})
        add_definitions(${Qt5${qtc}_DEFINITIONS})
        include_directories(${Qt5${qtc}_INCLUDE_DIRS})
	endforeach()
else()
    message(STATUS "Qt framework not found") 
endif()


######################################################
#
# OGLplus provides object oriented OpenGL wrappers
#

# oglplus uses CMake to detect compiler features, but 
# we can't use the full oglplus CMake include, so we 
# need to jump through some hoops to make it's 
# compiler detection logic work here.

set(SAVE_PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR})
set(PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/libraries/oglplus)
    include(libraries/oglplus/config/Compiler.cmake)
    include(libraries/oglplus/config/CPPFeature.cmake)
    set(OGLPLUS_CONFIG_SET_LOW_PROFILE 1)
    set(OGLPLUS_LOW_PROFILE 0)
    set(OGLPLUS_USE_GLCOREARB_H 0) 
    set(OGLPLUS_USE_GL3_H 0)
    set(OGLPLUS_USE_GLEW 1)
    set(OGLPLUS_USE_GL3W 0)
    set(OGLPLUS_USE_BOOST_CONFIG 0)
    configure_file(
        ${PROJECT_SOURCE_DIR}/config/oglplus/config/site.hpp.in
        ${PROJECT_BINARY_DIR}/libraries/oglplus/include/oglplus/config/site.hpp
    )
set(PROJECT_SOURCE_DIR ${SAVE_PROJECT_SOURCE_DIR})

add_definitions(-DOGLPLUS_LOW_PROFILE=1)
# aadd_definitions(-DOGLPLUS_USE_GLEW=1)
# aadd_definitions(-DOGLPLUS_USE_BOOST_CONFIG=1)
# aadd_definitions(-DOGLPLUS_NO_SITE_CONFIG=1)

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

include_directories(${CMAKE_SOURCE_DIR}/libraries/boost)

# GLM - Vector / matrix header only math library based on the syntax of GLSL
include_directories(${CMAKE_SOURCE_DIR}/libraries/glm)

include_directories(${CMAKE_SOURCE_DIR}/libraries/glew/include)
include_directories(${CMAKE_SOURCE_DIR}/libraries/glfw/include)

include_directories(${CMAKE_SOURCE_DIR}/libraries/oglplus/include)
include_directories(${CMAKE_SOURCE_DIR}/libraries/oglplus/implement)
include_directories(${CMAKE_BINARY_DIR}/libraries/oglplus/include)

include_directories(${CMAKE_SOURCE_DIR}/libraries/OpenCTM)
include_directories(${CMAKE_SOURCE_DIR}/libraries/OculusSDK/LibOVR/Src)

if (OpenCV_FOUND) 
    include_directories(${OpenCV_INCLUDE_DIRS})
else()
    if((WIN32 OR APPLE))
        include_directories(${CMAKE_SOURCE_DIR}/libraries/libpng)
        include_directories(${CMAKE_BINARY_DIR}/libraries/libpng)
    else()
        include_directories(${PNG_INCLUDE_DIR})
    endif()
endif()

if (Qt5_FOUND)
	foreach(qtc ${QT_COMPONENTS})
        include_directories(${Qt5${qtc}_INCLUDE_DIRS})
	endforeach()
endif()

###############################################################################
#
# The examples themselves
#
add_subdirectory(examples/cpp)

if (LeapMotion_FOUND)
    add_custom_command(TARGET ExampleResources POST_BUILD
        COMMAND ${CMAKE_PROGRAM} -E copy ${LeapMotion_BINARY} ${CMAKE_BINARY_DIR}/output/Leap.dll
    )
    message(${CMAKE_PROGRAM} -E copy ${LeapMotion_BINARY} ${CMAKE_BINARY_DIR}/output/Leap.dll)
endif()

