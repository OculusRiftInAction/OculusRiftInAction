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
set_target_properties(ovr PROPERTIES FOLDER "3rdparty")
list(APPEND EXAMPLE_LIBS ovr)

###############################################################################
#
#
# Non-Oculus third party dependencies 
#
#
###############################################################################

###############################################################################
# GLEW - Cross platform access to OpenGL extensions and > 1.x functions 

add_definitions(-DGLEW_STATIC)
add_subdirectory(libraries/glew)
set_target_properties(glew PROPERTIES FOLDER "3rdparty")
list(APPEND EXAMPLE_LIBS glew)


###############################################################################
# GLFW - Cross platform OpenGL window creation and input handling
#
# We need to set a bunch of values so we build only the library and not the 
# install targets, documentation or examples

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
add_subdirectory(libraries/glfw)
set_target_properties(glfw PROPERTIES FOLDER "3rdparty")
list(APPEND EXAMPLE_LIBS glfw ${GLFW_LIBRARIES})

###############################################################################
# OpenCTM - a 3D mesh (de)compression library

add_subdirectory(libraries/OpenCTM)
set_target_properties(OpenCTM PROPERTIES FOLDER "3rdparty")
list(APPEND EXAMPLE_LIBS OpenCTM)

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
    else()
        find_package(PNG REQUIRED)
        list(APPEND EXAMPLE_LIBS ${PNG_LIBRARIES})
    endif()
endif()

###############################################################################
# Leap Motion SDK - Required for building the chapter 14 examples that use 
# the leap motion controller

find_package(LeapMotion QUIET)
if (LeapMotion_FOUND)
    SET(HAVE_LEAP 1) 
    include_directories(${LeapMotion_INCLUDE_DIR})
    list(APPEND EXAMPLE_LIBS ${LeapMotion_LIBRARY})
	message(STATUS "Found Leap Motion SDK")
endif()

###############################################################################
# Intel RealSense SDK - Not currently in use

#find_package(RealSense QUIET)
#if (RealSense_FOUND) 
#    list(APPEND EXAMPLE_LIBS ${RealSense_LIBRARY})
#endif()

###############################################################################
# 3DxWare - Not currently in use

#find_package(3DxWare QUIET)
#if (3DxWare_FOUND) 
#    message(STATUS "3DxWare found")
#    list(APPEND EXAMPLE_LIBS ${3DxWare_LIBRARY})
#endif()

###############################################################################
# Qt - a cross platform framework for windowing and just about everything else
#
# Qt is optional, but required to build the chapter 10 examples
#
# Qt can be found by setting a QT5_ROOT to the root of your 

option(USE_QT "Attempt to find Qt and build the Qt based examples" OFF)
       
if (USE_QT)
	
	set(QT_COMPONENTS Core Gui QuickWidgets OpenGL Xml XmlPatterns Declarative)
	
	# Qt emits a ton of warnings which we're not interested in
	if(NOT DEFINED CMAKE_SUPPRESS_DEVELOPER_WARNINGS)
	    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "No dev warnings")
	endif()
	
	find_package(Qt5 QUIET COMPONENTS ${QT_COMPONENTS})
	
	if (Qt5Core_FOUND)
	    message(STATUS "Qt Found")
	    set(HAVE_QT 1)
	    set(CMAKE_INCLUDE_CURRENT_DIR ON)
	    set(CMAKE_AUTOMOC ON)
	    set(CMAKE_AUTORCC ON)
	    set(CMAKE_AUTOMOC_MOC_OPTIONS "-bQtCommon.h" "-bCommon.h" )
	   
		foreach(qtc ${QT_COMPONENTS})
	    	list(APPEND EXAMPLE_LIBS ${Qt5${qtc}_LIBRARIES})
	        add_definitions(${Qt5${qtc}_DEFINITIONS})
	        include_directories(${Qt5${qtc}_INCLUDE_DIRS})
		endforeach()
	else()
	    message(WARNING "Qt framework not found") 
	endif()

endif()
######################################################
# OGLplus provides object oriented OpenGL wrappers
#
# It's a header only library, so all we need to do is set some defines for how 
# we want it to act

add_definitions(-DOGLPLUS_LOW_PROFILE=1)
add_definitions(-DOGLPLUS_USE_GLEW=1)
add_definitions(-DOGLPLUS_USE_BOOST_CONFIG=1)
add_definitions(-DOGLPLUS_NO_SITE_CONFIG=1)


find_package(Threads)
list(APPEND EXAMPLE_LIBS ${CMAKE_THREAD_LIBS_INIT} )

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

include_directories(${CMAKE_SOURCE_DIR}/libraries/boostconfig/include)

# GLM - Vector / matrix header only math library based on the syntax of GLSL
include_directories(${CMAKE_SOURCE_DIR}/libraries/glm)

include_directories(${CMAKE_SOURCE_DIR}/libraries/glew/include)
include_directories(${CMAKE_SOURCE_DIR}/libraries/glfw/include)

include_directories(${CMAKE_SOURCE_DIR}/libraries/oglplus/include)
include_directories(${CMAKE_SOURCE_DIR}/libraries/oglplus/implement)
include_directories(${CMAKE_BINARY_DIR}/libraries/oglplus/include)

include_directories(${CMAKE_SOURCE_DIR}/libraries/OpenCTM)
include_directories(${CMAKE_SOURCE_DIR}/libraries/OculusSDK/LibOVR/Include)

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

if (RealSense_FOUND) 
    include_directories(${RealSense_INCLUDE_DIR})
endif()

if (3DxWare_FOUND) 
    include_directories(${3DxWare_INCLUDE_DIR})
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

