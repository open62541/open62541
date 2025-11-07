<div align="center">
  <a href="https://open62541.org">
    <img alt="open62541 Logo" src="https://open62541.org/images/logo-open62541.svg" width="400px">
  </a>
</div>
<br />

open62541 (<http://open62541.org>) is an open source implementation of OPC UA (OPC Unified Architecture / IEC 62541) written in the C language. The library is usable with all major compilers and provides the necessary tools to implement dedicated OPC UA clients and servers, or to integrate OPC UA-based communication into existing applications. See the [features overview](FEATURES.md) for full details.
The open62541 library is platform independent: All platform-specific functionality is implemented via exchangeable plugins for easy porting to different (embedded) targets.

open62541 is licensed under the Mozilla Public License v2.0 (MPLv2). This allows the open62541 library to be combined and distributed with any proprietary software. Only changes to the open62541 library itself need to be licensed under the MPLv2 when copied and distributed. Some plugins and examples are in the public domain (CC0 license) and some are licensed under MPLv2. The CC0 licensed ones can be reused under any license and changes do not have to be published.

The library is available in standard source and binary form. In addition, the single-file source distribution merges the entire library into a single .c and .h file that can be easily added to existing projects. Example server and client implementations can be found in the [/examples](examples/) directory or further down on this page.

[![Open Hub Project Status](https://www.openhub.net/p/open62541/widgets/project_thin_badge.gif)](https://www.openhub.net/p/open62541/)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/open62541/open62541?branch=master&svg=true)](https://ci.appveyor.com/project/open62541/open62541/branch/master)
[![Code Scanning](https://github.com/open62541/open62541/actions/workflows/codeql.yml/badge.svg)](https://github.com/open62541/open62541/actions/workflows/codeql.yml)
[![Fuzzing Status](https://oss-fuzz-build-logs.storage.googleapis.com/badges/open62541.svg)](https://bugs.chromium.org/p/oss-fuzz/issues/list?sort=-opened&can=1&q=proj:open62541)
[![codecov](https://codecov.io/gh/open62541/open62541/branch/master/graph/badge.svg)](https://codecov.io/gh/open62541/open62541)

## Documentation and Support

A general introduction to OPC UA and the open62541 documentation can be found at http://open62541.org.
Past releases of the library can be downloaded at https://github.com/open62541/open62541/releases.

The overall open62541 community handles public support requests on Github and the mailing list.
For individual discussion and support, use the following channels:

- [Mailing List](https://groups.google.com/d/forum/open62541)
- [Issue Tracker](https://github.com/open62541/open62541/issues)
- [Pull Requests](https://github.com/open62541/open62541/pulls)

[o6 Automation GmbH](https://www.o6-automation.com/) employs the core contributors to open62541 and provides **[commercial support](https://www.o6-automation.com/services)**.
The project is however open to outside contributions and **contributors retain their individual copyright**. This prevents future relicensing under different license conditions.

We want to foster an open and welcoming community. Please take our [code of conduct](CODE_OF_CONDUCT.md) into regard.

## Commercial Use

open62541 is licensed under the MPLv2. That is, changes to files under MPLv2 fall under the same open-source license.
But the library can be combined with private development from separate files, also if a static binary is produced, without the license affecting the private files.
See the full [license document](LICENSE) for details.

## Official Certification

An example server built with open62541 v1.4 was certified for the 'Standard Server 2017 Profile' by the OPC Foundation.
See https://open62541.org/certification for more details.

## Build System, Code Structure and Dependencies

The build environment of open62541 is generated via CMake. See the [build documentation](https://www.open62541.org/doc/master/building.html) for details.
To simplify the integration with existing software projects, the open62541 sources can be compressed (amalgamated) into a single-file-distribution, a pair of `open62541.c/.h` files.
The functionality included in the single-file-distribution depends on the current CMake configuration.

The source code is structured as follows:

- Public API (`/include`): The public API is exposed to applications using open62541. The headers for plugin implementations are in `/plugins/include`.
- Core Library (`/src`): The core library has no dependencies besides the C99 standard headers.
- Architecture Support (`/arch`): Architecture support is implemented via the `EventLoop` plugin. This keeps the architecture-specific code - for example to use the POSIX APIs - out of the core library. Ports to different (embedded) architectures are provided.
- Default Plugins Implementations (`/plugins`): The plugin interfaces allow the integration with different backend systems and libraries. For example concerning crypto primitives, storage of the information model, and so on. Default implementations are provided.
- Dependencies (`/deps`): Some additional libraries are used via git submodules or have been internalized in the `deps/` folder. More information on the third-party libraries and their respective licenses can be found in [deps/README.md](deps/README.md)
- Building and Code Generation: Some code is auto-generated from XML definitions that are part of the OPC UA standard. The code generation scripts use Python as part of the build process.

On most systems, a bare-bones open62541 requires the C standard library only.
Depending on the build configuration, open62541 depends on additional libraries, such as mbedTLS or OpenSSL for encryption.

## Development

As an open source project, new contributors are encouraged to help improve open62541.
The file [CONTRIBUTING.md](CONTRIBUTING.md) aggregates good practices that we expect for code contributions.
The following are good starting points for new contributors:

- [Report bugs](https://github.com/open62541/open62541/issues)
- Improve the [documentation](http://open62541.org/doc/current)
- Work on issues marked as "[good first issue](https://github.com/open62541/open62541/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)"

For custom development that shall eventually become part of the open62541 library, please keep one of the core maintainers in the loop.

### Code Quality

We emphasize code quality. The following quality metrics are continuously checked and are ensured to hold before an official release is made:

- Zero errors indicated by the Compliance Testing Tool (CTT) of the OPC Foundation for the supported features
- Zero compiler warnings from GCC/Clang/MSVC with very strict compilation flags
- Zero issues indicated by unit tests (we target more than 80% code coverage)
- Zero issues indicated by clang-analyzer, clang-tidy, cpp-check and the Codacy static code analysis tools
- Zero unresolved issues from fuzzing the library in Google's oss-fuzz infrastructure
- Zero issues indicated by Valgrind (Linux), DrMemory (Windows) and Clang AddressSanitizer / MemorySanitizer for the CTT tests, unit tests and fuzzing

### Security and Vulnerability Handling

The project has established a process for handling vulnerabilities.
See the [SECURITY.md](SECURITY.md) for details and how to responsibly disclose findings to the maintainers.

## Installation and Examples

On Debian/Ubuntu systems, a simple ```apt install libopen62541-1.4-dev``` installs the library and the development header files.
Using the GCC compiler, just run ```gcc -std=c99 <server.c> -lopen62541 -o server```.

A more detailed explanation on how to install the open62541 SDK is given in our [documentation](https://www.open62541.org/doc/master/building.html#building-the-library).
In essence, clone the repository and initialize all the submodules using `git submodule update --init --recursive`. Then use CMake to configure your build.

A complete list of examples can be found in the [examples directory](https://github.com/open62541/open62541/tree/master/examples).

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
    UA_NodeId parentNodeId = UA_NS0ID(OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NS0ID(ORGANIZES);
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
