file(GLOB PY_TEST_FILES [0-9][0-9]-*.py)

set(EXCLUDE_LIST
    # none
)


set(PORT_REQ_2_LIST
	02-subscribe-qos1-ws
	03-publish-qos1-ws-large
	03-publish-qos1-ws
	03-publish-socks-auth-failed
	03-publish-socks-no-auth
	03-publish-socks
	04-rr-qos1-ws
)

generate_test_names(PY_TEST_NAMES PY_TEST_FILES)

foreach(PY_TEST_FILE ${PY_TEST_FILES})
    get_filename_component(PY_TEST_NAME ${PY_TEST_FILE} NAME_WE)
    if(${PY_TEST_NAME} IN_LIST EXCLUDE_LIST)
        continue()
    endif()
    add_test(NAME client-${PY_TEST_NAME}
        COMMAND ${PY_TEST_FILE}
    )
	mosq_set_test_props("client" ${PY_TEST_NAME} "${PY_TEST_NAMES}" "${PORT_REQ_2_LIST}" "" "")
endforeach()
