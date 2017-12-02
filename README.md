open62541
=========

open62541 (http://open62541.org) is an open source and free implementation of OPC UA (OPC Unified Architecture) written in the common subset of the C99 and C++98 languages. The library is usable with all major compilers and provides the necessary tools to implement dedicated OPC UA clients and servers, or to integrate OPC UA-based communication into existing applications. open62541 library is platform independent. All platform-specific functionality is implemented via exchangeable plugins. Plugin implementations are provided for the major operating systems.

open62541 is licensed under the Mozilla Public License v2.0. So the *open62541 library can be used in projects that are not open source*. Only changes to the open62541 library itself need to published under the same license. The plugins, as well as the server and client examples are in the public domain (CC0 license). They can be reused under any license and changes do not have to be published.

The library is [available](https://github.com/open62541/open62541/releases) in standard source and binary form. In addition, the single-file source distribution merges the entire library into a single .c and .h file that can be easily added to existing projects. Example server and client implementations can be found in the [/examples](examples/) directory or further down on this page.

[![Ohloh Project Status](https://www.ohloh.net/p/open62541/widgets/project_thin_badge.gif)](https://www.ohloh.net/p/open62541)
[![Build Status](https://img.shields.io/travis/open62541/open62541/master.svg)](https://travis-ci.org/open62541/open62541)
[![MSVS build status](https://img.shields.io/appveyor/ci/open62541/open62541/master.svg)](https://ci.appveyor.com/project/open62541/open62541/branch/master)
[![Coverity Scan Build Status](https://img.shields.io/coverity/scan/12248.svg)](https://scan.coverity.com/projects/open62541-open62541)
[![Coverage Status](https://img.shields.io/coveralls/open62541/open62541/master.svg)](https://coveralls.io/r/open62541/open62541?branch=master)
[![Overall Downloads](https://img.shields.io/github/downloads/open62541/open62541/total.svg)](https://github.com/open62541/open62541/releases)

### Features

For a complete list of features check: [open62541 Features](FEATURES.md)

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
- Code quality checks
  - Every commit checked with continuous integration and unit tests (code coverage > 80%)
  - Memory leak detection using Valgrind (Linux) and DrMemory (Windows)
  - Static code analysis with cpp-check and clang static code analysis
  - Fuzz testing with [oss-fuzz](https://github.com/google/oss-fuzz)
  
Features currently being implemented:
- Target 0.3 release (to be released in the coming weeks):
  - Encryption (#1282)
  - Access control for individual nodes (Done)
- Target 0.4 release:
  - Events (notifications emitted by objects, data change notifications are implemented), WIP by @Pro
  - Event-loop (background tasks) and asynchronous service requests in the client, WIP #1117 #1410
  - Publish/Subscribe based on UDP (Specification Part 14), WIP by @jpfr

### Using open62541

A general introduction to OPC UA and the open62541 documentation can be found at http://open62541.org/doc/current.
Past releases of the library can be downloaded at https://github.com/open62541/open62541/releases.
To use the latest improvements, download a nightly build of the *single-file distribution* (the entire library merged into a single source and header file) from http://open62541.org/releases. Nightly builds of MSVC binaries of the library are available [here](https://ci.appveyor.com/project/open62541/open62541/build/artifacts).

For discussion and help, you can use
- the [mailing list](https://groups.google.com/d/forum/open62541)
- our [IRC channel](http://webchat.freenode.net/?channels=%23open62541)
- the [bugtracker](https://github.com/open62541/open62541/issues)

### Development

Besides the general open62541 community, a group of core maintainers jointly steers the long-term development. The current core maintainers are (as of Mai 2017, in alphabetical order):

- Chris-Paul Iatrou (Dresden University of Technology, Chair for Process Control Systems Engineering)
- Florian Palm (RWTH Aachen University, Chair of Process Control Engineering)
- Julius Pfrommer (Fraunhofer IOSB, Karlsruhe)
- Stefan Profanter (fortiss, Munich)

As an open source project, we encourage new contributors to help improve open62541. There are ways to begin contributing without deep knowledge of the OPC UA standard:
- [Report bugs](https://github.com/open62541/open62541/issues)
- Improve the [documentation](http://open62541.org/doc/current)
- Work on issues marked as "[easy hacks](https://github.com/open62541/open62541/labels/easy%20hack)"

### Example Server Implementation
Compile the examples with the single-file distribution `open62541.h/.c` header and source file.
Using the GCC compiler, just run ```gcc -std=c99 <server.c> open62541.c -o server``` (under Windows you may need to add ``` -lws2_32```).
```c
#include <signal.h>
#include "open62541.h"

UA_Boolean running = true;
void signalHandler(int sig) {
    running = false;
}

int main(int argc, char** argv)
{
    signal(SIGINT, signalHandler); /* catch ctrl-c */

    /* Create a server listening on port 4840 */
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

    /* Add a variable node */
    /* 1) Define the node attributes */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "the answer");
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
    UA_ServerConfig_delete(config);
    return status;
}
```

### Example Client Implementation
```c
#include <stdio.h>
#include "open62541.h"

int main(int argc, char *argv[])
{
    /* Create a client and connect */
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
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
    if(status == UA_STATUSCODE_GOOD &&
       UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32])) {
        printf("the value is: %i\n", *(UA_Int32*)value.data);
    }

    /* Clean up */
    UA_Variant_deleteMembers(&value);
    UA_Client_delete(client); /* Disconnects the client internally */
    return status;
}
```
