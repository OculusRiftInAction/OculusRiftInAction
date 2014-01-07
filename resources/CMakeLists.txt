project(Resources)
include (GenerateExportHeader)

file(GLOB MESHES meshes/*.ctm)
file(GLOB FONTS fonts/*.sdff)
file(GLOB_RECURSE IMAGES images/*.png)
file(GLOB_RECURSE VERTEX_SHADERS shaders/*.vs)
file(GLOB_RECURSE FRAGMENT_SHADERS shaders/*.fs)
file(GLOB_RECURSE COMMON_SHADERS shaders/*.glsl)

set(ALL_SHADERS
    ${VERTEX_SHADERS} 
    ${FRAGMENT_SHADERS}
    ${COMMON_SHADERS}
) 

list(SORT MESHES )
list(SORT FONTS )
list(SORT IMAGES )
list(SORT VERTEX_SHADERS )
list(SORT FRAGMENT_SHADERS )
list(SORT COMMON_SHADERS )
list(SORT ALL_SHADERS)

source_group("Common Shaders" FILES 
    ${COMMON_VERTEX_SHADERS} 
    ${COMMON_FRAGMENT_SHADERS}
    ${COMMON_LIB_SHADERS}
)

set(ALL_RESOURCES 
    ${MESHES}
    ${FONTS}
    ${IMAGES}
    ${ALL_SHADERS}
)

set(ALL_RESOURCES
  ${ALL_RESOURCES}
  PARENT_SCOPE)
set(RESOURCE_ROOT
  ${CMAKE_CURRENT_SOURCE_DIR}
  PARENT_SCOPE)


# This is used by the resources and shaders logic to 
# allow us to load non-C++ resources into the program 
function(files_to_enum SOURCE_FILES RESULT_HEADER)

    foreach (file ${SOURCE_FILES})
        file(RELATIVE_PATH relative ${CMAKE_CURRENT_SOURCE_DIR} ${file})
        set(res ${relative})
        string(TOUPPER ${res} res)
        string(REGEX REPLACE "\\.\\./" "" res "${res}" )
        string(REGEX REPLACE "[^A-Z1-9]" "_" res "${res}" )
        set(HEADER_BUFFER "${HEADER_BUFFER}\t${res},\n")
        set(MAP_BUFFER "${MAP_BUFFER}  fileMap[${res}] = \"${file}\";\n")
        if (NOT(RIFT_DEBUG)) 
            if (APPLE)
                set(MAP_BUFFER "${MAP_BUFFER}  fileMap[${res}] = \"${relative}\";\n")
            elseif(WIN32) 
                set(MAP_BUFFER "${MAP_BUFFER}  fileMap[${res}] = \"${res}\";\n")
            endif()
        endif()
        set(RC_BUFFER "${RC_BUFFER}\n${res} TEXTFILE \"${file}\"")
    endforeach()
    set(${RESULT_HEADER} "${HEADER_BUFFER}" PARENT_SCOPE)

    list(LENGTH ARGN SIZE)
    if (SIZE GREATER 0)
        list(GET ARGN 0 RESULT_MAP) 
        list(REMOVE_AT ARGN 0) 
        set(${RESULT_MAP} "${MAP_BUFFER}" PARENT_SCOPE)
        list(LENGTH ARGN SIZE)
    endif()

    list(LENGTH ARGN SIZE)
    if (SIZE GREATER 0)
        list(GET ARGN 0 RESULT_RC) 
        list(REMOVE_AT ARGN 0) 
        set(${RESULT_RC} "${RC_BUFFER}" PARENT_SCOPE)
        list(LENGTH ARGN SIZE)
    endif()
endfunction()

files_to_enum("${VERTEX_SHADERS}" "VERTEX_SHADER_ENUM")
files_to_enum("${FRAGMENT_SHADERS}" "FRAGMENT_SHADER_ENUM" )
files_to_enum("${COMMON_SHADERS}" "COMMON_SHADER_ENUM")
files_to_enum("${ALL_SHADERS}" "SHADER_ENUM" "SHADER_MAP" "SHADER_RC")
files_to_enum("${ALL_RESOURCES}" "RESOURCE_ENUM" "RESOURCE_MAP" "RESOURCE_RC")

configure_file( Resources.h.in Resources.h )
configure_file( Resources.cpp.in Resources.cpp )

set(SOURCE_FILES 
    ResourcesImpl.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/Resources.cpp 
    ${CMAKE_CURRENT_BINARY_DIR}/Resources.h
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

if (HAVE_BOOST)
    list(APPEND RESOURCE_LIBRARIES ${Boost_LIBRARIES})
endif()

if(WIN32)
  configure_file( Resources.rc.in Resources.rc )
    list(APPEND SOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/Resources.rc)
    add_library(Resources SHARED ${SOURCE_FILES})
elseif(APPLE)
    # For OSX, until I can figure out how to create a framework, we're stuck 
    # with embedding all the resounces in every single application bundle.
    add_library(Resources STATIC ${SOURCE_FILES})
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    list(APPEND RESOURCE_LIBRARIES ${COREFOUNDATION_LIBRARY})
else()
    add_library(Resources STATIC ${SOURCE_FILES})
endif()

target_link_libraries(Resources ${RESOURCE_LIBRARIES})

GENERATE_EXPORT_HEADER( Resources
             BASE_NAME Resources
             EXPORT_MACRO_NAME Resources_EXPORT
             EXPORT_FILE_NAME Resources_Export.h
             STATIC_DEFINE Resources_STATIC
)
