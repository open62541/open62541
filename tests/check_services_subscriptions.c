/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this 
*  file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include "ua_server.h"
#include "server/ua_services.h"
#include "server/ua_server_internal.h"
#include "server/ua_subscription.h"
#include "ua_config_standard.h"

#include "check.h"
#include "testing_clock.h"

UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_new(UA_ServerConfig_standard);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

UA_UInt32 subscriptionId;
UA_UInt32 monitoredItemId;

START_TEST(Server_createSubscription) {
    /* Create a subscription */
    UA_CreateSubscriptionRequest request;
    UA_CreateSubscriptionRequest_init(&request);
    request.publishingEnabled = true;

    UA_CreateSubscriptionResponse response;
    UA_CreateSubscriptionResponse_init(&response);

    Service_CreateSubscription(server, &adminSession, &request, &response);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    subscriptionId = response.subscriptionId;

    UA_CreateSubscriptionResponse_deleteMembers(&response);
}
END_TEST

START_TEST(Server_modifySubscription) {
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

    Service_ModifySubscription(server, &adminSession, &request, &response);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_ModifySubscriptionResponse_deleteMembers(&response);
}
END_TEST

START_TEST(Server_setPublishingMode) {
    UA_SetPublishingModeRequest request;
    UA_SetPublishingModeRequest_init(&request);
    request.publishingEnabled = UA_TRUE;
    request.subscriptionIdsSize = 1;
    request.subscriptionIds = &subscriptionId;

    UA_SetPublishingModeResponse response;
    UA_SetPublishingModeResponse_init(&response);

    Service_SetPublishingMode(server, &adminSession, &request, &response);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert_uint_eq(response.results[0], UA_STATUSCODE_GOOD);

    UA_SetPublishingModeResponse_deleteMembers(&response);
}
END_TEST

START_TEST(Server_republish) {
    UA_RepublishRequest request;
    UA_RepublishRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.retransmitSequenceNumber = 0;

    UA_RepublishResponse response;
    UA_RepublishResponse_init(&response);

    Service_Republish(server, &adminSession, &request, &response);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_BADMESSAGENOTAVAILABLE);

    UA_RepublishResponse_deleteMembers(&response);

}
END_TEST


START_TEST(Server_republish_invalid) {
    UA_RepublishRequest request;
    UA_RepublishRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.retransmitSequenceNumber = 0;

    UA_RepublishResponse response;
    UA_RepublishResponse_init(&response);

    Service_Republish(server, &adminSession, &request, &response);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID);

    UA_RepublishResponse_deleteMembers(&response);

}
END_TEST

START_TEST(Server_deleteSubscription) {
    /* Remove the subscription */
    UA_DeleteSubscriptionsRequest del_request;
    UA_DeleteSubscriptionsRequest_init(&del_request);
    del_request.subscriptionIdsSize = 1;
    del_request.subscriptionIds = &subscriptionId;

    UA_DeleteSubscriptionsResponse del_response;
    UA_DeleteSubscriptionsResponse_init(&del_response);

    Service_DeleteSubscriptions(server, &adminSession, &del_request, &del_response);
    ck_assert_uint_eq(del_response.resultsSize, 1);
    ck_assert_uint_eq(del_response.results[0], UA_STATUSCODE_GOOD);

    UA_DeleteSubscriptionsResponse_deleteMembers(&del_response);
}
END_TEST

