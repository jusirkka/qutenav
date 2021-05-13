cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

# enable_testing()

project(qutenav LANGUAGES CXX C)

if (NOT DEFINED PLATFORM)
  set(PLATFORM qtcontrols)
endif()

#
# dependencies
#

set(QT_MIN_VERSION "5.6.3")

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS
  Sql
  Xml
  Quick
  OpenGL
  DBus
)


if (PLATFORM STREQUAL qtcontrols)

  set(FONTCONFIG_MIN_VERSION "2.13")
  find_package(Fontconfig ${FONTCONFIG_MIN_VERSION} REQUIRED)

  set(GLM_VERSION "0.9.9")
  find_package(glm ${GLM_VERSION} REQUIRED)

  set(PLATFORM_SHADERS "shaders_opengl_desktop")
  set(PLATFORM_QML_QRC "qml_qtcontrols.qrc")

elseif (PLATFORM STREQUAL silica)

  include(FindPkgConfig)
  # make it cooperate with qt-creator by setting EMU_PATH to appr. value
  set (CMAKE_PREFIX_PATH "$ENV{EMU_PATH}")
  pkg_search_module(SAILFISH REQUIRED sailfishapp)
  pkg_search_module(GLM REQUIRED glm)
  pkg_search_module(Fontconfig REQUIRED fontconfig)

  set(PLATFORM_LDFLAGS "${SAILFISH_LDFLAGS}")
  set(PLATFORM_LIBS -lGLESv2)
  set(PLATFORM_SHADERS "shaders_opengl_es")

  add_custom_target(Qml SOURCES
    qml/Boat.qml
    qml/Bubble.qml
    qml/CenterButton.qml
    qml/ChartPage.qml
    qml/CoverPage.qml
    qml/DimensionalValue.qml
    qml/DistanceButton.qml
    qml/EditButton.qml
    qml/InfoPage.qml
    qml/MenuButton.qml
    qml/MenuPage.qml
    qml/platform.silica/ApplicationWindowPL.qml
    qml/platform.silica/ChartPagePL.qml
    qml/platform.silica/ComboBoxPL.qml
    qml/platform.silica/DialogPL.qml
    qml/platform.silica/IconListItemPL.qml
    qml/platform.silica/LabelPL.qml
    qml/platform.silica/ListViewDelegatePL.qml
    qml/platform.silica/ListViewPL.qml
    qml/platform.silica/MapButtonPL.qml
    qml/platform.silica/PagePL.qml
    qml/platform.silica/SectionHeaderPL.qml
    qml/platform.silica/SwitchDelegatePL.qml
    qml/platform.silica/TextFieldPL.qml
    qml/platform.silica/TextSwitchPL.qml
    qml/platform.silica/ThemePL.qml
    qml/platform.silica/ViewPlaceholderPL.qml
    qml/PreferencesPage.qml
    qml/qutenav.qml
    qml/RouteButton.qml
    qml/RouteDisplayDialog.qml
    qml/RoutePoint.qml
    qml/RoutingResultDialog.qml
    qml/RoutingStartDialog.qml
    qml/ScaleBar.qml
    qml/TrackButton.qml
    qml/TrackDisplayDialog.qml
    qml/TrackInfoBox.qml
    qml/TrackInfo.qml
    qml/TrackPointInfoBox.qml
    qml/TrackResultDialog.qml
    qml/TrackSpeedInfoBox.qml
    qml/TrackTargetInfoBox.qml
    qml/ValuePref.qml
  )


endif ()

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

add_executable(qutenav)
add_executable(qutenav_dbupdater)


set_target_properties(qutenav
  PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)

