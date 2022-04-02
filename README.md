# FreeBSD libbe(3) Extension for PHP

This PHP extension is a 1:1 wrapper for the FreeBSD libbe(3) library.

The FreeBSD libbe(3) is a library for creating, destroying and modifying ZFS boot environments. 

The official documentation for libbe(3) can be found [here](https://www.freebsd.org/cgi/man.cgi?query=libbe&sektion=3&manpath=FreeBSD+13.0-RELEASE+and+Ports).

## Build

```
% git clone https://github.com/theonemcdonald/php-ext-libbe.git
% cd php-ext-libbe
% phpize
% ./configure
% make
$ make install
```
