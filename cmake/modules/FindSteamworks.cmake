# This script locates the STEAMWORKS SDK
# ------------------------------------
#
# usage:
# find_package(STEAMWORKS ...)
#
# searches in STEAMWORKS_ROOT and usual locations
#
# Sets STEAMWORKS_INCLUDE_DIR, STEAMWORKS_LIBRARY_STATIC and STEAMWORKS_LIBRARY_DYNAMIC

set(STEAMWORKS_POSSIBLE_PATHS
    ${STEAMWORKS_SDK}
    $ENV{STEAMWORKS_SDK}
)

find_path(STEAMWORKS_INCLUDE_DIR 
    NAMES steam_api.h
    PATH_SUFFIXES "public/steam"
    PATHS ${STEAMWORKS_POSSIBLE_PATHS}
)

find_library(STEAMWORKS_LIBRARY 
    NAMES steam_api
    PATH_SUFFIXES "redistributable_bin"
    PATHS ${STEAMWORKS_POSSIBLE_PATHS}
)


find_package_handle_standard_args(Steamworks  DEFAULT_MSG STEAMWORKS_LIBRARY STEAMWORKS_INCLUDE_DIR)
