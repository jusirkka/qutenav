add_library(Oesureader STATIC)

set_target_properties(Oesureader
  PROPERTIES
    AUTOMOC ON
)

target_sources(Oesureader
  PRIVATE
    src/oesureader.cpp
)

target_include_directories(Oesureader
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../qutenavlib/src
)


target_compile_features(Oesureader
  PRIVATE
    cxx_std_17
)

target_compile_definitions(Oesureader
  PRIVATE
    QT_STATICPLUGIN
)

target_link_libraries(Oesureader
  PRIVATE
    Qt5::OpenGL
)
