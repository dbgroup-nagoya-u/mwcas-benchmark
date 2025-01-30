cmake_minimum_required(VERSION "3.11")

# decline in-source build
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# project information
project(
  mwcas_bench
  VERSION "0.1.0"
  DESCRIPTION "A benchmark program for MwCAS implementations."
  LANGUAGES "CXX"
)

# enable features
include(FetchContent)

#------------------------------------------------------------------------------#
# Set Compile Options
#------------------------------------------------------------------------------#

set(
  MWCAS_BENCH_MAX_TARGET_NUM
  "8"
  CACHE STRING
  "The maximum number of target words of MwCAS."
)

option(
  MWCAS_BENCH_USE_PMWCAS
  "A flag for using microsoft/pmwcas as a competitor."
  OFF
)

option(
  MWCAS_BENCH_USE_TBBMALLOC
  "A flag for overriding entire memory allocation by Intel oneTBB malloc."
  OFF
)

#------------------------------------------------------------------------------#
# Competitors
#------------------------------------------------------------------------------#

# our MwCAS implementations
set(MWCAS_CAPACITY "${MWCAS_BENCH_MAX_TARGET_NUM}" CACHE STRING "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/mwcas")

# microsoft/pmwcas
if (${MWCAS_BENCH_USE_PMWCAS})
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/microsoft_pmwcas.cmake")
endif()

#------------------------------------------------------------------------------#
# Configurations
#------------------------------------------------------------------------------#

# use command line options
find_package(gflags REQUIRED)

# use an efficient memory allocator
if (${MWCAS_BENCH_USE_TBBMALLOC})
  find_package(TBB REQUIRED)
endif()

# use our utility
FetchContent_Declare(
  cpp-utility
  GIT_REPOSITORY "https://github.com/dbgroup-nagoya-u/cpp-utility.git"
  GIT_TAG "6487d3fe2ab4e70b5ab5cee7742cccf92bb5f072"
)
FetchContent_MakeAvailable(cpp-utility)

# use our benchmark builder
FetchContent_Declare(
    cpp-benchmark
    GIT_REPOSITORY "https://github.com/dbgroup-nagoya-u/cpp-benchmark.git"
    GIT_TAG "bc98d176fe46aab25d91bd058c9d85b9d84c64a4"
)
FetchContent_MakeAvailable(cpp-benchmark)

#------------------------------------------------------------------------------#
# Build Benchmark
#------------------------------------------------------------------------------#

# build as library for testing
add_library(benchmarker STATIC
  "${CMAKE_CURRENT_SOURCE_DIR}/src/operation_engine.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/mwcas_target.cpp"
)
target_compile_options(benchmarker PRIVATE
  -Wall
  -Wextra
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Release">:"-O2 -march=native">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"RelWithDebInfo">:"-g3 -Og -pg">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Debug">:"-g3 -O0 -pg">
)
target_compile_definitions(benchmarker PUBLIC
  $<$<BOOL:${MWCAS_BENCH_USE_PMWCAS}>:MWCAS_BENCH_USE_PMWCAS>
)
target_include_directories(benchmarker PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(benchmarker PUBLIC
  dbgroup::cpp_utility
  dbgroup::cpp_bench
  dbgroup::mwcas
  $<$<BOOL:${MWCAS_BENCH_USE_PMWCAS}>:microsoft::pmwcas>
)

# build executable
add_executable(mwcas_bench
  "${CMAKE_CURRENT_SOURCE_DIR}/src/mwcas_bench.cpp"
)
target_compile_options(mwcas_bench PRIVATE
  -Wall
  -Wextra
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Release">:"-O2 -march=native">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"RelWithDebInfo">:"-g3 -Og -pg">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Debug">:"-g3 -O0 -pg">
)
target_link_libraries(mwcas_bench PRIVATE
  benchmarker
  gflags
  $<$<BOOL:${MWCAS_BENCH_USE_TBBMALLOC}>:tbbmalloc_proxy>
)

#------------------------------------------------------------------------------#
# Build unit tests
#------------------------------------------------------------------------------#

option(MWCAS_BENCH_BUILD_TESTS "Build unit tests" OFF)
if(${MWCAS_BENCH_BUILD_TESTS})
  # enable CTest
  enable_testing()
  # include unit tests
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")
endif()
