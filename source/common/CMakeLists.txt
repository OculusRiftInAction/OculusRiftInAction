file(GLOB SOURCE_FILES *.cpp *.h)

configure_file(Config.h.in Config.h)
list(APPEND SOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/Config.h )
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(ExampleCommon STATIC ${SOURCE_FILES} ${HEADER_FILES} ${SHADERS} )
target_link_libraries(ExampleCommon ${EXAMPLE_LIBS})
