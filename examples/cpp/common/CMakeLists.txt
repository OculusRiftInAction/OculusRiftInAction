file(GLOB SOURCE_FILES *.h *.cpp)
source_group(core FILES ${SOURCE_FILES})

foreach(dir rendering opengl glfw ovr)
    file(GLOB MORE_FILES ${dir}/*)
    list(APPEND SOURCE_FILES ${MORE_FILES})
    source_group(${dir} FILES ${MORE_FILES})
endforeach()

configure_file(Config.h.in Config.h)
list(APPEND SOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/Config.h )
source_group(core FILES ${CMAKE_CURRENT_BINARY_DIR}/Config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(ExampleCommon STATIC ${SOURCE_FILES})
target_link_libraries(ExampleCommon ${EXAMPLE_LIBS})
