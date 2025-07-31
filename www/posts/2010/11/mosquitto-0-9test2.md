<!--
.. title: Mosquitto 0.9~test2
.. slug: mosquitto-0-9test2
.. date: 2010-11-04 10:37:49
.. tags: Testing
.. category:
.. link:
.. description:
.. type: text
-->

Mosquitto 0.9, which I hope to release mid November, represents the most
significant change to mosquitto to date - the removal of sqlite as an absolute
dependency. In addition, this removes the dependency on the sqlite3-pcre
extension and on pcre. This gives a definite performance improvement, reduces
the amount of object code that needs loading by around 95%, reduces memory
usage and also makes it lots easier to compile on more unusual systems.

It's quite a substantial change though, so I've made a test release to
hopefully get some external testing. If you could give it a try and report back
that'd be great. The source is at <http://mosquitto.org/files/source/test/>.
(use the highest numbered version available). There are also Ubuntu packages
available at the [mosquitto-expt ppa] and binaries for Fedora, Mandriva,  SLES
and openSUSE at the [openSUSE build service]. If you'd like binaries for other
systems, please get in touch.

Note that this is a test release, not a release candidate - there are
definitely things that still need changing. The following list shows the points
I'm currently aware of:

 * Old style sqlite will be imported when the option is compiled in (enabled by
   default). This import currently only imports retained messages and durable
   subscriptions, but not queued messages.
 * ~~The `max_inflight_messages` and `max_queued_messages` config options are
   ignored and no maximum is applied.~~
 * ~~The CMake compilation scripts aren't updated.~~

# Update
I've uploaded test3 with a python fix, updated CMake scripts and fixed
`max_inflight_messages` and `max_queued_messages`.

[mosquitto-expt ppa]: https://launchpad.net/~mosquitto-dev/+archive/mosquitto-expt
[openSUSE build service]: https://build.opensuse.org/project/show?project=home%3Aoojah%3Amqtt_expt
