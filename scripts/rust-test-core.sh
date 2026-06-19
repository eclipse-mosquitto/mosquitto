#!/bin/sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_dir"

cargo test --manifest-path rust/Cargo.toml --workspace
cargo build --manifest-path rust/Cargo.toml -p rusquitto-broker

test_root="$repo_dir/rust/target/mosquitto-test-root"
mkdir -p "$test_root/src"
cp "$repo_dir/rust/target/debug/mosquitto" "$test_root/src/mosquitto"

export BUILD_ROOT="$test_root"

(
    cd test/broker
    python3 01-connect-allow-anonymous.py
    python3 01-connect-disconnect-v5.py
    python3 02-subpub-qos0-long-topic.py
    python3 02-subpub-qos0-send-retain.py
    python3 02-subpub-qos0-topic-alias.py
    python3 04-retain-qos0-repeated.py
    python3 07-will-qos0.py
)
