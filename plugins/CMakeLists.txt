function(add_mosquitto_plugin_no_install PLUGIN_NAME SRCLIST INCLIST LINKLIST)
	add_library(${PLUGIN_NAME} MODULE ${SRCLIST})
	target_include_directories(${PLUGIN_NAME} PRIVATE
		${INCLIST}
		"${mosquitto_SOURCE_DIR}/"
		"${mosquitto_SOURCE_DIR}/common"
		"${mosquitto_SOURCE_DIR}/include"
	)
	if(WITH_BUNDLED_DEPS)
		target_include_directories(${PLUGIN_NAME} PRIVATE "${mosquitto_SOURCE_DIR}/deps")
	endif()

	set_target_properties(${PLUGIN_NAME} PROPERTIES
		PREFIX ""
		POSITION_INDEPENDENT_CODE 1
	)

	target_link_libraries(${PLUGIN_NAME} PRIVATE
		${LINKLIST}
		common-options
		mosquitto
	)
endfunction()

function(add_mosquitto_plugin PLUGIN_NAME SRCLIST INCLIST LINKLIST)
	add_mosquitto_plugin_no_install("${PLUGIN_NAME}" "${SRCLIST}" "${INCLIST}" "${LINKLIST}")

	if(WIN32)
		install(TARGETS ${PLUGIN_NAME}
			DESTINATION "${CMAKE_INSTALL_BINDIR}")
	else()
		install(TARGETS ${PLUGIN_NAME}
			RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
			LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		)
	endif()
endfunction()


option(WITH_PLUGIN_ACL_FILE "Build acl-file plugin?" ON)
option(WITH_PLUGIN_DYNAMIC_SECURITY "Build dynamic-security plugin?" ON)
option(WITH_PLUGIN_EXAMPLES "Build example plugins?" ON)
option(WITH_PLUGIN_PERSIST_SQLITE "Build persist-sqlite plugin?" ON)
option(WITH_PLUGIN_PASSWORD_FILE "Build password-file plugin?" ON)
option(WITH_PLUGIN_SPARKPLUG_AWARE "Build sparkplug-aware plugin?" ON)

if(WITH_PLUGIN_ACL_FILE)
	add_subdirectory(acl-file)
endif()

if(WITH_PLUGIN_DYNAMIC_SECURITY)
	add_subdirectory(dynamic-security)
endif()

if (WITH_PLUGIN_EXAMPLES)
	add_subdirectory(examples)
endif()

if(WITH_PLUGIN_PASSWORD_FILE)
	add_subdirectory(password-file)
endif()

if (WITH_PLUGIN_PERSIST_SQLITE)
	find_package(SQLite3 REQUIRED)
	add_subdirectory(persist-sqlite)
endif()

if(WITH_PLUGIN_SPARKPLUG_AWARE)
	add_subdirectory(sparkplug-aware)
endif()
