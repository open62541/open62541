/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. */

#ifndef UA_EVENTLOOP_IOCP_H_
#define UA_EVENTLOOP_IOCP_H_

#include <open62541/config.h>
#include <open62541/plugin/eventloop.h>

#include "../common/timer.h"
#include "../common/eventloop_common.h"
#include "../deps/open62541_queue.h"

#include <stdint.h>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

_UA_BEGIN_DECLS

#define UA_MAXBACKLOG 100
#define UA_MAXHOSTNAME_LENGTH 256
#define UA_MAXPORTSTR_LENGTH 6

#define UA_IOCP_ACCEPT_DEPTH 8u
#define UA_IOCP_COMPLETION_BATCH 64u
#define UA_IOCP_DEFAULT_UDP_RECEIVE_DEPTH 1u
#define UA_IOCP_ACCEPT_ADDRESS_BYTES \
    (2u * (sizeof(SOCKADDR_STORAGE) + 16u))

#define UA_IOCP_DEFAULT_SEND_QUEUE_LOW (256u * 1024u)
#define UA_IOCP_DEFAULT_SEND_QUEUE_HIGH (512u * 1024u)
#define UA_IOCP_DEFAULT_SEND_QUEUE_HARD (1024u * 1024u)
#define UA_IOCP_DEFAULT_SEND_QUEUE_MESSAGES 1024u
#define UA_IOCP_DEFAULT_GLOBAL_SEND_QUEUE_HARD (64u * 1024u * 1024u)
#define UA_IOCP_DEFAULT_SEND_STALL_TIMEOUT_MS 60000u
#define UA_IOCP_SEND_BUFFER_MAGIC 0x494F4350u
#define UA_IOCP_BUFFER_POOL_BLOCKS 64u

typedef struct UA_EventLoopWIN32 UA_EventLoopWIN32;
typedef struct UA_ConnectionManagerIOCP UA_ConnectionManagerIOCP;
typedef struct UA_RegisteredSocketIOCP UA_RegisteredSocketIOCP;
typedef struct UA_IOCPOperation UA_IOCPOperation;

typedef enum {
    UA_IOCP_SOURCE_SOCKET,
    UA_IOCP_SOURCE_CONTROL,
    UA_IOCP_SOURCE_INTERRUPT
} UA_IOCPSourceKind;

typedef enum {
    UA_IOCP_CONTROL_CANCEL = 1u << 0,
    UA_IOCP_CONTROL_TIMER_CHANGED = 1u << 1,
    UA_IOCP_CONTROL_DELAYED_CALLBACK = 1u << 2
} UA_IOCPControlPacket;

typedef enum {
    UA_IOCP_SOCKET_TCP_LISTENER,
    UA_IOCP_SOCKET_TCP_CONNECTING,
    UA_IOCP_SOCKET_TCP_ESTABLISHED,
    UA_IOCP_SOCKET_UDP_READY,
    UA_IOCP_SOCKET_CLOSING
} UA_IOCPSocketState;

typedef enum {
    UA_IOCP_BUFFER_POOLED,
    UA_IOCP_BUFFER_HEAP
} UA_IOCPBufferKind;

typedef struct {
    UA_IOCPSourceKind kind;
    void *owner;
} UA_IOCPCompletionSource;

typedef struct {
    UA_IOCPCompletionSource source;
} UA_IOCPControlSource;

typedef void
(*UA_IOCPCompletionHandler)(UA_IOCPOperation *operation,
                            DWORD bytesTransferred, DWORD error);

struct UA_IOCPOperation {
    OVERLAPPED overlapped; /* Must be first. */
    UA_RegisteredSocketIOCP *owner;
    UA_IOCPCompletionHandler handler;
    UA_Boolean submitted;
};

struct UA_RegisteredSocketIOCP {
    UA_IOCPCompletionSource source;
    ZIP_ENTRY(UA_RegisteredSocketIOCP) treeEntry;
    UA_DelayedCallback delayedClose;

    UA_ConnectionManagerIOCP *manager;
    SOCKET socket;
    UA_IOCPSocketState state;

    UA_ConnectionManager_connectionCallback callback;
    void *application;
    void *context;

    UA_UInt32 outstandingOperations;
    UA_Boolean delayedCloseQueued;
    UA_Boolean callbackOpened;
    UA_Boolean closingCallbackSent;
};

enum ZIP_CMP
UA_IOCP_compareSocket(const SOCKET *a, const SOCKET *b);

typedef ZIP_HEAD(UA_IOCPSocketTree, UA_RegisteredSocketIOCP)
    UA_IOCPSocketTree;

ZIP_FUNCTIONS(UA_IOCPSocketTree, UA_RegisteredSocketIOCP, treeEntry,
              SOCKET, socket, UA_IOCP_compareSocket)

typedef struct UA_IOCPSendBuffer {
    struct UA_IOCPSendBuffer *next;
    UA_ConnectionManagerIOCP *owner;
    size_t capacity;
    size_t length;
    UA_UInt32 magic;
    UA_IOCPBufferKind kind;
    UA_Byte data[];
} UA_IOCPSendBuffer;

typedef struct {
    UA_IOCPSendBuffer *freeList;
    size_t blockCapacity;
    size_t cachedBlocks;
    size_t maxCachedBlocks;
} UA_IOCPBufferPool;

struct UA_ConnectionManagerIOCP {
    UA_ConnectionManager cm;
    UA_EventLoopWIN32 *eventLoop;

    UA_IOCPSocketTree sockets;
    size_t socketCount;
    size_t connectionCount;
    size_t maxConnections;
    size_t receiveBufferSize;

