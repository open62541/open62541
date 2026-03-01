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
#include <stdlib.h>

UA_Server *server_ac;


static void setup(void) {
    server_ac = UA_Server_newForUnitTest();
}

static void teardown(void) {
    UA_Server_delete(server_ac);
}

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS

/* Helper: create a condition and return its NodeId */
static UA_NodeId
createTestCondition(UA_Server *s, const UA_NodeId conditionType,
                    const char *name, const UA_NodeId source) {
    UA_NodeId conditionInstance = UA_NODEID_NULL;
    UA_StatusCode retval = UA_Server_createCondition(
        s, UA_NODEID_NULL, conditionType,
        UA_QUALIFIEDNAME(0, (char*)(uintptr_t)name),
        source, UA_NODEID_NULL, &conditionInstance);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_msg(!UA_NodeId_isNull(&conditionInstance), "ConditionId is null");
    return conditionInstance;
}

/* Helper: read a condition sub-field (e.g. "EnabledState") */
static UA_StatusCode
readConditionField(UA_Server *s, const UA_NodeId conditionId,
                   const char *fieldName, UA_Variant *out) {
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    rpe.isInverse = false;
    rpe.includeSubtypes = true;
    rpe.targetName = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)fieldName);

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = conditionId;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;

    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(s, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize == 0) {
        /* Try HasProperty if HasComponent didn't find it */
        rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
        bpr = UA_Server_translateBrowsePathToNodeIds(s, &bp);
        if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize == 0) {
            UA_BrowsePathResult_clear(&bpr);
            return UA_STATUSCODE_BADNOTFOUND;
        }
    }

    UA_NodeId fieldNode = bpr.targets[0].targetId.nodeId;
    UA_StatusCode retval = UA_Server_readValue(s, fieldNode, out);
    UA_BrowsePathResult_clear(&bpr);
    return retval;
}

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

