set(EXTERNAL_NAME glfw)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://github.com/glfw/glfw/releases/download/3.1.1/glfw-3.1.1.bin.WIN32.zip
  URL_MD5 a66736409fa3328dd2222773e0884b9c
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE TYPE INTERNAL)
set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${SOURCE_DIR}/lib-vc2013/glfw3.lib CACHE TYPE INTERNAL)
set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG "" CACHE TYPE INTERNAL)
