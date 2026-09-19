/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

#include <open62541/client_subscriptions.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>
#include "server/ua_server_internal.h"
#include "server/ua_subscription.h"

#include <check.h>
#include <stdlib.h>

/* On libcheck < 0.11 (ubuntu-20.04 ships 0.10), ck_assert_ptr_null /
 * ck_assert_ptr_nonnull are missing. Shim them to ck_assert_msg. */
#ifndef ck_assert_ptr_null
# define ck_assert_ptr_null(p) ck_assert_msg((p) == NULL, #p " is not NULL")
#endif
#ifndef ck_assert_ptr_nonnull
# define ck_assert_ptr_nonnull(p) ck_assert_msg((p) != NULL, #p " is NULL")
#endif

#include "test_helpers.h"
#include "testing_clock.h"
#include "testing_networklayers.h"

#ifdef UA_ENABLE_STATUSCODE_DESCRIPTIONS
    #define ASSERT_STATUSCODE(a,b) ck_assert_str_eq(UA_StatusCode_name(a),UA_StatusCode_name(b))
#else
    #define ASSERT_STATUSCODE(a,b) ck_assert_uint_eq((a),(b))
#endif

UA_Server *server;
size_t callbackCount = 0;
UA_StatusCode expectedDataValueStatus;
static UA_Boolean deleteAtMonitoredItemCreated;
static UA_StatusCode deleteAtMonitoredItemCreatedResult;
static UA_Boolean deleteAtMonitoredItemDelete;
static UA_StatusCode deleteAtMonitoredItemDeleteResult;
static UA_Boolean captureCreatedForDataSource;
static UA_UInt32 deleteFromDataSourceMonitoredItemId;
static UA_StatusCode deleteFromDataSourceResult;
static UA_Boolean deletedReadCompletesAsync;
static UA_DataValue *deletedReadOutput;

UA_NodeId parentNodeId;
UA_NodeId parentReferenceNodeId;
UA_NodeId outNodeId;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    UA_StatusCode retval = UA_Server_run_startup(server);
    ASSERT_STATUSCODE(retval, UA_STATUSCODE_GOOD);
    /* Define the attribute of the uint32 variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_UInt32 myUint32 = 40;
    UA_Variant_setScalar(&attr.value, &myUint32, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    //attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId uint32NodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName uint32Name = UA_QUALIFIEDNAME(1, "the answer");
    parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId_init(&outNodeId);
    ASSERT_STATUSCODE(UA_Server_addVariableNode(server,
                                                uint32NodeId,
                                                parentNodeId,
                                                parentReferenceNodeId,
                                                uint32Name,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                attr,
                                                NULL,
                                                &outNodeId), UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    /* cleanup */
    UA_NodeId_clear(&parentNodeId);
    UA_NodeId_clear(&parentReferenceNodeId);
    UA_NodeId_clear(&outNodeId);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void
dataChangeNotificationCallback(UA_Server *thisServer,
                               UA_UInt32 monitoredItemId,
                               void *monitoredItemContext,
                               const UA_NodeId *nodeId,
                               void *nodeContext,
                               UA_UInt32 attributeId,
                               const UA_DataValue *value)
{
    static UA_UInt32 lastValue = 100;
    UA_UInt32 currentValue = *((UA_UInt32*)value->value.data);
    ck_assert_uint_ne(lastValue, currentValue);
    lastValue = currentValue;
    callbackCount++;
}

static void
monitoredItemLifecycleCallback(UA_Server *thisServer,
                               UA_ApplicationNotificationType type,
                               const UA_KeyValueMap payload) {
    if(type == UA_APPLICATIONNOTIFICATIONTYPE_MONITOREDITEM_CREATED &&
       captureCreatedForDataSource) {
        captureCreatedForDataSource = false;
        deleteFromDataSourceMonitoredItemId =
            *(const UA_UInt32*)payload.map[2].value.data;
        return;
    }

    if(type == UA_APPLICATIONNOTIFICATIONTYPE_MONITOREDITEM_DELETED &&
       deleteAtMonitoredItemDelete) {
        deleteAtMonitoredItemDelete = false;
        const UA_UInt32 *monitoredItemId =
            (const UA_UInt32*)payload.map[2].value.data;
        deleteAtMonitoredItemDeleteResult =
            UA_Server_deleteMonitoredItem(thisServer, *monitoredItemId);
        return;
    }

    if(type != UA_APPLICATIONNOTIFICATIONTYPE_MONITOREDITEM_CREATED ||
       !deleteAtMonitoredItemCreated)
        return;

    deleteAtMonitoredItemCreated = false;
    const UA_UInt32 *monitoredItemId =
        (const UA_UInt32*)payload.map[2].value.data;
    deleteAtMonitoredItemCreatedResult =
        UA_Server_deleteMonitoredItem(thisServer, *monitoredItemId);
}

