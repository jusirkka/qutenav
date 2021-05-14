add_library(Oesencreader STATIC)

set_target_properties(Oesencreader
  PROPERTIES
    AUTOMOC ON
)

target_sources(Oesencreader
  PRIVATE
    src/oedevice.cpp
    src/oesencreader.cpp
)

target_include_directories(Oesencreader
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../qutenavlib/src
)


target_compile_features(Oesencreader
  PRIVATE
    cxx_std_17
)

target_compile_definitions(Oesencreader
  PRIVATE
    QT_STATICPLUGIN
)

target_link_libraries(Oesencreader
  PRIVATE
    Qt5::OpenGL
)