    UA_IOCPBufferPool sendBufferPool;
    size_t globalQueuedSendBytes;
    size_t globalSendQueueHardLimit;

    size_t sendQueueLowWatermark;
    size_t sendQueueHighWatermark;
    size_t sendQueueHardLimit;
    size_t sendQueueMessageLimit;
};

struct UA_EventLoopWIN32 {
    UA_EventLoop eventLoop;
    UA_Timer timer;

    UA_atomic(UA_DelayedCallback *) delayedHead1;
    UA_atomic(UA_DelayedCallback *) delayedHead2;
    UA_atomic(UA_atomic(UA_DelayedCallback *)*) delayedTail;

    HANDLE completionPort;
    UA_IOCPControlSource controlSource;
    UA_atomic(uintptr_t) pendingControlPackets;
    size_t outstandingOperations;
    DWORD eventLoopThreadId;

    volatile UA_Boolean executing;
    UA_Boolean winsockStarted;
    UA_Boolean stopping;

#if UA_MULTITHREADING >= 100
    UA_Lock elMutex;
#endif
};

/* Event-loop core */
UA_DateTime
UA_EventLoopWIN32_nextTimer(UA_EventLoop *public_el);

UA_StatusCode
UA_EventLoopWIN32_addTimer(UA_EventLoop *public_el, UA_Callback cb,
                           void *application, void *data, UA_Double interval_ms,
                           UA_DateTime *baseTime, UA_TimerPolicy timerPolicy,
                           UA_UInt64 *callbackId);

UA_StatusCode
UA_EventLoopWIN32_modifyTimer(UA_EventLoop *public_el, UA_UInt64 callbackId,
                              UA_Double interval_ms, UA_DateTime *baseTime,
                              UA_TimerPolicy timerPolicy);

void
UA_EventLoopWIN32_removeTimer(UA_EventLoop *public_el, UA_UInt64 callbackId);

void
UA_EventLoopWIN32_addDelayedCallback(UA_EventLoop *public_el,
                                     UA_DelayedCallback *dc);

void
UA_EventLoopWIN32_removeDelayedCallback(UA_EventLoop *public_el,
                                        UA_DelayedCallback *dc);

void
UA_EventLoopWIN32_processDelayed(UA_EventLoopWIN32 *el);

UA_StatusCode
UA_EventLoopWIN32_registerEventSource(UA_EventLoop *el, UA_EventSource *es);

UA_StatusCode
UA_EventLoopWIN32_deregisterEventSource(UA_EventLoop *el, UA_EventSource *es);

void
UA_EventLoopWIN32_lock(UA_EventLoop *public_el);

void
UA_EventLoopWIN32_unlock(UA_EventLoop *public_el);

void
UA_EventLoopWIN32_cancel(UA_EventLoop *el);

UA_StatusCode
UA_IOCP_associateSocket(UA_EventLoopWIN32 *el,
                        UA_RegisteredSocketIOCP *registeredSocket);

void
UA_IOCP_prepareOperation(UA_IOCPOperation *operation);

void
UA_IOCP_abortOperation(UA_IOCPOperation *operation);

void
UA_IOCP_postControl(UA_EventLoopWIN32 *el, UA_UInt32 controlBit);

UA_StatusCode
UA_IOCP_statusFromSocketError(DWORD error);

/* Common connection-manager and buffer helpers */
void
UA_IOCP_initConnectionManager(UA_ConnectionManagerIOCP *manager,
                              const UA_String eventSourceName,
                              const UA_String protocol);

UA_StatusCode
UA_IOCP_configureConnectionManager(UA_ConnectionManagerIOCP *manager);

void
UA_IOCP_clearConnectionManager(UA_ConnectionManagerIOCP *manager);

UA_StatusCode
UA_IOCP_allocNetworkBuffer(UA_ConnectionManager *cm,
                           uintptr_t connectionId,
                           UA_ByteString *buf, size_t bufSize);

void
UA_IOCP_freeNetworkBuffer(UA_ConnectionManager *cm,
                          uintptr_t connectionId,
                          UA_ByteString *buf);

UA_IOCPSendBuffer *
UA_IOCP_takeNetworkBuffer(UA_ConnectionManagerIOCP *manager,
                          UA_ByteString *buf);

void
UA_IOCP_releaseSendBuffer(UA_ConnectionManagerIOCP *manager,
                          UA_IOCPSendBuffer *buffer);

UA_RegisteredSocketIOCP *
UA_IOCP_findSocket(UA_ConnectionManagerIOCP *manager, uintptr_t connectionId);

UA_StatusCode
UA_IOCP_insertSocket(UA_ConnectionManagerIOCP *manager,
                     UA_RegisteredSocketIOCP *registeredSocket,
                     UA_IOCPSocketState state, SOCKET socket,
                     UA_ConnectionManager_connectionCallback callback,
                     void *application, void *context);

void
UA_IOCP_removeSocket(UA_ConnectionManagerIOCP *manager,
                     UA_RegisteredSocketIOCP *registeredSocket);

void
UA_IOCP_cancelSocket(UA_RegisteredSocketIOCP *registeredSocket);

void
UA_IOCP_maybeQueueClose(UA_RegisteredSocketIOCP *registeredSocket);

/* Implemented by the interrupt manager. */
void
UA_InterruptManagerIOCP_dispatch(void *interruptManager, DWORD signal);
_UA_END_DECLS

#endif /* UA_EVENTLOOP_IOCP_H_ */
