.. _building:

Building open62541
==================

Building the Library
--------------------

open62541 uses CMake to build the library and binaries. CMake generates a
Makefile or a Visual Studio project. This is then used to perform the actual
build.

Building with CMake on Ubuntu or Debian
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   sudo apt-get install git build-essential gcc pkg-config cmake python3

   # enable additional features
   sudo apt-get install cmake-curses-gui     # for the ccmake graphical interface
   sudo apt-get install libmbedtls-dev       # for encryption support
   sudo apt-get install check libsubunit-dev # for unit tests
   sudo apt-get install python3-sphinx graphviz  # for documentation generation
   sudo apt-get install python3-sphinx-rtd-theme # documentation style
   sudo apt-get install libavahi-client-dev libavahi-common-dev # for LDS-ME (multicast discovery)

   git clone https://github.com/open62541/open62541.git
   cd open62541
   git submodule update --init --recursive
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

Note: parallel compilation can be enable by using 

.. code-block:: bash

    make -j$(nproc)

You can install open62541 using the well known `make install` command. This
allows you to use pre-built libraries and headers for your own project. In order
to use open62541 as a shared library (.dll or .so) make sure to activate the
``BUILD_SHARED_LIBS`` CMake option.

To override the default installation directory use ``cmake
-DCMAKE_INSTALL_PREFIX=/some/path``. Based on the SDK Features you selected, as
described in :ref:`build_options`, these features will also be included in the
installation. Thus we recommend to enable as many non-experimental features as
possible for the installed binary.

In your own CMake project you can then include the open62541 library. A simple
CMake project definition looks as follows:

.. code-block:: cmake

    cmake_minimum_required(VERSION 3.5)
    project("open62541SampleApplication")
    add_executable(main main.c)

    # Linux/Unix configuration using pkg-config
    find_package(PkgConfig)
    pkg_check_modules(open62541 REQUIRED open62541)
    target_link_libraries(main open62541)

    # Alternative CMake-based library definition.
    # This might not be included in some package distributions.
    #
    #   find_package(open62541 REQUIRED)
    #   target_link_libraries(main open62541::open62541)

Building with Visual Studio on Windows
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here we explain the build process for Visual Studio 2022 Community.

- Download and install
    - Python 3.x: https://python.org/downloads
    - Visual Studio 2022: https://visualstudio.microsoft.com
        - When installing Visual Studio select the workload "Desktop development with C++"

- Open Visual Studio and from the Get started window select "Clone a repository"
    - Repository location: https://github.com/open62541/open62541.git
    - Select a local path where to download the project and press Clone
- Switch to Folder View from the Solution Explorer
- Project / CMake Settings for open62541
        - Customize CMake variables as wanted and save
- Build / Build All
- Build / Install open62541

Note: the solution generated with cmake can also be compiled in parallel with the command

.. code-block:: bash

    msbuild open62541.sln /v:n -t:rebuild -m

Building with CMake/MinGW on Windows
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To build with MinGW, just replace the compiler selection in the call to CMake.

- Download and install
    - MinGW http://sourceforge.net/projects/mingw
    - Python 3.x: https://python.org/downloads
    - CMake: http://www.cmake.org/cmake/resources/software.html

- Download the open62541 sources (using git or as a zipfile from github)
- Open a command shell (cmd) and run

.. code-block:: bat

   cd <path-to>\open62541
   mkdir build
   cd build
   <path-to>\cmake.exe .. -G "MinGW Makefiles"
   :: You can use use cmake-gui for a graphical user-interface to select features
   make

Building on OS X
^^^^^^^^^^^^^^^^

- Download and install

  - Xcode: https://itunes.apple.com/us/app/xcode/id497799835?ls=1&mt=12
  - Homebrew: http://brew.sh/
  - Pip (a package manager for Python, may be preinstalled): ``sudo easy_install pip``

- Run the following in a shell

