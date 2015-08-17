set(TDXWARE_POSSIBLE_PATHS
    "C:/dev/3dxware"
)

find_path(3DxWare_INCLUDE_DIR 
    NAMES si.h
    PATH_SUFFIXES "Inc"
    PATHS ${TDXWARE_POSSIBLE_PATHS}
)

find_library(3DxWare_LIBRARY 
    NAMES siapp
    PATH_SUFFIXES "Lib/x86"
    PATHS ${TDXWARE_POSSIBLE_PATHS}
)

find_package_handle_standard_args(
    3DxWare
    FOUND_VAR 3DxWare_FOUND
    REQUIRED_VARS 3DxWare_LIBRARY 3DxWare_INCLUDE_DIR)

