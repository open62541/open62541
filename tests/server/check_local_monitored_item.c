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

#include <stddef.h>

#include "check.h"
#include "testing_clock.h"
#include "testing_networklayers.h"

#ifdef UA_ENABLE_STATUSCODE_DESCRIPTIONS
    #define ASSERT_STATUSCODE(a,b) ck_assert_str_eq(UA_StatusCode_name(a),UA_StatusCode_name(b));
#else
    #define ASSERT_STATUSCODE(a,b) ck_assert_uint_eq((a),(b));
#endif

UA_Server *server;
size_t callbackCount = 0;

UA_NodeId parentNodeId;
UA_NodeId parentReferenceNodeId;
UA_NodeId outNodeId;

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    UA_StatusCode retval = UA_Server_run_startup(server);
    ASSERT_STATUSCODE(retval, UA_STATUSCODE_GOOD);
    /* Define the attribute of the uint32 variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_UInt32 myUint32 = 40;
    UA_Variant_setScalar(&attr.value, &myUint32, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
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
    UA_NodeId_deleteMembers(&parentNodeId);
    UA_NodeId_deleteMembers(&parentReferenceNodeId);
    UA_NodeId_deleteMembers(&outNodeId);
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

START_TEST(Server_LocalMonitoredItem) {
    ck_assert_uint_eq(callbackCount, 0);

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

static Suite* testSuite_Client(void)
{
    Suite *s = suite_create("Local Monitored Item");
    TCase *tc_server = tcase_create("Local Monitored Item Basic");
    tcase_add_checked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_LocalMonitoredItem);
    suite_add_tcase(s, tc_server);

    return s;
}

int main(void)
{
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
