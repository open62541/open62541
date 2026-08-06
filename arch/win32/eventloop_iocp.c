/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. */

#include "eventloop_iocp.h"

#include <limits.h>

/*********************/
/* Atomic bit helpers */
/*********************/

static UA_Boolean
setAtomicBit(UA_atomic(uintptr_t) *value, UA_UInt32 bit) {
    uintptr_t old = UA_atomic_load(value);
    for(;;) {
        if(old & bit)
            return false;
        uintptr_t expected = old;
        UA_atomic_cmpxchg(value, &expected, old | (uintptr_t)bit);
        if(expected == old)
            return true;
        old = expected;
    }
}

static void
clearAtomicBit(UA_atomic(uintptr_t) *value, UA_UInt32 bit) {
    uintptr_t old = UA_atomic_load(value);
    for(;;) {
        uintptr_t expected = old;
        UA_atomic_cmpxchg(value, &expected, old & ~(uintptr_t)bit);
        if(expected == old)
            return;
        old = expected;
    }
}

/*********************/
/* Timer and delayed */
/*********************/

UA_DateTime
UA_EventLoopWIN32_nextTimer(UA_EventLoop *public_el) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    if(el->delayedHead1 > (UA_DelayedCallback*)0x01 ||
       el->delayedHead2 > (UA_DelayedCallback*)0x01)
        return el->eventLoop.dateTime_nowMonotonic(public_el);
    return UA_Timer_next(&el->timer);
}

UA_StatusCode
UA_EventLoopWIN32_addTimer(UA_EventLoop *public_el, UA_Callback cb,
                           void *application, void *data, UA_Double interval_ms,
                           UA_DateTime *baseTime, UA_TimerPolicy timerPolicy,
                           UA_UInt64 *callbackId) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    UA_StatusCode res =
        UA_Timer_add(&el->timer, cb, application, data, interval_ms,
                     public_el->dateTime_nowMonotonic(public_el),
                     baseTime, timerPolicy, callbackId);
    if(res == UA_STATUSCODE_GOOD)
        UA_IOCP_postControl(el, UA_IOCP_CONTROL_TIMER_CHANGED);
    return res;
}

UA_StatusCode
UA_EventLoopWIN32_modifyTimer(UA_EventLoop *public_el, UA_UInt64 callbackId,
                              UA_Double interval_ms, UA_DateTime *baseTime,
                              UA_TimerPolicy timerPolicy) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    UA_StatusCode res =
        UA_Timer_modify(&el->timer, callbackId, interval_ms,
                        public_el->dateTime_nowMonotonic(public_el),
                        baseTime, timerPolicy);
    if(res == UA_STATUSCODE_GOOD)
        UA_IOCP_postControl(el, UA_IOCP_CONTROL_TIMER_CHANGED);
    return res;
}

void
UA_EventLoopWIN32_removeTimer(UA_EventLoop *public_el, UA_UInt64 callbackId) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    UA_Timer_remove(&el->timer, callbackId);
    UA_IOCP_postControl(el, UA_IOCP_CONTROL_TIMER_CHANGED);
}

void
UA_EventLoopWIN32_addDelayedCallback(UA_EventLoop *public_el,
                                     UA_DelayedCallback *dc) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    dc->next = NULL;

    UA_atomic(UA_atomic(UA_DelayedCallback*)*) prevNext;
    UA_atomic_xchg(&el->delayedTail, &dc->next, &prevNext);
    UA_atomic_store(prevNext, dc);
    UA_IOCP_postControl(el, UA_IOCP_CONTROL_DELAYED_CALLBACK);
}

static void
resetDelayedQueue(UA_EventLoopWIN32 *el,
                  UA_atomic(UA_DelayedCallback*) *oldHead,
                  UA_atomic(UA_atomic(UA_DelayedCallback*)*) *oldTail) {
    if(el->delayedHead1 <= (UA_DelayedCallback*)0x01 &&
       el->delayedHead2 <= (UA_DelayedCallback*)0x01)
        return;

    UA_Boolean active1 =
        (el->delayedHead1 != (UA_DelayedCallback*)0x01);
    UA_atomic(UA_DelayedCallback*) *activeHead =
        active1 ? &el->delayedHead1 : &el->delayedHead2;
    UA_atomic(UA_DelayedCallback*) *inactiveHead =
        active1 ? &el->delayedHead2 : &el->delayedHead1;

    UA_atomic_store(inactiveHead, NULL);
    UA_atomic_xchg(activeHead, (UA_DelayedCallback*)0x01, oldHead);
    UA_atomic_xchg(&el->delayedTail, inactiveHead, oldTail);
}

