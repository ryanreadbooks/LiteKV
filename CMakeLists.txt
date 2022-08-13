cmake_minimum_required(VERSION 3.5)

project(LiteKV)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-std=c++11 -ggdb -Wall -Wno-unused")
set(SRC
    src/core.cpp
    src/dlist.cpp
    src/str.cpp
    src/hash.cpp
    src/hashdict.cpp
    src/hashset.cpp
    src/rehashable.cpp
    src/valueobject.cpp
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

SET(LIBS
    pthread)

function(add_executable_and_link target src depend libs)
  add_executable(${target} ${src} ${depend})
  target_link_libraries(${target} ${libs})
endfunction(add_executable_and_link)

find_library(TCMALLOC_LIB NAMES tcmalloc)
if(TCMALLOC_LIB)
  message(STATUS "Found TCMALLOC_LIB: ${TCMALLOC_LIB}")
else()
  message(STATUS "TCMALLOC_LIB library not found, use default malloc instead")
endif()

add_executable_and_link(test_core "test/test_core.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(test_core2 "test/test_core2.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(test_core3 "test/test_core3.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(test_dlist "test/test_dlist.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(test_dlist2 "test/test_dlist2.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(test_dynamic_string "test/test_dynamic_string.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(test_dict "test/test_dict.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(test_static_string "test/test_static_string.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(test_time_event "test/test_time_event.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(test_appendable_file "test/test_appendable_file.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(test_config "test/test_config.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(test_protocol "test/test_protocol.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(test_buffer "test/test_buffer.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(generate_pseudo_data "tools/generate_pseudo_data.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(shrink_dumpfile "tools/shrink_dumpfile.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(litekv-benchmark "tools/litekv-benchmark.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(kvmain "src/main.cpp" "${SRC}" "${LIBS}")

# benchmark test
add_executable_and_link(benchmark_dict "benchmark/benchmark_dict.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(benchmark_int "benchmark/benchmark_int.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(benchmark_string "benchmark/benchmark_string.cpp" "${SRC}" "${LIBS}")
add_executable_and_link(benchmark_list "benchmark/benchmark_list.cpp" "${SRC}" "${LIBS}")

if (TCMALLOC_LIB)
  target_compile_options(shrink_dumpfile PRIVATE -O2 -DTCMALLOC_FOUND)
  target_link_libraries(shrink_dumpfile tcmalloc)
  target_compile_options(generate_pseudo_data PRIVATE -O2 -DTCMALLOC_FOUND)
  target_link_libraries(generate_pseudo_data tcmalloc)
  target_compile_options(kvmain PRIVATE -O2 -DTCMALLOC_FOUND)
  target_link_libraries(kvmain tcmalloc)
  target_compile_options(benchmark_int PRIVATE -O2 -DTCMALLOC_FOUND)
  target_link_libraries(benchmark_int tcmalloc)
  target_compile_options(benchmark_string PRIVATE -O2 -DTCMALLOC_FOUND)
  target_link_libraries(benchmark_string tcmalloc)
  target_compile_options(benchmark_list PRIVATE -O2 -DTCMALLOC_FOUND)
  target_link_libraries(benchmark_list tcmalloc)
  target_compile_options(benchmark_dict PRIVATE -O2 -DTCMALLOC_FOUND)
  target_link_libraries(benchmark_dict tcmalloc)
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)