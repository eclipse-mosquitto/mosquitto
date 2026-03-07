# Convert CMake ON/OFF options to yes/no for test scripts
foreach(_var WITH_HTTP_API WITH_PERSISTENCE WITH_SOCKS WITH_TLS WITH_TLS_PSK WITH_WEBSOCKETS)
    if(${_var})
        set(${_var}_YESNO "yes")
    else()
        set(${_var}_YESNO "no")
    endif()
endforeach()

set(TEST_ENVIRONMENT
    "BUILD_ROOT=${CMAKE_BINARY_DIR}"
    "WITH_HTTP_API=${WITH_HTTP_API_YESNO}"
    "WITH_PERSISTENCE=${WITH_PERSISTENCE_YESNO}"
    "WITH_SOCKS=${WITH_SOCKS_YESNO}"
    "WITH_TLS=${WITH_TLS_YESNO}"
    "WITH_TLS_PSK=${WITH_TLS_PSK_YESNO}"
    "WITH_WEBSOCKETS=${WITH_WEBSOCKETS_YESNO}"
)

add_subdirectory(mock)

add_subdirectory(apps)
add_subdirectory(broker)
add_subdirectory(client)
add_subdirectory(lib)
add_subdirectory(unit)

add_custom_target(coverage
    COMMAND lcov --quiet --capture --directory . --output-file ${PROJECT_BINARY_DIR}/coverage.info --no-external
    COMMAND genhtml --quiet ${PROJECT_BINARY_DIR}/coverage.info --output-directory ${PROJECT_BINARY_DIR}/coverage
    COMMENT "Generating coverage.info and coverage/index.html in ${PROJECT_BINARY_DIR}"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
