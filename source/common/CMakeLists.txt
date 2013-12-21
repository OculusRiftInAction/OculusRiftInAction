
set(HEADER_FILES
    Common.h
    Files.h
    Font.h
    GlfwApp.h
    GlMesh.h
    GlUtils.h
    Interaction.h
    IO.h
    Rift.h
    Vertex.h
)

set(SOURCE_FILES
    Common.cpp
    Files.cpp
    Font.cpp
    GlfwApp.cpp
    GlMesh.cpp
    GlUtils.cpp
    Interaction.cpp
    Rift.cpp
)

configure_file(Config.h.in Config.h)
list(APPEND HEADER_FILES Config.h )

file(GLOB COMMON_VERTEX_SHADERS shaders/*.vs)
file(GLOB COMMON_FRAGMENT_SHADERS shaders/*.fs)
file(GLOB COMMON_LIB_SHADERS shaders/noise/*.glsl)

files_to_enum("${COMMON_VERTEX_SHADERS}" "VERTEX_SHADER_ENUM" "SHADER_MAP")
files_to_enum("${COMMON_FRAGMENT_SHADERS}" "FRAGMENT_SHADER_ENUM" "SHADER_MAP")
files_to_enum("${COMMON_LIB_SHADERS}" "COMMON_SHADER_ENUM" "SHADER_MAP")

source_group("Common Shaders" FILES 
    ${COMMON_VERTEX_SHADERS} 
    ${COMMON_FRAGMENT_SHADERS}
    ${COMMON_LIB_SHADERS}
)

file(GLOB EXAMPLE_SHADERS ../*.vs ../*.fs)

set(ALL_SHADERS
    ${COMMON_VERTEX_SHADERS} 
    ${COMMON_FRAGMENT_SHADERS}
    ${COMMON_LIB_SHADERS}
    ${EXAMPLE_SHADERS}) 
list(SORT ALL_SHADERS)

files_to_enum("${ALL_SHADERS}" "SHADER_ENUM" "SHADER_MAP")

configure_file(
    Shaders.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/Shaders.h
)
list(APPEND HEADER_FILES ${CMAKE_CURRENT_BINARY_DIR}/Shaders.h)

configure_file(
    Shaders.cpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/Shaders.cpp
)
list(APPEND HEADER_FILES ${CMAKE_CURRENT_BINARY_DIR}/Shaders.cpp)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(ExampleCommon STATIC ${SOURCE_FILES} ${HEADER_FILES} ${SHADERS})
#target_link_libraries(ExampleCommon boost_filesystem boost_system)
