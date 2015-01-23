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

file(GLOB SHADERTOY_FILES 
	shadertoy/*.cpp 
	shadertoy/*.h
)

file(GLOB SHADERTOY_RESOURCES  shadertoy/layouts/* )
source_group("Resources/Layouts" FILES ${SHADERTOY_RESOURCES})
list(APPEND SHADERTOY_FILES ${SHADERTOY_RESOURCES})

file(GLOB SHADERTOY_RESOURCES  shadertoy/presets/* )
source_group("Resources/Presets" FILES ${SHADERTOY_RESOURCES})
list(APPEND SHADERTOY_FILES ${SHADERTOY_RESOURCES})

file(GLOB SHADERTOY_RESOURCES  shadertoy/shaders/* )
source_group("Resources/Shaders" FILES ${SHADERTOY_RESOURCES})
list(APPEND SHADERTOY_FILES ${SHADERTOY_RESOURCES})

qt5_add_resources(SHADERTOY_RCC shadertoy/ShadertoyVR.qrc)
list(APPEND SHADERTOY_FILES ${SHADERTOY_RCC} ${SHADERTOY_RESOURCES})

make_example2("Examples/Qt" "ShadertoyVR" "${SHADERTOY_FILES}")


