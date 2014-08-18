open62541
=========

An open-source communication stack implementation of OPC UA (OPC Unified Architecture) licensed under LGPL + static linking exception.

[![Ohloh Project Status](https://www.ohloh.net/p/open62541/widgets/project_thin_badge.gif)](https://www.ohloh.net/p/open62541)
[![Build Status](https://travis-ci.org/acplt/open62541.png?branch=master)](https://travis-ci.org/acplt/open62541)
[![Coverage Status](https://coveralls.io/repos/acplt/open62541/badge.png?branch=master)](https://coveralls.io/r/acplt/open62541?branch=master)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/1864/badge.svg)](https://scan.coverity.com/projects/1864)

### Documentation
Documentation is generated from Doxygen annotations in the source code. The current version can be accessed at [http://open62541.org/doxygen/](http://open62541.org/doxygen/).

## Build Procedure
### Ubuntu

#### Install build infrastructure
```bash
sudo apt-get install git build-essential gcc cmake python python-lxml
```

#####  Notes on older systems e.g. 12.04 LTS
* Manual install of check framework 0.9.10 (symptoms like "error: implicit declaration of function ‘ck_assert_ptr_eq’")
```bash
wget http://security.ubuntu.com/ubuntu/pool/main/c/check/check_0.9.10-6ubuntu3_amd64.deb
sudo dpkg -i check_0.9.10-6ubuntu3_amd64.deb
```
or for i386
```bash
wget http://security.ubuntu.com/ubuntu/pool/main/c/check/check_0.9.10-6ubuntu3_i386.deb
sudo dpkg -i check_0.9.10-6ubuntu3_386.deb
```
* Manuall install of gcc-4.8 (symptoms like "error: initialization discards ‘const’ qualifier from pointer target type [-Werror]")
```bash
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update; sudo apt-get install gcc-4.8
sudo update-alternatives --remove-all gcc 
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 20
sudo update-alternatives --config gcc
```

#### Build
```bash
git clone https://github.com/acplt/open62541.git
cd open62541
mkdir build
cmake .. # generate the build scripts
# Optionally create an Eclipse project: cmake -G "Eclipse CDT4 - Unix Makefiles" .. 
make # creates executables in the build directory

# enable additional features
sudo apt-get install libexpat1-dev # for XML-encoding
sudo apt-get install liburcu-dev # for multithreading
sudo apt-get install check # for unit tests
sudo apt-get install graphviz doxygen # for documentation generation

ccmake .. # to select features for compilation. Use "cmake-gui .." for more eye-candy
make
make test # unit tests
make doc # generate documentation
```

### Windows (Visual Studio)
* Get and install Python 2.7.x and CMake: https://python.org/downloads, http://www.cmake.org/cmake/resources/software.html
* Get and install Visual Studio 2013 (Express): http://www.microsoft.com/en-US/download/details.aspx?id=40787
* Download the open62541 sources (using git or as a zipfile from github)
* Add the folder with the python executable to the Windows PATH (System Settings)
* Download https://bootstrap.pypa.io/get-pip.py
* Open a command shell (cmd) and run
```Batchfile
python <path-to>\get-pip.py
python -m pip install lxml
cd <path-to>\open62541
mkdir build
cd build
<path-to>\cmake.exe .. -G "Visual Studio 12 2013"
open "build\open62541.sln" in Visual Studio 2013 and build as usual
```

### Windows (MinGW)
* Execute the same steps as in the Visual Studio case. But instead of installing Visual Studio, get and install MinGW:
   * Get the latest MinGW installer: http://sourceforge.net/projects/mingw/files/latest/download?source=files
   * Select following packages: mingw-developer-toolkit, mingw32-base, msys-base
   * After install, run MinGW\msys\1.0\postinstall\pi.bat
* The cmake command changes to
```Batchfile
<path-to>\cmake.exe .. -G "MinGW Makefiles"
```
* Then run (still in the build folder)
```Batchfile
<path-to>\mingw32-make.exe
```

#### Install pkg-config (for CMake)
* Download http://win32builder.gnome.org/packages/3.6/pkg-config_0.28-1_win32.zip and unpack bin/pkg-config.exe to <MinGW-path>/bin
* Download http://ftp.gnome.org/pub/gnome/binaries/win32/glib/2.28/glib_2.28.8-1_win32.zip and unzip bin/libglib-2.0-0.dll to <MinGW-path>/bin
* Download http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/gettext-runtime_0.18.1.1-2_win32.zip and unzip bin/intl.dll to <MinGW-path>/bin

#### Get expat
* Start MinGW Installation Manager ```mingw-get.exe```
* Choose all Packages, mark mingw32-expat and install

#### Get the *check* unit testing framework
* Download check from http://check.sourceforge.net/
* Open MinGW\msys\1.0\msys.bat
```bash
cd check-code
autoreconf --install
mount c:/<MinGW-path> /mingw
./configure --prefix=/mingw CC=mingw32-gcc
make
make install
```