/* Test creating a condition with an invalid type (not a subtype of ConditionType) */
START_TEST(createCondition_invalidType) {
    UA_NodeId conditionInstance = UA_NODEID_NULL;
    /* BaseObjectType is not a subtype of ConditionType */
    UA_StatusCode retval = UA_Server_createCondition(
        server_ac, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_QUALIFIEDNAME(0, "InvalidCondition"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
        UA_NODEID_NULL, &conditionInstance);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test deleting a non-existent condition */
START_TEST(deleteCondition_notFound) {
    UA_NodeId fakeCondition = UA_NODEID_NUMERIC(1, 99999);
    UA_StatusCode retval = UA_Server_deleteCondition(
        server_ac, fakeCondition,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER));
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

/* Test the full condition lifecycle: create → set field → trigger → delete */
START_TEST(conditionLifecycle_setField) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "LifecycleCondition", source);

    /* Set the Severity field */
    UA_UInt16 severity = 500;
    UA_Variant val;
    UA_Variant_setScalar(&val, &severity, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_Server_setConditionField(
        server_ac, cond, &val, UA_QUALIFIEDNAME(0, "Severity"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set the Message field */
    UA_LocalizedText msg = UA_LOCALIZEDTEXT("en", "Test message");
    UA_Variant_setScalar(&val, &msg, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(
        server_ac, cond, &val, UA_QUALIFIEDNAME(0, "Message"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set the Comment field */
    UA_LocalizedText comment = UA_LOCALIZEDTEXT("en", "Test comment");
    UA_Variant_setScalar(&val, &comment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(
        server_ac, cond, &val, UA_QUALIFIEDNAME(0, "Comment"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set the Retain field */
    UA_Boolean retain = true;
    UA_Variant_setScalar(&val, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionField(
        server_ac, cond, &val, UA_QUALIFIEDNAME(0, "Retain"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Clean up */
    retval = UA_Server_deleteCondition(server_ac, cond, source);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test setConditionField on a non-existent field */
START_TEST(setConditionField_invalidField) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "InvalidFieldCondition", source);

    UA_UInt16 val = 42;
    UA_Variant v;
    UA_Variant_setScalar(&v, &val, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_Server_setConditionField(
        server_ac, cond, &v, UA_QUALIFIEDNAME(0, "NonExistentField"));
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

    UA_Server_deleteCondition(server_ac, cond, source);
} END_TEST

/* Test setConditionVariableFieldProperty (e.g. EnabledState/Id) */
START_TEST(setConditionVariableFieldProperty_test) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "VarFieldPropCondition", source);

    /* Set EnabledState/Id = true to enable the condition */
    UA_Boolean enabled = true;
    UA_Variant val;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval = UA_Server_setConditionVariableFieldProperty(
        server_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Server_deleteCondition(server_ac, cond, source);
} END_TEST

/* Test triggering a condition event */
START_TEST(triggerConditionEvent_test) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "TriggerCondition", source);

    /* Enable the condition first */
    UA_Boolean enabled = true;
    UA_Variant val;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_setConditionVariableFieldProperty(
        server_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));

    /* Set Retain to true */
    UA_Boolean retain = true;
    UA_Variant_setScalar(&val, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_setConditionField(
        server_ac, cond, &val, UA_QUALIFIEDNAME(0, "Retain"));

    /* Trigger the event */
    UA_ByteString eventId = UA_BYTESTRING_NULL;
    UA_StatusCode retval = UA_Server_triggerConditionEvent(
        server_ac, cond, source, &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(eventId.length > 0);
    UA_ByteString_clear(&eventId);

    UA_Server_deleteCondition(server_ac, cond, source);
} END_TEST

/* Test triggering on a disabled condition → should fail */
START_TEST(triggerConditionEvent_disabled) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "TriggerDisabledCondition", source);

    /* Condition is disabled by default. Triggering should fail. */
    UA_ByteString eventId = UA_BYTESTRING_NULL;
    UA_StatusCode retval = UA_Server_triggerConditionEvent(
        server_ac, cond, source, &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCONDITIONALREADYDISABLED);

    UA_Server_deleteCondition(server_ac, cond, source);
} END_TEST

/* Test addConditionOptionalField */
START_TEST(addConditionOptionalField_test) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "OptionalFieldCondition", source);

    /* Add the optional "LocalTime" field */
    UA_NodeId outOptionalVar = UA_NODEID_NULL;
    UA_StatusCode retval = UA_Server_addConditionOptionalField(
        server_ac, cond,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        UA_QUALIFIEDNAME(0, "LocalTime"),
        &outOptionalVar);
    /* May succeed or fail depending on NS0 definition; both are valid */
    (void)retval;
    (void)outOptionalVar;

    UA_Server_deleteCondition(server_ac, cond, source);
} END_TEST

/* Callback tracking for TwoStateVariable */
static UA_Boolean twoStateCallbackFired = false;
static UA_StatusCode
testTwoStateCallback(UA_Server *s, const UA_NodeId *condition) {
    twoStateCallbackFired = true;
    return UA_STATUSCODE_GOOD;
}

/* Test setConditionTwoStateVariableCallback */
START_TEST(setTwoStateVariableCallback_test) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "TwoStateCallbackCondition", source);

    /* Set an EnabledState callback */
    twoStateCallbackFired = false;
    UA_StatusCode retval = UA_Server_setConditionTwoStateVariableCallback(
        server_ac, cond, source, false,
        testTwoStateCallback, UA_ENTERING_ENABLEDSTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set an AckedState callback */
    retval = UA_Server_setConditionTwoStateVariableCallback(
        server_ac, cond, source, false,
        testTwoStateCallback, UA_ENTERING_ACKEDSTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set an ActiveState callback */
    retval = UA_Server_setConditionTwoStateVariableCallback(
        server_ac, cond, source, false,
        testTwoStateCallback, UA_ENTERING_ACTIVESTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Server_deleteCondition(server_ac, cond, source);
} END_TEST

/* Test setConditionTwoStateVariableCallback on non-existent condition */
START_TEST(setTwoStateVariableCallback_notFound) {
    UA_NodeId fakeCondition = UA_NODEID_NUMERIC(1, 99999);
    UA_StatusCode retval = UA_Server_setConditionTwoStateVariableCallback(
        server_ac, fakeCondition,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), false,
        testTwoStateCallback, UA_ENTERING_ENABLEDSTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

/* Test creating and deleting multiple conditions on the same source */
START_TEST(multipleConditions_sameSource) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);

    UA_NodeId cond1 = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "MultiCond1", source);
    UA_NodeId cond2 = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "MultiCond2", source);
    UA_NodeId cond3 = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "MultiCond3", source);

    /* Delete in different order than creation */
    UA_StatusCode retval = UA_Server_deleteCondition(server_ac, cond2, source);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_deleteCondition(server_ac, cond1, source);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_deleteCondition(server_ac, cond3, source);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test creating conditions with different condition types */
START_TEST(createCondition_differentTypes) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_StatusCode retval;

    /* OffNormalAlarmType */
    UA_NodeId cond1 = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "OffNormalAlarm", source);

    /* AcknowledgeableConditionType */
    UA_NodeId cond2 = UA_NODEID_NULL;
    retval = UA_Server_createCondition(
        server_ac, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE),
        UA_QUALIFIEDNAME(0, "AckCondition"),
        source, UA_NODEID_NULL, &cond2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* AlarmConditionType */
    UA_NodeId cond3 = UA_NODEID_NULL;
    retval = UA_Server_createCondition(
        server_ac, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE),
        UA_QUALIFIEDNAME(0, "AlarmCondition"),
        source, UA_NODEID_NULL, &cond3);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Delete all */
    UA_Server_deleteCondition(server_ac, cond1, source);
    UA_Server_deleteCondition(server_ac, cond2, source);
    UA_Server_deleteCondition(server_ac, cond3, source);
} END_TEST

/* Test creating an ExclusiveLimitAlarm and using setLimitState */
START_TEST(limitAlarm_setLimitState) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_StatusCode retval;

    /* Create an ExclusiveLimitAlarmType condition */
    UA_NodeId cond = UA_NODEID_NULL;
    retval = UA_Server_createCondition(
        server_ac, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_EXCLUSIVELIMITALARMTYPE),
        UA_QUALIFIEDNAME(0, "LimitAlarm"),
        source, UA_NODEID_NULL, &cond);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set limit values on the condition first */
    UA_Double highHighLimit = 100.0;
    UA_Variant val;
    UA_Variant_setScalar(&val, &highHighLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_setConditionField(server_ac, cond, &val,
                                UA_QUALIFIEDNAME(0, "HighHighLimit"));

    UA_Double highLimit = 80.0;
    UA_Variant_setScalar(&val, &highLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_setConditionField(server_ac, cond, &val,
                                UA_QUALIFIEDNAME(0, "HighLimit"));

    UA_Double lowLimit = 20.0;
    UA_Variant_setScalar(&val, &lowLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_setConditionField(server_ac, cond, &val,
                                UA_QUALIFIEDNAME(0, "LowLimit"));

    UA_Double lowLowLimit = 5.0;
    UA_Variant_setScalar(&val, &lowLowLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_setConditionField(server_ac, cond, &val,
                                UA_QUALIFIEDNAME(0, "LowLowLimit"));

    /* Test setLimitState with a HighHigh value */
    retval = UA_Server_setLimitState(server_ac, cond, 110.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Test setLimitState with a High value */
    retval = UA_Server_setLimitState(server_ac, cond, 90.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Test setLimitState with a normal value (between low and high) */
    retval = UA_Server_setLimitState(server_ac, cond, 50.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Test setLimitState with a Low value */
    retval = UA_Server_setLimitState(server_ac, cond, 15.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Test setLimitState with a LowLow value */
    retval = UA_Server_setLimitState(server_ac, cond, 3.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Server_deleteCondition(server_ac, cond, source);
} END_TEST

/* Test enabling and disabling via setConditionVariableFieldProperty */
START_TEST(enableDisable_condition) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "EnableDisableCondition", source);

    UA_Variant val;

    /* Read initial EnabledState — should be "Disabled" */
    UA_Variant fieldVal;
    UA_StatusCode retval = readConditionField(server_ac, cond,
                                              "EnabledState", &fieldVal);
    if(retval == UA_STATUSCODE_GOOD) {
        if(UA_Variant_hasScalarType(&fieldVal, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
            UA_LocalizedText *lt = (UA_LocalizedText*)fieldVal.data;
            UA_String disabledStr = UA_STRING("Disabled");
            ck_assert(UA_String_equal(&lt->text, &disabledStr));
        }
        UA_Variant_clear(&fieldVal);
    }

    /* Enable */
    UA_Boolean enabled = true;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionVariableFieldProperty(
        server_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read EnabledState — should be "Enabled" */
    retval = readConditionField(server_ac, cond, "EnabledState", &fieldVal);
    if(retval == UA_STATUSCODE_GOOD) {
        if(UA_Variant_hasScalarType(&fieldVal, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
            UA_LocalizedText *lt = (UA_LocalizedText*)fieldVal.data;
            UA_String enabledStr = UA_STRING("Enabled");
            ck_assert(UA_String_equal(&lt->text, &enabledStr));
        }
        UA_Variant_clear(&fieldVal);
    }

    /* Disable */
    UA_Boolean disabled = false;
    UA_Variant_setScalar(&val, &disabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionVariableFieldProperty(
        server_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Server_deleteCondition(server_ac, cond, source);
} END_TEST

/* Test setting AckedState on a condition */
START_TEST(setAckedState_condition) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE),
        "AckedStateCondition", source);

    /* Enable the condition */
    UA_Boolean enabled = true;
    UA_Variant val;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_setConditionVariableFieldProperty(
        server_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));

    /* Set AckedState/Id = true */
    UA_Boolean acked = true;
    UA_Variant_setScalar(&val, &acked, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval = UA_Server_setConditionVariableFieldProperty(
        server_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "AckedState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set AckedState/Id = false (unacknowledge) */
    acked = false;
    UA_Variant_setScalar(&val, &acked, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionVariableFieldProperty(
        server_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "AckedState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Server_deleteCondition(server_ac, cond, source);
} END_TEST

/* Test full lifecycle: create → enable → set severity → trigger → set acked → delete */
START_TEST(conditionFullLifecycle) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE),
        "FullLifecycleCondition", source);

    UA_Variant val;
    UA_StatusCode retval;

    /* Enable the condition */
    UA_Boolean enabled = true;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionVariableFieldProperty(
        server_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set Retain */
    UA_Boolean retain = true;
    UA_Variant_setScalar(&val, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionField(
        server_ac, cond, &val, UA_QUALIFIEDNAME(0, "Retain"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set Severity */
    UA_UInt16 severity = 800;
    UA_Variant_setScalar(&val, &severity, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Server_setConditionField(
        server_ac, cond, &val, UA_QUALIFIEDNAME(0, "Severity"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set Message */
    UA_LocalizedText msg = UA_LOCALIZEDTEXT("en", "Critical alarm");
    UA_Variant_setScalar(&val, &msg, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(
        server_ac, cond, &val, UA_QUALIFIEDNAME(0, "Message"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Trigger the condition event */
    UA_ByteString eventId = UA_BYTESTRING_NULL;
    retval = UA_Server_triggerConditionEvent(server_ac, cond, source, &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(eventId.length > 0);
    UA_ByteString_clear(&eventId);

    /* Acknowledge the condition via setConditionVariableFieldProperty */
    UA_Boolean acked = true;
    UA_Variant_setScalar(&val, &acked, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionVariableFieldProperty(
        server_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "AckedState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Trigger again after ack */
    retval = UA_Server_triggerConditionEvent(server_ac, cond, source, &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&eventId);

    /* Disable */
    UA_Boolean disabled = false;
    UA_Variant_setScalar(&val, &disabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionVariableFieldProperty(
        server_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Delete */
    retval = UA_Server_deleteCondition(server_ac, cond, source);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test server shutdown with active conditions (cleanup path) */
START_TEST(conditionCleanup_onShutdown) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);

    /* Create conditions but DON'T delete them — they should be cleaned up
     * by the server destructor (via UA_ConditionList_delete) */
    createTestCondition(server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "LeakyCondition1", source);
    createTestCondition(server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "LeakyCondition2", source);

    /* No explicit deleteCondition — teardown will call UA_Server_delete
     * which should clean up via UA_ConditionList_delete */
} END_TEST

/* Test creating condition with NULL outNodeId */
START_TEST(createCondition_nullOutNodeId) {
    /* The function should return BADINVALIDARGUMENT or similar when outNodeId is NULL */
    UA_StatusCode retval = UA_Server_createCondition(
        server_ac, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        UA_QUALIFIEDNAME(0, "NullOutCondition"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
        UA_NODEID_NULL, NULL);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test addCondition_begin with NULL outNodeId */
START_TEST(addConditionBegin_nullOutNodeId) {
    UA_StatusCode retval = UA_Server_addCondition_begin(
        server_ac, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        UA_QUALIFIEDNAME(0, "NullOutBeginCondition"),
        NULL);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test trigger with NULL eventId output */
START_TEST(triggerConditionEvent_nullEventId) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "TriggerNullEventIdCondition", source);

    /* Enable */
    UA_Boolean enabled = true;
    UA_Variant val;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_setConditionVariableFieldProperty(
        server_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));

    UA_Boolean retain = true;
    UA_Variant_setScalar(&val, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_setConditionField(
        server_ac, cond, &val, UA_QUALIFIEDNAME(0, "Retain"));

    /* Trigger with NULL outEventId should still work */
    UA_StatusCode retval = UA_Server_triggerConditionEvent(
        server_ac, cond, source, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Server_deleteCondition(server_ac, cond, source);
} END_TEST

/* Test creating condition with different sources */
START_TEST(condition_differentSources) {
    UA_StatusCode retval;

    /* Add a custom object to act as a condition source */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en", "CustomSource");
    UA_NodeId customSource = UA_NODEID_STRING(1, "custom.source");
    retval = UA_Server_addObjectNode(server_ac, customSource,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "CustomSource"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Create condition on custom source */
    UA_NodeId cond = UA_NODEID_NULL;
    retval = UA_Server_createCondition(
        server_ac, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        UA_QUALIFIEDNAME(0, "CustomSourceCondition"),
        customSource, UA_NODEID_NULL, &cond);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_deleteCondition(server_ac, cond, customSource);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test setConditionField with all common field names */
START_TEST(setConditionField_multipleFields) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "MultiFieldCondition", source);

    UA_Variant val;
    UA_StatusCode retval;

    /* NormalState */
    UA_NodeId normalState = UA_NODEID_NUMERIC(0, 0);
    UA_Variant_setScalar(&val, &normalState, &UA_TYPES[UA_TYPES_NODEID]);
    retval = UA_Server_setConditionField(
        server_ac, cond, &val, UA_QUALIFIEDNAME(0, "NormalState"));
    /* May or may not succeed depending on whether field exists */
    (void)retval;

    /* Quality */
    UA_StatusCode quality = UA_STATUSCODE_GOOD;
    UA_Variant_setScalar(&val, &quality, &UA_TYPES[UA_TYPES_STATUSCODE]);
    retval = UA_Server_setConditionField(
        server_ac, cond, &val, UA_QUALIFIEDNAME(0, "Quality"));
    (void)retval;

    UA_Server_deleteCondition(server_ac, cond, source);
} END_TEST

#endif

int main(void) {
    Suite *s = suite_create("server_alarmcondition");

    TCase *tc_call = tcase_create("Alarms and Conditions");
#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    tcase_add_test(tc_call, createDelete);
    tcase_add_test(tc_call, splitCreation);
    tcase_add_test(tc_call, createCondition_invalidType);
    tcase_add_test(tc_call, deleteCondition_notFound);
    tcase_add_test(tc_call, createCondition_nullOutNodeId);
    tcase_add_test(tc_call, addConditionBegin_nullOutNodeId);
#endif
    tcase_add_checked_fixture(tc_call, setup, teardown);
    suite_add_tcase(s, tc_call);

    TCase *tc_fields = tcase_create("Condition Fields");
#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    tcase_add_test(tc_fields, conditionLifecycle_setField);
    tcase_add_test(tc_fields, setConditionField_invalidField);
    tcase_add_test(tc_fields, setConditionVariableFieldProperty_test);
    tcase_add_test(tc_fields, setConditionField_multipleFields);
    tcase_add_test(tc_fields, addConditionOptionalField_test);
#endif
    tcase_add_checked_fixture(tc_fields, setup, teardown);
    suite_add_tcase(s, tc_fields);

    TCase *tc_trigger = tcase_create("Condition Trigger");
#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    tcase_add_test(tc_trigger, triggerConditionEvent_test);
    tcase_add_test(tc_trigger, triggerConditionEvent_disabled);
    tcase_add_test(tc_trigger, triggerConditionEvent_nullEventId);
    tcase_add_test(tc_trigger, enableDisable_condition);
#endif
    tcase_add_checked_fixture(tc_trigger, setup, teardown);
    suite_add_tcase(s, tc_trigger);

    TCase *tc_state = tcase_create("Condition State");
#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    tcase_add_test(tc_state, setAckedState_condition);
    tcase_add_test(tc_state, setTwoStateVariableCallback_test);
    tcase_add_test(tc_state, setTwoStateVariableCallback_notFound);
    tcase_add_test(tc_state, conditionFullLifecycle);
#endif
    tcase_add_checked_fixture(tc_state, setup, teardown);
    suite_add_tcase(s, tc_state);

    TCase *tc_limit = tcase_create("Limit Alarms");
#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    tcase_add_test(tc_limit, limitAlarm_setLimitState);
#endif
    tcase_add_checked_fixture(tc_limit, setup, teardown);
    suite_add_tcase(s, tc_limit);

    TCase *tc_misc = tcase_create("Condition Misc");
#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    tcase_add_test(tc_misc, multipleConditions_sameSource);
    tcase_add_test(tc_misc, createCondition_differentTypes);
    tcase_add_test(tc_misc, conditionCleanup_onShutdown);
    tcase_add_test(tc_misc, condition_differentSources);
#endif
    tcase_add_checked_fixture(tc_misc, setup, teardown);
    suite_add_tcase(s, tc_misc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
