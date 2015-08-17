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


set(SIXENSE_SUFFIX "")

if (WIN32)
#    set(SIXENSE_SUFFIX "${SIXENSE_SUFFIX}_s")
    if(${TARGET_ARCHITECTURE} STREQUAL "x86")
        set(SIXENSE_LIB_DIR "lib/win32/release_dll")
        set(SIXENSE_BIN_DIR "bin/win32/release_dll")
    else()
        set(SIXENSE_SUFFIX "${SIXENSE_SUFFIX}_x64")
        set(SIXENSE_LIB_DIR "lib/x64/release_dll")
        set(SIXENSE_BIN_DIR "bin/win32/release_dll")
    endif() 
elseif(APPLE)
    if(${TARGET_ARCHITECTURE} STREQUAL "x86")
        set(SIXENSE_LIB_DIR "lib/osx/release_dll")
    else()
        set(SIXENSE_SUFFIX "${SIXENSE_SUFFIX}_x64")
        set(SIXENSE_LIB_DIR "lib/osx_x64/release_dll")
    endif() 
elseif(UNIX)
    if(${TARGET_ARCHITECTURE} STREQUAL "x86")
        set(SIXENSE_LIB_DIR "lib/linux/release")
    else()
        set(SIXENSE_SUFFIX "${SIXENSE_SUFFIX}_x64")
        set(SIXENSE_LIB_DIR "lib/linux_x64/release")
    endif() 
endif()

find_path(SIXENSE_INCLUDE_DIR 
        NAMES           sixense.h
        PATH_SUFFIXES   include
        PATHS           ${SIXENSE_POSSIBLE_PATHS}
)

find_library(SIXENSE_LIBRARY 
        NAMES           sixense${SIXENSE_SUFFIX}
        PATH_SUFFIXES   ${SIXENSE_LIB_DIR}
        PATHS           ${SIXENSE_POSSIBLE_PATHS}
)

find_library(SIXENSE_UTIL_LIBRARY 
        NAMES           sixense_utils${SIXENSE_SUFFIX}
        PATH_SUFFIXES   ${SIXENSE_LIB_DIR}
        PATHS           ${SIXENSE_POSSIBLE_PATHS}
)

set(SIXENSE_LIBRARIES ${SIXENSE_UTIL_LIBRARY} ${SIXENSE_LIBRARY} )

if (WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES_SAVE ${CMAKE_FIND_LIBRARY_SUFFIXES})
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".dll")
    find_library(SIXENSE_BINARY
            NAMES           sixense${SIXENSE_SUFFIX}
            PATH_SUFFIXES   ${SIXENSE_BIN_DIR}
            PATHS           ${SIXENSE_POSSIBLE_PATHS}
    )
    
    find_library(SIXENSE_UTIL_BINARY
            NAMES           sixense_utils${SIXENSE_SUFFIX}
            PATH_SUFFIXES   ${SIXENSE_BIN_DIR}
            PATHS           ${SIXENSE_POSSIBLE_PATHS}
    )
    
    set(SIXENSE_BINARIES ${SIXENSE_UTIL_BINARY} ${SIXENSE_BINARY} )
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_SAVE})
endif()

find_package_handle_standard_args(Sixense DEFAULT_MSG SIXENSE_LIBRARIES SIXENSE_UTIL_LIBRARY SIXENSE_INCLUDE_DIR)
