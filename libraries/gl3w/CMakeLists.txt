project (GL3W C)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(gl3w STATIC gl3w.c GL/gl3w.h)
