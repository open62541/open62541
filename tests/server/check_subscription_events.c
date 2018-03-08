/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_server.h"
#include "server/ua_services.h"
#include "server/ua_server_internal.h"
#include "server/ua_subscription.h"
#include "ua_config_default.h"

#include "check.h"
#include "testing_clock.h"

static UA_Server *server = NULL;
static UA_ServerConfig *config = NULL;

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

static UA_UInt32 subscriptionId;
static UA_UInt32 monitoredItemId;
static UA_NodeId eventType;
static size_t nSelectClauses = 1;
static UA_SimpleAttributeOperand *ptr_filter = NULL;

static UA_SimpleAttributeOperand *setupSelectClauses(void) {
    // only creating 1 selectClause for now, if more things are to be checked it can be changed easily
    UA_SimpleAttributeOperand *selectClauses = (UA_SimpleAttributeOperand*)
            UA_Array_new(nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    if(!selectClauses)
        return NULL;

    ptr_filter = selectClauses;
    for(size_t i =0; i<nSelectClauses; ++i) {
        UA_SimpleAttributeOperand_init(&selectClauses[i]);
    }

    selectClauses[0].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectClauses[0].browsePathSize = 1;
    selectClauses[0].browsePath = (UA_QualifiedName*)
            UA_Array_new(selectClauses[0].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectClauses[0].browsePath) {
        UA_SimpleAttributeOperand_delete(selectClauses);
        return NULL;
    }
    selectClauses[0].attributeId = UA_ATTRIBUTEID_VALUE;
    selectClauses[0].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Severity");

    return selectClauses;
}


// create a subscription and add a monitored item to it
static void setupSubscription(void) {
    // Create subscription
    UA_CreateSubscriptionRequest subRequest;
    UA_CreateSubscriptionRequest_init(&subRequest);
    subRequest.publishingEnabled = true;

    UA_CreateSubscriptionResponse subResponse;
    UA_CreateSubscriptionResponse_init(&subResponse);

    Service_CreateSubscription(server, &adminSession, &subRequest, &subResponse);
    subscriptionId = subResponse.subscriptionId;

    UA_CreateSubscriptionResponse_deleteMembers(&subResponse);

    // Create monitored item
    UA_CreateMonitoredItemsRequest monRequest;
    UA_CreateMonitoredItemsRequest_init(&monRequest);
    monRequest.subscriptionId = subscriptionId;
    monRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_SERVER;
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    rvi.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    rvi.indexRange = UA_STRING_NULL;
    item.itemToMonitor = rvi;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;

    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    filter.selectClauses = setupSelectClauses();
    filter.selectClausesSize = nSelectClauses;
    item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    item.requestedParameters.filter.content.decoded.data = &filter;
    item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];

    monRequest.itemsToCreateSize = 1;
    monRequest.itemsToCreate = &item;

    UA_CreateMonitoredItemsResponse monResponse;
    UA_CreateMonitoredItemsResponse_init(&monResponse);

    Service_CreateMonitoredItems(server, &adminSession, &monRequest, &monResponse);
    monitoredItemId = monResponse.results[0].monitoredItemId;

    UA_CreateMonitoredItemsResponse_deleteMembers(&monResponse);
}

static void addNewEventType(void) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "SimpleEventType");
    attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "The simple event type we created");

    UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(0, "SimpleEventType"),
                                attr, NULL, &eventType);
    UA_LocalizedText_deleteMembers(&attr.displayName);
    UA_LocalizedText_deleteMembers(&attr.description);
}
static void setup(void) {
    config = UA_ServerConfig_new_default();
    server = UA_Server_new(config);
    UA_Server_run_startup(server);
    setupSubscription();
    addNewEventType();
}

static void removeSubscription(void) {
    /* Remove the subscriptions */
    UA_DeleteSubscriptionsRequest deleteSubscriptionsRequest;
    UA_DeleteSubscriptionsRequest_init(&deleteSubscriptionsRequest);
    UA_UInt32 removeId = subscriptionId;
    deleteSubscriptionsRequest.subscriptionIdsSize = 1;
    deleteSubscriptionsRequest.subscriptionIds = &removeId;

    UA_DeleteSubscriptionsResponse deleteSubscriptionsResponse;
    UA_DeleteSubscriptionsResponse_init(&deleteSubscriptionsResponse);

    Service_DeleteSubscriptions(server, &adminSession, &deleteSubscriptionsRequest,
                                &deleteSubscriptionsResponse);
    UA_DeleteSubscriptionsResponse_deleteMembers(&deleteSubscriptionsResponse);
}

static void teardown(void) {
    removeSubscription();
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    UA_Array_delete(ptr_filter, nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
}

// ensure events are received with proper values
START_TEST(generateEvents) {
    UA_StatusCode retval;
    UA_NodeId eventNodeId;
    retval = UA_Server_createEvent(server, eventType, &eventNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    // add a severity to the event
    UA_Variant value;
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = eventNodeId;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;
    rpe.targetName = UA_QUALIFIEDNAME(0, "Severity");
    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    // number with no special meaning
    UA_UInt16 eventSeverity = 1000;
    UA_Variant_setScalar(&value, &eventSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    UA_BrowsePathResult_deleteMembers(&bpr);

    // trigger the event
    UA_ByteString eventId;
    retval = UA_Server_triggerEvent(server, &eventNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_ByteString_deleteMembers(&eventId);

    // get the monitored item to check whether the event is in the queue

}
END_TEST

// ensures an eventQueueOverflowEvent is published when appropriate
START_TEST(eventOverflow) {
    return;
}
END_TEST

#endif //UA_ENABLE_SUBSCRIPTIONS_EVENTS

//assumes subscriptions work fine with data change because of other unit test
static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server Subscription Events");
    TCase *tc_server = tcase_create("Server Subscription Events");
    tcase_add_checked_fixture(tc_server, setup, teardown);
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    tcase_add_test(tc_server, generateEvents);
    tcase_add_test(tc_server, eventOverflow);
#endif //UA_ENABLE_SUBSCRIPTIONS_EVENTS
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

