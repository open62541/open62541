Building the Library
====================

Building the Single-File Release
--------------------------------

Using the GCC compiler, the following calls build the library on Linux.

.. code-block:: bash

   gcc -std=c99 -fPIC -c open62541.c
   gcc -shared open62541.o -o libopen62541.so
   

Building with CMake on Ubuntu or Debian
---------------------------------------

.. code-block:: bash
   
   sudo apt-get install git build-essential gcc pkg-config cmake python python-lxml

   # enable additional features
   sudo apt-get install libexpat1-dev # for XML-encodingi
   sudo apt-get install liburcu-dev # for multithreading
   sudo apt-get install check # for unit tests
   sudo apt-get install graphviz doxygen # for documentation generation

   cd open62541
   mkdir build
   cd build
   cmake ..
   make

   # select additional features
   ccmake ..
   make

Building with CMake on Windows
------------------------------

Here we explain the build process for Visual Studio. To build with MinGW, just
replace the compiler selection in the call to CMake.

- Download and install

  - Python 2.7.x (Python 3.x should work, too): https://python.org/downloads
  - Python lxml: https://pypi.python.org/pypi/lxml
  - CMake: http://www.cmake.org/cmake/resources/software.html
  - Microsoft Visual Studio 2015 Community Edition: https://www.visualstudio.com/products/visual-studio-community-vs
    
- Download the open62541 sources (using git or as a zipfile from github)
- Open a command shell (cmd) and run

.. code-block:: bat

   cd <path-to>\open62541
   mkdir build
   cd build
   <path-to>\cmake.exe .. -G "Visual Studio 14 2015"
   :: You can use use cmake-gui for a graphical user-interface to select single features

- Then open "build\open62541.sln" in Visual Studio 2015 and build as usual

Building on OS X
----------------

- Download and install

  - Xcode: https://itunes.apple.com/us/app/xcode/id497799835?ls=1&mt=12
  - Homebrew: http://brew.sh/
  - Pip (a package manager for python, may be preinstalled): ``sudo easy_install pip``

- Run the following in a shell

.. code-block:: bash

   brew install cmake
   brew install libxml2
   pip install lxml
   brew install check # for unit tests
   brew install userspace-rcu # for multi-threading support
   brew install graphviz doxygen # for documentation generation
   pip install sphinx # for documentation generation

Follow Ubuntu instructions without the ``apt-get`` commands as these are taken care of by the above packages.
   
Build Options
-------------

Generic CMake options
~~~~~~~~~~~~~~~~~~~~~

**CMAKE_BUILD_TYPE**
  - RelWithDebInfo: -O2 optimization with debug symbols
  - Release: -O2 optimization without debug symbols
  - Debug: -O0 optimization with debug symbols
  - MinSizeRel: -Os optimization without debug symbols

BUILD_* group
~~~~~~~~~~~~~

By default only the shared object libopen62541.so or the library open62541.dll
and open62541.dll.a resp. open62541.lib are build. Additional artifacts can be
specified by the following options:

**BUILD_DOCUMENTATION**
   Generate documentation with doxygen
**BUILD_EXAMPLECLIENT**
   Compile example clients from client.c. There are a static and a dynamic binary client and client_static, respectively
**BUILD_EXAMPLESERVER**
   Compile example server from server.c There are a static and a dynamic binary server and server_static, respectively
**BUILD_UNIT_TESTS**
   Compile unit tests with Check framework. The tests can be executed with make test
**BUILD_EXAMPLES**
   Compile specific examples from https://github.com/acplt/open62541/blob/master/examples/

ENABLE_* group
~~~~~~~~~~~~~~

This group contains build options related to the supported OPC UA features.

**ENABLE_ADDNODES**
   AddNodes services in sever and client
**ENABLE_AMALGAMATION**
   Compile a single-file release files open62541.c and open62541.h
**ENABLE_COVERAGE**
   Measure the coverage of unit tests
**ENABLE_EXTERNAL_NAMESPACES**
   Enable external namespaces in server
**ENABLE_GNERATE_NAMESPACE0**
   Enable automatic generation of NS0
**GENERATE_NAMESPACE0_FILE**
   File for NS0 generation from namespace0 folder. Default value is Opc.Ua.NodeSet2.xml
**ENABLE_METHODCALL**
   Enable method calls in server and client
**ENABLE_MULTITHREADING**
   Enable multi-threading support (experimental)
**ENABLE_SUBSCRIPTONS**
   Enable subscriptions
**GENERATE_SELFSIGNED**
   Generate a self-signed certificate for the server (openSSL required)

EXTENSION_* group
~~~~~~~~~~~~~~~~~

Extensions mark experimental extensions that are not compliant with IEC 62541 or
other OPC UA clients

**EXTENSION_STATELESS**
   Stateless service calls
**EXTENSION_UDP**
   UDP network layer

UA_* group
~~~~~~~~~~

**UA_LOGLEVEL**
   The level of logging events that are reported
   - 600: Fatal and all below
   - 500: Error and all below
   - 400: Error and all below
   - 300: Info and all below
   - 200: Debug and all below
   - 100: Trace and all below

C precompiler configuration options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Options that are not inherited from the CMake configuration are defined in
ua_config.h. Usually there is no need to adjust them.

**UA_NON_LITTLEENDIAN_ARCHITECTURE**
   Big-endian or mixed endian platform
**UA_MIXED_ENDIAN**
   Mixed-endian platform (e.g., ARM7TDMI)
**UA_ALIGNED_MEMORY_ACCESS**
   Platform with aligned memory access only (some ARM processors, e.g. Cortex M3/M4 ARM7TDMI etc.)
