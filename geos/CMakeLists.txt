add_library(Geos STATIC)


target_sources(Geos
  PRIVATE
    src/LineSegment.cpp
    src/LineString.cpp
    src/Distance.cpp
)

target_include_directories(Geos
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)


target_compile_features(Geos
  PRIVATE
    cxx_std_17
)

target_link_libraries(Geos
  PRIVATE
    Qt5::Core
)
