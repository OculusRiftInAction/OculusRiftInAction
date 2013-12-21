project(Resources)

file(GLOB MESHES ${CMAKE_CURRENT_SOURCE_DIR}/meshes/*.ctm)
file(GLOB FONTS ${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.sdff)
file(GLOB_RECURSE IMAGES ${CMAKE_CURRENT_SOURCE_DIR}/images/*.png)
file(GLOB STEREO_IMAGES ${CMAKE_CURRENT_SOURCE_DIR}/stereo_images/*.png)

list(SORT MESHES )
list(SORT FONTS )
list(SORT IMAGES )
list(SORT STEREO_IMAGES )

set(ALL_RESOURCES 
    ${MESHES}
    ${FONTS}
    ${IMAGES}
    ${STEREO_IMAGES}
)

files_to_enum("${ALL_RESOURCES}" "RESOURCE_ENUM" "RESOURCE_MAP")

configure_file(
    Resources.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/Resources.h
)

configure_file(
    Resources.cpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/Resources.cpp
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_library(resources STATIC 
    ${CMAKE_CURRENT_BINARY_DIR}/Resources.cpp 
    ${CMAKE_CURRENT_BINARY_DIR}/Resources.h)
