Building a simple server
------------------------

This series of tutorial guide you through your first steps with open62541. For
compiling the examples, you need a compiler (MS Visual Studio 2015 or newer,
GCC, Clang and MinGW32 are all known to be working). The compilation
instructions are given for GCC but should be straightforward to adapt.

It will also be very helpful to install an OPC UA Client with a graphical
frontend, such as UAExpert by Unified Automation. That will enable you to
examine the information model of any OPC UA server.

To get started, downdload the open62541 single-file release from
http://open62541.org or generate it according to the :ref:`build instructions
<building>` with the "amalgamation" option enabled. From now on, we assume you
have the ``open62541.c/.h`` files in the current folder.

Now create a new C source-file called ``myServer.c`` with the following content:

.. literalinclude:: server_firstSteps.c
   :language: c
   :linenos:
   :lines: 4,12,14-

This is all that is needed for a simple OPC UA server. Compile the the server
with GCC using the following command:

.. code-block:: bash

   $ gcc -std=c99 open62541.c myServer.c -o myServer

Now start the server (and stop with ctrl-c):

.. code-block:: bash

   $ ./myServer

You have now compiled and run your first OPC UA server. You can go ahead and
browse the information model with UA Expert. The server will be listening on
``opc.tcp://localhost:16664`` - go ahead and give it a try.

In the following tutorials, you will be shown how to populate the server's
information model and how to create a client application.
