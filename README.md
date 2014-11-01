open62541
=========

An open-source communication stack implementation of OPC UA (OPC Unified Architecture) licensed under LGPL + static linking exception.

[![Ohloh Project Status](https://www.ohloh.net/p/open62541/widgets/project_thin_badge.gif)](https://www.ohloh.net/p/open62541)
[![Build Status](https://travis-ci.org/acplt/open62541.png?branch=master)](https://travis-ci.org/acplt/open62541)
[![Coverage Status](https://coveralls.io/repos/acplt/open62541/badge.png?branch=master)](https://coveralls.io/r/acplt/open62541?branch=master)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/1864/badge.svg)](https://scan.coverity.com/projects/1864)

### What is currently working?
The project is in an early stage. We retain the right to break APIs until a first stable release.
- Binary serialization of all data types: 100%
- Node storage and setup of a minimal namespace zero: 100%
- Standard OPC UA Clients can connect/open a SecureChannel/open a Session: 100%
- Browsing/reading and writing attributes: 100%
- Advanced SecureChannel and Session Management: 75%

### Documentation
Documentation is generated from Doxygen annotations in the source code. The current version can be accessed at [http://open62541.org/doxygen/](http://open62541.org/doxygen/).

### Example Server Implementation
```c
#include <stdio.h>
#include <signal.h>

// provided by the open62541 lib
#include "ua_server.h"
#include "ua_namespace_0.h"

// provided by the user, implementations available in the /examples folder
#include "logger_stdout.h"
#include "networklayer_tcp.h"

UA_Boolean running = UA_TRUE;
void stopHandler(int sign) {
	running = UA_FALSE;
}

void serverCallback(UA_Server *server) {
    // add your maintenance functionality here
    printf("does whatever servers do\n");
}

int main(int argc, char** argv) {
	signal(SIGINT, stopHandler); /* catches ctrl-c */

    /* init the server */
	UA_String endpointUrl;
	UA_String_copycstring("opc.tcp://192.168.56.101:16664",&endpointUrl);
	UA_Server *server = UA_Server_new(&endpointUrl, NULL);

    /* add a variable node */
    UA_Int32 myInteger = 42;
    UA_String myIntegerName;
    UA_STRING_STATIC(myIntegerName, "The Answer");
    UA_Server_addScalarVariableNode(server,
                                    /* The browse name, the value and the datatype's vtable*/
                                    &myIntegerName, (void*)&myInteger, &UA_TYPES[UA_INT32],

                                    /* The parent node to which the variable shall be attached */
                                    &UA_NODEIDS[UA_OBJECTSFOLDER],
                                    
                                    /* The (hierarchical) reference type from the "parent" node*/
                                    &UA_NODEIDS[UA_HASCOMPONENT]);

    /* attach a network layer */
	#define PORT 16664
	NetworklayerTCP* nl = NetworklayerTCP_new(UA_ConnectionConfig_standard, PORT);
	printf("Server started, connect to to opc.tcp://127.0.0.1:%i\n", PORT);
	struct timeval callback_interval = {1, 0}; // 1 second

    /* run the server loop */
	NetworkLayerTCP_run(nl, server, callback_interval, serverCallback, &running);
    
    /* clean up */
	NetworklayerTCP_delete(nl);
	UA_Server_delete(server);
    UA_String_deleteMembers(&endpointUrl);
	return 0;
}
```

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
