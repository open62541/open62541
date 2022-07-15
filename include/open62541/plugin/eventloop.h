/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#ifndef UA_EVENTLOOP_H_
#define UA_EVENTLOOP_H_

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/util.h>
#include <open62541/plugin/log.h>
#include <open62541/plugin/network.h>

_UA_BEGIN_DECLS

struct UA_EventLoop;
typedef struct UA_EventLoop UA_EventLoop;

struct UA_EventSource;
typedef struct UA_EventSource UA_EventSource;

struct UA_ConnectionManager;
typedef struct UA_ConnectionManager UA_ConnectionManager;

struct UA_InterruptManager;
typedef struct UA_InterruptManager UA_InterruptManager;

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
 * architecture-specific. The goal is to have a single call to "poll" (epoll,
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
    void *context;
} UA_DelayedCallback;

typedef enum {
    UA_EVENTLOOPSTATE_FRESH = 0,
    UA_EVENTLOOPSTATE_STOPPED,
    UA_EVENTLOOPSTATE_STARTED,
    UA_EVENTLOOPSTATE_STOPPING /* Stopping in progress, needs EventLoop
                                * cycles to finish */
} UA_EventLoopState;

struct UA_EventLoop {
    /* Configuration
     * ~~~~~~~~~~~~~~~
     * The configuration should be set before the EventLoop is started */
    const UA_Logger *logger;
    size_t paramsSize;
    UA_KeyValuePair *params; /* See the implementation-specific documentation */

    /* EventLoop Lifecycle
     * ~~~~~~~~~~~~~~~~~~~~ */
    const volatile UA_EventLoopState state; /* Only read the state from outside */

    /* Start the EventLoop and start all already registered EventSources */
    UA_StatusCode (*start)(UA_EventLoop *el);

    /* Stop all EventSources. This is asynchronous and might need a few
     * iterations of the main-loop to succeed. */
    void (*stop)(UA_EventLoop *el);

    /* Process events for at most "timeout" ms or until an unrecoverable error
     * occurs. If timeout==0, then only already received events are
     * processed. */
    UA_StatusCode (*run)(UA_EventLoop *el, UA_UInt32 timeout);

    /* Clean up the EventLoop and free allocated memory. Can fail if the
     * EventLoop is not stopped. */
    UA_StatusCode (*free)(UA_EventLoop *el);

    /* EventLoop Time Domain
     * ~~~~~~~~~~~~~~~~~~~~~
     * Each EventLoop instance can manage its own time domain. This affects the
     * execution of timed/cyclic callbacks and time-based sending of network
     * packets (if this is implemented). Managing independent time domains is
     * important when different parts of a system a synchronized to different
     * external (network-wide) clocks.
     *
     * Note that the logger configured in the EventLoop generates timestamps
     * internally as well. If the logger uses a different time domain than the
     * EventLoop, discrepancies may appear in the logs.
     *
     * The time domain of the EventLoop is exposed via the following functons.
     * See `open62541/types.h` for the documentation of their equivalent
     * globally defined functions. */
    UA_DateTime (*dateTime_now)(UA_EventLoop *el);
    UA_DateTime (*dateTime_nowMonotonic)(UA_EventLoop *el);
    UA_Int64    (*dateTime_localTimeUtcOffset)(UA_EventLoop *el);

    /* Cyclic and Delayed Callbacks
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * Cyclic callbacks are executed regularly with an interval. A delayed
     * callback is executed in the next cycle of the EventLoop. The memory for
     * the delayed callback is freed after the execution. */

    /* Time of the next cyclic callback. Returns the max DateTime if no cyclic
     * callback is registered. */
    UA_DateTime (*nextCyclicTime)(UA_EventLoop *el);

    /* The execution interval is in ms. Returns the callbackId if the pointer is
     * non-NULL. */
    UA_StatusCode
    (*addCyclicCallback)(UA_EventLoop *el, UA_Callback cb, void *application,
                         void *data, UA_Double interval_ms, UA_DateTime *baseTime,
                         UA_TimerPolicy timerPolicy, UA_UInt64 *callbackId);

    UA_StatusCode
    (*modifyCyclicCallback)(UA_EventLoop *el, UA_UInt64 callbackId,
                            UA_Double interval_ms, UA_DateTime *baseTime,
                            UA_TimerPolicy timerPolicy);

    void (*removeCyclicCallback)(UA_EventLoop *el, UA_UInt64 callbackId);

    /* Like a cyclic callback, but executed only once */
    UA_StatusCode
    (*addTimedCallback)(UA_EventLoop *el, UA_Callback cb, void *application,
                        void *data, UA_DateTime date, UA_UInt64 *callbackId);

    void (*addDelayedCallback)(UA_EventLoop *el, UA_DelayedCallback *dc);

