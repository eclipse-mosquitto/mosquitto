# This is a cmake script. Process it with the CMake gui or command line utility
# to produce makefiles / Visual Studio project files on Mac OS X and Windows.
#
# To configure the build options either use the CMake gui, or run the command
# line utility including the "-i" option.

cmake_minimum_required(VERSION 3.18)

set (VERSION 2.1.2)
project(mosquitto
	VERSION ${VERSION}
	DESCRIPTION "Eclipse Mosquitto"
	LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

add_definitions (-DCMAKE -DVERSION=\"${VERSION}\")

if(WIN32)
	add_definitions("-D_CRT_SECURE_NO_WARNINGS")
	add_definitions("-D_CRT_NONSTDC_NO_DEPRECATE")
endif()

if(APPLE)
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -undefined dynamic_lookup")
endif()

add_library(common-options INTERFACE)
if(NOT WIN32)
	target_compile_options(common-options INTERFACE -Wall -Wextra -Wconversion)
endif()

include(CMakePushCheckState)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSourceCompiles)
include(GNUInstallDirs)
include(CMakePushCheckState)
include(CheckSourceCompiles)

option(WITH_BUNDLED_DEPS "Build with bundled dependencies?" ON)
option(WITH_TLS "Include SSL/TLS support?" ON)
option(WITH_TLS_PSK "Include TLS-PSK support (requires WITH_TLS)?" ON)
option(WITH_TESTS "Enable tests" ON)
option(INC_MEMTRACK "Include memory tracking support?" ON)
if (WITH_TLS)
	find_package(OpenSSL REQUIRED)
	add_definitions("-DWITH_TLS")

	# mosquitto uses OpenSSL 1.1 API, so set OPENSSL_API_COMPAT accordingly:
	# https://www.openssl.org/docs/manmaster/man7/OPENSSL_API_COMPAT.html
	# TODO: migrate off ENGINE API (deprecated since OpenSSL 3.0), see:
	#       https://www.openssl.org/docs/manmaster/man7/migration_guide.html#Engines-and-METHOD-APIs
	add_definitions("-DOPENSSL_API_COMPAT=0x10100000L")

	if (WITH_TLS_PSK)
		add_definitions("-DWITH_TLS_PSK")
	endif (WITH_TLS_PSK)
else()
	set (OPENSSL_INCLUDE_DIR "")
endif()

option(WITH_UNIX_SOCKETS "Include Unix Domain Socket support?" ON)
if(WITH_UNIX_SOCKETS)
	add_definitions("-DWITH_UNIX_SOCKETS")
endif()

option(WITH_SOCKS "Include SOCKS5 support?" ON)
if(WITH_SOCKS)
	add_definitions("-DWITH_SOCKS")
endif()

option(WITH_WEBSOCKETS "Include websockets support?" ON)
option(WITH_WEBSOCKETS_BUILTIN "Websockets support uses builtin library? Set OFF to use libwebsockets" ON)

if(WITH_WEBSOCKETS AND NOT WITH_TLS)
	message(FATAL_ERROR "WITH_WEBSOCKETS support requires WITH_TLS.")
endif()
option(WITH_SRV "Include SRV lookup support?" OFF)
option(WITH_STATIC_LIBRARIES "Build static versions of the libmosquitto/pp libraries?" OFF)
option(WITH_PIC "Build the static library with PIC (Position Independent Code) enabled archives?" OFF)

option(WITH_THREADING "Include threading support?" ON)
if(WITH_THREADING)
	add_definitions("-DWITH_THREADING")
	if(WIN32)
		find_package(PThreads4W REQUIRED)
	endif()
endif()

option(WITH_DLT "Include DLT support?" OFF)
message(STATUS "WITH_DLT = ${WITH_DLT}")
if(WITH_DLT)
	find_package(PkgConfig)
	pkg_check_modules(DLT "automotive-dlt >= 2.11" REQUIRED)
endif()

find_package(cJSON REQUIRED)
find_package(argon2)

option(WITH_CTRL_SHELL "Include mosquitto_ctrl interactive shell support?" ON)
if(WITH_CTRL_SHELL)
	find_package(LineEditing)
endif()
if(ARGON2_FOUND)
	# Disable until separate password handling thread is implemented
	#add_definitions("-DWITH_ARGON2")
endif()

option(WITH_LTO "Build with link time optimizations (IPO) / interprocedural optimization (IPO) enabled." ON)
if(WITH_LTO)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT is_ipo_supported OUTPUT output)
	if(is_ipo_supported)
		set_property(TARGET common-options PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
	else()
		message(WARNING "LTO/IPO is not supported: ${output}")
	endif()
endif()

# ========================================
# Include projects
# ========================================

option(WITH_CLIENTS "Build clients?" ON)
option(WITH_BROKER "Build broker?" ON)
option(WITH_APPS "Build apps?" ON)
option(WITH_PLUGINS "Build plugins?" ON)
option(WITH_DOCS "Build documentation?" ON)

add_library(config-header INTERFACE)
target_sources(config-header INTERFACE config.h)
target_include_directories(config-header
	INTERFACE
		${mosquitto_SOURCE_DIR}
)

if(WITH_TLS)
	target_include_directories(config-header
		INTERFACE
			"${OPENSSL_INCLUDE_DIR}"
	)
endif()


add_subdirectory(libcommon)
add_subdirectory(lib)
if(WITH_CLIENTS)
	add_subdirectory(client)
endif()

if(WITH_BROKER)
	add_subdirectory(src)
endif()

if(WITH_APPS)
	add_subdirectory(apps)
endif()

if(WITH_PLUGINS)
	add_subdirectory(plugins)
endif()

if(WITH_DOCS)
	add_subdirectory(man)
endif()

# ========================================
# Install config file
# ========================================

if(WITH_BROKER)
	install(FILES mosquitto.conf RENAME mosquitto.conf.example DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/mosquitto")
	install(FILES aclfile.example pskfile.example pwfile.example DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/mosquitto")
endif()

# ========================================
# Install pkg-config files
# ========================================

configure_file(libmosquitto.pc.in libmosquitto.pc @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libmosquitto.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
configure_file(libmosquittopp.pc.in libmosquittopp.pc @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libmosquittopp.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

# ========================================
# Install headers
# ========================================
install(
	FILES
	${mosquitto_SOURCE_DIR}/include/mosquitto.h
	${mosquitto_SOURCE_DIR}/include/mosquitto_broker.h
	${mosquitto_SOURCE_DIR}/include/mosquitto_plugin.h
	${mosquitto_SOURCE_DIR}/include/mosquittopp.h
	${mosquitto_SOURCE_DIR}/include/mqtt_protocol.h
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(
	FILES
	${mosquitto_SOURCE_DIR}/include/mosquitto/broker.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/broker_control.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/broker_plugin.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/defs.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libcommon.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libcommon_base64.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libcommon_cjson.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libcommon_file.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libcommon_memory.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libcommon_password.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libcommon_properties.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libcommon_random.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libcommon_string.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libcommon_time.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libcommon_topic.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libcommon_utf8.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquitto.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquitto_auth.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquitto_callbacks.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquitto_connect.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquitto_create_delete.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquitto_helpers.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquitto_loop.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquitto_message.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquitto_options.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquitto_publish.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquitto_socks.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquitto_subscribe.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquitto_tls.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquitto_unsubscribe.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquitto_will.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/libmosquittopp.h
	${mosquitto_SOURCE_DIR}/include/mosquitto/mqtt_protocol.h
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mosquitto"
)

# ========================================
# Testing
# ========================================
if(WITH_TESTS)
	find_package(GTest REQUIRED)
	include(GoogleTest)
	enable_testing()
	add_subdirectory(test)
endif()
