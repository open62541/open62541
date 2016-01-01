open62541
=========

open62541 (http://open62541.org) is an open source and free implementation of OPC UA (OPC Unified Architecture). open62541 is a C-based library that contains all the necessary tools to set up a dedicated OPC UA server, to integrate OPC UA-based communication into existing applications (linking with C++ projects [is possible](examples/server.cpp)), or to create an OPC UA client. The library is distributed as a single pair of [header](https://github.com/open62541/open62541/releases/download/v0.1.1/open62541.h) and [source](https://github.com/open62541/open62541/releases/download/v0.1.1/open62541.c) files, that can be easily dropped into your project. An example server and client implementation can be found in the [/examples](examples/) directory or further down on this page.

open62541 is licensed under the LGPL + static linking exception. That means **open62541 can be freely used also in commercial projects**, although changes to the open62541 library itself need to be released under the same license. The server and client implementations in the [/examples](examples/) directory are in the public domain (CC0 license). They can be used under any license and changes don't have to be published.

[![Ohloh Project Status](https://www.ohloh.net/p/open62541/widgets/project_thin_badge.gif)](https://www.ohloh.net/p/open62541)
[![Build Status](https://travis-ci.org/open62541/open62541.png?branch=master)](https://travis-ci.org/open62541/open62541)
[![MSVS build status](https://ci.appveyor.com/api/projects/status/kkxppc28ek5t6yk8/branch/master?svg=true)](https://ci.appveyor.com/project/Stasik0/open62541/branch/master)
[![Coverage Status](https://coveralls.io/repos/open62541/open62541/badge.png?branch=master)](https://coveralls.io/r/open62541/open62541?branch=master)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/1864/badge.svg)](https://scan.coverity.com/projects/1864)

### Documentation
A general introduction to OPC UA and the open62541 documentation can be found at http://open62541.org/doc/current.
Build instruction are here: https://github.com/open62541/open62541/wiki/Building-open62541.

For discussion and help, you can use
- the [mailing list](https://groups.google.com/d/forum/open62541)
- our [IRC channel](http://webchat.freenode.net/?channels=%23open62541)
- the [bugtracker](https://github.com/open62541/open62541/issues)

Automated builds of 50 last single-file distributions are available [here](http://open62541.org/releases).
Automatically compiled MSVC binaries are available [here](https://ci.appveyor.com/project/Stasik0/open62541/build/artifacts).

### Contribute to open62541
As an open source project, we invite new contributors to help improving open62541. If you are a developer, your bugfixes and new features are very welcome. Note that there are ways to contribute even without deep knowledge of the project or the UA standard:
- [Report bugs](https://github.com/open62541/open62541/issues)
- Improve the [documentation for 0.1](http://open62541.org/doc/0.1) and [documentation for 0.2](http://open62541.org/doc/current)
- Work on issues marked as "[easy hacks](https://github.com/open62541/open62541/labels/easy%20hack)"

### Example Server Implementation
Compile the examples with the single file distribution `open62541.h` and `open62541.c` from the latest [release](https://github.com/open62541/open62541/releases).
With the GCC compiler, just run ```gcc -std=c99 <server.c> open62541.c -o server```.
```c
#include <signal.h>
#include "open62541.h"

#define PORT 16664

UA_Boolean running = UA_TRUE;
void signalHandler(int sign) {
    running = UA_FALSE;
}

int main(int argc, char** argv)
{
    /* catch ctrl-c */
    signal(SIGINT, signalHandler);

    /* init the server */
    UA_ServerConfig config = UA_ServerConfig_standard;
    config.running = &running;
    config.logger = Logger_Stdout;
    UA_Server *server = UA_Server_new(config);
    UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, PORT));

    /* add a variable node */
    /* 1) set the variable attributes (no memory allocations here) */
    UA_Int32 myInteger = 42;
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.displayName = UA_LOCALIZEDTEXT("en_US","the answer");

    /* 2) define where the variable shall be added with which browsename */
    UA_NodeId newNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableType = UA_NODEID_NULL; /* no variable type defined */
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "the answer");

    /* 3) add the variable */
    UA_Server_addVariableNode(server, newNodeId, parentNodeId,
                              parentReferenceNodeId, browseName,
                              variableType, attr, NULL);

    /* run the server loop */
    UA_StatusCode retval = UA_Server_run(server);
    UA_Server_delete(server);
    return retval;
}
```

### Example Client Implementation
```c
#include <stdio.h>
#include "open62541.h"

int main(int argc, char *argv[])
{
    /* create a client and connect */
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout_new());
    UA_StatusCode retval = UA_Client_connect(client, ClientNetworkLayerTCP_connect,
                                             "opc.tcp://localhost:16664");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
    	return retval;
    }

    /* create a readrequest with one entry */
    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.nodesToRead = UA_ReadValueId_new();
    req.nodesToReadSize = 1;
    
    /* define the node and attribute to be read */
    req.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    req.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

    /* call the service and print the result */
    UA_ReadResponse resp = UA_Client_Service_read(client, req);
    if(resp.responseHeader.serviceResult == UA_STATUSCODE_GOOD &&
       resp.resultsSize > 0 && resp.results[0].hasValue &&
       UA_Variant_isScalar(&resp.results[0].value) &&
       resp.results[0].value.type == &UA_TYPES[UA_TYPES_INT32]) {
           UA_Int32 *value = (UA_Int32*)resp.results[0].value.data;
           printf("the value is: %i\n", *value);
   }

    UA_ReadRequest_deleteMembers(&req);
    UA_ReadResponse_deleteMembers(&resp);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return UA_STATUSCODE_GOOD;
}
```
