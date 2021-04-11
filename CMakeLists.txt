cmake_minimum_required(VERSION "3.8")

# decline in-source build
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# project information
project(cpp_template
  VERSION "0.1.0"
  DESCRIPTION "A template project for C++ language."
  LANGUAGES "CXX"
)

# define source directories
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(MWCAS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/mwcas")
set(PMWCAS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/pmwcas-microsoft")

# add MwCAS as a benchmark target
add_subdirectory("${MWCAS_SOURCE_DIR}")

# add PMwCAS as a benchmark target
set(PMEM_BACKEND "Volatile" CACHE STRING "" FORCE)
set(GOOGLE_FRAMEWORK OFF CACHE BOOL "" FORCE)
set(BUILD_APPS OFF CACHE BOOL "" FORCE)
add_subdirectory("${PMWCAS_SOURCE_DIR}")

#--------------------------------------------------------------------------------------#
# Configure gflags
#--------------------------------------------------------------------------------------#
configure_file(external/CMakeLists-gflags.txt.in gflags-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gflags-download )
if(result)
  message(FATAL_ERROR "CMake step for gflags failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gflags-download )
if(result)
  message(FATAL_ERROR "Build step for gflags failed: ${result}")
endif()

# Prevent building unit tests
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

# Add gflags directly to our build
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/gflags-src
                 ${CMAKE_CURRENT_BINARY_DIR}/gflags-build
                 EXCLUDE_FROM_ALL)

#--------------------------------------------------------------------------------------#
# Configure glog
#--------------------------------------------------------------------------------------#
configure_file(external/CMakeLists-glog.txt.in glog-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/glog-download )
if(result)
  message(FATAL_ERROR "CMake step for glog failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/glog-download )
if(result)
  message(FATAL_ERROR "Build step for glog failed: ${result}")
endif()

# Prevent building gflags
set(WITH_GFLAGS OFF CACHE BOOL "" FORCE)

# Prevent building GoogleTest
set(WITH_GTEST OFF CACHE BOOL "" FORCE)

# Prevent building unit tests
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

# Add glog directly to our build
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/glog-src
                 ${CMAKE_CURRENT_BINARY_DIR}/glog-build
                 EXCLUDE_FROM_ALL)

#--------------------------------------------------------------------------------------#
# Build Benchmark
#--------------------------------------------------------------------------------------#

# build executable
add_executable(cpp_template
  "${PROJECT_SOURCE_DIR}/main.cpp"
)
target_compile_features(cpp_template PRIVATE "cxx_std_17")
target_compile_options(cpp_template PRIVATE
  -Wall
  -Wextra
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Release">:"-O2 -march=native">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"RelWithDebInfo">:"-g3 -Og -pg">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Debug">:"-g3 -O0 -pg">
)
target_include_directories(cpp_template PRIVATE
  "${PMWCAS_SOURCE_DIR}/"
  "${PMWCAS_SOURCE_DIR}/src"
  "${PMWCAS_SOURCE_DIR}/include"
)
target_link_libraries(cpp_template PRIVATE
  mwcas
  pmwcas_static
  glog
  gflags
  rt
)
add_dependencies(cpp_template
  mwcas
  pmwcas_static
)

# build unit tests if required
option(BUILD_TESTS, "Build unit tests" OFF)
if(${BUILD_TESTS})
  # enable CTest
  enable_testing()
  # include unit tests
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")
endif()
