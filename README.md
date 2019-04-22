# open62541

open62541 (<http://open62541.org>) is an open source and free implementation of OPC UA (OPC Unified Architecture) written in the common subset of the C99 and C++98 languages. The library is usable with all major compilers and provides the necessary tools to implement dedicated OPC UA clients and servers, or to integrate OPC UA-based communication into existing applications. open62541 library is platform independent. All platform-specific functionality is implemented via exchangeable plugins. Plugin implementations are provided for the major operating systems.

open62541 is licensed under the Mozilla Public License v2.0. So the *open62541 library can be used in projects that are not open source*. Only changes to the open62541 library itself need to published under the same license. The plugins, as well as the server and client examples are in the public domain (CC0 license). They can be reused under any license and changes do not have to be published.

The library is [available](https://github.com/open62541/open62541/releases) in standard source and binary form. In addition, the single-file source distribution merges the entire library into a single .c and .h file that can be easily added to existing projects. Example server and client implementations can be found in the [/examples](examples/) directory or further down on this page.

## Project Information

Build Status:

[![Build Status](https://img.shields.io/travis/open62541/open62541/master.svg)](https://travis-ci.org/open62541/open62541)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/open62541/open62541?branch=master&svg=true)](https://ci.appveyor.com/project/open62541/open62541/branch/master)

Code Quality:

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/68ad08e897624c77a64fc2be66ca7b50)](https://www.codacy.com/app/open62541/open62541?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=open62541/open62541&amp;utm_campaign=Badge_Grade)
[![Total Alerts](https://img.shields.io/lgtm/alerts/g/open62541/open62541.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/open62541/open62541/alerts)
[![Coverage Status](https://img.shields.io/coveralls/open62541/open62541/master.svg)](https://coveralls.io/r/open62541/open62541?branch=master)
[![codecov](https://codecov.io/gh/open62541/open62541/branch/master/graph/badge.svg)](https://codecov.io/gh/open62541/open62541)
[![Code Quality: Cpp](https://img.shields.io/lgtm/grade/cpp/g/open62541/open62541.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/open62541/open62541/context:cpp)

General Project Info:

[![Open Hub Project Status](https://www.openhub.net/p/open62541/widgets/project_thin_badge.gif)](https://www.openhub.net/p/open62541/)
[![Overall Downloads](https://img.shields.io/github/downloads/open62541/open62541/total.svg)](https://github.com/open62541/open62541/releases)

### Features and Certification

open62541 implements the OPC UA binary protocol stack as well as a client and server SDK. The final server binaries can be well under 100kb, depending on the selected features and the size of the information model.

- Communication Stack
  - OPC UA binary protocol
  - OPC UA JSON encoding
  - Secure communication with encrypted messages
  - Exchangeable network layer (plugin) for using custom networking APIs (e.g. on embedded targets)
  - Support for generating data types from standard XML definitions
- Server
  - Support for all OPC UA node types
  - Access control for individual nodes
  - Support for generating server-side information models from standard XML definitions (nodesets)
  - Support for adding and removing nodes and references also at runtime.
  - Support for inheritance and instantiation of object- and variable-types (custom constructor/destructor, instantiation of child nodes)
  - Support for subscriptions/monitoreditems (data change notifications and events)
- Client
  - All OPC UA services supported
  - Asynchronous service requests
  - Background handling of subscriptions
- Publish/Subscribe
  - UADP Binary protocol with UDP-multicast or Ethernet communication
  - PubSub JSON encoding

open62541 is currently self-certifying. That is, the Conformance Testing Tools (CTT) of the OPC Foundation are regularly applied. But the SDK has not started an official certification so far.
The CTT configuration and results are tracked at https://github.com/open62541/open62541-ctt. The OPC UA profiles under regular test in the CTT are currently:

- Micro Embedded Device Server
- Method Server Facet

The goal for the upcoming 0.4 release is to support the *Embedded UA Server* profile in the CTT.
See the page on [open62541 Features](FEATURES.md) for an in-depth look at the support for the conformance units that make up the OPC UA profiles.
  
### Dependencies

On most systems, open62541 requires the C standard library only. For dependencies during the build process, see the following list and the [build documentation](https://open62541.org/doc/current/building.html) for details.

- Core Library: The core library has no dependencies besides the C99 standard headers.
- Default Plugins: The default plugins use the POSIX interfaces for networking and accessing the system clock. Ports to different (embedded) architectures are achieved by customizing the plugins.
- Building and Code Generation: The build environment is generated via CMake. Some code is auto-generated from XML definitions that are part of the OPC UA standard. The code generation scripts run with both Python 2 and 3.

### Code Quality

We emphasize code quality. The following quality metrics are continuously checked and are ensured to hold before an official release is made:

- Zero errors indicated by the Compliance Testing Tool (CTT) of the OPC Foundation for the supported features
- Zero compiler warnings from GCC/Clang/MSVC with very strict compilation flags
- Zero issues indicated by unit tests (more than 80% coverage)
- Zero issues indicated by clang-analyzer, clang-tidy, cpp-check and the Codacy static code analysis tools
- Zero unresolved issues from fuzzing the library in Google's oss-fuzz infrastructure
- Zero issues indicated by Valgrind (Linux), DrMemory (Windows) and Clang AddressSanitizer / MemorySanitizer for the CTT tests, unit tests and fuzzing

### Documentation and Support

A general introduction to OPC UA and the open62541 documentation can be found at http://open62541.org/doc/current.
Past releases of the library can be downloaded at https://github.com/open62541/open62541/releases.
To use the latest improvements, download a nightly build of the *single-file distribution* (the entire library merged into a single source and header file) from http://open62541.org/releases. Nightly builds of MSVC binaries of the library are available [here](https://ci.appveyor.com/project/open62541/open62541/build/artifacts).

For individual discussion and support, use the following channels:

- the [mailing list](https://groups.google.com/d/forum/open62541)
- our [IRC channel](http://webchat.freenode.net/?channels=%23open62541)
- the [bugtracker](https://github.com/open62541/open62541/issues)

We want to foster an open and welcoming community. Please take our [code of conduct](CODE_OF_CONDUCT.md) into regard.

Jointly with the overall open62541 community, the core maintainers steer the long-term development. The current core maintainers are (as of April 2018, in alphabetical order):

- Chris-Paul Iatrou (Dresden University of Technology, Chair for Process Control Systems Engineering)
- Florian Palm (RWTH Aachen University, Chair of Process Control Engineering)
- Julius Pfrommer (Fraunhofer IOSB, Karlsruhe)
- Stefan Profanter (fortiss, Munich)

## Support & Development

### Commercial Support

The open62541 community handles support requests for the open source library and its development. Custom development and individual support is provided by commercial partners that are affiliated with open62541:

- [Kalycito Infotech, Tamil Nadu, INDIA](https://www.kalycito.com/landing/open62541-commercial-partner) for embedded and realtime IIoT applications (Contact: enterprise.services@kalycito.com)
- [basysKom GmbH, Darmstadt, Germany](https://www.basyskom.com/) Software Engineering for Embedded Systems. (Contact: info@basysKom.com) 

For custom development that shall eventually become part of the open62541 library, please keep one of the core maintainers in the loop. Again, please note that all changes to files that are already licensed under the MPLv2 automatically become MPLv2 as well. Static linking of the open62541 library with code under a different license is possible. All architecture-specific code is implemented in the form of exchangeable plugins under a very permissible CC0 license.

### Development

As an open source project, new contributors are encouraged to help improve open62541. The following are good starting points for new contributors:

- [Report bugs](https://github.com/open62541/open62541/issues)
- Improve the [documentation](http://open62541.org/doc/current)
- Work on issues marked as "[good first issue](https://github.com/open62541/open62541/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)"

### Success Stories and Users of open62541

A list of projects and companies using our open62541 stack can be found in our Wiki:

https://github.com/open62541/open62541/wiki/References-to-open62541

## Examples

### Example Server Implementation
Compile the examples with the single-file distribution `open62541.h/.c` header and source file.
Using the GCC compiler, just run ```gcc -std=c99 -DUA_ARCHITECTURE_POSIX <server.c> open62541.c -o server``` (under Windows you may need to add ``` -lws2_32``` 
and change `-DUA_ARCHITECTURE_POSIX` to `-DUA_ARCHITECTURE_WIN32`).
```c
#include <signal.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

UA_Boolean running = true;
void signalHandler(int sig) {
    running = false;
}

int main(int argc, char** argv)
{
    signal(SIGINT, signalHandler); /* catch ctrl-c */

    /* Create a server listening on port 4840 */
    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

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
    return status;
}
```

### Example Client Implementation
```c
#include <stdio.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>

int main(int argc, char *argv[])
{
    /* Create a client and connect */
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
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
