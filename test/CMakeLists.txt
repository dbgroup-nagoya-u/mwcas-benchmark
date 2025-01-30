#------------------------------------------------------------------------------#
# Configure GoogleTest
#------------------------------------------------------------------------------#

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY "https://github.com/google/googletest.git"
  GIT_TAG "58d77fa8070e8cec2dc1ed015d66b454c8d78850" # v1.12.1
)
FetchContent_MakeAvailable(googletest)

#------------------------------------------------------------------------------#
# Build Unit Tests
#------------------------------------------------------------------------------#

  # define function to add unit tests in the same format
function(ADD_MWCAS_BENCH_TEST MWCAS_BENCH_TEST_TARGET)
  add_executable(${MWCAS_BENCH_TEST_TARGET}
    "${CMAKE_CURRENT_SOURCE_DIR}/${MWCAS_BENCH_TEST_TARGET}.cpp"
  )
  target_compile_options(${MWCAS_BENCH_TEST_TARGET} PRIVATE
    -Wall
    -Wextra
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Release">:"-O2 -march=native">
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},"RelWithDebInfo">:"-g3 -Og -pg">
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Debug">:"-g3 -O0 -pg">
  )
  target_link_libraries(${MWCAS_BENCH_TEST_TARGET} PRIVATE
    benchmarker
    gtest_main
  )
  add_test(NAME ${MWCAS_BENCH_TEST_TARGET} COMMAND $<TARGET_FILE:${MWCAS_BENCH_TEST_TARGET}>)
endfunction()

# add unit tests to build targets
ADD_MWCAS_BENCH_TEST("operation_engine_test")
