set(DEPTHSENSE_POSSIBLE_PATHS
    ${DEPTHSENSE_ROOT}
    $ENV{DEPTHSENSESDK64}
    $ENV{DEPTHSENSESDK}
    "C:/Program Files/SoftKinetic/DepthSenseSDK"
    "C:/Program Files (x86)/SoftKinetic/DepthSenseSDK"
)

find_path(DEPTHSENSE_INCLUDE_DIR 
    NAMES DepthSense.hxx
    PATH_SUFFIXES "include"
    PATHS ${DEPTHSENSE_POSSIBLE_PATHS}
)

find_library(DEPTHSENSE_LIBRARY 
    NAMES DepthSense
    PATH_SUFFIXES "lib"
    PATHS ${DEPTHSENSE_POSSIBLE_PATHS}
)

find_package_handle_standard_args(Depthsense DEFAULT_MSG DEPTHSENSE_LIBRARY DEPTHSENSE_INCLUDE_DIR)
