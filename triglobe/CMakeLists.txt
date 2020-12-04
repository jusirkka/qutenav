add_executable(triglobe)

set_target_properties(triglobe
  PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_sources(triglobe
  PRIVATE
    src/main.cpp
    src/sdfunction.cpp
    src/triangulator.cpp
)

target_include_directories(triglobe
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}
)

target_compile_features(triglobe
  PRIVATE
    cxx_std_17
)

target_link_libraries(triglobe
  PRIVATE
    GDAL::GDAL
    Qt5::Core
)


install(TARGETS triglobe)

