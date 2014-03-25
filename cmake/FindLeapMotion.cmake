# This script locates the PERC SDK
# ------------------------------------
#
# usage:
# find_package(PERC ...)
#
# searches in LEAP_ROOT and usual locations
#
# Sets LEAP_INCLUDE_DIR, LEAP_LIBRARY_STATIC and LEAP_LIBRARY_DYNAMIC

set(LEAP_POSSIBLE_PATHS
    ${LEAP_SDK}
    $ENV{LEAP_SDK}
)

find_path(LEAP_INCLUDE_DIR 
    NAMES Leap.h
    PATH_SUFFIXES "include"
    PATHS ${LEAP_POSSIBLE_PATHS}
)

find_library(LEAP_LIBRARY
    NAMES Leap Leapd
    PATH_SUFFIXES "lib/x86"
    PATHS ${LEAP_POSSIBLE_PATHS}
)

set(CMAKE_FIND_LIBRARY_SUFFIXES_SAVE ${CMAKE_FIND_LIBRARY_SUFFIXES})
if (WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll")
elseif(APPLE)
else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".so")
endif()
find_library(LEAP_BINARY
    NAMES Leap Leapd
    PATH_SUFFIXES "lib/x86"
    PATHS ${LEAP_POSSIBLE_PATHS}
)
set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_SAVE})
find_package_handle_standard_args(LEAP DEFAULT_MSG LEAP_LIBRARY LEAP_INCLUDE_DIR LEAP_BINARY)
