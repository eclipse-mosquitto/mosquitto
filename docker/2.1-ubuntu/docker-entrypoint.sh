#!/bin/sh
set -e

# Set permissions
user="$(id -u)"
if [ "$PUID" = "" ]; then
	PUID="1883"
fi
if [ "$PGID" = "" ]; then
	PGID="1883"
fi
if [ "$user" = '0' ]; then
	[ -d "/mosquitto/data" ] && chown -R ${PUID}:${PGID} /mosquitto/data || true
fi

exec "$@"
