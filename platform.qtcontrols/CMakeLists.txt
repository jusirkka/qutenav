add_library(Platform STATIC)

set_target_properties(Platform
  PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)

target_sources(Platform
  PRIVATE
    src/main.cpp
)

target_include_directories(Platform
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../geos/src
)


target_compile_features(Platform
  PRIVATE
    cxx_std_17
)

target_link_libraries(Platform
  PRIVATE
    Qt5::Core
    Qt5::Quick
    Qt5::Sql
    Freetype::Freetype
)