    /* EventSources
     * ~~~~~~~~~~~~
     * EventSources are stored in a singly-linked list for direct access. But
     * only the below methods shall be used for adding and removing - this
     * impacts the lifecycle of the EventSource. For example it may be
     * auto-started if the EventLoop is already running. */

    /* Linked list of EventSources */
    UA_EventSource *eventSources;

    /* Register the ES. Immediately starts the ES if the EventLoop is already
     * started. Otherwise the ES is started together with the EventLoop. */
    UA_StatusCode
    (*registerEventSource)(UA_EventLoop *el, UA_EventSource *es);

    /* Stops the EventSource before deregistrering it */
    UA_StatusCode
    (*deregisterEventSource)(UA_EventLoop *el, UA_EventSource *es);
};

/**
 * Event Source
 * ------------
 * Event Sources are attached to an EventLoop. Typically the event source and
 * the EventLoop are developed together and share a private API in the
 * background. */

typedef enum {
    UA_EVENTSOURCESTATE_FRESH = 0,
    UA_EVENTSOURCESTATE_STOPPED,      /* Registered but stopped */
    UA_EVENTSOURCESTATE_STARTING,
    UA_EVENTSOURCESTATE_STARTED,
    UA_EVENTSOURCESTATE_STOPPING      /* Stopping in progress, needs
                                       * EventLoop cycles to finish */
} UA_EventSourceState;

/* Type-tag for proper casting of the difference EventSource (e.g. when they are
 * looked up via UA_EventLoop_findEventSource). */
typedef enum {
    UA_EVENTSOURCETYPE_ANY = 0,
    UA_EVENTSOURCETYPE_CONNECTIONMANAGER,
    UA_EVENTSOURCETYPE_INTERRUPTMANAGER
} UA_EventSourceType;

struct UA_EventSource {
    struct UA_EventSource *next; /* Singly-linked list for use by the
                                  * application that registered the ES */

    UA_EventSourceType eventSourceType;

    /* Configuration
     * ~~~~~~~~~~~~~ */
    UA_String name;                 /* Unique name of the ES */
    UA_EventLoop *eventLoop;        /* EventLoop where the ES is registered */
    size_t paramsSize;              /* Configuration parameters */
    UA_KeyValuePair *params;

    /* Lifecycle
     * ~~~~~~~~~ */
    UA_EventSourceState state;
    UA_StatusCode (*start)(UA_EventSource *es);
    void (*stop)(UA_EventSource *es); /* Asynchronous. Iterate theven EventLoop
                                       * until the EventSource is stopped. */
    UA_StatusCode (*free)(UA_EventSource *es);
};

/**
 * Connection Manager
 * ------------------
 * Every Connection is created by a ConnectionManager. Every ConnectionManager
 * belongs to just one application. A ConnectionManager can act purely as a
 * passive "Factory" for Connections. But it can also be stateful. For example,
 * it can keep a session to an MQTT broker open which is used by individual
 * connections that are each bound to an MQTT topic. */

/* The ConnectionCallback is the only interface from the connection back to
 * the application.
 *
 * - The connectionId is initially unknown to the target application and
 *   "announced" to the application when first used first in this callback.
 *
 * - The context is attached to the connection. Initially a default context
 *   is set. The context can be replaced within the callback (via the
 *   double-pointer).
 *
 * - The state argument indicates the lifecycle of the connection. Every
 *   connection calls the callback a last time with UA_CONNECTIONSTATE_CLOSING.
 *   Protocols individually can forward diagnostic information relevant to the
 *   state as part of the key-value parameters.
 *
 * - The parameters are a key-value list with additional information. The
 *   possible keys and their meaning are documented for the individual
 *   ConnectionManager implementations.
 *
 * - The msg ByteString is the message (or packet) received on the
 *   connection. Can be empty. */
typedef void
(*UA_ConnectionManager_connectionCallback)
     (UA_ConnectionManager *cm, uintptr_t connectionId,
      void *application, void **connectionContext, UA_ConnectionState state,
      size_t paramsSize, const UA_KeyValuePair *params, UA_ByteString msg);

struct UA_ConnectionManager {
    /* Every ConnectionManager is treated like an EventSource from the
     * perspective of the EventLoop. */
    UA_EventSource eventSource;

    /* Name of the protocol supported by the ConnectionManager. For example
     * "mqtt", "udp", "mqtt". */
    UA_String protocol;

