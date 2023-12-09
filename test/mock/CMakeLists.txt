add_subdirectory(lib)
add_subdirectory(apps)

if(EDITLINE_FOUND)
	add_library(editline_mock OBJECT editline_mock.cpp)
	target_include_directories(editline_mock
		PUBLIC
			${mosquitto_SOURCE_DIR}/test/mock
	)
endif()

add_library(pthread_mock OBJECT pthread_mock.cpp)
target_include_directories(pthread_mock
    PUBLIC
        ${mosquitto_SOURCE_DIR}/test/mock
)
