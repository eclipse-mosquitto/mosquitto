#!/bin/sh

valgrind --log-file=vglog ../../../src/mosquitto -c test.conf -v
