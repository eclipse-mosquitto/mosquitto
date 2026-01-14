file(GLOB PY_TEST_FILES db-dump-*.py)

set(EXCLUDE_LIST
    # none
)

add_python_tests_from_current_directory2("apps" "${PY_TEST_FILES}" "${EXCLUDE_LIST}")