target_sources(qutenav
  PRIVATE
    ${PLATFORM_QML_QRC}
    shaders/${PLATFORM_SHADERS}.qrc
    src/camera.cpp
    src/cachereader.cpp
    src/chartcover.cpp
    src/chartdatabase.cpp
    src/chartfilereader.cpp
    src/chartmanager.cpp
    src/chartmode.cpp
    src/chartpainter.cpp
    src/chartupdater.cpp
    src/dbupdater_interface.cpp
    src/detailmode.cpp
    src/drawable.cpp
    src/geoprojection.cpp
    src/globe.cpp
    src/glthread.cpp
    src/glyphmanager.cpp
    src/hpglparser.cpp
    src/linecalculator.cpp
    src/logging.cpp
    src/orthocam.cpp
    src/outlinemode.cpp
    src/outliner.cpp
    src/osenc.cpp
    src/perscam.cpp
    src/platform.cpp
    src/rastersymbolmanager.cpp
    src/region.cpp
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
    src/sqlitedatabase.cpp
    src/symboldata.cpp
    src/textmanager.cpp
    src/tiny_sdf.cpp
    src/types.cpp
    src/utils.cpp
    src/vectorsymbolmanager.cpp
    src/wfreader.cpp
    src/chartdisplay.cpp
    src/chartrenderer.cpp
    src/configgroup.cpp
    src/conf_detailmode.cpp
    src/conf_mainwindow.cpp
    src/conf_marinerparams.cpp
    src/crosshairs.cpp
    src/databasemodel.cpp
    src/routedatabase.cpp
    src/router.cpp
    src/routetracker.cpp
    src/routemodel.cpp
    src/trackdatabase.cpp
    src/tracker.cpp
    src/trackmodel.cpp
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

# automoc generates duplicate mocs for dbus adaptor files in older versions
if (${CMAKE_VERSION} VERSION_LESS "3.18")
  set_target_properties(qutenav_dbupdater
    PROPERTIES
      AUTOMOC OFF
  )
else()
  set_target_properties(qutenav_dbupdater
    PROPERTIES
      AUTOMOC ON
  )
endif()

set(dbus_adaptor_SRCS)
qt5_add_dbus_adaptor(dbus_adaptor_SRCS
  src/dbupdater.xml
  updater.h
  Updater
  dbupdater_adaptor
)

target_sources(qutenav_dbupdater
  PRIVATE
    src/dbupdater.cpp
    src/chartdatabase.cpp
    src/chartfilereader.cpp
    src/geoprojection.cpp
    src/logging.cpp
    src/osenc.cpp
    src/platform.cpp
    src/region.cpp
    src/s52names.cpp
    src/s57chartoutline.cpp
    src/s57object.cpp
    src/sqlitedatabase.cpp
    src/types.cpp
    ${dbus_adaptor_SRCS}
)


target_include_directories(qutenav
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/geographiclib/src
    ${CMAKE_CURRENT_SOURCE_DIR}/geos/src
    ${CMAKE_CURRENT_SOURCE_DIR}/triangulate/src
    ${CMAKE_BINARY_DIR}
)

target_include_directories(qutenav_dbupdater
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/dbupdater/src
    ${CMAKE_CURRENT_SOURCE_DIR}/geographiclib/src
    ${CMAKE_CURRENT_SOURCE_DIR}/triangulate/src
    ${CMAKE_BINARY_DIR}
)


target_compile_features(qutenav
  PRIVATE
    cxx_std_17
)

target_compile_features(qutenav_dbupdater
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
add_subdirectory(platform.${PLATFORM})
add_subdirectory(geos)


target_link_libraries(qutenav
  PRIVATE
    ${PLATFORM_LDFLAGS}
    GeographicLib
    Triangulate
    Platform
    Osencreader
    Oesencreader
    Cm93reader
    S57reader
    Geos
    Qt5::Quick
    Qt5::Gui
    Qt5::Sql
    Qt5::DBus
    Freetype::Freetype
    fontconfig
    harfbuzz::harfbuzz
    ${PLATFORM_LIBS}
)


target_link_libraries(qutenav_dbupdater
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

install(TARGETS qutenav DESTINATION bin)
install(TARGETS qutenav_dbupdater DESTINATION bin)

