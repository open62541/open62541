Open62541
=========

An open-source communication stack implementation of OPC UA (OPC Unified Architecture) licensed under LGPL + static linking exception.

[![Ohloh Project Status](https://www.ohloh.net/p/open62541/widgets/project_thin_badge.gif)](https://www.ohloh.net/p/open62541)
[![Build Status](https://travis-ci.org/acplt/open62541.png?branch=master)](https://travis-ci.org/acplt/open62541)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/1864/badge.svg)](https://scan.coverity.com/projects/1864)

### Documentation
Documentation is generated from Doxygen annotations in the source code. The current version can be accessed at http://acplt.github.io/open62541/doxygen/.

## Getting dependencies
### Ubuntu
##### Getting gcc toolchain:
```bash
sudo apt-get install build-essential subversion git autoconf libtool pkg-config texinfo
```
##### Getting python toolchain for the 62541 structures code generator:
```bash
sudo apt-get install python python-lxml 
```
##### Getting additional libraries:
```bash
sudo apt-get install expat libexpat1-dev
```
##### Getting and installing *check* as unit test framework (http://check.sourceforge.net/):
```bash
$ svn checkout https://svn.code.sf.net/p/check/code/trunk check-code
$ cd check-code
$ autoreconf --install
$ ./configure
$ make
$ sudo make install
$ sudo ldconfig
```
##### Getting and using Doxygen
* install the needed packages
```bash
sudo apt-get install graphviz doxygen
```
* configure autotools, clean and build:
```bash
$ ./configure --enable-doxygen
$ make clean
$ make all
```
* the output is generated in doc/html/index.htm
* configure the output of Doxygen with doc/Doxygen.in file

### Windows
##### Getting MinGW and MSYS:
* Get the latest MinGW installer: http://sourceforge.net/projects/mingw/files/latest/download?source=files
* Select following packages: mingw-developer-toolkit, mingw32-base, msys-base
* After install, run MinGW\msys\1.0\postinstall\pi.bat

##### Get Gtk+ bundle (just for m4 marcros and pkg-config):
* Download http://ftp.gnome.org/pub/gnome/binaries/win32/gtk+/2.24/gtk+-bundle_2.24.10-20120208_win32.zip and extract it
* Copy gtk+/share/aclocal/*.m4 files to MinGW/share/aclocal
* Merge grk+ folder and MinGW\msys\1.0\ folder

##### Get expat
* start MinGW Installation Manager
* choose all Packages, mark mingw32-expat and install
* Open MinGW\msys\1.0\msys.bat
```bash
$ mingw-get install libexpat
```

##### Get Python and lxml:
* download Python (Windows x86 MSI Installer) at https://python.org/downloads (necessary version: 2.7.x)
* install the executable
* add the install directory (e. g. "c:\python27") to your windows path variable [Selectable in the setup-options]
* restart mingw console
* install lxml by either downloading and installing http://www.lfd.uci.edu/~gohlke/pythonlibs/#lxml (choose the version which fits    to your python installation (x86 version)) or by following the instructions
  given here: http://lxml.de/installation.html 
* add ("C:\Python27\Tools\Scripts") to your windows path variable


##### Get git (IMPORTANT: get 1.8.4, since 1.8.5.2 has a bug):
* http://code.google.com/p/msysgit/downloads/detail?name=Git-1.8.4-preview20130916.exe&can=2&q=

##### SVN:
* download: http://tortoisesvn.net/downloads.html
* install svn @ c:\MinGW\

##### Getting and installing *check* as unit testing framework (http://check.sourceforge.net/):
* Open MinGW\msys\1.0\msys.bat

```bash
$ svn checkout svn://svn.code.sf.net/p/check/code/trunk check-code
$ cd check-code
$ autoreconf --install
$ ./configure
$ make
$ make install
```

##### Adjusting MinGW
* open the file c:\MinGW\include\io.h and replace every off64_t with _off64_t (4x should off64_t appear)
* open the file c:\MinGW\include\unistd.h and replace every off_t with _off_t (2x should off_t appear)
* download the queue.h header @ http://cvsweb.netbsd.org/bsdweb.cgi/src/sys/sys/queue.h and copy it to c:\MinGW\include\sys

## Building 
* use autogen.sh only first time and whenever aclocal.m4 or configure.ac were modified
```bash
$ cd open62541
$ ./autogen.sh
$ ./configure --enable-debug=yes
$ make
$ make check
```

### Configure Options 

* --enable-debug=(yes|no|verbose) - omit/include debug code
* --enable-multithreading - enable pthreads (for examples/src/opcuaServerMT)
* --enable-doxygen - make documentation as well
* --enable-coverage - profiling with gcov,lcov, make check will generate reports in tests/coverage 
