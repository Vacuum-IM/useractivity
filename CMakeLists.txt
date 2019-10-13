cmake_minimum_required(VERSION 2.8)

project(useractivity-plugin)
set(PLUGIN_NAME useractivity)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Xml REQUIRED)
list(APPEND QT_LIBRARIES Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Xml)

set(SOURCES useractivity.cpp useractivitydialog.cpp)
set(HEADERS useractivity.h useractivitydialog.h definitions.h)
set(MOC_HEADERS useractivity.h useractivitydialog.h)
set(UIS useractivitydialog.ui)

include_directories("${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")

include("${CMAKE_SOURCE_DIR}/cmake/config.cmake")
include("${CMAKE_SOURCE_DIR}/translations/languages.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/plugin.cmake")

install(DIRECTORY "resources/menuicons" DESTINATION "${INSTALL_RESOURCES}")
install(DIRECTORY "resources/activityicons" DESTINATION "${INSTALL_RESOURCES}")