START_TEST(Server_LocalMonitoredItem_deleteFromCreatedNotification) {
    if(_i == 1)
        ASSERT_STATUSCODE(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->globalNotificationCallback = monitoredItemLifecycleCallback;
    deleteAtMonitoredItemCreated = true;
    deleteAtMonitoredItemCreatedResult = UA_STATUSCODE_BADINTERNALERROR;
    callbackCount = 0;

    UA_MonitoredItemCreateRequest request =
        UA_MonitoredItemCreateRequest_default(outNodeId);
    request.monitoringMode = UA_MONITORINGMODE_REPORTING;
    request.requestedParameters.samplingInterval = 0.0;

    UA_MonitoredItemCreateResult result =
        UA_Server_createDataChangeMonitoredItem(
            server, UA_TIMESTAMPSTORETURN_BOTH, request, NULL,
            dataChangeNotificationCallback);
    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
    ASSERT_STATUSCODE(deleteAtMonitoredItemCreatedResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callbackCount, 0);

    /* The outer create path must not reactivate a MonitoredItem that the
     * notification callback already deleted. */
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 0);
    ASSERT_STATUSCODE(UA_Server_deleteMonitoredItem(
        server, result.monitoredItemId),
        UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
    config->globalNotificationCallback = NULL;
}
END_TEST

START_TEST(Server_LocalMonitoredItem_deleteFromDeleteNotification) {
    UA_MonitoredItemCreateRequest request =
        UA_MonitoredItemCreateRequest_default(outNodeId);
    UA_MonitoredItemCreateResult result =
        UA_Server_createDataChangeMonitoredItem(
            server, UA_TIMESTAMPSTORETURN_BOTH, request, NULL,
            dataChangeNotificationCallback);
    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->globalNotificationCallback = monitoredItemLifecycleCallback;
    deleteAtMonitoredItemDelete = true;
    deleteAtMonitoredItemDeleteResult = UA_STATUSCODE_BADINTERNALERROR;

    ASSERT_STATUSCODE(UA_Server_deleteMonitoredItem(
        server, result.monitoredItemId), UA_STATUSCODE_GOOD);
    ASSERT_STATUSCODE(deleteAtMonitoredItemDeleteResult,
                      UA_STATUSCODE_BADMONITOREDITEMIDINVALID);

    /* Recursive deletion must not enqueue the embedded delayed callback twice. */
    UA_Server_run_iterate(server, false);
    ASSERT_STATUSCODE(UA_Server_deleteMonitoredItem(
        server, result.monitoredItemId),
        UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
    config->globalNotificationCallback = NULL;
}
END_TEST

START_TEST(Server_LocalMonitoredItem) {
    callbackCount = 0;

    UA_MonitoredItemCreateRequest monitorRequest =
            UA_MonitoredItemCreateRequest_default(outNodeId);
    monitorRequest.requestedParameters.samplingInterval = (double)100;
    monitorRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_MonitoredItemCreateResult result =
            UA_Server_createDataChangeMonitoredItem(server,
                                                    UA_TIMESTAMPSTORETURN_BOTH,
                                                    monitorRequest,
                                                    NULL,
                                                    &dataChangeNotificationCallback);

    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 1);

    UA_UInt32 count = 0;
    UA_Variant val;
    UA_Variant_setScalar(&val, &count, &UA_TYPES[UA_TYPES_UINT32]);

    for(size_t i = 0; i < 10; i++) {
        count++;
        UA_Server_writeValue(server, outNodeId, val);
        UA_fakeSleep(100);
        UA_Server_run_iterate(server, 1);
    }
    ck_assert_uint_eq(callbackCount, 11);
}
END_TEST

static void
deleteMonitoredItemCallback(UA_Server *thisServer,
                            UA_UInt32 monitoredItemId,
                            void *monitoredItemContext,
                            const UA_NodeId *nodeId,
                            void *nodeContext,
                            UA_UInt32 attributeId,
                            const UA_DataValue *value) {
    UA_StatusCode *deleteStatus = (UA_StatusCode*)monitoredItemContext;
    callbackCount++;
    *deleteStatus = UA_Server_deleteMonitoredItem(thisServer, monitoredItemId);
}

START_TEST(Server_LocalMonitoredItem_deleteInCallback) {
    callbackCount = 0;
    UA_StatusCode deleteStatus = UA_STATUSCODE_BADINTERNALERROR;
    UA_MonitoredItemCreateRequest request =
        UA_MonitoredItemCreateRequest_default(outNodeId);
    request.requestedParameters.samplingInterval = 0.0;
    request.requestedParameters.queueSize = 3;

    UA_MonitoredItemCreateResult result =
        UA_Server_createDataChangeMonitoredItem(
            server, UA_TIMESTAMPSTORETURN_BOTH, request, &deleteStatus,
            deleteMonitoredItemCallback);
    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);

    UA_UInt32 newValue = 41;
    UA_Variant value;
    UA_Variant_setScalar(&value, &newValue, &UA_TYPES[UA_TYPES_UINT32]);
    ASSERT_STATUSCODE(UA_Server_writeValue(server, outNodeId, value),
                      UA_STATUSCODE_GOOD);

    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 1);
    ASSERT_STATUSCODE(deleteStatus, UA_STATUSCODE_GOOD);

    UA_Server_run_iterate(server, false);
}
END_TEST

static UA_UInt32 staticUInt32 = 1337;

static UA_StatusCode
readDataSource(UA_Server *s, const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeId, void *nodeContext,
               UA_Boolean includeSourceTimeStamp, const UA_NumericRange *range,
               UA_DataValue *value) {
    if(deleteFromDataSourceMonitoredItemId != 0) {
        UA_UInt32 monitoredItemId = deleteFromDataSourceMonitoredItemId;
        deleteFromDataSourceMonitoredItemId = 0;
        deleteFromDataSourceResult =
            UA_Server_deleteMonitoredItem(s, monitoredItemId);
        /* Deletion must not reclaim the read description still in use. */
        for(size_t i = 0; i < 2; i++)
            UA_Server_run_iterate(s, false);
        if(deletedReadCompletesAsync) {
            deletedReadOutput = value;
            return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
        }
    }

    UA_Variant_setScalar(&value->value, &staticUInt32, &UA_TYPES[UA_TYPES_UINT32]);
    value->value.storageType = UA_VARIANT_DATA_NODELETE;
    value->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

/* Use a datasource with a static memory location */
START_TEST(Server_LocalMonitoredItem_dataSource) {
    callbackCount = 0;

    UA_DataSource ds = {readDataSource, NULL};
    UA_Server_setVariableNode_dataSource(server, outNodeId, ds);

    UA_MonitoredItemCreateRequest monitorRequest =
            UA_MonitoredItemCreateRequest_default(outNodeId);
    monitorRequest.requestedParameters.samplingInterval = (double)100;
    monitorRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_MonitoredItemCreateResult result =
            UA_Server_createDataChangeMonitoredItem(server,
                                                    UA_TIMESTAMPSTORETURN_BOTH,
                                                    monitorRequest,
                                                    NULL,
                                                    &dataChangeNotificationCallback);

    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 1);

    for(size_t i = 0; i < 10; i++) {
        staticUInt32++;
        UA_fakeSleep(100);
        UA_Server_run_iterate(server, 1);
    }
    ck_assert_uint_eq(callbackCount, 11);
}
END_TEST

