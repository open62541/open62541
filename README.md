open62541
=========

open62541 (http://open62541.org) is an open-source implementation of OPC UA (OPC Unified Architecture) licensed under LGPL + static linking exception. The open62541 library can be used to build a dedicated OPC UA server or to integrate OPC UA-based communication into existing applications.

The project is in an early stage but already usable. See below for a simple server implemenation that provides access to a single variable.

[![Ohloh Project Status](https://www.ohloh.net/p/open62541/widgets/project_thin_badge.gif)](https://www.ohloh.net/p/open62541)
[![Build Status](https://travis-ci.org/acplt/open62541.png?branch=master)](https://travis-ci.org/acplt/open62541)
[![Coverage Status](https://coveralls.io/repos/acplt/open62541/badge.png?branch=master)](https://coveralls.io/r/acplt/open62541?branch=master)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/1864/badge.svg)](https://scan.coverity.com/projects/1864)

### Documentation
The developer documentation is generated from Doxygen annotations in the source code: http://open62541.org/doc.
Build instruction can be found under https://github.com/acplt/open62541/wiki/Building-open62541.

For discussion and help, you can use
- the [mailing list](https://groups.google.com/d/forum/open62541)
- our [IRC channel](http://webchat.freenode.net/?channels=%23open62541)
- the [bugtracker](https://github.com/acplt/open62541/issues)

If open62541 is not fit for your needs, have a look at some [other open source OPC UA implementations](https://github.com/acplt/open62541/wiki/List-of-Open-Source-OPC-UA-Implementations).
Or even better, help us improving open62541 by sending bug reports/bugfixes on github or the mailing list!

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

UA_Boolean running = UA_TRUE;
void stopHandler(int sign) {
	running = UA_FALSE;
}

void serverCallback(UA_Server *server) {
    // add your maintenance functionality here
    printf("does whatever servers do\n");
}

int main(int argc, char** argv) {
	signal(SIGINT, stopHandler); /* catches ctrl-c */

    /* init the server */
	#define PORT 16664
	UA_String endpointUrl;
	UA_String_copyprintf("opc.tcp://127.0.0.1:%i", &endpointUrl, PORT);
	UA_Server *server = UA_Server_new(&endpointUrl, NULL);

    /* add a variable node */
    UA_Int32 myInteger = 42;
    UA_String myIntegerName;
    UA_STRING_STATIC(myIntegerName, "The Answer");
    UA_Server_addScalarVariableNode(server,
                 /* browse name, the value and the datatype's vtable */
                 &myIntegerName, (void*)&myInteger, &UA_TYPES[UA_INT32],
                 /* the parent node where the variable shall be attached */
                 &UA_NODEIDS[UA_OBJECTSFOLDER],
                 /* the (hierarchical) referencetype from the parent */
                 &UA_NODEIDS[UA_HASCOMPONENT]);

    /* attach a network layer */
	NetworklayerTCP* nl = NetworklayerTCP_new(UA_ConnectionConfig_standard, PORT);
	printf("Server started, connect to to opc.tcp://127.0.0.1:%i\n", PORT);

    /* run the server loop */
	struct timeval callback_interval = {1, 0}; // 1 second
	NetworkLayerTCP_run(nl, server, callback_interval, serverCallback, &running);
    
    /* clean up */
	NetworklayerTCP_delete(nl);
	UA_Server_delete(server);
    UA_String_deleteMembers(&endpointUrl);
	return 0;
}
```
