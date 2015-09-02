First steps with open62541-client
===================================

In the previous :doc:`tutorial_firstStepsServer` tutorial, you should have gotten your build environment verified and created a first OPC UA server using the open62541 stack. The created server however doesn't do much yet and there is no client to interact with the server. We are going to remedy that in this tutorial by creating some nodes and variables.

----------------------

You should already have a basic server from the previous tutorial. open62541 provides both a server- and clientside API, so creating a client is equally as easy as creating a server. We are going to use dynamic linking (libopen62541.so) from now on, because our client and server will share a lot of code. Reusing the shared library will considerably reduce the overhead. To avoid confusion, remove the amalgated open62541.c/h files from your example directory.

As a recap, your directory structure should now look like this::
 
  :myServerApp> rm *.o open62541.*
  :myServerApp> ln -s ../open62541/build/*so ./
  :myServerApp> tree
  .
  ├── include
  │   ├── logger_stdout.h
  │   ├── networklayer_tcp.h
  │   ├── networklayer_udp.h
  │   ├── open62541.h
  │   ├── ua_client.h
  │   ├── ua_config.h
  │   ├── ua_config.h.in
  │   ├── ua_connection.h
  │   ├── ua_log.h
  │   ├── ua_nodeids.h
  │   ├── ua_server.h
  │   ├── ua_statuscodes.h
  │   ├── ua_transport_generated.h
  │   ├── ua_types_generated.h
  │   └── ua_types.h
  ├── libopen62541.so -> ../../open62541/build/libopen62541.so
  ├── myServer
  └── myServer.c

Note that I have linked the library into the folder to spare me the trouble of copying it every time I change/rebuild the stack.

To create a really basic client, navigate back into the MyServerApp folder from the previous tutorial and create a client::

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

    :myServerApp> gcc -Wl,-rpath=./ -L./ -I ./include -o myClient myClient.c  -lopen62541

We will also make a slight change to our server: We want it to exit cleanly when pressing ``CTRL+C``. We will add signal handler for SIGINT and SIGTERM to accomplish that to the server::

    #include <stdio.h>
    #include <signal.h>

    # include "ua_types.h"
    # include "ua_server.h"
    # include "logger_stdout.h"
    # include "networklayer_tcp.h"

    UA_Boolean running;
    UA_Logger logger;

    void stopHandler(int signal) {
      running = 0;
    }

    int main(void) {
      signal(SIGINT,  stopHandler);
      signal(SIGTERM, stopHandler);
      
      UA_Server *server = UA_Server_new(UA_ServerConfig_standard);
      logger = Logger_Stdout_new();
      UA_Server_setLogger(server, logger);
      UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));
      running = UA_TRUE;
      UA_Server_run(server, 1, &running);
      UA_Server_delete(server);
      
      printf("Terminated\n");
      return 0;
    }
And then of course, recompile it::

    :myServerApp> gcc -Wl,-rpath=./ -L./ -I ./include -o myServer myServer.c  -lopen62541

You can now start and background the server, run the client, and then terminate the server like so::

    :myServerApp> ./myServer &
    [xx/yy/zz aa:bb:cc.dd.ee] info/communication	Listening on opc.tcp://localhost:16664
    [1] 2114
    :myServerApp> ./myClient && killall myServer
    Terminated
    [1]+  Done                    ./myServer
    :myServerApp> 

Notice how the server received the SIGTERM signal from kill and exited cleany? We also used the return value of our client by inserting the ``&&``, so kill is only called after a clean client exit (``return 0``).

Asserting success/failure
-------------------------

Almost all functions of the open62541 API will return a ``UA_StatusCode``, which in the C world would be represented by a ``unsigned int``. OPC UA defines large number of good and bad return codes represented by this number. The constant UA_STATUSCODE_GOOD is defined as 0 in ``include/ua_statuscodes.h`` along with many other return codes. It pays off to check the return code of your function calls, as we already did implicitly in the client.



