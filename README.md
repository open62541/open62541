open62541
=========

open62541 (http://open62541.org) is an open source and free implementation of OPC UA (OPC Unified Architecture). open62541 is a C-based library that contains all the necessary tools to set up a dedicated OPC UA server, to integrate OPC UA-based communication into existing applications (linking with C++ projects [is possible](examples/server.cpp)), or to create an OPC UA client. The library is distributed as a single pair of [header](http://open62541.org/open62541.h) and [source](http://open62541.org/open62541.c) files, that can be easily dropped into your project. An example server and client implementation can be found in the [/examples](examples/) directory or further down on this page.

open62541 is licensed under the LGPL + static linking exception. That means **open62541 can be freely used also in commercial projects**, although changes to the open62541 library itself need to be released under the same license. The server and client implementations in the [/examples](examples/) directory are in the public domain (CC0 license). They can be used under any license and changes don't have to be published.

[![Ohloh Project Status](https://www.ohloh.net/p/open62541/widgets/project_thin_badge.gif)](https://www.ohloh.net/p/open62541)
[![Build Status](https://travis-ci.org/acplt/open62541.png?branch=master)](https://travis-ci.org/acplt/open62541)
[![Coverage Status](https://coveralls.io/repos/acplt/open62541/badge.png?branch=master)](https://coveralls.io/r/acplt/open62541?branch=master)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/1864/badge.svg)](https://scan.coverity.com/projects/1864)

### Documentation
A general introduction to OPC UA and the open62541 documentation can be found at http://open62541.org/doc.
Build instruction are here: https://github.com/acplt/open62541/wiki/Building-open62541.

For discussion and help, you can use
- the [mailing list](https://groups.google.com/d/forum/open62541)
- our [IRC channel](http://webchat.freenode.net/?channels=%23open62541)
- the [bugtracker](https://github.com/acplt/open62541/issues)

### Contribute to open62541
As an open source project, we invite new contributors to help improving open62541. If you are a developer, your bugfixes and new features are very welcome. Note that there are ways to contribute even without deep knowledge of the project or the UA standard:
- [Report bugs](https://github.com/acplt/open62541/issues)
- Improve the [documentation](http://open62541.org/doc)
- Work on issues marked as "[easy hacks](https://github.com/acplt/open62541/labels/easy%20hack)"

### Example Server Implementation
Compile the examples with the single file distribution `open62541.h` and `open62541.c` from the latest [release](https://github.com/acplt/open62541/releases).
With the GCC compiler, just run ```gcc -std=c99 <server.c> open62541.c -o server```.
```c
#include <signal.h>
#include "open62541.h"

#define WORKER_THREADS 2 /* if multithreading is enabled */
#define PORT 16664

UA_Boolean running = UA_TRUE;
void signalHandler(int sign) {
    running = UA_FALSE;
}

int main(int argc, char** argv) {
    /* catch ctrl-c */
    signal(SIGINT, signalHandler);

    /* init the server */
    UA_Server *server = UA_Server_new();
    UA_Server_addNetworkLayer(server,
        ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, PORT));
    UA_Server_setLogger(server, Logger_Stdout_new());

    /* add a variable node */
    UA_Variant *myIntegerVariant = UA_Variant_new();
    UA_Int32 myInteger = 42;
    UA_Variant_setScalarCopy(myIntegerVariant, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerVariant, myIntegerName,
                              myIntegerNodeId, parentNodeId, parentReferenceNodeId);

    /* run the server loop */
    UA_StatusCode retval = UA_Server_run(server, WORKER_THREADS, &running);
    UA_Server_delete(server);
    return retval;
}
```

### Example Client Implementation
```c
#include <stdio.h>
#include "open62541.h"

int main(int argc, char *argv[]) {
	UA_Client *client = UA_Client_new();
	UA_ClientNetworkLayer nl = ClientNetworkLayerTCP_new(UA_ConnectionConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, UA_ConnectionConfig_standard, nl,
                                             "opc.tcp://localhost:16664");
	if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
    	return retval;
    }

    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.nodesToRead = UA_ReadValueId_new();
    req.nodesToReadSize = 1;
    req.nodesToRead[0].nodeId = UA_NODEID_STRING(1, "the.answer");
    req.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

    UA_ReadResponse resp = UA_Client_read(client, &req);
    if(resp.responseHeader.serviceResult == UA_STATUSCODE_GOOD &&
       resp.resultsSize > 0 && resp.results[0].hasValue &&
       resp.results[0].value.data /* an empty array returns a null-ptr */ &&
       resp.results[0].value.type == &UA_TYPES[UA_TYPES_INT32])
        printf("the answer is: %i\n", *(UA_Int32*)resp.results[0].value.data);

    UA_ReadRequest_deleteMembers(&req);
    UA_ReadResponse_deleteMembers(&resp);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return UA_STATUSCODE_GOOD;
}
```
