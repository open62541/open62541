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
 * Timer Policies
 * --------------
 * A timer comes with a cyclic interval in which a callback is executed. If an
 * application is congested the interval can be missed. Two different policies
 * can be used when this happens. Either schedule the next execution after the
 * interval has elapsed again from the current time onwards or stay within the
 * regular interval with respect to the original basetime. */

typedef enum {
    UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME = 0, /* deprecated */
    UA_TIMER_HANDLE_CYCLEMISS_WITH_BASETIME = 1,    /* deprecated */
    UA_TIMERPOLICY_CURRENTTIME = 0,
    UA_TIMERPOLICY_BASETIME = 1,
} UA_TimerPolicy;

/**
 * Event Loop
 * ----------
 * The EventLoop implementation is part of the selected architecture. For
 * example, "Win32/POSIX" stands for a Windows environment with an EventLoop
 * that uses the POSIX API. Several EventLoops can be instantiated in parallel.
 * But the globally defined functions are the same everywhere. */

typedef void (*UA_Callback)(void *application, void *context);

/* Delayed callbacks are executed not when they are registered, but in the
 * following EventLoop cycle */
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

    /* See the implementation-specific documentation for possible parameters.
     * The params map is cleaned up when the EventLoop is _free'd. */
    UA_KeyValueMap params;

    /* EventLoop Lifecycle
     * ~~~~~~~~~~~~~~~~~~~~
     * The EventLoop state also controls the state of the configured
     * EventSources. Stopping the EventLoop gracefully closes e.g. the open
     * network connections. The only way to process incoming events is to call
     * the 'run' method. Events are then triggering their respective callbacks
     * from within that method.*/

    const volatile UA_EventLoopState state; /* Only read the state from outside */

    /* Start the EventLoop and start all already registered EventSources */
    UA_StatusCode (*start)(UA_EventLoop *el);

    /* Stop all EventSources. This is asynchronous and might need a few
     * iterations of the main-loop to succeed. */
    void (*stop)(UA_EventLoop *el);

    /* Clean up the EventLoop and free allocated memory. Can fail if the
     * EventLoop is not stopped. */
    UA_StatusCode (*free)(UA_EventLoop *el);

    /* Wait for events and processs them for at most "timeout" ms or until an
     * unrecoverable error occurs. If timeout==0, then only already received
     * events are processed. Returns immediately after processing the first
     * (batch of) event(s). */
    UA_StatusCode (*run)(UA_EventLoop *el, UA_UInt32 timeout);

    /* The "run" method is blocking and waits for events during a timeout
     * period. This cancels the "run" method to return immediately. */
    void (*cancel)(UA_EventLoop *el);

    /* EventLoop Time Domain
     * ~~~~~~~~~~~~~~~~~~~~~
     * Each EventLoop instance can manage its own time domain. This affects the
     * execution of timed callbacks and time-based sending of network packets.
     * Managing independent time domains is important when different parts of
     * the same system are synchronized to different external master clocks.
     *
     * Each EventLoop uses a "normal" and a "monotonic" clock. The monotonic
     * clock does not (necessarily) conform to the current wallclock date. But
     * its time intervals are more precise. So it is used for all internally
     * scheduled events of the EventLoop (e.g. timed callbacks and time-based
     * sending of network packets). The normal and monotonic clock sources can
     * be configured via parameters before starting the EventLoop. See the
     * architecture-specific documentation for that.
     *
     * Note that the logger configured in the EventLoop generates timestamps
     * independently. If the logger uses a different time domain than the
     * EventLoop, discrepancies may appear in the logs.
     *
     * The EventLoop clocks can be read via the following functons. See
     * `open62541/types.h` for the documentation of their equivalent globally
     * defined functions. */

    UA_DateTime (*dateTime_now)(UA_EventLoop *el);
    UA_DateTime (*dateTime_nowMonotonic)(UA_EventLoop *el);
    UA_Int64    (*dateTime_localTimeUtcOffset)(UA_EventLoop *el);

    /* Timed Callbacks
     * ~~~~~~~~~~~~~~~
     * Cyclic callbacks are executed regularly with an interval.
     * A timed callback is executed only once. */

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

    /* Delayed Callbacks
     * ~~~~~~~~~~~~~~~~~
     * Delayed callbacks are executed once in the next iteration of the
     * EventLoop and then deregistered automatically. A typical use case is to
     * delay a resource cleanup to a point where it is known that the resource
     * has no remaining users.
     *
     * The delayed callbacks are processed in each of the cycle of the EventLoop
     * between the handling of timed cyclic callbacks and polling for (network)
     * events. The memory for the delayed callback is *NOT* automatically freed
     * after the execution.
     *
     * addDelayedCallback is non-blocking and can be called from an interrupt
     * context. removeDelayedCallback can take a mutex and is blocking. */

    void (*addDelayedCallback)(UA_EventLoop *el, UA_DelayedCallback *dc);
    void (*removeDelayedCallback)(UA_EventLoop *el, UA_DelayedCallback *dc);

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
    UA_KeyValueMap params;

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
      const UA_KeyValueMap *params, UA_ByteString msg);

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
    (*openConnection)(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
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
                          const UA_KeyValueMap *params, UA_ByteString *buf);

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
 * - Registered interrupts are only intercepted from within the running EventLoop
 * - Processing an interrupt in the EventLoop is handled similarly to handling a
 *   network event: all methods and also memory allocation are available from
 *   within the interrupt callback. */

