First steps with open62541-server
=================================

This tutorial will attempt to guide you through your first steps with open62541. It offers a bit of a more "hands on" approach to learning how to use this stack by talking you through building several small example OPC UA server/client applications.

Before we start: a word of warning; open62541 is under active development. New functionality is added and stale bits overhauled all the time in order to respond to our communities feedback. Please understand that if you come back here next week, some things might have changed... eeem... improved.

Prerequisites
-------------

This series of tutorials assumes that you are familiar both with coding in C, the OPC Unified Architecture namespace concepts, its datatypes and services. This tutorial will not teach you OPC UA.

For running these tutorials, you will require cmake, python (<= 2.7) and a compiler (MS Visual Studio 2015, gcc, clang and mingw32 are known to be working). Note that if you are using MSVS, the 2015 version is manditory. open62541 makes extensive use of C99, which is not supported by earlier Versions of MSVS. It will also be very helpfull to install a OPC UA Client with a graphical frontend, such as UAExpert by Unified Automation, that will enable you to examine the namespace of your server.

For now, this tutorial will assume that you are using an up-to-date Linux or BSD distribution to run these examples.

Before we can get started you will require the stack. You may either clone the current master or download a ZIP/TGZ archive from github. Let's assume that you want to clone the github repository. Open a shell, navigate to a folder of your choice and clone the repository::

   :> git clone https://github.com/acplt/open62541
   Cloning into './open62541'...
   remote: Counting objects: 14443, done.
   remote: Compressing objects: 100% (148/148), done.
   remote: Total 14443 (delta 106), reused 0 (delta 0), pack-reused 14293
   Receiving objects: 100% (14443/14443), 7.18 MiB | 654.00 KiB/s, done.
   Resolving deltas: 100% (10894/10894), done.
   Checking connectivity... done.
   :>

Then create a build directory and read the next section.::

   :> cd open62541
   :open62541> mkdir build
   :open62541/build> cd build
  
Note that the shell used here was BASH. You might have to adapt some of the following examples for your shell if you prefer tcsh or csh.

Verifying your build environment
--------------------------------

