file(GLOB PY_TEST_FILES signal-*.py)

set(EXCLUDE_LIST
    # none
)

add_python_tests_from_current_directory2("apps" "${PY_TEST_FILES}" "${EXCLUDE_LIST}")