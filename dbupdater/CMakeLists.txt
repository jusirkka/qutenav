add_library(Dbupdater STATIC)

set_target_properties(Dbupdater
  PROPERTIES
    AUTOMOC ON
)

target_sources(Dbupdater
  PRIVATE
    src/updater.cpp
)

target_include_directories(Dbupdater
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../qutenavlib/src
    ${CMAKE_BINARY_DIR}
)

target_compile_features(Dbupdater
  PRIVATE
    cxx_std_17
)

target_link_libraries(Dbupdater
  PRIVATE
    Qt5::Core
    Qt5::DBus
    Qt5::Sql
    Qt5::OpenGL
)