.. code-block:: bash

   brew install cmake
   pip install sphinx # for documentation generation
   pip install sphinx_rtd_theme # documentation style
   brew install graphviz # for graphics in the documentation
   brew install check # for unit tests

Follow Ubuntu instructions without the ``apt-get`` commands as these are taken care of by the above packages.

Building on OpenBSD
^^^^^^^^^^^^^^^^^^^

The procedure below works on OpenBSD 5.8 with gcc version 4.8.4, cmake version
3.2.3 and Python version 2.7.10.

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

Building Debian Packages inside Docker Container with CMake on Ubuntu or Debian
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This is how to build the Debian packages.

.. code-block:: bash

   # Assume a fresh checkout of open62541 in the ~/open62541 directory
   cd ~/open62541

   # Create the debian packaging definitions
   python3 ./tools/prepare_packaging.py

   # Build the package (generated packages and source will be in the ~ folder)
   debuild

Build Options
-------------

The open62541 project uses CMake to manage the build options, for code
generation and to generate build projects for the different systems and IDEs.
The tools *ccmake* or *cmake-gui* can be used to graphically set the build
options.

Most options can be changed manually in :file:`ua_config.h` (:file:`open62541.h`
for the single-file release) after the code generation. But usually there is no
need to adjust them.

Main Build Options
^^^^^^^^^^^^^^^^^^

**CMAKE_BUILD_TYPE**
  - ``RelWithDebInfo`` -O2 optimization with debug symbols
  - ``Release`` -O2 optimization without debug symbols
  - ``Debug`` -O0 optimization with debug symbols
  - ``MinSizeRel`` -Os optimization without debug symbols

**BUILD_SHARED_LIBS**
   Build a shared library (.dll/.so) or (an archive of) object files for linking
   into a static binary. Shared libraries are recommended for a system-wide
   install. Note that this option modifies the :file:`ua_config.h` file that is
   also included in :file:`open62541.h` for the single-file distribution.

**UA_LOGLEVEL**
   The SDK logs events of the level defined in ``UA_LOGLEVEL`` and above only.
   The logging event levels are as follows:

   - 600: Fatal
   - 500: Error
   - 400: Warning
   - 300: Info
   - 200: Debug
   - 100: Trace

   This compilation flag defines which log levels get compiled into the code. In
   addition, the implementations of :ref:`logging` allow to set a filter for the
   logging level at runtime. So the logging level can be changed in the
   configuration without recompiling.

**UA_MULTITHREADING**
   Level of multi-threading support. The supported levels are currently as follows:

  - 0-99: Multithreading support disabled.
  - >=100: API functions marked with the UA_THREADSAFE-macro are protected internally with mutexes.
    Multiple threads are allowed to call these functions of the SDK at the same time without causing race conditions.
    Furthermore, this level support the handling of asynchronous method calls from external worker threads.

Select build artefacts
^^^^^^^^^^^^^^^^^^^^^^

By default only the main library shared object libopen62541.so (open62541.dll)
or static linking archive open62541.a (open62541.lib) is built. Additional
artifacts can be specified by the following options:

**UA_BUILD_EXAMPLES**
   Compile example servers and clients from :file:`examples/*.c`.

**UA_BUILD_UNIT_TESTS**
   Compile unit tests. The tests can be executed with ``make test``.
   An individual test can be executed with ``make test ARGS="-R <test_name> -V"``.
   The list of available tests can be displayed with ``make test ARGS="-N"``.

Detailed SDK Features
^^^^^^^^^^^^^^^^^^^^^

**UA_ENABLE_SUBSCRIPTIONS**
   Enable subscriptions

**UA_ENABLE_SUBSCRIPTIONS_EVENTS**
    Enable the use of events for subscriptions. This is a new feature and currently marked as EXPERIMENTAL.

**UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS (EXPERIMENTAL)**
    Enable the use of A&C for subscriptions. This is a new feature build upon events and currently marked as EXPERIMENTAL.

