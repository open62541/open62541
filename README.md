open62541
=========

open62541 (http://open62541.org) is an open-source implementation of OPC UA (OPC Unified Architecture). open62541 is a C-based library that contains all the necessary tools to set up a dedicated OPC UA server or to integrate OPC UA-based communication into existing applications. An example server implementation can be found in the /examples directory or further down on this page.

open62541 is licensed under the LGPL + static linking exception. That means **open62541 can be freely used also in commercial projects**, although changes to the open62541 library itself need to be released under the same license. The server and client implementations in the /examples directory are in the public domain (CC0 license). They can be used under any license and changes don't have to be published.

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
As an open source project, we invite new contributors to help improving open6241. If you are a developer, your bugfixes and new features are very welcome. Note that there are ways to contribute even without deep knowledge of the project or the UA standard:
- [Report bugs](https://github.com/acplt/open62541/issues)
- Improve the [documentation](http://open62541.org/doc)
- Work on issues marked as "[easy hacks](https://github.com/acplt/open62541/labels/easy%20hack)"

### Example Server Implementation
```c
#include <signal.h>

/* provided by the open62541 lib */
#include "ua_server.h"

/* provided by the user, implementations available in the /examples folder */
#include "logger_stdout.h"
#include "networklayer_tcp.h"

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
    NetworklayerTCP *nl = ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, PORT);
    UA_Server_addNetworkLayer(server, nl);

    /* add a variable node */
    UA_Int32 *myInteger = UA_Int32_new();
    *myInteger = 42;
    UA_Variant *myIntegerVariant = UA_Variant_new();
    UA_Variant_setValue(myIntegerVariant, myInteger, UA_TYPES_INT32);
    UA_QualifiedName myIntegerName;
    UA_QUALIFIEDNAME_ASSIGN(myIntegerName, "the answer");
    UA_Server_addVariableNode(server,
                              myIntegerVariant, /* the variant */
                              &UA_NODEID_NULL, /* assign a new nodeid */
                              &myIntegerName, /* the browse name */
                              /* the parent node and the referencetype to the parent */
                              &UA_NODEID_STATIC(0, UA_NS0ID_OBJECTSFOLDER),
                              &UA_NODEID_STATIC(0, UA_NS0ID_ORGANIZES));

    /* run the server loop */
    UA_StatusCode retval = UA_Server_run(server, WORKER_THREADS, &running);
	UA_Server_delete(server);
	return retval;
}
```
