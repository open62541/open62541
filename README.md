open62541
=========

open62541 (http://open62541.org) is an open-source implementation of OPC UA (OPC Unified Architecture) licensed under LGPL + static linking exception. The open62541 library can be used to build a dedicated OPC UA server or to integrate OPC UA-based communication into existing applications.

The project is in an early stage but already usable. See below for a simple server implemenation that provides access to a single variable.

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
- Report bugs
- Improve the documentation
- Work on issues marked as "easy hacks"

### Example Server Implementation
```c
#include <stdio.h>
#include <signal.h>

// provided by the open62541 lib
#include "ua_server.h"
#include "ua_namespace_0.h"

// provided by the user, implementations available in the /examples folder
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
    NetworklayerTCP *nl = NetworkLayerTCP_new(UA_ConnectionConfig_standard, PORT);
    UA_Server_addNetworkLayer(server, nl);

    /* add a variable node */
    UA_Int32 myInteger = 42;
    UA_String myIntegerName;
    UA_STRING_STATIC(myIntegerName, "The Answer");
    UA_Server_addScalarVariableNode(server,
                 /* the browse name, the value, and the datatype vtable */
                 &myIntegerName, (void*)&myInteger, &UA_TYPES[UA_INT32],
                 /* the parent node of the variable */
                 &UA_NODEIDS[UA_OBJECTSFOLDER],
                 /* the (hierarchical) referencetype from the parent */
                 &UA_NODEIDS[UA_ORGANIZES]);

    /* run the server loop */
    UA_StatusCode retval = UA_Server_run(server, WORKER_THREADS, &running);
	UA_Server_delete(server);
	return retval;
}
```