void
UA_EventLoopWIN32_removeDelayedCallback(UA_EventLoop *public_el,
                                        UA_DelayedCallback *dc) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    UA_LOCK(&el->elMutex);

    UA_atomic(UA_DelayedCallback*) current = NULL;
    UA_atomic(UA_atomic(UA_DelayedCallback*)*) tail = NULL;
    resetDelayedQueue(el, &current, &tail);
    UA_DelayedCallback *last = (UA_DelayedCallback*)(uintptr_t)tail;

    UA_DelayedCallback *next;
    for(; current; current = next) {
        next = current->next;
        while(!next && current != last)
            next = UA_atomic_load(&current->next);
        if(current != dc)
            UA_EventLoopWIN32_addDelayedCallback(public_el, current);
    }

    UA_UNLOCK(&el->elMutex);
}

void
UA_EventLoopWIN32_processDelayed(UA_EventLoopWIN32 *el) {
    UA_LOCK_ASSERT(&el->elMutex);

    UA_atomic(UA_DelayedCallback*) current = NULL;
    UA_atomic(UA_atomic(UA_DelayedCallback*)*) tail = NULL;
    resetDelayedQueue(el, &current, &tail);
    UA_DelayedCallback *last = (UA_DelayedCallback*)(uintptr_t)tail;

    UA_DelayedCallback *next;
    for(; current; current = next) {
        next = current->next;
        while(!next && current != last)
            next = UA_atomic_load(&current->next);
        if(current->callback)
            current->callback(current->application, current->context);
    }
}

/*****************/
/* IOCP helpers  */
/*****************/

enum ZIP_CMP
UA_IOCP_compareSocket(const SOCKET *a, const SOCKET *b) {
    if(*a == *b)
        return ZIP_CMP_EQ;
    return (*a < *b) ? ZIP_CMP_LESS : ZIP_CMP_MORE;
}

