/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. */

#include "eventloop_iocp.h"

#include <signal.h>

#define UA_IOCP_SIGNAL_SLOTS 128
#define UA_IOCP_INTERRUPT_MANAGER_SLOTS 8

typedef struct UA_IOCPRegisteredSignal {
    LIST_ENTRY(UA_IOCPRegisteredSignal) listEntry;
    UA_InterruptCallback callback;
    void *context;
    int signal;
    UA_Boolean active;
} UA_IOCPRegisteredSignal;

typedef struct UA_InterruptManagerIOCP {
    UA_InterruptManager interruptManager;
    UA_IOCPCompletionSource source;
    UA_EventLoopWIN32 *eventLoop;
    LIST_HEAD(, UA_IOCPRegisteredSignal) signals;

    HANDLE wakeEvent;
    HANDLE bridgeThread;
    volatile LONG stopBridge;
    volatile LONG pending[UA_IOCP_SIGNAL_SLOTS];
    volatile LONG packetPosted[UA_IOCP_SIGNAL_SLOTS];
    volatile LONG queuedPackets;

    volatile LONG active;
    struct UA_InterruptManagerIOCP * volatile *managerSlot;
} UA_InterruptManagerIOCP;

static UA_InterruptManagerIOCP * volatile
    interruptManagers[UA_IOCP_INTERRUPT_MANAGER_SLOTS];
static volatile LONG signalRefCounts[UA_IOCP_SIGNAL_SLOTS];
static void (*previousActions[UA_IOCP_SIGNAL_SLOTS])(int);
static volatile LONG consoleHandlerRefCount;
static volatile LONG signalRoutingHandlers;
static SRWLOCK signalRegistrationLock = SRWLOCK_INIT;

static UA_StatusCode
startBridge(UA_InterruptManagerIOCP *manager);

static void
stopRouting(UA_InterruptManagerIOCP *manager);

static UA_Boolean
signalIsSupported(int signalNumber) {
    return signalNumber > 0 && signalNumber < UA_IOCP_SIGNAL_SLOTS;
}

static UA_InterruptManagerIOCP *
atomicLoadManager(UA_InterruptManagerIOCP * volatile *slot) {
    return (UA_InterruptManagerIOCP*)InterlockedCompareExchangePointer(
        (PVOID volatile*)slot, NULL, NULL);
}

static UA_Boolean
publishManager(UA_InterruptManagerIOCP *manager) {
    for(size_t i = 0; i < UA_IOCP_INTERRUPT_MANAGER_SLOTS; i++) {
        PVOID previous = InterlockedCompareExchangePointer(
            (PVOID volatile*)&interruptManagers[i], manager, NULL);
        if(previous == NULL) {
            manager->managerSlot = &interruptManagers[i];
            return true;
        }
    }
    return false;
}

static void
unpublishManager(UA_InterruptManagerIOCP *manager) {
    if(!manager->managerSlot)
        return;
    InterlockedCompareExchangePointer(
        (PVOID volatile*)manager->managerSlot, NULL, manager);
    manager->managerSlot = NULL;
}

static UA_Boolean
routeSignal(int signalNumber) {
    if(!signalIsSupported(signalNumber))
        return false;

    /* Protect published manager pointers against concurrent reclamation. */
    InterlockedIncrement(&signalRoutingHandlers);
    UA_Boolean routed = false;
    for(size_t i = 0; i < UA_IOCP_INTERRUPT_MANAGER_SLOTS; i++) {
        UA_InterruptManagerIOCP *manager =
            atomicLoadManager(&interruptManagers[i]);
        if(manager &&
           atomicLoadManager(&interruptManagers[i]) == manager &&
           InterlockedCompareExchange(&manager->active, 0, 0) != 0) {
            InterlockedIncrement(&manager->pending[signalNumber]);
            SetEvent(manager->wakeEvent);
            routed = true;
        }
    }
    InterlockedDecrement(&signalRoutingHandlers);
    return routed;
}