/* Interrupts can have additional key-value 'instanceInfos' for each individual
 * triggering. See the architecture-specific documentation. */
typedef void
(*UA_InterruptCallback)(UA_InterruptManager *im,
                        uintptr_t interruptHandle, void *interruptContext,
                        const UA_KeyValueMap *instanceInfos);

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
                         const UA_KeyValueMap *params,
                         UA_InterruptCallback callback, void *interruptContext);

    /* Remove a registered interrupt. Returns no error code if the interrupt is
     * already deregistered. */
    void
    (*deregisterInterrupt)(UA_InterruptManager *im, uintptr_t interruptHandle);
};

#if defined(UA_ARCHITECTURE_POSIX) || defined(UA_ARCHITECTURE_WIN32)

/**
 * POSIX EventLop Implementation
 * -----------------------------
 * The POSIX compatibility of Win32 is 'close enough'. So a joint implementation
 * is provided. The configuration paramaters must be set before starting the
 * EventLoop.
 *
 * **Clock configuration (Linux and BSDs only)**
 *
 * 0:clock-source [int32]
 *    Clock source (default: CLOCK_REALTIME).
 *
 * 0:clock-source-monotonic [int32]:
 *   Clock source used for time intervals. A non-monotonic source can be used as
 *   well. But expect accordingly longer sleep-times for timed events when the
 *   clock is set to the past. See the man-page of "clock_gettime" on how to get
 *   a clock source id for a character-device such as /dev/ptp0. (default:
 *   CLOCK_MONOTONIC_RAW) */

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
 * **Configuration parameters for the ConnectionManager (set before start)**
 *
 * 0:recv-bufsize [uint32]
 *    Size of the buffer that is statically allocated for receiving messages
 *    (default 64kB).
 *
 * 0:send-bufsize [uint32]
 *    Size of the statically allocated buffer for sending messages. This then
 *    becomes an upper bound for the message size. If undefined a fresh buffer
 *    is allocated for every `allocNetworkBuffer` (default: no buffer).
 *
 * **Open Connection Parameters:**
 *
 * 0:address [string | array of string]
 *    Hostname or IPv4/v6 address for the connection (scalar parameter required
 *    for active connections). For listen-connections the address contains the
 *    local hostnames or IP addresses for listening. If undefined, listen on all
 *    interfaces INADDR_ANY. (default: undefined)
 *
 * 0:port [uint16]
 *    Port of the target host (required).
 *
 * 0:listen [boolean]
 *    Listen-connection or active-connection (default: false)
 *
 * 0:validate [boolean]
 *    If true, the connection setup will act as a dry-run without actually
 *    creating any connection but solely validating the provided parameters
 *    (default: false)
 *
 * **Active Connection Connection Callback Parameters (first callback only):**
 *
 * 0:remote-address [string]
 *    Address of the remote side (hostname or IP address).
 *
 * **Listen Connection Connection Callback Parameters (first callback only):**
 *
 * 0:listen-address [string]
 *    Local address (IP or hostname) for the new listen-connection.
 *
 * 0:listen-port [uint16]
 *    Port on which the new connection listens.
 *
 * **Send Parameters:**
 *
 * No additional parameters for sending over an established TCP socket
 * defined. */
