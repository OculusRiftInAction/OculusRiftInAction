project (jsoncpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(jsoncpp STATIC jsoncpp.cpp json/json.h)
