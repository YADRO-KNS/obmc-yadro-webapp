# OpenBMC YADRO Web application #

This is an alternative implementation for the OpenBMC API web server, which does serve each public API.

## Improvements ##
+ Separate the HTTP protocol and OpenBMC business logic via lighttpd and fastcgi-server applications.
+ Indepeneds capsular implementation of a BL.
+ Caching each an API entity.
+ TODO

## Configuration

Build-time configuration of `obmc-yadro-webapp` is possible with meson options:
```
meson -Dkvm=disabled
```
See obmc-webserver/meson_options.txt for the list of available configuration options
The option names become C++ preprocessor symbols that control which code is compiled into the program.

### Compile obmc-yadro-webapp with default options:
```ascii
meson builddir
ninja -C builddir
```
### Compile obmc-yadro-webapp with yocto defaults:
```ascii
meson builddir -Dbuildtype=minsize -Db_lto=true -Dtests=disabled
ninja -C buildir
```

### Enable/Disable meson wrap feature
```ascii
meson builddir -Dwrap_mode=nofallback
ninja -C builddir
```

### Certificates
TODO
