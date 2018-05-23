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

   sudo apt-get install git build-essential gcc pkg-config cmake python python-six

   # enable additional features
   sudo apt-get install cmake-curses-gui # for the ccmake graphical interface
   sudo apt-get install libmbedtls-dev # for encryption support
   sudo apt-get install check # for unit tests
   sudo apt-get install python-sphinx graphviz # for documentation generation
   sudo apt-get install python-sphinx-rtd-theme # documentation style

   cd open62541
   mkdir build
   cd build
   cmake ..
   make

   # select additional features
   ccmake ..
   make

   # build documentation
   make doc # html documentation
   make doc_pdf # pdf documentation (requires LaTeX)

Building with CMake on Windows
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here we explain the build process for Visual Studio (2013 or newer). To build
with MinGW, just replace the compiler selection in the call to CMake.

- Download and install

  - Python 2.7.x (Python 3.x works as well): https://python.org/downloads
  - Install python-six with the pip package manager (``pip install six``)
  - CMake: http://www.cmake.org/cmake/resources/software.html
  - Microsoft Visual Studio: https://www.visualstudio.com/products/visual-studio-community-vs

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
   pip install six # python 2/3 compatibility workarounds
   pip install sphinx # for documentation generation
   pip install sphinx_rtd_theme # documentation style
   brew install graphviz # for graphics in the documentation
   brew install check # for unit tests
   brew install userspace-rcu # for multi-threading support

Follow Ubuntu instructions without the ``apt-get`` commands as these are taken care of by the above packages.

Building on OpenBSD
^^^^^^^^^^^^^^^^^^^
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

The open62541 project uses CMake to manage the build options, for code
generation and to generate build projects for the different systems and IDEs.
The tools *ccmake* or *cmake-gui* can be used to graphically set the build
options.

Most options can be changed manually in :file:`ua_config.h` (:file:`open62541.h`
for the single-file release) after the code generation. But usually there is no
need to adjust them.

Build Type and Logging
^^^^^^^^^^^^^^^^^^^^^^

**CMAKE_BUILD_TYPE**
  - ``RelWithDebInfo`` -O2 optimization with debug symbols
  - ``Release`` -O2 optimization without debug symbols
  - ``Debug`` -O0 optimization with debug symbols
  - ``MinSizeRel`` -Os optimization without debug symbols

**UA_LOGLEVEL**
   The SDK logs events of the level defined in ``UA_LOGLEVEL`` and above only.
   The logging event levels are as follows:

     - 600: Fatal
     - 500: Error
     - 400: Warning
     - 300: Info
     - 200: Debug
     - 100: Trace

UA_BUILD_* group
^^^^^^^^^^^^^^^^

By default only the shared object libopen62541.so or the library open62541.dll
and open62541.dll.a resp. open62541.lib are build. Additional artifacts can be
specified by the following options:

**UA_BUILD_EXAMPLES**
   Compile example servers and clients from :file:`examples/{xyz}.c`. A static and a dynamic binary is linked, respectively.

**UA_BUILD_UNIT_TESTS**
   Compile unit tests with Check framework. The tests can be executed with ``make test``

**UA_BUILD_EXAMPLES_NODESET_COMPILER**
   Generate an OPC UA information model from a nodeset XML (experimental)

**UA_BUILD_SELFSIGNED_CERTIFICATE**
   Generate a self-signed certificate for the server (openSSL required)

UA_ENABLE_* group
^^^^^^^^^^^^^^^^^

This group contains build options related to the supported OPC UA features.

**UA_ENABLE_SUBSCRIPTIONS**
   Enable subscriptions
**UA_ENABLE_SUBSCRIPTIONS_EVENTS**
    Enable the use of events for subscriptions
**UA_ENABLE_METHODCALLS**
   Enable the Method service set
**UA_ENABLE_NODEMANAGEMENT**
   Enable dynamic addition and removal of nodes at runtime
**UA_ENABLE_AMALGAMATION**
   Compile a single-file release into the files :file:`open62541.c` and :file:`open62541.h`
**UA_ENABLE_MULTITHREADING**
   Enable multi-threading support
**UA_ENABLE_IMMUTABLE_NODES**
   Nodes in the information model are not edited but copied and replaced. The replacement is done with atomic operations so that the information model is always consistent and can be accessed from an interrupt or parallel thread (depends on the node storage plugin implementation). This feature is a prerequisite for ``UA_ENABLE_MULTITHREADING``.
**UA_ENABLE_COVERAGE**
   Measure the coverage of unit tests

Some options are marked as advanced. The advanced options need to be toggled to
be visible in the cmake GUIs.

**UA_ENABLE_TYPENAMES**
   Add the type and member names to the UA_DataType structure. Enabled by default.
**UA_ENABLE_STATUSCODE_DESCRIPTIONS**
   Compile the human-readable name of the StatusCodes into the binary. Enabled by default.
**UA_ENABLE_GENERATE_NAMESPACE0**
   Generate and load UA XML Namespace 0 definition
   ``UA_GENERATE_NAMESPACE0_FILE`` is used to specify the file for NS0 generation from namespace0 folder. Default value is ``Opc.Ua.NodeSet2.xml``
**UA_ENABLE_NONSTANDARD_UDP**
   Enable udp extension

UA_DEBUG_* group
^^^^^^^^^^^^^^^^

This group contains build options mainly useful for development of the library itself.

**UA_DEBUG**
   Enable assertions and additional definitions not intended for production builds

**UA_DEBUG_DUMP_PKGS**
   Dump every package received by the server as hexdump format

Building a shared library
^^^^^^^^^^^^^^^^^^^^^^^^^

open62541 is small enough that most users will want to statically link the
library into their programs. If a shared library (.dll, .so) is required, this
can be enabled in CMake with the ``BUILD_SHARED_LIBS`` option. Note that this
option modifies the :file:`ua_config.h` file that is also included in
:file:`open62541.h` for the single-file distribution.


Minimizing the binary size
^^^^^^^^^^^^^^^^^^^^^^^^^^

The size of the generated binary can be reduced considerably by adjusting the
build configuration. First, in CMake, the build type can be set to
``CMAKE_BUILD_TYPE=MinSizeRel``. This sets the compiler flags to minimize the
binary size. The build type also strips out debug information. Second, the
binary size can be reduced by removing features via the build-flags described
above.

Especially, logging takes up a lot of space in the binary and might not be
needed in embedded scenarios. Setting ``UA_LOGLEVEL`` to a value above 600
(=FATAL) disables all logging. In addition, the feature-flags
``UA_ENABLE_TYPENAMES`` and ``UA_ENABLE_STATUSCODE_DESCRIPTIONS`` add static
information to the binary that is only used for human-readable logging and
debugging.
