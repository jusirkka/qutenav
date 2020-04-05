cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(qopencpn LANGUAGES CXX)

#
# dependencies
#

set(QT_MIN_VERSION "5.9.7")
find_package(Qt5 ${QT_MIN_VERSION} REQUIRED CONFIG COMPONENTS
  Widgets
  Gui
)

set(KF5_MIN_VERSION "5.56.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED CONFIG)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
  Config
  I18n
  XmlGui
)

set(GDAL_MIN_VERSION "3.0.0")
find_package(GDAL ${GDAL_MIN_VERSION} REQUIRED)

#
# targets
#

add_executable(qopencpn)

set_target_properties(qopencpn
  PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_sources(qopencpn
  PRIVATE
    shaders.qrc
    files.qrc
    src/orthocam.cpp
    src/globe.cpp
    src/mainwindow.cpp
    src/outliner.cpp
    src/detailmode.cpp
    src/chartmode.cpp
    src/outlinemode.cpp
    src/glwidget.cpp
    src/main.cpp
    src/types.cpp
    src/perscam.cpp
    src/s57chart.cpp
)

#kconfig_add_kcfg_files(qopencpn
#  src/conf_mainwindow.kcfgc
#)


target_include_directories(qopencpn
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(qopencpn
  PRIVATE
    cxx_auto_type
    cxx_constexpr
    cxx_override
    cxx_range_for
)

target_link_libraries(qopencpn
  PRIVATE
    KF5::I18n
    KF5::XmlGui
    GDAL::GDAL
)


install(TARGETS qopencpn)

#
# .. wait, there's more!
#

#add_subdirectory(util)




feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

