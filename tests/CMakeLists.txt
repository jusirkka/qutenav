cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

enable_testing()

project(qutenav_tests LANGUAGES CXX)


#
# dependencies
#

set(QT_MIN_VERSION "5.6.3")


find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS
  Test
  OpenGL
)

#
# targets
#

add_executable(test_region)
add_test(NAME test_region COMMAND test_region)


set_target_properties(test_region
  PROPERTIES
    AUTOMOC ON
)

target_sources(test_region
  PRIVATE
    src/test_region.cpp
    ../src/region.cpp
)


target_include_directories(test_region
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_compile_features(test_region
  PRIVATE
    cxx_std_17
)

target_link_libraries(test_region
  PRIVATE
    Qt5::Test
    Qt5::OpenGL
)



