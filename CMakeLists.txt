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

add_definitions(-DGLEW_STATIC)
add_subdirectory(libraries/glew)
set_target_properties(glew PROPERTIES FOLDER "3rdparty")
list(APPEND EXAMPLE_LIBS glew)

# Cross platform access to shader functionality

# GLFW - Cross platform OpenGL window creation and input handling
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
add_subdirectory(libraries/glfw)
set_target_properties(glfw PROPERTIES FOLDER "3rdparty")
list(APPEND EXAMPLE_LIBS glfw ${GLFW_LIBRARIES})

# OpenCTM - a 3D mesh compression
add_subdirectory(libraries/OpenCTM)
set_target_properties(OpenCTM PROPERTIES FOLDER "3rdparty")
list(APPEND EXAMPLE_LIBS OpenCTM)

# Computer vision library with advanced image loading and manipulation 
# functionality.  
# OpenCV is OPTIONAL.  
set(OpenCV_DIR "/dev/opencv-2.4.9/build")
find_package(OpenCV QUIET)
if (OpenCV_FOUND) 
    include_directories(${OpenCV_INCLUDE_DIRS})
    list(APPEND EXAMPLE_LIBS ${OpenCV_LIBS})
    set(HAVE_OPENCV 1)
else()
    # Without OpenCV, we have to fall back on libpng
    # which requires zlib.  For windows and apple we
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

find_package(Threads)
list(APPEND EXAMPLE_LIBS ${CMAKE_THREAD_LIBS_INIT} )

######################################################
#
# Qt is a cross platform GUI framework
#
set(Qt5_DIR $ENV{QT5_ROOT}/lib/cmake/Qt5)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

#find_package(Qt5 REQUIRED Core Gui OpenGL QuickWidgets Widgets)

set(HAVE_QT 1)
foreach(lib Qt5Core Qt5Widgets Qt5Gui Qt5QuickWidgets Qt5OpenGL)
    find_package(${lib} REQUIRED)
    include_directories(${${lib}_INCLUDE_DIRS})
    list(APPEND EXAMPLE_LIBS ${${lib}_LIBRARIES})
    add_definitions(${${lib}_DEFINITIONS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${${lib}_EXECUTABLE_COMPILE_FLAGS}")
endforeach(lib)



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

#add_definitions(-DOGLPLUS_USE_BOOST_CONFIG)
#add_definitions(-DOGLPLUS_NO_SITE_CONFIG)


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
set_target_properties(ExampleResourcesQt PROPERTIES FOLDER "Examples/Shared")
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
    include_directories(${Qt5Core_INCLUDE_DIRS})
    include_directories(${Qt5Gui_INCLUDE_DIRS})
    include_directories(${Qt5OpenGL_INCLUDE_DIRS})
    include_directories(${Qt5QuickWidgets_INCLUDE_DIRS})
    include_directories(${Qt5Widgets_INCLUDE_DIRS})
endif()

###############################################################################
#
# The examples themselves
#
add_subdirectory(examples/cpp)

