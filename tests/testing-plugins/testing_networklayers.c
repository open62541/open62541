/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testing_networklayers.h"

#define TEST_CM_MAX_CONNS 16

typedef struct {
    uintptr_t connId;
    void *application;
    void *context;
    UA_ConnectionManager_connectionCallback callback;
    size_t rxCount;
    size_t txCount;
} TestCMConnection;

/* TestCM embeds UA_ConnectionManager as its first member so that a
 * UA_ConnectionManager* can be cast directly to TestCM* and back. */
typedef struct TestCM {
    UA_ConnectionManager cm;
    void *context; /* opaque user context, get/set via public API */
    UA_ByteString lastSent; /* last buffer stolen by sendWithConnection */
    UA_StatusCode (*sendWithConnectionOverload)(UA_ConnectionManager *cm,
                                               uintptr_t connectionId,
                                               const UA_KeyValueMap *params,
                                               UA_ByteString *buf);
    TestCMConnection conns[TEST_CM_MAX_CONNS];
    size_t connCount;
    uintptr_t nextConnId;
} TestCM;

void
TestConnectionManager_setContext(UA_ConnectionManager *cm, void *context) {
    ((TestCM *)(void *)cm)->context = context;
}

void *
TestConnectionManager_getContext(UA_ConnectionManager *cm) {
    return ((TestCM *)(void *)cm)->context;
}

const UA_ByteString *
TestConnectionManager_getLastSent(UA_ConnectionManager *cm) {
    return &((TestCM *)(void *)cm)->lastSent;
}

UA_StatusCode
TestConnectionManager_createConnection(UA_ConnectionManager *cm,
                                       void *application,
                                       void *context,
                                       UA_ConnectionManager_connectionCallback connectionCallback,
                                       uintptr_t *outConnectionId) {
    TestCM *tcm = (TestCM *)(void *)cm;
    if(!tcm || !connectionCallback || !outConnectionId)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    size_t slot = TEST_CM_MAX_CONNS;
    for(size_t i = 0; i < TEST_CM_MAX_CONNS; i++) {
        if(!tcm->conns[i].callback) {
            slot = i;
            break;
        }
    }
    if(slot == TEST_CM_MAX_CONNS)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    uintptr_t connId = tcm->nextConnId++;
    tcm->conns[slot].connId = connId;
    tcm->conns[slot].application = application;
    tcm->conns[slot].context = context;
    tcm->conns[slot].callback = connectionCallback;
    tcm->connCount++;
    *outConnectionId = connId;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
TestConnectionManager_removeConnection(UA_ConnectionManager *cm,
                                       uintptr_t connectionId) {
    TestCM *tcm = (TestCM *)(void *)cm;
    if(!tcm)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    for(size_t i = 0; i < TEST_CM_MAX_CONNS; i++) {
        if(!tcm->conns[i].callback || tcm->conns[i].connId != connectionId)
            continue;
        tcm->conns[i].callback = NULL;
        tcm->conns[i].context = NULL;
        tcm->conns[i].application = NULL;
        tcm->connCount--;
        if(tcm->connCount == 0 &&
           tcm->cm.eventSource.state == UA_EVENTSOURCESTATE_STOPPING)
            tcm->cm.eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
        return UA_STATUSCODE_GOOD;
    }
    return UA_STATUSCODE_BADNOTFOUND;
}

UA_StatusCode
TestConnectionManager_inject(UA_ConnectionManager *cm,
                             uintptr_t connectionId,
                             UA_ConnectionState state,
                             const UA_KeyValueMap *params,
                             const UA_ByteString *msg) {
    TestCM *tcm = (TestCM *)(void *)cm;
    if(!tcm)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_KeyValueMap emptyKvm = UA_KEYVALUEMAP_NULL;
    UA_ByteString emptyMsg = UA_BYTESTRING_NULL;
    const UA_KeyValueMap *injectParams = params ? params : &emptyKvm;
    UA_ByteString injectMsg = msg ? *msg : emptyMsg;

    for(size_t i = 0; i < TEST_CM_MAX_CONNS; i++) {
        if(!tcm->conns[i].callback || tcm->conns[i].connId != connectionId)
            continue;

        tcm->conns[i].callback(cm, tcm->conns[i].connId,
                             tcm->conns[i].application,
                             &tcm->conns[i].context,
                             state, injectParams, injectMsg);

        if(injectMsg.length > 0)
            tcm->conns[i].rxCount++;

        if(state == UA_CONNECTIONSTATE_CLOSING ||
           state == UA_CONNECTIONSTATE_CLOSED) {
            tcm->conns[i].callback = NULL;
            tcm->conns[i].context = NULL;
            tcm->conns[i].application = NULL;
            tcm->connCount--;
        }

        return UA_STATUSCODE_GOOD;
    }

    return UA_STATUSCODE_BADNOTFOUND;
}

static UA_StatusCode
testCM_eventSourceStart(UA_EventSource *es) {
    es->state = UA_EVENTSOURCESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void
testCM_eventSourceStop(UA_EventSource *es) {
    UA_ConnectionManager *cm = (UA_ConnectionManager *)es;
    TestCM *tcm = (TestCM *)(void *)cm;
    UA_assert(tcm);

    if(tcm->connCount == 0) {
        es->state = UA_EVENTSOURCESTATE_STOPPED;
        return;
    }

    es->state = UA_EVENTSOURCESTATE_STOPPING;

    UA_KeyValueMap emptyKvm = UA_KEYVALUEMAP_NULL;
    UA_ByteString emptyMsg = UA_BYTESTRING_NULL;
    for(size_t i = 0; i < TEST_CM_MAX_CONNS; i++) {
        if(!tcm->conns[i].callback)
            continue;
        tcm->conns[i].callback(cm, tcm->conns[i].connId,
                                 tcm->conns[i].application,
                                 &tcm->conns[i].context,
                                 UA_CONNECTIONSTATE_CLOSING,
                                 &emptyKvm, emptyMsg);
    }
}

static UA_StatusCode
testCM_eventSourceFree(UA_EventSource *es) {
    UA_ConnectionManager *cm = (UA_ConnectionManager *)es;
    TestCM *tcm = (TestCM *)(void *)cm;
    UA_assert(tcm);

    testCM_eventSourceStop(es);
    UA_ByteString_clear(&tcm->lastSent);
    UA_free(tcm);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
testCM_sendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                          const UA_KeyValueMap *params, UA_ByteString *buf) {
    TestCM *tcm = (TestCM *)(void *)cm;
    /* Update tx counter if the connection is tracked (opportunistic). */
    for(size_t i = 0; i < TEST_CM_MAX_CONNS; i++) {
        if(tcm->conns[i].callback && tcm->conns[i].connId == connectionId) {
            tcm->conns[i].txCount++;
            break;
        }
    }
    if(tcm->sendWithConnectionOverload)
        return tcm->sendWithConnectionOverload(cm, connectionId, params, buf);
    /* Default: steal the buffer into lastSent */
    UA_ByteString_clear(&tcm->lastSent);
    tcm->lastSent = *buf;
    UA_ByteString_init(buf);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
testCM_openConnection(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
                      void *application, void *context,
                      UA_ConnectionManager_connectionCallback cb) {
    (void)params;
    uintptr_t connId;
    UA_StatusCode ret = TestConnectionManager_createConnection(cm, application,
                                                               context, cb, &connId);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;
    UA_KeyValueMap emptyKvm = UA_KEYVALUEMAP_NULL;
    UA_ByteString emptyMsg = UA_BYTESTRING_NULL;
    return TestConnectionManager_inject(cm, connId, UA_CONNECTIONSTATE_ESTABLISHED,
                                        &emptyKvm, &emptyMsg);
}

static UA_StatusCode
testCM_closeConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    UA_KeyValueMap emptyKvm = UA_KEYVALUEMAP_NULL;
    UA_ByteString emptyMsg = UA_BYTESTRING_NULL;
    TestConnectionManager_inject(cm, connectionId, UA_CONNECTIONSTATE_CLOSING,
                                 &emptyKvm, &emptyMsg);
    return TestConnectionManager_removeConnection(cm, connectionId);
}

UA_StatusCode
TestConnectionManager_getCounters(UA_ConnectionManager *cm, uintptr_t connectionId,
                                   size_t *rxCount, size_t *txCount) {
    TestCM *tcm = (TestCM *)(void *)cm;
    for(size_t i = 0; i < TEST_CM_MAX_CONNS; i++) {
        if(!tcm->conns[i].callback || tcm->conns[i].connId != connectionId)
            continue;
        if(rxCount) *rxCount = tcm->conns[i].rxCount;
        if(txCount) *txCount = tcm->conns[i].txCount;
        return UA_STATUSCODE_GOOD;
    }
    return UA_STATUSCODE_BADNOTFOUND;
}

static UA_StatusCode
testCM_allocNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                          UA_ByteString *buf, size_t bufSize) {
    (void)cm;
    (void)connectionId;
    return UA_ByteString_allocBuffer(buf, bufSize);
}

static void
testCM_freeNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                         UA_ByteString *buf) {
    (void)cm;
    (void)connectionId;
    UA_ByteString_clear(buf);
}

