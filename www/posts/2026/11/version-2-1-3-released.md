<!--
.. title: Version 2.1.3 released.
.. slug: version-2-1-3-released
.. date: 2026-11-17 09:30:00 UTC
.. tags: Releases
.. category:
.. link:
.. description:
.. type: text
-->

Version 2.1.3 of Mosquitto has been released. This is a critical security release.
Version 2.0.23 of Mosquitto has been released. This is a security release.


# 2.1.3 changes

# Security
- Fix incorrect handling of websocket frames for the builtin implementation,
  which could lead to out of bounds memory access and potential remote code
  execution.
- Fix case where use of a specific will delay interval could cause all delayed
  wills to be suppressed.
- Fix potential authentication bypass due to not checking failing password
  hash functions for OpenSSL 4.0 when using the FIPS module.
- Fix potential crash in the dynamic security plugin caused if
  createClient/modifyClient/createGroup/modifyGroup was called with a role
  array, then a role in that array was later deleted and then a further change
  was made to the dynsec config without having restarted the broker.
- Fix crashes in mosquitto_ctrl if connecting to a malicious broker.

# Broker
- Fix MOSQ_EVT_DISCONNECT being called before MOSQ_EVT_ACL_CHECK for the will
  of that client. Closes #3487.
- Fix password length not being passed to MOSQ_EVT_BASIC_AUTH events.
  Closes #3490.
- Fix incorrect maximum-packet-size restriction for outgoing packets.
  Closes #3503.
- Fix incorrect maximum-packet-size restriction for incoming packets.
  Closes #3515.
- Fix will messages being incorrectly delayed if a client set
  session-expiry-interval=0 when using will-delay-interval>0. Closes #3505.
- Reject PROXY v2 connections with duplicate SSL_CN entries.
- Reject PROXY v2 connections with SSL string entries that are not valid UTF-8.
- Fix case where use of a specific will delay interval could cause all delayed
  wills to be suppressed.

# Common lib:
- Fix potential crash if reading a file in restricted mode and the group id
  does not have an entry in /etc/groups. Closes #3498.
- Fix missing SONAME. Closes #3483.
- Increase default iterations for sha512-pbkdf2 to 20000 and document it.
- Fix missing checks on password hashing functions.

# Lib
- Fix mosquitto_loop_start() leaving the mosq struct in an invalid state if
  thread creation fails. Closes #3496.
- Reject payloads>65535 bytes in mosquitto_will_set().

# Plugins
- Fix migrate_to_persist_sqlite.py not base64 decoding message payloads when
  migrating. Closes #3492.

# Dynamic security plugin
- Fix potential crash caused if createClient/modifyClient/createGroup/modifyGroup was
  called with a role array, then a role in that array was later deleted and
  then a further change was made to the dynsec config without having restarted
  the broker.

# Apps
- Ensure mosquitto_ctrl checks that all string parameters are strings.

# Build:
- Fix test when nbuilding with WITH_EDITLINE=no. Closes #3484.
- Fix tests when building with WITH_WEBSOCKETS=no. Closes #3502.
- Fix libmosquitto_static cmake build.
- Enable WITH_UNIX_SOCKETS on Windows.

--

# 2.0.23 changes 
# Broker
- Fix handling of disconnected sessions for `per_listener_settings true`
- Check return values of openssl `*_get_ex_data()` and `*_set_ex_data()` to
  prevent possible crash. This could occur only in extremely unlikely
  situations. Closes #3389.
- Check return value of openssl ASN1_string_[get0_]data() functions for NULL.
  This prevents a crash in case of incorrect certificate handling in openssl.
  Closes #3390.
- Fix potential crash on startup if a malicious/corrupt persistence file from
  mosquitto 1.5 or earlier is loaded. Closes #3439.
- Limit auto_id_prefix to 50 characters. Closes #3440.
- Fix max_connections not being honoured on websockets listeners. Closes #3455.
- Fix outgoing maximum-packet-size limit check. Closes #3503.
- Fix creation of log and runtime directories in systemd unit files.
  Closes #3344.
- Fix will messages being incorrectly delayed if a client set
  session-expiry-interval=0 when using will-delay-interval>0. Closes #3505.

# Library
- Fix mosquitto_loop_start() leaving the mosq stuct in an invalid state if
  thread creation fails. Closes #3496.
- Fix will messages being incorrectly delayed if a client set
  session-expiry-interval=0 when using will-delay-interval>0. Closes #3505.

# Windows
- Installer will not overwrite an existing mosquitto.conf

[#3344]: https://github.com/eclipse-mosquitto/mosquitto/issues/3344
[#3389]: https://github.com/eclipse-mosquitto/mosquitto/issues/3389
[#3390]: https://github.com/eclipse-mosquitto/mosquitto/issues/3390
[#3439]: https://github.com/eclipse-mosquitto/mosquitto/issues/3439
[#3440]: https://github.com/eclipse-mosquitto/mosquitto/issues/3440
[#3455]: https://github.com/eclipse-mosquitto/mosquitto/issues/3455
[#3483]: https://github.com/eclipse-mosquitto/mosquitto/issues/3483
[#3484]: https://github.com/eclipse-mosquitto/mosquitto/issues/3484
[#3487]: https://github.com/eclipse-mosquitto/mosquitto/issues/3487
[#3490]: https://github.com/eclipse-mosquitto/mosquitto/issues/3490
[#3492]: https://github.com/eclipse-mosquitto/mosquitto/issues/3492
[#3496]: https://github.com/eclipse-mosquitto/mosquitto/issues/3496
[#3498]: https://github.com/eclipse-mosquitto/mosquitto/issues/3498
[#3502]: https://github.com/eclipse-mosquitto/mosquitto/issues/3502
[#3503]: https://github.com/eclipse-mosquitto/mosquitto/issues/3503
[#3505]: https://github.com/eclipse-mosquitto/mosquitto/issues/3505
[#3515]: https://github.com/eclipse-mosquitto/mosquitto/issues/3515
