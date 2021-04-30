#--------------------------------------------------------------------------------------#
# Configure GoogleTest
#--------------------------------------------------------------------------------------#

set(GTEST_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/googletest")

# Prevent overriding the parent MWCAS_BENCH's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines the gtest and gtest_main targets.
add_subdirectory("${GTEST_SOURCE_DIR}")

#--------------------------------------------------------------------------------------#
# Build Unit Tests
#--------------------------------------------------------------------------------------#

  # define function to add unit tests in the same format
function(ADD_MWCAS_BENCH_TEST MWCAS_BENCH_TEST_TARGET)
  add_executable(${MWCAS_BENCH_TEST_TARGET}
    "${CMAKE_CURRENT_SOURCE_DIR}/${MWCAS_BENCH_TEST_TARGET}.cpp"
  )
  target_compile_features(${MWCAS_BENCH_TEST_TARGET} PRIVATE "cxx_std_17")
  target_compile_options(${MWCAS_BENCH_TEST_TARGET} PRIVATE
    -Wall
    -Wextra
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Release">:"-O2 -march=native">
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},"RelWithDebInfo">:"-g3 -Og -pg">
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Debug">:"-g3 -O0 -pg">
  )
  target_compile_definitions(
    ${MWCAS_BENCH_TEST_TARGET} PRIVATE
    MWCAS_BENCH_MAX_FIELD_NUM=${MWCAS_BENCH_MAX_FIELD_NUM}
  )
  target_include_directories(
    ${MWCAS_BENCH_TEST_TARGET} PRIVATE
    "${MWCAS_BENCH_SOURCE_DIR}"
    "${PMWCAS_SOURCE_DIR}/"
    "${PMWCAS_SOURCE_DIR}/src"
    "${PMWCAS_SOURCE_DIR}/include"
  )
  target_link_libraries(
    ${MWCAS_BENCH_TEST_TARGET} PRIVATE
    mwcas
    pmwcas_static
    gflags
    gtest_main
    rt
    cpp_utility
  )
  add_dependencies(
    ${MWCAS_BENCH_TEST_TARGET}
    mwcas
    pmwcas_static
    gtest_main
  )
  add_test(NAME ${MWCAS_BENCH_TEST_TARGET} COMMAND $<TARGET_FILE:${MWCAS_BENCH_TEST_TARGET}>)
endfunction()

# add unit tests to build targets
ADD_MWCAS_BENCH_TEST("worker_mwcas_test")
ADD_MWCAS_BENCH_TEST("worker_pmwcas_test")
ADD_MWCAS_BENCH_TEST("worker_cas_test")
