#------------------------------------------------------------------------------#
# Configure GoogleTest
#------------------------------------------------------------------------------#

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY "https://github.com/google/googletest"
  GIT_TAG "52eb8108c5bdec04579160ae17225d66034bd723" # v1.17.0
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
    $<$<STREQUAL:"${CMAKE_BUILD_TYPE}","Release">:-march=native>
    $<$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">:-g3>
    -Wall
    -Wextra
  )
  target_link_libraries(${MWCAS_BENCH_TEST_TARGET} PRIVATE
    benchmarker
    gtest_main
  )
  add_test(NAME ${MWCAS_BENCH_TEST_TARGET} COMMAND $<TARGET_FILE:${MWCAS_BENCH_TEST_TARGET}>)
endfunction()

# add unit tests to build targets
ADD_MWCAS_BENCH_TEST("operation_engine_test")
