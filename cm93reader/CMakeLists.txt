add_library(Cm93reader STATIC)

set_target_properties(Cm93reader
  PROPERTIES
    AUTOMOC ON
)

target_sources(Cm93reader
  PRIVATE
    src/cm93presentation.cpp
    src/cm93reader.cpp
)

target_include_directories(Cm93reader
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../qutenavlib/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../triangulate/src
)


target_compile_features(Cm93reader
  PRIVATE
    cxx_std_17
)

target_compile_definitions(Cm93reader
  PRIVATE
    QT_STATICPLUGIN
)

target_link_libraries(Cm93reader
  PRIVATE
    Qt5::OpenGL
)
