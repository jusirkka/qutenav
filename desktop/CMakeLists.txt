add_library(Desktop OBJECT)

set_target_properties(Desktop
  PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)

target_sources(Desktop
  PRIVATE
    shaders.qrc
    files.qrc
    src/glwindow.cpp
    src/linecalculator.cpp
    src/main.cpp
    src/mainwindow.cpp
)

kconfig_add_kcfg_files(Desktop
  src/conf_mainwindow.kcfgc
  src/conf_detailmode.kcfgc
  src/conf_marinerparams.kcfgc
)

target_include_directories(Desktop
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_BINARY_DIR}/desktop
)

target_link_libraries(Desktop
  PRIVATE
    KF5::I18n
    KF5::XmlGui
    Qt5::Sql
    Freetype::Freetype
    Fontconfig::Fontconfig
    harfbuzz::harfbuzz
)

target_compile_features(Desktop
  PRIVATE
    cxx_std_17
)

