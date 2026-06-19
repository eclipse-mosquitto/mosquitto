# Rusquitto Rust Workspace

This workspace is the side-by-side Rust rewrite path for Mosquitto. It is
intentionally staged: the C implementation remains the compatibility oracle
until the Rust targets pass the existing broker, client, ABI, plugin, and
packaging gates.

Current crates:

- `rusquitto-protocol`: MQTT packet framing, basic decode/encode, properties,
  and topic validation/matching.
- `rusquitto-core`: in-memory broker state for sessions, subscriptions,
  retained messages, routing, and wills.
- `rusquitto-broker`: a `mosquitto` binary target used by the first broker
  compatibility tests.

Useful commands:

```sh
make rust
make rust-test-core
```

`make rust-test-core` builds a temporary test root where the existing Python
broker tests can find a `src/mosquitto` executable backed by Rust. The test
allowlist is deliberately small and should grow only as the Rust implementation
reaches real compatibility for each feature group.
