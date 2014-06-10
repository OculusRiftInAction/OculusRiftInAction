file(GLOB SOURCE_FILES *.cpp)
list(REMOVE_ITEM SOURCE_FILES Common.cpp)
file(GLOB HEADER_FILES *.h)

configure_file(Config.h.in Config.h)
list(APPEND HEADER_FILES ${CMAKE_CURRENT_BINARY_DIR}/Config.h )
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(ExampleCommon STATIC Common.cpp ${SOURCE_FILES} ${HEADER_FILES} ${SHADERS} )
target_link_libraries(ExampleCommon ${EXAMPLE_LIBS})
