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
   # e.g. find_package(open62541 1.0.0)
   find_package(open62541 REQUIRED COMPONENTS Events FullNamespace)
   add_executable(main main.cpp)
   target_link_libraries(main open62541::open62541)


A full list of enabled features during build time is stored in the CMake Variable ``open62541_COMPONENTS_ALL``


Prebuilt packages
-----------------

Pack branches
^^^^^^^^^^^^^

Github allows you to download a specific branch as .zip package. Just using this .zip package for open62541 will likely fail:

 * CMake uses ``git describe --tags`` to automatically detect the version string. The .zip package does not include any git information
 * Specific options during the build stack require additional git submodules which are not inlined in the .zip

Therefore we provide packaging branches. They have the prefix `pack/` and are automatically updated to match the referenced branch.

Here are some examples:

 * `pack/master.zip <https://github.com/open62541/open62541/archive/pack/master.zip>`_
 * `pack/1.0.zip <https://github.com/open62541/open62541/archive/pack/1.0.zip>`_

These pack branches have inlined submodules and the version string is hardcoded. If you need to build from source but do not want to use git,
use these specific pack versions.

Prebuilt binaries
^^^^^^^^^^^^^^^^^

You can always find prebuilt binaries for every release on our `Github Release Page <https://github.com/open62541/open62541/releases>`_.


Nightly single file releases for Linux and Windows of the last 50 commits can be found here: https://open62541.org/releases/


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
