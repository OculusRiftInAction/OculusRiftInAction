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

set(STEAMWORKS_LIB_PATH "public/steam/lib")
set(STEAMWORKS_LIB_NAME "sdkencryptedappticket")

if(APPLE)
    set(STEAMWORKS_LIB_PATH "${STEAMWORKS_LIB_PATH}/osx32")
elseif(WIN32)
    if (${CMAKE_GENERATOR} MATCHES ".*WIN64.*")
        message(FATAL_ERROR "Found 64 bit" )
    endif()
    set(STEAMWORKS_LIB_PATH "${STEAMWORKS_LIB_PATH}/win32")
else()
    set(STEAMWORKS_LIB_PATH "${STEAMWORKS_LIB_PATH}/linux32")
endif()

find_library(STEAMWORKS_LIBRARY 
    NAMES sdkencryptedappticket
    PATH_SUFFIXES ${STEAMWORKS_LIB_PATH}
    PATHS ${STEAMWORKS_POSSIBLE_PATHS}
)


find_package_handle_standard_args(Steamworks  DEFAULT_MSG STEAMWORKS_LIBRARY STEAMWORKS_INCLUDE_DIR)
