# CMakeLists.txt for pulse proto files
# Generates C++ code from protobuf definitions

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(absl REQUIRED)

# Find gRPC++ library (not always properly exported by gRPC cmake config)
find_library(GRPCPP_LIBRARY NAMES grpc++ PATHS /opt/homebrew/lib /usr/local/lib)
if(NOT GRPCPP_LIBRARY)
	message(FATAL_ERROR "Could not find grpc++ library")
endif()

# Find protoc and grpc_cpp_plugin
find_program(PROTOC protoc REQUIRED)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

# Collect all proto files recursively
file(GLOB_RECURSE PULSE_PROTO_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/*.proto"
)

# Output directory for generated files
set(PULSE_PROTO_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${PULSE_PROTO_OUT_DIR})

# Lists for generated sources
set(PULSE_PROTO_SRCS "")
set(PULSE_PROTO_HDRS "")
set(PULSE_GRPC_SRCS "")
set(PULSE_GRPC_HDRS "")

foreach(PROTO_FILE ${PULSE_PROTO_FILES})
	# Get the relative path and filename without extension
	file(RELATIVE_PATH PROTO_REL "${CMAKE_CURRENT_SOURCE_DIR}" "${PROTO_FILE}")
	get_filename_component(PROTO_DIR "${PROTO_REL}" DIRECTORY)
	get_filename_component(PROTO_NAME "${PROTO_FILE}" NAME_WE)

	# Create output subdirectory
	set(PROTO_OUT_SUBDIR "${PULSE_PROTO_OUT_DIR}/${PROTO_DIR}")
	file(MAKE_DIRECTORY ${PROTO_OUT_SUBDIR})

	# Generated file paths
	set(PROTO_SRC "${PROTO_OUT_SUBDIR}/${PROTO_NAME}.pb.cc")
	set(PROTO_HDR "${PROTO_OUT_SUBDIR}/${PROTO_NAME}.pb.h")

	list(APPEND PULSE_PROTO_SRCS "${PROTO_SRC}")
	list(APPEND PULSE_PROTO_HDRS "${PROTO_HDR}")

	# Generate protobuf sources
	add_custom_command(
		OUTPUT "${PROTO_SRC}" "${PROTO_HDR}"
		COMMAND ${PROTOC}
			--proto_path=${CMAKE_CURRENT_SOURCE_DIR}
			--cpp_out=${PULSE_PROTO_OUT_DIR}
			"${PROTO_FILE}"
		DEPENDS "${PROTO_FILE}"
		COMMENT "Generating protobuf sources for ${PROTO_REL}"
	)

	# Check if this is a service proto (contains gRPC service definitions)
	file(READ "${PROTO_FILE}" PROTO_CONTENT)
	if(PROTO_CONTENT MATCHES "service [A-Za-z]")
		set(GRPC_SRC "${PROTO_OUT_SUBDIR}/${PROTO_NAME}.grpc.pb.cc")
		set(GRPC_HDR "${PROTO_OUT_SUBDIR}/${PROTO_NAME}.grpc.pb.h")

		list(APPEND PULSE_GRPC_SRCS "${GRPC_SRC}")
		list(APPEND PULSE_GRPC_HDRS "${GRPC_HDR}")

		# Generate gRPC sources
		add_custom_command(
			OUTPUT "${GRPC_SRC}" "${GRPC_HDR}"
			COMMAND ${PROTOC}
				--proto_path=${CMAKE_CURRENT_SOURCE_DIR}
				--grpc_out=${PULSE_PROTO_OUT_DIR}
				--plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
				"${PROTO_FILE}"
			DEPENDS "${PROTO_FILE}"
			COMMENT "Generating gRPC sources for ${PROTO_REL}"
		)
	endif()
endforeach()

# Create the pulse_proto library
add_library(pulse_proto STATIC
	${PULSE_PROTO_SRCS}
	${PULSE_PROTO_HDRS}
	${PULSE_GRPC_SRCS}
	${PULSE_GRPC_HDRS}
)

target_include_directories(pulse_proto
	PUBLIC
		${PULSE_PROTO_OUT_DIR}
		${Protobuf_INCLUDE_DIRS}
)

target_link_libraries(pulse_proto
	PUBLIC
		protobuf::libprotobuf
		gRPC::grpc
		${GRPCPP_LIBRARY}
		absl::base
		absl::log
		absl::log_internal_check_op
		absl::log_internal_conditions
		absl::log_internal_message
		absl::status
		absl::strings
		absl::cord
)

# Suppress warnings in generated code
if(NOT WIN32)
	target_compile_options(pulse_proto PRIVATE -w)
endif()
