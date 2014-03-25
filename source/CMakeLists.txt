
function(make_example FILE PROJECT_FOLDER)
    set(SOURCE_FILES ${FILE})
    #set(PRECOMPILED_BINARY ${CMAKE_BINARY_DIR}/source/common/Common.pch)
    #SET_SOURCE_FILES_PROPERTIES(${FILE}
    #                            PROPERTIES COMPILE_FLAGS "/Yu\"Common.h\" /FI\"Common.h\" /Fp\"${PRECOMPILED_BINARY}\""
    #                                       OBJECT_DEPENDS "${PRECOMPILED_BINARY}")

    file(RELATIVE_PATH FILE ${CMAKE_CURRENT_SOURCE_DIR} ${FILE})
    get_filename_component(NAME ${FILE} NAME_WE)
    string(REGEX REPLACE "^([^_]+(_[0-9]+)+).*\\.cpp$" "\\1" ROOT ${FILE} )

    file(GLOB EXAMPLE_SHADERS "../resources/shaders/${ROOT}*.vs" "../resources/shaders/${ROOT}*.fs")
    list(APPEND SOURCE_FILES ${EXAMPLE_SHADERS})
    source_group("Shaders" FILES ${EXAMPLE_SHADERS})

    file(GLOB_RECURSE COMMON_SHADERS "../resources/shaders/*.vs" "../resources/shaders/*.fs")
    list(APPEND SOURCE_FILES ${COMMON_SHADERS})
    source_group("Common Shaders" FILES ${COMMON_SHADERS})
    set(EXECUTABLE "${NAME}")

    if(WIN32)
        add_executable(${EXECUTABLE} WIN32 ${SOURCE_FILES})
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

function(make_examples GLOB PROJECT_FOLDER)
    file(GLOB EXAMPLE_APPS ${GLOB})
    list(SORT EXAMPLE_APPS)
    foreach (FILE ${EXAMPLE_APPS})
        make_example(${FILE} ${PROJECT_FOLDER})
    endforeach()
endfunction() 

make_examples(*.cpp "Examples")

if (OpenCV_FOUND) 
    make_examples(opencv/*.cpp "Examples/OpenCV")
endif()


find_package( OpenSceneGraph COMPONENTS OpenThreads osg osgDB osgUtil osgGA osgViewer osgText osgSim)
if(OPENSCENEGRAPH_FOUND)
    include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS}) 
    list(APPEND EXAMPLE_LIBS ${OPENSCENEGRAPH_LIBRARIES})
    make_examples(OpenSceneGraph/*.cpp  "Examples/OpenSceneGraph")
endif()

find_package( Steamworks )
if(STEAMWORKS_FOUND)
    include_directories(${STEAMWORKS_INCLUDE_DIR}) 
    list(APPEND EXAMPLE_LIBS ${STEAMWORKS_LIBRARY})
    make_examples(steamworks/*.cpp  "Examples/Steamworks")
endif()

find_package( Depthsense )
if(DEPTHSENSE_FOUND)
    include_directories(${PCL_INCLUDE_DIR})
    include_directories(${DEPTHSENSE_INCLUDE_DIR})
    list(APPEND EXAMPLE_LIBS ${DEPTHSENSE_LIBRARY})
    set(HAVE_DEPTHSENSE 1)
    make_examples(opencv/depthsense/*.cpp "Examples/DepthSense")
endif()

if (RIFT_BUILD_SCRATCH_EXAMPLES)
    message("Building scratch examples") 
    make_examples(scratch/*.cpp "Examples/Scratch")
endif()




if (0)
    find_package( LeapMotion )
    if(LEAP_FOUND)
        file(COPY ${LEAP_BINARY} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        include_directories(${LEAP_INCLUDE_DIR}) 
        list(APPEND EXAMPLE_LIBS ${LEAP_LIBRARY})
    endif()

    find_package( PerceptualComputing )
    if (PERC_FOUND) 
        include_directories(${PERC_INCLUDE_DIR})
        list(APPEND EXAMPLE_LIBS ${PERC_LIBRARY})
        set(HAVE_PERC 1)
    endif()
endif()


#find_package( PCL 1.3 REQUIRED COMPONENTS io filters segmentation surface)
#  include_directories(${PCL_INCLUDE_DIRS}) 
#  message(${PCL_INCLUDE_DIRS})
#  link_directories(${PCL_LIBRARY_DIRS})
#  list(APPEND EXAMPLE_LIBS ${PCL_LIBRARIES})
#  add_definitions(${PCL_DEFINITIONS})