UA_StatusCode
UA_IOCP_statusFromSocketError(DWORD error) {
    switch(error) {
    case 0:
        return UA_STATUSCODE_GOOD;
    case WSAEWOULDBLOCK:
    case WSA_IO_PENDING:
        return UA_STATUSCODE_BADWOULDBLOCK;
    case WSAECONNABORTED:
    case WSAECONNRESET:
    case WSAENOTCONN:
    case ERROR_OPERATION_ABORTED:
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    case WSAETIMEDOUT:
        return UA_STATUSCODE_BADTIMEOUT;
    case WSAENOBUFS:
        return UA_STATUSCODE_BADOUTOFMEMORY;
    default:
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
}

UA_StatusCode
UA_IOCP_associateSocket(UA_EventLoopWIN32 *el,
                        UA_RegisteredSocketIOCP *registeredSocket) {
    if(!el->completionPort)
        return UA_STATUSCODE_BADINTERNALERROR;

    HANDLE result =
        CreateIoCompletionPort((HANDLE)registeredSocket->socket,
                               el->completionPort,
                               (ULONG_PTR)&registeredSocket->source, 0);
    if(result != el->completionPort)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

void
UA_IOCP_prepareOperation(UA_IOCPOperation *operation) {
    UA_assert(operation && operation->owner && !operation->submitted);
    memset(&operation->overlapped, 0, sizeof(OVERLAPPED));
    operation->submitted = true;
    operation->owner->outstandingOperations++;
    operation->owner->manager->eventLoop->outstandingOperations++;
}

void
UA_IOCP_abortOperation(UA_IOCPOperation *operation) {
    UA_assert(operation && operation->submitted);
    operation->submitted = false;
    UA_assert(operation->owner->outstandingOperations > 0);
    UA_assert(operation->owner->manager->eventLoop->outstandingOperations > 0);
    operation->owner->outstandingOperations--;
    operation->owner->manager->eventLoop->outstandingOperations--;
}

void
UA_IOCP_postControl(UA_EventLoopWIN32 *el, UA_UInt32 controlBit) {
    if(!el || !el->completionPort)
        return;
    if(!setAtomicBit(&el->pendingControlPackets, controlBit))
        return;

    if(!PostQueuedCompletionStatus(el->completionPort, controlBit,
                                   (ULONG_PTR)&el->controlSource.source, NULL))
        clearAtomicBit(&el->pendingControlPackets, controlBit);
}

void
UA_EventLoopWIN32_cancel(UA_EventLoop *public_el) {
    UA_IOCP_postControl((UA_EventLoopWIN32*)public_el,
                        UA_IOCP_CONTROL_CANCEL);
}

static void
dispatchCompletion(UA_EventLoopWIN32 *el, OVERLAPPED_ENTRY *entry) {
    UA_IOCPCompletionSource *source =
        (UA_IOCPCompletionSource*)(uintptr_t)entry->lpCompletionKey;

    if(!entry->lpOverlapped) {
        if(!source)
            return;
        if(source->kind == UA_IOCP_SOURCE_CONTROL) {
            clearAtomicBit(&el->pendingControlPackets,
                           entry->dwNumberOfBytesTransferred);
        } else if(source->kind == UA_IOCP_SOURCE_INTERRUPT) {
            UA_InterruptManagerIOCP_dispatch(source->owner,
                                             entry->dwNumberOfBytesTransferred);
        }
        return;
    }

    UA_IOCPOperation *operation =
        (UA_IOCPOperation*)entry->lpOverlapped;
    if(!operation->submitted || !operation->owner)
        return;

    DWORD bytes = entry->dwNumberOfBytesTransferred;
    DWORD flags = 0;
    DWORD error = 0;
    if(!WSAGetOverlappedResult(operation->owner->socket,
                               &operation->overlapped,
                               &bytes, FALSE, &flags))
        error = (DWORD)WSAGetLastError();

    UA_IOCP_abortOperation(operation);
    if(operation->handler)
        operation->handler(operation, bytes, error);
}

/************************/
/* Event-loop lifecycle */
/************************/

static UA_Boolean
hasDelayedCallbacks(UA_EventLoopWIN32 *el) {
    return (el->delayedHead1 > (UA_DelayedCallback*)0x01 ||
            el->delayedHead2 > (UA_DelayedCallback*)0x01);
}

static void
checkClosed(UA_EventLoopWIN32 *el) {
    UA_LOCK_ASSERT(&el->elMutex);

    for(UA_EventSource *es = el->eventLoop.eventSources; es; es = es->next) {
        if(es->state != UA_EVENTSOURCESTATE_STOPPED)
            return;
    }

    if(hasDelayedCallbacks(el) || el->outstandingOperations != 0)
        return;

    /* An external stop first wakes the thread blocked in GQCSEx. */
    if(el->executing && el->eventLoopThreadId != GetCurrentThreadId())
        return;

    if(el->completionPort) {
        CloseHandle(el->completionPort);
        el->completionPort = NULL;
    }

    el->stopping = false;
    *(UA_EventLoopState*)(uintptr_t)&el->eventLoop.state =
        UA_EVENTLOOPSTATE_STOPPED;
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "The Win32 IOCP EventLoop has stopped");
}

static UA_StatusCode
eventLoopStart(UA_EventLoop *public_el) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    UA_LOCK(&el->elMutex);

    if(el->eventLoop.state != UA_EVENTLOOPSTATE_FRESH &&
       el->eventLoop.state != UA_EVENTLOOPSTATE_STOPPED) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    el->completionPort =
        CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    if(!el->completionPort) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    el->controlSource.source.kind = UA_IOCP_SOURCE_CONTROL;
    el->controlSource.source.owner = el;
    el->pendingControlPackets = 0;
    el->outstandingOperations = 0;
    el->stopping = false;

    UA_StatusCode result = UA_STATUSCODE_GOOD;
    for(UA_EventSource *es = el->eventLoop.eventSources; es; es = es->next)
        result |= es->start(es);

    *(UA_EventLoopState*)(uintptr_t)&el->eventLoop.state =
        UA_EVENTLOOPSTATE_STARTED;
    UA_UNLOCK(&el->elMutex);
    return result;
}

