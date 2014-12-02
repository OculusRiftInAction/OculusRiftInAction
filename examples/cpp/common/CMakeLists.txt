file(GLOB SOURCE_FILES 
    *.h *.cpp 
    rendering/*.h rendering/*.cpp 
    opengl/*.h opengl/*.cpp
)

if(HAVE_QT)
    file(GLOB QT_SOURCE_FILES qt/*.h qt/*.cpp)
    list(APPEND SOURCE_FILES ${QT_SOURCE_FILES})
endif()

configure_file(Config.h.in Config.h)
list(APPEND HEADER_FILES ${CMAKE_CURRENT_BINARY_DIR}/Config.h )
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(ExampleCommon STATIC ${SOURCE_FILES} ${HEADER_FILES} ${SHADERS} )
target_link_libraries(ExampleCommon ${EXAMPLE_LIBS})