UA_ConnectionManager *
TestConnectionManager_new(const char *protocol,
                          const TestConnectionManager_CallbackOverloads *overloads) {
    if(!protocol || !protocol[0])
        return NULL;

    TestCM *tcm = (TestCM*)UA_calloc(1, sizeof(TestCM));
    if(!tcm)
        return NULL;

    tcm->nextConnId = 100;

    UA_ConnectionManager *cm = &tcm->cm;
    cm->eventSource.next = NULL;
    cm->eventSource.eventSourceType = UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
    cm->eventSource.name = UA_STRING((char*)(uintptr_t)"test-cm");
    cm->eventSource.eventLoop = NULL;
    cm->eventSource.params = UA_KEYVALUEMAP_NULL;
    cm->eventSource.state = UA_EVENTSOURCESTATE_FRESH;
    cm->eventSource.start = testCM_eventSourceStart;
    cm->eventSource.stop = testCM_eventSourceStop;
    cm->eventSource.free = testCM_eventSourceFree;
    cm->protocol = UA_STRING((char*)(uintptr_t)protocol);
    cm->openConnection = (overloads && overloads->openConnection) ?
                         overloads->openConnection : testCM_openConnection;
    cm->sendWithConnection = testCM_sendWithConnection;
    cm->closeConnection = (overloads && overloads->closeConnection) ?
                          overloads->closeConnection : testCM_closeConnection;
    tcm->sendWithConnectionOverload = overloads ? overloads->sendWithConnection : NULL;
    cm->allocNetworkBuffer = testCM_allocNetworkBuffer;
    cm->freeNetworkBuffer = testCM_freeNetworkBuffer;

    return cm;
}
