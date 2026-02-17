# Pulse Plugin for Mosquitto

This plugin provides HiveMQ Pulse integration for Eclipse Mosquitto, enabling topic discovery and governance features.

## Features

- **Topic Discovery**: Automatically tracks MQTT topics that are not governed by the information model
- **Information Model**: Fetches and maintains the Pulse information model
- **gRPC Streams**: Handles bidirectional communication with Pulse server for:
  - Ungoverned topics requests
  - Data point sampling requests
  - Information model update notifications
- **Heartbeat**: Maintains connection health with the Pulse server

## Configuration

The plugin is configured via the mosquitto configuration file using plugin options:

```conf
# Load the plugin
plugin /path/to/mosquitto_pulse.so

# Option 1: Use a connection string (JWT) - preferred method
plugin_opt_connection_string <JWT_TOKEN>

# Option 2: Specify individual configuration values
plugin_opt_agent_id <UUID>
plugin_opt_project_id <UUID>
plugin_opt_grpc_server localhost:50051

# Optional: Limit number of discovered topics (default: 1000)
plugin_opt_discovery_max 1000
```

### Connection String

The connection string is a JWT token that contains:
- `sub`: Agent ID (UUID)
- `project`: Project ID (UUID)
- `uri`: gRPC server address

When using `plugin_opt_connection_string`, these values are automatically extracted from the JWT.

## Building

The plugin requires gRPC support. Build with:

```bash
mkdir build && cd build
cmake .. -DWITH_GRPC=ON -DWITH_PLUGIN_PULSE=ON
make mosquitto_pulse
```

## Dependencies

- gRPC C++ library
- Protocol Buffers
- OpenSSL (for JWT parsing)
- cJSON

## Plugin Events

The plugin registers callbacks for:
- `MOSQ_EVT_MESSAGE_IN`: Processes incoming messages for topic discovery
- `MOSQ_EVT_TICK`: Logs discovery statistics periodically

## License

EPL-2.0 OR BSD-3-Clause
