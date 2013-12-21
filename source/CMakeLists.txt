file(GLOB EXAMPLE_APPS *.cpp)
list(SORT EXAMPLE_APPS)

foreach (file ${EXAMPLE_APPS})
    file(RELATIVE_PATH file ${CMAKE_CURRENT_SOURCE_DIR} ${file})
    string(REGEX REPLACE "^(.*)\\.cpp$" "\\1" name ${file} )
    string(REGEX REPLACE "^([^_]+(_[0-9]+)+).*\\.cpp$" "\\1" root ${file} )
    set(SOURCE_FILES ${file})

    file(GLOB EXAMPLE_SHADERS "${root}*.vs" "${root}*.fs")
    list(APPEND SOURCE_FILES ${EXAMPLE_SHADERS})
    source_group("Shaders" FILES ${EXAMPLE_SHADERS})

    file(GLOB_RECURSE COMMON_SHADERS "common/*.vs" "common/*.fs")
    list(APPEND SOURCE_FILES ${COMMON_SHADERS})
    source_group("Common Shaders" FILES ${COMMON_SHADERS})


    set(EXECUTABLE "${name}")

    if(WIN32)
        add_executable(${EXECUTABLE} WIN32 ${SOURCE_FILES} )
    elseif(APPLE)
        add_executable(${EXECUTABLE} MACOSX_BUNDLE ${SOURCE_FILES} )
    else()
        add_executable(${EXECUTABLE} ${SOURCE_FILES} )
    endif()

    target_link_libraries(${EXECUTABLE} ExampleCommon ${EXAMPLE_LIBS} ${EXTERNAL_LIBS})
endforeach()


