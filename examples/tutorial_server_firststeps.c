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
 * To get started, download the open62541 single-file release from
 * http://open62541.org or generate it according to the :ref:`build instructions
 * <building>` with the "amalgamation" option enabled. From now on, we assume
 * you have the ``open62541.c/.h`` files in the current folder. Now create a new
 * C source-file called ``myServer.c`` with the following content: */

#include <open62541/server.h>

int main(void) {
    UA_Server *server = UA_Server_new();
    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return 0;
}

/**
 * This is all that is needed for a simple OPC UA server. With the GCC compiler,
 * the following command produces an executable:
 *
 * .. code-block:: bash
 *
 *    $ gcc -std=c99 myServer.c -lopen62541 -o myServer
 *
 * In a MinGW environment, the Winsock library must be added.
 *
 * .. code-block:: bash
 *
 *    $ gcc -std=c99 myServer.c -lopen62541 -lws2_32 -o myServer.exe
 *
 * Now start the server (stop with ctrl-c):
 *
 * .. code-block:: bash
 *
 *    $ ./myServer
 *
 * You have now compiled and run your first OPC UA server. You can go ahead and
 * browse the information model with client. The server is listening on
 * ``opc.tcp://localhost:4840``.
 *
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
 * The server is started with ``UA_Server_run``. Internally, the server
 * schedules regular tasks. Between the timeouts, the server listens on the
 * network layer for incoming messages.
 *
 * In order to integrate OPC UA in a single-threaded application with its own
 * mainloop (for example provided by a GUI toolkit), one can alternatively drive
 * the server manually. See the section of the server documentation on
 * :ref:`server-lifecycle` for details.
 */
