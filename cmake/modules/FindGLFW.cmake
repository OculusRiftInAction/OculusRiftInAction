#
#  FindGLFW.cmake
#
#  Once done this will define
#
#  GLFW_FOUND - system found GLFW
#  GLFW_INCLUDE_DIRS - the GLFW include directory
#  GLFW_LIBRARIES - Link this to use GLFW
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

include(SelectLibraryConfigurations)
select_library_configurations(GLFW)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLFW DEFAULT_MSG GLFW_INCLUDE_DIRS GLFW_LIBRARIES)

mark_as_advanced(GLFW_INCLUDE_DIRS GLFW_LIBRARIES GLFW_SEARCH_DIRS)
