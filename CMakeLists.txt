cmake_minimum_required(VERSION 3.5)

project(LiteKV)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-std=c++11 -ggdb -Wall -Wno-unused")
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

add_executable_and_link(generate_pseudo_data "tools/generate_pseudo_data.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(shrink_dumpfile "tools/shrink_dumpfile.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(litekv-benchmark "tools/litekv-benchmark.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(kvmain "src/main.cpp" "${SRC}" "${LIBS}")

if (BUILD_TESTS)
  add_subdirectory(test)
endif(BUILD_TESTS)

# benchmark test
if (BUILD_BENCHMARKS)
  add_subdirectory(benchmark)
endif(BUILD_BENCHMARKS)

if (TCMALLOC_LIB)
  target_compile_options(shrink_dumpfile PRIVATE -O2 -DTCMALLOC_FOUND)
  target_link_libraries(shrink_dumpfile tcmalloc)
  target_compile_options(generate_pseudo_data PRIVATE -O2 -DTCMALLOC_FOUND)
  target_link_libraries(generate_pseudo_data tcmalloc)
  target_compile_options(kvmain PRIVATE -O2 -DTCMALLOC_FOUND)
  target_link_libraries(kvmain tcmalloc)
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)