UA_EXPORT UA_ConnectionManager *
UA_ConnectionManager_new_POSIX_TCP(const UA_String eventSourceName);

/**
 * UDP Connection Manager
 * ~~~~~~~~~~~~~~~~~~~~~~
 * Manages UDP connections. This should be available for all architectures. The
 * configuration parameters have to set before calling _start to take effect.
 *
 * **Configuration parameters for the ConnectionManager (set before start)**
 *
 * 0:recv-bufsize [uint32]
 *    Size of the buffer that is statically allocated for receiving messages
 *    (default 64kB).
 *
 * 0:send-bufsize [uint32]
 *    Size of the statically allocated buffer for sending messages. This then
 *    becomes an upper bound for the message size. If undefined a fresh buffer
 *    is allocated for every `allocNetworkBuffer` (default: no buffer).
 *
 * **Open Connection Parameters:**
 *
 * 0:listen [boolean]
 *    Use the connection for listening or for sending (default: false)
 *
 * 0:address [string | string array]
 *    Hostname (or IPv4/v6 address) for sending or receiving. A scalar is
 *    required for sending. For listening a string array for the list-hostnames
 *    is possible as well (default: list on all hostnames).
 *
 * 0:port [uint16]
 *    Port for sending or listening (required).
 *
 * 0:interface [string]
 *    Network interface for listening or sending (e.g. when using multicast
 *    addresses)
 *
 * 0:ttl [uint32]
 *    Multicast time to live, (optional, default: 1 - meaning multicast is
 *    available only to the local subnet).
 *
 * 0:loopback [boolean]
 *    Whether or not to use multicast loopback, enabling local interfaces
 *    belonging to the multicast group to receive packages. (default: enabled).
 *
 * 0:reuse [boolean]
 *    Enables sharing of the same listening address on different sockets
 *    (default: disabled).
 *
 * 0:sockpriority [uint32]
 *    The socket priority (optional) - only available on linux. packets with a
 *    higher priority may be processed first depending on the selected device
 *    queueing discipline. Setting a priority outside the range 0 to 6 requires
 *    the CAP_NET_ADMIN capability (on Linux).
 *
 * 0:validate [boolean]
 *    If true, the connection setup will act as a dry-run without actually
 *    creating any connection but solely validating the provided parameters
 *    (default: false)
 *
 * **Connection Callback Paramters:**
 *
 * 0:remote-address [string]
 *    Contains the remote IP address.
 *
 * 0:remote-port [uint16]
 *    Contains the remote port.
 *
 * **Send Parameters:**
 *
 * No additional parameters for sending over an UDP connection defined. */
UA_EXPORT UA_ConnectionManager *
UA_ConnectionManager_new_POSIX_UDP(const UA_String eventSourceName);

