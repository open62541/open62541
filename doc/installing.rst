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

Conan package
^^^^^^^^^^^^^^

Conan is a package management utility based on Python, which fetches a pre-pacakaged library from a remote repository. If you are new to conan you can read 
up on it at `getting started <https://docs.conan.io/en/latest/getting_started.html>`_. For a fresh instalation of conana package manager, please follow 
the `instalation guide <https://docs.conan.io/en/latest/installation.html>`_.

To add open62541 as a dependency to your projecet, add the following lines to your ``conanfile.txt`` file:

.. code-block:: bash

   [requires]
   open62541/1.0.0

This package provides the following options as described in :ref:`build_options` for you to configure:

+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| Cmake Option name                       | Conan option name              | Posible Values                             | Default Value |
+=========================================+================================+============================================+===============+
| POSITION_INDEPENDENT_CODE               | fPIC                           | True/False                                 | True          |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| BUILD_SHARED_LIBS                       | shared                         | True/False                                 | True          |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_LOGLEVEL                             | logging_level                  | Fatal, Error, Warrning, Info, Debug, Trace | Info          |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_DA                            | data_access                    | True/False                                 | True          |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_SUBSCRIPTIONS                 | subscription                   | True/False                                 | True          |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_SUBSCRIPTIONS_EVENTS          | subscription_events            | True/False                                 | False         |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_METHODCALLS                   | methods                        | True/False                                 | True          |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_NODEMANAGEMENT                | dynamic_nodes                  | True/False                                 | True          |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_AMALGAMATION                  | single_header                  | True/False                                 | False         |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_MULTITHREADING                | multithreading                 | True/False                                 | False         |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_IMMUTABLE_NODES               | imutable_nodes                 | True/False                                 | False         |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_WEBSOCKET_SERVER              | web_socket                     | True/False                                 | False         |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_HISTORIZING                   | historize                      | True/False                                 | False         |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_DISCOVERY                     | discovery                      | True/False                                 | True          |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_DISCOVERY_MULTICAST           | discovery_multicast            | True/False                                 | False         |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_DISCOVERY_SEMAPHORE           | discovery_semaphore            | True/False                                 | True          |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_QUERY                         | query                          | True/False                                 | False         |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_ENCRYPTION                    | encription                     | True/False                                 | False         |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_JSON_ENCODING                 | json_support                   | True/False                                 | False         |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_PUBSUB                        | *pub_sub                       | None, Simple, Ethernet, Ethernet_XDP       | None          |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS | compiled_nodeset_descriptions  | True/False                                 | True          |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_NAMESPACE_ZERO                       | namescpae_zero                 | MINIMAL, REDUCED, FULL                     | FULL          |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_MICRO_EMB_DEV_PROFILE         | embedded_profile               | True/False                                 | False         |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_TYPENAMES                     | typenames                      | True/False                                 | True          |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_STATUSCODE_DESCRIPTIONS       | readable_statuscodes           | True/False                                 | True          |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_ENABLE_HARDENING                     | hardening                      | True/False                                 | False         |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+
| UA_COMPILE_AS_CXX                       | cpp_compatible                 | True/False                                 | False         |
+-----------------------------------------+--------------------------------+--------------------------------------------+---------------+

.. note::

   *pub_sub - this option controls ``UA_ENABLE_PUBSUB``, ``UA_ENABLE_PUBSUB_ETH_UADP`` and ``UA_ENABLE_PUBSUB_ETH_UADP_XDP`` CMake Options. 
    * To enable the default Pub/Sub use Simple as a value, this will activate ``UA_ENABLE_PUBSUB``
    * To enable Pub/Sub over UADP Ethernet``use Ethernet as a value, this will activate ``UA_ENABLE_PUBSUB`` and ``UA_ENABLE_PUBSUB_ETH_UADP``
    * To enable XDP over UADP Ethernet use Ethernet_XDP as a value, this will activate ``UA_ENABLE_PUBSUB``,  ``UA_ENABLE_PUBSUB_ETH_UADP`` 
    and ``UA_ENABLE_PUBSUB_ETH_UADP_XDP``

To change these options you need to add the following in your ``conanfile.txt`` file:

.. code-block:: bash

   [options]
   open62541:logging_level=Fatal # PACKAGE:OPTION=VALUE

``CMAKE_BUILD_TYPE`` is set via settings instead of options. It can be done set in your ``conanfile.txt`` so: 

.. code-block:: bash

   [settings]
   open62541:build_type=Debug

This setting supprots the following options:

 * RelWithDebInfo
 * Release
 * Debug
 * MinSizeRel

This package does not contain any utilities or exampels for you to use since these can not be imported from the package into the user project space, 
where they would be useful.

This pacakge does not build Test Cases to save package space.

If you experience a bug or a problem with this package please open as issue at `conan central index <https://github.com/conan-io/conan-center-index/issues>`_.

Prebuild binaries
^^^^^^^^^^^^^^^^^

You can always find prebuild binaries for every release on our `Github Release Page <https://github.com/open62541/open62541/releases>`_.


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

Arch packages are available in the AUR

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
