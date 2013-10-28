project(ExampleCommon)


set(HEADER_FILES
    Common.h
    ExampleResources.h
    Files.h
    Font.h
    GlfwApp.h
    GlUtils.h
    IO.h
    Rift.h
    Vertex.h
)

set(SOURCE_FILES
    Common.cpp
    Files.cpp
    Font.cpp
    GlfwApp.cpp
    GlUtils.cpp
    Rift.cpp
)



if(WIN32)
elseif(APPLE)
else()
endif()

configure_file(${EXAMPLE_RESOURCE_ROOT}/ExampleResources.h.in
	${CMAKE_CURRENT_BINARY_DIR}/ExampleResources.h
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})



configure_file(${EXAMPLE_RESOURCE_ROOT}/ExampleResources.cpp.in
	${CMAKE_CURRENT_BINARY_DIR}/ExampleResources.cpp
)
list(APPEND SOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/ExampleResources.cpp )

add_library(ExampleCommon STATIC ${SOURCE_FILES} ${HEADER_FILES})
#target_link_libraries(ExampleCommon boost_filesystem boost_system)
