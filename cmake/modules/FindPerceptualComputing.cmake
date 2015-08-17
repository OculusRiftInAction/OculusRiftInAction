# This script locates the PERC SDK
# ------------------------------------
#
# usage:
# find_package(PERC ...)
#
# searches in PERC_ROOT and usual locations
#
# Sets PERC_INCLUDE_DIR, PERC_LIBRARY_STATIC and PERC_LIBRARY_DYNAMIC

set(PERC_POSSIBLE_PATHS
    ${PERC_SDK}
    $ENV{PERC_SDK}
    "$ENV{ProgramFiles(x86)}/Intel/PCSDK"
)


find_path(PERC_INCLUDE_DIR 
    NAMES pxcbase.h
    PATH_SUFFIXES "include"
    PATHS ${PERC_POSSIBLE_PATHS}
)

find_library(PERC_LIBRARY 
    NAMES libpxc
    PATH_SUFFIXES "lib/Win32"
    PATHS ${PERC_POSSIBLE_PATHS}
)

find_package_handle_standard_args(PERC  DEFAULT_MSG PERC_LIBRARY PERC_INCLUDE_DIR)
