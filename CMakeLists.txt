cmake_minimum_required(VERSION 3.0.0)

project(joml-cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(cmake/wall.cmake)

if (JOML_ENABLE_ASAN)
  include(cmake/asan.cmake)
endif()

add_library(joml-cpp src/joml.cpp)
target_include_directories(joml-cpp PUBLIC include)
set_wall(joml-cpp)

if (JOML_BUILD_JOML2JSON)
  add_executable(joml2json src/joml2json.cpp)
  target_include_directories(joml2json PUBLIC include)
  target_link_libraries(joml2json joml-cpp)
  set_wall(joml2json)
endif()
