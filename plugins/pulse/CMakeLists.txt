# CMakeLists.txt for mosquitto_pulse plugin
# Provides HiveMQ Pulse integration for topic discovery and governance

set(PLUGIN_NAME mosquitto_pulse)

# Require gRPC and protobuf (provided by parent CMakeLists.txt)
if(NOT TARGET pulse_proto)
	message(FATAL_ERROR "pulse_proto library not found. Ensure WITH_GRPC is enabled.")
endif()

set(SRCLIST
	plugin.cpp
	pulse_client.cpp
	pulse_client.h
	pulse_discovery.c
	pulse_discovery.h
	pulse_information_model.h
)

set(INCLIST
	"${CJSON_INCLUDE_DIRS}"
	"${mosquitto_SOURCE_DIR}/include"
	"${mosquitto_SOURCE_DIR}/lib"
	"${mosquitto_SOURCE_DIR}/deps"
)

set(LINKLIST
	pulse_proto
	cJSON
	OpenSSL::SSL
)

# Add threading library
if(NOT WIN32)
	find_package(Threads REQUIRED)
	list(APPEND LINKLIST Threads::Threads)
endif()

# Use the standard plugin helper macro
add_mosquitto_plugin("${PLUGIN_NAME}" "${SRCLIST}" "${INCLIST}" "${LINKLIST}")

# Need C++17 for the gRPC/protobuf code
set_property(TARGET ${PLUGIN_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PLUGIN_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
