###############################################################################
#
# 
#

file(GLOB SOURCE_FILES *.cpp *.h)

file(GLOB APP_RESOURCES  layouts/* )
source_group("Resources/Layouts" FILES ${APP_RESOURCES})
list(APPEND SOURCE_FILES ${APP_RESOURCES})

file(GLOB APP_RESOURCES  presets/* )
source_group("Resources/Presets" FILES ${APP_RESOURCES})
list(APPEND SOURCE_FILES ${APP_RESOURCES})

file(GLOB APP_RESOURCES  shaders/* )
source_group("Resources/Shaders" FILES ${APP_RESOURCES})
list(APPEND SOURCE_FILES ${APP_RESOURCES})

file(GLOB QRC *.qrc )
qt5_add_resources(APP_RCC ${QRC})
list(APPEND SOURCE_FILES ${APP_RCC} ${APP_RESOURCES})

make_example2("Examples/Qt" "ShadertoyVR" "${SOURCE_FILES}")
qt5_use_modules(ShadertoyVR  ${QT_COMPONENTS})


