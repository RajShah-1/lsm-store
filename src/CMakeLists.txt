file(GLOB_RECURSE SOURCES *.cpp *.hpp)

add_library(geokv STATIC ${SOURCES})

add_executable(geokv_cli main.cpp)
target_link_libraries(geokv_cli geokv)
