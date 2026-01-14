add_subdirectory(c)
add_subdirectory(cpp)

file(GLOB PY_TEST_FILES [0-9][0-9]-*.py)
list(APPEND PY_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/msg_sequence_test.py")

set(EXCLUDE_LIST
    03-publish-c2b-qos1-timeout
    03-publish-c2b-qos2-timeout
)

add_python_tests_from_current_directory2("lib" "${PY_TEST_FILES}" "${EXCLUDE_LIST}")