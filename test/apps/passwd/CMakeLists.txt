file(GLOB PY_TEST_FILES passwd-*.py)

set(EXCLUDE_LIST
    # none
)

set(PORT_REQ_1_LIST
	passwd-changes
)

foreach(PY_TEST_FILE ${PY_TEST_FILES})
    get_filename_component(PY_TEST_NAME ${PY_TEST_FILE} NAME_WE)
    if(${PY_TEST_NAME} IN_LIST EXCLUDE_LIST)
        continue()
    endif()
    add_test(NAME apps-${PY_TEST_NAME}
        COMMAND ${PY_TEST_FILE}
    )
	mosq_set_test_props("apps" ${PY_TEST_NAME} ${PORT_REQ_1_LIST} "" "" "")
endforeach()
