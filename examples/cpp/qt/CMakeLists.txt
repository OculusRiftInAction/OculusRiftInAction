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

add_subdirectory(shadertoy)
add_subdirectory(video)
