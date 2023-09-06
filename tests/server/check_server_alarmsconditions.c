/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "test_helpers.h"

#include <check.h>

UA_Server *server_ac;


static void setup(void) {
    server_ac = UA_Server_newForUnitTest();
}

static void teardown(void) {
    UA_Server_delete(server_ac);
}

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS

START_TEST(createDelete) {
    UA_StatusCode retval;
    // Loop to increase the chance of capturing dead pointers
    for(UA_UInt16 i = 0; i < 3; ++i)
    {
        UA_NodeId conditionInstance = UA_NODEID_NULL;

        retval = UA_Server_createCondition(
            server_ac,
            UA_NODEID_NULL,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
            UA_QUALIFIEDNAME(0, "Condition createDelete"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
            UA_NODEID_NULL,
            &conditionInstance);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert_msg(!UA_NodeId_isNull(&conditionInstance), "ConditionId is null");

        retval = UA_Server_deleteCondition(
            server_ac,
            conditionInstance,
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER)
        );
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }
} END_TEST

START_TEST(splitCreation) {

    UA_UInt16 nsIdx = UA_Server_addNamespace(server_ac, "http://yourorganisation.org/test/");

    UA_StatusCode retval;
    const UA_NodeId requestedId = UA_NODEID_NUMERIC(nsIdx, 1000);
    UA_NodeId actualId = UA_NODEID_NULL;

    retval = UA_Server_addCondition_begin(
        server_ac,
        requestedId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        UA_QUALIFIEDNAME(0, "Condition split creation"),
        &actualId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_msg(UA_NodeId_equal(&requestedId, &actualId),
                                  "Actual node Id differs from requested Id");

    // explicit add member EnabledState
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.minimumSamplingInterval = 0.000000;
    attr.userAccessLevel = 1;
    attr.accessLevel = 1;
    /* Value rank inherited */
    attr.valueRank = -1;
    attr.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_LOCALIZEDTEXT);
    attr.displayName = UA_LOCALIZEDTEXT("", "EnabledState");

    UA_NodeId var;
    retval = UA_Server_addVariableNode(
        server_ac,
        UA_NODEID_NULL,
        actualId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_TWOSTATEVARIABLETYPE),
        attr,
        NULL,
        &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_addCondition_finish(
        server_ac,
        actualId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
        UA_NODEID_NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // Check if EnabledState is linked - correctly set to "Disabled"
    UA_Variant enabledStateVariant;
    UA_Variant_init(&enabledStateVariant);
    retval = UA_Server_readValue(server_ac, var, &enabledStateVariant);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_msg(UA_Variant_hasScalarType(&enabledStateVariant, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]),
        "Unexpected Data Type of EnabledState");

    const UA_LocalizedText* enabledState = (UA_LocalizedText*)enabledStateVariant.data;
    UA_String disabledStr = UA_STRING("Disabled");
    ck_assert(UA_String_equal(&enabledState->text, &disabledStr));

    UA_Variant_clear(&enabledStateVariant);

    retval = UA_Server_deleteCondition(
        server_ac,
        actualId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

#endif

int main(void) {
    Suite *s = suite_create("server_alarmcondition");

    TCase *tc_call = tcase_create("Alarms and Conditions");
#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    tcase_add_test(tc_call, createDelete);
    tcase_add_test(tc_call, splitCreation);
#endif
    tcase_add_checked_fixture(tc_call, setup, teardown);

    suite_add_tcase(s, tc_call);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