**UA_ENABLE_METHODCALLS**
   Enable the Method service set

**UA_ENABLE_PARSING**
   Enable parsing human readable formats of builtin data types (Guid, NodeId, etc.).
   Utility functions that are not essential to the SDK.

**UA_ENABLE_NODEMANAGEMENT**
   Enable dynamic addition and removal of nodes at runtime

**UA_ENABLE_AMALGAMATION**
   Compile a single-file release into the files :file:`open62541.c` and :file:`open62541.h`.
   Invoke the CMake target to generate the amalgamation as ``make open62541-amalgamation``.

**UA_ENABLE_IMMUTABLE_NODES**
   Nodes in the information model are not edited but copied and replaced. The
   replacement is done with atomic operations so that the information model is
   always consistent and can be accessed from an interrupt or parallel thread
   (depends on the node storage plugin implementation).

**UA_ENABLE_COVERAGE**
   Measure the coverage of unit tests

**UA_ENABLE_DISCOVERY**
   Enable Discovery Service (LDS)

**UA_ENABLE_DISCOVERY_MULTICAST**
   Enable Discovery Service with multicast support (LDS-ME) and specify the
   multicast backend. The possible options are:

   - ``OFF`` No multicast support. (default)
   - ``MDNSD`` Multicast support using libmdnsd
   - ``AVAHI`` Multicast support using Avahi

**UA_ENABLE_DISCOVERY_SEMAPHORE**
   Enable Discovery Semaphore support

**UA_ENABLE_ENCRYPTION**
   Enable encryption support and specify the used encryption backend. The possible
   options are:

   - ``OFF`` No encryption support. (default)
   - ``MBEDTLS`` Encryption support using mbed TLS
   - ``OPENSSL`` Encryption support using OpenSSL
   - ``LIBRESSL`` EXPERIMENTAL: Encryption support using LibreSSL

**UA_ENABLE_ENCRYPTION_TPM2**
   Enable TPM hardware for encryption. The possible options are:
      - ``OFF`` No TPM encryption support. (default)
      - ``ON`` TPM encryption support

**UA_NAMESPACE_ZERO**
   Namespace zero contains the standard-defined nodes. The full namespace zero
   may not be required for all applications. The selectable options are as follows:

   - ``MINIMAL``: A barebones namespace zero that is compatible with most
     clients. But this namespace 0 is so small that it does not pass the CTT
     (Conformance Testing Tools of the OPC Foundation).
   - ``REDUCED``: Small namespace zero that passes the CTT.
   - ``FULL``: Full namespace zero generated from the official XML definitions.

   The advanced build option ``UA_FILE_NS0`` can be used to override the XML
   file used for namespace zero generation.

**UA_ENABLE_DIAGNOSTICS**
   Enable diagnostics information exposed by the server. Enabled by default.

**UA_ENABLE_JSON_ENCODING**
   Enable JSON encoding. Enabled by default. The JSON encoding changed with the
   1.05 version of the OPC UA specification. The legacy encoding can be enabled
   via the ``UA_ENABLE_JSON_ENCODING_LEGACY`` option. Note that this legacy
   feature wil get removed at some point in the future.

Some options are marked as advanced. The advanced options need to be toggled to
be visible in the cmake GUIs.

**UA_ENABLE_TYPEDESCRIPTION**
   Add the type and member names to the UA_DataType structure. Enabled by default.

**UA_ENABLE_STATUSCODE_DESCRIPTIONS**
   Compile the human-readable name of the StatusCodes into the binary. Enabled by default.

**UA_ENABLE_FULL_NS0**
   Use the full NS0 instead of a minimal Namespace 0 nodeset
   ``UA_FILE_NS0`` is used to specify the file for NS0 generation from namespace0 folder. Default value is ``Opc.Ua.NodeSet2.xml``

