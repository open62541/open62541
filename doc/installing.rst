.. _installing:

Installing open62541
====================

Manual installation
-------------------

You can install open62541 using the well known `make install` command.
This allows you to use pre-built libraries and headers for your own project.

To override the default installation directory use ``cmake -DCMAKE_INSTALL_PREFIX=/some/path``.
Based on the SDK Features you selected, as described in :ref:`build_options`, these features will also
be included in the installation. Thus we recommend to enable as many non-experimental features as possible
for the installed binary.

The recommended cmake options for a default installation are:

.. code-block:: bash

   git submodule update --init --recursive
   mkdir build && cd build
   cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUA_NAMESPACE_ZERO=FULL ..
   make
   sudo make install

This will enable the following features in 0.4:

 * Discovery
 * FullNamespace
 * Methods
 * Subscriptions

The following features are not enabled and can be optionally enabled using the build options as described in :ref:`build_options`:

 * Amalgamation
 * DiscoveryMulticast
 * Encryption
 * Multithreading
 * Subscriptions

.. important::
   We strongly recommend to not use ``UA_ENABLE_AMALGAMATION=ON`` for your installation. This will only generate a single ``open62541.h`` header file instead of the single header files.
   We encourage our users to use the non-amalgamated version to reduce the header size and simplify dependency management.


In your own CMake project you can then include the open62541 library using:

.. code-block:: cmake

   # optionally you can also specify a specific version
   # e.g. find_package(open62541 0.4.0)
   find_package(open62541 REQUIRED COMPONENTS Events FullNamespace)
   add_executable(main main.cpp)
   target_link_libraries(main open62541::open62541)


A full list of enabled features during build time is stored in the CMake Variable ``open62541_COMPONENTS_ALL``


Prebuilt packages
-----------------


Prebuild binaries
^^^^^^^^^^^^^^^^^

You can always find prebuild binaries for every release on our `Github Release Page <https://github.com/open62541/open62541/releases>`_.


Nightly single file releases for Linux and Windows of the last 50 commits can be found here: https://open62541.org/releases/


OS Specific packages
^^^^^^^^^^^^^^^^^^^^

Debian packages can be found in our official PPA:

 * Daily Builds (based on master branch): https://launchpad.net/~open62541-team/+archive/ubuntu/daily
 * Release Builds (starting with Version 0.4): https://launchpad.net/~open62541-team/+archive/ubuntu/ppa
