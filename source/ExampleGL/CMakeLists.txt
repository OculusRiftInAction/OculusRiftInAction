project(ExampleGL)

set(HEADER_FILES
    GlBuffers.h
    GlDebug.h
    GlFrameBuffer.h
    GlGeometry.h
    GlLighting.h
    GlMethods.h
    GlShaders.h
    GlVertexArrays.h
)

add_library(ExampleGL STATIC ${HEADER_FILES})
set_target_properties(ExampleGL PROPERTIES LINKER_LANGUAGE CXX)