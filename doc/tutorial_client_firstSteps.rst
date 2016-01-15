First steps with open62541-client
=================================

In the previous :doc:`tutorial_server_firstSteps` tutorial, you should have gotten your build environment verified and created a first OPC UA server using the open62541 stack. The created server however doesn't do much yet and there is no client to interact with the server. We are going to remedy that in this tutorial by creating some nodes and variables.

You should already have a basic server from the previous tutorial. open62541 provides both a server- and clientside API, so creating a client is equally as easy as creating a server. We are going to use dynamic linking (libopen62541.so) from now on, because our client and server will share a lot of code. Reusing the shared library will considerably reduce the overhead. To avoid confusion, remove the amalgated open62541.c/h files from your example directory.

As a recap, your directory structure should now look like this::
 
  :myApp> rm *.o open62541.*
  :myApp> ln -s ../open62541/build/*so ./
  :myApp> tree
  .
  +── include
  │   +── logger_stdout.h
  │   +── networklayer_tcp.h
  │   +── networklayer_udp.h
  │   +── open62541.h
  │   +── ua_client.h
  │   +── ua_config.h
  │   +── ua_config.h.in
  │   +── ua_connection.h
  │   +── ua_log.h
  │   +── ua_nodeids.h
  │   +── ua_server.h
  │   +── ua_statuscodes.h
  │   +── ua_transport_generated.h
  │   +── ua_types_generated.h
  │   +── ua_types.h
  +── libopen62541.so -> ../../open62541/build/libopen62541.so
  +── myServer
  +── myServer.c

Note that I have linked the library into the folder to spare me the trouble of copying it every time I change/rebuild the stack.

To create a really basic client, navigate back into the myApp folder from the previous tutorial and create a client:

.. code-block:: c

    #include <stdio.h>

    #include "ua_types.h"
    #include "ua_server.h"
    #include "logger_stdout.h"
    #include "networklayer_tcp.h"

    int main(void) {
      UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout_new());
      UA_StatusCode retval = UA_Client_connect(client, ClientNetworkLayerTCP_connect, "opc.tcp://localhost:16664");
      if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
      }
      
      UA_Client_disconnect(client);
      UA_Client_delete(client);
      return 0;
    } 

Let's recompile both server and client - if you feel up to it, you can create a Makefile for this procedure. I will show a final command line compile example and ommit the compilation directives in future examples.::

    :myApp> gcc -Wl,-rpath=./ -L./ -I ./include -o myClient myClient.c  -lopen62541


Asserting success/failure
-------------------------

Almost all functions of the open62541 API will return a ``UA_StatusCode``, which in the C world would be represented by a ``unsigned int``. OPC UA defines large number of good and bad return codes represented by this number. The constant UA_STATUSCODE_GOOD is defined as 0 in ``include/ua_statuscodes.h`` along with many other return codes. It pays off to check the return code of your function calls, as we already did implicitly in the client.

Minimalistic introduction to OPC UA nodes and node IDs
------------------------------------------------------

OPC UA nodespace model defines 9 standard attribute for every node:

+---------------+----------------+
| Type          | Name           |
+===============+================+
| NodeId        | nodeID         |
+---------------+----------------+
| NodeClass     | nodeClass      |
+---------------+----------------+
| QualifiedName | browseName     |
+---------------+----------------+
| LocalizedText | displayName    |
+---------------+----------------+
| LocalizedText | description    |
+---------------+----------------+
| UInt32        | writeMask      |
+---------------+----------------+
| UInt32        | userWriteMask  |
+---------------+----------------+
| Int32         | referencesSize |
+---------------+----------------+
|ReferenceNode[]| references     |
+---------------+----------------+

Furthermore, there are different node types that are stored in NodeClass. 
For different classes, nodes have additional properties.

In this tutorial we are interested in one of these types: "Variable". In this case a node will have an additional attribute called "value" which we are going to read.

Let us go on with node IDs. A node ID is a unique identifier in server's context. It is composed of two members:

+-------------+-----------------+---------------------------+
| Type        | Name            | Notes                     |
+=============+=================+===========================+
| UInt16      | namespaceIndex  |  Number of the namespace  |
+-------------+-----------------+---------------------------+
| Union       | identifier      |  One idenifier of the     |
|             |  * String       |  listed types             |
|             |  * Integer      |                           |
|             |  * GUID         |                           |
|             |  * ByteString   |                           |
+-------------+-----------------+---------------------------+

The first parameter is the number of node's namespace, the second one may be a numeric, a string or a GUID (Globally Unique ID) identifier. 

Reading variable's node value
-----------------------------

In this example we are going to read node (n=0,i=2258), i.e. a node in namespace 0 with a numerical id 2258. This node is present in every server (since it is located in namespace 0) and contains server current time (encoded as UInt64).

Let us extend the client with with an action reading node's value:

.. code-block:: c

    #include <stdio.h>

    #include "ua_types.h"
    #include "ua_server.h"
    #include "logger_stdout.h"
    #include "networklayer_tcp.h"

    int main(void) {
      UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout_new());
      UA_StatusCode retval = UA_Client_connect(client, ClientNetworkLayerTCP_connect, "opc.tcp://localhost:16664");
      if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
      }
      
      //variable to store data
      UA_DateTime raw_date = 0;

      UA_ReadRequest rReq;
      UA_ReadRequest_init(&rReq);
      rReq.nodesToRead = UA_Array_new(&UA_TYPES[UA_TYPES_READVALUEID], 1);
      rReq.nodesToReadSize = 1;
      rReq.nodesToRead[0].nodeId = UA_NODEID_NUMERIC(0, 2258);
      rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

      UA_ReadResponse rResp = UA_Client_read(client, &rReq);
      if(rResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD &&
         rResp.resultsSize > 0 && rResp.results[0].hasValue &&
         UA_Variant_isScalar(&rResp.results[0].value) &&
         rResp.results[0].value.type == &UA_TYPES[UA_TYPES_DATETIME]) {
             raw_date = *(UA_DateTime*)rResp.results[0].value.data;
             printf("raw date is: %" PRId64 "\n", raw_date);
      }
      
      UA_ReadRequest_deleteMembers(&rReq);
      UA_ReadResponse_deleteMembers(&rResp);

      UA_Client_disconnect(client);
      UA_Client_delete(client);
      return 0;
    } 

You should see raw time in milliseconds since January 1, 1601 UTC midnight::

    :myApp> ./myClient
    :myApp> raw date is: 130856974061125520

Firstly we constructed a read request "rReq", it contains 1 node's attribute we want to query for. The attribute is filled with the numeric id "UA_NODEID_NUMERIC(0, 2258)" and the attribute we are reading "UA_ATTRIBUTEID_VALUE". After the read request was sent, we can find the actual read value in the read response.

As the last step for this tutorial, we are going to convert the raw date value into a well formatted string:

.. code-block:: c

    #include <stdio.h>
    
    #include "ua_types.h"
    #include "ua_server.h"
    #include "logger_stdout.h"
    #include "networklayer_tcp.h"
    
    int main(void) {
      UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout_new());
      UA_StatusCode retval = UA_Client_connect(client, ClientNetworkLayerTCP_connect, "opc.tcp://localhost:16664");
      if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
      }
    
      //variables to store data
      UA_DateTime raw_date = 0;
      UA_String* string_date = UA_String_new();

      UA_ReadRequest rReq;
      UA_ReadRequest_init(&rReq);
      rReq.nodesToRead = UA_Array_new(&UA_TYPES[UA_TYPES_READVALUEID], 1);
      rReq.nodesToReadSize = 1;
      rReq.nodesToRead[0].nodeId = UA_NODEID_NUMERIC(0, 2258);
      rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

      UA_ReadResponse rResp = UA_Client_read(client, &rReq);
      if(rResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD &&
         rResp.resultsSize > 0 && rResp.results[0].hasValue &&
         UA_Variant_isScalar(&rResp.results[0].value) &&
         rResp.results[0].value.type == &UA_TYPES[UA_TYPES_DATETIME]) {
             raw_date = *(UA_DateTime*)rResp.results[0].value.data;
             printf("raw date is: %llu\n", raw_date);
             UA_DateTime_toString(raw_date, string_date);
             printf("string date is: %.*s\n", string_date->length, string_date->data);
      }
      
      UA_ReadRequest_deleteMembers(&rReq);
      UA_ReadResponse_deleteMembers(&rResp);
      UA_String_delete(string_date);

      UA_Client_disconnect(client);
      UA_Client_delete(client);
      return 0;
    }

Note that this file can be found as "examples/client_firstSteps.c" in the repository.
    
Now you should see raw time and a formatted date::

    :myApp> ./myClient
    :myApp> raw date is: 130856981449041870
            string date is: 09/02/2015 20:09:04.904.187.000

Further tasks
-------------
* Try to connect to some other OPC UA server by changing "opc.tcp://localhost:16664" to an appropriate address (remember that the queried node is contained in any OPC UA server).
* Display the value of the variable node (ns=1,i="the.answer") containing an "Int32" from the example server (which is built in :doc:`tutorial_server_firstSteps`). Note that the identifier of this node is a string type: use "UA_NODEID_STRING_ALLOC". The answer can be found in "examples/exampleClient.c".
* Try to set the value of the variable node (ns=1,i="the.answer") containing an "Int32" from the example server (which is built in :doc:`tutorial_server_firstSteps`) using "UA_Client_write" function. The example server needs some more modifications, i.e., changing request types. The answer can be found in "examples/exampleClient.c".
