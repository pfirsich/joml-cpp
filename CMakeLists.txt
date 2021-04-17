cmake_minimum_required(VERSION 3.0.0)

project(joml)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

include(cmake/wall.cmake)

if (JOML_ENABLE_ASAN)
  include(cmake/asan.cmake)
endif()

set(SRC
  main.cpp
  joml.cpp
)
list(TRANSFORM SRC PREPEND src/)

add_executable(joml ${SRC})
target_include_directories(joml PUBLIC include)
set_wall(joml)
