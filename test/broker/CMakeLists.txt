add_subdirectory(c)

file(GLOB PY_TEST_FILES [0-9][0-9]-*.py)
file(GLOB PY_PERSIST_TEST_FILES 15-*.py)
file(GLOB PY_HTTP_API_TEST_FILES 22-*.py)

list(APPEND PY_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/msg_sequence_test.py")

set(PERSIST_LIST
)

if(WITH_PERSIST_SQLITE)
    list(APPEND PERSIST_LIST persist_sqlite)
endif()

set(EXCLUDE_LIST
    01-connect-uname-password-success-no-tls
    03-publish-qos1-queued-bytes
    03-publish-qos2-max-inflight-exceeded
    09-extended-auth-single2
    # Not a test
    06-bridge-clean-session-core
)

set(PORT_REQ_4_LIST
    16-config-huge
    17-control-list-listeners
)

set(PORT_REQ_3_LIST
    08-tls-psk-bridge
    09-plugin-load-basic-auth
    22-http-api-api
)

set(PORT_REQ_2_LIST
    01-connect-auto-id
    01-connect-listener-allow-anonymous
    01-connect-zero-length-id
    04-retain-check-source-persist-diff-port
    06-bridge-b2br-disconnect-qos1
    06-bridge-b2br-disconnect-qos2
    06-bridge-b2br-late-connection-retain
    06-bridge-b2br-late-connection
    06-bridge-b2br-remapping
    06-bridge-br2b-disconnect-qos1
    06-bridge-br2b-disconnect-qos2
    06-bridge-br2b-remapping
    06-bridge-clean-session-csF-lcsF
    06-bridge-clean-session-csF-lcsN
    06-bridge-clean-session-csF-lcsT
    06-bridge-clean-session-csT-lcsF
    06-bridge-clean-session-csT-lcsN
    06-bridge-clean-session-csT-lcsT
    06-bridge-config-reload
    06-bridge-fail-persist-resend-qos1
    06-bridge-fail-persist-resend-qos2
    06-bridge-outgoing-retain
    06-bridge-per-listener-settings
    06-bridge-reconnect-local-out
    06-bridge-remap-receive-wildcard
    06-bridge-remote-shutdown
    08-ssl-bridge
    08-ssl-connect-cert-auth-crl
    08-ssl-connect-cert-auth-expired-allowed
    08-ssl-connect-cert-auth-expired
    08-ssl-connect-cert-auth-revoked
    08-ssl-connect-cert-auth-without
    08-ssl-connect-cert-auth
    08-ssl-connect-dhparam
    08-ssl-connect-identity
    08-ssl-connect-no-auth-wrong-ca
    08-ssl-connect-no-auth
    08-ssl-connect-no-identity
    08-tls-psk-pub
    09-plugin-load-acl
    09-plugin-load-extended-auth
    10-listener-mount-point
    15-persist-bridge-queue
    22-http-api-acl
    22-http-api-auth
    22-http-api-file
    22-http-api-tls
)

generate_test_names(PY_TEST_NAMES PY_TEST_FILES)

if(NOT MICROHTTPD_LIBRARY)
	foreach(PY_HTTP_API_TEST_FILE ${PY_HTTP_API_TEST_FILES})
		get_filename_component(PY_HTTP_API_TEST_NAME ${PY_HTTP_API_TEST_FILE} NAME_WE)
		list(APPEND EXCLUDE_LIST ${PY_HTTP_API_TEST_NAME})
	endforeach()
endif()

foreach(PY_PERSIST_TEST_FILE ${PY_PERSIST_TEST_FILES})
    get_filename_component(PY_PERSIST_TEST_NAME ${PY_PERSIST_TEST_FILE} NAME_WE)
    list(APPEND EXCLUDE_LIST ${PY_PERSIST_TEST_NAME})
endforeach()

foreach(PY_TEST_FILE ${PY_TEST_FILES})
    get_filename_component(PY_TEST_NAME ${PY_TEST_FILE} NAME_WE)
    if(${PY_TEST_NAME} IN_LIST EXCLUDE_LIST OR ${PY_TEST_NAME} IN_LIST SQLITE_LIST)
        continue()
    endif()
    add_test(NAME broker-${PY_TEST_NAME}
        COMMAND ${PY_TEST_FILE}
    )
	mosq_set_test_props("broker" ${PY_TEST_NAME}
		"${PY_TEST_NAMES}"
		"${PORT_REQ_2_LIST}"
		"${PORT_REQ_3_LIST}"
		"${PORT_REQ_4_LIST}"
	)
endforeach()

foreach(PERSIST_TYPE ${PERSIST_LIST})
    foreach(PY_TEST_FILE ${PY_PERSIST_TEST_FILES})
        get_filename_component(PY_TEST_NAME ${PY_TEST_FILE} NAME_WE)
        add_test(NAME broker-${PY_TEST_NAME}-${PERSIST_TYPE}
            COMMAND ${PY_TEST_FILE} ${PERSIST_TYPE}
        )
        set(PORTREQ "ports:1")
		mosq_set_test_props("broker" ${PY_TEST_NAME}-${PERSIST_TYPE}
			"broker-${PY_TEST_NAME}-${PERSIST_TYPE}" "" "" ""
		)
    endforeach()
endforeach()
