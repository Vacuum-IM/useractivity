cmake_minimum_required(VERSION 2.8)

project(useractivity-plugin)
set(PLUGIN_NAME useractivity)
set(SOURCES useractivity.cpp useractivitydialog.cpp)
set(HEADERS useractivity.h useractivitydialog.h definitions.h)
set(MOC_HEADERS useractivity.h useractivitydialog.h)
set(UIS useractivitydialog.ui)

include_directories("${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")

set(LOCALIZED_LANGS "ru;pl;de;uk")

include("${CMAKE_SOURCE_DIR}/cmake/config.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/translations.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/plugin.cmake")

install(DIRECTORY "resources/menuicons" DESTINATION "${INSTALL_RESOURCES}")
install(DIRECTORY "resources/activityicons" DESTINATION "${INSTALL_RESOURCES}")
