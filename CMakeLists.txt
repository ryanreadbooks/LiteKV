cmake_minimum_required(VERSION 3.5)

project(LiteKV)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -ggdb -Wall -Wno-unused")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall -Wno-unused")

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_VERBOSE_MAKEFILE ON)
endif ()

# global cache LITEKV_SOURCE_DIR
set(LITEKV_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
option(BUILD_TESTS "Build test" OFF)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)

# function to add executable and link libraries
function(add_executable_and_link target src depend libs)
  add_executable(${target} ${src} ${depend})
  target_link_libraries(${target} ${libs})
endfunction(add_executable_and_link)

set(SRC
    src/core.cpp
    src/dlist.cpp
    src/str.cpp
    src/valueobject.cpp
    src/hashdict.cpp
    src/hashset.cpp
    src/persistence.cpp
    src/config.cpp
    src/encoding.cpp
    src/net/addr.cpp
    src/net/net.cpp
    src/net/server.cpp
    src/net/utils.cpp
    src/net/buffer.cpp
    src/net/commands.cpp
    src/net/time_event.cpp
    src/net/protocol.cpp
    )

SET(LIBS pthread)

# add prefix to SRC list 
set(LITEKV_SRC "")
foreach(file ${SRC})
  list(APPEND LITEKV_SRC "${LITEKV_SOURCE_DIR}/${file}")
endforeach(file)

find_library(TCMALLOC_LIB NAMES tcmalloc)
if(TCMALLOC_LIB)
  message(STATUS "Found TCMALLOC_LIB: ${TCMALLOC_LIB}")
else()
  message(STATUS "TCMALLOC_LIB library not found, use default malloc instead")
endif()

add_executable_and_link(litekv-gen-pseudo-data "tools/litekv-gen-pseudo-data.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(litekv-rewrite-dump "tools/litekv-rewrite-dump.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(litekv-benchmark "tools/litekv-benchmark.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(kvmain "src/main.cpp" "${SRC}" "${LIBS}")

if (BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif(BUILD_TESTS)

# benchmark test
if (BUILD_BENCHMARKS)
  add_subdirectory(benchmark)
endif(BUILD_BENCHMARKS)

if (TCMALLOC_LIB)
  target_compile_options(litekv-rewrite-dump PRIVATE -DTCMALLOC_FOUND)
  target_link_libraries(litekv-rewrite-dump tcmalloc)
  target_compile_options(litekv-gen-pseudo-data PRIVATE -DTCMALLOC_FOUND)
  target_link_libraries(litekv-gen-pseudo-data tcmalloc)
  target_compile_options(kvmain PRIVATE -DTCMALLOC_FOUND)
  target_link_libraries(kvmain tcmalloc)
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)