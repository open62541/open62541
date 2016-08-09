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

.. literalinclude:: ../../examples/client_firstSteps.c
   :language: c
   :linenos:
   :lines: 4,5,12,14-


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
