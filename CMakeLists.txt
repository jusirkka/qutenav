cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(qopencpn LANGUAGES CXX)

#
# dependencies
#

set(QT_MIN_VERSION "5.9.7")
find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS
  Sql
  Widgets
  Gui
  Xml
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

set(BISON_MIN_VERSION "3.1")
find_package(BISON ${BISON_MIN_VERSION} REQUIRED)

set(FLEX_MIN_VERSION "2.6.1")
find_package(FLEX ${FLEX_MIN_VERSION} REQUIRED)

set(FREETYPE_MIN_VERSION "2.10.4")
find_package(Freetype ${FREETYPE_MIN_VERSION} REQUIRED)

set(FONTCONFIG_MIN_VERSION "2.13")
find_package(Fontconfig ${FONTCONFIG_MIN_VERSION} REQUIRED)

#set(HARFBUZZ_VERSION "2.6.7")
find_package(harfbuzz ${HARFBUZZ_VERSION} REQUIRED)

set(GLM_VERSION "0.9.9")
find_package(glm ${GLM_VERSION} REQUIRED)

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
    src/camera.cpp
    src/chartfilereader.cpp
    src/chartmanager.cpp
    src/chartmode.cpp
    src/chartpainter.cpp
    src/detailmode.cpp
    src/drawable.cpp
    src/geoprojection.cpp
    src/glcontext.cpp
    src/globe.cpp
    src/glthread.cpp
    src/glwindow.cpp
    src/glyphmanager.cpp
    src/hpglparser.cpp
    src/linecalculator.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/orthocam.cpp
    src/osencreader.cpp
    src/outlinemode.cpp
    src/outliner.cpp
    src/perscam.cpp
    src/rastersymbolmanager.cpp
    src/s52functions.cpp
    src/s52presentation.cpp
    src/s52presentation_p.cpp
    src/s57chart.cpp
    src/s57chartoutline.cpp
    src/s57object.cpp
    src/settings.cpp
    src/shader.cpp
    src/symboldata.cpp
    src/textmanager.cpp
    src/tiny_sdf.cpp
    src/triangleoptimizer.cpp
    src/types.cpp
    src/vectorsymbolmanager.cpp
    src/wfreader.cpp
    src/GeographicLib/Geodesic.cpp
    src/GeographicLib/GeodesicLine.cpp
    src/GeographicLib/Math.cpp
    ${CMAKE_BINARY_DIR}/wavefront_parser.cpp
    ${CMAKE_BINARY_DIR}/wavefront_scanner.cpp
    ${CMAKE_BINARY_DIR}/s52instr_parser.cpp
    ${CMAKE_BINARY_DIR}/s52instr_scanner.cpp
    ${CMAKE_BINARY_DIR}/s52hpgl_parser.cpp
    ${CMAKE_BINARY_DIR}/s52hpgl_scanner.cpp
)

FLEX_TARGET(WFScanner src/wavefront_scanner.l ${CMAKE_BINARY_DIR}/wavefront_scanner.cpp
  DEFINES_FILE ${CMAKE_BINARY_DIR}/wavefront_scanner.h)

BISON_TARGET(WFParser src/wavefront_parser.y ${CMAKE_BINARY_DIR}/wavefront_parser.cpp
  DEFINES_FILE ${CMAKE_BINARY_DIR}/wavefront_parser.h)

FLEX_TARGET(S52IScanner src/s52instr_scanner.l ${CMAKE_BINARY_DIR}/s52instr_scanner.cpp
  DEFINES_FILE ${CMAKE_BINARY_DIR}/s52instr_scanner.h)

BISON_TARGET(S52IParser src/s52instr_parser.y ${CMAKE_BINARY_DIR}/s52instr_parser.cpp
  DEFINES_FILE ${CMAKE_BINARY_DIR}/s52instr_parser.h)

FLEX_TARGET(S52HScanner src/s52hpgl_scanner.l ${CMAKE_BINARY_DIR}/s52hpgl_scanner.cpp
  DEFINES_FILE ${CMAKE_BINARY_DIR}/s52hpgl_scanner.h)

BISON_TARGET(S52HParser src/s52hpgl_parser.y ${CMAKE_BINARY_DIR}/s52hpgl_parser.cpp
  DEFINES_FILE ${CMAKE_BINARY_DIR}/s52hpgl_parser.h)

kconfig_add_kcfg_files(qopencpn
  src/conf_mainwindow.kcfgc
  src/conf_detailmode.kcfgc
  src/conf_marinerparams.kcfgc
)


target_include_directories(qopencpn
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}
)

target_compile_features(qopencpn
  PRIVATE
    cxx_std_17
)

target_link_libraries(qopencpn
  PRIVATE
    KF5::I18n
    KF5::XmlGui
    Qt5::Sql
    Freetype::Freetype
    Fontconfig::Fontconfig
    harfbuzz::harfbuzz
)


install(TARGETS qopencpn)

#
# .. wait, there's more!
#

add_subdirectory(triglobe)




feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

