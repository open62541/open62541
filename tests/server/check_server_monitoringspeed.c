/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* This example is just to see how fast we can monitor value changes. The server does
   not open a TCP port. */

#include <open62541/server_config_default.h>

#include "server/ua_subscription.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdio.h>
#include <time.h>

#include "testing_networklayers.h"
#include "testing_policy.h"

static UA_SecureChannel testChannel;
static UA_SecurityPolicy dummyPolicy;
static UA_Connection testingConnection;
static funcs_called funcsCalled;
static key_sizes keySizes;
static UA_Server *server;

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    TestingPolicy(&dummyPolicy, UA_BYTESTRING_NULL, &funcsCalled, &keySizes);
    UA_SecureChannel_init(&testChannel);
    UA_SecureChannel_setSecurityPolicy(&testChannel, &dummyPolicy, &UA_BYTESTRING_NULL);

    testingConnection = createDummyConnection(65535, NULL);
    UA_Connection_attachSecureChannel(&testingConnection, &testChannel);
    testChannel.connection = &testingConnection;
}

static void teardown(void) {
    UA_SecureChannel_close(&testChannel);
    UA_SecureChannel_deleteMembers(&testChannel);
    dummyPolicy.deleteMembers(&dummyPolicy);
    testingConnection.close(&testingConnection);

    UA_Server_delete(server);
}

static size_t callbackCount = 0;

static void
dataChangeNotificationCallback(UA_Server *s, UA_UInt32 monitoredItemId,
                               void *monitoredItemContext, const UA_NodeId *nodeId,
                               void *nodeContext, UA_UInt32 attributeId,
                               const UA_DataValue *value) {
    callbackCount++;
}

START_TEST(monitorIntegerNoChanges) {
    /* add a variable node to the address space */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_StatusCode retval = UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                                                     parentReferenceNodeId, myIntegerName,
                                                     UA_NODEID_NULL, attr, NULL, NULL);
    UA_assert(retval == UA_STATUSCODE_GOOD);

    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = myIntegerNodeId;
    item.itemToMonitor.attributeId = UA_ATTRIBUTEID_VALUE;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_NEITHER,
                                            item, NULL, dataChangeNotificationCallback);

    callbackCount = 0;

    UA_MonitoredItem *mon = LIST_FIRST(&server->localMonitoredItems);

    clock_t begin, finish;
    begin = clock();

    for(int i = 0; i < 1000000; i++) {
        UA_MonitoredItem_sampleCallback(server, mon);
    }

    finish = clock();

    double time_spent = (double)(finish - begin) / CLOCKS_PER_SEC;
    printf("duration was %f s\n", time_spent);
    printf("retval is %s\n", UA_StatusCode_name(retval));

    UA_assert(callbackCount == 0);
}
END_TEST

static Suite * monitoring_speed_suite (void) {
    Suite *s = suite_create ("Monitoring Speed");

    TCase* tc_datachange = tcase_create ("DataChange");
    tcase_add_checked_fixture(tc_datachange, setup, teardown);
    tcase_add_test (tc_datachange, monitorIntegerNoChanges);
    suite_add_tcase (s, tc_datachange);

    return s;
}

int main (void) {
    int number_failed = 0;
    Suite *s = monitoring_speed_suite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr,CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed (sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
