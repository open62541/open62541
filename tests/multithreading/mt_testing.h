#ifndef OPEN62541_MT_TESTING_H
#define OPEN62541_MT_TESTING_H

#include <open62541/server_config_default.h>
#include "test_helpers.h"

typedef struct {
    void (*func)(void *param); //function to execute
    size_t counter; //index of the iteration
    size_t index; // index within workerContext array of global TestContext
    size_t upperBound; //number of iterations each thread schould execute func
    THREAD_HANDLE handle;
} ThreadContext;

typedef struct {
    size_t numberOfWorkers;
    ThreadContext *workerContext;
    size_t numberofClients;
    ThreadContext *clientContext;
    UA_Boolean running;
    UA_Server *server;
    UA_Client **clients;
    void (*checkServerNodes)(void);
} TestContext;

TestContext tc;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(tc.running)
        UA_Server_run_iterate(tc.server, true);
    return 0;
}

static
void teardown(void) {
    for(size_t i = 0; i < tc.numberOfWorkers; i++)
        THREAD_JOIN(tc.workerContext[i].handle);

    for(size_t i = 0; i < tc.numberofClients; i++)
        THREAD_JOIN(tc.clientContext[i].handle);

    tc.running = false;
    THREAD_JOIN(server_thread);
    if(tc.checkServerNodes)
        tc.checkServerNodes();
    UA_Server_run_shutdown(tc.server);
    UA_Server_delete(tc.server);
}

THREAD_CALLBACK_PARAM(workerLoop, val) {
    ThreadContext tmp = (*(ThreadContext *) val);
    for(size_t i = 0; i < tmp.upperBound; i++) {
        tmp.counter = i;
        tmp.func(&tmp);
    }
    return 0;
}

THREAD_CALLBACK_PARAM(clientLoop, val) {
    ThreadContext tmp = (*(ThreadContext *) val);
    tc.clients[tmp.index] = UA_Client_newForUnitTest();
    UA_ClientConfig_setDefault(UA_Client_getConfig(tc.clients[tmp.index]));
    UA_StatusCode retval = UA_Client_connect(tc.clients[tmp.index], "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    for(size_t i = 0; i < tmp.upperBound; i++) {
        tmp.counter = i;
        tmp.func(&tmp);
    }
    UA_Client_disconnect(tc.clients[tmp.index]);
    UA_Client_delete(tc.clients[tmp.index]);
    return 0;
}

static UA_INLINE void
createThreadContext(size_t numberOfWorkers, size_t numberOfClients,
                  void (*checkServerNodes)(void)) {
    tc.numberOfWorkers = numberOfWorkers;
    tc.numberofClients = numberOfClients;
    tc.checkServerNodes = checkServerNodes;
    tc.workerContext = (ThreadContext*) UA_calloc(tc.numberOfWorkers, sizeof(ThreadContext));
    tc.clients =  (UA_Client**) UA_calloc(tc.numberofClients, sizeof(UA_Client*));
    tc.clientContext = (ThreadContext*) UA_calloc(tc.numberofClients, sizeof(ThreadContext));
}

static UA_INLINE void
deleteThreadContext(void) {
    if(tc.workerContext)
        UA_free(tc.workerContext);
    if(tc.clients)
        UA_free(tc.clients);
    if(tc.clientContext)
        UA_free(tc.clientContext);
    memset(&tc, 0, sizeof(tc));
}

static UA_INLINE void
setThreadContext(ThreadContext *workerContext, size_t index, size_t upperBound,
                 void (*func)(void *param)) {
    workerContext->index = index;
    workerContext->upperBound = upperBound;
    workerContext->func = func;
}

static UA_INLINE void
startMultithreading(void) {
    for(size_t i = 0; i < tc.numberOfWorkers; i++)
        THREAD_CREATE_PARAM(tc.workerContext[i].handle, workerLoop, tc.workerContext[i]);

    for(size_t i = 0; i < tc.numberofClients; i++)
        THREAD_CREATE_PARAM(tc.clientContext[i].handle, clientLoop, tc.clientContext[i]);
}

#endif /* OPEN62541_MT_TESTING_H */
