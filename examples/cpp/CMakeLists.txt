###############################################################################
#
# Shared codebase for all the examples and demos
#
add_subdirectory(common)
set_target_properties(ExampleCommon PROPERTIES FOLDER "Examples/Shared")

include_directories(common)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/common)

function(make_example2 PROJECT_FOLDER NAME SOURCE_FILES) 
    set(EXECUTABLE "${NAME}")
    message("Making executable ${NAME} in folder ${PROJECT_FOLDER}")

    if(WIN32)
        add_executable(${EXECUTABLE} WIN32 ${SOURCE_FILES})
        list(APPEND EXAMPLE_LIBS d3d11)
        list(APPEND EXAMPLE_LIBS dxgi)
    elseif(APPLE)
        list(APPEND SOURCE_FILES ${ALL_RESOURCES})
        foreach(resource_file ${ALL_RESOURCES}) 
            file(RELATIVE_PATH relative_path ${RESOURCE_ROOT} ${resource_file})
            get_filename_component(relative_path ${relative_path} PATH)
            set_source_files_properties(${resource_file} 
                PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${relative_path})
            source_group("Resources\\\\${relative_path}" FILES ${resource_file})
        endforeach()
        add_executable(${EXECUTABLE} MACOSX_BUNDLE ${SOURCE_FILES} )
    else()
        add_executable(${EXECUTABLE} ${SOURCE_FILES} )
    endif()

    target_link_libraries(${EXECUTABLE} ExampleCommon ${EXAMPLE_LIBS})
    if (RIFT_DEBUG)
        set_property(TARGET ${EXECUTABLE} PROPERTY DEBUG_OUTPUT_NAME ${EXECUTABLE}_d)
    endif()
    set_target_properties(${EXECUTABLE} PROPERTIES FOLDER ${PROJECT_FOLDER})
endfunction()

function(make_example FILE PROJECT_FOLDER)
    set(SOURCE_FILES ${FILE})
    file(RELATIVE_PATH FILE ${CMAKE_CURRENT_SOURCE_DIR} ${FILE})
    get_filename_component(NAME ${FILE} NAME_WE)
    string(REGEX REPLACE "^([^_]+(_[0-9]+)+).*\\.cpp$" "\\1" ROOT ${FILE} )

    make_example2(${PROJECT_FOLDER} ${NAME} ${SOURCE_FILES} )
#    file(GLOB EXAMPLE_SHADERS "../resources/shaders/${ROOT}*.vs" "../resources/shaders/${ROOT}*.fs")
#    list(APPEND SOURCE_FILES ${EXAMPLE_SHADERS})
#    source_group("Shaders" FILES ${EXAMPLE_SHADERS})
#    file(GLOB_RECURSE COMMON_SHADERS "../resources/shaders/*.vs" "../resources/shaders/*.fs")
#    list(APPEND SOURCE_FILES ${COMMON_SHADERS})
#    source_group("Common Shaders" FILES ${COMMON_SHADERS})
endfunction()

function(make_examples GLOB PROJECT_FOLDER)
    file(GLOB EXAMPLE_APPS ${GLOB})
    list(SORT EXAMPLE_APPS)
    foreach (FILE ${EXAMPLE_APPS})
        make_example(${FILE} ${PROJECT_FOLDER})
    endforeach()
endfunction() 

make_examples(*.cpp "Examples")

if (OpenCV_FOUND)
    message(STATUS "Creating OpenCV examples")
    make_examples(opencv/*.cpp "Examples/OpenCV")
endif()

if (EXPERIMENTAL)
    make_examples(experimental/*.cpp "Examples/Experimental")
endif()

if (HAVE_QT)
    message(STATUS "Creating Qt examples")
    add_subdirectory(qt)
endif()




