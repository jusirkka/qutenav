add_library(GeographicLib STATIC)


target_sources(GeographicLib
  PRIVATE
    src/Geodesic.cpp
    src/GeodesicLine.cpp
    src/Math.cpp
)


target_include_directories(GeographicLib
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)


target_compile_features(GeographicLib
  PRIVATE
    cxx_std_17
)

target_compile_options(GeographicLib
  PRIVATE
    -fPIC
)

target_compile_definitions(GeographicLib
  PRIVATE
    GEOGRAPHICLIB_GEODESIC_ORDER=4
)

