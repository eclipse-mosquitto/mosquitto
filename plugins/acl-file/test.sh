VG="valgrind --log-file=vglog"
${VG} ../../src/mosquitto -c test.conf -v
