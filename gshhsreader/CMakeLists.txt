add_library(GSHHSreader STATIC)

set_target_properties(GSHHSreader
  PROPERTIES
    AUTOMOC ON
)

target_sources(GSHHSreader
  PRIVATE
    src/shapereader.cpp
    src/gshhsreader.cpp
)

target_include_directories(GSHHSreader
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../qutenavlib/src
)


target_compile_features(GSHHSreader
  PRIVATE
    cxx_std_17
)

target_compile_definitions(GSHHSreader
  PRIVATE
    QT_STATICPLUGIN
)

target_link_libraries(GSHHSreader
  PRIVATE
    Qt5::OpenGL
)
