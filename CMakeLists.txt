cmake_minimum_required(VERSION 3.21)

project(checktex)

add_library(libchecktex
checktex.hpp
checktex.cpp)

option(CHECKTEX_TESTS "Enable tests" OFF)

if(CHECKTEX_TESTS)
  add_executable(checktextests
  tests.cpp)
  target_include_directories(checktextests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(checktextests libchecktex)
endif()
