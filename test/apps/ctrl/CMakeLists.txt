file(GLOB PY_TEST_FILES ctrl-*.py)

set(EXCLUDE_LIST
    # none
)

add_python_tests_from_current_directory2("apps" "${PY_TEST_FILES}" "${EXCLUDE_LIST}")


function(add_ctrl_shell_test TEST_NAME)
	if(NOT EDITLINE_FOUND)
		return()
	endif()
	add_executable(${TEST_NAME}
		${TEST_NAME}.cpp
		${mosquitto_SOURCE_DIR}/common/json_help.c
		$<TARGET_OBJECTS:ctrl_shell>
		$<TARGET_OBJECTS:ctrl_shell_mock>
		$<TARGET_OBJECTS:libmosquitto_mock>
	)
	target_compile_definitions(${TEST_NAME} PRIVATE WITH_EDITLINE)
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
	if(NOT EDITLINE_FOUND)
		return()
	endif()
	add_executable(${TEST_NAME}
		${TEST_NAME}.cpp
		${mosquitto_SOURCE_DIR}/common/json_help.c
		$<TARGET_OBJECTS:ctrl_shell>
		$<TARGET_OBJECTS:ctrl_shell_mock>
		$<TARGET_OBJECTS:libmosquitto_mock>
	)
	target_compile_definitions(${TEST_NAME} PRIVATE WITH_EDITLINE)
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
		"${EDITLINE_LIBRARIES}"
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