Let's proceed with the default stack build, which will give you an impression of what you should see if your building process is successfull::

   :open62541/build> cmake ../
   -- The C compiler identification is GNU 4.8.3
   -- Check for working C compiler: /usr/bin/cc
   -- Check for working C compiler: /usr/bin/cc -- works
   -- Detecting C compiler ABI info
   -- Detecting C compiler ABI info - done
   -- Found PythonInterp: /usr/bin/python (found version "2.7.8") 
   -- Found Git: /usr/bin/git (found version "2.1.4") 
   -- Git version: v0.1.0-RC4-365-g35331dc
   -- CMAKE_BUILD_TYPE not given; setting to 'RelWithDebInfo'.
   -- Configuring done
   -- Generating done
   -- Build files have been written to: /home/ichrispa/work/svn/working_copies/tmpopen/build

   :open62541/build> make
   [  3%] Generating src_generated/ua_nodeids.h
   [  7%] Generating src_generated/ua_types_generated.c, src_generated/ua_types_generated.h
   [ 11%] Generating src_generated/ua_transport_generated.c, src_generated/ua_transport_generated.h
   Scanning dependencies of target open62541-object
   [ 14%] Building C object CMakeFiles/open62541-object.dir/src/ua_types.c.o
   [ 18%] Building C object CMakeFiles/open62541-object.dir/src/ua_types_encoding_binary.c.o
   [ 22%] Building C object CMakeFiles/open62541-object.dir/src_generated/ua_types_generated.c.o
   [ 25%] Building C object CMakeFiles/open62541-object.dir/src_generated/ua_transport_generated.c.o
   [ 29%] Building C object CMakeFiles/open62541-object.dir/src/ua_connection.c.o
   [ 33%] Building C object CMakeFiles/open62541-object.dir/src/ua_securechannel.c.o
   [ 37%] Building C object CMakeFiles/open62541-object.dir/src/ua_session.c.o
   [ 40%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_server.c.o
   [ 44%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_server_addressspace.c.o
   [ 48%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_server_binary.c.o
   [ 51%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_nodes.c.o
   [ 55%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_server_worker.c.o
   [ 59%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_securechannel_manager.c.o
   [ 62%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_session_manager.c.o
   [ 66%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_services_discovery.c.o
   [ 70%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_services_securechannel.c.o
   [ 74%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_services_session.c.o
   [ 77%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_services_attribute.c.o
   [ 81%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_services_nodemanagement.c.o
   [ 85%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_services_view.c.o
   [ 88%] Building C object CMakeFiles/open62541-object.dir/src/client/ua_client.c.o
   [ 92%] Building C object CMakeFiles/open62541-object.dir/examples/networklayer_tcp.c.o
   [ 96%] Building C object CMakeFiles/open62541-object.dir/examples/logger_stdout.c.o
   [100%] Building C object CMakeFiles/open62541-object.dir/src/server/ua_nodestore.c.o
   [100%] Built target open62541-object
   Scanning dependencies of target open62541
   Linking C shared library libopen62541.so
   [100%] Built target open62541

   :open62541/build>
   
The line where ``cmake ../`` is executed tells cmake to prepare the build process in the current subdirectory. ``make`` then executes the generated Makefiles which build the stack. At this point, a shared library named *libopen62541.so* should have been generated in the build folder. By using this library and the header files contained in the ``open62541/include`` folder you can enable your applications to use the open62541 OPC UA server and client stack.

Creating your first server
--------------------------

Let's build a very rudimentary server. Create a separate folder for your application and copy the necessary source files into an a subfolder named ``include``. Don't forget to also copy the shared library. Then create a new C sourcefile called ``myServer.c``. If you choose to use a shell, the whole process should look like this::

   :open62541/build> cd ../../
   :> mkdir myApp
   :> cd myApp
   :myApp> mkdir include
   :myApp> cp ../open62541/include/* ./include
   :myApp> cp ../open62541/plugins/*.h ./include
   :myApp> cp ../open62541/build/src_generated/*.h ./include
   :myApp> cp ../open62541/build/*.so .
   :myApp> tree
   .
   ├── include
   │   ├── logger_stdout.h
   │   ├── networklayer_tcp.h
   │   ├── networklayer_udp.h
   │   ├── ua_client.h
   │   ├── ua_client_highlevel.h
   │   ├── ua_config.h
   │   ├── ua_config.h.in
   │   ├── ua_config_standard.h
   │   ├── ua_connection.h
   │   ├── ua_constants.h
   │   ├── ua_job.h
   │   ├── ua_log.h
   │   ├── ua_nodeids.h
   │   ├── ua_server_external_ns.h
   │   ├── ua_server.h
   │   ├── ua_transport_generated_encoding_binary.h
   │   ├── ua_transport_generated.h
   │   ├── ua_types_generated_encoding_binary.h
   │   ├── ua_types_generated.h
   │   └── ua_types.h
   ├── libopen62541.so
   └── myServer.c
   :myApp> touch myServer.c

Open myServer.c and write/paste your minimal server application:

.. code-block:: c

   #include <stdio.h>

   # include "ua_types.h"
   # include "ua_server.h"
   # include "ua_config_standard.h"
   # include "logger_stdout.h"
   # include "networklayer_tcp.h"

   UA_Boolean running;
   UA_Logger logger = Logger_Stdout;

   int main(void) {
     UA_ServerConfig config = UA_ServerConfig_standard;
     UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
     config.logger = Logger_Stdout;
     config.networkLayers = &nl;
     config.networkLayersSize = 1;
     UA_Server *server = UA_Server_new(config);
     running = true;
     UA_Server_run(server, &running);
     UA_Server_delete(server);

     return 0;
   }

This is all that is needed to start your OPC UA Server. Compile the the server with GCC using the following command::

   :myApp> gcc -Wl,-rpath,`pwd` -I ./include -L ./ ./myServer.c -o myServer  -lopen62541

Some notes: You are using a dynamically linked library (libopen62541.so), which needs to be located in your dynamic linkers search path. Unless you copy libopen62541.so into a common folder like /lib or /usr/lib, the linker will fail to find the library and complain (i.e. not run the application). ``-Wl,-rpath,`pwd``` adds your present working directory to the relative searchpaths of the linker when executing the binary (you can also use ``-Wl,-rpath,.`` if the binary and the library are always in the same directory).

Now execute the server::

   :myApp> ./myServer

You have now compiled and started your first OPC UA Server. Though quite unspectacular and only terminatable with ``CTRL+C`` (SIGTERM) at the moment, you can already launch it and browse around with UA Expert. The Server will be listening on localhost:16664 - go ahead and give it a try.

We will also make a slight change to our server: We want it to exit cleanly when pressing ``CTRL+C``. We will add signal handler for SIGINT and SIGTERM to accomplish that to the server:

.. code-block:: c

    #include <stdio.h>
    #include <signal.h>

    #include "ua_types.h"
    #include "ua_server.h"
    #include "logger_stdout.h"
    #include "networklayer_tcp.h"

    UA_Boolean running = true;
    static void stopHandler(int signal) {
        running = false;
    }
    
    int main(void) {
        signal(SIGINT,  stopHandler);
        signal(SIGTERM, stopHandler);
    
        UA_ServerConfig config = UA_ServerConfig_standard;
        UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664, Logger_Stdout);
        config.logger = Logger_Stdout;
        config.networkLayers = &nl;
        config.networkLayersSize = 1;
        UA_Server *server = UA_Server_new(config);
    
        UA_StatusCode retval = UA_Server_run(server, &running);
        UA_Server_delete(server);
        nl.deleteMembers(&nl);
        return retval;
    }

Note that this file can be found as "examples/server_firstSteps.c" in the repository.
    
And then of course, recompile it::

    :myApp> gcc -Wl,-rpath=./ -L./ -I ./include -o myServer myServer.c  -lopen62541

You can now start and background the server, run the client, and then terminate the server like so::

    :myApp> ./myServer &
    [xx/yy/zz aa:bb:cc.dd.ee] info/communication	Listening on opc.tcp://localhost:16664
    [1] 2114
    :myApp> ./myClient && killall myServer
    Terminated
    [1]+  Done                    ./myServer
    :myApp> 

Notice how the server received the SIGTERM signal from kill and exited cleany? We also used the return value of our client by inserting the ``&&``, so kill is only called after a clean client exit (``return 0``).

Introduction to Configuration options (Amalgamation)
----------------------------------------------------

If you browsed through your new servers namespace with UAExpert or some other client, you might have noticed that the server can't do a lot. Indeed, even Namespace 0 appears to be mostly missing.

open62541 is a highly configurable stack that lets you turn several features on or off depending on your needs. This allows you to create anything from a very minimal and ressource saving OPC UA client to a full-fledged server. Picking which features you want is part of the cmake building process. CMake will handle the configuration of Makefiles, definition of precompiler variables and calling of auxilary scripts for you.If the building process above has failed on your system, please make sure that you have all the prerequisites installed and configured properly.

A detailed list of all configuration options is given in the documentation of open62541. This tutorial will introduce you to some of these options one by one in due course, but I will mention a couple of non-feature related options at this point to give readers a heads-up on the advantages and consequences of using them.

**Warning:** If you change cmake options, always make sure that you have a clean build directory first (unless you know what you are doing). CMake will *not* reliably detect changes to non-source files, such as source files for scripts and generators. Always run ``make clean`` between builds, and remove the ``CMakeCache.txt`` file from your build directory to make super-double-extra-sure that your build is clean before executing cmake.

**UA_ENABLE_AMALGAMATION**

This one might appear quite mysterious at first... this option will enable a python script (tools/amalgate.py) that will merge all headers of open62541 into a single header and c files into a single c file. Why? The most obvious answer is that you might not want to use a shared library in your project, but compile everything into your own binary. Let's give that a try... get back into the build folder ``make clean`` and then try this::

   :open62541/build> make clean
   :open62541/build> cmake -DUA_ENABLE_AMALGAMATION=On ../
   :open62541/build> make
   [  5%] Generating open62541.h
   Starting amalgamating file /open62541/build/open62541.h
   Integrating file '/open62541/build/src_generated/ua_config.h'...done.
   (...)
   The size of /open62541/build/open62541.h is 243350 Bytes.
   [ 11%] Generating open62541.c
   Starting amalgamating file /open62541/build/open62541.c
   Integrating file '/open62541/src/ua_util.h'...done.
   (...)
   Integrating file '/open62541/src/server/ua_nodestore.c'...done.
   The size of /open62541/build/open62541.c is 694855 Bytes.
   [ 27%] Built target amalgamation
   Scanning dependencies of target open62541-object
   [ 33%] Building C object CMakeFiles/open62541-object.dir/open62541.c.o
   [ 61%] Built target open62541-object
   Scanning dependencies of target open62541
   Linking C shared library libopen62541.so
   :open62541/build> 

Switch back to your myApp directory and recompile your binary, this time embedding all open62541 functionality in one executable::

   :open62541/build> cd ../../myApp
   :open62541/build> cp ../open62541/build/open62541.* .
   :myApp> gcc -std=c99 -I ./ -c ./open62541.c
   :myApp> gcc -std=c99 -I ./include -o myServer myServer.c open62541.o
   :myApp> ./myServer
   
You can now start the server and browse around as before. As you might have noticed, no shared library is required anymore. That makes the application more portable or runnable on systems without dynamic linking support and allows you to use functions that are not exported by the library (which propably means we haven't documented them as thouroughly...); on the other hand the application is also much bigger, so if you intend to also use a client with open62541, you might be inclined to overthink amalgamation.

The next step is to simplify the header dependencies. Instead of picking header files one-by-one, we can use the copied amalgamated header including all the public headers dependencies.

Open myServer.c and simplify it to:

.. code-block:: c

   #include <stdio.h>
   #include "open62541.h"

   UA_Boolean running;
   UA_Logger logger = Logger_Stdout;
   int main(void) {
     UA_ServerConfig config = UA_ServerConfig_standard;
     UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664, logger);
     config.logger = Logger_Stdout;
     config.networkLayers = &nl;
     config.networkLayersSize = 1;
     UA_Server *server = UA_Server_new(config);
     running = true;
     UA_Server_run(server, &running);
     UA_Server_delete(server);

     return 0;
   }
   
It can now also be compiled without the include directory, i.e., ::

   :myApp> gcc -std=c99 myServer.c open62541.c -o myServer
   :myApp> ./myServer

Please note that at times the amalgamation script has... well, bugs. It might include files in the wrong order or include features even though the feature is turned off. Please report problems with amalgamation so we can improve it.

**UA_BUILD_EXAMPLECLIENT** and **UA_BUILD_EXAMPLESERVER**

If you build your stack with the above two options, you will enable the example server/client applications to be built. You can review their sources under ``examples/server.c`` and ``example/client.c``. These provide a neat reference for just about any features of open62541, as most of them are included in these examples by us (the developers) for testing and demonstration purposes.

Unfortunately, these examples include just about everything the stack can do... which makes them rather bad examples for newcomers seeking an easy introduction. They are also dynamically configured by the CMake options, so they might be a bit more difficult to read. Nontheless you can find any of the concepts demonstrated here in these examples as well, and you can build them like so (and this is what you will see when you run them)::

   :open62541/build> make clean
   :open62541/build> cmake -DUA_BUILD_EXAMPLECLIENT=On -DUA_BUILD_EXAMPLESERVER=On ../
   :open62541/build> make
   :open62541/build> ./server &
   [07/28/2015 21:42:07.977.352] info/communication        Listening on opc.tcp://Cassandra:16664
   :open62541/build> ./client
   Browsing nodes in objects folder:
   NAMESPACE NODEID           BROWSE NAME      DISPLAY NAME    
   0         61               FolderType       FolderType      
   0         2253             Server           Server          
   1         96               current time     current time    
   1         the.answer       the answer       the answer      
   1         50000            Demo             Demo            
   1         62541            ping             ping            
   Create subscription succeeded, id 1187379785
   Monitoring 'the.answer', id 1187379785
   The Answer has changed!

   Reading the value of node (1, "the.answer"):
   the value is: 42

   Writing a value of node (1, "the.answer"):
   the new value is: 43
   The Answer has changed!
   Subscription removed
   Method call was unsuccessfull, and 80750000 returned values available.
   Created 'NewReference' with numeric NodeID 12133
   Created 'NewObjectType' with numeric NodeID 12134
   Created 'NewObject' with numeric NodeID 176
   Created 'NewVariable' with numeric NodeID 177
   :open62541/build> fg
   ./server
   [07/28/2015 21:43:21.815.890] info/server   Received Ctrl-C
   :open62541/build> 
   
**UA_BUILD_DOCUMENTATION**

If you have doxygen installed, this will produce a reference under ``/doc`` that documents functions that the shared library advertises (i.e. are available to users). We are doing our best to keep the source well commented.

**CMAKE_BUILD_TYPE**

There are several ways of building open62541, and all have their advantages and disadvanted. The build type mainly affects optimization flags (the more release, the heavier the optimization) and the inclusion of debugging symbols. The following are available:

 * Debug: Will only include debugging symbols (-g)
 * Release: Will run heavy optimization and *not* include debugging info (-O3 -DNBEBUG)
 * RelWithDebInfo: Will run mediocre optimization and include debugging symbols (-O2 -g)
 * MinSizeRel: Will run string optimziation and include no debugging info (-Os -DNBEBUG)

**WARNING:** If you are generating namespaces (please read the following sections), the compiler will try to optimize a function with 32k lines of generated code. This will propably result in a compilation run of >60Minutes (79min; 8-Core AMD FX; 16GB RAM; 64Bit Linux). Please pick build type ``Debug`` if you intend to compile large namespaces.

Conclusion
----------

In this first tutorial, you hopefully have compiled your first OPC UA Server with open62541. By going through that process, you now have a good impression of what steps building the stack involves and how you can use it. You were also introduced to several build options that affect the overall behavior of the compilation process. In the following tutorials, you will be shown how to build a client application and manipulate some nodes and variables.


