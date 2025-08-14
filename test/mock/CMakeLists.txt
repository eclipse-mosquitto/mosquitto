add_subdirectory(libcommon)
add_subdirectory(lib)
add_subdirectory(apps)

function(add_mosquitto_mock MOCK_NAME)
	add_library(${MOCK_NAME}_mock OBJECT ${MOCK_NAME}_mock.cpp)
	target_include_directories(${MOCK_NAME}_mock
		PUBLIC
			${mosquitto_SOURCE_DIR}/test/mock
	)
	target_link_libraries(${MOCK_NAME}_mock
		PUBLIC
			GTest::gmock
	)
endfunction()

if(EDITLINE_FOUND)
	add_mosquitto_mock(editline)
endif()

add_mosquitto_mock(pthread)
