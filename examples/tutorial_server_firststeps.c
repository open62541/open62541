/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Building a Simple Server
 * ------------------------
 *
 * This series of tutorial guide you through your first steps with open62541.
 * For compiling the examples, you need a compiler (MS Visual Studio 2015 or
 * newer, GCC, Clang and MinGW32 are all known to be working). The compilation
 * instructions are given for GCC but should be straightforward to adapt.
 *
 * It will also be very helpful to install an OPC UA Client with a graphical
 * frontend, such as UAExpert by Unified Automation. That will enable you to
 * examine the information model of any OPC UA server.
 *
 * To get started, downdload the open62541 single-file release from
 * http://open62541.org or generate it according to the :ref:`build instructions
 * <building>` with the "amalgamation" option enabled. From now on, we assume
 * you have the ``open62541.c/.h`` files in the current folder. Now create a new
 * C source-file called ``myServer.c`` with the following content: */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

static volatile UA_Boolean running = true;
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

/**
 * This is all that is needed for a simple OPC UA server. With the GCC compiler,
 * the following command produces an executable:
 *
 * .. code-block:: bash
 *
 *    $ gcc -std=c99 open62541.c myServer.c -o myServer
 *
 * In a MinGW environment, the Winsock library must be added.
 *
 * .. code-block:: bash
 *
 *    $ gcc -std=c99 open62541.c myServer.c -lws2_32 -o myServer.exe
 *
 * Now start the server (stop with ctrl-c):
 *
 * .. code-block:: bash
 *
 *    $ ./myServer
 *
 * You have now compiled and run your first OPC UA server. You can go ahead and
 * browse the information model with client. The server is listening on
 * ``opc.tcp://localhost:4840``. In the next two sections, we will continue to
 * explain the different parts of the code in detail.
 *
 * Server Configuration and Plugins
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * *open62541* provides a flexible framework for building OPC UA servers and
 * clients. The goals is to have a core library that accommodates for all use
 * cases and runs on all platforms. Users can then adjust the library to fit
 * their use case via configuration and by developing (platform-specific)
 * plugins. The core library is based on C99 only and does not even require
 * basic POSIX support. For example, the lowlevel networking code is implemented
 * as an exchangeable plugin. But don't worry. *open62541* provides plugin
 * implementations for most platforms and sensible default configurations
 * out-of-the-box.
 * 
 * In the above server code, we simply take the default server configuration and
 * add a single TCP network layer that is listerning on port 4840.
 *
 * Server Lifecycle
 * ^^^^^^^^^^^^^^^^
 *
 * The code in this example shows the three parts for server lifecycle
 * management: Creating a server, running the server, and deleting the server.
 * Creating and deleting a server is trivial once the configuration is set up.
 * The server is started with ``UA_Server_run``. Internally, the server then
 * uses timeouts to schedule regular tasks. Between the timeouts, the server
 * listens on the network layer for incoming messages.
 *
 * You might ask how the server knows when to stop running. For this, we have
 * created a global variable ``running``. Furthermore, we have registered the
 * method ``stopHandler`` that catches the signal (interrupt) the program
 * receives when the operating systems tries to close it. This happens for
 * example when you press ctrl-c in a terminal program. The signal handler then
 * sets the variable ``running`` to false and the server shuts down once it
 * takes back control.
 *
 * In order to integrated OPC UA in a single-threaded application with its own
 * mainloop (for example provided by a GUI toolkit), one can alternatively drive
 * the server manually. See the section of the server documentation on
 * :ref:`server-lifecycle` for details.
 *
 * The server configuration and lifecycle management is needed for all servers.
 * We will use it in the following tutorials without further comment.
 */
