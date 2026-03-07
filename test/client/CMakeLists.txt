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


foreach(PY_TEST_FILE ${PY_TEST_FILES})
    get_filename_component(PY_TEST_NAME ${PY_TEST_FILE} NAME_WE)
    if(${PY_TEST_NAME} IN_LIST EXCLUDE_LIST)
        continue()
    endif()
    if(${PY_TEST_NAME} IN_LIST PORT_REQ_2_LIST)
        set(PORTREQ "ports:1,ports:1")
    else()
        set(PORTREQ "ports:1")
    endif()
    add_test(NAME client-${PY_TEST_NAME}
        COMMAND ${PY_TEST_FILE}
    )
    set_tests_properties(client-${PY_TEST_NAME}
        PROPERTIES
            RESOURCE_GROUPS ${PORTREQ}
            ENVIRONMENT "BUILD_ROOT=${CMAKE_BINARY_DIR}"
    )
endforeach()
