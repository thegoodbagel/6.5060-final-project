cmake_minimum_required(VERSION 3.16)
project(partition_learning LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Optional OpenMP: works on Linux GCC, and on macOS if libomp is installed
find_package(OpenMP)

add_library(plcore INTERFACE)
target_include_directories(plcore INTERFACE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(plcore INTERFACE -Wall -Wextra)
if(OpenMP_CXX_FOUND)
  target_link_libraries(plcore INTERFACE OpenMP::OpenMP_CXX)
  target_compile_definitions(plcore INTERFACE PL_HAVE_OPENMP=1)
  message(STATUS "OpenMP enabled")
else()
  message(STATUS "OpenMP not found: building single-threaded")
endif()

add_executable(partition_experiment src/main.cpp)
target_link_libraries(partition_experiment PRIVATE plcore)

# Tests (Catch2 fetched automatically so there's nothing to install by hand)
option(PL_BUILD_TESTS "Build tests" ON)
if(PL_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2)
  FetchContent_MakeAvailable(Catch2)
  enable_testing()
  file(GLOB TEST_SOURCES tests/*.cpp)
  if(TEST_SOURCES)
    add_executable(unit_tests ${TEST_SOURCES})
    target_link_libraries(unit_tests PRIVATE plcore Catch2::Catch2WithMain)
    add_test(NAME unit_tests COMMAND unit_tests)
  endif()
endif()