PubSub Build Options
^^^^^^^^^^^^^^^^^^^^

**UA_ENABLE_PUBSUB**
   Enable the experimental OPC UA PubSub support. The option will include the
   PubSub UDP multicast plugin. Enabled by default.

**UA_ENABLE_PUBSUB_FILE_CONFIG**
   Enable loading OPC UA PubSub configuration from File/ByteString. Enabling
   PubSub informationmodel methods also will add a method to the
   Publish/Subscribe object which allows configuring PubSub at runtime. Disabled by default.

**UA_ENABLE_PUBSUB_INFORMATIONMODEL**
   Enable the information model representation of the PubSub configuration. For
   more details take a look at the following section `PubSub Information Model
   Representation`. Enabled by default.

Debug Build Options
^^^^^^^^^^^^^^^^^^^

This group contains build options mainly useful for development of the library itself.

**UA_DEBUG**
   Enable assertions and additional definitions not intended for production builds

**UA_DEBUG_DUMP_PKGS**
   Dump every package received by the server as hexdump format

Minimizing the binary size
^^^^^^^^^^^^^^^^^^^^^^^^^^

The size of the generated binary can be reduced considerably by adjusting the
build configuration. With open62541, it is possible to configure minimal servers
that require less than 100kB of RAM and ROM.

The following options influence the ROM requirements:

First, in CMake, the build type can be set to ``CMAKE_BUILD_TYPE=MinSizeRel``.
This sets the compiler flags to minimize the binary size. The build type also
strips out debug information. Second, the binary size can be reduced by removing
features via the build-flags described above.

Second, setting ``UA_NAMESPACE_ZERO`` to ``MINIMAL`` reduces the size of the
builtin information model. Setting this option can reduce the binary size by
half in some cases.

Third, some features might not be needed and can be disabled to reduce the
binary footprint. Examples for this are Subscriptions or encrypted
communication.

Last, logging messages take up a lot of space in the binary and might not be
needed in embedded scenarios. Setting ``UA_LOGLEVEL`` to a value above 600
(``FATAL``) disables all logging. In addition, the feature-flags
``UA_ENABLE_TYPEDESCRIPTION`` and ``UA_ENABLE_STATUSCODE_DESCRIPTIONS`` add static
information to the binary that is only used for human-readable logging and
debugging.

The RAM requirements of a server are mostly due to the following settings:

- The size of the information model
- The number of connected clients
- The configured maximum message size that is preallocated

Prebuilt packages
-----------------

Debian
^^^^^^
Debian packages can be found in our official PPA:

 * Daily Builds (based on master branch): https://launchpad.net/~open62541-team/+archive/ubuntu/daily
 * Release Builds (starting with Version 0.4): https://launchpad.net/~open62541-team/+archive/ubuntu/ppa

Install them with:

.. code-block:: bash

    sudo add-apt-repository ppa:open62541-team/ppa
    sudo apt-get update
    sudo apt-get install libopen62541-1-dev

Arch
^^^^
Arch packages are available in the AUR:

 * Stable Builds: https://aur.archlinux.org/packages/open62541/
 * Unstable Builds (current master): https://aur.archlinux.org/packages/open62541-git/
 * In order to add custom build options (:ref:`build_options`), you can set the environment variable ``OPEN62541_CMAKE_FLAGS``

OpenBSD
^^^^^^^
Starting with OpenBSD 6.7 the ports directory misc/open62541 can
build the released version of open62541.
Install the binary package from the OpenBSD mirrors:

.. code-block:: bash
   
   pkg_add open62541

Building the Examples
---------------------

Make sure that you have installed the shared library as explained in the
previous steps. Then the build system should automatically find the includes and
the shared library.

.. code-block:: bash

   cp /path-to/examples/tutorial_server_firststeps.c . # copy the example server
   gcc -std=c99 -o server tutorial_server_firststeps.c -lopen62541

