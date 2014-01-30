Open62541
=========

An open communication stack implementation of OPC UA.

# Getting dependencies
### Ubuntu
```bash
sudo apt-get install build-essential check
```
### Windows
##### Getting MinGW and MSYS:
* Get the latest MinGW installer: http://sourceforge.net/projects/mingw/files/latest/download?source=files
* Select following packages: mingw-developer-toolkit, mingw32-base, msys-base
* After install, run MinGW\msys\1.0\postinstall\pi.bat

##### Get Gtk+ bundle (just for m4 marcros and pkg-config):
* Download http://ftp.gnome.org/pub/gnome/binaries/win32/gtk+/2.24/gtk+-bundle_2.24.10-20120208_win32.zip and extract it
* Copy gtk+/share/aclocal/*.m4 files to MinGW/share/aclocal
* Merge grk+ folder and MinGW\msys\1.0\ folder

##### Get git (IMPORTANT: get 1.8.4, since 1.8.5.2 has a bug):
* http://code.google.com/p/msysgit/downloads/detail?name=Git-1.8.4-preview20130916.exe&can=2&q=

##### Getting check unit test framework:
* Open MinGW\msys\1.0\msys.bat

```bash
$ svn checkout svn://svn.code.sf.net/p/check/code/trunk check-code
$ autoreconf --install
$ ./configure --prefix=
$ make
$ make install
```

# Building
```bash
$ cd OPCUAServer
$ ./configure
$ make
$ make check
```

### Rebuilding configure tools (only if configure.am is changed)
```bash
$ ./autogen.sh
```
