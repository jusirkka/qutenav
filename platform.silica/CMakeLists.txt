add_library(Platform STATIC)


target_sources(Platform
  PRIVATE
    src/main.cpp
)

target_include_directories(Platform
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../geos/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../qutenavlib/src
    ${SAILFISH_INCLUDE_DIRS}
    $ENV{EMU_PATH}/include/sailfishapp
)


target_compile_features(Platform
  PRIVATE
    cxx_std_17
)

target_link_libraries(Platform
  PRIVATE
    ${SAILFISH_LDFLAGS}
    Qt5::Core
    Qt5::Quick
    Qt5::Sql
    Qt5::Positioning
    Freetype::Freetype
)
