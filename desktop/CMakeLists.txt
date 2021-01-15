add_library(Platform OBJECT)

set_target_properties(Platform
  PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)

target_sources(Platform
  PRIVATE
    shaders.qrc
    files.qrc
    src/main.cpp
    src/mainwindow.cpp
    src/settings.cpp
)

kconfig_add_kcfg_files(Platform
  src/conf_mainwindow.kcfgc
  src/conf_detailmode.kcfgc
  src/conf_marinerparams.kcfgc
)

target_include_directories(Platform
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_BINARY_DIR}/desktop
)

target_link_libraries(Platform
  PRIVATE
    KF5::I18n
    KF5::XmlGui
    Qt5::Sql
    Freetype::Freetype
    Fontconfig::Fontconfig
    harfbuzz::harfbuzz
)

target_compile_features(Platform
  PRIVATE
    cxx_std_17
)

