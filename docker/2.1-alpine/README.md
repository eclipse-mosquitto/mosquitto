# Eclipse Mosquitto Docker Image
Containers built with this Dockerfile build as source from published tarballs.

## Mount Points
The image uses three directories that are intended to be mounted from the host
or from a docker volume:
```
/mosquitto/config
/mosquitto/data
/mosquitto/log
```

The image does not declare any of these as a `VOLUME`, so docker will not
create anonymous volumes for them. Anything written to a directory that you have
not mounted is stored in the container's writable layer and is **lost when the
container is removed or recreated**, for example when updating to a new image.
If you enable persistence or file logging, or keep password, ACL or dynamic
security files in `/mosquitto/data`, mount that directory.

### Named volumes
Name the volumes however you like:
```
docker volume create mosquitto-data
docker volume create mosquitto-log
docker run -it -p 1883:1883 \
    -v mosquitto-data:/mosquitto/data \
    -v mosquitto-log:/mosquitto/log \
    eclipse-mosquitto:<version>
```

An empty named volume is populated with the contents and ownership of the
image directory the first time it is mounted.

### Bind mounts
```
docker run -it -p 1883:1883 \
    -v <absolute-path-to-config-directory>:/mosquitto/config \
    -v <absolute-path-to-data-directory>:/mosquitto/data \
    -v <absolute-path-to-log-directory>:/mosquitto/log \
    eclipse-mosquitto:<version>
```

Mosquitto runs as uid/gid 1883, so it must be able to write to the data and log
directories. When the container starts as root, the entrypoint changes the
ownership of `/mosquitto/data` to `PUID`:`PGID` (default 1883:1883). The log
directory is not changed, so make it writable for that user yourself, e.g.
`chown 1883:1883 <absolute-path-to-log-directory>`.

### Docker Compose
```
services:
  mosquitto:
    image: eclipse-mosquitto:<version>
    ports:
      - "1883:1883"
    volumes:
      - ./config:/mosquitto/config
      - mosquitto-data:/mosquitto/data
      - mosquitto-log:/mosquitto/log

volumes:
  mosquitto-data:
  mosquitto-log:
```

### Upgrading from an image that declared volumes
Earlier images declared `/mosquitto/data` and `/mosquitto/log` as volumes. If
you did not mount anything there yourself, your data is in an anonymous volume
attached to the old container. Copy it into a named volume before removing that
container:
```
docker volume create mosquitto-data
docker run --rm --volumes-from <old-container> -v mosquitto-data:/new alpine \
    cp -a /mosquitto/data/. /new/
```
Then start the new container with `-v mosquitto-data:/mosquitto/data`. The same
applies to `/mosquitto/log` if you need to keep old log files.

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

**Note**: Data is only kept between containers for directories that are mounted,
see [Mount Points](#mount-points).
