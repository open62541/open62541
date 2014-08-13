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
```bash
sudo apt-get install git build-essential gcc cmake python python-lxml # install build infrastructure

git clone https://github.com/acplt/open62541.git
cd open62541
mkdir build
cmake .. # generate the build scripts
make # creates executables in the build directory

# enable additional features
sudo apt-get install libexpat1-dev # for XML-encoding
sudo apt-get install liburcu-dev # for multithreading
sudo apt-get install check # for unit tests
sudo apt-get install graphviz doxygen # for documentation generation

ccmake .. # to select features for compilation. Use "cmake-gui .." for more eye-candy
make
```

### Windows (Visual Studio)
* Get and install Python and CMake: https://python.org/downloads, http://www.cmake.org/cmake/resources/software.html
* Get and install Visual Studio 2013 (Express): http://www.microsoft.com/en-US/download/details.aspx?id=40787
* Download the open62541 sources (using git or as a zipfile from github)
* Add the folder with the python executable to the Windows PATH (System Settings)
* Download https://bootstrap.pypa.io/get-pip.py
* Open a command shell (cmd) and run
```bash
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
```bash
<path-to>\cmake.exe .. -G "MinGW Makefiles"
```
* Then run (still in the build folder)
```bash
<path-to>/mingw32-make.exe
```

#### Get expat
* Start MinGW Installation Manager ```mingw-get.exe```
* Choose all Packages, mark mingw32-expat and install

#### Get the *check* unit testing framework
* Download check from http://check.sourceforge.net/
* Open MinGW\msys\1.0\msys.bat
```bash
cd check-code
autoreconf --install
./configure
make
make install
```