static void
eventLoopStop(UA_EventLoop *public_el) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    UA_LOCK(&el->elMutex);

    if(el->eventLoop.state != UA_EVENTLOOPSTATE_STARTED) {
        UA_UNLOCK(&el->elMutex);
        return;
    }

    *(UA_EventLoopState*)(uintptr_t)&el->eventLoop.state =
        UA_EVENTLOOPSTATE_STOPPING;
    el->stopping = true;

    for(UA_EventSource *es = el->eventLoop.eventSources; es; es = es->next) {
        if(es->state == UA_EVENTSOURCESTATE_STARTING ||
           es->state == UA_EVENTSOURCESTATE_STARTED)
            es->stop(es);
    }

    if(el->executing && el->eventLoopThreadId != GetCurrentThreadId())
        UA_IOCP_postControl(el, UA_IOCP_CONTROL_CANCEL);
    checkClosed(el);
    UA_UNLOCK(&el->elMutex);
}

static DWORD
timeoutToMilliseconds(UA_DateTime timeout) {
    if(timeout <= 0)
        return 0;
    UA_DateTime milliseconds =
        (timeout + UA_DATETIME_MSEC - 1) / UA_DATETIME_MSEC;
    if(milliseconds >= (UA_DateTime)INFINITE)
        return INFINITE - 1;
    return (DWORD)milliseconds;
}

static UA_StatusCode
eventLoopRun(UA_EventLoop *public_el, UA_UInt32 timeoutMs) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    UA_LOCK(&el->elMutex);

    if(el->executing ||
       (el->eventLoop.state != UA_EVENTLOOPSTATE_STARTED &&
        el->eventLoop.state != UA_EVENTLOOPSTATE_STOPPING)) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    el->executing = true;
    el->eventLoopThreadId = GetCurrentThreadId();

    UA_DateTime now =
        el->eventLoop.dateTime_nowMonotonic(&el->eventLoop);
    UA_DateTime next = UA_Timer_process(&el->timer, now);
    UA_EventLoopWIN32_processDelayed(el);

    if(el->eventLoop.state == UA_EVENTLOOPSTATE_STOPPING) {
        checkClosed(el);
        if(!el->completionPort) {
            el->eventLoopThreadId = 0;
            el->executing = false;
            UA_UNLOCK(&el->elMutex);
            return UA_STATUSCODE_GOOD;
        }
    }

    now = el->eventLoop.dateTime_nowMonotonic(&el->eventLoop);
    UA_DateTime maximum = (UA_DateTime)timeoutMs * UA_DATETIME_MSEC;
    UA_DateTime untilTimer = next - now;
    if(untilTimer < 0)
        untilTimer = 0;
    UA_DateTime waitDuration =
        (untilTimer < maximum) ? untilTimer : maximum;
    if(hasDelayedCallbacks(el))
        waitDuration = 0;
    DWORD waitMs = timeoutToMilliseconds(waitDuration);

    OVERLAPPED_ENTRY entries[UA_IOCP_COMPLETION_BATCH];
    ULONG entryCount = 0;
    UA_UNLOCK(&el->elMutex);
    BOOL ok = GetQueuedCompletionStatusEx(el->completionPort, entries,
                                          UA_IOCP_COMPLETION_BATCH,
                                          &entryCount, waitMs, FALSE);
    DWORD waitError = ok ? 0 : GetLastError();
    UA_LOCK(&el->elMutex);

    UA_StatusCode result = UA_STATUSCODE_GOOD;
    if(!ok && waitError != WAIT_TIMEOUT) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                       "GetQueuedCompletionStatusEx failed with %lu",
                       (unsigned long)waitError);
        result = UA_STATUSCODE_BADINTERNALERROR;
    }

    for(ULONG i = 0; i < entryCount; i++)
        dispatchCompletion(el, &entries[i]);
    UA_EventLoopWIN32_processDelayed(el);

    if(el->eventLoop.state == UA_EVENTLOOPSTATE_STOPPING)
        checkClosed(el);

    el->eventLoopThreadId = 0;
    el->executing = false;
    UA_UNLOCK(&el->elMutex);
    return result;
}

/*****************************/
/* Event-source registration */
/*****************************/

