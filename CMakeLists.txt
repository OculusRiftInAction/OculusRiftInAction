project(OculusRiftExamples)
cmake_minimum_required(VERSION 2.8)

# Required for statically linking the MSVC runtime
set(CMAKE_USER_MAKE_RULES_OVERRIDE
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Comment out to disable the debug build
set(CMAKE_BUILD_TYPE "Debug")

# Verbosity is good, especially if a command fails
set(CMAKE_VERBOSE_MAKEFILE ON)

if(CMAKE_COMPILER_IS_GNUCXX)
    # Ensure we use C++ 11 
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    # Ensure we generate position independent code 
    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC"  )
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC"  )    
    endif()
endif()

if(APPLE)
    # Ensure we use C++ 11 
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -stdlib=libc++ -g -Wall")
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

# OpenCTM - a 3D mesh compression
add_subdirectory(libraries/OpenCTM)
include_directories(${CMAKE_SOURCE_DIR}/libraries/OpenCTM)
list(APPEND EXAMPLE_LIBS OpenCTM)

# OpenCV is a computer vision library with advanced
# image loading and manipulation functionality
find_package( OpenCV )

if (OpenCV_FOUND) 
    include_directories(${OpenCV_INCLUDE_DIRS})
    list(APPEND EXTERNAL_LIBS ${OpenCV_LIBS})
    set(HAVE_OPENCV 1)
else()
    # Without OpenCV, we have to fall back on libpng
    # which requires zlib.  For windows and apple we
    # build the library.  For Unix systems we locate 
    # the native package
    if((WIN32 OR APPLE))
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
    else()
        find_package(PNG REQUIRED)
        include_directories(${PNG_INCLUDE_DIR})
        list(APPEND EXAMPLE_LIBS ${PNG_LIBRARIES})
    endif()
endif()

# Vector / matrix header only math library based on
# the syntax of GLSL
include_directories(${CMAKE_SOURCE_DIR}/libraries/glm)

# Boost is a general purpose C++ library
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost COMPONENTS filesystem system) 

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) 
    list(APPEND EXAMPLE_LIBS ${Boost_LIBRARIES})
    set(HAVE_BOOST 1)
endif()

set(SIXENSE_ROOT "/home/bdavis/src/sixenseSDK_linux_OSX")
#find_package(Sixense)

if(SIXENSE_FOUND)
    include_directories(${SIXENSE_INCLUDE_DIR}) 
    list(APPEND EXAMPLE_LIBS ${SIXENSE_LIBRARIES})
    set(HAVE_SIXENSE 1)
endif()

find_package(SpaceNavigator)
if(SPNAV_FOUND)
    include_directories(${SPNAV_INCLUDE_DIR}) 
    list(APPEND EXAMPLE_LIBS ${SPNAV_LIBRARIES})
    set(HAVE_SPNAV 1)
endif()

###############################################################################
#
# Common GL code, using only headers.  
#
add_subdirectory(libraries/gl)
include_directories(${CMAKE_SOURCE_DIR}/libraries/gl)

# This is used by the resources and shaders logic to 
# allow us to load non-C++ resources into the program 
function(files_to_enum SOURCE_FILES RESULT_HEADER RESULT_MAP)
    foreach (file ${SOURCE_FILES})
        file(RELATIVE_PATH res ${CMAKE_CURRENT_SOURCE_DIR} ${file})
        string(TOUPPER ${res} res)
        string(REGEX REPLACE "\\.\\./" "" res "${res}" )
        string(REGEX REPLACE "[^A-Z1-9]" "_" res "${res}" )
        set(HEADER_BUFFER "${HEADER_BUFFER}\t${res},\n")
        set(MAP_BUFFER "${MAP_BUFFER}\t\tfileMap[${res}] = \"${file}\";\n")
    endforeach()
    set(${RESULT_HEADER} "${HEADER_BUFFER}" PARENT_SCOPE)
    set(${RESULT_MAP} "${MAP_BUFFER}" PARENT_SCOPE)
endfunction()

###############################################################################
#
# Non-C++ resources that are shared by the examples
# (shaders, meshes, images, fonts, etc)
#
add_subdirectory(resources)
include_directories(${CMAKE_BINARY_DIR}/resources)
list(APPEND EXAMPLE_LIBS resources)

add_subdirectory(source/common)
include_directories(source/common)
include_directories(${CMAKE_BINARY_DIR}/source/common)
add_subdirectory(source)