static void
crtSignalHandler(int signalNumber) {
    signal(signalNumber, crtSignalHandler);
    routeSignal(signalNumber);
}

static BOOL WINAPI
consoleControlHandler(DWORD controlType) {
    int signalNumber = 0;
    if(controlType == CTRL_C_EVENT)
        signalNumber = SIGINT;
    else if(controlType == CTRL_BREAK_EVENT)
        signalNumber = SIGBREAK;
    else
        return FALSE;
    return routeSignal(signalNumber) ? TRUE : FALSE;
}

static UA_Boolean
isConsoleSignal(int signalNumber) {
    return (signalNumber == SIGINT || signalNumber == SIGBREAK);
}

static UA_StatusCode
installSignalHandler(int signalNumber) {
    AcquireSRWLockExclusive(&signalRegistrationLock);
    LONG references = InterlockedIncrement(
        &signalRefCounts[signalNumber]);
    if(references != 1) {
        ReleaseSRWLockExclusive(&signalRegistrationLock);
        return UA_STATUSCODE_GOOD;
    }

    void (*previous)(int) = signal(signalNumber, crtSignalHandler);
    if(previous == SIG_ERR) {
        InterlockedDecrement(&signalRefCounts[signalNumber]);
        ReleaseSRWLockExclusive(&signalRegistrationLock);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    UA_StatusCode result = UA_STATUSCODE_GOOD;
    if(isConsoleSignal(signalNumber) &&
       InterlockedIncrement(&consoleHandlerRefCount) == 1 &&
       !SetConsoleCtrlHandler(consoleControlHandler, TRUE)) {
        InterlockedDecrement(&consoleHandlerRefCount);
        signal(signalNumber, previous);
        InterlockedDecrement(&signalRefCounts[signalNumber]);
        result = UA_STATUSCODE_BADINTERNALERROR;
    } else {
        previousActions[signalNumber] = previous;
    }

    ReleaseSRWLockExclusive(&signalRegistrationLock);
    return result;
}

static void
removeSignalHandler(int signalNumber) {
    AcquireSRWLockExclusive(&signalRegistrationLock);
    LONG references = InterlockedDecrement(
        &signalRefCounts[signalNumber]);
    UA_assert(references >= 0);
    if(references == 0) {
        if(isConsoleSignal(signalNumber) &&
           InterlockedDecrement(&consoleHandlerRefCount) == 0)
            SetConsoleCtrlHandler(consoleControlHandler, FALSE);
        signal(signalNumber, previousActions[signalNumber]);
        previousActions[signalNumber] = SIG_DFL;
    }
    ReleaseSRWLockExclusive(&signalRegistrationLock);
}

static UA_StatusCode
activateSignal(UA_IOCPRegisteredSignal *registeredSignal) {
    if(registeredSignal->active)
        return UA_STATUSCODE_GOOD;
    UA_StatusCode result =
        installSignalHandler(registeredSignal->signal);
    if(result == UA_STATUSCODE_GOOD)
        registeredSignal->active = true;
    return result;
}

static void
deactivateSignal(UA_IOCPRegisteredSignal *registeredSignal) {
    if(!registeredSignal->active)
        return;
    registeredSignal->active = false;
    removeSignalHandler(registeredSignal->signal);
}

static DWORD WINAPI
interruptBridge(void *context) {
    UA_InterruptManagerIOCP *manager =
        (UA_InterruptManagerIOCP*)context;

    for(;;) {
        DWORD waitResult = WaitForSingleObject(manager->wakeEvent, INFINITE);
        if(waitResult != WAIT_OBJECT_0)
            break;
        ResetEvent(manager->wakeEvent);
        if(InterlockedCompareExchange(&manager->stopBridge, 0, 0) != 0)
            break;

        for(int signalNumber = 1;
            signalNumber < UA_IOCP_SIGNAL_SLOTS; signalNumber++) {
            if(InterlockedCompareExchange(
                   &manager->pending[signalNumber], 0, 0) == 0)
                continue;
            if(InterlockedCompareExchange(
                   &manager->packetPosted[signalNumber], 1, 0) != 0)
                continue;

            InterlockedIncrement(&manager->queuedPackets);
            if(!PostQueuedCompletionStatus(
                   manager->eventLoop->completionPort,
                   (DWORD)signalNumber,
                   (ULONG_PTR)&manager->source, NULL)) {
                InterlockedDecrement(&manager->queuedPackets);
                InterlockedExchange(
                    &manager->packetPosted[signalNumber], 0);
                if(InterlockedCompareExchange(
                       &manager->active, 0, 0) != 0)
                    SetEvent(manager->wakeEvent);
            }
        }
    }
    return 0;
}

static UA_IOCPRegisteredSignal *
findSignal(UA_InterruptManagerIOCP *manager, int signalNumber) {
    UA_IOCPRegisteredSignal *registeredSignal;
    LIST_FOREACH(registeredSignal, &manager->signals, listEntry) {
        if(registeredSignal->signal == signalNumber)
            return registeredSignal;
    }
    return NULL;
}

static void
maybeStopped(UA_InterruptManagerIOCP *manager) {
    if(manager->interruptManager.eventSource.state ==
           UA_EVENTSOURCESTATE_STOPPING &&
       manager->bridgeThread == NULL &&
       InterlockedCompareExchange(&manager->queuedPackets, 0, 0) == 0) {
        if(manager->wakeEvent) {
            CloseHandle(manager->wakeEvent);
            manager->wakeEvent = NULL;
        }
        manager->interruptManager.eventSource.state =
            UA_EVENTSOURCESTATE_STOPPED;
    }
}

void
UA_InterruptManagerIOCP_dispatch(void *interruptManager,
                                 DWORD signalValue) {
    UA_InterruptManagerIOCP *manager =
        (UA_InterruptManagerIOCP*)interruptManager;
    if(!manager || signalValue >= UA_IOCP_SIGNAL_SLOTS)
        return;

    int signalNumber = (int)signalValue;
    LONG count = InterlockedExchange(
        &manager->pending[signalNumber], 0);
    InterlockedExchange(&manager->packetPosted[signalNumber], 0);
    if(InterlockedCompareExchange(
           &manager->pending[signalNumber], 0, 0) != 0)
        SetEvent(manager->wakeEvent);
    InterlockedDecrement(&manager->queuedPackets);

    if(InterlockedCompareExchange(&manager->active, 0, 0) != 0) {
        for(LONG i = 0; i < count; i++) {
            UA_IOCPRegisteredSignal *registeredSignal =
                findSignal(manager, signalNumber);
            if(!registeredSignal || !registeredSignal->active)
                break;
            registeredSignal->callback(
                &manager->interruptManager,
                (uintptr_t)signalNumber,
                registeredSignal->context,
                &UA_KEYVALUEMAP_NULL);
        }
    }
    maybeStopped(manager);
}

static UA_StatusCode
registerInterrupt(UA_InterruptManager *interruptManager,
                  uintptr_t interruptHandle,
                  const UA_KeyValueMap *params,
                  UA_InterruptCallback callback,
                  void *interruptContext) {
    if(!UA_KeyValueMap_isEmpty(params) || !callback)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_InterruptManagerIOCP *manager =
        (UA_InterruptManagerIOCP*)interruptManager;
    UA_EventLoopWIN32 *el =
        (UA_EventLoopWIN32*)interruptManager->eventSource.eventLoop;
    if(el)
        UA_LOCK(&el->elMutex);

    int signalNumber = (int)interruptHandle;
    if(!signalIsSupported(signalNumber) ||
       findSignal(manager, signalNumber)) {
        if(el)
            UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_IOCPRegisteredSignal *registeredSignal =
        (UA_IOCPRegisteredSignal*)UA_calloc(
            1, sizeof(UA_IOCPRegisteredSignal));
    if(!registeredSignal) {
        if(el)
            UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    registeredSignal->callback = callback;
    registeredSignal->context = interruptContext;
    registeredSignal->signal = signalNumber;
    LIST_INSERT_HEAD(&manager->signals,
                     registeredSignal, listEntry);

    UA_StatusCode result = UA_STATUSCODE_GOOD;
    if(interruptManager->eventSource.state ==
           UA_EVENTSOURCESTATE_STARTED) {
        UA_Boolean bridgeStarted = (manager->bridgeThread != NULL);
        result = startBridge(manager);
        if(result == UA_STATUSCODE_GOOD)
            result = activateSignal(registeredSignal);
        if(result != UA_STATUSCODE_GOOD) {
            if(!bridgeStarted && manager->bridgeThread) {
                stopRouting(manager);
                if(manager->wakeEvent) {
                    CloseHandle(manager->wakeEvent);
                    manager->wakeEvent = NULL;
                }
            }
            LIST_REMOVE(registeredSignal, listEntry);
            UA_free(registeredSignal);
        }
    }

    if(el)
        UA_UNLOCK(&el->elMutex);
    return result;
}

static void
deregisterInterrupt(UA_InterruptManager *interruptManager,
                    uintptr_t interruptHandle) {
    UA_InterruptManagerIOCP *manager =
        (UA_InterruptManagerIOCP*)interruptManager;
    UA_EventLoopWIN32 *el =
        (UA_EventLoopWIN32*)interruptManager->eventSource.eventLoop;
    if(el)
        UA_LOCK(&el->elMutex);

    UA_IOCPRegisteredSignal *registeredSignal =
        findSignal(manager, (int)interruptHandle);
    if(registeredSignal) {
        deactivateSignal(registeredSignal);
        LIST_REMOVE(registeredSignal, listEntry);
        UA_free(registeredSignal);
    }

    if(el)
        UA_UNLOCK(&el->elMutex);
}

static void
stopBridge(UA_InterruptManagerIOCP *manager) {
    if(!manager->bridgeThread)
        return;
    InterlockedExchange(&manager->stopBridge, 1);
    SetEvent(manager->wakeEvent);
    WaitForSingleObject(manager->bridgeThread, INFINITE);
    CloseHandle(manager->bridgeThread);
    manager->bridgeThread = NULL;
}

static UA_StatusCode
startBridge(UA_InterruptManagerIOCP *manager) {
    if(manager->bridgeThread)
        return UA_STATUSCODE_GOOD;

    manager->wakeEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if(!manager->wakeEvent)
        return UA_STATUSCODE_BADINTERNALERROR;

    InterlockedExchange(&manager->stopBridge, 0);
    manager->bridgeThread =
        CreateThread(NULL, 0, interruptBridge, manager, 0, NULL);
    if(!manager->bridgeThread) {
        CloseHandle(manager->wakeEvent);
        manager->wakeEvent = NULL;
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    InterlockedExchange(&manager->active, 1);
    if(publishManager(manager))
        return UA_STATUSCODE_GOOD;

    InterlockedExchange(&manager->active, 0);
    stopBridge(manager);
    CloseHandle(manager->wakeEvent);
    manager->wakeEvent = NULL;
    return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
}

static void
stopRouting(UA_InterruptManagerIOCP *manager) {
    InterlockedExchange(&manager->active, 0);
    unpublishManager(manager);
    while(InterlockedCompareExchange(
              &signalRoutingHandlers, 0, 0) != 0)
        Sleep(0);
    stopBridge(manager);
}

static UA_StatusCode
Interrupt_eventSourceStart(UA_EventSource *eventSource) {
    UA_InterruptManagerIOCP *manager =
        (UA_InterruptManagerIOCP*)eventSource;
    UA_EventLoopWIN32 *el =
        (UA_EventLoopWIN32*)eventSource->eventLoop;
    UA_LOCK(&el->elMutex);

    if(eventSource->state != UA_EVENTSOURCESTATE_STOPPED ||
       !el->completionPort) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    manager->eventLoop = el;
    manager->source.kind = UA_IOCP_SOURCE_INTERRUPT;
    manager->source.owner = manager;
    InterlockedExchange(&manager->queuedPackets, 0);
    for(int signalNumber = 0;
        signalNumber < UA_IOCP_SIGNAL_SLOTS; signalNumber++) {
        InterlockedExchange(&manager->pending[signalNumber], 0);
        InterlockedExchange(&manager->packetPosted[signalNumber], 0);
    }
    UA_StatusCode result = UA_STATUSCODE_GOOD;
    UA_IOCPRegisteredSignal *registeredSignal;
    if(!LIST_EMPTY(&manager->signals)) {
        result = startBridge(manager);
        if(result == UA_STATUSCODE_GOOD) {
            LIST_FOREACH(registeredSignal, &manager->signals, listEntry) {
                result = activateSignal(registeredSignal);
                if(result != UA_STATUSCODE_GOOD)
                    break;
            }
        }
    }

    if(result != UA_STATUSCODE_GOOD) {
        LIST_FOREACH(registeredSignal, &manager->signals, listEntry)
            deactivateSignal(registeredSignal);
        stopRouting(manager);
        if(manager->wakeEvent) {
            CloseHandle(manager->wakeEvent);
            manager->wakeEvent = NULL;
        }
        UA_UNLOCK(&el->elMutex);
        return result;
    }

    eventSource->state = UA_EVENTSOURCESTATE_STARTED;
    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static void
Interrupt_eventSourceStop(UA_EventSource *eventSource) {
    UA_InterruptManagerIOCP *manager =
        (UA_InterruptManagerIOCP*)eventSource;
    UA_EventLoopWIN32 *el = manager->eventLoop;
    UA_LOCK(&el->elMutex);

    if(eventSource->state != UA_EVENTSOURCESTATE_STARTED) {
        UA_UNLOCK(&el->elMutex);
        return;
    }

    eventSource->state = UA_EVENTSOURCESTATE_STOPPING;
    UA_IOCPRegisteredSignal *registeredSignal;
    LIST_FOREACH(registeredSignal, &manager->signals, listEntry)
        deactivateSignal(registeredSignal);

    stopRouting(manager);
    maybeStopped(manager);
    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
Interrupt_eventSourceFree(UA_EventSource *eventSource) {
    UA_InterruptManagerIOCP *manager =
        (UA_InterruptManagerIOCP*)eventSource;
    if(eventSource->state != UA_EVENTSOURCESTATE_STOPPED &&
       eventSource->state != UA_EVENTSOURCESTATE_FRESH)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_IOCPRegisteredSignal *registeredSignal;
    UA_IOCPRegisteredSignal *next;
    LIST_FOREACH_SAFE(registeredSignal, &manager->signals,
                      listEntry, next) {
        deactivateSignal(registeredSignal);
        LIST_REMOVE(registeredSignal, listEntry);
        UA_free(registeredSignal);
    }

    if(manager->wakeEvent)
        CloseHandle(manager->wakeEvent);
    UA_KeyValueMap_clear(&eventSource->params);
    UA_String_clear(&eventSource->name);
    UA_free(manager);
    return UA_STATUSCODE_GOOD;
}

UA_InterruptManager *
UA_InterruptManager_new_WIN32(const UA_String eventSourceName) {
    UA_InterruptManagerIOCP *manager =
        (UA_InterruptManagerIOCP*)UA_calloc(
            1, sizeof(UA_InterruptManagerIOCP));
    if(!manager)
        return NULL;

    LIST_INIT(&manager->signals);
    UA_InterruptManager *interruptManager =
        &manager->interruptManager;
    interruptManager->eventSource.eventSourceType =
        UA_EVENTSOURCETYPE_INTERRUPTMANAGER;
    UA_String_copy(&eventSourceName,
                   &interruptManager->eventSource.name);
    interruptManager->eventSource.start = Interrupt_eventSourceStart;
    interruptManager->eventSource.stop = Interrupt_eventSourceStop;
    interruptManager->eventSource.free = Interrupt_eventSourceFree;
    interruptManager->registerInterrupt = registerInterrupt;
    interruptManager->deregisterInterrupt = deregisterInterrupt;
    return interruptManager;
}