UA_StatusCode
UA_EventLoopWIN32_registerEventSource(UA_EventLoop *public_el,
                                      UA_EventSource *es) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    UA_LOCK(&el->elMutex);

    if(es->state != UA_EVENTSOURCESTATE_FRESH) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    es->next = el->eventLoop.eventSources;
    el->eventLoop.eventSources = es;
    es->eventLoop = public_el;
    es->state = UA_EVENTSOURCESTATE_STOPPED;

    UA_StatusCode result = UA_STATUSCODE_GOOD;
    if(el->eventLoop.state == UA_EVENTLOOPSTATE_STARTED)
        result = es->start(es);

    UA_UNLOCK(&el->elMutex);
    return result;
}

UA_StatusCode
UA_EventLoopWIN32_deregisterEventSource(UA_EventLoop *public_el,
                                        UA_EventSource *es) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    UA_LOCK(&el->elMutex);

    if(es->state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_EventSource **cursor = &el->eventLoop.eventSources;
    while(*cursor && *cursor != es)
        cursor = &(*cursor)->next;
    if(*cursor == es)
        *cursor = es->next;
    es->state = UA_EVENTSOURCESTATE_FRESH;
    es->eventLoop = NULL;

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static UA_DateTime
dateTimeNow(UA_EventLoop *el) {
    (void)el;
    return UA_DateTime_now();
}

static UA_DateTime
dateTimeNowMonotonic(UA_EventLoop *el) {
    (void)el;
    return UA_DateTime_nowMonotonic();
}

static UA_Int64
dateTimeLocalTimeUtcOffset(UA_EventLoop *el) {
    (void)el;
    return UA_DateTime_localTimeUtcOffset();
}

void
UA_EventLoopWIN32_lock(UA_EventLoop *public_el) {
    UA_LOCK(&((UA_EventLoopWIN32*)public_el)->elMutex);
}

void
UA_EventLoopWIN32_unlock(UA_EventLoop *public_el) {
    UA_UNLOCK(&((UA_EventLoopWIN32*)public_el)->elMutex);
}

static UA_StatusCode
eventLoopFree(UA_EventLoop *public_el) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    UA_LOCK(&el->elMutex);

    if(el->eventLoop.state != UA_EVENTLOOPSTATE_STOPPED &&
       el->eventLoop.state != UA_EVENTLOOPSTATE_FRESH) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    while(el->eventLoop.eventSources) {
        UA_EventSource *es = el->eventLoop.eventSources;
        UA_EventLoopWIN32_deregisterEventSource(public_el, es);
        es->free(es);
    }

    UA_Timer_clear(&el->timer);
    UA_EventLoopWIN32_processDelayed(el);
    if(el->completionPort)
        CloseHandle(el->completionPort);
    UA_KeyValueMap_clear(&el->eventLoop.params);
    if(el->winsockStarted)
        WSACleanup();

    UA_UNLOCK(&el->elMutex);
    UA_LOCK_DESTROY(&el->elMutex);
    UA_free(el);
    return UA_STATUSCODE_GOOD;
}

UA_EventLoop *
UA_EventLoop_new_WIN32(const UA_Logger *logger) {
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return NULL;

    UA_EventLoopWIN32 *el =
        (UA_EventLoopWIN32*)UA_calloc(1, sizeof(UA_EventLoopWIN32));
    if(!el) {
        WSACleanup();
        return NULL;
    }

    el->winsockStarted = true;
    UA_LOCK_INIT(&el->elMutex);
    UA_Timer_init(&el->timer);
    el->delayedTail = &el->delayedHead1;
    el->delayedHead2 = (UA_DelayedCallback*)0x01;

    el->eventLoop.logger = logger;
    el->eventLoop.start = eventLoopStart;
    el->eventLoop.stop = eventLoopStop;
    el->eventLoop.free = eventLoopFree;
    el->eventLoop.run = eventLoopRun;
    el->eventLoop.cancel = UA_EventLoopWIN32_cancel;

    el->eventLoop.dateTime_now = dateTimeNow;
    el->eventLoop.dateTime_nowMonotonic = dateTimeNowMonotonic;
    el->eventLoop.dateTime_localTimeUtcOffset = dateTimeLocalTimeUtcOffset;

    el->eventLoop.nextTimer = UA_EventLoopWIN32_nextTimer;
    el->eventLoop.addTimer = UA_EventLoopWIN32_addTimer;
    el->eventLoop.modifyTimer = UA_EventLoopWIN32_modifyTimer;
    el->eventLoop.removeTimer = UA_EventLoopWIN32_removeTimer;
    el->eventLoop.addDelayedCallback = UA_EventLoopWIN32_addDelayedCallback;
    el->eventLoop.removeDelayedCallback =
        UA_EventLoopWIN32_removeDelayedCallback;

    el->eventLoop.registerEventSource =
        UA_EventLoopWIN32_registerEventSource;
    el->eventLoop.deregisterEventSource =
        UA_EventLoopWIN32_deregisterEventSource;
    el->eventLoop.lock = UA_EventLoopWIN32_lock;
    el->eventLoop.unlock = UA_EventLoopWIN32_unlock;

    return &el->eventLoop;
}

