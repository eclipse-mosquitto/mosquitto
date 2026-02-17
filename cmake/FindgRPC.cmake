# FindgRPC.cmake - Find gRPC C library
#
# This module defines:
#   GRPC_FOUND        - True if gRPC was found
#   GRPC_INCLUDE_DIRS - Include directories for gRPC
#   GRPC_LIBRARIES    - Libraries to link against
#   gRPC::grpc        - Imported target for gRPC

include(FindPackageHandleStandardArgs)

# Try pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(PC_GRPC QUIET grpc)
endif()

# Check environment variable
if(DEFINED ENV{GRPC_DIR})
	set(GRPC_DIR "$ENV{GRPC_DIR}")
endif()

find_path(GRPC_INCLUDE_DIR
	NAMES grpc/grpc.h
	HINTS
		${GRPC_DIR}
		${PC_GRPC_INCLUDEDIR}
		${PC_GRPC_INCLUDE_DIRS}
	PATH_SUFFIXES include
)

find_library(GRPC_LIBRARY
	NAMES grpc
	HINTS
		${GRPC_DIR}
		${PC_GRPC_LIBDIR}
		${PC_GRPC_LIBRARY_DIRS}
	PATH_SUFFIXES lib lib64
)

# gRPC depends on gpr (gRPC Platform Support)
find_library(GPR_LIBRARY
	NAMES gpr
	HINTS
		${GRPC_DIR}
		${PC_GRPC_LIBDIR}
		${PC_GRPC_LIBRARY_DIRS}
	PATH_SUFFIXES lib lib64
)

find_package_handle_standard_args(gRPC DEFAULT_MSG
	GRPC_INCLUDE_DIR GRPC_LIBRARY
)

if(GRPC_FOUND)
	set(GRPC_INCLUDE_DIRS ${GRPC_INCLUDE_DIR})
	set(GRPC_LIBRARIES ${GRPC_LIBRARY})
	if(GPR_LIBRARY)
		list(APPEND GRPC_LIBRARIES ${GPR_LIBRARY})
	endif()

	mark_as_advanced(
		GRPC_LIBRARY
		GRPC_INCLUDE_DIR
		GRPC_DIR
		GPR_LIBRARY
	)

	if(NOT TARGET gRPC::grpc)
		add_library(gRPC::grpc SHARED IMPORTED)
		set_target_properties(gRPC::grpc
			PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${GRPC_INCLUDE_DIRS}"
				IMPORTED_LOCATION "${GRPC_LIBRARY}"
				IMPORTED_IMPLIB "${GRPC_LIBRARY}"
		)
		if(GPR_LIBRARY)
			set_target_properties(gRPC::grpc
				PROPERTIES
					INTERFACE_LINK_LIBRARIES "${GPR_LIBRARY}"
			)
		endif()
	endif()
else()
	set(GRPC_DIR "" CACHE STRING
		"An optional hint to a directory for finding `gRPC`"
	)
endif()
