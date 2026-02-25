add_executable(libcommon-test
	base64_test.c
	file_test.c
	property_add.c
	property_value.c
	strings_test.c
	test.c
	topic_test.c
	trim_test.c
	utf8.c
)

target_include_directories(libcommon-test
	PRIVATE
		${mosquitto_SOURCE_DIR}/libcommon
)
target_link_libraries(libcommon-test
	PRIVATE
		common-unit-test-header
		OpenSSL::SSL
		libmosquitto_common
)
add_test(NAME unit-libcommon-test COMMAND libcommon-test)
set_tests_properties(unit-libcommon-test
    PROPERTIES
        SKIP_RETURN_CODE 77
)
