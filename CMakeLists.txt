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
  Positioning
  OpenGL
  DBus
  LinguistTools
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
    qml/ChartDialog.qml
    qml/ChartFolderSelector.qml
    qml/ChartPage.qml
    qml/ChartPreferencesPage.qml
    qml/CoverPage.qml
    qml/DimensionalValue.qml
    qml/DistanceButton.qml
    qml/EditButton.qml
    qml/InfoPage.qml
    qml/LinkDetailItem.qml
    qml/MapLabel.qml
    qml/MenuButton.qml
    qml/MenuPage.qml
    qml/ObjectInfoBubble.qml
    qml/Pinpoint.qml
    qml/PreferencesPage.qml
    qml/Renamer.qml
    qml/RouteButton.qml
    qml/RouteDisplayDialog.qml
    qml/RoutePoint.qml
    qml/RoutingResultDialog.qml
    qml/RoutingStartDialog.qml
    qml/Ruler.qml
    qml/ScaleBar.qml
    qml/SystemPreferencesPage.qml
    qml/SimpleBubble.qml
    qml/TrackButton.qml
    qml/TrackDisplayDialog.qml
    qml/TrackInfo.qml
    qml/TrackInfoBox.qml
    qml/TrackPointInfoBox.qml
    qml/TrackResultDialog.qml
    qml/TrackSpeedInfoBox.qml
    qml/TrackStatisticsPage.qml
    qml/TrackTargetInfoBox.qml
    qml/UnitPreferencesPage.qml
    qml/ValuePref.qml
    qml/platform.silica/ApplicationWindowPL.qml
    qml/platform.silica/ButtonPL.qml
    qml/platform.silica/BusyIndicatorPL.qml
    qml/platform.silica/ChartPagePL.qml
    qml/platform.silica/ComboBoxPL.qml
    qml/platform.silica/ContextMenuItemPL.qml
    qml/platform.silica/ContextMenuPL.qml
    qml/platform.silica/DetailItemPL.qml
    qml/platform.silica/DialogPL.qml
    qml/platform.silica/FolderPickerPL.qml
    qml/platform.silica/IconListItemPL.qml
    qml/platform.silica/LabelPL.qml
    qml/platform.silica/LinkAreaPL.qml
    qml/platform.silica/ListItemPL.qml
    qml/platform.silica/ListViewPL.qml
    qml/platform.silica/MapButtonPL.qml
    qml/platform.silica/PagePL.qml
    qml/platform.silica/SectionHeaderPL.qml
    qml/platform.silica/SwitchPL.qml
    qml/platform.silica/TextFieldPL.qml
    qml/platform.silica/TextSwitchPL.qml
    qml/platform.silica/ThemePL.qml
    qml/platform.silica/ViewPlaceholderPL.qml
    qml/qutenav.qml
  )

endif ()

add_custom_target(Doc SOURCES
  TODO
  README.md
)


set(BISON_MIN_VERSION "3.1")
find_package(BISON ${BISON_MIN_VERSION} REQUIRED)

set(FLEX_MIN_VERSION "2.6.1")
find_package(FLEX ${FLEX_MIN_VERSION} REQUIRED)

set(FREETYPE_MIN_VERSION "2.10.0")
find_package(Freetype ${FREETYPE_MIN_VERSION} REQUIRED)

find_package(harfbuzz REQUIRED)

#
# translations
#

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(MyAddTranslation)

# -- Update:
# lupdate -locations relative -target-language en_US . -ts translations/id_en.ts
# -- and/or
# ./s57totr.py translations/id_en.ts

