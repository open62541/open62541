/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
*    Copyright 2020 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
*/

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include "open62541_queue.h"

UA_Server *acserver;
static uint32_t eventCount = 0;


static void setup(void) {
   eventCount = 0;
   acserver = UA_Server_new();
   UA_ServerConfig_setDefault(UA_Server_getConfig(acserver));
}

static void teardown(void) {
   UA_Server_delete(acserver);
}

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS

static UA_Boolean
isConditionTwoStateVariableInTrueState (UA_Server *server, UA_NodeId condition, UA_QualifiedName twoStateVariableName)
{
   UA_Boolean state = false;
   UA_NodeId stateNodeId;
   UA_StatusCode status = UA_Server_getNodeIdWithBrowseName(server, &condition, twoStateVariableName, &stateNodeId);
   assert (status == UA_STATUSCODE_GOOD);

   UA_NodeId stateIdNodeId;
   status = UA_Server_getNodeIdWithBrowseName(server, &stateNodeId, UA_QUALIFIEDNAME(0, "Id"), &stateIdNodeId);
   UA_NodeId_clear(&stateNodeId);
   assert (status == UA_STATUSCODE_GOOD);

   UA_Variant val;
   status = UA_Server_readValue(server, stateIdNodeId, &val);
   UA_NodeId_clear(&stateIdNodeId);
   assert (status == UA_STATUSCODE_GOOD);
   assert (val.data != NULL && val.type == &UA_TYPES[UA_TYPES_BOOLEAN]);
   state = *(UA_Boolean*)val.data;
   UA_Variant_clear(&val);
   return state;
}

START_TEST(createMultiple) {
   UA_StatusCode retval;

   UA_CreateConditionProperties conditionProperties = {
       .sourceNode = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
       .browseName = UA_QUALIFIEDNAME(0, "Condition create multiple")
   };

   for(UA_UInt16 i = 0; i < 10; ++i)
   {
       UA_NodeId conditionInstance = UA_NODEID_NULL;
       retval = __UA_Server_createCondition(
           acserver,
           UA_NODEID_NULL,
           UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE),
           &conditionProperties,
           NULL,
           NULL,
           &conditionInstance
       );
       ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
       ck_assert_msg(!UA_NodeId_isNull(&conditionInstance), "ConditionId is null");
   }
} END_TEST

START_TEST(createDelete) {
   UA_StatusCode retval;

   UA_CreateConditionProperties conditionProperties = {
       .sourceNode = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
       .browseName = UA_QUALIFIEDNAME(0, "Condition createDelete")
   };

   // Loop to increase the chance of capturing dead pointers
   for(UA_UInt16 i = 0; i < 3; ++i)
   {
       UA_NodeId conditionInstance = UA_NODEID_NULL;
       retval = __UA_Server_createCondition(
           acserver,
           UA_NODEID_NULL,
           UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE),
           &conditionProperties,
           NULL,
           NULL,
           &conditionInstance
       );
       ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
       ck_assert_msg(!UA_NodeId_isNull(&conditionInstance), "ConditionId is null");

       retval = UA_Server_deleteCondition(
           acserver,
           conditionInstance
       );
       ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
   }
} END_TEST

#endif

int main(void) {
   Suite *s = suite_create("server_alarmcondition");

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
   TCase *tc_call = tcase_create("Alarms and Conditions");
   tcase_add_test(tc_call, createDelete);
   tcase_add_test(tc_call, createMultiple);
   tcase_add_checked_fixture(tc_call, setup, teardown);
   suite_add_tcase(s, tc_call);

#endif

   SRunner *sr = srunner_create(s);
   srunner_set_fork_status(sr, CK_NOFORK);
   srunner_run_all(sr, CK_NORMAL);
   int number_failed = srunner_ntests_failed(sr);
   srunner_free(sr);
   return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
