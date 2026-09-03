/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * A complete OPC UA server plugged into the GLib-backed EventLoop
 * (UA_EventLoop_new_GLib). The server is started once (UA_Server_run_startup)
 * and from then on entirely driven by g_main_loop_run() -- there is no loop
 * calling UA_Server_run_iterate() / el->run(). GLib services the listening
 * socket, every client SecureChannel/Session, subscriptions and the
 * server's internal housekeeping timer via the single EventLoop GSource.
 *
 * Build (against a tree configured with -DUA_ENABLE_EVENTLOOP_GLIB=ON):
 *   cc eventloop_glib_server.c -o eventloop_glib_server \
 *      $(pkg-config --cflags glib-2.0) \
 *      -I<build>/src_generated -I../include -I../plugins/include \
 *      <build>/bin/libopen62541.a $(pkg-config --libs glib-2.0) -lpthread -lm
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>
#include <glib.h>
#include <signal.h>
#include <string.h>

static GMainLoop *loop;

static void
stopHandler(int sign) {
    (void)sign;
    g_main_loop_quit(loop);
}

/* Add one demo VariableNode so there is something to Browse/Read once
 * connected. */
static void
addAnswerVariable(UA_Server *server) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 value = 42;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US", "the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "the answer");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId nodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
    UA_Server_addVariableNode(server, nodeId, parentNodeId, parentReferenceNodeId,
                              browseName, variableType, attr, NULL, NULL);
}

int
main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    loop = g_main_loop_new(NULL, FALSE);

    UA_ServerConfig config;
    memset(&config, 0, sizeof(config));

    /* Plug in the GLib EventLoop *before* UA_ServerConfig_setMinimal runs.
     * setMinimal (like UA_ServerConfig_setDefault) only creates its own
     * POSIX EventLoop + ConnectionManagers if config.eventLoop is still
     * NULL -- otherwise it reuses exactly what is registered here, mirroring
     * what it would have set up itself. config.externalEventLoop stays
     * false (the memset default), so the EventLoop is still stopped/freed
     * automatically together with the server. */
    config.eventLoop = UA_EventLoop_new_GLib(UA_Log_Stdout, NULL);
    UA_ConnectionManager *tcpCM =
        UA_ConnectionManager_new_POSIX_TCP(UA_STRING("tcp connection manager"));
    config.eventLoop->registerEventSource(config.eventLoop, (UA_EventSource *)tcpCM);

    UA_StatusCode res = UA_ServerConfig_setMinimal(&config, 4840, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not initialize the server config");
        return EXIT_FAILURE;
    }

    UA_Server *server = UA_Server_newWithConfig(&config);
    addAnswerVariable(server);

    /* Starts the EventSources (the TCP listen socket) and attaches the
     * EventLoop's GSource to the (default) GMainContext -- the same context
     * g_main_loop_new(NULL, ...) above will iterate. */
    res = UA_Server_run_startup(server);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Could not start the server");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
               "Server listening on opc.tcp://localhost:4840 -- "
               "driven entirely by g_main_loop_run()");

    /* This single call now services the listening socket, every accepted
     * SecureChannel/Session, subscriptions and the server's internal
     * housekeeping timer. UA_Server_run_iterate() is never called. */
    g_main_loop_run(loop);

    /* Stops the EventSources and iterates the EventLoop (still via its GLib
     * run() implementation) until everything has shut down cleanly. */
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);

    g_main_loop_unref(loop);
    return EXIT_SUCCESS;
}
