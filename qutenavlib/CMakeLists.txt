add_library(QuteNavLib STATIC)


target_sources(QuteNavLib
  PRIVATE
    src/chartdatabase.cpp
    src/chartfilereader.cpp
    src/geomutils.cpp
    src/geoprojection.cpp
    src/logging.cpp
    src/osenc.cpp
    src/platform.cpp
    src/region.cpp
    src/s52names.cpp
    src/s57chartoutline.cpp
    src/s57object.cpp
    src/sqlitedatabase.cpp
    src/translationmanager.cpp
    src/types.cpp
)


target_include_directories(QuteNavLib
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../triangulate/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../geographiclib/src
)

target_link_libraries(QuteNavLib
  PRIVATE
    Qt5::Sql
    Qt5::OpenGL
)



target_compile_features(QuteNavLib
  PRIVATE
    cxx_std_17
)