/*************************************/
/* Common connection-manager helpers */
/*************************************/

static UA_StatusCode
getUInt32Parameter(UA_KeyValueMap *params, const char *name,
                   UA_UInt32 defaultValue, size_t *result) {
    UA_QualifiedName key = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)name);
    const UA_UInt32 *value =
        (const UA_UInt32*)UA_KeyValueMap_getScalar(params, key,
                                                    &UA_TYPES[UA_TYPES_UINT32]);
    if(value) {
        *result = *value;
        return UA_STATUSCODE_GOOD;
    }

    UA_StatusCode status =
        UA_KeyValueMap_setScalar(params, key, &defaultValue,
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(status == UA_STATUSCODE_GOOD)
        *result = defaultValue;
    return status;
}

void
UA_IOCP_initConnectionManager(UA_ConnectionManagerIOCP *manager,
                              const UA_String eventSourceName,
                              const UA_String protocol) {
    memset(manager, 0, sizeof(*manager));
    manager->cm.eventSource.eventSourceType =
        UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
    UA_String_copy(&eventSourceName, &manager->cm.eventSource.name);
    manager->cm.protocol = protocol;
    manager->cm.allocNetworkBuffer = UA_IOCP_allocNetworkBuffer;
    manager->cm.freeNetworkBuffer = UA_IOCP_freeNetworkBuffer;
}

UA_StatusCode
UA_IOCP_configureConnectionManager(UA_ConnectionManagerIOCP *manager) {
    UA_KeyValueMap *params = &manager->cm.eventSource.params;
    size_t sendBufferSize = 0;

    UA_StatusCode result =
        getUInt32Parameter(params, "recv-bufsize", 65536u,
                           &manager->receiveBufferSize);
    result |= getUInt32Parameter(params, "send-bufsize",
                                 (UA_UInt32)manager->receiveBufferSize,
                                 &sendBufferSize);
    result |= getUInt32Parameter(params, "send-queue-low-watermark",
                                 UA_IOCP_DEFAULT_SEND_QUEUE_LOW,
                                 &manager->sendQueueLowWatermark);
    result |= getUInt32Parameter(params, "send-queue-high-watermark",
                                 UA_IOCP_DEFAULT_SEND_QUEUE_HIGH,
                                 &manager->sendQueueHighWatermark);
    result |= getUInt32Parameter(params, "send-queue-hard-limit",
                                 UA_IOCP_DEFAULT_SEND_QUEUE_HARD,
                                 &manager->sendQueueHardLimit);
    result |= getUInt32Parameter(params, "send-queue-message-limit",
                                 UA_IOCP_DEFAULT_SEND_QUEUE_MESSAGES,
                                 &manager->sendQueueMessageLimit);
    result |= getUInt32Parameter(params, "send-queue-global-limit",
                                 UA_IOCP_DEFAULT_GLOBAL_SEND_QUEUE_HARD,
                                 &manager->globalSendQueueHardLimit);
    if(result != UA_STATUSCODE_GOOD)
        return result;

    if(manager->receiveBufferSize == 0 || sendBufferSize == 0 ||
       manager->sendQueueLowWatermark > manager->sendQueueHighWatermark ||
       manager->sendQueueHighWatermark > manager->sendQueueHardLimit ||
       manager->sendQueueMessageLimit == 0 ||
       manager->sendQueueHardLimit > manager->globalSendQueueHardLimit)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    manager->sendBufferPool.blockCapacity = sendBufferSize;
    manager->sendBufferPool.maxCachedBlocks = UA_IOCP_BUFFER_POOL_BLOCKS;
    manager->eventLoop =
        (UA_EventLoopWIN32*)manager->cm.eventSource.eventLoop;
    return UA_STATUSCODE_GOOD;
}