    /* Open a Connection
     * ~~~~~~~~~~~~~~~~~
     * Connecting is asynchronous. The connection-callback is called when the
     * connection is open (status=GOOD) or aborted (status!=GOOD) when
     * connecting failed.
     *
     * Some ConnectionManagers can also passively listen for new connections.
     * Configuration parameters for this are passed via the key-value list. The
     * `context` pointer of the listening connection is also set as the initial
     * context of newly opened connections.
     *
     * The parameters describe the connection. For example hostname and port
     * (for TCP). Other protocols (e.g. MQTT, AMQP, etc.) may required
     * additional arguments to open a connection in the key-value list.
     *
     * The provided context is set as the initial context attached to this
     * connection. It is already set before the first call to
     * connectionCallback.
     *
     * The connection can be opened synchronously or asynchronously.
     *
     * - For synchronous connection, the connectionCallback is called with the
     *   status UA_CONNECTIONSTATE_ESTABLISHED immediately from within the
     *   openConnection operation.
     *
     * - In the asynchronous case the connectionCallback is called immediately
     *   from within the openConnection operation with the status
     *   UA_CONNECTIONSTATE_OPENING. The connectionCallback is called with the
     *   status UA_CONNECTIONSTATE_ESTABLISHED once the connection has fully
     *   opened.
     *
     * Note that a single call to openConnection might open multiple
     * connections. For example listening on IPv4 and IPv6 for a single
     * hostname. Each protocol implementation documents whether multiple
     * connections might be opened at once. */
    UA_StatusCode
    (*openConnection)(UA_ConnectionManager *cm,
                      size_t paramsSize, const UA_KeyValuePair *params,
                      void *application, void *context,
                      UA_ConnectionManager_connectionCallback connectionCallback);

    /* Send a message over a Connection
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * Sending is asynchronous. That is, the function returns before the message
     * is ACKed from remote. The memory for the buffer is expected to be
     * allocated with allocNetworkBuffer and is released internally (also if
     * sending fails).
     *
     * Some ConnectionManagers can accept additional parameters for sending. For
     * example a tx-time for sending in time-synchronized TSN settings. */
    UA_StatusCode
    (*sendWithConnection)(UA_ConnectionManager *cm, uintptr_t connectionId,
                          size_t paramsSize, const UA_KeyValuePair *params,
                          UA_ByteString *buf);

    /* Close a Connection
     * ~~~~~~~~~~~~~~~~~~
     * When a connection is closed its `connectionCallback` is called with
     * (status=BadConnectionClosed, msg=empty). Then the connection is cleared
     * up inside the ConnectionManager. This is the case both for connections
     * that are actively closed and those that are closed remotely. The return
     * code is non-good only if the connection is already closed. */
    UA_StatusCode
    (*closeConnection)(UA_ConnectionManager *cm, uintptr_t connectionId);

    /* Buffer Management
     * ~~~~~~~~~~~~~~~~~
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
};

/**
 * Interrupt Manager
 * -----------------
 * The Interrupt Manager allows to register to listen for system interrupts.
 * Triggering the interrupt calls the callback associated with it.
 *
 * The implementations of the interrupt manager for the different platforms
 * shall be designed such that:
 *
 * Registered interrupts are only intercepted from within the running EventLoop
 *
 * Processing an interrupt in the EventLoop is handled similarly to handling a
 * network event: all methods and also memory allocation are available from
 * within the interrupt callback. */

/* Interrupts can have additional key-value 'instanceInfos' for each individual
 * triggering. See the architecture-specific documentation. */
typedef void
(*UA_InterruptCallback)(UA_InterruptManager *im,
                        uintptr_t interruptHandle, void *interruptContext,
                        size_t instanceInfosSize,
                        const UA_KeyValuePair *instanceInfos);

struct UA_InterruptManager {
    /* Every InterruptManager is treated like an EventSource from the
     * perspective of the EventLoop. */
    UA_EventSource eventSource;

    /* Register an interrupt. The handle and context information is passed
     * through to the callback.
     *
     * The interruptHandle is a numerical identifier of the interrupt. In some
     * cases, such as POSIX signals, this is enough information to register
     * callback. For other interrupt systems (architectures) additional
     * parameters may be required and can be passed in via the parameters
     * key-value list. See the implementation-specific documentation.
     *
     * The interruptContext is opaque user-defined information and passed
     * through to the callback without modification. */
    UA_StatusCode
    (*registerInterrupt)(UA_InterruptManager *im, uintptr_t interruptHandle,
                         size_t paramsSize, const UA_KeyValuePair *params,
                         UA_InterruptCallback callback, void *interruptContext);

