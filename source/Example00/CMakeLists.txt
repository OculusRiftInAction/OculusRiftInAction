project(Example00)

set(SOURCE_FILES
	Example00.cpp 
)

set(SHADER_FILES
	Simple.vs Simple.fs 
	Texture.vs Texture.fs 
	Distort.vs Distort.fs
)

if(WIN32)

    list(APPEND SOURCE_FILES
		Example00.rc
		resource.h
    )
    
    add_executable( Example00 WIN32 ${SOURCE_FILES} ${SHADER_FILES} )

else(APPLE)

    SET_SOURCE_FILES_PROPERTIES(
        ${SHADER_FILES}
        PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
    add_executable(Example00 MACOSX_BUNDLE ${SOURCE_FILES} ${SHADER_FILES} )

else()

    add_executable(Example00 ${SOURCE_FILES} ${SHADER_FILES} )

endif()

target_link_libraries(Example00 OculusVR gl3w glfw ${GLFW_LIBRARIES})
