###############################################################################
#
# 
#

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(common)
include_directories(common)
list(APPEND EXAMPLE_LIBS ExampleCommonQt)
set_target_properties(ExampleCommonQt PROPERTIES FOLDER "Examples/Qt")

file(GLOB SHADERTOY_FILES shadertoy/*.cpp shadertoy/*.h)
qt5_add_resources(SHADERTOY_RCC shadertoy/ShadertoyVR.qrc)
list(APPEND SHADERTOY_FILES ${SHADERTOY_RCC})

make_example2("Examples/Qt" "ShadertoyVR" "${SHADERTOY_FILES}")