my_add_translation(QUTENAV_QMS translations/id_en.ts
  OPTIONS -idbased
)


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
    src/chartindicator.cpp
    src/chartmanager.cpp
    src/chartmode.cpp
    src/chartpainter.cpp
    src/chartproxy.cpp
    src/chartupdater.cpp
    src/dbupdater_interface.cpp
    src/decompose.cpp
    src/detailmode.cpp
    src/drawable.cpp
    src/globe.cpp
    src/glyphmanager.cpp
    src/hpglparser.cpp
    src/hpglopenglparser.cpp
    src/hpglpixmapparser.cpp
    src/linecalculator.cpp
    src/orthocam.cpp
    src/outlinemode.cpp
    src/outliner.cpp
    src/perscam.cpp
    src/rastersymbolmanager.cpp
    src/s52functions.cpp
    src/s52presentation.cpp
    src/s52presentation_p.cpp
    src/s57imageprovider.cpp
    src/s57chart.cpp
    src/s57paintdata.cpp
    src/settings.cpp
    src/shader.cpp
    src/slotcounter.cpp
    src/symboldata.cpp
    src/textmanager.cpp
    src/tiny_sdf.cpp
    src/units.cpp
    src/utils.cpp
    src/vectorsymbolmanager.cpp
    src/wfreader.cpp
    src/chartdisplay.cpp
    src/chartrenderer.cpp
    src/configgroup.cpp
    src/conf_detailmode.cpp
    src/conf_quick.cpp
    src/conf_mainwindow.cpp
    src/conf_marinerparams.cpp
    src/conf_units.cpp
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
    ${QUTENAV_QMS}
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


# automoc generates duplicate mocs for dbus adaptor files in sfos-sdk - dunno why
if (PLATFORM STREQUAL silica)
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
  dbupdater/src/dbupdater.xml
  updater.h
  Updater
  dbupdater_adaptor
)


target_sources(qutenav_dbupdater
  PRIVATE
    dbupdater/src/main.cpp
    ${dbus_adaptor_SRCS}
)


target_include_directories(qutenav
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/geographiclib/src
    ${CMAKE_CURRENT_SOURCE_DIR}/geos/src
    ${CMAKE_CURRENT_SOURCE_DIR}/triangulate/src
    ${CMAKE_CURRENT_SOURCE_DIR}/qutenavlib/src
    ${CMAKE_BINARY_DIR}
)

target_include_directories(qutenav_dbupdater
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dbupdater/src
    ${CMAKE_CURRENT_SOURCE_DIR}/qutenavlib/src
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
add_subdirectory(oesureader)
add_subdirectory(cm93reader)
add_subdirectory(s57reader)
add_subdirectory(gshhsreader)
add_subdirectory(platform.${PLATFORM})
add_subdirectory(geos)
add_subdirectory(qutenavlib)
add_subdirectory(dbupdater)


target_link_libraries(qutenav
  PRIVATE
    ${PLATFORM_LDFLAGS}
    Platform
    Osencreader
    Oesencreader
    Oesureader
    Cm93reader
    S57reader
    GSHHSreader
    Geos
    QuteNavLib
    GeographicLib
    Triangulate
    Qt5::Quick
    Qt5::Gui
    Qt5::Sql
    Qt5::Positioning
    Qt5::DBus
    Freetype::Freetype
    fontconfig
    harfbuzz::harfbuzz
    ${PLATFORM_LIBS}
)


target_link_libraries(qutenav_dbupdater
  PRIVATE
    Dbupdater
    Osencreader
    Oesencreader
    Oesureader
    Cm93reader
    S57reader
    QuteNavLib
    Triangulate
    GeographicLib
    Qt5::Core
    Qt5::Sql
    Qt5::OpenGL
)


include(GNUInstallDirs)

# binaries
install(TARGETS qutenav
        DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS qutenav_dbupdater
        DESTINATION ${CMAKE_INSTALL_BINDIR})
# desktop file
install(FILES data/qutenav.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
# icons
install(FILES data/qutenav-48x48.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps
        RENAME qutenav.png)
install(FILES data/qutenav-64x64.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps
        RENAME qutenav.png)

# wavefront file & world map
install(FILES data/sphere.obj
              data/negx.jpg data/negy.jpg data/negz.jpg
              data/posx.jpg data/posy.jpg data/posz.jpg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/qutenav/globe)

# s57 data files
install(FILES data/s57objectclasses.csv
              data/s57attributes.csv
              data/s57expectedinput.csv
              data/chartsymbols.xml
              data/rastersymbols-dark.png
              data/rastersymbols-day.png
              data/rastersymbols-dusk.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/qutenav/s57data)

# translations
install(FILES ${QUTENAV_QMS}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/qutenav/translations)