void
UA_IOCP_clearConnectionManager(UA_ConnectionManagerIOCP *manager) {
    UA_IOCPSendBuffer *buffer = manager->sendBufferPool.freeList;
    while(buffer) {
        UA_IOCPSendBuffer *next = buffer->next;
        UA_free(buffer);
        buffer = next;
    }
    manager->sendBufferPool.freeList = NULL;
    manager->sendBufferPool.cachedBlocks = 0;
    UA_KeyValueMap_clear(&manager->cm.eventSource.params);
    UA_String_clear(&manager->cm.eventSource.name);
}

static UA_IOCPSendBuffer *
allocateSendBuffer(UA_ConnectionManagerIOCP *manager, size_t size) {
    UA_IOCPSendBuffer *buffer = NULL;
    while(manager->sendBufferPool.freeList &&
          manager->sendBufferPool.freeList->capacity < size) {
        buffer = manager->sendBufferPool.freeList;
        manager->sendBufferPool.freeList = buffer->next;
        manager->sendBufferPool.cachedBlocks--;
        UA_free(buffer);
    }
    buffer = NULL;
    if(size <= manager->sendBufferPool.blockCapacity &&
       manager->sendBufferPool.freeList) {
        buffer = manager->sendBufferPool.freeList;
        manager->sendBufferPool.freeList = buffer->next;
        manager->sendBufferPool.cachedBlocks--;
        buffer->kind = UA_IOCP_BUFFER_POOLED;
    } else {
        size_t capacity = size;
        UA_IOCPBufferKind kind = UA_IOCP_BUFFER_HEAP;
        if(size <= manager->sendBufferPool.blockCapacity) {
            capacity = manager->sendBufferPool.blockCapacity;
            kind = UA_IOCP_BUFFER_POOLED;
        }
        if(capacity > SIZE_MAX - offsetof(UA_IOCPSendBuffer, data))
            return NULL;
        buffer = (UA_IOCPSendBuffer*)
            UA_malloc(offsetof(UA_IOCPSendBuffer, data) + capacity);
        if(!buffer)
            return NULL;
        buffer->capacity = capacity;
        buffer->kind = kind;
    }

    buffer->next = NULL;
    buffer->owner = manager;
    buffer->length = size;
    buffer->magic = UA_IOCP_SEND_BUFFER_MAGIC;
    return buffer;
}

UA_StatusCode
UA_IOCP_allocNetworkBuffer(UA_ConnectionManager *cm,
                           uintptr_t connectionId,
                           UA_ByteString *buf, size_t bufSize) {
    (void)connectionId;
    if(!cm || !buf)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_ConnectionManagerIOCP *manager = (UA_ConnectionManagerIOCP*)cm;
    UA_EventLoopWIN32 *el =
        (UA_EventLoopWIN32*)cm->eventSource.eventLoop;
    if(el)
        UA_LOCK(&el->elMutex);
    UA_IOCPSendBuffer *buffer = allocateSendBuffer(manager, bufSize);
    if(el)
        UA_UNLOCK(&el->elMutex);
    if(!buffer)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    buf->data = buffer->data;
    buf->length = bufSize;
    return UA_STATUSCODE_GOOD;
}

UA_IOCPSendBuffer *
UA_IOCP_takeNetworkBuffer(UA_ConnectionManagerIOCP *manager,
                          UA_ByteString *buf) {
    if(!buf || !buf->data)
        return NULL;

    UA_IOCPSendBuffer *buffer = (UA_IOCPSendBuffer*)
        (buf->data - offsetof(UA_IOCPSendBuffer, data));
    if(buffer->magic != UA_IOCP_SEND_BUFFER_MAGIC ||
       buffer->owner != manager ||
       buffer->data != buf->data ||
       buf->length > buffer->capacity)
        return NULL;

    buffer->length = buf->length;
    UA_ByteString_init(buf);
    return buffer;
}

