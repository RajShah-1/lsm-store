file(GLOB_RECURSE SOURCES *.cpp *.hpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -Wpedantic")
add_library(geokv STATIC ${SOURCES})

add_executable(geokv_cli main.cpp)
target_link_libraries(geokv_cli geokv)
