add_library(Triangulate STATIC)

target_sources(Triangulate
  PRIVATE
    src/triangulator.cpp
    src/earcuttessellator.cpp
)


target_include_directories(Triangulate
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../qutenavlib/src
)

target_link_libraries(Triangulate
  PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::OpenGL
)

target_compile_features(Triangulate
  PRIVATE
    cxx_std_17
)

