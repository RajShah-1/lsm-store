cmake_minimum_required(VERSION 3.14)
project(geokv CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GEOKV_ENABLE_COVERAGE "Build GeoKV with gcov-compatible coverage flags" OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)
enable_testing()
add_subdirectory(tests)

if(GEOKV_ENABLE_COVERAGE)
  find_program(GCOV_EXECUTABLE gcov)
  if(GCOV_EXECUTABLE)
    set(GEOKV_COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)
    add_custom_target(coverage
      COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
      COMMAND ${CMAKE_COMMAND} -E rm -rf ${GEOKV_COVERAGE_DIR}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${GEOKV_COVERAGE_DIR}
      COMMAND ${CMAKE_COMMAND} -E chdir ${GEOKV_COVERAGE_DIR}
              ${GCOV_EXECUTABLE}
              ${CMAKE_BINARY_DIR}/tests/CMakeFiles/geokv_tests.dir/test_geokv.cpp.gcda
      DEPENDS geokv_tests
      COMMENT "Generating gcov reports in ${GEOKV_COVERAGE_DIR}"
      VERBATIM
    )
  else()
    message(WARNING "GEOKV_ENABLE_COVERAGE is ON, but gcov was not found.")
  endif()
endif()
