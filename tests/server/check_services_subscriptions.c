/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#include "server/ua_subscription.h"

#include <check.h>

#include "testing_clock.h"

static UA_Server *server = NULL;
static UA_Session *session = NULL;
static UA_UInt32 monitored = 0; /* Number of active MonitoredItems */

static void
monitoredRegisterCallback(UA_Server *s,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *nodeId, void *nodeContext,
                          const UA_UInt32 attrId, const UA_Boolean removed) {
    if(!removed)
        monitored++;
    else
        monitored--;
}

static void
createSession(void) {
    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);
    request.requestedSessionTimeout = UA_UINT32_MAX;
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_Server_createSession(server, NULL, &request, &session);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(retval, 0);
}

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    config->monitoredItemRegisterCallback = monitoredRegisterCallback;
    UA_Server_run_startup(server);
    createSession();
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    ck_assert_uint_eq(monitored, 0); /* All MonitoredItems have been de-registered */
}

#ifdef UA_ENABLE_SUBSCRIPTIONS

static UA_UInt32 subscriptionId;
static UA_UInt32 monitoredItemId;

static void
createSubscription(void) {
    UA_CreateSubscriptionRequest request;
    UA_CreateSubscriptionRequest_init(&request);
    request.publishingEnabled = true;

    UA_CreateSubscriptionResponse response;
    UA_CreateSubscriptionResponse_init(&response);

    UA_LOCK(&server->serviceMutex);
    Service_CreateSubscription(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    subscriptionId = response.subscriptionId;

    UA_CreateSubscriptionResponse_clear(&response);
}

static void
createMonitoredItem(void) {
    UA_CreateMonitoredItemsRequest request;
    UA_CreateMonitoredItemsRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_SERVER;
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    rvi.attributeId = UA_ATTRIBUTEID_BROWSENAME;
    rvi.indexRange = UA_STRING_NULL;
    item.itemToMonitor = rvi;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_MonitoringParameters params;
    UA_MonitoringParameters_init(&params);
    item.requestedParameters = params;
    request.itemsToCreateSize = 1;
    request.itemsToCreate = &item;

    UA_CreateMonitoredItemsResponse response;
    UA_CreateMonitoredItemsResponse_init(&response);

    UA_LOCK(&server->serviceMutex);
    Service_CreateMonitoredItems(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert_uint_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);

    monitoredItemId = response.results[0].monitoredItemId;
    ck_assert_uint_gt(monitoredItemId, 0);

    UA_MonitoredItemCreateRequest_clear(&item);
    UA_CreateMonitoredItemsResponse_clear(&response);
}

START_TEST(Server_createSubscription) {
    /* Create a subscription */
    UA_CreateSubscriptionRequest request;
    UA_CreateSubscriptionRequest_init(&request);
    request.publishingEnabled = true;

    UA_CreateSubscriptionResponse response;
    UA_CreateSubscriptionResponse_init(&response);
    UA_LOCK(&server->serviceMutex);
    Service_CreateSubscription(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    subscriptionId = response.subscriptionId;

    UA_CreateSubscriptionResponse_clear(&response);
}
END_TEST

START_TEST(Server_modifySubscription) {
    /* Create a subscription */
    createSubscription();

    /* Modify the subscription */
    UA_ModifySubscriptionRequest request;
    UA_ModifySubscriptionRequest_init(&request);
    request.subscriptionId = subscriptionId;
    // just some arbitrary numbers to test. They have no specific reason
    request.requestedPublishingInterval = 100; // in ms
    request.requestedLifetimeCount = 1000;
    request.requestedMaxKeepAliveCount = 1000;
    request.maxNotificationsPerPublish = 1;
    request.priority = 10;

    UA_ModifySubscriptionResponse response;
    UA_ModifySubscriptionResponse_init(&response);

    UA_LOCK(&server->serviceMutex);
    Service_ModifySubscription(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_ModifySubscriptionResponse_clear(&response);
}
END_TEST

START_TEST(Server_setPublishingMode) {
    createSubscription();

    UA_SetPublishingModeRequest request;
    UA_SetPublishingModeRequest_init(&request);
    request.publishingEnabled = UA_TRUE;
    request.subscriptionIdsSize = 1;
    request.subscriptionIds = &subscriptionId;

    UA_SetPublishingModeResponse response;
    UA_SetPublishingModeResponse_init(&response);

    UA_LOCK(&server->serviceMutex);
    Service_SetPublishingMode(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);

    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert_uint_eq(response.results[0], UA_STATUSCODE_GOOD);

    UA_SetPublishingModeResponse_clear(&response);
}
END_TEST

START_TEST(Server_republish) {
    createSubscription();

    UA_RepublishRequest request;
    UA_RepublishRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.retransmitSequenceNumber = 0;

    UA_RepublishResponse response;
    UA_RepublishResponse_init(&response);

    UA_LOCK(&server->serviceMutex);
    Service_Republish(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_BADMESSAGENOTAVAILABLE);

    UA_RepublishResponse_clear(&response);
}
END_TEST


START_TEST(Server_republish_invalid) {
    UA_RepublishRequest request;
    UA_RepublishRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.retransmitSequenceNumber = 0;

    UA_RepublishResponse response;
    UA_RepublishResponse_init(&response);

    UA_LOCK(&server->serviceMutex);
    Service_Republish(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID);

    UA_RepublishResponse_clear(&response);
}
END_TEST

START_TEST(Server_deleteSubscription) {
    createSubscription();

    /* Remove the subscription */
    UA_DeleteSubscriptionsRequest del_request;
    UA_DeleteSubscriptionsRequest_init(&del_request);
    del_request.subscriptionIdsSize = 1;
    del_request.subscriptionIds = &subscriptionId;

    UA_DeleteSubscriptionsResponse del_response;
    UA_DeleteSubscriptionsResponse_init(&del_response);

    UA_LOCK(&server->serviceMutex);
    Service_DeleteSubscriptions(server, session, &del_request, &del_response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(del_response.resultsSize, 1);
    ck_assert_uint_eq(del_response.results[0], UA_STATUSCODE_GOOD);

    UA_DeleteSubscriptionsResponse_clear(&del_response);
}
END_TEST

START_TEST(Server_publishCallback) {
    /* Create a subscription */
    UA_CreateSubscriptionRequest request;
    UA_CreateSubscriptionResponse response;

    UA_CreateSubscriptionRequest_init(&request);
    request.publishingEnabled = true;
    UA_CreateSubscriptionResponse_init(&response);
    UA_LOCK(&server->serviceMutex);
    Service_CreateSubscription(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subscriptionId1 = response.subscriptionId;
    UA_CreateSubscriptionResponse_clear(&response);

    /* Create a second subscription */
    UA_CreateSubscriptionRequest_init(&request);
    request.publishingEnabled = true;
    UA_CreateSubscriptionResponse_init(&response);
    UA_LOCK(&server->serviceMutex);
    Service_CreateSubscription(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subscriptionId2 = response.subscriptionId;
    UA_Double publishingInterval = response.revisedPublishingInterval;
    ck_assert(publishingInterval > 0.0f);
    UA_CreateSubscriptionResponse_clear(&response);

    /* Keepalive is set to max initially */
    UA_Subscription *sub;
    TAILQ_FOREACH(sub, &session->subscriptions, sessionListEntry)
        ck_assert_uint_eq(sub->currentKeepAliveCount, sub->maxKeepAliveCount);

    /* Sleep until the publishing interval times out */
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);
    UA_realSleep(100);

    TAILQ_FOREACH(sub, &session->subscriptions, sessionListEntry) {
        if ((sub->subscriptionId == subscriptionId1) || (sub->subscriptionId == subscriptionId2))
            ck_assert_uint_eq(sub->currentKeepAliveCount, sub->maxKeepAliveCount+1);
    }

    /* Remove the subscriptions */
    UA_DeleteSubscriptionsRequest del_request;
    UA_DeleteSubscriptionsRequest_init(&del_request);
    UA_UInt32 removeIds[2] = {subscriptionId1, subscriptionId2};
    del_request.subscriptionIdsSize = 2;
    del_request.subscriptionIds = removeIds;

    UA_DeleteSubscriptionsResponse del_response;
    UA_DeleteSubscriptionsResponse_init(&del_response);

    UA_LOCK(&server->serviceMutex);
    Service_DeleteSubscriptions(server, session, &del_request, &del_response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(del_response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(del_response.resultsSize, 2);
    ck_assert_uint_eq(del_response.results[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(del_response.results[1], UA_STATUSCODE_GOOD);

    UA_DeleteSubscriptionsResponse_clear(&del_response);
}
END_TEST

START_TEST(Server_createMonitoredItems) {
    createSubscription();
    createMonitoredItem();
}
END_TEST

START_TEST(Server_modifyMonitoredItems) {
    createSubscription();
    createMonitoredItem();

    UA_ModifyMonitoredItemsRequest request;
    UA_ModifyMonitoredItemsRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_SERVER;
    UA_MonitoredItemModifyRequest item;
    UA_MonitoredItemModifyRequest_init(&item);
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    rvi.attributeId = UA_ATTRIBUTEID_DESCRIPTION;
    rvi.indexRange = UA_STRING_NULL;
    item.monitoredItemId = monitoredItemId;
    UA_MonitoringParameters params;
    UA_MonitoringParameters_init(&params);
    item.requestedParameters = params;
    request.itemsToModifySize = 1;
    request.itemsToModify = &item;

    UA_ModifyMonitoredItemsResponse response;
    UA_ModifyMonitoredItemsResponse_init(&response);

    UA_LOCK(&server->serviceMutex);
    Service_ModifyMonitoredItems(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert_uint_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);

    UA_MonitoredItemModifyRequest_clear(&item);
    UA_ModifyMonitoredItemsResponse_clear(&response);
}
END_TEST

START_TEST(Server_overflow) {
    /* Create a subscription */
    UA_CreateSubscriptionRequest createSubscriptionRequest;
    UA_CreateSubscriptionResponse createSubscriptionResponse;

    UA_CreateSubscriptionRequest_init(&createSubscriptionRequest);
    createSubscriptionRequest.publishingEnabled = true;
    UA_CreateSubscriptionResponse_init(&createSubscriptionResponse);
    UA_LOCK(&server->serviceMutex);
    Service_CreateSubscription(server, session, &createSubscriptionRequest, &createSubscriptionResponse);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(createSubscriptionResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 localSubscriptionId = createSubscriptionResponse.subscriptionId;
    UA_Double publishingInterval = createSubscriptionResponse.revisedPublishingInterval;
    ck_assert(publishingInterval > 0.0f);
    UA_CreateSubscriptionResponse_clear(&createSubscriptionResponse);

    /* Create a monitoredItem */
    UA_CreateMonitoredItemsRequest createMonitoredItemsRequest;
    UA_CreateMonitoredItemsRequest_init(&createMonitoredItemsRequest);
    createMonitoredItemsRequest.subscriptionId = localSubscriptionId;
    createMonitoredItemsRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_SERVER;
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    rvi.indexRange = UA_STRING_NULL;
    item.itemToMonitor = rvi;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_MonitoringParameters params;
    UA_MonitoringParameters_init(&params);
    item.requestedParameters = params;
    item.requestedParameters.queueSize = 3;
    item.requestedParameters.discardOldest = true;
    createMonitoredItemsRequest.itemsToCreateSize = 1;
    createMonitoredItemsRequest.itemsToCreate = &item;

    UA_CreateMonitoredItemsResponse createMonitoredItemsResponse;
    UA_CreateMonitoredItemsResponse_init(&createMonitoredItemsResponse);

    UA_LOCK(&server->serviceMutex);
    Service_CreateMonitoredItems(server, session, &createMonitoredItemsRequest, &createMonitoredItemsResponse);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(createMonitoredItemsResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createMonitoredItemsResponse.resultsSize, 1);
    ck_assert_uint_eq(createMonitoredItemsResponse.results[0].statusCode, UA_STATUSCODE_GOOD);

    UA_UInt32 localMonitoredItemId = createMonitoredItemsResponse.results[0].monitoredItemId;
    ck_assert_uint_gt(localMonitoredItemId, 0);

    UA_MonitoredItemCreateRequest_clear(&item);
    UA_CreateMonitoredItemsResponse_clear(&createMonitoredItemsResponse);

    UA_MonitoredItem *mon = NULL;
    UA_Subscription *sub;
    TAILQ_FOREACH(sub, &session->subscriptions, sessionListEntry) {
        if(sub->subscriptionId == localSubscriptionId)
            mon = UA_Subscription_getMonitoredItem(sub, localMonitoredItemId);
    }
    ck_assert_ptr_ne(mon, NULL);
    UA_assert(mon);
    ck_assert_uint_eq(mon->queueSize, 1);
    ck_assert_uint_eq(mon->parameters.queueSize, 3);
    UA_Notification *notification;
    notification = TAILQ_LAST(&mon->queue, NotificationQueue);
    ck_assert_uint_eq(notification->data.dataChange.value.hasStatus, false);

    UA_fakeSleep(1); /* modify the server's currenttime */

    UA_MonitoredItem_sampleCallback(server, mon);
    ck_assert_uint_eq(mon->queueSize, 2);
    ck_assert_uint_eq(mon->parameters.queueSize, 3);
    notification = TAILQ_LAST(&mon->queue, NotificationQueue);
    ck_assert_uint_eq(notification->data.dataChange.value.hasStatus, false);

    UA_fakeSleep(1); /* modify the server's currenttime */

    UA_MonitoredItem_sampleCallback(server, mon);
    ck_assert_uint_eq(mon->queueSize, 3);
    ck_assert_uint_eq(mon->parameters.queueSize, 3);
    notification = TAILQ_LAST(&mon->queue, NotificationQueue);
    ck_assert_uint_eq(notification->data.dataChange.value.hasStatus, false);

    UA_fakeSleep(1); /* modify the server's currenttime */

    UA_MonitoredItem_sampleCallback(server, mon);
    ck_assert_uint_eq(mon->queueSize, 3);
    ck_assert_uint_eq(mon->parameters.queueSize, 3);
    notification = TAILQ_FIRST(&mon->queue);
    ck_assert_uint_eq(notification->data.dataChange.value.hasStatus, true);
    ck_assert_uint_eq(notification->data.dataChange.value.status,
                      UA_STATUSCODE_INFOTYPE_DATAVALUE | UA_STATUSCODE_INFOBITS_OVERFLOW);

    /* Remove status for next test */
    notification->data.dataChange.value.hasStatus = false;
    notification->data.dataChange.value.status = 0;

    /* Modify the MonitoredItem */
    UA_ModifyMonitoredItemsRequest modifyMonitoredItemsRequest;
    UA_ModifyMonitoredItemsRequest_init(&modifyMonitoredItemsRequest);
    modifyMonitoredItemsRequest.subscriptionId = localSubscriptionId;
    modifyMonitoredItemsRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_SERVER;
    UA_MonitoredItemModifyRequest itemToModify;
    UA_MonitoredItemModifyRequest_init(&itemToModify);
    itemToModify.monitoredItemId = localMonitoredItemId;
    UA_MonitoringParameters_init(&params);
    itemToModify.requestedParameters = params;
    itemToModify.requestedParameters.queueSize = 2;
    itemToModify.requestedParameters.discardOldest = true;
    modifyMonitoredItemsRequest.itemsToModifySize = 1;
    modifyMonitoredItemsRequest.itemsToModify = &itemToModify;

    UA_ModifyMonitoredItemsResponse modifyMonitoredItemsResponse;
    UA_ModifyMonitoredItemsResponse_init(&modifyMonitoredItemsResponse);

    UA_LOCK(&server->serviceMutex);
    Service_ModifyMonitoredItems(server, session, &modifyMonitoredItemsRequest,
                                 &modifyMonitoredItemsResponse);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(modifyMonitoredItemsResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(modifyMonitoredItemsResponse.resultsSize, 1);
    ck_assert_uint_eq(modifyMonitoredItemsResponse.results[0].statusCode, UA_STATUSCODE_GOOD);

    UA_MonitoredItemModifyRequest_clear(&itemToModify);
    UA_ModifyMonitoredItemsResponse_clear(&modifyMonitoredItemsResponse);

    ck_assert_uint_eq(mon->queueSize, 2);
    ck_assert_uint_eq(mon->parameters.queueSize, 2);
    notification = TAILQ_FIRST(&mon->queue);
    ck_assert_uint_eq(notification->data.dataChange.value.hasStatus, true);
    ck_assert_uint_eq(notification->data.dataChange.value.status,
                      UA_STATUSCODE_INFOTYPE_DATAVALUE | UA_STATUSCODE_INFOBITS_OVERFLOW);

    /* Modify the MonitoredItem */
    UA_ModifyMonitoredItemsRequest_init(&modifyMonitoredItemsRequest);
    modifyMonitoredItemsRequest.subscriptionId = localSubscriptionId;
    modifyMonitoredItemsRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_SERVER;
    UA_MonitoredItemModifyRequest_init(&itemToModify);
    itemToModify.monitoredItemId = localMonitoredItemId;
    UA_MonitoringParameters_init(&params);
    itemToModify.requestedParameters = params;
    itemToModify.requestedParameters.queueSize = 1;
    modifyMonitoredItemsRequest.itemsToModifySize = 1;
    modifyMonitoredItemsRequest.itemsToModify = &itemToModify;

    UA_ModifyMonitoredItemsResponse_init(&modifyMonitoredItemsResponse);

    UA_LOCK(&server->serviceMutex);
    Service_ModifyMonitoredItems(server, session, &modifyMonitoredItemsRequest,
                                 &modifyMonitoredItemsResponse);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(modifyMonitoredItemsResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(modifyMonitoredItemsResponse.resultsSize, 1);
    ck_assert_uint_eq(modifyMonitoredItemsResponse.results[0].statusCode, UA_STATUSCODE_GOOD);

    UA_MonitoredItemModifyRequest_clear(&itemToModify);
    UA_ModifyMonitoredItemsResponse_clear(&modifyMonitoredItemsResponse);

    ck_assert_uint_eq(mon->queueSize, 1);
    ck_assert_uint_eq(mon->parameters.queueSize, 1);
    notification = TAILQ_LAST(&mon->queue, NotificationQueue);
    ck_assert_uint_eq(notification->data.dataChange.value.hasStatus, false);

    /* Modify the MonitoredItem */
    UA_ModifyMonitoredItemsRequest_init(&modifyMonitoredItemsRequest);
    modifyMonitoredItemsRequest.subscriptionId = localSubscriptionId;
    modifyMonitoredItemsRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_SERVER;
    UA_MonitoredItemModifyRequest_init(&itemToModify);
    itemToModify.monitoredItemId = localMonitoredItemId;
    UA_MonitoringParameters_init(&params);
    itemToModify.requestedParameters = params;
    itemToModify.requestedParameters.discardOldest = false;
    itemToModify.requestedParameters.queueSize = 1;
    modifyMonitoredItemsRequest.itemsToModifySize = 1;
    modifyMonitoredItemsRequest.itemsToModify = &itemToModify;

    UA_ModifyMonitoredItemsResponse_init(&modifyMonitoredItemsResponse);

    UA_LOCK(&server->serviceMutex);
    Service_ModifyMonitoredItems(server, session, &modifyMonitoredItemsRequest,
                                 &modifyMonitoredItemsResponse);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(modifyMonitoredItemsResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(modifyMonitoredItemsResponse.resultsSize, 1);
    ck_assert_uint_eq(modifyMonitoredItemsResponse.results[0].statusCode, UA_STATUSCODE_GOOD);

    UA_MonitoredItemModifyRequest_clear(&itemToModify);
    UA_ModifyMonitoredItemsResponse_clear(&modifyMonitoredItemsResponse);

    UA_MonitoredItem_sampleCallback(server, mon);
    ck_assert_uint_eq(mon->queueSize, 1);
    ck_assert_uint_eq(mon->parameters.queueSize, 1);
    notification = TAILQ_FIRST(&mon->queue);
    ck_assert_uint_eq(notification->data.dataChange.value.hasStatus, false); /* the infobit is only set if the queue is larger than one */

    /* Remove the subscriptions */
    UA_DeleteSubscriptionsRequest deleteSubscriptionsRequest;
    UA_DeleteSubscriptionsRequest_init(&deleteSubscriptionsRequest);
    UA_UInt32 removeId = localSubscriptionId;
    deleteSubscriptionsRequest.subscriptionIdsSize = 1;
    deleteSubscriptionsRequest.subscriptionIds = &removeId;

    UA_DeleteSubscriptionsResponse deleteSubscriptionsResponse;
    UA_DeleteSubscriptionsResponse_init(&deleteSubscriptionsResponse);

    UA_LOCK(&server->serviceMutex);
    Service_DeleteSubscriptions(server, session, &deleteSubscriptionsRequest,
                                &deleteSubscriptionsResponse);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(deleteSubscriptionsResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteSubscriptionsResponse.resultsSize, 1);
    ck_assert_uint_eq(deleteSubscriptionsResponse.results[0], UA_STATUSCODE_GOOD);

    UA_DeleteSubscriptionsResponse_clear(&deleteSubscriptionsResponse);

}
END_TEST

START_TEST(Server_setMonitoringMode) {
    createSubscription();
    createMonitoredItem();

    UA_SetMonitoringModeRequest request;
    UA_SetMonitoringModeRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.monitoringMode = UA_MONITORINGMODE_DISABLED;
    request.monitoredItemIdsSize = 1;
    request.monitoredItemIds = &monitoredItemId;

    UA_SetMonitoringModeResponse response;
    UA_SetMonitoringModeResponse_init(&response);

    UA_LOCK(&server->serviceMutex);
    Service_SetMonitoringMode(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert_uint_eq(response.results[0], UA_STATUSCODE_GOOD);

    UA_SetMonitoringModeResponse_clear(&response);
}
END_TEST

START_TEST(Server_deleteMonitoredItems) {
    createSubscription();
    createMonitoredItem();

    UA_DeleteMonitoredItemsRequest request;
    UA_DeleteMonitoredItemsRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.monitoredItemIdsSize = 1;
    request.monitoredItemIds = &monitoredItemId;

    UA_DeleteMonitoredItemsResponse response;
    UA_DeleteMonitoredItemsResponse_init(&response);

    UA_LOCK(&server->serviceMutex);
    Service_DeleteMonitoredItems(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert_uint_eq(response.results[0], UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&response);
}
END_TEST

START_TEST(Server_lifeTimeCount) {
    /* Create a subscription */
    UA_CreateSubscriptionRequest request;
    UA_CreateSubscriptionResponse response;

    UA_CreateSubscriptionRequest_init(&request);
    request.publishingEnabled = true;
    request.requestedLifetimeCount = 3;
    request.requestedMaxKeepAliveCount = 1;
    UA_CreateSubscriptionResponse_init(&response);
    UA_LOCK(&server->serviceMutex);
    Service_CreateSubscription(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.revisedMaxKeepAliveCount, 1);
    ck_assert_uint_eq(response.revisedLifetimeCount, 3);
    UA_CreateSubscriptionResponse_clear(&response);

    /* Create a second subscription */
    UA_CreateSubscriptionRequest_init(&request);
    request.publishingEnabled = true;
    request.requestedLifetimeCount = 4;
    request.requestedMaxKeepAliveCount = 2;
    UA_CreateSubscriptionResponse_init(&response);
    UA_LOCK(&server->serviceMutex);
    Service_CreateSubscription(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.revisedMaxKeepAliveCount, 2);
    /* revisedLifetimeCount is revised to 3*MaxKeepAliveCount == 3 */
    ck_assert_uint_eq(response.revisedLifetimeCount, 6);
    UA_Double publishingInterval = response.revisedPublishingInterval;
    ck_assert(publishingInterval > 0.0f);
    subscriptionId = response.subscriptionId;
    UA_CreateSubscriptionResponse_clear(&response);

    /* Add a MonitoredItem to the second subscription */
    UA_CreateMonitoredItemsRequest mrequest;
    UA_CreateMonitoredItemsRequest_init(&mrequest);
    mrequest.subscriptionId = subscriptionId;
    mrequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_SERVER;
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    rvi.attributeId = UA_ATTRIBUTEID_BROWSENAME;
    rvi.indexRange = UA_STRING_NULL;
    item.itemToMonitor = rvi;
    item.requestedParameters.samplingInterval = publishingInterval / 5.0;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_MonitoringParameters params;
    UA_MonitoringParameters_init(&params);
    item.requestedParameters = params;
    mrequest.itemsToCreateSize = 1;
    mrequest.itemsToCreate = &item;

    UA_CreateMonitoredItemsResponse mresponse;
    UA_CreateMonitoredItemsResponse_init(&mresponse);
    UA_LOCK(&server->serviceMutex);
    Service_CreateMonitoredItems(server, session, &mrequest, &mresponse);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(mresponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(mresponse.resultsSize, 1);
    ck_assert_uint_eq(mresponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = mresponse.results[0].monitoredItemId;
    ck_assert_uint_gt(monitoredItemId, 0);
    UA_MonitoredItemCreateRequest_clear(&item);
    UA_CreateMonitoredItemsResponse_clear(&mresponse);

    UA_Server_run_iterate(server, false);
    UA_UInt32 count = 0;
    UA_Subscription *sub;
    TAILQ_FOREACH(sub, &session->subscriptions, sessionListEntry) {
        ck_assert_uint_eq(sub->currentLifetimeCount, 0);
        count++;
    }
    ck_assert_uint_eq(count, 2);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    count = 0;
    TAILQ_FOREACH(sub, &session->subscriptions, sessionListEntry) {
        ck_assert_uint_eq(sub->currentLifetimeCount, 1);
        count++;
    }
    ck_assert_uint_eq(count, 2);

    /* Sleep until the publishing interval times out */
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    count = 0;
    TAILQ_FOREACH(sub, &session->subscriptions, sessionListEntry) {
        ck_assert_uint_eq(sub->currentLifetimeCount, 2);
        count++;
    }
    ck_assert_uint_eq(count, 2);

    /* Sleep until the publishing interval times out */
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    count = 0;
    TAILQ_FOREACH(sub, &session->subscriptions, sessionListEntry) {
        ck_assert_uint_eq(sub->currentLifetimeCount, 3);
        count++;
    }
    ck_assert_uint_eq(count, 2);

    /* Sleep until the publishing interval times out */
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    count = 0;
    TAILQ_FOREACH(sub, &session->subscriptions, sessionListEntry) {
        if(sub->statusChange == UA_STATUSCODE_GOOD) {
            ck_assert_uint_eq(sub->currentLifetimeCount, 4);
            count++;
        }
    }
    ck_assert_uint_eq(count, 1);

    /* Sleep until the publishing interval times out */
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    count = 0;
    TAILQ_FOREACH(sub, &session->subscriptions, sessionListEntry) {
        if(sub->statusChange == UA_STATUSCODE_GOOD) {
            ck_assert_uint_eq(sub->currentLifetimeCount, 5);
            count++;
        }
    }
    ck_assert_uint_eq(count, 1);

    /* Sleep until the publishing interval times out */
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    count = 0;
    TAILQ_FOREACH(sub, &session->subscriptions, sessionListEntry) {
        if(sub->statusChange == UA_STATUSCODE_GOOD) {
            ck_assert_uint_eq(sub->currentLifetimeCount, 6);
            count++;
        }
    }
    ck_assert_uint_eq(count, 1);

    /* Sleep until the publishing interval times out. The next iteration removes
     * the subscription. */
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    count = 0;
    TAILQ_FOREACH(sub, &session->subscriptions, sessionListEntry) {
        if(sub->statusChange == UA_STATUSCODE_GOOD)
            count++;
    }
    ck_assert_uint_eq(count, 0);
}
END_TEST

START_TEST(Server_invalidPublishingInterval) {
    UA_Double savedPublishingIntervalLimitsMin = server->config.publishingIntervalLimits.min;
    server->config.publishingIntervalLimits.min = 1;
    /* Create a subscription */
    UA_CreateSubscriptionRequest request;
    UA_CreateSubscriptionResponse response;

    UA_CreateSubscriptionRequest_init(&request);
    request.publishingEnabled = true;
    request.requestedPublishingInterval = -5.0; // Must be positive
    UA_CreateSubscriptionResponse_init(&response);
    UA_LOCK(&server->serviceMutex);
    Service_CreateSubscription(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert(response.revisedPublishingInterval ==
              server->config.publishingIntervalLimits.min);
    UA_CreateSubscriptionResponse_clear(&response);

    server->config.publishingIntervalLimits.min = savedPublishingIntervalLimitsMin;
}
END_TEST

START_TEST(Server_invalidSamplingInterval) {
    createSubscription();

    UA_Double savedSamplingIntervalLimitsMin = server->config.samplingIntervalLimits.min;
    server->config.samplingIntervalLimits.min = 1;

    UA_CreateMonitoredItemsRequest request;
    UA_CreateMonitoredItemsRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_SERVER;
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    rvi.attributeId = UA_ATTRIBUTEID_BROWSENAME;
    rvi.indexRange = UA_STRING_NULL;
    item.itemToMonitor = rvi;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_MonitoringParameters params;
    UA_MonitoringParameters_init(&params);
    params.samplingInterval = -5.0; // Must be positive
    item.requestedParameters = params;
    request.itemsToCreateSize = 1;
    request.itemsToCreate = &item;

    UA_CreateMonitoredItemsResponse response;
    UA_CreateMonitoredItemsResponse_init(&response);
    UA_LOCK(&server->serviceMutex);
    Service_CreateMonitoredItems(server, session, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert_uint_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert(response.results[0].revisedSamplingInterval == 0.0);

    UA_MonitoredItemCreateRequest_clear(&item);
    UA_CreateMonitoredItemsResponse_clear(&response);

    server->config.samplingIntervalLimits.min = savedSamplingIntervalLimitsMin;
}
END_TEST

#endif /* UA_ENABLE_SUBSCRIPTIONS */

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server Subscription");
    TCase *tc_server = tcase_create("Server Subscription Basic");
    tcase_add_checked_fixture(tc_server, setup, teardown);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    tcase_add_test(tc_server, Server_createSubscription);
    tcase_add_test(tc_server, Server_modifySubscription);
    tcase_add_test(tc_server, Server_setPublishingMode);
    tcase_add_test(tc_server, Server_invalidSamplingInterval);
    tcase_add_test(tc_server, Server_createMonitoredItems);
    tcase_add_test(tc_server, Server_modifyMonitoredItems);
    tcase_add_test(tc_server, Server_overflow);
    tcase_add_test(tc_server, Server_setMonitoringMode);
    tcase_add_test(tc_server, Server_deleteMonitoredItems);
    tcase_add_test(tc_server, Server_republish);
    tcase_add_test(tc_server, Server_republish_invalid);
    tcase_add_test(tc_server, Server_deleteSubscription);
    tcase_add_test(tc_server, Server_publishCallback);
    tcase_add_test(tc_server, Server_lifeTimeCount);
    tcase_add_test(tc_server, Server_invalidPublishingInterval);
#endif /* UA_ENABLE_SUBSCRIPTIONS */
    suite_add_tcase(s, tc_server);

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
