cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(qopencpn LANGUAGES CXX C)

set(DESKTOP ON)

if (MOBILE)
  set (DESKTOP OFF)
endif()

#
# dependencies
#

set(QT_MIN_VERSION "5.6.3")

if (DESKTOP)

  find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Sql
    Widgets
    Gui
    Xml
    OpenGL
    DBus
  )

  set(KF5_MIN_VERSION "5.56.0")

  find_package(ECM ${KF5_MIN_VERSION} REQUIRED CONFIG)
  list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

  find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    I18n
    XmlGui
  )

  set(GDAL_MIN_VERSION "3.0.0")
  find_package(GDAL ${GDAL_MIN_VERSION} REQUIRED)

  set(FONTCONFIG_MIN_VERSION "2.13")
  find_package(Fontconfig ${FONTCONFIG_MIN_VERSION} REQUIRED)

  set(FONTCONFIG_MIN_VERSION "2.13")
  find_package(Fontconfig ${FONTCONFIG_MIN_VERSION} REQUIRED)

  set(GLM_VERSION "0.9.9")
  find_package(glm ${GLM_VERSION} REQUIRED)

else()

  find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Sql
    Xml
    Quick
    OpenGL
    DBus
  )

  include(FindPkgConfig)
  # make it cooperate with qt-creator by setting EMU_PATH to appr. value
  set (CMAKE_PREFIX_PATH "$ENV{EMU_PATH}")
  pkg_search_module(SAILFISH REQUIRED sailfishapp)
  pkg_search_module(GLM REQUIRED glm)
  pkg_search_module(Fontconfig REQUIRED fontconfig)

endif()

set(BISON_MIN_VERSION "3.1")
find_package(BISON ${BISON_MIN_VERSION} REQUIRED)

set(FLEX_MIN_VERSION "2.6.1")
find_package(FLEX ${FLEX_MIN_VERSION} REQUIRED)

set(FREETYPE_MIN_VERSION "2.10.0")
find_package(Freetype ${FREETYPE_MIN_VERSION} REQUIRED)

find_package(harfbuzz REQUIRED)


#
# targets
#

add_executable(qopencpn)
add_executable(qopencpn_dbupdater)


set_target_properties(qopencpn
  PROPERTIES
    AUTOMOC ON
)

target_sources(qopencpn
  PRIVATE
    src/camera.cpp
    src/cachereader.cpp
    src/chartcover.cpp
    src/chartdatabase.cpp
    src/chartfilereader.cpp
    src/chartmanager.cpp
    src/chartmode.cpp
    src/chartpainter.cpp
    src/detailmode.cpp
    src/drawable.cpp
    src/geoprojection.cpp
    src/globe.cpp
    src/glthread.cpp
    src/glyphmanager.cpp
    src/hpglparser.cpp
    src/linecalculator.cpp
    src/orthocam.cpp
    src/outlinemode.cpp
    src/outliner.cpp
    src/osenc.cpp
    src/perscam.cpp
    src/rastersymbolmanager.cpp
    src/s52functions.cpp
    src/s52names.cpp
    src/s52presentation.cpp
    src/s52presentation_p.cpp
    src/s57chart.cpp
    src/s57chart_objectinfo.cpp
    src/s57chartoutline.cpp
    src/s57object.cpp
    src/s57paintdata.cpp
    src/settings.cpp
    src/shader.cpp
    src/symboldata.cpp
    src/textmanager.cpp
    src/tiny_sdf.cpp
    src/types.cpp
    src/utils.cpp
    src/vectorsymbolmanager.cpp
    src/wfreader.cpp
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

if (DESKTOP)
set_target_properties(qopencpn_dbupdater
  PROPERTIES
    AUTOMOC ON
)
else()
  set_target_properties(qopencpn_dbupdater
    PROPERTIES
      AUTOMOC OFF
  )
endif()

set(dbus_SRCS)
qt5_add_dbus_adaptor(dbus_SRCS
  src/dbupdater.xml
  updater.h
  Updater
  dbupdater_adaptor
)

target_sources(qopencpn_dbupdater
  PRIVATE
    src/dbupdater.cpp
    src/chartdatabase.cpp
    src/chartfilereader.cpp
    src/geoprojection.cpp
    src/osenc.cpp
    src/s52names.cpp
    src/s57chartoutline.cpp
    src/s57object.cpp
    src/types.cpp
    ${dbus_SRCS}
)

if (DESKTOP)

  target_include_directories(qopencpn
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/geographiclib/src
      ${CMAKE_CURRENT_SOURCE_DIR}/triangulate/src
      ${CMAKE_CURRENT_SOURCE_DIR}/desktop/src
      ${CMAKE_BINARY_DIR}/desktop
      ${CMAKE_BINARY_DIR}
  )

else()

  target_include_directories(qopencpn
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/geographiclib/src
      ${CMAKE_CURRENT_SOURCE_DIR}/triangulate/src
      ${CMAKE_CURRENT_SOURCE_DIR}/mobile/src
      ${CMAKE_BINARY_DIR}
  )

endif()

target_include_directories(qopencpn_dbupdater
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/dbupdater/src
    ${CMAKE_CURRENT_SOURCE_DIR}/geographiclib/src
    ${CMAKE_CURRENT_SOURCE_DIR}/triangulate/src
    ${CMAKE_BINARY_DIR}
)


target_compile_features(qopencpn
  PRIVATE
    cxx_std_17
)

target_compile_features(qopencpn_dbupdater
  PRIVATE
    cxx_std_17
)
#
# subdirectories
#

add_subdirectory(geographiclib)
add_subdirectory(triangulate)
add_subdirectory(osencreader)
add_subdirectory(oesencreader)
add_subdirectory(cm93reader)
add_subdirectory(s57reader)
add_subdirectory(dbupdater)


if (DESKTOP)

  add_subdirectory(triglobe)
  add_subdirectory(desktop)

else()

  add_subdirectory(mobile)

endif()

if (DESKTOP)

  target_link_libraries(qopencpn
    PRIVATE
      GeographicLib
      Triangulate
      Desktop
      Osencreader
      Oesencreader
      Cm93reader
      S57reader
      Qt5::Core
      Qt5::OpenGL
      KF5::I18n
      KF5::XmlGui
      Freetype::Freetype
      Fontconfig::Fontconfig
      harfbuzz::harfbuzz
  )

else()

  target_link_libraries(qopencpn
    PRIVATE
      ${SAILFISH_LDFLAGS}
      GeographicLib
      Triangulate
      Mobile
      Osencreader
      Oesencreader
      Cm93reader
      S57reader
      Qt5::Quick
      Qt5::Gui
      Qt5::Sql
      Freetype::Freetype
      fontconfig
      harfbuzz::harfbuzz
      -lGLESv2
  )

endif()

target_link_libraries(qopencpn_dbupdater
  PRIVATE
    Osencreader
    Oesencreader
    Cm93reader
    S57reader
    Dbupdater
    GeographicLib
    Triangulate
    Qt5::Core
    Qt5::Sql
    Qt5::OpenGL
)

install(TARGETS qopencpn DESTINATION bin)
install(TARGETS qopencpn_dbupdater DESTINATION bin)

