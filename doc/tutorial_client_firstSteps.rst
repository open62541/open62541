Building a simple client
------------------------

You should already have a basic server from the previous tutorials. open62541
provides both a server- and clientside API, so creating a client is as easy as
creating a server. Copy the following into a file `myClient.c`:

.. literalinclude:: client_firstSteps.c
   :language: c
   :linenos:
   :lines: 4,5,12,14-

Compilation is similar to the server example.

.. code-block:: bash

   $ gcc -std=c99 open6251.c myClient.c -o myClient

Further tasks
^^^^^^^^^^^^^
* Try to connect to some other OPC UA server by changing
  ``opc.tcp://localhost:16664`` to an appropriate address (remember that the
  queried node is contained in any OPC UA server).
* Try to set the value of the variable node (ns=1,i="the.answer") containing an
  ``Int32`` from the example server (which is built in
  :doc:`tutorial_server_firstSteps`) using "UA_Client_write" function. The
  example server needs some more modifications, i.e., changing request types.
  The answer can be found in "examples/exampleClient.c".
