/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Minimal example for UA_EventLoop_new_GLib: the open62541 EventLoop is
 * attached to a GMainContext once (el->start) and from then on is driven
 * *exclusively* by g_main_loop_run() -- el->run() is never called. A TCP
 * ConnectionManager listens on a port and echoes back whatever it receives;
 * a repeating GLib-independent open62541 timer prints a heartbeat to show
 * that timers are serviced the same way as sockets. Ctrl-C (or the
 * heartbeat count below) stops the GMainLoop, after which the EventLoop is
 * cleanly stopped and freed.
 *
 * Build (against a tree configured with -DUA_ENABLE_EVENTLOOP_GLIB=ON):
 *   cc eventloop_glib_minimal.c -o eventloop_glib_minimal \
 *      $(pkg-config --cflags glib-2.0) \
 *      -I<build>/src_generated -I../include -I../plugins/include \
 *      <build>/bin/libopen62541.a $(pkg-config --libs glib-2.0) -lpthread -lm
 */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>
#include <glib.h>
#include <signal.h>
#include <string.h>

static GMainLoop *loop;
static UA_EventLoop *el;
static UA_ConnectionManager *tcpCM;
static unsigned heartbeats = 0;

/* Echo whatever is received back to the sender. Runs whenever GLib
 * dispatches the EventLoop's GSource -- i.e. whenever g_main_loop_run()
 * notices activity on the listening/accepted sockets. */
static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                    void *application, void **connectionContext,
                    UA_ConnectionState state, const UA_KeyValueMap *params,
                    UA_ByteString msg) {
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Connection %lu closed", (unsigned long)connectionId);
        return;
    }

    /* The state stays ESTABLISHED both for the initial "connection opened"
     * notification (msg.length == 0) and for every subsequent data event. */
    if(msg.length == 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Connection %lu established", (unsigned long)connectionId);
        return;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Connection %lu: echoing %u bytes",
                (unsigned long)connectionId, (unsigned)msg.length);

    UA_ByteString out;
    UA_StatusCode res = cm->allocNetworkBuffer(cm, connectionId, &out, msg.length);
    if(res != UA_STATUSCODE_GOOD)
        return;
    memcpy(out.data, msg.data, msg.length);
    cm->sendWithConnection(cm, connectionId, &UA_KEYVALUEMAP_NULL, &out);
}

/* An open62541 timer -- not a GLib timeout -- to show that the EventLoop's
 * own timer subsystem is serviced by the GMainContext iteration as well. */
static void
heartbeatCallback(void *application, void *data) {
    heartbeats++;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Heartbeat %u (serviced by g_main_loop_run)", heartbeats);
    if(heartbeats >= 5)
        g_main_loop_quit(loop);
}

static void
stopHandler(int sign) {
    (void)sign;
    g_main_loop_quit(loop);
}

int
main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    loop = g_main_loop_new(NULL, FALSE);

    /* NULL -> attach to the process-wide default GMainContext, the same one
     * g_main_loop_new(NULL, ...) iterates below. */
    el = UA_EventLoop_new_GLib(UA_Log_Stdout, NULL);

    tcpCM = UA_ConnectionManager_new_POSIX_TCP(UA_STRING("tcp"));
    el->registerEventSource(el, &tcpCM->eventSource);

    /* Starting the EventLoop attaches its GSource to the GMainContext. From
     * here on el->run() is never called again -- g_main_loop_run() alone
     * services sockets and timers. */
    el->start(el);

    el->addTimer(el, heartbeatCallback, NULL, NULL, 500.0, NULL,
                 UA_TIMERPOLICY_CURRENTTIME, NULL);

    UA_UInt16 port = 4841;
    UA_Boolean listen = true;
    UA_KeyValuePair params[2];
    params[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    params[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&params[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap paramsMap = {2, params};
    tcpCM->openConnection(tcpCM, &paramsMap, NULL, NULL, connectionCallback);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Listening on port %u -- entirely driven by g_main_loop_run()", port);

    /* This single call drives the whole open62541 stack: accepting
     * connections, reading/writing sockets and firing timers. */
    g_main_loop_run(loop);

    /* Shut down cleanly. Stopping is asynchronous -- keep iterating the
     * context (still via GLib) until the last EventSource has confirmed. */
    el->stop(el);
    while(el->state != UA_EVENTLOOPSTATE_STOPPED)
        g_main_context_iteration(NULL, TRUE);
    el->free(el);

    g_main_loop_unref(loop);
    return 0;
}
