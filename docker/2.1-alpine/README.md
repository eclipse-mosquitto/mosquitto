# Eclipse Mosquitto Docker Image
Containers built with this Dockerfile build as source from published tarballs.

## Mount Points
A docker mount point has been created in the image to be used for configuration.
```
/mosquitto/config
```

Two docker volumes have been created in the image to be used for persistent storage and logs.
```
/mosquitto/data
/mosquitto/log
```

## User/Group

The image runs mosquitto under the mosquitto user and group, which are created
with a uid and gid of 1883.

## Running without a configuration file
Mosquitto 2.0 and up requires you to configure listeners and authentication
before it will allow connections from anything other than the loopback
interface. In the context of a container, this means you would normally need to
provide a configuration file with your settings.

However, this container provides a default configuration which listens on port
1883 for unauthenticated access, and port 9883 for the local http dashboard.
If you wish to run mosquitto without any authentication, and without setting
any other configuration options, you can run without a configuration by binding
the appropriate network ports:
```
docker run -it -p 1883:1883 -p localhost:9883:9883 eclipse-mosquitto:<version>
```

## Configuration
To use a custom configuration file, create a **local** config directory with a
mosquitto.conf inside, then mount this directory to `/mosquitto/config`

```
docker run -it -p 1883:1883 -v <absolute-path-to-config-directory>:/mosquitto/config eclipse-mosquitto:<version>
```

Your configuration file must include a `listener`, and you must configure some
form of authentication or allow unauthenticated access. If you do not do this,
clients will be unable to connect.


File based authentication and authorisation:
```
listener 1883
plugin /usr/lib/mosquitto_password_file.so
plugin_opt_password_file /mosquitto/data/mosquitto.password_file

plugin /usr/lib/mosquitto_acl_file.so
plugin_opt_acl_file /mosquitto/data/mosquitto.aclfile
```

Plugin based authentication and authorisation:
```
listener 1883
plugin /usr/lib/mosquitto_dynamic_security.so
plugin_opt_config_file /mosquitto/data/mosquitto-dynsec.json
```

Unauthenticated access:
```
listener 1883
allow_anonymous true
```

:boom: if the mosquitto configuration (mosquitto.conf) was modified
to use non-default ports, the docker run command will need to be updated
to expose the ports that have been configured, for example:

```
docker run -it -p 1883:1883 -p 8080:8080 -v <absolute-path-to-config-directory>:/mosquitto/config eclipse-mosquitto:<version>
```

Configuration can be changed to:

* persist data to `/mosquitto/data`
* log to `/mosquitto/log/mosquitto.log`

i.e. add the following to `mosquitto.conf`:
```
persistence_location /mosquitto/data/
plugin /usr/lib/mosquitto_persist_sqlite.so

log_dest file /mosquitto/log/mosquitto.log
```

**Note**: For any volume used, the data will be persistent between containers.
