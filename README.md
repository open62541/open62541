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

    // Browse nodes in objects folder    
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER); //browse objects folder
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;	//return everything

    UA_BrowseResponse bResp = UA_Client_browse(client, &bReq);
    printf("%-9s %-16s %-16s %-16s\n", "NAMESPACE", "NODEID", "BROWSE NAME", "DISPLAY NAME");
    for (int i = 0; i < bResp.resultsSize; ++i) {
        for (int j = 0; j < bResp.results[i].referencesSize; ++j) {
            UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);
            if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC){
                printf("%-9d %-16d %-16.*s %-16.*s\n", ref->browseName.namespaceIndex,
                ref->nodeId.nodeId.identifier.numeric, ref->browseName.name.length, 
                ref->browseName.name.data, ref->displayName.text.length, ref->displayName.text.data);
            }else if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING){
                printf("%-9d %-16.*s %-16.*s %-16.*s\n", ref->browseName.namespaceIndex,
                ref->nodeId.nodeId.identifier.string.length, ref->nodeId.nodeId.identifier.string.data,
                ref->browseName.name.length, ref->browseName.name.data, ref->displayName.text.length,
                ref->displayName.text.data);
            }
            //TODO: distinguish further types
        }
    }
    UA_BrowseRequest_deleteMembers(&bReq);
    UA_BrowseResponse_deleteMembers(&bResp);

    UA_Int32 value = 0;	
    //Read node's value
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

    UA_ReadResponse rResp = UA_Client_read(client, &rReq);
    if(rResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD &&
       rResp.resultsSize > 0 && rResp.results[0].hasValue &&
       rResp.results[0].value.data /* an empty array returns a null-ptr */ &&
       rResp.results[0].value.type == &UA_TYPES[UA_TYPES_INT32]){
           value = *(UA_Int32*)rResp.results[0].value.data;
           printf("the value is: %i\n", value);
   }

    UA_ReadRequest_deleteMembers(&rReq);
    UA_ReadResponse_deleteMembers(&rResp);
    
    value++;
	// Write node's value
	printf("\nWriting a value of node (1, \"the.answer\"):\n");
	UA_WriteRequest wReq;
	UA_WriteRequest_init(&wReq);
	wReq.nodesToWrite = UA_WriteValue_new();
	wReq.nodesToWriteSize = 1;
	wReq.nodesToWrite[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer"); /* assume this node exists */
	wReq.nodesToWrite[0].attributeId = UA_ATTRIBUTEID_VALUE;
	wReq.nodesToWrite[0].value.hasValue = UA_TRUE;
	wReq.nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_INT32];
	wReq.nodesToWrite[0].value.value.storageType = UA_VARIANT_DATA_NODELETE; //do not free the integer on deletion
	wReq.nodesToWrite[0].value.value.data = &value;

	UA_WriteResponse wResp = UA_Client_write(client, &wReq);
	if(wResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
			printf("the new value is: %i\n", value);

	UA_WriteRequest_deleteMembers(&wReq);
	UA_WriteResponse_deleteMembers(&wResp);
    
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return UA_STATUSCODE_GOOD;
}
```