START_TEST(Server_LocalMonitoredItem_deleteFromDataSourceRead) {
    deletedReadCompletesAsync = (_i % 2 == 1);
    deletedReadOutput = NULL;
    callbackCount = 0;
    captureCreatedForDataSource = true;
    deleteFromDataSourceMonitoredItemId = 0;
    deleteFromDataSourceResult = UA_STATUSCODE_BADINTERNALERROR;

    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->globalNotificationCallback = monitoredItemLifecycleCallback;

    UA_DataSource ds = {readDataSource, NULL};
    ASSERT_STATUSCODE(UA_Server_setVariableNode_dataSource(
        server, outNodeId, ds), UA_STATUSCODE_GOOD);

    UA_MonitoredItemCreateRequest request =
        UA_MonitoredItemCreateRequest_default(outNodeId);
    request.requestedParameters.samplingInterval = 0.0;
    request.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_MonitoredItemCreateResult result =
        UA_Server_createDataChangeMonitoredItem(
            server, UA_TIMESTAMPSTORETURN_BOTH, request, NULL,
            dataChangeNotificationCallback);

    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
    ASSERT_STATUSCODE(deleteFromDataSourceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callbackCount, 0);
    if(deletedReadCompletesAsync) {
        UA_AsyncOperation *op = TAILQ_FIRST(&server->asyncManager.operations);
        ck_assert_ptr_nonnull(op);
        ck_assert_uint_eq(op->resultIndex, 0);
        ck_assert_ptr_nonnull(op->handling.callback.context);
    }
    for(size_t i = 0; i < 5; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 0);
    ASSERT_STATUSCODE(UA_Server_deleteMonitoredItem(
        server, result.monitoredItemId),
        UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
    config->globalNotificationCallback = NULL;
    if(deletedReadCompletesAsync) {
        ck_assert_ptr_nonnull(deletedReadOutput);
        ASSERT_STATUSCODE(UA_Server_setAsyncReadResult(server, deletedReadOutput),
                          UA_STATUSCODE_GOOD);
        for(size_t i = 0; i < 5; i++)
            UA_Server_run_iterate(server, false);
        ck_assert_uint_eq(callbackCount, 0);
        ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    }
    deletedReadCompletesAsync = false;
}
END_TEST

static UA_DataValue *pendingReadOutputs[3 * UA_MONITOREDITEM_ASYNC_MAX];
static size_t pendingReadCount;
static size_t canceledReadCount;

static UA_StatusCode
pendingRead(UA_Server *s, const UA_NodeId *sessionId, void *sessionContext,
            const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimestamp,
            const UA_NumericRange *range, UA_DataValue *value) {
    ck_assert_uint_lt(pendingReadCount,
                      sizeof(pendingReadOutputs) / sizeof(pendingReadOutputs[0]));
    pendingReadOutputs[pendingReadCount++] = value;
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

static void
acknowledgeCanceledRead(UA_Server *s, const void *out) {
    canceledReadCount++;
    ASSERT_STATUSCODE(UA_Server_setAsyncReadResult(s, (UA_DataValue*)(uintptr_t)out),
                      UA_STATUSCODE_GOOD);
}

/* Deliver the last sample from inside the deletion notification. The item
 * must remain alive until both the notification and deletion have returned. */
static void
finishReadDuringDeletion(UA_Server *s, UA_ApplicationNotificationType type,
                         const UA_KeyValueMap payload) {
    if(type != UA_APPLICATIONNOTIFICATIONTYPE_MONITOREDITEM_DELETED)
        return;
    ck_assert_uint_eq(pendingReadCount, 1);
    ASSERT_STATUSCODE(UA_Server_setAsyncReadResult(s, pendingReadOutputs[0]),
                      UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 3; i++) {
        UA_fakeSleep(100);
        UA_Server_run_iterate(s, false);
    }
    ck_assert_uint_eq(s->asyncManager.trackedOpsCount, 0);
    const UA_UInt32 *id = (const UA_UInt32*)UA_KeyValueMap_getScalar(
        &payload, UA_QUALIFIEDNAME(0, "monitoreditem-id"), &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_ptr_nonnull(id);
    ck_assert_uint_ne(*id, 0);
}

START_TEST(Server_LocalMonitoredItem_completeReadDuringDeletion) {
    pendingReadCount = 0;
    UA_MonitoredItemCreateRequest request = UA_MonitoredItemCreateRequest_default(outNodeId);
    request.monitoringMode = UA_MONITORINGMODE_DISABLED;
    UA_MonitoredItemCreateResult result = UA_Server_createDataChangeMonitoredItem(
        server, UA_TIMESTAMPSTORETURN_NEITHER, request, NULL, dataChangeNotificationCallback);
    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
    UA_CallbackValueSource source = {pendingRead, NULL};
    ASSERT_STATUSCODE(UA_Server_setVariableNode_callbackValueSource(server, outNodeId, source),
                      UA_STATUSCODE_GOOD);
    lockServer(server);
    UA_MonitoredItem *mon = UA_Subscription_getMonitoredItem(
        server->adminSubscription, result.monitoredItemId);
    UA_MonitoredItem_sample(server, mon);
    unlockServer(server);
    server->config.globalNotificationCallback = finishReadDuringDeletion;
    ASSERT_STATUSCODE(UA_Server_deleteMonitoredItem(server, result.monitoredItemId),
                      UA_STATUSCODE_GOOD);
    server->config.globalNotificationCallback = NULL;
    UA_Server_run_iterate(server, false);
}
END_TEST

/* Delete with all reads pending, with a ready result, or from inside the ready
 * result's notification. Complete after deletion, or cancel during shutdown. */
START_TEST(Server_LocalMonitoredItem_deletePendingReads) {
    pendingReadCount = 0;
    canceledReadCount = 0;
    callbackCount = 0;
    UA_MonitoredItemCreateRequest request = UA_MonitoredItemCreateRequest_default(outNodeId);
    request.requestedParameters.samplingInterval = 10000;
    request.monitoringMode = UA_MONITORINGMODE_DISABLED;
    UA_StatusCode deleteStatus = UA_STATUSCODE_BADINTERNALERROR;
    UA_MonitoredItemCreateResult first = UA_Server_createDataChangeMonitoredItem(
        server, UA_TIMESTAMPSTORETURN_NEITHER, request, &deleteStatus, deleteMonitoredItemCallback);
    UA_MonitoredItemCreateResult second = UA_Server_createDataChangeMonitoredItem(
        server, UA_TIMESTAMPSTORETURN_NEITHER, request, &deleteStatus, deleteMonitoredItemCallback);
    ASSERT_STATUSCODE(first.statusCode, UA_STATUSCODE_GOOD);
    ASSERT_STATUSCODE(second.statusCode, UA_STATUSCODE_GOOD);

    /* Complete creation's synchronous validation reads before installing the
     * deferred source. Enabling reporting then starts the actual async samples. */
    UA_CallbackValueSource source = {pendingRead, NULL};
    ASSERT_STATUSCODE(UA_Server_setVariableNode_callbackValueSource(server, outNodeId, source),
                      UA_STATUSCODE_GOOD);
    UA_Server_getConfig(server)->asyncOperationCancelCallback = acknowledgeCanceledRead;
    lockServer(server);
    UA_MonitoredItem *mon = UA_Subscription_getMonitoredItem(
        server->adminSubscription, first.monitoredItemId);
    UA_MonitoredItem *other = UA_Subscription_getMonitoredItem(
        server->adminSubscription, second.monitoredItemId);
    ck_assert_ptr_nonnull(mon);
    ck_assert_ptr_nonnull(other);
    ASSERT_STATUSCODE(UA_MonitoredItem_setMonitoringMode(server, mon, UA_MONITORINGMODE_REPORTING),
                      UA_STATUSCODE_GOOD);
    ASSERT_STATUSCODE(UA_MonitoredItem_setMonitoringMode(server, other, UA_MONITORINGMODE_REPORTING),
                      UA_STATUSCODE_GOOD);
    UA_MonitoredItem_sample(server, mon);
    ck_assert_uint_eq(mon->outstandingAsyncReads, 2);
    ck_assert_uint_eq(other->outstandingAsyncReads, 1);
    unlockServer(server);
    ck_assert_uint_eq(pendingReadCount, 3);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 3);

    if(_i == 1 || _i == 2) {
        UA_UInt32 value = 42;
        ASSERT_STATUSCODE(UA_Variant_setScalarCopy(&pendingReadOutputs[0]->value, &value,
                                                  &UA_TYPES[UA_TYPES_UINT32]), UA_STATUSCODE_GOOD);
        pendingReadOutputs[0]->hasValue = true;
        ASSERT_STATUSCODE(UA_Server_setAsyncReadResult(server, pendingReadOutputs[0]),
                          UA_STATUSCODE_GOOD);
    }
    if(_i != 2)
        ASSERT_STATUSCODE(UA_Server_deleteMonitoredItem(server, first.monitoredItemId),
                          UA_STATUSCODE_GOOD);
    ASSERT_STATUSCODE(UA_Server_deleteMonitoredItem(server, second.monitoredItemId),
                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_null(other->subscription);
    if(_i != 2)
        ck_assert_ptr_null(mon->subscription);
    /* Deleted items survive event-loop iterations until their reads finish. */
    for(size_t i = 0; i < 10 && server->asyncManager.trackedOpsCount > 0; i++)
        UA_Server_run_iterate(server, false);

    ck_assert_uint_eq(callbackCount, _i == 2 ? 1 : 0);
    ck_assert_ptr_null(mon->subscription);
    ck_assert_ptr_null(other->subscription);
    ck_assert_uint_eq(mon->outstandingAsyncReads, (_i == 0 || _i == 3) ? 2 : 1);
    ck_assert_uint_eq(other->outstandingAsyncReads, 1);
    if(_i == 2)
        ASSERT_STATUSCODE(deleteStatus, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(canceledReadCount, 0);
    ASSERT_STATUSCODE(UA_Server_deleteMonitoredItem(server, first.monitoredItemId),
                      UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
    ASSERT_STATUSCODE(UA_Server_deleteMonitoredItem(server, second.monitoredItemId),
                      UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, (_i == 0 || _i == 3) ? 3 : 2);
    if(_i == 3) {
        ASSERT_STATUSCODE(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(canceledReadCount, 3);
    } else {
        for(size_t i = (_i == 0 ? 0 : 1); i < pendingReadCount; i++) {
            UA_UInt32 value = 43;
            ASSERT_STATUSCODE(UA_Variant_setScalarCopy(&pendingReadOutputs[i]->value, &value,
                              &UA_TYPES[UA_TYPES_UINT32]), UA_STATUSCODE_GOOD);
            pendingReadOutputs[i]->hasValue = true;
            ASSERT_STATUSCODE(UA_Server_setAsyncReadResult(server, pendingReadOutputs[i]),
                              UA_STATUSCODE_GOOD);
        }
        for(size_t i = 0; i < 5; i++)
            UA_Server_run_iterate(server, false);
    }
    ck_assert_uint_eq(callbackCount, _i == 2 ? 1 : 0);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
}
END_TEST

static UA_Boolean
allowReadSubscriptionTransfer(UA_Server *s, UA_AccessControl *ac,
                              const UA_NodeId *oldSessionId, void *oldContext,
                              const UA_NodeId *newSessionId, void *newContext) {
    return true;
}

static void
pooledSample(UA_Server *s, UA_UInt32 id, void *context, const UA_NodeId *nodeId,
              void *nodeContext, UA_UInt32 attributeId, const UA_DataValue *value) {
    callbackCount++;
}

static UA_MonitoredItem *immediateItems[20];
static size_t immediateReads;

/* The entire listener snapshot must be retained before any read callback. */
static void
checkImmediateReferences(UA_Server *s, const UA_NodeId *sessionId, void *sessionContext,
                         const UA_NodeId *nodeId, void *nodeContext,
                         const UA_NumericRange *range, const UA_DataValue *value) {
    for(size_t i = 0; i < 20; i++)
        ck_assert_uint_gt(immediateItems[i]->outstandingAsyncReads, 0);
    immediateReads++;
}

START_TEST(Server_LocalMonitoredItem_immediateBatch) {
    UA_MonitoredItemCreateRequest request = UA_MonitoredItemCreateRequest_default(outNodeId);
    request.requestedParameters.samplingInterval = 0.0;
    for(size_t i = 0; i < 20; i++) {
        UA_MonitoredItemCreateResult result = UA_Server_createDataChangeMonitoredItem(
            server, UA_TIMESTAMPSTORETURN_BOTH, request, NULL, pooledSample);
        ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
        immediateItems[i] = UA_Subscription_getMonitoredItem(server->adminSubscription,
                                                            result.monitoredItemId);
        ck_assert_ptr_nonnull(immediateItems[i]);
        ck_assert_uint_eq(immediateItems[i]->outstandingAsyncReads, 0);
    }
    UA_Server_run_iterate(server, false);
    callbackCount = 0;
    immediateReads = 0;
    UA_ValueSourceNotifications notifications = {checkImmediateReferences, NULL};
    ASSERT_STATUSCODE(UA_Server_setVariableNode_internalValueSource(
        server, outNodeId, NULL, &notifications), UA_STATUSCODE_GOOD);
    UA_UInt32 number = 42;
    UA_Variant value;
    UA_Variant_setScalar(&value, &number, &UA_TYPES[UA_TYPES_UINT32]);
    ASSERT_STATUSCODE(UA_Server_writeValue(server, outNodeId, value), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(immediateReads, 20);
    for(size_t i = 0; i < 20; i++)
        ck_assert_uint_eq(immediateItems[i]->outstandingAsyncReads, 0);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 20);
}
END_TEST

/* Neither timed nor write-triggered sampling runs outside STARTED. Restart
 * resumes normal triggers without forcing an initial zero-interval sample. */
START_TEST(Server_LocalMonitoredItem_samplingRequiresStarted) {
    server->config.externalEventLoop = (_i == 1);
    ASSERT_STATUSCODE(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_Server_getLifecycleState(server),
                     _i == 1 ? UA_LIFECYCLESTATE_STOPPING : UA_LIFECYCLESTATE_STOPPED);
    callbackCount = 0;

    UA_MonitoredItem *items[2];
    for(size_t i = 0; i < 2; i++) {
        UA_MonitoredItemCreateRequest request = UA_MonitoredItemCreateRequest_default(outNodeId);
        request.requestedParameters.samplingInterval = i == 0 ? 0.0 : 1000.0;
        UA_MonitoredItemCreateResult result = UA_Server_createDataChangeMonitoredItem(
            server, UA_TIMESTAMPSTORETURN_NEITHER, request, NULL, pooledSample);
        ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
        items[i] = UA_Subscription_getMonitoredItem(server->adminSubscription,
                                                   result.monitoredItemId);
        ck_assert_ptr_nonnull(items[i]);
        ck_assert(!items[i]->lastValue.hasValue);
        UA_MonitoredItemCreateResult_clear(&result);
    }

    /* Writes still succeed, but neither sampling entry point produces data. */
    UA_UInt32 number = 41;
    UA_Variant value;
    UA_Variant_setScalar(&value, &number, &UA_TYPES[UA_TYPES_UINT32]);
    ASSERT_STATUSCODE(UA_Server_writeValue(server, outNodeId, value), UA_STATUSCODE_GOOD);
    lockServer(server);
    for(size_t i = 0; i < 2; i++) {
        UA_MonitoredItem_sample(server, items[i]);
        ck_assert_uint_eq(items[i]->outstandingAsyncReads, 0);
        ck_assert(!items[i]->lastValue.hasValue);
        ck_assert_uint_eq(items[i]->lastValue.status, ~(UA_StatusCode)0);
        ck_assert_uint_eq(items[i]->queueSize, 0);
    }
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    unlockServer(server);
    if(_i == 1) {
        UA_fakeSleep(1000);
        for(size_t i = 0; i < 5; i++)
            UA_Server_run_iterate(server, false);
    }
    ck_assert_uint_eq(callbackCount, 0);
    server->config.externalEventLoop = false;

    /* The timed item resumes on its tick; the zero-interval item waits for a write. */
    ASSERT_STATUSCODE(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    UA_fakeSleep(1000);
    for(size_t i = 0; i < 5; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 1);
    ck_assert(!items[0]->lastValue.hasValue);
    ck_assert(items[1]->lastValue.hasValue);
    number = 42;
    ASSERT_STATUSCODE(UA_Server_writeValue(server, outNodeId, value), UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 2);
    ck_assert(items[0]->lastValue.hasValue);
}
END_TEST

/* Shutdown returns read ownership without publishing a cancellation as data. */
START_TEST(Server_LocalMonitoredItem_shutdownPendingSample) {
    UA_MonitoredItemCreateRequest request = UA_MonitoredItemCreateRequest_default(outNodeId);
    request.requestedParameters.samplingInterval = 10000.0;
    UA_MonitoredItemCreateResult result = UA_Server_createDataChangeMonitoredItem(
        server, UA_TIMESTAMPSTORETURN_NEITHER, request, NULL, pooledSample);
    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    callbackCount = pendingReadCount = canceledReadCount = 0;
    UA_CallbackValueSource source = {pendingRead, NULL};
    ASSERT_STATUSCODE(UA_Server_setVariableNode_callbackValueSource(server, outNodeId, source),
                      UA_STATUSCODE_GOOD);
    lockServer(server);
    UA_MonitoredItem *mon = UA_Subscription_getMonitoredItem(
        server->adminSubscription, result.monitoredItemId);
    UA_MonitoredItem_sample(server, mon);
    ck_assert_uint_eq(mon->outstandingAsyncReads, 1);
    unlockServer(server);

    server->config.asyncOperationCancelCallback = acknowledgeCanceledRead;
    ASSERT_STATUSCODE(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(canceledReadCount, 1);
    ck_assert_uint_eq(callbackCount, 0);
    ck_assert_uint_eq(mon->outstandingAsyncReads, 0);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    ck_assert(mon->lastValue.hasValue);
    ASSERT_STATUSCODE(mon->lastValue.status, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(*(UA_UInt32*)mon->lastValue.value.data, 40);
    UA_MonitoredItemCreateResult_clear(&result);
} END_TEST

START_TEST(Server_LocalMonitoredItem_readContextLimit) {
    UA_MonitoredItem *items[2];
    UA_MonitoredItemCreateRequest request = UA_MonitoredItemCreateRequest_default(outNodeId);
    request.monitoringMode = UA_MONITORINGMODE_DISABLED;
    for(size_t i = 0; i < 2; i++) {
        UA_MonitoredItemCreateResult result = UA_Server_createDataChangeMonitoredItem(
            server, UA_TIMESTAMPSTORETURN_NEITHER, request, NULL, pooledSample);
        ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
        lockServer(server);
        items[i] = UA_Subscription_getMonitoredItem(server->adminSubscription,
                                                   result.monitoredItemId);
        unlockServer(server);
        UA_MonitoredItemCreateResult_clear(&result);
    }
    UA_CallbackValueSource source = {pendingRead, NULL};
    ASSERT_STATUSCODE(UA_Server_setVariableNode_callbackValueSource(server, outNodeId, source),
                      UA_STATUSCODE_GOOD);
    pendingReadCount = 0;
    lockServer(server);
    for(size_t i = 0; i < 2; i++) {
        for(size_t j = 0; j < UA_MONITOREDITEM_ASYNC_MAX; j++)
            UA_MonitoredItem_sample(server, items[i]);
        ck_assert_uint_eq(items[i]->outstandingAsyncReads, UA_MONITOREDITEM_ASYNC_MAX);
    }
    ck_assert_uint_eq(pendingReadCount, 2 * UA_MONITOREDITEM_ASYNC_MAX);
    /* The per-item limit still applies without starting another read. */
    UA_MonitoredItem_sample(server, items[0]);
    ck_assert_uint_eq(pendingReadCount, 2 * UA_MONITOREDITEM_ASYNC_MAX);
    unlockServer(server);

    for(size_t i = 0; i < pendingReadCount; i++)
        ASSERT_STATUSCODE(UA_Server_setAsyncReadResult(server, pendingReadOutputs[i]),
                          UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    lockServer(server);
    for(size_t i = 0; i < 2; i++) {
        ck_assert_uint_eq(items[i]->outstandingAsyncReads, 0);
    }
    unlockServer(server);
} END_TEST

/* The initiating session is freed before completion. The subscription is
 * deleted, detached on timeout, or transferred to a different session. */
START_TEST(Server_MonitoredItem_readAfterSessionRemoval) {
    pendingReadCount = 0;
    lockServer(server);
    UA_CreateSessionRequest sessionRequest;
    UA_CreateSessionRequest_init(&sessionRequest);
    sessionRequest.requestedSessionTimeout = UA_UINT32_MAX;
    UA_Session *session = NULL;
    ASSERT_STATUSCODE(UA_Session_create(server, NULL, &sessionRequest, &session),
                      UA_STATUSCODE_GOOD);
    UA_CreateSubscriptionRequest subRequest;
    UA_CreateSubscriptionRequest_init(&subRequest);
    subRequest.requestedPublishingInterval = 10000;
    UA_CreateSubscriptionResponse subResponse;
    UA_CreateSubscriptionResponse_init(&subResponse);
    Service_CreateSubscription(server, session, &subRequest, &subResponse);
    ASSERT_STATUSCODE(subResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subscriptionId = subResponse.subscriptionId;
    UA_CreateSubscriptionResponse_clear(&subResponse);

    UA_MonitoredItemCreateRequest item = UA_MonitoredItemCreateRequest_default(outNodeId);
    item.monitoringMode = UA_MONITORINGMODE_DISABLED;
    item.requestedParameters.samplingInterval = 10000;
    UA_CreateMonitoredItemsRequest monRequest;
    UA_CreateMonitoredItemsRequest_init(&monRequest);
    monRequest.subscriptionId = subscriptionId;
    monRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    monRequest.itemsToCreateSize = 1;
    monRequest.itemsToCreate = &item;
    UA_CreateMonitoredItemsResponse monResponse;
    UA_CreateMonitoredItemsResponse_init(&monResponse);
    Service_CreateMonitoredItems(server, session, &monRequest, &monResponse);
    ASSERT_STATUSCODE(monResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(monResponse.resultsSize, 1);
    ASSERT_STATUSCODE(monResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    UA_UInt32 monitoredItemId = monResponse.results[0].monitoredItemId;
    UA_CreateMonitoredItemsResponse_clear(&monResponse);
    unlockServer(server);

    UA_CallbackValueSource source = {pendingRead, NULL};
    ASSERT_STATUSCODE(UA_Server_setVariableNode_callbackValueSource(server, outNodeId, source),
                      UA_STATUSCODE_GOOD);
    lockServer(server);
    UA_Subscription *sub = getSubscriptionById(server, subscriptionId);
    UA_MonitoredItem *mon = UA_Subscription_getMonitoredItem(sub, monitoredItemId);
    ASSERT_STATUSCODE(UA_MonitoredItem_setMonitoringMode(server, mon, UA_MONITORINGMODE_REPORTING),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pendingReadCount, 1);
    ck_assert_uint_eq(mon->outstandingAsyncReads, 1);
    /* An admission failure must undo only its own read count, leaving the
     * first sample outstanding. */
    size_t previousLimit = server->config.maxAsyncOperationQueueSize;
    server->config.maxAsyncOperationQueueSize = 1;
    UA_MonitoredItem_sample(server, mon);
    server->config.maxAsyncOperationQueueSize = previousLimit;
    ck_assert_uint_eq(pendingReadCount, 1);
    ck_assert_uint_eq(mon->outstandingAsyncReads, 1);
    ASSERT_STATUSCODE(mon->lastValue.status, UA_STATUSCODE_BADTOOMANYOPERATIONS);
    if(_i == 2) {
        UA_Session *target = NULL;
        ASSERT_STATUSCODE(UA_Session_create(server, NULL, &sessionRequest, &target),
                          UA_STATUSCODE_GOOD);
        server->config.accessControl.allowTransferSubscription = allowReadSubscriptionTransfer;
        UA_TransferSubscriptionsRequest transfer;
        UA_TransferSubscriptionsRequest_init(&transfer);
        transfer.subscriptionIdsSize = 1;
        transfer.subscriptionIds = &subscriptionId;
        UA_TransferSubscriptionsResponse response;
        UA_TransferSubscriptionsResponse_init(&response);
        Service_TransferSubscriptions(server, target, &transfer, &response);
        ASSERT_STATUSCODE(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(response.resultsSize, 1);
        ASSERT_STATUSCODE(response.results[0].statusCode, UA_STATUSCODE_GOOD);
        UA_TransferSubscriptionsResponse_clear(&response);
    }
    UA_Session_remove(server, session,
                      _i == 1 ? UA_SHUTDOWNREASON_TIMEOUT : UA_SHUTDOWNREASON_CLOSE);
    unlockServer(server);
    for(size_t i = 0; i < 5; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 1);

    /* Deletion detaches the retained item from its now-freed subscription. */
    if(_i == 0)
        ck_assert_ptr_null(mon->subscription);
    else
        ck_assert_ptr_nonnull(mon->subscription);
    ck_assert_uint_eq(mon->outstandingAsyncReads, 1);

    UA_UInt32 value = 42;
    ASSERT_STATUSCODE(UA_Variant_setScalarCopy(&pendingReadOutputs[0]->value, &value,
                                              &UA_TYPES[UA_TYPES_UINT32]), UA_STATUSCODE_GOOD);
    pendingReadOutputs[0]->hasValue = true;
    ASSERT_STATUSCODE(UA_Server_setAsyncReadResult(server, pendingReadOutputs[0]),
                      UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 5; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    lockServer(server);
    sub = getSubscriptionById(server, subscriptionId);
    if(_i == 0) {
        ck_assert_ptr_null(sub);
    } else {
        ck_assert_ptr_nonnull(sub);
        mon = UA_Subscription_getMonitoredItem(sub, monitoredItemId);
        ck_assert_ptr_nonnull(mon);
        ck_assert_uint_eq(mon->outstandingAsyncReads, 0);
        ck_assert(mon->lastValue.hasValue);
        ck_assert_uint_eq(*(UA_UInt32*)mon->lastValue.value.data, 42);
    }
    unlockServer(server);
}
END_TEST

/* Custom datatype with a String NodeId */
typedef struct {
    UA_Float p;
} Point;

static UA_DataTypeMember members[1] = {
    {
        UA_TYPENAME("p")           /* .memberName */
        &UA_TYPES[UA_TYPES_FLOAT], /* .memberType */
        0,                         /* .padding */
        false,                     /* .isArray */
        false                      /* .isOptional*/
    }
};

static UA_DataType PointType = {
    UA_TYPENAME("Point")             /* .typeName */
    {1, UA_NODEIDTYPE_NUMERIC, {0}}, /* .typeId */
    {1, UA_NODEIDTYPE_NUMERIC, {0}}, /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
    {1, UA_NODEIDTYPE_NUMERIC, {0}}, /* .xmlEncodingId */
    sizeof(Point),                   /* .memSize */
    UA_DATATYPEKIND_STRUCTURE,       /* .typeKind */
    true,                            /* .pointerFree */
    false,                           /* .overlayable (depends on endianness and
                                         the absence of padding) */
    1,                               /* .membersSize */
    members
};

UA_DataTypeArray customDataTypes = {NULL, 1, &PointType, UA_FALSE};

START_TEST(Server_LocalMonitoredItem_CustomType) {
    callbackCount = 0;

    PointType.binaryEncodingId = UA_NODEID_STRING(1, "pointbinary");

    UA_ServerConfig *config = UA_Server_getConfig(server);
    customDataTypes.next = config->customDataTypes;
    config->customDataTypes = &customDataTypes;

    UA_MonitoredItemCreateRequest monitorRequest =
            UA_MonitoredItemCreateRequest_default(outNodeId);
    monitorRequest.requestedParameters.samplingInterval = (double)100;
    monitorRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_MonitoredItemCreateResult result =
            UA_Server_createDataChangeMonitoredItem(server,
                                                    UA_TIMESTAMPSTORETURN_BOTH,
                                                    monitorRequest,
                                                    NULL,
                                                    &dataChangeNotificationCallback);

    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 1);

    /* Use a value that requires the ExtensionObject to encode the NodeId of the
     * data type */
    Point p = {0.0};
    UA_Variant val;
    UA_ExtensionObject arr[100];
    UA_Variant_setArray(&val, arr, 100, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    for(size_t i = 0; i < 100; i++) {
        UA_ExtensionObject_setValueNoDelete(&arr[i], &p, &PointType);
    }
    UA_StatusCode retval = UA_Server_writeValue(server, outNodeId, val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_fakeSleep(100);
    UA_Server_run_iterate(server, 1);
    ck_assert_uint_eq(callbackCount, 2);

    p.p = 1.0;
    UA_fakeSleep(100);
    UA_Server_run_iterate(server, 1);
    ck_assert_uint_eq(callbackCount, 2);
}
END_TEST

static void setupIndexRange(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    UA_StatusCode retval = UA_Server_run_startup(server);
    ASSERT_STATUSCODE(retval, UA_STATUSCODE_GOOD);
    /* Define the attribute of the uint32 array variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_UInt32 myUint32Array[3] = {40, 41, 42};
    UA_Variant_setArray(&attr.value, &myUint32Array, 3, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","UInt32 Array");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","UInt32 Array");
    //attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId uint32ArrayNodeId = UA_NODEID_STRING(1, "UInt32Array");
    UA_QualifiedName uint32ArrayName = UA_QUALIFIEDNAME(1, "UInt32 Array");
    parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId_init(&outNodeId);
    ASSERT_STATUSCODE(UA_Server_addVariableNode(server,
                                                uint32ArrayNodeId,
                                                parentNodeId,
                                                parentReferenceNodeId,
                                                uint32ArrayName,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                attr,
                                                NULL,
                                                &outNodeId), UA_STATUSCODE_GOOD);
}

static void
dataChangeNotificationValidateStatusCallback(UA_Server *thisServer, UA_UInt32 monitoredItemId,
                                             void *monitoredItemContext, const UA_NodeId *nodeId,
                                             void *nodeContext, UA_UInt32 attributeId,
                                             const UA_DataValue *value) {
    ASSERT_STATUSCODE(value->status, expectedDataValueStatus);
    callbackCount++;
}

START_TEST(Server_LocalMonitoredItemIndexRange) {
    callbackCount = 0;
    expectedDataValueStatus = UA_STATUSCODE_GOOD;

    UA_MonitoredItemCreateRequest monitorRequest =
        UA_MonitoredItemCreateRequest_default(outNodeId);
    monitorRequest.requestedParameters.samplingInterval = (double)100;
    monitorRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;
    monitorRequest.itemToMonitor.indexRange = UA_STRING("0:2");
    UA_MonitoredItemCreateResult result = UA_Server_createDataChangeMonitoredItem(
        server, UA_TIMESTAMPSTORETURN_BOTH, monitorRequest, NULL,
        &dataChangeNotificationValidateStatusCallback);

    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 1);
}
END_TEST

START_TEST(Server_LocalMonitoredItemIndexRangeOutOfBounds) {
    callbackCount = 0;
    expectedDataValueStatus = UA_STATUSCODE_BADINDEXRANGENODATA;

    UA_MonitoredItemCreateRequest monitorRequest =
        UA_MonitoredItemCreateRequest_default(outNodeId);
    monitorRequest.requestedParameters.samplingInterval = (double)100;
    monitorRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;
    monitorRequest.itemToMonitor.indexRange = UA_STRING("3:5");
    UA_MonitoredItemCreateResult result = UA_Server_createDataChangeMonitoredItem(
        server, UA_TIMESTAMPSTORETURN_BOTH, monitorRequest, NULL,
        &dataChangeNotificationValidateStatusCallback);

    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 1);
}
END_TEST

START_TEST(Server_LocalMonitoredItem_EventNotifierRejected) {
    /* src/server/ua_services_monitoreditem.c:656-662:
     *   if(item.itemToMonitor.attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER) {
     *     result.statusCode = UA_STATUSCODE_BADINTERNALERROR;
     *     return result;
     *   }
     * The DataChange local-monitored-item creator must reject
     * the EventNotifier attribute (use createEventMonitoredItem
     * for events). None of the existing tests exercises this
     * mis-use guard. */
    UA_MonitoredItemCreateRequest request;
    UA_MonitoredItemCreateRequest_init(&request);
    request.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    request.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    request.monitoringMode = UA_MONITORINGMODE_REPORTING;

    UA_MonitoredItemCreateResult result = UA_Server_createDataChangeMonitoredItem(
        server, UA_TIMESTAMPSTORETURN_BOTH, request, NULL,
        &dataChangeNotificationCallback);
    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_BADINTERNALERROR);
    /* No monitored item was created */
    ck_assert_uint_eq(result.monitoredItemId, 0);
}
END_TEST

/* ==== UA_Subscription_resendData ==== */

START_TEST(Server_Subscription_resendData_emptySubscription) {
    /* src/server/ua_subscription.c:947-977 (UA_Subscription_resendData):
     * The function walks the subscription's monitoredItems list and
     * creates a DataChange notification for each REPORTING
     * monitored item with an empty value queue. With no monitored
     * items attached to the admin subscription, the LIST_FOREACH
     * loop body never executes -- but the function entry/exit
     * (assertions, lock check) is exercised. This is a smoke test
     * for the function's public entry. */
    ck_assert_ptr_nonnull(server->adminSubscription);
    /* The admin subscription has no monitored items by default */
    /* Reset the shared counter -- earlier tests in this file may
     * have left it non-zero. */
    callbackCount = 0;
    /* Take the lock before calling -- UA_Subscription_resendData
     * asserts UA_LOCK_ASSERT(&server->serviceMutex) */
    lockServer(server);
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_Subscription_resendData(server, server->adminSubscription);
    unlockServer(server);
    /* No crash, no callback fired (no items) */
    ck_assert_uint_eq(callbackCount, 0);
}
END_TEST

START_TEST(Server_Subscription_resendData_withDataChangeItem) {
    /* Same as above but with a real DataChange local monitored item
     * attached to the admin subscription. The resendData call must
     * fire the callback once with the last sampled value. */
    UA_MonitoredItemCreateRequest request =
        UA_MonitoredItemCreateRequest_default(outNodeId);
    request.monitoringMode = UA_MONITORINGMODE_REPORTING;
    request.requestedParameters.samplingInterval = 100.0;
    UA_MonitoredItemCreateResult res = UA_Server_createDataChangeMonitoredItem(
        server, UA_TIMESTAMPSTORETURN_BOTH, request, NULL,
        &dataChangeNotificationCallback);
    ASSERT_STATUSCODE(res.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(res.monitoredItemId, 0);
    UA_UInt32 monId = res.monitoredItemId;
    (void)monId;

    /* Run a server iteration so the monitored item gets a sample. */
    UA_Server_run_iterate(server, false);
    ck_assert_uint_ge(callbackCount, 1);
    UA_UInt32 countAfterSample = callbackCount;
    (void)countAfterSample;

    /* resendData must not crash, even with a populated item. */
    lockServer(server);
    UA_Subscription_resendData(server, server->adminSubscription);
    unlockServer(server);

    /* Clean up the local monitored item. */
    UA_Server_deleteMonitoredItem(server, res.monitoredItemId);
}
END_TEST

static UA_Boolean expectNotificationFilter;
static unsigned filterNotifications;
static void
checkNotificationFilter(UA_Server *thisServer, UA_ApplicationNotificationType type,
                        const UA_KeyValueMap payload) {
    if(type != UA_APPLICATIONNOTIFICATIONTYPE_MONITOREDITEM_CREATED &&
       type != UA_APPLICATIONNOTIFICATIONTYPE_MONITOREDITEM_DELETED)
        return;
    const UA_Variant *filter = &payload.map[10].value;
    if(expectNotificationFilter)
        ck_assert(UA_Variant_hasScalarType(filter, &UA_TYPES[UA_TYPES_DATACHANGEFILTER]));
    else
        ck_assert_ptr_null(filter->type);
    filterNotifications++;
}

START_TEST(Server_NotificationFilterDoesNotOutliveItem) {
    UA_Server_getConfig(server)->globalNotificationCallback = checkNotificationFilter;
    filterNotifications = 0;
    for(unsigned i = 0; i < 2; i++) {
        expectNotificationFilter = (i == 0);
        UA_MonitoredItemCreateRequest request =
            UA_MonitoredItemCreateRequest_default(outNodeId);
        UA_DataChangeFilter filter;
        UA_DataChangeFilter_init(&filter);
        filter.trigger = UA_DATACHANGETRIGGER_STATUSVALUE;
        if(expectNotificationFilter)
            UA_ExtensionObject_setValueNoDelete(&request.requestedParameters.filter,
                &filter, &UA_TYPES[UA_TYPES_DATACHANGEFILTER]);
        UA_MonitoredItemCreateResult result = UA_Server_createDataChangeMonitoredItem(
            server, UA_TIMESTAMPSTORETURN_BOTH, request, NULL, dataChangeNotificationCallback);
        ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(UA_Server_deleteMonitoredItem(server, result.monitoredItemId), UA_STATUSCODE_GOOD);
        UA_MonitoredItemCreateResult_clear(&result);
    }
    ck_assert_uint_eq(filterNotifications, 4);
} END_TEST

static Suite * testSuite_Client(void) {
    Suite *s = suite_create("Local Monitored Item");
    TCase *tc_server = tcase_create("Local Monitored Item Basic");
    tcase_add_checked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_NotificationFilterDoesNotOutliveItem);
    tcase_add_test(tc_server, Server_LocalMonitoredItem);
    tcase_add_loop_test(tc_server,
                       Server_LocalMonitoredItem_deleteFromCreatedNotification, 0, 2);
    tcase_add_test(tc_server,
                   Server_LocalMonitoredItem_deleteFromDeleteNotification);
    tcase_add_test(tc_server, Server_LocalMonitoredItem_deleteInCallback);
    tcase_add_test(tc_server, Server_LocalMonitoredItem_dataSource);
    tcase_add_loop_test(tc_server, Server_LocalMonitoredItem_deleteFromDataSourceRead, 0, 2);
    tcase_add_loop_test(tc_server, Server_LocalMonitoredItem_deletePendingReads, 0, 4);
    tcase_add_test(tc_server, Server_LocalMonitoredItem_completeReadDuringDeletion);
    tcase_add_test(tc_server, Server_LocalMonitoredItem_immediateBatch);
    tcase_add_test(tc_server, Server_LocalMonitoredItem_shutdownPendingSample);
    tcase_add_loop_test(tc_server, Server_LocalMonitoredItem_samplingRequiresStarted, 0, 2);
    tcase_add_test(tc_server, Server_LocalMonitoredItem_readContextLimit);
    tcase_add_loop_test(tc_server, Server_MonitoredItem_readAfterSessionRemoval, 0, 3);
    tcase_add_test(tc_server, Server_LocalMonitoredItem_CustomType);
    tcase_add_test(tc_server, Server_LocalMonitoredItem_EventNotifierRejected);
    tcase_add_test(tc_server, Server_Subscription_resendData_emptySubscription);
    tcase_add_test(tc_server, Server_Subscription_resendData_withDataChangeItem);
    suite_add_tcase(s, tc_server);

    TCase *tc_server_indexrange = tcase_create("Local Monitored Item Index Range");
    tcase_add_checked_fixture(tc_server_indexrange, setupIndexRange, teardown);
    tcase_add_test(tc_server_indexrange, Server_LocalMonitoredItemIndexRange);
    tcase_add_test(tc_server_indexrange, Server_LocalMonitoredItemIndexRangeOutOfBounds);
    suite_add_tcase(s, tc_server_indexrange);

    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
