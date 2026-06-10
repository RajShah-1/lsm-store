add_library(geokv INTERFACE)
target_include_directories(geokv INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(geokv INTERFACE cxx_std_20)

add_compile_options(-Wall -Wextra -Wpedantic)

if(GEOKV_ENABLE_COVERAGE)
  target_compile_options(geokv INTERFACE --coverage -O0 -g)
  target_link_options(geokv INTERFACE --coverage)
endif()

add_executable(geokv_cli main.cpp)
target_link_libraries(geokv_cli geokv)
