add_library(Mobile STATIC)

set_target_properties(Mobile
  PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)

target_sources(Mobile
  PRIVATE
    shaders.qrc
    src/chartdisplay.cpp
    src/chartrenderer.cpp
    src/configgroup.cpp
    src/conf_detailmode.cpp
    src/conf_mainwindow.cpp
    src/conf_marinerparams.cpp
    src/crosshairs.cpp
    src/databasemodel.cpp
    src/main.cpp
    src/routedatabase.cpp
    src/router.cpp
    src/routetracker.cpp
    src/routemodel.cpp
    src/trackdatabase.cpp
    src/tracker.cpp
    src/trackmodel.cpp
)

target_include_directories(Mobile
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../geos/src
    ${SAILFISH_INCLUDE_DIRS}
    $ENV{EMU_PATH}/include/sailfishapp
)


target_compile_features(Mobile
  PRIVATE
    cxx_std_17
)

target_link_libraries(Mobile
  PRIVATE
    ${SAILFISH_LDFLAGS}
    Qt5::Quick
    Qt5::Gui
    Qt5::Sql
    Freetype::Freetype
)

add_custom_target(Qml SOURCES
  qml/qopencpn.qml
  qml/Boat.qml
  qml/Bubble.qml
  qml/ChartPage.qml
  qml/ChartSetCombo.qml
  qml/CenterButton.qml
  qml/CoverPage.qml
  qml/DimensionalValue.qml
  qml/EditButton.qml
  qml/DistanceButton.qml
  qml/IconListItem.qml
  qml/InfoPage.qml
  qml/MenuButton.qml
  qml/MenuPage.qml
  qml/PreferencesPage.qml
  qml/RouteButton.qml
  qml/RouteDisplayDialog.qml
  qml/RoutePoint.qml
  qml/RoutingStartDialog.qml
  qml/RoutingResultDialog.qml
  qml/ScaleBar.qml
  qml/SliderPref.qml
  qml/TrackButton.qml
  qml/TrackDisplayDialog.qml
  qml/TrackInfo.qml
  qml/TrackInfoBox.qml
  qml/TrackPointInfoBox.qml
  qml/TrackSpeedInfoBox.qml
  qml/TrackTargetInfoBox.qml
  qml/TrackResultDialog.qml
  qml/ValuePref.qml
)
