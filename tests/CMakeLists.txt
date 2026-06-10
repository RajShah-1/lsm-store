find_package(GTest REQUIRED)

add_executable(geokv_tests test_geokv.cpp)
target_link_libraries(geokv_tests geokv GTest::gtest_main)
target_compile_options(geokv_tests PRIVATE -Wall -Wextra -Wpedantic)

get_target_property(GTEST_MAIN_LOCATION GTest::gtest_main IMPORTED_LOCATION)
if(NOT GTEST_MAIN_LOCATION)
  get_target_property(GTEST_MAIN_LOCATION GTest::gtest_main IMPORTED_LOCATION_RELEASE)
endif()
if(GTEST_MAIN_LOCATION)
  get_filename_component(GTEST_LIB_DIR "${GTEST_MAIN_LOCATION}" DIRECTORY)
  set_target_properties(geokv_tests PROPERTIES BUILD_RPATH "${GTEST_LIB_DIR}")
endif()

include(GoogleTest)
gtest_discover_tests(geokv_tests)
