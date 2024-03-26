# open62541

open62541 (<http://open62541.org>) is an open source implementation of OPC UA (OPC Unified Architecture / IEC 62541) written in the C language. The library is usable with all major compilers and provides the necessary tools to implement dedicated OPC UA clients and servers, or to integrate OPC UA-based communication into existing applications. The open62541 library is platform independent: All platform-specific functionality is implemented via exchangeable plugins for easy porting to different (embedded) targets.

open62541 is licensed under the Mozilla Public License v2.0 (MPLv2). This allows the open62541 library to be combined and distributed with any proprietary software. Only changes to the open62541 library itself need to be licensed under the MPLv2 when copied and distributed. The plugins, as well as the server and client examples are in the public domain (CC0 license). They can be reused under any license and changes do not have to be published.

The library is [available](https://github.com/open62541/open62541/releases) in standard source and binary form. In addition, the single-file source distribution merges the entire library into a single .c and .h file that can be easily added to existing projects. Example server and client implementations can be found in the [/examples](examples/) directory or further down on this page.

[![Open Hub Project Status](https://www.openhub.net/p/open62541/widgets/project_thin_badge.gif)](https://www.openhub.net/p/open62541/)
[![Build Status](https://dev.azure.com/open62541/open62541/_apis/build/status/open62541.open62541?branchName=master)](https://dev.azure.com/open62541/open62541/_build/latest?definitionId=1&branchName=master)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/open62541/open62541?branch=master&svg=true)](https://ci.appveyor.com/project/open62541/open62541/branch/master)
[![Code Scanning](https://github.com/open62541/open62541/actions/workflows/codeql.yml/badge.svg)](https://github.com/open62541/open62541/actions/workflows/codeql.yml)
[![Fuzzing Status](https://oss-fuzz-build-logs.storage.googleapis.com/badges/open62541.svg)](https://bugs.chromium.org/p/oss-fuzz/issues/list?sort=-opened&can=1&q=proj:open62541)
[![codecov](https://codecov.io/gh/open62541/open62541/branch/master/graph/badge.svg)](https://codecov.io/gh/open62541/open62541)

## Features

open62541 implements an OPC UA SDK with support for servers, clients and PubSub (publish-subscribe) communication.
See the [features overview](FEATURES.md) for full details.

- Core Stack
  - OPC UA binary and JSON encoding
  - TCP-based OPC UA SecureChannel
  - Custom data types (generated from XML definitions)
  - Portable C99 -- architecture-specific code is encapsulated behind standard interfaces
  - Highly configurable with default plugins for encryption (OpenSSL, mbedTLS), access control, historizing, logging, etc.
- Server
  - Support for all OPC UA services (except the Query service -- not implemented by any SDK)
  - Support for generating information models from standard XML definitions (Nodeset Compiler)
  - Support for adding and removing nodes and references at runtime
  - Support for subscriptions (data-change and event notifications)
- Client
  - Support for all OPC UA services
  - Support for asynchronous service requests
  - Background handling of subscriptions
- PubSub
  - PubSub message encoding (binary and JSON)
  - Transport over UDP-multicast, Ethernet, MQTT
  - Runtime configuration via the information model
  - Configurable realtime fast-path

## Commercial Use and Official Support

open62541 is licensed under the MPLv2. That is, changes to files under MPLv2 fall under the same open-source license.
But the library can be combined with private development from separate files, also if a static binary is produced, without the license affecting the private files.
See the full [license document](LICENSE) for details.

**Fraunhofer IOSB** maintains open62541 and provides **[official commercial support](https://www.iosb.fraunhofer.de/en/projects-and-products/open62541.html)**.
Additional service providers are listed on [open62541.org](https://www.open62541.org/).

## Official Certification

The sample server (server_ctt) built using open62541 v1.0 is in conformance with the 'Micro Embedded Device Server' Profile of OPC Foundation supporting OPC UA client/server communication, subscriptions, method calls and security (encryption) with the security policies 'Basic128Rsa15', 'Basic256' and 'Basic256Sha256' and the facets 'method server' and 'node management'. See https://open62541.org/certified-sdk for more details.

PubSub (UADP) is implemented in open62541. But the feature cannot be certified at this point in time (Sep-2019) due to the lack of official test cases and testing tools.

During development, the Conformance Testing Tools (CTT) of the OPC Foundation are regularly applied.
The CTT configuration and results are tracked at https://github.com/open62541/open62541-ctt. The OPC UA profiles under regular test in the CTT are currently:

- Micro Embedded Device Server
- Method Server Facet
- Node Management Facet
- Security Policies
  - Basic128Rsa15
  - Basic256
  - Basic256Sha256
- User Tokens
  - Anonymous Facet
  - User Name Password Server Facet

See the page on [open62541 Features](FEATURES.md) for an in-depth look at the support for the conformance units that make up the OPC UA profiles.
  
## Documentation and Support

A general introduction to OPC UA and the open62541 documentation can be found at http://open62541.org.
Past releases of the library can be downloaded at https://github.com/open62541/open62541/releases.

The overall open62541 community handles public support requests on Github and the mailing list.
For individual discussion and support, use the following channels:

- [Mailing List](https://groups.google.com/d/forum/open62541)
- [Issue Tracker](https://github.com/open62541/open62541/issues)
- [Pull Requests](https://github.com/open62541/open62541/pulls)

We want to foster an open and welcoming community. Please take our [code of conduct](CODE_OF_CONDUCT.md) into regard.

## Development

As an open source project, new contributors are encouraged to help improve open62541. The file [CONTRIBUTING.md](CONTRIBUTING.md) aggregates good practices that we expect for code contributions. The following are good starting points for new contributors:

- [Report bugs](https://github.com/open62541/open62541/issues)
- Improve the [documentation](http://open62541.org/doc/current)
- Work on issues marked as "[good first issue](https://github.com/open62541/open62541/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)"

For custom development that shall eventually become part of the open62541 library, please keep one of the core maintainers in the loop.

### Dependencies

On most systems, open62541 requires the C standard library only. For dependencies during the build process, see the following list and the [build documentation](https://www.open62541.org/doc/master/building.html) for details.

- Core Library: The core library has no dependencies besides the C99 standard headers.
- Default Plugins: The default plugins use the POSIX interfaces for networking and accessing the system clock. Ports to different (embedded) architectures are achieved by customizing the plugins.
- Building and Code Generation: The build environment is generated via CMake. Some code is auto-generated from XML definitions that are part of the OPC UA standard. The code generation scripts run with both Python 2 and 3.

**Note:**
Some (optional) features are dependent on third-party libraries. These are all listed under the `deps/` folder.
Depending on the selected feature set, some of these libraries will be included in the resulting library.
More information on the third-party libraries can be found in the corresponding [deps/README.md](deps/README.md)

### Code Quality

We emphasize code quality. The following quality metrics are continuously checked and are ensured to hold before an official release is made:

- Zero errors indicated by the Compliance Testing Tool (CTT) of the OPC Foundation for the supported features
- Zero compiler warnings from GCC/Clang/MSVC with very strict compilation flags
- Zero issues indicated by unit tests (more than 80% coverage)
- Zero issues indicated by clang-analyzer, clang-tidy, cpp-check and the Codacy static code analysis tools
- Zero unresolved issues from fuzzing the library in Google's oss-fuzz infrastructure
- Zero issues indicated by Valgrind (Linux), DrMemory (Windows) and Clang AddressSanitizer / MemorySanitizer for the CTT tests, unit tests and fuzzing

## Installation and code usage

For every release, we provide some pre-packed release packages which you can directly use in your compile infrastructure.

Have a look at the [release page](https://github.com/open62541/open62541/releases) and the corresponding attached assets.

A more detailed explanation on how to install the open62541 SDK is given in our [documentation](https://www.open62541.org/doc/master/building.html#building-the-library).
In essence, clone the repository and initialize all the submodules using `git submodule update --init --recursive`. Then use CMake to configure your build.

Furthermore we provide "pack branches" that are up-to-date with the corresponding base branches, and in addition have the git submodules in-place for a zip download.
Here are some direct download links for the current pack branches:  

  - [pack/master.zip](https://github.com/open62541/open62541/archive/pack/master.zip)
  - [pack/1.0.zip](https://github.com/open62541/open62541/archive/pack/1.0.zip)

## Examples

A complete list of examples can be found in the [examples directory](https://github.com/open62541/open62541/tree/master/examples).
To build the examples, we recommend to install open62541 as mentioned in the previous section.
Using the GCC compiler, just run ```gcc -std=c99 <server.c> -lopen62541 -o server``` (under Windows you may need to add additionally link against the ```ws2_32``` 
socket library).

### Example Server Implementation

```c
#include <open62541/server.h>

int main(int argc, char** argv)
{
    /* Create a server listening on port 4840 (default) */
    UA_Server *server = UA_Server_new();

    /* Add a variable node to the server */

    /* 1) Define the variable attributes */
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
    UA_Server_addVariableNode(server, newNodeId, parentNodeId,
                              parentReferenceNodeId, browseName,
                              variableType, attr, NULL, NULL);

    /* Run the server (until ctrl-c interrupt) */
    UA_StatusCode status = UA_Server_runUntilInterrupt(server);

    /* Clean up */
    UA_Server_delete(server);
    return status == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
```

### Example Client Implementation

```c
#include <stdio.h>
#include <open62541/client.h>
#include <open62541/client_highlevel.h>

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
    UA_Variant_clear(&value);
    UA_Client_delete(client); /* Disconnects the client internally */
    return status == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
```
