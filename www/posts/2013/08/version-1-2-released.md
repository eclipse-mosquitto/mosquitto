<!--
.. title: Version 1.2 released
.. slug: version-1-2-released
.. date: 2013-08-07 23:13:34
.. tags: Releases
.. category:
.. link:
.. description:
.. type: text
-->

This is a (long overdue) feature release.

There is a potential gotcha when upgrading to this release because the default
version of TLS used has changed from 1.0 to 1.2. Python does not yet have
support for TLS&gt;1.0 so Python clients will be unable to communicate with
brokers using the default TLS settings.

The source is available at the [download page] and binaries will become
available in the near future.

# Broker

 * Replace O(n) username lookup on CONNECT with a roughly O(1) hashtable
   version.
 * It is now possible to disable $SYS at compile time.
 * Add dropped publish messages to load tree in $SYS. Closes bug #1183318.
 * Add support for logging SUBSCRIBE/UNSUBSCRIBE events.
 * Add `log_dest file` logging support.
 * Auth plugin ACL check function now passes the client id as well as username
   and password.
 * The `queue_qos0_messages` option wasn't working correctly, this has now been
   fixed. Closes bug #1125200.
 * Don't drop all messages for disconnected durable clients when
   `max_queued_messages=0`.
 * Add support for `log_type all`.
 * Add support for `-v` option on the command line to provide the equivalent of
   `log_type all` without needing a config file.
 * Add the `upgrade_outgoing_qos` option, a non-standard feature.
 * Persistence data is now written to a temporary file which is atomically
   renamed on completion, so a crash during writing will not produce a corrupt
   file.
 * mosquitto.conf is now installed as mosquitto.conf.example
 * Configuration file errors are now reported with filename and line number.
 * The broker now uses a monotonic clock if available, to avoid changes in time
   causing client disconnections or message retries.
 * Clean session and keepalive status are now display the log when a client
   connects.
 * Add support for TLSv1.2 and TLSv1.1.
 * Clients that connect with zero length will topics are now rejected.
 * Add the ability to set a maximum allowed PUBLISH payload size.
 * Fix an ACL with topic `#` incorrectly granting access to $SYS.
 * Fix retained messages incorrectly being set on wildcard topics, leading to
   duplicate retained messages being sent on subscription. Closes bug #1116233.
 * Don't discard listener values when no "port" option given. Closes bug
   #1131406.
 * Client password check was always failing when security was being reapplied
   after a config reload. This meant that all clients were being disconnected.
   This has been fixed.
 * Fix build when `WITH_TLS=no`. Closes bug #1174971.
 * Fix single outgoing packets not being sent in a timely fashion if they were
   not sent in one call to write(). Closes bug #1176796.
 * Fix remapping of messages for clients connected to a listener with
   `mount_point` set. Closes bug #1180765.
 * Fix duplicate retained messages being sent for some wildcard patterns.
 * If a client connects with a will topic to which they do not have write
   access, they are now disconnected with CONNACK "not authorised".
 * Fix retained messages on topic foo being incorrectly delivered to
   subscriptions of /#
 * Fix handling of SSL errors on SSL_accept().
 * Fix handling of QoS 2 messages on client reconnect.
 * Drop privileges now sets supplementary groups correctly.
 * Fix load reporting interval (is now 60s).
 * Be strict with malformed PUBLISH packets - clients are now disconnected
   rather than the packet discarded. This goes inline with future OASIS spec
   changes and makes other changes more straightforward.
 * Process incoming messages denied by ACL properly so that clients don't keep
   resending them.
 * Add support for `round_robin` bridge option.
 * Add bridge support for verifying remote server certificate subject against
   the remote hostname.
 * Fix problem with out of order calls to free() when restarting a lazy bridge.
 * The broker now attempts to resolve `bind_address` and bridge addresses
   immediately when parsing the config file in order to detect invalid hosts.
 * Bridges now set their notification state before attempting to connect, so if
   they fail to connect the state can still be seen.
 * Fix bridge notification payload length - no need to send a null byte.
 * mosquitto_passwd utility now reports errors more clearly.
 * Fix `mosquitto_passwd -U`.

 
# Client library

 * Add support for TLSv1.2 and TLSv1.1, except for on the Python module.
 * Add support for verifying remote server certificate subject against the
   remote hostname.
 * Add mosquitto_reconnect_async() support and make asynchronous connections
   truly asynchronous rather than simply deferred. DNS lookups are still
   blocking, so asynchronous connections require an IP address instead of
   hostname.
 * Allow control of reconnection timeouts in mosquitto_loop_forever() and after
   mosquitto_loop_start() by using mosquitto_reconnect_delay_set().
 * Fix building on Android NDK.
 * Re-raise unhandled errors in Python so as not to provide confusing error
   messages later on.
 * Python module supports IPv6 connections.
 * mosquitto_sub_topic_tokenise() was behaving incorrectly if the last topic
   hierarchy had only a single character. This has been fixed. Closes bug
   #1163348.
 * Fix possible crash after disconnects when using the threaded interface with
   TLS.
 * Allow build/install without Python. Closes bug #1174972.
 * Add support for binding connection to a local interface.
 * Implement maximum inflight messages handling.
 * Fix Python client not handling `will_payload==None`.
 * Fix potential memory leak when setting username/password.
 * Fix handling of QoS 2 messages on reconnect.
 * Improve handling of mosquitto_disconnect() with threaded mode.

# Clients

 * Add support for TLSv1.2 and TLSv1.1.
 * Sub client can now suppress printing of messages with the retain bit set.
 * Add support for binding connection to a local interface.
 * Implement maximum inflight messages handling for the pub client.

[download page]: /download
