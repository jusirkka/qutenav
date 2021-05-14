add_library(Osencreader STATIC)

set_target_properties(Osencreader
  PROPERTIES
    AUTOMOC ON
)

target_sources(Osencreader
  PRIVATE
    src/osencreader.cpp
)

target_include_directories(Osencreader
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../qutenavlib/src
)


target_compile_features(Osencreader
  PRIVATE
    cxx_std_17
)

target_compile_definitions(Osencreader
  PRIVATE
    QT_STATICPLUGIN
)

target_link_libraries(Osencreader
  PRIVATE
    Qt5::OpenGL
)
