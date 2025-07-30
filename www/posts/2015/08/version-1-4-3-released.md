<!--
.. title: Version 1.4.3 released
.. slug: version-1-4-3-released
.. date: 2015-08-19 15:42:27
.. tags: Releases
.. category:
.. link:
.. description:
.. type: text
-->

This is a bugfix release.

# Broker

 * Fix incorrect bridge notification on initial connection. Closes [#467096].
 * Build fixes for OpenBSD.
 * Fix incorrect behaviour for `autosave_interval`, most noticeable for
   `autosave_interval=1`. Closes [#465438].
 * Fix handling of outgoing QoS&gt;0 messages for bridges that could not be
   sent because the bridge connection was down.
 * Free unused topic tree elements. Closes [#468987].
 * Fix some potential memory leaks. Closes [#470253].
 * Fix potential crash on libwebsockets error.

# Client library

 * Add missing error strings to `mosquitto_strerror`.
 * Handle fragmented TLS packets without a delay. Closes [#470660].
 * Fix incorrect loop timeout being chosen when using threaded interface and
   keepalive = 0. Closes [#471334].
 * Increment inflight messages count correctly. Closes [#474935].

# Clients

 * Report error string on connection failure rather than error code.

[#467096]: https://bugs.eclipse.org/bugs/show_bug.cgi?id=467096
[#465438]: https://bugs.eclipse.org/bugs/show_bug.cgi?id=465438
[#468987]: https://bugs.eclipse.org/bugs/show_bug.cgi?id=468987
[#470253]: https://bugs.eclipse.org/bugs/show_bug.cgi?id=470253
[#470660]: https://bugs.eclipse.org/bugs/show_bug.cgi?id=470660
[#471334]: https://bugs.eclipse.org/bugs/show_bug.cgi?id=471334
[#474935]: https://bugs.eclipse.org/bugs/show_bug.cgi?id=474935