/**
 * Ethernet Connection Manager
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Listens on the network and manages UDP connections. This should be available
 * for all architectures. The configuration parameters have to set before
 * calling _start to take effect.
 *
 * **Configuration parameters for the ConnectionManager (set before start)**
 *
 * 0:recv-bufsize [uint32]
 *    Size of the buffer that is statically allocated for receiving messages
 *    (default 64kB).
 *
 * 0:send-bufsize [uint32]
 *    Size of the statically allocated buffer for sending messages. This then
 *    becomes an upper bound for the message size. If undefined a fresh buffer
 *    is allocated for every `allocNetworkBuffer` (default: no buffer).
 *
 * **Open Connection Parameters:**
 *
 * 0:listen [bool]
 *    The connection is either for sending or for listening (default: false).
 *
 * 0:interface [string]
 *    The name of the Ethernet interface to use (required).
 *
 * 0:address [string]
 *    MAC target address consisting of six groups of hexadecimal digits
 *    separated by hyphens such as 01-23-45-67-89-ab. For sending this is a
 *    required parameter. For listening this is a multicast address that the
 *    connections tries to register for.
 *
 * 0:priority [int32]
 *    Set the socket priority for sending (cf. SO_PRIORITY)
 *
 * 0:ethertype [uint16]
 *    EtherType for sending and receiving frames (optional). For listening
 *    connections, this filters out all frames with different EtherTypes.
 *
 * 0:promiscuous [bool]
 *    Receive frames also for different target addresses. Defined only for
 *    listening connections (default: false).
 *
 * 0:vid [uint16]
 *    12-bit VLAN identifier (optional for send connections).
 *
 * 0:pcp [byte]
 *    3-bit priority code point (optional for send connections).
 *
 * 0:dei [bool]
 *    1-bit drop eligible indicator (optional for send connections).
 *
 * 0:validate [boolean]
 *    If true, the connection setup will act as a dry-run without actually
 *    creating any connection but solely validating the provided parameters
 *    (default: false)
 *
 * Sending with a txtime (for Time-Sensitive Networking) is possible on recent
 * Linux kernels, If enabled for the socket, then a txtime parameters can be
 * passed to `sendWithConnection`. Note that the clock source for txtime sending
 * is the monotonic clock source set for the entire EventLoop. Check the
 * EventLoop parameters for how to set that e.g. to a PTP clock source. The
 * txtime parameters uses Linux conventions.
 *
 * 0:txtime-enable [bool]
 *    Enable sending with a txtime for the connection (default: false).
 *
 * 0:txtime-flags [uint32]
 *    txtime flags set for the socket (default: SOF_TXTIME_REPORT_ERRORS).
 *
 * **Send Parameters (only with txtime enabled for the connection)**
 *
 * 0:txtime [datetime]
 *    Time when the message is sent out (Datetime has 100ns precision) for the
 *    "monotonic" clock source of the EventLoop.
 *
 * 0:txtime-pico [uint16]
 *    Picoseconds added to the txtime timestamp (default: 0).
 *
 * 0:txtime-drop-late [bool]
 *    Drop message if it cannot be sent in time (default: true). */
UA_EXPORT UA_ConnectionManager *
UA_ConnectionManager_new_POSIX_Ethernet(const UA_String eventSourceName);

/**
 * MQTT Connection Manager
 * ~~~~~~~~~~~~~~~~~~~~~~~
 * The MQTT ConnectionManager reuses the TCP ConnectionManager that is
 * configured in the EventLoop. Hence the MQTT ConnectionManager is platform
 * agnostic and does not require porting. An MQTT connection is for a
 * combination of broker and topic. The MQTT ConnectionManager can group
 * connections to the same broker in the background. Hence adding multiple
 * connections for the same broker is "cheap". To have individual control,
 * separate connections are created for each topic and for each direction
 * (publishing / subscribing).
 *
 * **Open Connection Parameters:**
 *
 * 0:address [string]
 *    Hostname or IPv4/v6 address of the MQTT broker (required).
 *
 * 0:port [uint16]
 *    Port of the MQTT broker (default: 1883).
 *
 * 0:username [string]
 *    Username to use (default: none)
 *
 * 0:password [string]
 *    Password to use (default: none)
 *
 * 0:keep-alive [uint16]
 *   Number of seconds for the keep-alive (ping) (default: 400).
 *
 * 0:validate [boolean]
 *    If true, the connection setup will act as a dry-run without actually
 *    creating any connection but solely validating the provided parameters
 *    (default: false)
 *
 * 0:topic [string]
 *    Topic to which the connection is associated (required).
 *
 * 0:subscribe [bool]
 *    Subscribe to the topic (default: false). Otherwise it is only possible to
 *    publish on the topic. Subscribed topics can also be published to.
 *
 * **Connection Callback Parameters:**
 *
 * 0:topic [string]
 *    The value set during connect.
 *
 * 0:subscribe [bool]
 *    The value set during connect.
 *
 * **Send Parameters:**
 *
 * No additional parameters for sending over an Ethernet connection defined. */
UA_EXPORT UA_ConnectionManager *
UA_ConnectionManager_new_MQTT(const UA_String eventSourceName);

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
