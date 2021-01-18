add_library(Triangulate STATIC)

target_sources(Triangulate
  PRIVATE
    src/triangulator.cpp
    src/earcuttessellator.cpp
    src/lib2tessellator.cpp
    src/libtess2/bucketalloc.c
    src/libtess2/dict.c
    src/libtess2/geom.c
    src/libtess2/mesh.c
    src/libtess2/priorityq.c
    src/libtess2/sweep.c
    src/libtess2/tess.c
)


target_include_directories(Triangulate
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(Triangulate
  PRIVATE
    Qt5::Core
    Qt5::OpenGL
)

target_compile_features(Triangulate
  PRIVATE
    cxx_std_17
)