    /* Remove a registered interrupt. Returns no error code if the interrupt is
     * already deregistered. */
    void
    (*deregisterInterrupt)(UA_InterruptManager *im, uintptr_t interruptHandle);
};

/**
 * POSIX-Specific Implementation
 * -----------------------------
 * The POSIX compatibility of WIN32 is 'close enough'. So a joint implementation
 * is provided. */

#if defined(UA_ARCHITECTURE_POSIX) || defined(UA_ARCHITECTURE_WIN32)

UA_EXPORT UA_EventLoop *
UA_EventLoop_new_POSIX(const UA_Logger *logger);

/**
 * TCP Connection Manager
 * ~~~~~~~~~~~~~~~~~~~~~~
 * Listens on the network and manages TCP connections. This should be available
 * for all architectures.
 *
 * The `openConnection` callback is used to create both client and server
 * sockets. A server socket listens and accepts incoming connections (creates an
 * active connection). This is distinguished by the key-value parameters passed
 * to `openConnection`. Note that a single call to `openConnection` for a server
 * connection may actually create multiple connections (one per hostname /
 * device).
 *
 * The `connectionCallback` of the server socket and `context` of the server
 * socket is reused for each new connection. But the key-value parameters for
 * the first callback are different between server and client connections.
 *
 * The following list defines the parameters and their type. Note that some
 * parameters are only set for the first callback when a new connection opens.
 *
 * Configuration parameters for the entire ConnectionManager:
 * - 0:recv-bufsize [uint32]: Size of the buffer that is allocated for receiving
 *                            messages (default 64kB).
 *
 * Open Connection Parameters:
 * - Active Connection
 *   - 0:hostname [string]: Hostname (or IPv4/v6 address) to connect to (required).
 *   - 0:port [uint16]: Port of the target host (required).
 * - Passive Connection (Listening on a port for incoming connections)
 *   - 0:listen-port [uint16]: Port to listen for new connections (required).
 *   - 0:listen-hostnames [string array]: Hostnames of the devices to listen on
 *                                        (default: listen on all devices).
 *
 * Connection Callback Parameters (first callback only):
 * - Active Connection
 *   - 0:remote-hostname [string]: Hostname of the remote connection.
 * - Passive Connection
 *   - 0:listen-hostname [string]: Local hostname for that particular
 *                                 listen-connection.
 *   - 0:listen-port [uint16]: Port on which the connection listens.
 *
 * Send Parameters:
 * No additional parameters for sending over an established TCP socket defined. */
UA_EXPORT UA_ConnectionManager *
UA_ConnectionManager_new_POSIX_TCP(const UA_String eventSourceName);

/**
 * UDP Connection Manager
 * ~~~~~~~~~~~~~~~~~~~~~~
 * Listens on the network and manages UDP connections. This should be available
 * for all architectures.
 *
 * The configuration parameters have to set before calling _start to take
 * effect.
 *
 * Configuration Parameters:
 * - 0:listen-port [uint16]: Port to listen for new connections (default: do not
 *                           listen on any port).
 * - 0:listen-hostnames [string | string array]: Hostnames of the devices to
 *                                               listen on (default: listen on
 *                                               all devices).
 * - 0:recv-bufsize [uint32]: Size of the buffer that is allocated for receiving
 *                            messages (default 64kB).
 *
 * Open Connection Parameters:
 * - 0:hostname [string]: Hostname (or IPv4/v6 address) to connect to (required).
 * - 0:port [uint16]: Port of the target host (required).
 * - 0:ttl [uint32]: Multicast time to live, (optional, default: 1 - meaning multicast is
 *                   available only to the local subnet).
 * - 0:loopback [boolean]: Whether or not to use multicast loopback, enabling
 *                         local interfaces belonging to the multicast group
 *                         to receive packages. (optional, default: enabled).
 * - 0:reuse [boolean]: Set reuse address -> enables sharing of the same
 *                      listening address on different sockets (optional, default: disabled).
 * - 0:sockpriority [uint32]: The socket priority (optional) - only available on linux.
 *                            packets with a higher priority may be
 *                            processed first depending on the selected device queueing
 *                            discipline.  Setting a priority outside the range 0 to 6
 *                            requires the CAP_NET_ADMIN capability.
 *
 * Connection Callback Paramters:
 * - 0:remote-hostname [string]: When a new connection is opened by listening on
 *                               a port, the first callback contains the remote
 *                               hostname parameter.
 *
 * Send Parameters:
 * No additional parameters for sending over an established UDP socket defined. */
UA_EXPORT UA_ConnectionManager *
UA_ConnectionManager_new_POSIX_UDP(const UA_String eventSourceName);

/**
 * Signal Interrupt Manager
 * ~~~~~~~~~~~~~~~~~~~~~~~~
 * Create an instance of the interrupt manager that handles POSX signals. This
 * interrupt manager takes the numerical interrupt identifiers from <signal.h>
 * for the interruptHandle. */
UA_EXPORT UA_InterruptManager *
UA_InterruptManager_new_POSIX(const UA_String eventSourceName);

#endif /* defined(UA_ARCHITECTURE_POSIX) || defined(UA_ARCHITECTURE_WIN32) */

_UA_END_DECLS

#endif /* UA_EVENTLOOP_H_ */
