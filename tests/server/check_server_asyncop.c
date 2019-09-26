/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* This example is just to see how fast we can process messages. The server does
   not open a TCP port. */

#include <open62541/server_config_default.h>

#include "server/ua_services.h"
#include "ua_server_internal.h"
#include "ua_server_methodqueue.h"

#include <check.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include "testing_clock.h"

static UA_Server* globalServer;
static UA_Session session;


START_TEST(InternalTestingQueue) {
    globalServer->config.asyncOperationTimeout = 2;
    globalServer->config.maxAsyncOperationQueueSize = 5;
    UA_UInt32 reqId = 1;

    const UA_AsyncOperationRequest* pRequest = NULL;
    void *pContext = NULL;
    UA_AsyncOperationType type;
    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking queue: fetch from empty queue");
    bool rv = UA_Server_getAsyncOperation(globalServer, &type, &pRequest, &pContext);
    ck_assert_int_eq(rv, UA_FALSE);
    
    UA_CallMethodRequest* pRequest1 = UA_CallMethodRequest_new();
    UA_CallMethodRequest_init(pRequest1);
    UA_NodeId id = UA_NODEID_NUMERIC(1, 62540);
    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking queue: create request queue entry");
    UA_StatusCode result = UA_Server_SetNextAsyncMethod(globalServer, reqId++, &id, 0, pRequest1);
    ck_assert_int_eq(result, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(globalServer->nMQCurSize, 1);

    const UA_AsyncOperationRequest* pRequestWorker = NULL;
    void *pContextWorker = NULL;
    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking queue: fetch from queue");
    rv = UA_Server_getAsyncOperation(globalServer, &type, &pRequestWorker, &pContextWorker);
    ck_assert_int_eq(rv, UA_TRUE);
    ck_assert_int_eq(globalServer->nMQCurSize, 0);
        
    UA_AsyncOperationResponse pResponse;
    UA_CallMethodResult_init(&pResponse.callMethodResult);
    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking queue: set result to response queue");
    UA_Server_setAsyncOperationResult(globalServer, &pResponse, pContextWorker);
    UA_CallMethodResult_deleteMembers(&pResponse.callMethodResult);

    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking queue: fetch result from response queue");
    struct AsyncMethodQueueElement* pResponseServer = NULL;
    rv = UA_Server_GetAsyncMethodResult(globalServer, &pResponseServer);
    ck_assert_int_eq(rv, UA_TRUE);
    UA_Server_DeleteMethodQueueElement(globalServer, pResponseServer);
    
    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking queue: testing queue limit (%zu)", globalServer->config.maxAsyncOperationQueueSize);
    UA_UInt32 i;
    for (i = 0; i < globalServer->config.maxAsyncOperationQueueSize + 1; i++) {
        UA_NodeId idTmp = UA_NODEID_NUMERIC(1, 62541);
        UA_StatusCode resultTmp = UA_Server_SetNextAsyncMethod(globalServer, reqId++, &idTmp, 0, pRequest1);
        if (i < globalServer->config.maxAsyncOperationQueueSize)
            ck_assert_int_eq(resultTmp, UA_STATUSCODE_GOOD);
        else
            ck_assert_int_ne(resultTmp, UA_STATUSCODE_GOOD);
    }
    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking queue: queue should not be empty");
    UA_Server_CheckQueueIntegrity(globalServer,NULL);
    ck_assert_int_ne(globalServer->nMQCurSize, 0);
    UA_fakeSleep((UA_Int32)(globalServer->config.asyncOperationTimeout + 1) * 1000);
    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking queue: empty queue caused by queue timeout (%ds)", (UA_Int32)globalServer->config.asyncOperationTimeout);
    /* has to be done twice due to internal queue delete limit */
    UA_Server_CheckQueueIntegrity(globalServer,NULL);
    UA_Server_CheckQueueIntegrity(globalServer,NULL);
    ck_assert_int_eq(globalServer->nMQCurSize, 0);    
    
    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking queue: adding one new entry to empty queue");
    result = UA_Server_SetNextAsyncMethod(globalServer, reqId++, &id, 0, pRequest1);
    ck_assert_int_eq(result, UA_STATUSCODE_GOOD);    
    ck_assert_int_eq(globalServer->nMQCurSize, 1);
    UA_CallMethodRequest_delete(pRequest1);
}
END_TEST


START_TEST(InternalTestingManager) {    
    UA_Session_init(&session);
    session.sessionId = UA_NODEID_NUMERIC(1, 62541);
    UA_SecureChannel channel;
    UA_SecureChannel_init(&channel);
    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking UA_AsyncMethodManager_createEntry: create CallRequests");
    UA_DataType dataType = UA_TYPES[UA_TYPES_CALLMETHODREQUEST];
    for (UA_Int32 i = 1; i < 7; i++) {
        UA_StatusCode result = UA_AsyncMethodManager_createEntry(&globalServer->asyncMethodManager, &session.sessionId,
            channel.securityToken.channelId, i, i, &dataType, 1);
        ck_assert_int_eq(result, UA_STATUSCODE_GOOD);
    }
    UA_fakeSleep(121000);
    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking UA_AsyncMethodManager_createEntry: empty CallRequest list");
    UA_AsyncMethodManager_checkTimeouts(globalServer, &globalServer->asyncMethodManager);
    ck_assert_int_eq(globalServer->asyncMethodManager.currentCount, 0);
}
END_TEST

START_TEST(InternalTestingPendingList) {
    globalServer->config.asyncOperationTimeout = 2;
    globalServer->config.maxAsyncOperationQueueSize = 5;

    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking PendingList");

    struct AsyncMethodQueueElement* elem1 = (struct AsyncMethodQueueElement*)UA_calloc(1, sizeof(struct AsyncMethodQueueElement));
    struct AsyncMethodQueueElement* elem2 = (struct AsyncMethodQueueElement*)UA_calloc(1, sizeof(struct AsyncMethodQueueElement));
    struct AsyncMethodQueueElement* elem3 = (struct AsyncMethodQueueElement*)UA_calloc(1, sizeof(struct AsyncMethodQueueElement));

    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking PendingList: Adding 3 elements");
    UA_Server_AddPendingMethodCall(globalServer, elem1);
    UA_Server_AddPendingMethodCall(globalServer, elem2);
    UA_Server_AddPendingMethodCall(globalServer, elem3);

    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking PendingList: check if element is found");
    UA_Boolean bFound = UA_Server_IsPendingMethodCall(globalServer, elem2);
    if (!bFound) {
        ck_assert_int_eq(bFound, UA_TRUE);
        UA_Server_RmvPendingMethodCall(globalServer, elem2);
    }
    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking PendingList: remove remaining elements");
    UA_Server_RmvPendingMethodCall(globalServer, elem1);
    UA_Server_RmvPendingMethodCall(globalServer, elem3);
    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking PendingList: check if removed element is NOT found");
    bFound = UA_Server_IsPendingMethodCall(globalServer, elem1);
    if (!bFound) {
        ck_assert_int_eq(bFound, UA_FALSE);
        UA_Server_RmvPendingMethodCall(globalServer, elem1);
    }

    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking PendingList: queue integrity");
    UA_Server_AddPendingMethodCall(globalServer, elem1);
    UA_Server_AddPendingMethodCall(globalServer, elem2);
    UA_Server_AddPendingMethodCall(globalServer, elem3);
    UA_fakeSleep((UA_Int32)(globalServer->config.asyncOperationTimeout + 1) * 1000);
    UA_Server_CheckQueueIntegrity(globalServer, NULL);
    ck_assert_ptr_eq(globalServer->ua_method_pending_list.sqh_first,NULL);    

    UA_LOG_INFO(&globalServer->config.logger, UA_LOGCATEGORY_SERVER, "* Checking PendingList: global removal/delete");
    UA_Server_AddPendingMethodCall(globalServer, elem1);
    /* remove all entries and delete queues */
    UA_Server_MethodQueues_delete(globalServer);
    /* re-init queues */    
    UA_Server_MethodQueues_init(globalServer);
}
END_TEST

static void setup(void) {
    globalServer = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(globalServer);
    UA_ServerConfig_setDefault(config);
}

static void teardown(void) {    
    UA_Server_delete(globalServer);
}

static Suite* method_async_suite(void) {
    /* set up unit test for internal data structures */
    Suite *s = suite_create("Async Method");

    /* UA_Server_PendingList */
    TCase* tc_pending = tcase_create("PendingList");
    tcase_add_checked_fixture(tc_pending, setup, NULL);
    tcase_add_test(tc_pending, InternalTestingPendingList);
    suite_add_tcase(s, tc_pending);

    /* UA_AsyncMethodManager */
    TCase* tc_manager = tcase_create("AsyncMethodManager");
    tcase_add_checked_fixture(tc_manager, NULL, NULL);
    tcase_add_test(tc_manager, InternalTestingManager);
    suite_add_tcase(s, tc_manager);
    
    /* UA_Server_MethodQueues */
    TCase* tc_queue = tcase_create("AsyncMethodQueue");
    tcase_add_checked_fixture(tc_queue, NULL, teardown);
    tcase_add_test(tc_queue, InternalTestingQueue);
    suite_add_tcase(s, tc_queue);    
    
    return s;
}


int main(void) {
    /* Unit tests for internal data structures for async methods */
    int number_failed = 0;
    Suite *s = method_async_suite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
