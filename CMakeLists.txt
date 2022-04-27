cmake_minimum_required(VERSION 3.5)

project(kvengine)

set(CMAKE_CXX_FLAGS "-std=c++11 -ggdb -Werror -Wall -Wno-unused")

set(SRC 
  core.cpp
  dlist.cpp
  str.cpp
  valueobject.cpp
)

add_executable(test_core test/test_core.cpp ${SRC})
add_executable(test_core2 test/test_core2.cpp ${SRC})
add_executable(test_dlist test/test_dlist.cpp ${SRC})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)