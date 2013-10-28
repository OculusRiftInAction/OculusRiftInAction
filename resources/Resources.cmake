set(EXAMPLE_RESOURCE_ROOT
    ${CMAKE_SOURCE_DIR}/resources
)

file(GLOB EXAMPLE_MESHES ${EXAMPLE_RESOURCE_ROOT}/meshes/*.ctm)

file(GLOB EXAMPLE_FONTS ${EXAMPLE_RESOURCE_ROOT}/fonts/*)

file(GLOB EXAMPLE_VERTEX_SHADERS ${EXAMPLE_RESOURCE_ROOT}/shaders/*.vs)
file(GLOB EXAMPLE_FRAGMENT_SHADERS ${EXAMPLE_RESOURCE_ROOT}/shaders/*.fs)
file(GLOB EXAMPLE_SHADERS ${EXAMPLE_RESOURCE_ROOT}/shaders/*)

list(SORT EXAMPLE_VERTEX_SHADERS )
list(SORT EXAMPLE_FRAGMENT_SHADERS )
list(SORT EXAMPLE_SHADERS )


set(EXAMPLE_ALL_RESOURCES 
    ${EXAMPLE_MESHES}
    ${EXAMPLE_SHADERS}
    ${EXAMPLE_FONTS}
)

set(EXAMPLE_RC_FILE
    ${EXAMPLE_RESOURCE_ROOT}/ExampleResources.rc
)

set(WINDOWS_RESOURCES "")

foreach (file ${EXAMPLE_ALL_RESOURCES})
	file(RELATIVE_PATH res ${EXAMPLE_RESOURCE_ROOT}  ${file})
	string(REGEX REPLACE " " "_" res "${res}" )
    set(WINDOWS_RESOURCES "${WINDOWS_RESOURCES}\n${res} TEXTFILE \"${file}\"")
endforeach()

set(RESOURCE_HEADER "")

function(process_resources PATH SOURCE_FILES)
	foreach (file ${SOURCE_FILES})
		file(RELATIVE_PATH res ${EXAMPLE_RESOURCE_ROOT}/${PATH}  ${file})
		set(res "${PATH}_${res}")
		string(TOUPPER ${res} res)
		string(REGEX REPLACE "[^A-Z1-9]" "_" res "${res}" )
	    set(HEADER_BUFFER "${HEADER_BUFFER}\t${res},\n")
	    set(MAP_BUFFER "${MAP_BUFFER}\t\tfileMap[${res}] = \"${file}\";\n")
    endforeach()
    set(RESOURCE_HEADER "${RESOURCE_HEADER}${HEADER_BUFFER}" PARENT_SCOPE)
	set(RESOURCE_MAP "${RESOURCE_MAP}${MAP_BUFFER}" PARENT_SCOPE)
endfunction()

process_resources("shaders" "${EXAMPLE_SHADERS}")
process_resources("fonts" "${EXAMPLE_FONTS}")
process_resources("meshes" "${EXAMPLE_MESHES}")

