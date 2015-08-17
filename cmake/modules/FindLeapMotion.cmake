# This script locates the PERC SDK
# ------------------------------------
#
# usage:
# find_package(PERC ...)
#
# searches in LeapMotion_ROOT and usual locations
#
# Sets LeapMotion_INCLUDE_DIR, LeapMotion_LIBRARY_STATIC and LeapMotion_LIBRARY_DYNAMIC

set(LeapMotion_POSSIBLE_PATHS
    ${LeapMotion_DIR}
    $ENV{LeapMotion_DIR}
)

find_path(LeapMotion_INCLUDE_DIR 
    NAMES Leap.h
    PATH_SUFFIXES "include"
    PATHS ${LeapMotion_POSSIBLE_PATHS}
)

find_library(LeapMotion_LIBRARY
    NAMES Leap Leapd
    PATH_SUFFIXES "lib/x86"
    PATHS ${LeapMotion_POSSIBLE_PATHS}
)

set(CMAKE_FIND_LIBRARY_SUFFIXES_SAVE ${CMAKE_FIND_LIBRARY_SUFFIXES})
if (WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll")
elseif(APPLE)
else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".so")
endif()
find_library(LeapMotion_BINARY
    NAMES Leap Leapd
    PATH_SUFFIXES "lib/x86"
    PATHS ${LeapMotion_POSSIBLE_PATHS}
)
set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_SAVE})

find_package_handle_standard_args(LeapMotion 
    FOUND_VAR LeapMotion_FOUND
    REQUIRED_VARS LeapMotion_LIBRARY LeapMotion_INCLUDE_DIR LeapMotion_BINARY)
