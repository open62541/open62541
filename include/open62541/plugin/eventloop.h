/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_EVENTLOOP_H_
#define UA_EVENTLOOP_H_

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/plugin/network.h>

_UA_BEGIN_DECLS

struct UA_EventLoop;
typedef struct UA_EventLoop UA_EventLoop;

struct UA_EventSource;
typedef struct UA_EventSource UA_EventSource;

/**
 * Event Loop Subsystem
 * ====================
 *
 * An OPC UA-enabled application can have several clients and servers. And
 * server can serve different transport-level protocols for OPC UA. The
 * EventLoop is a central module that provides a unified control-flow for all of
 * these. Hence, several applications can share an EventLoop.
 *
 * The EventLoop and the ConnectionManager implementation is
 * architecture-specific. The goal is to have a single call to "select" (epoll,
 * kqueue, ...) in the EventLoop that covers all ConnectionManagers. Hence the
 * EventLoop plugin implementation must know implementation details of the
 * ConnectionManager implementations. So the EventLoop can extract socket
 * information, etc. from the ConnectionManagers.
 *
 * Event Loop
 * ----------
 * The EventLoop implementation is part of the selected architecture. For
 * example, "Win32/POSIX" stands for a Windows environment with an EventLoop
 * that uses the POSIX API. Several EventLoops can be instantiated in parallel.
 * But the globally defined functions are the same everywhere. */

typedef void (*UA_Callback)(void *application, void *context);

/* To be executed in the next EventLoop cycle */
typedef struct UA_DelayedCallback {
    struct UA_DelayedCallback *next; /* Singly-linked list */
    UA_Callback callback;
    void *application;
    void *data;
} UA_DelayedCallback;

typedef enum {
    UA_EVENTLOOPSTATE_FRESH = 0,
    UA_EVENTLOOPSTATE_STARTED,
    UA_EVENTLOOPSTATE_STOPPING, /* stopping in progress, needs EventLoop
                                 * cycles to finish */
    UA_EVENTLOOPSTATE_STOPPED
} UA_EventLoopState;

/**
 * EventLoop Lifecycle
 * ~~~~~~~~~~~~~~~~~~~ */

UA_EXPORT UA_EventLoop *
UA_EventLoop_new(const UA_Logger *logger);

/* Clean up the EventLoop and free allocated memory. Can fail if the EventLoop
 * is not stopped. */
UA_EXPORT UA_StatusCode
UA_EventLoop_delete(UA_EventLoop *el);

UA_EXPORT UA_EventLoopState
UA_EventLoop_getState(UA_EventLoop *el);

UA_EXPORT UA_StatusCode
UA_EventLoop_start(UA_EventLoop *el);

/* Stop all EventSources. This is asynchronous and might need a few
 * iterations of the main-loop to succeed. */
UA_EXPORT void
UA_EventLoop_stop(UA_EventLoop *el);

/* Process events for at most "timout" ms or until an unrecoverable error
 * occurs. If timeout==0, then only already received events are processed. */
UA_EXPORT UA_StatusCode
UA_EventLoop_run(UA_EventLoop *el, UA_UInt32 timeout);

/**
 * Cyclic and Delayed Callbacks
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Cyclic callbacks are executed regularly with an interval. A delayed callback
 * is executed in the next cycle of the EventLoop. The memory for the delayed
 * callback is freed after the execution. */

/* The execution interval is in ms. Returns the callbackId if the pointer is
 * non-NULL. */
UA_EXPORT UA_StatusCode
UA_EventLoop_addCyclicCallback(UA_EventLoop *el, UA_Callback cb,
                               void *application, void *data, UA_Double interval_ms,
                               UA_DateTime *baseTime, UA_TimerPolicy timerPolicy,
                               UA_UInt64 *callbackId);

UA_EXPORT UA_StatusCode
UA_EventLoop_modifyCyclicCallback(UA_EventLoop *el, UA_UInt64 callbackId,
                                  UA_Double interval_ms, UA_DateTime *baseTime,
                                  UA_TimerPolicy timerPolicy);

UA_EXPORT UA_StatusCode
UA_EventLoop_removeCyclicCallback(UA_EventLoop *el, UA_UInt64 callbackId);

UA_EXPORT void
UA_EventLoop_addDelayedCallback(UA_EventLoop *el, UA_DelayedCallback *dc);

/* Helper Functions */
UA_EXPORT const UA_Logger *
UA_EventLoop_getLogger(UA_EventLoop *el);

/**
 * Event Source
 * ------------ */

typedef enum {
    UA_EVENTSOURCESTATE_FRESH = 0,
    UA_EVENTSOURCESTATE_STOPPED,      /* Registered but stopped */
    UA_EVENTSOURCESTATE_STARTING,
    UA_EVENTSOURCESTATE_STARTED,
    UA_EVENTSOURCESTATE_STOPPING      /* Stopping in progress, needs
                                       * EventLoop cycles to finish */
} UA_EventSourceState;

struct UA_EventSource {
    struct UA_EventSource *next; /* Singly-linked list for use by the
                                  * application that registered the ES */

    /* Configuration
     * ~~~~~~~~~~~~~ */
    UA_String name;                 /* Unique name of the ES for logging */
    UA_ConfigParameter *parameters; /* Configuration parameters, depend on the
                                     * ES type */
    void *application;              /* Application to which the ES belongs */

