# This script locates the RealSense SDK
# ------------------------------------
#
# usage:
# find_package(RealSense ...)
#
# searches in REALSENSE_ROOT and usual locations
#
# Sets REALSENSE_INCLUDE_DIR, REALSENSE_LIBRARY_STATIC and REALSENSE_LIBRARY_DYNAMIC

set(REALSENSE_POSSIBLE_PATHS
        ${REALSENSE_ROOT}
        $ENV{REALSENSE_ROOT}
        "C:/Program Files (x86)/Intel/RSSDK"
)

if (WIN32)
    if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
        set(REALSENSE_LIB_DIR "lib/Win32")
        set(REALSENSE_BIN_DIR "bin/Win32")
    else()
        set(REALSENSE_LIB_DIR "lib/x64")
        set(REALSENSE_BIN_DIR "bin/x64")
    endif() 
elseif(APPLE)
elseif(UNIX)
endif()

find_path(RealSense_INCLUDE_DIR 
        NAMES           pxcbase.h
        PATH_SUFFIXES   include
        PATHS           ${REALSENSE_POSSIBLE_PATHS}
)

find_library(RealSense_LIBRARY 
        NAMES           pxc libpxc libpxc.lib
        PATH_SUFFIXES   ${REALSENSE_LIB_DIR}
        PATHS           ${REALSENSE_POSSIBLE_PATHS}
)

find_package_handle_standard_args(RealSense 
    FOUND_VAR RealSense_FOUND
    REQUIRED_VARS RealSense_LIBRARY RealSense_INCLUDE_DIR)
