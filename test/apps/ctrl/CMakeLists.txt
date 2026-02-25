file(GLOB PY_TEST_FILES ctrl-*.py)

set(EXCLUDE_LIST
    # none
)

foreach(PY_TEST_FILE ${PY_TEST_FILES})
    get_filename_component(PY_TEST_NAME ${PY_TEST_FILE} NAME_WE)
    if(${PY_TEST_NAME} IN_LIST EXCLUDE_LIST)
        continue()
    endif()
    add_test(NAME apps-${PY_TEST_NAME}
        COMMAND ${PY_TEST_FILE}
    )
    set_tests_properties(apps-${PY_TEST_NAME}
        PROPERTIES
            ENVIRONMENT "${TEST_ENVIRONMENT}"
            SKIP_RETURN_CODE 77
    )
endforeach()


function(add_ctrl_shell_test TEST_NAME)
	if(NOT WITH_CTRL_SHELL OR NOT EDITLINE_FOUND)
		return()
	endif()
	add_executable(${TEST_NAME}
		${TEST_NAME}.cpp
		${mosquitto_SOURCE_DIR}/common/json_help.c
		$<TARGET_OBJECTS:ctrl_shell>
		$<TARGET_OBJECTS:ctrl_shell_mock>
		$<TARGET_OBJECTS:libmosquitto_mock>
	)
	target_compile_definitions(${TEST_NAME} PRIVATE
		WITH_CTRL_SHELL
		WITH_EDITLINE
	)
	target_include_directories(${TEST_NAME} PRIVATE
		${mosquitto_SOURCE_DIR}
		${mosquitto_SOURCE_DIR}/apps/mosquitto_ctrl
		${mosquitto_SOURCE_DIR}/common
		${mosquitto_SOURCE_DIR}/include
		${mosquitto_SOURCE_DIR}/lib
		${mosquitto_SOURCE_DIR}/test/mock
		${mosquitto_SOURCE_DIR}/test/mock/apps/mosquitto_ctrl
		${mosquitto_SOURCE_DIR}/test/mock/lib
	)
	target_link_libraries(${TEST_NAME} PRIVATE
		GTest::gmock_main
		editline_mock
		pthread_mock
		cjson
		libmosquitto_common
	)
	gtest_discover_tests(${TEST_NAME})
endfunction()

function(add_ctrl_shell_test_real_editline TEST_NAME)
	if(NOT WITH_CTRL_SHELL OR NOT EDITLINE_FOUND)
		return()
	endif()
	add_executable(${TEST_NAME}
		${TEST_NAME}.cpp
		${mosquitto_SOURCE_DIR}/common/json_help.c
		$<TARGET_OBJECTS:ctrl_shell>
		$<TARGET_OBJECTS:ctrl_shell_mock>
		$<TARGET_OBJECTS:libmosquitto_mock>
	)
	target_compile_definitions(${TEST_NAME} PRIVATE
		WITH_CTRL_SHELL
	)
	target_include_directories(${TEST_NAME} PRIVATE
		${mosquitto_SOURCE_DIR}
		${mosquitto_SOURCE_DIR}/apps/mosquitto_ctrl
		${mosquitto_SOURCE_DIR}/common
		${mosquitto_SOURCE_DIR}/include
		${mosquitto_SOURCE_DIR}/lib
		${mosquitto_SOURCE_DIR}/test/mock
		${mosquitto_SOURCE_DIR}/test/mock/apps/mosquitto_ctrl
		${mosquitto_SOURCE_DIR}/test/mock/lib
	)
	target_link_libraries(${TEST_NAME} PRIVATE
		LineEditing::LineEditing
		GTest::gmock_main
		pthread_mock
		cjson
		libmosquitto_common
	)
	gtest_discover_tests(${TEST_NAME})
endfunction()

add_ctrl_shell_test(ctrl_shell_test)
add_ctrl_shell_test(ctrl_shell_broker_test)
add_ctrl_shell_test_real_editline(ctrl_shell_completion_test)
add_ctrl_shell_test(ctrl_shell_dynsec_test)
add_ctrl_shell_test(ctrl_shell_help_test)
add_ctrl_shell_test(ctrl_shell_options_test)
add_ctrl_shell_test(ctrl_shell_pre_connect_test)
