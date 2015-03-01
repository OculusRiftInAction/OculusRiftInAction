file(GLOB SOURCE_FILES *.h *.cpp)

qt5_add_resources(COMMON_RCC Common.qrc)
list(APPEND SOURCE_FILES ${COMMON_RCC})

add_library(ExampleCommonQt STATIC ${SOURCE_FILES})
qt5_use_modules(ExampleCommonQt  ${QT_COMPONENTS})
