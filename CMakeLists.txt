project(OculusRiftExamples)

cmake_minimum_required(VERSION 2.8)

# Comment out to disable the debug build
set(CMAKE_BUILD_TYPE "Debug")

# Verbosity is good, especially if a command fails
set(CMAKE_VERBOSE_MAKEFILE ON)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
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
# Oculus VR SDK dependency 
#

# Add debug output from the Oculus SDK
add_definitions(-DOVR_BUILD_DEBUG)

# The Linux version of the community SDK requires an addition EDID handling library
if(UNIX AND NOT(APPLE) AND NOT(CYGWIN))
    add_subdirectory(libraries/OculusSDK/3rdParty/EDID)
endif()

# The primary SDK artifact, a static library for Oculus access 
add_subdirectory(libraries/OculusSDK/LibOVR)
# Make sure to set up the OVR include paths
include_directories(libraries/OculusSDK/LibOVR/Include)
include_directories(libraries/OculusSDK/LibOVR/Src)
include_directories(libraries/OculusSDK/LibOVR/Src/Kernel)
include_directories(libraries/OculusSDK/LibOVR/Src/Util)

set(EXAMPLE_LIBS OculusVR )

###############################################################################
#
# Non-Oculus third party dependencies 
#

# Cross platform access to shader functionality
add_definitions(-DGLEW_STATIC)
add_subdirectory(libraries/glew)
include_directories(${CMAKE_SOURCE_DIR}/libraries/glew/include)
list(APPEND EXAMPLE_LIBS glew)

# GLFW - Cross platform OpenGL window creation and input handling
add_subdirectory(libraries/glfw)
include_directories(${CMAKE_SOURCE_DIR}/libraries/glfw/include)
list(APPEND EXAMPLE_LIBS glfw ${GLFW_LIBRARIES})

add_subdirectory(libraries/OpenCTM)
include_directories(${CMAKE_SOURCE_DIR}/libraries/OpenCTM)
list(APPEND EXAMPLE_LIBS OpenCTM)

add_subdirectory(libraries/zlib)
set(ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libraries/zlib)
include_directories(${CMAKE_BINARY_DIR}/libraries/zlib)
include_directories(${CMAKE_SOURCE_DIR}/libraries/zlib)
list(APPEND EXAMPLE_LIBS zlib)

SET(PNG_STATIC ON CACHE BOOL "Build PNG static library")
SET(PNG_TESTS OFF CACHE BOOL "Build PNG tests")
SET(PNG_SHARED OFF CACHE BOOL "Build PNG shared library")
add_subdirectory(libraries/libpng)
include_directories(${CMAKE_SOURCE_DIR}/libraries/libpng)
include_directories(${CMAKE_BINARY_DIR}/libraries/libpng)
list(APPEND EXAMPLE_LIBS png)

# OpenGL vector / matrix header only math library
include_directories(${CMAKE_SOURCE_DIR}/libraries/glm)

# Boost
#set(Boost_USE_STATIC_LIBS OFF) 
#set(Boost_USE_MULTITHREADED ON)  
#set(Boost_USE_STATIC_RUNTIME OFF) 
#find_package(Boost 1.45.0 COMPONENTS filesystem system) 
#if(Boost_FOUND)
#    include_directories(${Boost_INCLUDE_DIRS}) 
#    list(APPEND EXAMPLE_LIBS ${Boost_LIBRARIES})
#endif()

# OpenCV
#find_package( OpenCV )
#
#if (OpenCV_FOUND) 
#    include_directories(${OpenCV_INCLUDE_DIRS})
#    list(APPEND EXTERNAL_LIBS ${OpenCV_LIBS})
#endif()

###############################################################################
#
# Non-C++ resources that are shared by the examples
# (shaders, meshes, images, fonts, etc)
#
include(resources/Resources.cmake)

###############################################################################
#
# Common GL code, using only headers.  
#
add_subdirectory(source/ExampleGL)
include_directories(${CMAKE_SOURCE_DIR}/source/ExampleGL)

###############################################################################
#
# This creates a project for the shaders for easy access an editing. 
#
add_subdirectory(resources)

###############################################################################
#
# More common code 
#
add_subdirectory(source/ExampleCommon)
include_directories(${CMAKE_SOURCE_DIR}/source/ExampleCommon)
include_directories(${CMAKE_BINARY_DIR}/source/ExampleCommon)

###############################################################################
#
# A macro for reducing the boilerplate for adding an individual example
#
function(add_example SOURCE_FILES)
    list(APPEND SOURCE_FILES ${EXAMPLE_ALL_RESOURCES})
    source_group("Resources/Shaders" FILES ${EXAMPLE_SHADERS})

    if(WIN32)
        add_executable(${PROJECT_NAME} WIN32 ${SOURCE_FILES} )
    elseif(APPLE)
        add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCE_FILES} )
    else()
        add_executable(${PROJECT_NAME} ${SOURCE_FILES} )
    endif()

    target_link_libraries(${PROJECT_NAME}  ExampleCommon ${EXAMPLE_LIBS} ${EXTERNAL_LIBS})

endfunction()

add_subdirectory(source/Example00)
#add_subdirectory(source/Example02_SDK)
#add_subdirectory(source/Example03_FindUp)
#add_subdirectory(source/Example04_Rendering)
add_subdirectory(source/Example05_Tracker)
#add_subdirectory(source/Example07_Horizon)
#add_subdirectory(source/ExampleWork)
#add_subdirectory(source/Example08_HorizonBearing)

