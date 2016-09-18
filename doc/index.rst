Introduction
============

OPC Unified Architecture
------------------------

`OPC UA <http://en.wikipedia.org/wiki/OPC_Unified_Architecture>`_ (short for OPC Unified Architecture) is a protocol for industrial communication and has been standardized in the IEC 62541 series. At its core, OPC UA defines a set of services to interact with a server-side object-oriented information model. Besides the service-calls initiated by the client, push-notification (discrete events and data changes with a sampling interval) can be negotiated with the server. The client/server interaction is mapped either to a binary encoding and TCP-based transmission or to SOAP-based webservices. As of late, OPC UA is marketed as the one standard for non-realtime industrial communication.

The standard itself can be purchased from IEC or downloaded for free from the website of the OPC Foundation (you need to register with a valid email, though).

open62541
---------

open62541 (http://open62541.org) is an open source implementation of OPC UA (OPC Unified Architecture). open62541 is a C-based library (linking with C++ projects is possible) with all necessary tools to implement dedicated OPC UA clients and servers, or to integrate OPC UA-based communication into existing applications. open62541 is licensed under the LGPL with a static linking exception. So the *open62541 library can be used in projects that are not open source*. However, changes to the open62541 library itself need to published under the same license. The [plugins](plugins/), as well as the server and client [examples](examples/) are in the public domain (CC0 license). They can be reused under any license and changes do not have to be published.

Motivation
^^^^^^^^^^

The open62541 implementation adheres to the official specification as closely as possible. However, no warranty or service-level agreement can be provided by the open62541 community. If you intend to use OPC UA in a mission-critical product, please consider talking to a commercial vendor of OPC UA SDKs and services.

Features
^^^^^^^^

open62541 implements the OPC UA binary protocol stack as well as a client and server SDK. It currently supports the Micro Embedded Device Server Profile plus some additional features. The final server binaries can be well under 100kb, depending on the size of the information model.

- Communication Stack

  - OPC UA binary protocol
  - Chunking (splitting of large messages)
  - Exchangeable network layer (plugin) for using custom networking APIs (e.g. on embedded targets)

- Information model

  - Support for all OPC UA node types (including method nodes)
  - Support for adding and removing nodes and references also at runtime.
  - Support for inheritance and instantiation of object- and variable-types (custom constructor/destructor, instantiation of child nodes)

- Subscriptions

  - Support for subscriptions/monitoreditems for data change notifications
  - Very low resource consumption for each monitored value (event-based server architecture)

- Code-Generation

  - Support for generating data types from standard XML definitions
  - Support for generating server-side information models (nodesets) from standard XML definitions

Features still missing in the 0.2 release are:

- Encryption
- Access control for individual nodes
- Events (notifications emitted by objects, data change notifications are implemented)
- Event-loop (background tasks) and asynchronous service requests in the client

Getting Help
^^^^^^^^^^^^

Releases of the library can be downloaded at https://github.com/open62541/open62541/releases. To use the latest improvements, download a nightly build of the *single-file distribution* (the entire library merged into a single source and header file) from http://open62541.org/releases. Nightly builds of MSVC binaries of the library are available [here](https://ci.appveyor.com/project/Stasik0/open62541/build/artifacts).

For discussion and help, you can use

- the [mailing list](https://groups.google.com/d/forum/open62541)
- our [IRC channel](http://webchat.freenode.net/?channels=%23open62541)
- the [bugtracker](https://github.com/open62541/open62541/issues)

Contributing to open62541
^^^^^^^^^^^^^^^^^^^^^^^^^

As an open source project, we invite new contributors to help improve open62541. Issue reports, bugfixes and new features are very welcome. Note that there are ways to begin contributing without deep knowledge of the OPC UA standard:

- [Report bugs](https://github.com/open62541/open62541/issues)
- Improve the [documentation](http://open62541.org/doc/current)
- Work on issues marked as "[easy hacks](https://github.com/open62541/open62541/labels/easy%20hack)"

Example Server Implementation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Compile the examples with the single-file distribution `open62541.h/.c` header and source file. Using the GCC compiler, just run ``gcc -std=c99 <server.c> open62541.c -o server`` to compile.

.. code-block:: c

    #include <signal.h>
    #include "open62541.h"

    UA_Boolean running = true;
    void signalHandler(int sig) {
        running = false;
    }

    int main(int argc, char** argv)
    {
        signal(SIGINT, signalHandler); /* catch ctrl-c */

        /* Create a server with one network layer listening on port 4840 */
        UA_ServerConfig config = UA_ServerConfig_standard;
        UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 4840);
        config.networkLayers = &nl;
        config.networkLayersSize = 1;
        UA_Server *server = UA_Server_new(config);

        /* Add a variable node */
        /* 1) Define the node attributes */
        UA_VariableAttributes attr;
        UA_VariableAttributes_init(&attr);
        attr.displayName = UA_LOCALIZEDTEXT("en_US", "the answer");
        UA_Int32 myInteger = 42;
        UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);

        /* 2) Define where the node shall be added with which browsename */
        UA_NodeId newNodeId = UA_NODEID_STRING(1, "the.answer");
        UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
        UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
        UA_NodeId variableType = UA_NODEID_NULL; /* take the default variable type */
        UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "the answer");

        /* 3) Add the node */
        UA_Server_addVariableNode(server, newNodeId, parentNodeId, parentReferenceNodeId,
                                browseName, variableType, attr, NULL, NULL);

        /* Run the server loop */
        UA_StatusCode status = UA_Server_run(server, &running);
        UA_Server_delete(server);
        nl.deleteMembers(&nl);
        return status;
    }

Example Client Implementation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c

    #include <stdio.h>
    #include "open62541.h"

    int main(int argc, char *argv[])
    {
        /* Create a client and connect */
        UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
        UA_StatusCode status = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(status != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
            return status;
        }

        /* Read the value attribute of the node. UA_Client_readValueAttribute is a
        * wrapper for the raw read service available as UA_Client_Service_read. */
        UA_Variant value; /* Variants can hold scalar values and arrays of any type */
        UA_Variant_init(&value);
        status = UA_Client_readValueAttribute(client, UA_NODEID_STRING(1, "the.answer"), &value);
        if(status == UA_STATUSCODE_GOOD && UA_Variant_isScalar(&value) &&
        value.type == &UA_TYPES[UA_TYPES_INT32]) {
            printf("the value is: %i\n", *(UA_Int32*)value.data);
        }

        /* Clean up */
        UA_Variant_deleteMembers(&value);
        UA_Client_delete(client); /* Disconnects the client internally */
        return status;
    }

.. toctree::
   :maxdepth: 2

   index
   building
   tutorials
   types
   information_modelling
   services
   server
   client
   constants
   internal
