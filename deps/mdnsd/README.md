mdnsd - embeddable Multicast DNS Daemon
=======================================

[![Ohloh Project Status](https://www.ohloh.net/p/mdnsd_pro/widgets/project_thin_badge.gif)](https://www.ohloh.net/p/mdnsd_pro)
[![Build Status](https://travis-ci.org/Pro/mdnsd.png?branch=master)](https://travis-ci.org/Pro/mdnsd)
[![Build status](https://ci.appveyor.com/api/projects/status/gv4lros88uubrkwd?svg=true)](https://ci.appveyor.com/project/Pro/mdnsd)

This package is intended for software developers and integrators, there
isn't really anything here for an end user.  The project license is the
modified 3-clause BSD, https://en.wikipedia.org/wiki/BSD_licenses

You should be able to just type make and it will build the included
example apps.  Otherwise, check out `mdnsd.h` to get started, the API is
as simple as I could make it, but I hope to find some easier/better ways
to improve it in the future.  Also included are some other utilities,
`sdtxt.*` for service discovery TXT record parsing/generation, and
`xht.*` for simple fast hashtables, and `1035.*` which mdnsd uses for
standalone DNS parsing.

**Differences to the base repo (https://github.com/troglobit/mdnsd)**:
* Use CMake for the build
* Strict compilation flags for better portability
* Support of Linux, MinGW, **OS X** and **Windows**
* Continuous Integration

Build & Install
---------------

The software is built for and developed on GNU/Linux systems, but should
work on any UNIX like system.

The GNU configure and build system is used, simply call the configure
script to generate a Makefile.  If you are using the GitHub sources you
first need to call `./autogen.sh` to generate the configure script.

    ./configure
    make all
    make install


Running
-------

To test the included example applications you need to first start the
`mdnsd` daemon before calling `mquery`:

    ./mdnsd _name._service 192.168.1.2 80 &
    ./mquery 12 _http._tcp.local.


Origin & References
-------------------

This MDNS-SD implementation was developed by [Jeremie Miller][jeremie]
in 2003, originally [announced on the rendezvous-dev][announced] mailing
list.  It has many forks and has been used by many other applications
over the years.

This GitHub project is an attempt to clean it up, develop it further,
and maintain it for the long haul.


[jeremie]:       https://github.com/quartzjer
[announced]:     http://lists.apple.com/archives/rendezvous-dev/2003/Feb/msg00062.html
[Travis]:        https://travis-ci.org/troglobit/mdnsd
[Travis Status]: https://travis-ci.org/troglobit/mdnsd.png?branch=master
