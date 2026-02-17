# AGENTS.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview
Eclipse Mosquitto is an open source MQTT broker supporting MQTT v5.0, v3.1.1, and v3.1. It includes:
- The `mosquitto` broker
- `libmosquitto` C/C++ client library
- Command-line clients (`mosquitto_pub`, `mosquitto_sub`, `mosquitto_rr`)
- Administration tools (`mosquitto_ctrl`, `mosquitto_passwd`, `mosquitto_signal`)

## Build Commands

### CMake (recommended for all platforms)
```bash
mkdir build && cd build
cmake ..
make
```

### Make (Linux only)
```bash
make              # Full build including docs
make binary       # Build without man pages
```

### Common build options (both CMake and Make)
- `WITH_TLS=yes/no` - SSL/TLS support (default: yes)
- `WITH_WEBSOCKETS=yes/no` - WebSocket support (default: yes)
- `WITH_SQLITE=yes/no` - SQLite persistence plugin (default: yes)
- `WITH_DOCS=yes/no` - Build man pages (default: yes)
- `WITH_BUNDLED_DEPS=yes/no` - Use bundled uthash/utlist (default: yes)

## Testing

### Run all tests
```bash
make test         # Run tests serially
make ptest        # Run tests in parallel (faster, uses ~20 processes)
```

### Run specific test suites
```bash
make -C test/unit test      # Unit tests only (CUnit/gmock)
make -C test/broker test    # Broker integration tests
make -C test/lib test       # Library tests
make -C test/client test    # Client tests
```

### Run individual test
Tests are Python scripts that can be run directly:
```bash
python test/broker/01-connect-accept-protocol.py
```

### Test dependencies
- Python 3
- CUnit
- googletest/gmock (for unit tests)

## Code Formatting
Run `./format.sh` to format C/C++ code using uncrustify with `.uncrustify.cfg`.

## Architecture

### Directory Structure
- `src/` - Broker implementation
- `lib/` - libmosquitto client library
- `libcommon/` - Shared utilities (properties, topics, UTF-8, memory, time)
- `client/` - CLI clients source
- `apps/` - Administration tools (mosquitto_ctrl, mosquitto_passwd, db_dump)
- `plugins/` - Broker plugins (authentication, persistence)
- `include/` - Public API headers
- `deps/` - Bundled dependencies (uthash, utlist, picohttpparser)

### Key Internal Headers
- `src/mosquitto_broker_internal.h` - Broker internal structures and types
- `lib/mosquitto_internal.h` - Client library internal structures
- `include/mosquitto.h` - Main public API (includes all public headers)
- `include/mosquitto/broker_plugin.h` - Plugin API

### Plugin System
Plugins are dynamically loaded shared libraries. See `plugins/README.md` for examples.
Key plugin types:
- `dynamic-security` - Runtime ACL/user management via $CONTROL topics
- `acl-file` / `password-file` - File-based authentication
- `persist-sqlite` - SQLite-based message persistence

### Broker Core Flow
- `src/mosquitto.c` - Main entry point and event loop
- `src/handle_*.c` - MQTT packet handlers (connect, publish, subscribe, etc.)
- `src/database.c` - In-memory message and subscription storage
- `src/bridge.c` - Bridge connections to other brokers
- `src/net.c` - Network I/O

### Client Library Core
- `lib/libmosquitto.c` - Client creation and destruction
- `lib/loop.c` - Event loop handling
- `lib/net_mosq.c` - Network I/O
- `lib/send_*.c` / `lib/handle_*.c` - MQTT packet send/receive

## Contributing Guidelines
- New features: branch from `develop`
- Bug fixes: branch from `fixes`
- Sign commits with `git commit -s` for Eclipse ECA compliance
