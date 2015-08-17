macro(ADD_EXAMPLE_DEPENDENCIES)
    foreach(_PROJ_NAME ${ARGN})
        set(TARGET_NAME ${_PROJ_NAME})

        target_link_libraries(${TARGET_NAME} opengl32.lib)

        find_library(OpenGL OpenGL)  

        # GLM - Vector / matrix header only math library based on the syntax of GLSL
        add_dependency_external_projects(glm)
        find_package(GLM REQUIRED)

        add_dependency_external_projects(boostconfig)
        find_package(BoostConfig REQUIRED)

        add_dependency_external_projects(oglplus)
        find_package(OGLPLUS REQUIRED)
        add_definitions(-DOGLPLUS_LOW_PROFILE=1)
        add_definitions(-DOGLPLUS_USE_GLEW=1)
        add_definitions(-DOGLPLUS_USE_BOOST_CONFIG=1)
        add_definitions(-DOGLPLUS_NO_SITE_CONFIG=1)

        add_dependency_external_projects(glew)
        find_package(GLEW REQUIRED)
        add_definitions(-DGLEW_STATIC) 
        list(APPEND EXAMPLE_LIBS ${GLEW_LIBRARY})
        target_link_libraries(${TARGET_NAME} ${GLEW_LIBRARY})

        add_dependency_external_projects(glfw)
        find_package(GLFW REQUIRED)
        target_link_libraries(${TARGET_NAME} ${GLFW_LIBRARY})

        add_dependency_external_projects(LibOVR)
        find_package(LibOVR REQUIRED)
        target_link_libraries(${TARGET_NAME} ${LIBOVR_LIBRARIES})
    endforeach()
endmacro()