void
UA_IOCP_releaseSendBuffer(UA_ConnectionManagerIOCP *manager,
                          UA_IOCPSendBuffer *buffer) {
    if(!buffer)
        return;
    UA_assert(buffer->owner == manager);
    buffer->magic = 0;

    if(buffer->kind == UA_IOCP_BUFFER_POOLED &&
       manager->sendBufferPool.cachedBlocks <
           manager->sendBufferPool.maxCachedBlocks) {
        buffer->magic = UA_IOCP_SEND_BUFFER_MAGIC;
        buffer->next = manager->sendBufferPool.freeList;
        manager->sendBufferPool.freeList = buffer;
        manager->sendBufferPool.cachedBlocks++;
        return;
    }
    UA_free(buffer);
}

void
UA_IOCP_freeNetworkBuffer(UA_ConnectionManager *cm,
                          uintptr_t connectionId,
                          UA_ByteString *buf) {
    (void)connectionId;
    if(!cm || !buf || !buf->data)
        return;

    UA_ConnectionManagerIOCP *manager = (UA_ConnectionManagerIOCP*)cm;
    UA_EventLoopWIN32 *el =
        (UA_EventLoopWIN32*)cm->eventSource.eventLoop;
    if(el)
        UA_LOCK(&el->elMutex);
    UA_IOCPSendBuffer *buffer = UA_IOCP_takeNetworkBuffer(manager, buf);
    if(buffer)
        UA_IOCP_releaseSendBuffer(manager, buffer);
    else
        UA_ByteString_init(buf);
    if(el)
        UA_UNLOCK(&el->elMutex);
}

UA_RegisteredSocketIOCP *
UA_IOCP_findSocket(UA_ConnectionManagerIOCP *manager,
                   uintptr_t connectionId) {
    SOCKET socket = (SOCKET)connectionId;
    UA_RegisteredSocketIOCP *registeredSocket =
        ZIP_FIND(UA_IOCPSocketTree, &manager->sockets, &socket);
    if(registeredSocket &&
       registeredSocket->state != UA_IOCP_SOCKET_CLOSING)
        return registeredSocket;
    return NULL;
}

UA_StatusCode
UA_IOCP_insertSocket(UA_ConnectionManagerIOCP *manager,
                     UA_RegisteredSocketIOCP *registeredSocket,
                     UA_IOCPSocketState state, SOCKET socket,
                     UA_ConnectionManager_connectionCallback callback,
                     void *application, void *context) {
    registeredSocket->source.kind = UA_IOCP_SOURCE_SOCKET;
    registeredSocket->source.owner = registeredSocket;
    registeredSocket->manager = manager;
    registeredSocket->socket = socket;
    registeredSocket->state = state;
    registeredSocket->callback = callback;
    registeredSocket->application = application;
    registeredSocket->context = context;

    UA_StatusCode result =
        UA_IOCP_associateSocket(manager->eventLoop, registeredSocket);
    if(result != UA_STATUSCODE_GOOD)
        return result;

    ZIP_INSERT(UA_IOCPSocketTree, &manager->sockets, registeredSocket);
    manager->socketCount++;
    return UA_STATUSCODE_GOOD;
}

void
UA_IOCP_removeSocket(UA_ConnectionManagerIOCP *manager,
                     UA_RegisteredSocketIOCP *registeredSocket) {
    ZIP_REMOVE(UA_IOCPSocketTree, &manager->sockets, registeredSocket);
    UA_assert(manager->socketCount > 0);
    manager->socketCount--;
}

void
UA_IOCP_cancelSocket(UA_RegisteredSocketIOCP *registeredSocket) {
    if(registeredSocket->socket != INVALID_SOCKET)
        CancelIoEx((HANDLE)registeredSocket->socket, NULL);
}

void
UA_IOCP_maybeQueueClose(UA_RegisteredSocketIOCP *registeredSocket) {
    if(registeredSocket->state != UA_IOCP_SOCKET_CLOSING ||
       registeredSocket->outstandingOperations != 0 ||
       registeredSocket->delayedCloseQueued)
        return;

    registeredSocket->delayedCloseQueued = true;
    UA_EventLoopWIN32_addDelayedCallback(
        &registeredSocket->manager->eventLoop->eventLoop,
        &registeredSocket->delayedClose);
}
