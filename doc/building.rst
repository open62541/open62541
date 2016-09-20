.. _building:

Building open62541
==================

Building the Examples
---------------------

Using the GCC compiler, the following calls build the examples on Linux.

.. code-block:: bash

   cp /path-to/open62541.* . # copy single-file distribution to the local directory
   cp /path-to/examples/server_variable.c . # copy the example server
   gcc -std=c99 open62541.c server_variable.c -o server

Building the Library
--------------------

Building with CMake on Ubuntu or Debian
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   sudo apt-get install git build-essential gcc pkg-config cmake python

   # enable additional features
   sudo apt-get install liburcu-dev # for multithreading
   sudo apt-get install check # for unit tests
   sudo apt-get install sphinx graphviz # for documentation generation
   sudo apt-get install python-sphinx-rtd-theme # documentation style

   cd open62541
   mkdir build
   cd build
   cmake ..
   make

   # select additional features
   ccmake ..
   make

Building with CMake on Windows
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here we explain the build process for Visual Studio (2013 or newer). To build
with MinGW, just replace the compiler selection in the call to CMake.

- Download and install

  - Python 2.7.x (Python 3.x should work, too): https://python.org/downloads
  - CMake: http://www.cmake.org/cmake/resources/software.html
  - Microsoft Visual Studio 2015 Community Edition: https://www.visualstudio.com/products/visual-studio-community-vs

- Download the open62541 sources (using git or as a zipfile from github)
- Open a command shell (cmd) and run

.. code-block:: bat

   cd <path-to>\open62541
   mkdir build
   cd build
   <path-to>\cmake.exe .. -G "Visual Studio 14 2015"
   :: You can use use cmake-gui for a graphical user-interface to select features

- Then open :file:`build\open62541.sln` in Visual Studio 2015 and build as usual

Building on OS X
^^^^^^^^^^^^^^^^

- Download and install

  - Xcode: https://itunes.apple.com/us/app/xcode/id497799835?ls=1&mt=12
  - Homebrew: http://brew.sh/
  - Pip (a package manager for python, may be preinstalled): ``sudo easy_install pip``

- Run the following in a shell

.. code-block:: bash

   brew install cmake
   pip install sphinx # for documentation generation
   pip install sphinx_rtd_theme # documentation style
   brew install graphviz # for graphics in the documentation
   brew install check # for unit tests
   brew install userspace-rcu # for multi-threading support

Follow Ubuntu instructions without the ``apt-get`` commands as these are taken care of by the above packages.

Building on OpenBSD
-------------------
The procedure below works on OpenBSD 5.8 with gcc version 4.8.4, cmake version 3.2.3 and Python version 2.7.10.

- Install a recent gcc, python and cmake:

.. code-block:: bash
   
   pkg_add gcc python cmake

- Tell the system to actually use the recent gcc (it gets installed as egcc on OpenBSD): 

.. code-block:: bash
   
   export CC=egcc CXX=eg++

- Now procede as described for Ubuntu/Debian:

.. code-block:: bash

   cd open62541
   mkdir build
   cd build
   cmake ..
   make

Build Options
-------------

Build Type and Logging
^^^^^^^^^^^^^^^^^^^^^^

**CMAKE_BUILD_TYPE**
  - ``RelWithDebInfo`` -O2 optimization with debug symbols
  - ``Release`` -O2 optimization without debug symbols
  - ``Debug`` -O0 optimization with debug symbols
  - ``MinSizeRel`` -Os optimization without debug symbols

**UA_LOGLEVEL**
   The level of logging events that are reported

     - 600: Fatal and all below
     - 500: Error and all below
     - 400: Error and all below
     - 300: Info and all below
     - 200: Debug and all below
     - 100: Trace and all below

Further options that are not inherited from the CMake configuration are defined
in :file:`ua_config.h`. Usually there is no need to adjust them.

UA_BUILD_* group
^^^^^^^^^^^^^^^^

By default only the shared object libopen62541.so or the library open62541.dll
and open62541.dll.a resp. open62541.lib are build. Additional artifacts can be
specified by the following options:

**UA_BUILD_DOCUMENTATION**
  Generate Make targets for documentation

   * HTML documentation: ``make doc``
   * Latex Files: ``latex``
   * PDF documentation: ``make pdf``

**UA_BUILD_EXAMPLES**
   Compile example servers and clients from :file:`examples/{xyz}.c`. A static and a dynamic binary is linked, respectively.

**UA_BUILD_UNIT_TESTS**
   Compile unit tests with Check framework. The tests can be executed with ``make test``

**UA_BUILD_EXAMPLES_NODESET_COMPILER**
   Generate an OPC UA information model from a nodeset XML (experimental)

**UA_BUILD_SELFIGNED_CERTIFICATE**
   Generate a self-signed certificate for the server (openSSL required)

UA_ENABLE_* group
^^^^^^^^^^^^^^^^^

This group contains build options related to the supported OPC UA features.

**UA_ENABLE_SUBSCRIPTIONS**
   Enable subscriptions
**UA_ENABLE_METHODCALLS**
   Enable the Method service set
**UA_ENABLE_NODEMANAGEMENT**
   Enable dynamic addition and removal of nodes at runtime
**UA_ENABLE_AMALGAMATION**
   Compile a single-file release files :file:`open62541.c` and :file:`open62541.h`
**UA_ENABLE_MULTITHREADING**
   Enable multi-threading support
**UA_ENABLE_COVERAGE**
   Measure the coverage of unit tests

Some options are marked as advanced. The advanced options need to be toggled to
be visible in the cmake GUIs.

**UA_ENABLE_TYPENAMES**
   Add the type and member names to the UA_DataType structure
**UA_ENABLE_GENERATE_NAMESPACE0**
   Generate and load UA XML Namespace 0 definition
   ``UA_GENERATE_NAMESPACE0_FILE`` is used to specify the file for NS0 generation from namespace0 folder. Default value is ``Opc.Ua.NodeSet2.xml``
**UA_ENABLE_EMBEDDED_LIBC**
   Use a custom implementation of some libc functions that might be missing on embedded targets (e.g. string handling).
**UA_ENABLE_EXTERNAL_NAMESPACES**
  Enable namespace handling by an external component (experimental)
**UA_ENABLE_NONSTANDARD_STATELESS**
   Enable stateless extension
**UA_ENABLE_NONSTANDARD_UDP**
   Enable udp extension

Building a shared library
^^^^^^^^^^^^^^^^^^^^^^^^^

open62541 is small enough that most users will want to statically link the library into their programs. If a shared library (.dll, .so) is required, this can be enabled in CMake with the `BUILD_SHARED_LIBS` option.
Note that this option modifies the :file:`ua_config.h` file that is also included in :file:`open62541.h` for the single-file distribution.
