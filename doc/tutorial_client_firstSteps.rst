5. Building a simple client
===========================

You should already have a basic server from the previous tutorials. open62541
provides both a server- and clientside API, so creating a client is as easy as
creating a server. Copy the following into a file `myClient.c`:

.. code-block:: c

    #include "open62541.h"

    int main(void) {
      UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
      UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:16664");
      if(retval != UA_STATUSCODE_GOOD) {
          UA_Client_delete(client);
          return retval;
      }

      UA_Client_disconnect(client);
      UA_Client_delete(client);
      return 0;
    }

Compilation is very much similar to the server example.

.. code-block:: bash

   $ gcc -std=c99 open6251.c myClient.c -o myClient

Reading a node attibute
-----------------------

In this example we are going to connect to the server from the second tutorial
and read the value-attribute of the added variable node.

.. code-block:: c

    #include <stdio.h>
    #include "open62541.h"

    int main(int argc, char *argv[])
    {
        /* create a client and connect */
        UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:16664");
        if(retval != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
            return retval;
        }

        /* create a readrequest with one entry */
        UA_ReadRequest req;
        UA_ReadRequest_init(&req);
        req.nodesToRead = UA_Array_new(1, &UA_TYPES[UA_TYPES_READVALUEID]);
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

Further tasks
-------------
* Try to connect to some other OPC UA server by changing
  "opc.tcp://localhost:16664" to an appropriate address (remember that the
  queried node is contained in any OPC UA server).
* Try to set the value of the variable node (ns=1,i="the.answer") containing an
  "Int32" from the example server (which is built in
  :doc:`tutorial_server_firstSteps`) using "UA_Client_write" function. The
  example server needs some more modifications, i.e., changing request types.
  The answer can be found in "examples/exampleClient.c".
