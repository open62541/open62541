.. _building:

Building open62541
==================

open62541 uses CMake to build the library and binaries. The library version is automatically
detected using ``git describe``. This command returns a valid version string based on the current tag.
If you did not directly clone the sources, but use the tar or zip package from a release, you need
to manually specify the version. In that case use e.g. ``cmake -DOPEN62541_VERSION=v1.0.3``.

Building the Library
--------------------

Building with CMake on Ubuntu or Debian
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   sudo apt-get install git build-essential gcc pkg-config cmake python

   # enable additional features
   sudo apt-get install cmake-curses-gui # for the ccmake graphical interface
   sudo apt-get install libmbedtls-dev # for encryption support
   sudo apt-get install check libsubunit-dev # for unit tests
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

Building Debian Packages inside Docker Container with CMake on Ubuntu or Debian
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here is an example howto build the library as Debian package inside a Docker container

- Download and install

  - Docker Engine: https://docs.docker.com/install/linux/docker-ce/debian/
  - docker-deb-builder: https://github.com/tsaarni/docker-deb-builder.git
  - open62541: https://github.com/open62541/open62541.git

Install Docker as described at https://docs.docker.com/install/linux/docker-ce/debian/ .

Get the docker-deb-builder utility from github and make Docker images for the needed
Debian and/or Ubuntu relases

.. code-block:: bash

   # make and goto local development path (e.g. ~/development)
   mkdir ~/development
   cd ~/development

   # clone docker-deb-builder utility from github and change into builder directory
   git clone https://github.com/tsaarni/docker-deb-builder.git
   cd docker-deb-builder

   # make Docker builder images (e.g. Ubuntu 18.04 and 17.04)
   docker build -t docker-deb-builder:18.04 -f Dockerfile-ubuntu-18.04 .
   docker build -t docker-deb-builder:17.04 -f Dockerfile-ubuntu-17.04 .

Make a local copy of the open62541 git repo and checkout a pack branch

.. code-block:: bash

   # make a local copy of the open62541 git repo (e.g. in the home directory)
   # and checkout a pack branch (e.g. pack/1.0)
   cd ~
   git clone https://github.com/open62541/open62541.git
   cd ~/open62541
   git checkout pack/1.0

Now it's all set to build Debian/Ubuntu open62541 packages

.. code-block:: bash

   # goto local developmet path
   cd ~/development

   # make a local output directory for the builder where the packages can be placed after build
   mkdir output

   # build Debian/Ubuntu packages inside Docker container (e.g. Ubuntu-18.04)
   ./build -i docker-deb-builder:18.04 -o output ~/open62541

After a successfull build the Debian/Ubuntu packages can be found at :file:`~/development/docker-deb-builder/output`

CMake Build Options and Debian Packaging
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If the open62541 library will be build as a Debian package using a pack branch (e.g. pack/master or pack/1.0)
then altering or adding CMake build options should be done inside the :file:`debian/rules` file respectively in
the :file:`debian/rules-template` file if working with a development branch (e.g. master or 1.0).

The section in :file:`debian/rules` where the CMake build options are defined is

.. code-block:: bash

   ...
   override_dh_auto_configure:
       dh_auto_configure -- -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUA_NAMESPACE_ZERO=FULL -DUA_ENABLE_AMALGAMATION=OFF -DUA_PACK_DEBIAN=ON
   ...

This CMake build options will be passed as command line variables to CMake during Debian packaging.

.. _build_options:

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

**UA_LOGLEVEL**
   The SDK logs events of the level defined in ``UA_LOGLEVEL`` and above only.
   The logging event levels are as follows:

   - 600: Fatal
   - 500: Error
   - 400: Warning
   - 300: Info
   - 200: Debug
   - 100: Trace

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
   Compile unit tests. The tests can be executed with ``make test``

**UA_BUILD_SELFSIGNED_CERTIFICATE**
   Generate a self-signed certificate for the server (openSSL required)

Detailed SDK Features
^^^^^^^^^^^^^^^^^^^^^

**UA_ENABLE_SUBSCRIPTIONS**
   Enable subscriptions

**UA_ENABLE_SUBSCRIPTIONS_EVENTS (EXPERIMENTAL)**
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
   Compile a single-file release into the files :file:`open62541.c` and :file:`open62541.h`. Not recommended for installation.

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
   Enable Discovery Service with multicast support (LDS-ME)
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
   PubSub UDP multicast plugin. Disabled by default.

**UA_ENABLE_PUBSUB_DELTAFRAMES**
   The PubSub messages differentiate between keyframe (all published values
   contained) and deltaframe (only changed values contained) messages.
   Deltaframe messages creation consumes some additional resources and can be
   disabled with this flag. Disabled by default.

**UA_ENABLE_PUBSUB_FILE_CONFIG**
   Enable loading OPC UA PubSub configuration from File/ByteString. Enabling
   PubSub informationmodel methods also will add a method to the
   Publish/Subscribe object which allows configuring PubSub at runtime.

**UA_ENABLE_PUBSUB_INFORMATIONMODEL**
   Enable the information model representation of the PubSub configuration. For
   more details take a look at the following section `PubSub Information Model
   Representation`. Disabled by default.

**UA_ENABLE_PUBSUB_MONITORING**
   Enable the experimental PubSub monitoring. This feature provides a basic
   framework to implement monitoring/timeout checks for PubSub components.
   Initially the MessageReceiveTimeout check of a DataSetReader is provided. It
   uses the internal server callback implementation. The monitoring backend can
   be changed by the application to satisfy realtime requirements. Disabled by
   default.

**UA_ENABLE_PUBSUB_ETH_UADP**
   Enable the OPC UA Ethernet PubSub support to transport UADP NetworkMessages
   as payload of Ethernet II frame without IP or UDP headers. This option will
   include Publish and Subscribe based on EtherType B62C. Disabled by default.

Debug Build Options
^^^^^^^^^^^^^^^^^^^

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


Building the Examples
---------------------

Make sure that you can build the shared library as explained in the previous steps. Even easier way
to build the examples is to install open62541 in your operating system (see :ref:`installing`).

Then the compiler should automatically find the includes and the shared library.

.. code-block:: bash

   cp /path-to/examples/tutorial_server_firststeps.c . # copy the example server
   gcc -std=c99 -o server tutorial_server_firststeps.c -lopen62541

.. include:: building_arch.rst