    /* Lifecycle
     * ~~~~~~~~~ */
    UA_EventSourceState state;
    UA_EventLoop *eventLoop; /* EventLoop where the ES is registered */
    UA_StatusCode (*start)(UA_EventSource *es);
    void (*stop)(UA_EventSource *es); /* Asynchronous. Iterate theven EventLoop
                                       * until the EventSource is stopped. */
    UA_StatusCode (*free)(UA_EventSource *es);
};

/* Register the ES. Immediately starts the ES if the EventLoop is already
 * started. Otherwise the ES is started together with the EventLoop. */
UA_EXPORT UA_StatusCode
UA_EventLoop_registerEventSource(UA_EventLoop *el,
                                 UA_EventSource *es);

/* If still registered, call _stop (but not _clear) on the CM and deregister. */
UA_EXPORT UA_StatusCode
UA_EventLoop_deregisterEventSource(UA_EventLoop *el,
                                   UA_EventSource *es);

/**
 * Connection Manager
 * ------------------
 * Every Connection is created by a ConnectionManager. Every ConnectionManager
 * belongs to just one application. A ConnectionManager can act purely as a
 * passive "Factory" for Connections. But it can also be stateful. For example,
 * it can keep a session to an MQTT broker open which is used by individual
 * connections that are each bound to an MQTT topic. */

struct UA_ConnectionManager;
typedef struct UA_ConnectionManager UA_ConnectionManager;

/**
 * The ConnectionCallback is the only interface from the connection back to the
 * application. The connectionId is announced to the application when it is
 * first used for the callback. The context is a double-pointer so the context
 * can be overwritten by the application */
typedef void
(*UA_ConnectionCallback)(UA_ConnectionManager *cm, uintptr_t connectionId,
                         void **connectionContext, UA_StatusCode status,
                         UA_ByteString msg);

struct UA_ConnectionManager {
    /* Every ConnectionManager is treated like an EventSource from the
     * perspective of the EventLoop. */
    UA_EventSource eventSource;

    /* Passively listen for new connections
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * Some ConnectionManagers passively listen to open new Connections. The
     * configuration parameters stored in the EventSource are used during
     * "start" of the EventSource to set this up. The "connectionCallback"
     * callback is used to indicate that a new connection has been are created
     * (status==Good, msg=empty).
     *
     * The callback depends on the application and has to be manually
     * configured. */
    UA_ConnectionCallback connectionCallback;
    void *initialConnectionContext;

    /* Actively open connections
     * ~~~~~~~~~~~~~~~~~~~~~~~~~
     * Some ConnectionManagers can actively open a new Connection. This happens
     * asynchronously by a callback. The possible parameters depend on the
     * protocol and implementation.
     *
     * The connectString contains the necessary information to open up a
     * connection and has the schema "hostname:port".
     *
     * The connection is opened asynchronously. The ConnectionCallback is
     * triggered when the connection is fully opened (UA_STATUSCODE_GOOD) or has
     * failed (with an error code).
     *
     * The ConnectionCallback cb and context are set to the new connection. */
    UA_StatusCode
    (*openConnection)(UA_ConnectionManager *cm, const UA_String connectString,
                      void *context);

    /* Connection Activities
     * ~~~~~~~~~~~~~~~~~~~~~
     * The following are activities to be performed on an open connection.
     *
     * Each ConnectionManager allocates and frees his own memory for the network
     * buffers. This enables, for example, zero-copy neworking mechanisms. The
     * connectionId is part of the API to enable cases where memory is
     * statically allocated for every connection */
    UA_StatusCode
    (*allocNetworkBuffer)(UA_ConnectionManager *cm, uintptr_t connectionId,
                          UA_ByteString *buf, size_t bufSize);
    void
    (*freeNetworkBuffer)(UA_ConnectionManager *cm, uintptr_t connectionId,
                         UA_ByteString *buf);

    /* Send a message. Sending is asynchronous. That is, the function returns
     * before the message is ACKed from remote. The memory for the buffer is
     * expected to be allocated with allocNetworkBuffer and is released
     * internally (also if sending fails). */
    UA_StatusCode
    (*sendWithConnection)(UA_ConnectionManager *cm, uintptr_t connectionId,
                          UA_ByteString *buf);

    /* When a connection is closed, the connectionCallback is called with
     * (status=BadConnectionClosed, msg=empty). Then the connection is cleared
     * up inside the ConnectionManager. This is the case both for connections
     * that are actively closed and those that are closed remotely. The return
     * code is non-good only if the connection is already closed. */
    UA_StatusCode
    (*closeConnection)(UA_ConnectionManager *cm, uintptr_t connectionId);
};

/**
 * TCP Connection Manager
 * ~~~~~~~~~~~~~~~~~~~~~~
 * Listens on the network and manages TCP connections. The configuration
 * parameters have to set before calling _start to take effect.
 *
 * Configuration Parameters:
 *
 * - listen-port [uint16]: Port to listen for new connections (default: do not
 *                         listen on any port).
 * - listen-hostnames [array of strings]: Network devices to listen on (default:
 *                                        listens on all available devices).
 * - listen-bufsize [uint16]: Size of the buffer that is allocated for receiving
 *                            messages (default 16kB). */
UA_EXPORT UA_ConnectionManager *
UA_ConnectionManager_TCP_new(const UA_String eventSourceName);

_UA_END_DECLS

#endif /* UA_EVENTLOOP_H_ */
