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
    src/linecalculator.cpp
    src/main.cpp
)

target_include_directories(Mobile
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
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
    Freetype::Freetype
)

add_custom_target(Qml SOURCES
  qml/qopencpn.qml
  qml/ChartPage.qml
  qml/ChartSetCombo.qml
  qml/CoverPage.qml
  qml/IconListItem.qml
  qml/MenuButton.qml
  qml/MenuPage.qml
)
