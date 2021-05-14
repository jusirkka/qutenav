add_library(S57reader STATIC)

set_target_properties(S57reader
  PROPERTIES
    AUTOMOC ON
)

target_sources(S57reader
  PRIVATE
    src/record.cpp
    src/s57reader.cpp
)

target_include_directories(S57reader
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../qutenavlib/src
)


target_compile_features(S57reader
  PRIVATE
    cxx_std_17
)

target_compile_definitions(S57reader
  PRIVATE
    QT_STATICPLUGIN
)

target_link_libraries(S57reader
  PRIVATE
    Qt5::OpenGL
)
