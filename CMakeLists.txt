cmake_minimum_required(VERSION 3.5)

project(SimpleKV)
set(CMAKE_CXX_FLAGS "-std=c++11 -ggdb -Wall -Wno-unused")
set(SRC
    src/core.cpp
    src/dlist.cpp
    src/str.cpp
    src/dict.cpp
    src/valueobject.cpp
    )

set(NET_SRC
    src/net/addr.cpp
    src/net/net.cpp
    src/net/server.cpp
    src/net/utils.cpp
    src/net/buffer.cpp
    src/net/commands.cpp
    src/net/time_event.cpp
    )

add_executable(test_core test/test_core.cpp ${SRC})
add_executable(test_core2 test/test_core2.cpp ${SRC})
add_executable(test_core3 test/test_core3.cpp ${SRC})
add_executable(test_dlist test/test_dlist.cpp ${SRC})
add_executable(test_dlist2 test/test_dlist2.cpp ${SRC})
add_executable(test_dynamic_string test/test_dynamic_string.cpp ${SRC})
add_executable(test_dict test/test_dict.cpp ${SRC})
add_executable(test_static_string test/test_static_string.cpp ${SRC})
add_executable(test_time_event test/test_time_event.cpp ${SRC} ${NET_SRC})

add_executable(test_server test/test_server.cpp ${SRC} ${NET_SRC})

add_executable(benchmark_dict benchmark/benchmark_dict.cpp ${SRC})
set_target_properties(benchmark_dict PROPERTIES COMPILE_FLAGS "-O2 -DTCMALLOC_FOUND")
target_link_libraries(benchmark_dict tcmalloc)

add_executable(benchmark_int benchmark/benchmark_int.cpp ${SRC})
set_target_properties(benchmark_int PROPERTIES COMPILE_FLAGS "-O2 -DTCMALLOC_FOUND")
target_link_libraries(benchmark_int tcmalloc)

add_executable(benchmark_string benchmark/benchmark_string.cpp ${SRC})
set_target_properties(benchmark_string PROPERTIES COMPILE_FLAGS "-O2 -DTCMALLOC_FOUND")
target_link_libraries(benchmark_string tcmalloc)

add_executable(benchmark_list benchmark/benchmark_list.cpp ${SRC})
set_target_properties(benchmark_list PROPERTIES COMPILE_FLAGS "-O2 -DTCMALLOC_FOUND")
target_link_libraries(benchmark_list tcmalloc)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)