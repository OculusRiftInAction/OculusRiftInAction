project(ExampleResources)

file(GLOB EXAMPLE_SHADERS ${EXAMPLE_RESOURCE_ROOT}/shaders/*)

add_library(ExampleResources STATIC ${EXAMPLE_SHADERS})
set_target_properties(ExampleResources PROPERTIES LINKER_LANGUAGE CXX)