START_TEST(Server_publishCallback) {
    /* Create a subscription */
    UA_CreateSubscriptionRequest request;
    UA_CreateSubscriptionResponse response;

    UA_CreateSubscriptionRequest_init(&request);
    request.publishingEnabled = true;
    UA_CreateSubscriptionResponse_init(&response);
    Service_CreateSubscription(server, &adminSession, &request, &response);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subscriptionId1 = response.subscriptionId;
    UA_CreateSubscriptionResponse_deleteMembers(&response);

    /* Create a second subscription */
    UA_CreateSubscriptionRequest_init(&request);
    request.publishingEnabled = true;
    UA_CreateSubscriptionResponse_init(&response);
    Service_CreateSubscription(server, &adminSession, &request, &response);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subscriptionId2 = response.subscriptionId;
    UA_Double publishingInterval = response.revisedPublishingInterval;
    ck_assert(publishingInterval > 0.0f);
    UA_CreateSubscriptionResponse_deleteMembers(&response);

    /* Sleep until the publishing interval times out */
    UA_sleep((UA_DateTime)publishingInterval + 1);

    /* Keepalive is set to max initially */
    UA_Subscription *sub;
    LIST_FOREACH(sub, &adminSession.serverSubscriptions, listEntry)
        ck_assert_uint_eq(sub->currentKeepAliveCount, sub->maxKeepAliveCount);

    UA_Server_run_iterate(server, false);
#ifdef UA_ENABLE_MULTITHREADING
    UA_sleep(publishingInterval + 1);
    UA_Server_run_iterate(server, false);
    UA_sleep(publishingInterval + 1);
#endif

    LIST_FOREACH(sub, &adminSession.serverSubscriptions, listEntry)
        ck_assert_uint_eq(sub->currentKeepAliveCount, sub->maxKeepAliveCount+1);

    /* Remove the subscriptions */
    UA_DeleteSubscriptionsRequest del_request;
    UA_DeleteSubscriptionsRequest_init(&del_request);
    UA_UInt32 removeIds[2] = {subscriptionId1, subscriptionId2};
    del_request.subscriptionIdsSize = 2;
    del_request.subscriptionIds = removeIds;

    UA_DeleteSubscriptionsResponse del_response;
    UA_DeleteSubscriptionsResponse_init(&del_response);

    Service_DeleteSubscriptions(server, &adminSession, &del_request, &del_response);
    ck_assert_uint_eq(del_response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(del_response.resultsSize, 2);
    ck_assert_uint_eq(del_response.results[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(del_response.results[1], UA_STATUSCODE_GOOD);

    UA_DeleteSubscriptionsResponse_deleteMembers(&del_response);
}
END_TEST

START_TEST(Server_createMonitoredItems) {

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

    Service_CreateMonitoredItems(server, &adminSession, &request, &response);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert_uint_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);

    monitoredItemId = response.results[0].monitoredItemId;

    UA_MonitoredItemCreateRequest_deleteMembers(&item);

    UA_CreateMonitoredItemsResponse_deleteMembers(&response);
}
END_TEST

START_TEST(Server_modifyMonitoredItems) {
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

    Service_ModifyMonitoredItems(server, &adminSession, &request, &response);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert_uint_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);

    UA_MonitoredItemModifyRequest_deleteMembers(&item);

    UA_ModifyMonitoredItemsResponse_deleteMembers(&response);
}
END_TEST

START_TEST(Server_setMonitoringMode) {
    UA_SetMonitoringModeRequest request;
    UA_SetMonitoringModeRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.monitoringMode = UA_MONITORINGMODE_DISABLED;
    request.monitoredItemIdsSize = 1;
    request.monitoredItemIds = &monitoredItemId;

    UA_SetMonitoringModeResponse response;
    UA_SetMonitoringModeResponse_init(&response);

    Service_SetMonitoringMode(server, &adminSession, &request, &response);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert_uint_eq(response.results[0], UA_STATUSCODE_GOOD);


    UA_SetMonitoringModeResponse_deleteMembers(&response);
}
END_TEST

START_TEST(Server_deleteMonitoredItems) {
    UA_DeleteMonitoredItemsRequest request;
    UA_DeleteMonitoredItemsRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.monitoredItemIdsSize = 1;
    request.monitoredItemIds = &monitoredItemId;

    UA_DeleteMonitoredItemsResponse response;
    UA_DeleteMonitoredItemsResponse_init(&response);

    Service_DeleteMonitoredItems(server, &adminSession, &request, &response);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert_uint_eq(response.results[0], UA_STATUSCODE_GOOD);


    UA_DeleteMonitoredItemsResponse_deleteMembers(&response);

}
END_TEST


static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server Subscription");
    TCase *tc_server = tcase_create("Server Subscription Basic");
    tcase_add_checked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_createSubscription);
    tcase_add_test(tc_server, Server_modifySubscription);
    tcase_add_test(tc_server, Server_setPublishingMode);
    tcase_add_test(tc_server, Server_createMonitoredItems);
    tcase_add_test(tc_server, Server_modifyMonitoredItems);
    tcase_add_test(tc_server, Server_setMonitoringMode);
    tcase_add_test(tc_server, Server_deleteMonitoredItems);
    tcase_add_test(tc_server, Server_republish);
    tcase_add_test(tc_server, Server_deleteSubscription);
    tcase_add_test(tc_server, Server_republish_invalid);
    tcase_add_test(tc_server, Server_publishCallback);
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
