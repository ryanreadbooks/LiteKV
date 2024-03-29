cmake_minimum_required(VERSION 3.5)

project(LiteKV)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
message(STATUS ${EXECUTABLE_OUTPUT_PATH})

find_package(GTest REQUIRED)
if (${GTEST_FOUND})
  message(STATUS "GTest found in ${GTEST_MAIN_LIBRARIES}")
endif()

function(add_test_exec name test_name src other_src lib)
  add_executable(${name} ${src} ${other_src})
  target_link_libraries(${name} PRIVATE GTest::GTest GTest::Main ${lib})
  add_test(${test_name} ${EXECUTABLE_OUTPUT_PATH}/${name})
endfunction(add_test_exec)

function(add_test_exec_with_args name test_name src other_src lib extra_args)
  add_executable(${name} ${src} ${other_src})
  target_link_libraries(${name} PRIVATE GTest::GTest GTest::Main ${lib})
  add_test(${test_name} ${EXECUTABLE_OUTPUT_PATH}/${name} ${extra_args})
endfunction(add_test_exec_with_args)

add_test_exec(test_core core1_unittest "test_core.cpp" "${LITEKV_SRC}" "${LIBS}")
add_test_exec(test_dlist dlist_unittest "test_dlist.cpp" "${LITEKV_SRC}" "${LIBS}")
add_test_exec(test_buffer buffer_unittest "test_buffer.cpp" "${LITEKV_SRC}" "${LIBS}")
add_test_exec(test_hashset hashset_unittest "test_hashset.cpp" "${LITEKV_SRC}" "${LIBS}")
add_test_exec(test_protocol protocol_unittest "test_protocol.cpp" "${LITEKV_SRC}" "${LIBS}")
add_test_exec(test_hashdict hashdict_unittest "test_hashdict.cpp" "${LITEKV_SRC}" "${LIBS}")
add_test_exec(test_static_string static_string_unittest "test_static_string.cpp" "${LITEKV_SRC}" "${LIBS}")
add_test_exec(test_dynamic_string dynamic_string_unittest "test_dynamic_string.cpp" "${LITEKV_SRC}" "${LIBS}")
add_test_exec_with_args(test_config config_unittest "test_config.cpp" "${LITEKV_SRC}" "${LIBS}" "${LITEKV_SOURCE_DIR}/conf/litekv.conf")
add_test_exec_with_args(test_appendable_file appendable_file_unittest "test_appendable_file.cpp" "${LITEKV_SRC}" "${LIBS}" "${CMAKE_CURRENT_SOURCE_DIR}/test.aof")
add_test_exec(test_time_event time_event_unittest "test_time_event.cpp" "${LITEKV_SRC}" "${LIBS}")
add_test_exec(test_skiplist skiplist_unittest "test_skiplist.cpp" "${LITEKV_SRC}" "${LIBS}")
add_test_exec(test_serializable serializable_unittest "test_serializable.cpp" "${LITEKV_SRC}" "${LIBS}")
add_test_exec(test_lkvdb lkvdb_unittest "test_lkvdb.cpp" "${LITEKV_SRC}" "${LIBS}")