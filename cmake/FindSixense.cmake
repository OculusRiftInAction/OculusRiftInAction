# This script locates the Sixense SDK
# ------------------------------------
#
# usage:
# find_package(Sixense ...)
#
# searches in SIXENSE_ROOT and usual locations
#
# Sets SIXENSE_INCLUDE_DIR, SIXENSE_LIBRARY_STATIC and SIXENSE_LIBRARY_DYNAMIC

set(SIXENSE_POSSIBLE_PATHS
        ${SIXENSE_ROOT}
        $ENV{SIXENSE_ROOT}
        "C:/Program Files/Steam/steamapps/common/sixense sdk/SixenseSDK"
        "C:/Program Files (x86)/Steam/steamapps/common/sixense sdk/SixenseSDK"
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local/
        /usr/
        /sw # Fink
        /opt/local/ # DarwinPorts
        /opt/csw/ # Blastwave
        /opt/
        )


find_path(SIXENSE_INCLUDE_DIR sixense.h
        PATH_SUFFIXES
                "include"
        PATHS
                ${SIXENSE_POSSIBLE_PATHS}
        )

find_library(SIXENSE_LIBRARY 
        NAMES sixense sixense_x64
        PATH_SUFFIXES
                "lib/linux_x64/release"
        PATHS
                ${SIXENSE_POSSIBLE_PATHS}
        )

find_library(SIXENSE_UTIL_LIBRARY 
        NAMES sixense_utils sixense_utils_x64
        PATH_SUFFIXES
                "lib/linux_x64/release"
        PATHS
                ${SIXENSE_POSSIBLE_PATHS}
        )

set(SIXENSE_LIBRARIES ${SIXENSE_LIBRARY} ${SIXENSE_UTIL_LIBRARY})

find_package_handle_standard_args(Sixense  DEFAULT_MSG SIXENSE_LIBRARY SIXENSE_INCLUDE_DIR)
