include(FindPackageHandleStandardArgs)

set(FIND_PATH_OPTS "")
if(APPLE)
	list(APPEND FIND_PATH_OPTS
		NO_CMAKE_SYSTEM_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
    )
endif()

# Checks an environment variable; note that the first check
# does not require the usual CMake $-sign.
if(DEFINED env{EDITLINE_DIR})
	set(EDITLINE_DIR "$ENV{EDITLINE_DIR}")
endif()

find_path(
		EDITLINE_INCLUDE_DIR
		editline/readline.h
	HINTS
		EDITLINE_DIR
	${FIND_PATH_OPTS}
)

find_library(EDITLINE_LIBRARY
	NAMES edit
	HINTS ${EDITLINE_DIR}
)

if(EDITLINE_INCLUDE_DIR AND EDITLINE_LIBRARY)
	set(EDITLINE_FOUND TRUE)
	set(LINEEDITING_FOUND TRUE)
	set(LINEEDITING_INCLUDE_DIRS ${EDITLINE_INCLUDE_DIR})
	set(LINEEDITING_LIBRARIES ${EDITLINE_LIBRARY})

	if(NOT TARGET LineEditing::LineEditing)
		add_library(LineEditing::LineEditing UNKNOWN IMPORTED)
		set_target_properties(LineEditing::LineEditing PROPERTIES
			IMPORTED_LOCATION "${EDITLINE_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${EDITLINE_INCLUDE_DIR}"
			INTERFACE_COMPILE_DEFINITIONS "WITH_EDITLINE"
		)
	endif()
else()
	find_path(
			READLINE_INCLUDE_DIR
			readline/readline.h
		HINTS
			READLINE_DIR
		${FIND_PATH_OPTS}
	)

	find_library(READLINE_LIBRARY
		NAMES readline
		HINTS ${READLINE_DIR}
	)

	if(READLINE_INCLUDE_DIR AND READLINE_LIBRARY)
		set(LINEEDITING_FOUND TRUE)
		set(LINEEDITING_INCLUDE_DIRS ${READLINE_INCLUDE_DIR})
		set(LINEEDITING_LIBRARIES ${READLINE_LIBRARY})

		if(NOT TARGET LineEditing::LineEditing)
			add_library(LineEditing::LineEditing UNKNOWN IMPORTED)
			set_target_properties(LineEditing::LineEditing PROPERTIES
				IMPORTED_LOCATION "${READLINE_LIBRARY}"
				INTERFACE_INCLUDE_DIRECTORIES "${READLINE_INCLUDE_DIR}"
				INTERFACE_COMPILE_DEFINITIONS "WITH_READLINE"
			)
		endif()
	endif()
endif()

find_package_handle_standard_args(LineEditing
	REQUIRED_VARS LINEEDITING_LIBRARIES LINEEDITING_INCLUDE_DIRS
	FAIL_MESSAGE "Could not find libedit or readline library"
)
