# OpenBMC webserver #

This is an alternative implementation of the OpenBMC web server, which does serve each public API.


## Improvements ##
+ Separate the HTTP protocol and OpenBMC business logic via lighttpd and fastcgi-server applications.
+ Indepeneds capsular implementation of a BL.
+ Caching each an API entity.
+ TODO

## Configuration

obmc-web server is configured by setting `-D` flags that correspond to options in
`obmc-webserver/meson_options.txt` and then compiling.  For example, `meson <builddir> -Dkvm=disabled ...`
followed by `ninja` in build directory.
The option names become C++ preprocessor symbols that control which code is compiled into the program.

### Compile obmc-webserver with default options:
```ascii
meson builddir
ninja -C builddir
```
### Compile obmc-webserver with yocto defaults:
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
