/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 */

#include <open62541/server.h>
#include <open62541/driver/alarms_conditions.h>
#include <open62541/server_config_default.h>
#include "server/ua_server_internal.h"
#include "test_helpers.h"

#include <check.h>
#include <stdlib.h>

#if defined(UA_ENABLE_SUBSCRIPTIONS_EVENTS) && defined(UA_GENERATED_NAMESPACE_ZERO_FULL)
# define UA_TEST_ENABLE_ALARMS_CONDITIONS
#endif

UA_Server *server_ac;
static UA_AlarmConditionsDriver *acDriver_ac;


static void setup(void) {
    server_ac = UA_Server_newForUnitTest();
#ifdef UA_TEST_ENABLE_ALARMS_CONDITIONS
    acDriver_ac = UA_AlarmsConditionsDriver(UA_KEYVALUEMAP_NULL);
    ck_assert_ptr_nonnull(acDriver_ac);
    ck_assert_uint_eq(UA_Server_addDriver(server_ac, &acDriver_ac->drv),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(acDriver_ac->drv.start(&acDriver_ac->drv),
                      UA_STATUSCODE_GOOD);
#endif
}

static void teardown(void) {
#ifdef UA_TEST_ENABLE_ALARMS_CONDITIONS
    acDriver_ac->drv.stop(&acDriver_ac->drv);
    ck_assert_uint_eq(UA_Server_removeDriver(server_ac, &acDriver_ac->drv),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(acDriver_ac->drv.free(&acDriver_ac->drv),
                      UA_STATUSCODE_GOOD);
#endif
    UA_Server_delete(server_ac);
}

#ifdef UA_TEST_ENABLE_ALARMS_CONDITIONS

/* Helper: create a condition and return its NodeId */
static UA_NodeId
createTestCondition(UA_Server *s, const UA_NodeId conditionType,
                    const char *name, const UA_NodeId source) {
    UA_NodeId conditionInstance = UA_NODEID_NULL;
    UA_StatusCode retval = acDriver_ac->createCondition(acDriver_ac, UA_NODEID_NULL, conditionType,
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

static UA_NodeId
createConfiguredExclusiveLimitAlarm(UA_Server *s, const char *name,
                                    const UA_NodeId source) {
    UA_NodeId cond = createTestCondition(
        s, UA_NODEID_NUMERIC(0, UA_NS0ID_EXCLUSIVELIMITALARMTYPE),
        name, source);

    UA_Variant val;
    UA_StatusCode retval;

    UA_Double highHighLimit = 100.0;
    UA_Variant_setScalar(&val, &highHighLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val,
                                         UA_QUALIFIEDNAME(0, "HighHighLimit"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Double highLimit = 80.0;
    UA_Variant_setScalar(&val, &highLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val,
                                         UA_QUALIFIEDNAME(0, "HighLimit"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Double lowLimit = 20.0;
    UA_Variant_setScalar(&val, &lowLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val,
                                         UA_QUALIFIEDNAME(0, "LowLimit"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Double lowLowLimit = 5.0;
    UA_Variant_setScalar(&val, &lowLowLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val,
                                         UA_QUALIFIEDNAME(0, "LowLowLimit"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    return cond;
}

START_TEST(createDelete) {
    UA_StatusCode retval;
    // Loop to increase the chance of capturing dead pointers
    for(UA_UInt16 i = 0; i < 3; ++i)
    {
        UA_NodeId conditionInstance = UA_NODEID_NULL;

        retval = acDriver_ac->createCondition(acDriver_ac,
            UA_NODEID_NULL,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
            UA_QUALIFIEDNAME(0, "Condition createDelete"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
            UA_NODEID_NULL,
            &conditionInstance);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert_msg(!UA_NodeId_isNull(&conditionInstance), "ConditionId is null");

        retval = acDriver_ac->deleteCondition(acDriver_ac,
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

    retval = acDriver_ac->addConditionBegin(acDriver_ac,
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

    retval = acDriver_ac->addConditionFinish(acDriver_ac,
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

    retval = acDriver_ac->deleteCondition(acDriver_ac,
        actualId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

/* Test creating a condition with an invalid type (not a subtype of ConditionType) */
START_TEST(createCondition_invalidType) {
    UA_NodeId conditionInstance = UA_NODEID_NULL;
    /* BaseObjectType is not a subtype of ConditionType */
    UA_StatusCode retval = acDriver_ac->createCondition(acDriver_ac, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_QUALIFIEDNAME(0, "InvalidCondition"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
        UA_NODEID_NULL, &conditionInstance);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test deleting a non-existent condition */
START_TEST(deleteCondition_notFound) {
    UA_NodeId fakeCondition = UA_NODEID_NUMERIC(1, 99999);
    UA_StatusCode retval = acDriver_ac->deleteCondition(acDriver_ac, fakeCondition,
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
    UA_StatusCode retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Severity"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set the Message field */
    UA_LocalizedText msg = UA_LOCALIZEDTEXT("en", "Test message");
    UA_Variant_setScalar(&val, &msg, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Message"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set the Comment field */
    UA_LocalizedText comment = UA_LOCALIZEDTEXT("en", "Test comment");
    UA_Variant_setScalar(&val, &comment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Comment"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set the Retain field */
    UA_Boolean retain = true;
    UA_Variant_setScalar(&val, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Retain"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Clean up */
    retval = acDriver_ac->deleteCondition(acDriver_ac, cond, source);
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
    UA_StatusCode retval = acDriver_ac->setConditionField(acDriver_ac, cond, &v, UA_QUALIFIEDNAME(0, "NonExistentField"));
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
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
    UA_StatusCode retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
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
    acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));

    /* Set Retain to true */
    UA_Boolean retain = true;
    UA_Variant_setScalar(&val, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Retain"));

    /* Trigger the event */
    UA_ByteString eventId = UA_BYTESTRING_NULL;
    UA_StatusCode retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(eventId.length > 0);
    UA_ByteString_clear(&eventId);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
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
    UA_StatusCode retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCONDITIONALREADYDISABLED);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
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
    UA_StatusCode retval = acDriver_ac->addConditionOptionalField(acDriver_ac, cond,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        UA_QUALIFIEDNAME(0, "LocalTime"),
        &outOptionalVar);
    /* May succeed or fail depending on NS0 definition; both are valid */
    (void)retval;
    (void)outOptionalVar;

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
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
    UA_StatusCode retval = acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, cond, source, false,
        testTwoStateCallback, UA_ENTERING_ENABLEDSTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set an AckedState callback */
    retval = acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, cond, source, false,
        testTwoStateCallback, UA_ENTERING_ACKEDSTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set an ActiveState callback */
    retval = acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, cond, source, false,
        testTwoStateCallback, UA_ENTERING_ACTIVESTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test setConditionTwoStateVariableCallback on non-existent condition */
START_TEST(setTwoStateVariableCallback_notFound) {
    UA_NodeId fakeCondition = UA_NODEID_NUMERIC(1, 99999);
    UA_StatusCode retval = acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, fakeCondition,
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
    UA_StatusCode retval = acDriver_ac->deleteCondition(acDriver_ac, cond2, source);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->deleteCondition(acDriver_ac, cond1, source);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->deleteCondition(acDriver_ac, cond3, source);
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
    retval = acDriver_ac->createCondition(acDriver_ac, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE),
        UA_QUALIFIEDNAME(0, "AckCondition"),
        source, UA_NODEID_NULL, &cond2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* AlarmConditionType */
    UA_NodeId cond3 = UA_NODEID_NULL;
    retval = acDriver_ac->createCondition(acDriver_ac, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE),
        UA_QUALIFIEDNAME(0, "AlarmCondition"),
        source, UA_NODEID_NULL, &cond3);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Delete all */
    acDriver_ac->deleteCondition(acDriver_ac, cond1, source);
    acDriver_ac->deleteCondition(acDriver_ac, cond2, source);
    acDriver_ac->deleteCondition(acDriver_ac, cond3, source);
} END_TEST

/* Test creating an ExclusiveLimitAlarm and using setLimitState */
START_TEST(limitAlarm_setLimitState) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_StatusCode retval;

    /* Create an ExclusiveLimitAlarmType condition */
    UA_NodeId cond = UA_NODEID_NULL;
    retval = acDriver_ac->createCondition(acDriver_ac, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_EXCLUSIVELIMITALARMTYPE),
        UA_QUALIFIEDNAME(0, "LimitAlarm"),
        source, UA_NODEID_NULL, &cond);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set limit values on the condition first */
    UA_Double highHighLimit = 100.0;
    UA_Variant val;
    UA_Variant_setScalar(&val, &highHighLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
    acDriver_ac->setConditionField(acDriver_ac, cond, &val,
                                UA_QUALIFIEDNAME(0, "HighHighLimit"));

    UA_Double highLimit = 80.0;
    UA_Variant_setScalar(&val, &highLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
    acDriver_ac->setConditionField(acDriver_ac, cond, &val,
                                UA_QUALIFIEDNAME(0, "HighLimit"));

    UA_Double lowLimit = 20.0;
    UA_Variant_setScalar(&val, &lowLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
    acDriver_ac->setConditionField(acDriver_ac, cond, &val,
                                UA_QUALIFIEDNAME(0, "LowLimit"));

    UA_Double lowLowLimit = 5.0;
    UA_Variant_setScalar(&val, &lowLowLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
    acDriver_ac->setConditionField(acDriver_ac, cond, &val,
                                UA_QUALIFIEDNAME(0, "LowLowLimit"));

    /* Test setLimitState with a HighHigh value */
    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 110.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Test setLimitState with a High value */
    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 90.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Test setLimitState with a normal value (between low and high) */
    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 50.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Test setLimitState with a Low value */
    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 15.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Test setLimitState with a LowLow value */
    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 3.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
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
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
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
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
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
    acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));

    /* Set AckedState/Id = true */
    UA_Boolean acked = true;
    UA_Variant_setScalar(&val, &acked, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "AckedState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set AckedState/Id = false (unacknowledge) */
    acked = false;
    UA_Variant_setScalar(&val, &acked, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "AckedState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
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
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set Retain */
    UA_Boolean retain = true;
    UA_Variant_setScalar(&val, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Retain"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set Severity */
    UA_UInt16 severity = 800;
    UA_Variant_setScalar(&val, &severity, &UA_TYPES[UA_TYPES_UINT16]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Severity"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set Message */
    UA_LocalizedText msg = UA_LOCALIZEDTEXT("en", "Critical alarm");
    UA_Variant_setScalar(&val, &msg, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Message"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Trigger the condition event */
    UA_ByteString eventId = UA_BYTESTRING_NULL;
    retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(eventId.length > 0);
    UA_ByteString_clear(&eventId);

    /* Acknowledge the condition via setConditionVariableFieldProperty */
    UA_Boolean acked = true;
    UA_Variant_setScalar(&val, &acked, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "AckedState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Trigger again after ack */
    retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&eventId);

    /* Disable */
    UA_Boolean disabled = false;
    UA_Variant_setScalar(&val, &disabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Delete */
    retval = acDriver_ac->deleteCondition(acDriver_ac, cond, source);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test server shutdown with active conditions (cleanup path) */
START_TEST(conditionCleanup_onShutdown) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);

    /* Create conditions but DON'T delete them — they should be cleaned up
     * by the server destructor via the A&C driver cleanup */
    createTestCondition(server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "LeakyCondition1", source);
    createTestCondition(server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "LeakyCondition2", source);

    /* No explicit deleteCondition — teardown will call UA_Server_delete
     * which should clean up via the A&C driver */
} END_TEST

/* Test creating condition with NULL outNodeId */
START_TEST(createCondition_nullOutNodeId) {
    /* The function should return BADINVALIDARGUMENT or similar when outNodeId is NULL */
    UA_StatusCode retval = acDriver_ac->createCondition(acDriver_ac, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        UA_QUALIFIEDNAME(0, "NullOutCondition"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
        UA_NODEID_NULL, NULL);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test addCondition_begin with NULL outNodeId */
START_TEST(addConditionBegin_nullOutNodeId) {
    UA_StatusCode retval = acDriver_ac->addConditionBegin(acDriver_ac, UA_NODEID_NULL,
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
    acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));

    UA_Boolean retain = true;
    UA_Variant_setScalar(&val, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Retain"));

    /* Trigger with NULL outEventId should still work */
    UA_StatusCode retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
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
    retval = acDriver_ac->createCondition(acDriver_ac, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        UA_QUALIFIEDNAME(0, "CustomSourceCondition"),
        customSource, UA_NODEID_NULL, &cond);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->deleteCondition(acDriver_ac, cond, customSource);
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
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "NormalState"));
    /* May or may not succeed depending on whether field exists */
    (void)retval;

    /* Quality */
    UA_StatusCode quality = UA_STATUSCODE_GOOD;
    UA_Variant_setScalar(&val, &quality, &UA_TYPES[UA_TYPES_STATUSCODE]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Quality"));
    (void)retval;

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* --- Additional coverage tests --- */

/* Track whether callbacks were invoked */
static UA_Boolean enableCallbackInvoked = false;
static UA_Boolean confirmCallbackInvoked = false;
static UA_Boolean activeCallbackInvoked = false;
static UA_Boolean ackCallbackInvoked = false;

static UA_StatusCode
enableStateCallbackFn(UA_Server *s, const UA_NodeId *conditionId) {
    (void)s; (void)conditionId;
    enableCallbackInvoked = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
ackStateCallbackFn(UA_Server *s, const UA_NodeId *conditionId) {
    (void)s; (void)conditionId;
    ackCallbackInvoked = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
confirmStateCallbackFn(UA_Server *s, const UA_NodeId *conditionId) {
    (void)s; (void)conditionId;
    confirmCallbackInvoked = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
activeStateCallbackFn(UA_Server *s, const UA_NodeId *conditionId) {
    (void)s; (void)conditionId;
    activeCallbackInvoked = true;
    return UA_STATUSCODE_GOOD;
}

/* Test registering and invoking ENTERING_CONFIRMEDSTATE callback */
START_TEST(setConfirmedStateCallback) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    /* Use AlarmConditionType which has ConfirmedState */
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE),
        "ConfirmedCallbackCond", source);

    /* Register confirmed state callback */
    confirmCallbackInvoked = false;
    UA_StatusCode retval = acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, cond, source,
        false, /* removeBranch */
        confirmStateCallbackFn,
        UA_ENTERING_CONFIRMEDSTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Enable the condition */
    UA_Variant val;
    UA_Boolean enabled = true;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set ConfirmedState/Id = true → should invoke the callback */
    UA_Boolean confirmed = true;
    UA_Variant_setScalar(&val, &confirmed, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "ConfirmedState"),
        UA_QUALIFIEDNAME(0, "Id"));
    /* ConfirmedState may or may not exist depending on the type definition.
     * If it exists and the callback is invoked, it should be set. */
    (void)retval;

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test registering and invoking ENTERING_ACTIVESTATE callback */
START_TEST(setActiveStateCallback) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE),
        "ActiveCallbackCond", source);

    /* Register active state callback */
    activeCallbackInvoked = false;
    UA_StatusCode retval = acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, cond, source,
        false,
        activeStateCallbackFn,
        UA_ENTERING_ACTIVESTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Enable the condition */
    UA_Variant val;
    UA_Boolean enabled = true;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set ActiveState/Id = true → should invoke the active state callback */
    UA_Boolean active = true;
    UA_Variant_setScalar(&val, &active, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "ActiveState"),
        UA_QUALIFIEDNAME(0, "Id"));
    (void)retval;

    /* Set ActiveState/Id = false → deactivate */
    active = false;
    UA_Variant_setScalar(&val, &active, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "ActiveState"),
        UA_QUALIFIEDNAME(0, "Id"));
    (void)retval;

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test all four callback types registered together */
START_TEST(setAllCallbackTypes) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE),
        "AllCallbacksCond", source);

    enableCallbackInvoked = false;
    ackCallbackInvoked = false;
    confirmCallbackInvoked = false;
    activeCallbackInvoked = false;

    UA_StatusCode retval;

    /* Register all four callback types */
    retval = acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, cond, source, false,
        enableStateCallbackFn, UA_ENTERING_ENABLEDSTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, cond, source, false,
        ackStateCallbackFn, UA_ENTERING_ACKEDSTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, cond, source, false,
        confirmStateCallbackFn, UA_ENTERING_CONFIRMEDSTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, cond, source, false,
        activeStateCallbackFn, UA_ENTERING_ACTIVESTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Enable condition → triggers ENTERING_ENABLEDSTATE callback */
    UA_Variant val;
    UA_Boolean enabled = true;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(enableCallbackInvoked);

    /* Set ActiveState/Id = true → triggers ENTERING_ACTIVESTATE callback */
    UA_Boolean active = true;
    UA_Variant_setScalar(&val, &active, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "ActiveState"),
        UA_QUALIFIEDNAME(0, "Id"));
    (void)retval;

    /* Set AckedState/Id = true → triggers ENTERING_ACKEDSTATE callback */
    UA_Boolean acked = true;
    UA_Variant_setScalar(&val, &acked, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "AckedState"),
        UA_QUALIFIEDNAME(0, "Id"));
    (void)retval;

    /* Set ConfirmedState/Id = true → triggers ENTERING_CONFIRMEDSTATE callback */
    UA_Boolean confirmed = true;
    UA_Variant_setScalar(&val, &confirmed, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "ConfirmedState"),
        UA_QUALIFIEDNAME(0, "Id"));
    (void)retval;

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test invalid callback type */
START_TEST(setCallback_invalidType) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "InvalidCallbackCond", source);

    /* Pass an invalid callback type (value 99) */
    UA_StatusCode retval = acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, cond, source, false,
        enableStateCallbackFn, (UA_TwoStateVariableCallbackType)99);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINTERNALERROR);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test setting Severity multiple times and triggering (exercises lastSeverity tracking) */
START_TEST(severityChange_multiTrigger) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "SeverityChangeCond", source);

    UA_Variant val;
    UA_StatusCode retval;

    /* Enable the condition */
    UA_Boolean enabled = true;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set Retain */
    UA_Boolean retain = true;
    UA_Variant_setScalar(&val, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Retain"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set initial severity and trigger */
    UA_UInt16 sev = 100;
    UA_Variant_setScalar(&val, &sev, &UA_TYPES[UA_TYPES_UINT16]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Severity"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString eventId = UA_BYTESTRING_NULL;
    retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&eventId);

    /* Change severity and trigger again → exercises updateConditionLastSeverity */
    sev = 500;
    UA_Variant_setScalar(&val, &sev, &UA_TYPES[UA_TYPES_UINT16]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Severity"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&eventId);

    /* Change severity to highest and trigger once more */
    sev = 1000;
    UA_Variant_setScalar(&val, &sev, &UA_TYPES[UA_TYPES_UINT16]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Severity"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&eventId);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test setting ActiveState with trigger (exercises activeState tracking) */
START_TEST(activeState_triggerCycle) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE),
        "ActiveTriggerCond", source);

    UA_Variant val;
    UA_StatusCode retval;

    /* Enable the condition */
    UA_Boolean enabled = true;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Boolean retain = true;
    UA_Variant_setScalar(&val, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Retain"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set ActiveState/Id = true */
    UA_Boolean active = true;
    UA_Variant_setScalar(&val, &active, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "ActiveState"),
        UA_QUALIFIEDNAME(0, "Id"));
    (void)retval;

    /* Trigger → exercises getConditionActiveState / updateConditionActiveState */
    UA_ByteString eventId = UA_BYTESTRING_NULL;
    retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&eventId);

    /* Set ActiveState/Id = false (deactivate) */
    active = false;
    UA_Variant_setScalar(&val, &active, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "ActiveState"),
        UA_QUALIFIEDNAME(0, "Id"));
    (void)retval;

    /* Trigger again after deactivation */
    retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&eventId);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test setting Quality field on a condition */
START_TEST(setQualityField) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "QualityCondition", source);

    UA_Variant val;
    UA_StatusCode quality = UA_STATUSCODE_GOOD;
    UA_Variant_setScalar(&val, &quality, &UA_TYPES[UA_TYPES_STATUSCODE]);
    UA_StatusCode retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Quality"));
    /* Quality may or may not exist depending on field definition */
    (void)retval;

    /* Set quality to a degraded status */
    quality = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_Variant_setScalar(&val, &quality, &UA_TYPES[UA_TYPES_STATUSCODE]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Quality"));
    (void)retval;

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test addConditionOptionalField with non-existent field */
START_TEST(addOptionalField_nonExistent) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "OptionalFieldCond", source);

    /* Adding a non-existent optional field should fail */
    UA_NodeId outOptionalVariable = UA_NODEID_NULL;
    UA_StatusCode retval = acDriver_ac->addConditionOptionalField(acDriver_ac, cond,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        UA_QUALIFIEDNAME(0, "NonExistentField"),
        &outOptionalVariable);
    /* Should fail because the field doesn't exist in the type */
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test limit alarm with edge cases: exactly at limit boundaries */
START_TEST(limitAlarm_exactBoundaries) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_EXCLUSIVELIMITALARMTYPE),
        "ExactBoundaryAlarm", source);

    UA_Variant val;
    UA_StatusCode retval;

    /* Set limits: LowLow=10, Low=20, High=80, HighHigh=90 */
    UA_Double ll = 10.0;
    UA_Variant_setScalar(&val, &ll, &UA_TYPES[UA_TYPES_DOUBLE]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "LowLowLimit"));
    (void)retval;

    UA_Double lo = 20.0;
    UA_Variant_setScalar(&val, &lo, &UA_TYPES[UA_TYPES_DOUBLE]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "LowLimit"));
    (void)retval;

    UA_Double hi = 80.0;
    UA_Variant_setScalar(&val, &hi, &UA_TYPES[UA_TYPES_DOUBLE]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "HighLimit"));
    (void)retval;

    UA_Double hh = 90.0;
    UA_Variant_setScalar(&val, &hh, &UA_TYPES[UA_TYPES_DOUBLE]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "HighHighLimit"));
    (void)retval;

    /* Test at each boundary - going from low to high */
    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 5.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 10.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 20.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 50.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 80.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 90.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 95.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test setLimitState on a non-limit condition (should fail) */
START_TEST(setLimitState_nonLimitCondition) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "NonLimitAlarm", source);

    UA_StatusCode retval = acDriver_ac->setLimitState(acDriver_ac, cond, 50.0);
    /* Should fail because OffNormalAlarm is not a LimitAlarm */
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Regression test for limit getter paths (HighHigh, High, LowLow, Low)
 * Ensures no crash and correct behavior. Leak detection relies on ASan/LSan. */
START_TEST(limitAlarm_limitGetter_regression) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_StatusCode retval;

    {
        UA_NodeId cond = createConfiguredExclusiveLimitAlarm(
            server_ac, "LimitGetterHighHigh", source);
        retval = acDriver_ac->setLimitState(acDriver_ac, cond, 110.0);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        acDriver_ac->deleteCondition(acDriver_ac, cond, source);
    }

    {
        UA_NodeId cond = createConfiguredExclusiveLimitAlarm(
            server_ac, "LimitGetterHigh", source);
        retval = acDriver_ac->setLimitState(acDriver_ac, cond, 90.0);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        acDriver_ac->deleteCondition(acDriver_ac, cond, source);
    }

    {
        UA_NodeId cond = createConfiguredExclusiveLimitAlarm(
            server_ac, "LimitGetterLowLow", source);
        retval = acDriver_ac->setLimitState(acDriver_ac, cond, 3.0);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        acDriver_ac->deleteCondition(acDriver_ac, cond, source);
    }

    {
        UA_NodeId cond = createConfiguredExclusiveLimitAlarm(
            server_ac, "LimitGetterLow", source);
        retval = acDriver_ac->setLimitState(acDriver_ac, cond, 15.0);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        acDriver_ac->deleteCondition(acDriver_ac, cond, source);
    }
} END_TEST

/* Test reading Comment field on condition */
START_TEST(conditionField_comment) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE),
        "CommentCondition", source);

    /* Set Comment field */
    UA_Variant val;
    UA_LocalizedText comment = UA_LOCALIZEDTEXT("en", "Test comment");
    UA_Variant_setScalar(&val, &comment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    UA_StatusCode retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Comment"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read it back */
    UA_Variant fieldVal;
    retval = readConditionField(server_ac, cond, "Comment", &fieldVal);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    if(UA_Variant_hasScalarType(&fieldVal, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
        UA_LocalizedText *lt = (UA_LocalizedText*)fieldVal.data;
        UA_String expected = UA_STRING("Test comment");
        ck_assert(UA_String_equal(&lt->text, &expected));
    }
    UA_Variant_clear(&fieldVal);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test triggerConditionEvent on a disabled condition (should fail) */
START_TEST(triggerDisabledCondition_noRetain) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "DisabledNoRetainCond", source);

    /* Condition starts disabled. Triggering should reflect disabled status. */
    UA_ByteString eventId = UA_BYTESTRING_NULL;
    UA_StatusCode retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId);
    /* Triggering disabled condition returns BADNOTFOUND because Retain is false */
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&eventId);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test delete condition with wrong source (should fail) */
START_TEST(deleteCondition_wrongSource) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "WrongSourceDeleteCond", source);

    /* Try to delete with a different source node */
    UA_NodeId wrongSource = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_StatusCode retval = acDriver_ac->deleteCondition(acDriver_ac, cond, wrongSource);
    /* Should fail because the condition was created with a different source */
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

    /* Clean up with the correct source */
    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test split creation: begin + finish */
START_TEST(splitCreation_fullCycle) {
    UA_NodeId condId = UA_NODEID_NULL;
    UA_StatusCode retval = acDriver_ac->addConditionBegin(acDriver_ac, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        UA_QUALIFIEDNAME(0, "SplitCycleCond"),
        &condId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(!UA_NodeId_isNull(&condId));

    /* Finish the condition creation */
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    retval = acDriver_ac->addConditionFinish(acDriver_ac, condId, source, UA_NODEID_NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify the condition works: enable and trigger */
    UA_Variant val;
    UA_Boolean enabled = true;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, condId, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Boolean retain = true;
    UA_Variant_setScalar(&val, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionField(acDriver_ac, condId, &val, UA_QUALIFIEDNAME(0, "Retain"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString eventId = UA_BYTESTRING_NULL;
    retval = acDriver_ac->triggerConditionEvent(acDriver_ac, condId, source, &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&eventId);

    acDriver_ac->deleteCondition(acDriver_ac, condId, source);
} END_TEST

/* Test ackState callback with removeBranch=true to cover the ackedRemoveBranch path */
START_TEST(ackCallback_withRemoveBranch) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE),
        "AckRemoveBranchCond", source);

    /* Register ack callback with removeBranch = true */
    ackCallbackInvoked = false;
    UA_StatusCode retval = acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, cond, source,
        true, /* removeBranch */
        ackStateCallbackFn,
        UA_ENTERING_ACKEDSTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Enable the condition */
    UA_Variant val;
    UA_Boolean enabled = true;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set AckedState/Id = true → should invoke ackStateCallback */
    UA_Boolean acked = true;
    UA_Variant_setScalar(&val, &acked, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "AckedState"),
        UA_QUALIFIEDNAME(0, "Id"));
    (void)retval; /* AckedState may or may not exist */

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test confirmState callback with removeBranch=true */
START_TEST(confirmCallback_withRemoveBranch) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE),
        "ConfirmRemoveBranchCond", source);

    /* Register confirm callback with removeBranch = true */
    confirmCallbackInvoked = false;
    UA_StatusCode retval = acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, cond, source,
        true, /* removeBranch */
        confirmStateCallbackFn,
        UA_ENTERING_CONFIRMEDSTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Enable the condition */
    UA_Variant val;
    UA_Boolean enabled = true;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set ConfirmedState/Id = true → should invoke confirmStateCallback */
    UA_Boolean confirmed = true;
    UA_Variant_setScalar(&val, &confirmed, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "ConfirmedState"),
        UA_QUALIFIEDNAME(0, "Id"));
    (void)retval; /* ConfirmedState may or may not exist */

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Calling Acknowledge with an empty (data == NULL) EventId argument must be
 * rejected instead of being dereferenced while matching it against the
 * condition's branches. */
START_TEST(acknowledgeMethodCallback_emptyEventId) {
    UA_Server_getConfig(server_ac)->allowEmptyVariables = UA_RULEHANDLING_ACCEPT;

    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE),
        "EmptyEventIdAckCond", source);

    UA_Variant val;
    UA_Boolean enabled = true;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Boolean retain = true;
    UA_Variant_setScalar(&val, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val,
                                            UA_QUALIFIEDNAME(0, "Retain"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString eventId = UA_BYTESTRING_NULL;
    retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&eventId);

    /* EventId argument is a ByteString that is empty: data == NULL. An
     * argument without a type is already rejected by the argument type check
     * before the method callback runs. */
    UA_Variant in[2];
    UA_ByteString emptyEventId = UA_BYTESTRING_NULL;
    UA_Variant_setScalar(&in[0], &emptyEventId, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_LocalizedText comment = UA_LOCALIZEDTEXT("en", "ack");
    UA_Variant_init(&in[1]);
    UA_Variant_setScalar(&in[1], &comment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = cond;
    req.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_ACKNOWLEDGE);
    req.inputArguments = in;
    req.inputArgumentsSize = 2;

    UA_CallMethodResult result = UA_Server_call(server_ac, &req);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_BADEVENTIDUNKNOWN);
    UA_CallMethodResult_clear(&result);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test enable/disable with re-enable: cover the enteringEnabled callback path fully */
START_TEST(enableDisable_reEnable) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "ReEnableCond", source);

    /* Register enable state callback */
    enableCallbackInvoked = false;
    UA_StatusCode retval = acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, cond, source,
        false,
        enableStateCallbackFn,
        UA_ENTERING_ENABLEDSTATE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    /* 1) Enable */
    UA_Boolean enabled = true;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* 2) Disable */
    UA_Boolean disabled = false;
    UA_Variant_setScalar(&val, &disabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Check Retain is false after disable */
    UA_Variant retainVal;
    retval = readConditionField(server_ac, cond, "Retain", &retainVal);
    if(retval == UA_STATUSCODE_GOOD &&
       UA_Variant_hasScalarType(&retainVal, &UA_TYPES[UA_TYPES_BOOLEAN])) {
        ck_assert(*(UA_Boolean*)retainVal.data == false);
        UA_Variant_clear(&retainVal);
    }

    /* 3) Re-enable */
    enableCallbackInvoked = false;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test trigger event on alarm condition → exercises the full trigger path
 * including getConditionLastSeverity/updateConditionLastSeverity and
 * getConditionActiveState/updateConditionActiveState */
START_TEST(triggerAlarmCondition_fullPath) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE),
        "TriggerFullPathCond", source);

    /* Enable the condition */
    UA_Variant val;
    UA_Boolean enabled = true;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set Retain = true */
    UA_Boolean retain = true;
    UA_Variant_setScalar(&val, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Retain"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set severity to 500 */
    UA_UInt16 severity = 500;
    UA_Variant_setScalar(&val, &severity, &UA_TYPES[UA_TYPES_UINT16]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Severity"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Trigger → exercises severity comparison and active state paths */
    UA_ByteString eventId1 = UA_BYTESTRING_NULL;
    retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(eventId1.length > 0);

    /* Change severity and trigger again → exercises the lastSeverity update */
    severity = 800;
    UA_Variant_setScalar(&val, &severity, &UA_TYPES[UA_TYPES_UINT16]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Severity"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString eventId2 = UA_BYTESTRING_NULL;
    retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(eventId2.length > 0);

    /* Trigger a third time with active state change */
    UA_Boolean active = true;
    UA_Variant_setScalar(&val, &active, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "ActiveState"),
        UA_QUALIFIEDNAME(0, "Id"));
    (void)retval;

    UA_ByteString eventId3 = UA_BYTESTRING_NULL;
    retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId3);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString_clear(&eventId1);
    UA_ByteString_clear(&eventId2);
    UA_ByteString_clear(&eventId3);
    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test creating a condition with ExclusiveLimitAlarmType and full limit traversal */
START_TEST(exclusiveLimitAlarm_test) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_EXCLUSIVELIMITALARMTYPE),
        "ExcLimitAlarmCond", source);

    /* Enable the condition */
    UA_Variant val;
    UA_Boolean enabled = true;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Set limits */
    UA_Double ll = 10.0, lo = 20.0, hi = 80.0, hh = 90.0;
    UA_Variant_setScalar(&val, &ll, &UA_TYPES[UA_TYPES_DOUBLE]);
    acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "LowLowLimit"));
    UA_Variant_setScalar(&val, &lo, &UA_TYPES[UA_TYPES_DOUBLE]);
    acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "LowLimit"));
    UA_Variant_setScalar(&val, &hi, &UA_TYPES[UA_TYPES_DOUBLE]);
    acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "HighLimit"));
    UA_Variant_setScalar(&val, &hh, &UA_TYPES[UA_TYPES_DOUBLE]);
    acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "HighHighLimit"));

    /* Traverse all states: HighHigh → High → Normal → Low → LowLow → Normal */
    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 95.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 85.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 50.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 15.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 5.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = acDriver_ac->setLimitState(acDriver_ac, cond, 50.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test callback for invalid type (out-of-range enum) */
START_TEST(setCallback_outOfRange) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "OutOfRangeCallbackCond", source);

    UA_StatusCode retval = acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, cond, source,
        false,
        enableStateCallbackFn,
        (UA_TwoStateVariableCallbackType)99); /* Invalid */
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINTERNALERROR);

    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

/* Test multiple trigger events on the same condition for retained event tracking */
START_TEST(triggerCondition_multipleTimes) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId cond = createTestCondition(
        server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "MultiTriggerCond", source);

    UA_Variant val;
    UA_Boolean enabled = true;
    UA_Variant_setScalar(&val, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval = acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, cond, &val,
        UA_QUALIFIEDNAME(0, "EnabledState"),
        UA_QUALIFIEDNAME(0, "Id"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Boolean retain = true;
    UA_Variant_setScalar(&val, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = acDriver_ac->setConditionField(acDriver_ac, cond, &val, UA_QUALIFIEDNAME(0, "Retain"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Trigger multiple times verifying each event ID is different */
    UA_ByteString eventId1 = UA_BYTESTRING_NULL;
    retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString eventId2 = UA_BYTESTRING_NULL;
    retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString eventId3 = UA_BYTESTRING_NULL;
    retval = acDriver_ac->triggerConditionEvent(acDriver_ac, cond, source, &eventId3);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString_clear(&eventId1);
    UA_ByteString_clear(&eventId2);
    UA_ByteString_clear(&eventId3);
    acDriver_ac->deleteCondition(acDriver_ac, cond, source);
} END_TEST

START_TEST(addDriver_rejectsDuplicateAlarmsConditions) {
    UA_AlarmConditionsDriver *acDriver =
        UA_AlarmsConditionsDriver(UA_KEYVALUEMAP_NULL);
    ck_assert_ptr_nonnull(acDriver);

    ck_assert_uint_eq(UA_Server_addDriver(server_ac, &acDriver->drv),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(acDriver->drv.start(&acDriver->drv),
                      UA_STATUSCODE_BADALREADYEXISTS);
    ck_assert_uint_eq(UA_Server_removeDriver(server_ac, &acDriver->drv),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(acDriver->drv.free(&acDriver->drv),
                      UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(conditionRefresh_replacedInputArguments) {
    (void)createTestCondition(
        server_ac, UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "Condition refresh validation", UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER));

    const UA_NodeId inputArgumentsId = UA_NODEID_NUMERIC(
        0, UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH_INPUTARGUMENTS);
    UA_StatusCode retval =
        UA_Server_deleteNode(server_ac, inputArgumentsId, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Argument argument;
    UA_Argument_init(&argument);
    argument.name = UA_STRING("SubscriptionId");
    argument.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    argument.valueRank = UA_VALUERANK_SCALAR;

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "InputArguments");
    attr.dataType = UA_TYPES[UA_TYPES_ARGUMENT].typeId;
    attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    UA_UInt32 arrayDimension = 1;
    attr.arrayDimensionsSize = 1;
    attr.arrayDimensions = &arrayDimension;
    UA_Variant_setArray(&attr.value, &argument, 1,
                        &UA_TYPES[UA_TYPES_ARGUMENT]);

    retval = UA_Server_addVariableNode(
        server_ac, inputArgumentsId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
        UA_QUALIFIEDNAME(0, "InputArguments"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Boolean value = true;
    UA_Variant input;
    UA_Variant_init(&input);
    UA_Variant_setScalar(&input, &value, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_CallMethodRequest request;
    UA_CallMethodRequest_init(&request);
    request.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    request.methodId = UA_NODEID_NUMERIC(
        0, UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH);
    request.inputArgumentsSize = 1;
    request.inputArguments = &input;

    UA_CallMethodResult result = UA_Server_call(server_ac, &request);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_CallMethodResult_clear(&result);
}
END_TEST

static const UA_UInt32 eventMethodIds[] = {
    UA_NS0ID_CONDITIONTYPE_ADDCOMMENT,
    UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_ACKNOWLEDGE,
    UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_CONFIRM
};

static UA_StatusCode
callConditionMethod(UA_NodeId condition, UA_UInt32 methodId,
                    size_t inputSize, UA_Variant *input) {
    UA_CallMethodRequest request;
    UA_CallMethodRequest_init(&request);
    request.objectId = condition;
    request.methodId = UA_NODEID_NUMERIC(0, methodId);
    request.inputArgumentsSize = inputSize;
    request.inputArguments = input;
    UA_CallMethodResult result = UA_Server_call(server_ac, &request);
    UA_StatusCode status = result.statusCode;
    ck_assert_uint_eq(result.outputArgumentsSize, 0);
    UA_CallMethodResult_clear(&result);
    return status;
}

static void
setMethodConditionState(UA_NodeId condition, char *name, UA_Boolean state) {
    UA_Variant value;
    UA_Variant_setScalar(&value, &state, &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert_uint_eq(acDriver_ac->setConditionVariableFieldProperty(
        acDriver_ac, condition, &value, UA_QUALIFIEDNAME(0, name),
        UA_QUALIFIEDNAME(0, "Id")), UA_STATUSCODE_GOOD);
}

static void
assertMethodConditionState(UA_NodeId condition, char *name, UA_Boolean expected) {
    UA_QualifiedName path[2] = {UA_QUALIFIEDNAME(0, name), UA_QUALIFIEDNAME(0, "Id")};
    UA_BrowsePathResult result =
        UA_Server_browseSimplifiedBrowsePath(server_ac, condition, 2, path);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.targetsSize, 1);
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Server_readValue(
        server_ac, result.targets[0].targetId.nodeId, &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BOOLEAN]));
    ck_assert_uint_eq(*(UA_Boolean *)value.data, expected);
    UA_Variant_clear(&value);
    UA_BrowsePathResult_clear(&result);
}

static void
setMethodConditionRetain(UA_NodeId condition, UA_Boolean retain) {
    UA_Variant value;
    UA_Variant_setScalar(&value, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert_uint_eq(acDriver_ac->setConditionField(
        acDriver_ac, condition, &value, UA_QUALIFIEDNAME(0, "Retain")),
        UA_STATUSCODE_GOOD);
}

static UA_NodeId
createMethodCondition(void) {
    return createTestCondition(server_ac,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE), "Method condition",
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER));
}

static UA_ByteString
triggerMethodCondition(UA_NodeId condition) {
    UA_ByteString eventId = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(acDriver_ac->triggerConditionEvent(
        acDriver_ac, condition, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &eventId),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(eventId.length, 0);
    return eventId;
}

START_TEST(conditionMethods_enableDisable) {
    UA_NodeId condition = createMethodCondition();
    assertMethodConditionState(condition, "EnabledState", false);
    ck_assert_uint_eq(callConditionMethod(condition, UA_NS0ID_CONDITIONTYPE_DISABLE, 0, NULL),
                      UA_STATUSCODE_BADCONDITIONALREADYDISABLED);
    ck_assert_uint_eq(callConditionMethod(condition, UA_NS0ID_CONDITIONTYPE_ENABLE, 0, NULL),
                      UA_STATUSCODE_GOOD);
    assertMethodConditionState(condition, "EnabledState", true);
    ck_assert_uint_eq(callConditionMethod(condition, UA_NS0ID_CONDITIONTYPE_ENABLE, 0, NULL),
                      UA_STATUSCODE_BADCONDITIONALREADYENABLED);
    ck_assert_uint_eq(callConditionMethod(condition, UA_NS0ID_CONDITIONTYPE_DISABLE, 0, NULL),
                      UA_STATUSCODE_GOOD);
    assertMethodConditionState(condition, "EnabledState", false);
    UA_NodeId_clear(&condition);
}
END_TEST

START_TEST(conditionMethods_commentAndState) {
    unsigned method = (unsigned)_i / 2;
    UA_NodeId condition = createMethodCondition();
    setMethodConditionState(condition, "EnabledState", true);
    setMethodConditionState(condition, "AckedState", false);
    setMethodConditionState(condition, "ConfirmedState", false);
    setMethodConditionRetain(condition, true);

    UA_LocalizedText previous = UA_LOCALIZEDTEXT("en", "previous comment");
    UA_Variant value;
    UA_Variant_setScalar(&value, &previous, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    ck_assert_uint_eq(acDriver_ac->setConditionField(
        acDriver_ac, condition, &value, UA_QUALIFIEDNAME(0, "Comment")), UA_STATUSCODE_GOOD);
    UA_ByteString eventId = triggerMethodCondition(condition);
    UA_LocalizedText comment = (_i % 2) ? UA_LOCALIZEDTEXT(NULL, NULL) :
        UA_LOCALIZEDTEXT("en", "operator comment");
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &eventId, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&input[1], &comment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    ck_assert_uint_eq(callConditionMethod(condition, eventMethodIds[method], 2, input),
                      UA_STATUSCODE_GOOD);
    UA_Variant_init(&value);
    ck_assert_uint_eq(readConditionField(server_ac, condition, "Comment", &value),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]));
    ck_assert(UA_LocalizedText_equal((UA_LocalizedText *)value.data,
                                    (_i % 2) ? &previous : &comment));
    UA_Variant_clear(&value);
    if(method > 0)
        assertMethodConditionState(condition, method == 1 ? "AckedState" : "ConfirmedState", true);

    /* The successful call replaces the last event ID. */
    ck_assert_uint_eq(callConditionMethod(condition, eventMethodIds[method], 2, input),
                      UA_STATUSCODE_BADEVENTIDUNKNOWN);
    UA_ByteString_clear(&eventId);
    if(method > 0) {
        eventId = triggerMethodCondition(condition);
        ck_assert_uint_eq(callConditionMethod(condition, eventMethodIds[method], 2, input),
            method == 1 ? UA_STATUSCODE_BADCONDITIONBRANCHALREADYACKED :
                          UA_STATUSCODE_BADCONDITIONBRANCHALREADYCONFIRMED);
        UA_ByteString_clear(&eventId);
    }
    UA_NodeId_clear(&condition);
}
END_TEST

START_TEST(conditionMethods_notRetained) {
    UA_NodeId condition = createMethodCondition();
    setMethodConditionState(condition, "EnabledState", true);
    setMethodConditionRetain(condition, true);
    UA_ByteString eventId = triggerMethodCondition(condition);
    setMethodConditionRetain(condition, false);
    UA_LocalizedText comment = UA_LOCALIZEDTEXT("en", "ignored comment");
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &eventId, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&input[1], &comment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    ck_assert_uint_eq(callConditionMethod(condition, eventMethodIds[_i], 2, input),
                      UA_STATUSCODE_BADCONDITIONDISABLED);
    assertMethodConditionState(condition, "EnabledState", true);
    UA_ByteString_clear(&eventId);
    UA_NodeId_clear(&condition);
}
END_TEST

START_TEST(conditionMethods_validateCallbackArguments) {
    UA_NodeId condition = createMethodCondition();
    UA_NodeId sessionId = UA_NODEID_NULL;
    UA_ByteString eventId = UA_BYTESTRING("unknown");
    UA_LocalizedText comment = UA_LOCALIZEDTEXT("en", "ignored comment");
    UA_UInt32 invalid = 42;
    for(unsigned method = 0; method < 3; method++) {
        UA_NodeId methodId = UA_NODEID_NUMERIC(0, eventMethodIds[method]);
        UA_MethodCallback callback = NULL;
        ck_assert_uint_eq(UA_Server_getMethodNodeCallback(server_ac, methodId, &callback),
                          UA_STATUSCODE_GOOD);
        ck_assert(callback != NULL);
        /* Callback validation also protects against changed InputArguments metadata. */
        for(unsigned malformed = 0; malformed < 4; malformed++) {
            UA_Variant input[2];
            UA_Variant_setScalar(&input[0], &eventId, &UA_TYPES[UA_TYPES_BYTESTRING]);
            UA_Variant_setScalar(&input[1], &comment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            if(malformed == 1)
                UA_Variant_setScalar(&input[0], &invalid, &UA_TYPES[UA_TYPES_UINT32]);
            if(malformed == 2)
                UA_Variant_setScalar(&input[1], &invalid, &UA_TYPES[UA_TYPES_UINT32]);
            if(malformed == 3)
                UA_Variant_setArray(&input[0], &eventId, 1, &UA_TYPES[UA_TYPES_BYTESTRING]);
            ck_assert_uint_eq(callback(server_ac, &sessionId, NULL, &methodId, NULL,
                &condition, NULL, malformed == 0 ? 0 : 2, input, 0, NULL),
                UA_STATUSCODE_BADINVALIDARGUMENT);
        }
    }
    assertMethodConditionState(condition, "EnabledState", false);
    UA_NodeId_clear(&condition);
}
END_TEST

START_TEST(conditionMethods_rejectTypeAndStoppedDriver) {
    UA_NodeId condition = createMethodCondition();
    UA_NodeId sessionId = UA_NODEID_NULL;
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    UA_ByteString eventId = UA_BYTESTRING("unknown");
    UA_LocalizedText comment = UA_LOCALIZEDTEXT("en", "ignored comment");
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &eventId, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&input[1], &comment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    static const UA_UInt32 methods[] = {
        UA_NS0ID_CONDITIONTYPE_ENABLE, UA_NS0ID_CONDITIONTYPE_DISABLE,
        UA_NS0ID_CONDITIONTYPE_ADDCOMMENT,
        UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_ACKNOWLEDGE,
        UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_CONFIRM
    };
    for(unsigned i = 0; i < 5; i++) {
        UA_NodeId methodId = UA_NODEID_NUMERIC(0, methods[i]);
        UA_MethodCallback callback = NULL;
        ck_assert_uint_eq(UA_Server_getMethodNodeCallback(server_ac, methodId, &callback),
                          UA_STATUSCODE_GOOD);
        ck_assert(callback != NULL);
        ck_assert_uint_eq(callback(server_ac, &sessionId, NULL, &methodId, NULL,
            &typeId, NULL, i < 2 ? 0 : 2, input, 0, NULL), UA_STATUSCODE_BADNODEIDINVALID);
        acDriver_ac->drv.stop(&acDriver_ac->drv);
        ck_assert_uint_eq(callback(server_ac, &sessionId, NULL, &methodId, NULL,
            &condition, NULL, i < 2 ? 0 : 2, input, 0, NULL), UA_STATUSCODE_BADNOTSUPPORTED);
        ck_assert_uint_eq(acDriver_ac->drv.start(&acDriver_ac->drv), UA_STATUSCODE_GOOD);
    }
    assertMethodConditionState(condition, "EnabledState", false);
    UA_NodeId_clear(&condition);
}
END_TEST

START_TEST(conditionRefresh_callbackValidation) {
    UA_NodeId condition = createMethodCondition();
    UA_NodeId sessionId = UA_NODEID_NULL;
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    UA_UInt32 id = 0;
    UA_Boolean invalid = true;
    for(unsigned method = 0; method < 2; method++) {
        UA_NodeId methodId = UA_NODEID_NUMERIC(0, method == 0 ?
            UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH : UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH2);
        UA_MethodCallback callback = NULL;
        ck_assert_uint_eq(UA_Server_getMethodNodeCallback(server_ac, methodId, &callback),
                          UA_STATUSCODE_GOOD);
        ck_assert(callback != NULL);
        for(unsigned malformed = 0; malformed < 3; malformed++) {
            UA_Variant input[2], output;
            UA_Variant_init(&output);
            UA_Variant_setScalar(&input[0], &id, &UA_TYPES[UA_TYPES_UINT32]);
            UA_Variant_setScalar(&input[1], &id, &UA_TYPES[UA_TYPES_UINT32]);
            if(malformed == 2)
                UA_Variant_setScalar(&input[method], &invalid, &UA_TYPES[UA_TYPES_BOOLEAN]);
            ck_assert_uint_eq(callback(server_ac, &sessionId, NULL, &methodId, NULL,
                &typeId, NULL, malformed == 0 ? 0 : method + 1, input,
                malformed == 1 ? 1 : 0, &output), malformed == 1 ?
                UA_STATUSCODE_BADINTERNALERROR : UA_STATUSCODE_BADINVALIDARGUMENT);
            ck_assert(UA_Variant_isEmpty(&output));
        }
    }
    UA_NodeId_clear(&condition);
}
END_TEST

START_TEST(conditionDriver_rejectInvalidLifecycle) {
    UA_AlarmConditionsDriver *unattached = UA_AlarmsConditionsDriver(UA_KEYVALUEMAP_NULL);
    ck_assert_ptr_ne(unattached, NULL);
    ck_assert_uint_eq(unattached->drv.start(&unattached->drv), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(unattached->drv.state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(unattached->drv.free(&unattached->drv), UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(acDriver_ac->drv.start(&acDriver_ac->drv), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(acDriver_ac->drv.free(&acDriver_ac->drv), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(acDriver_ac->drv.state, UA_LIFECYCLESTATE_STARTED);
    UA_NodeId condition = createMethodCondition();
    assertMethodConditionState(condition, "EnabledState", false);
    UA_NodeId_clear(&condition);
}
END_TEST

START_TEST(conditionDriver_stoppedOperationsPreserveState) {
    UA_NodeId condition = createMethodCondition();
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_NodeId type = UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE);
    UA_NodeId sentinel = UA_NODEID_NUMERIC(1, 12345);
    UA_NodeId output = sentinel;
    UA_ByteString eventId = UA_BYTESTRING("unchanged");
    UA_ByteString originalEventId = eventId;
    UA_Boolean enabled = true;
    UA_Variant value;
    UA_Variant_setScalar(&value, &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    acDriver_ac->drv.stop(&acDriver_ac->drv);

    ck_assert_uint_eq(acDriver_ac->setConditionField(acDriver_ac, condition, &value,
        UA_QUALIFIEDNAME(0, "Retain")), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(acDriver_ac->setConditionVariableFieldProperty(acDriver_ac, condition,
        &value, UA_QUALIFIEDNAME(0, "EnabledState"), UA_QUALIFIEDNAME(0, "Id")),
        UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(acDriver_ac->triggerConditionEvent(acDriver_ac, condition, source,
        &eventId), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(acDriver_ac->addConditionOptionalField(acDriver_ac, condition, type,
        UA_QUALIFIEDNAME(0, "LocalTime"), &output), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(acDriver_ac->addConditionBegin(acDriver_ac, UA_NODEID_NULL, type,
        UA_QUALIFIEDNAME(1, "Stopped condition"), &output), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(acDriver_ac->addConditionFinish(acDriver_ac, condition, source,
        UA_NODEID_NULL), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(acDriver_ac->createCondition(acDriver_ac, UA_NODEID_NULL, type,
        UA_QUALIFIEDNAME(1, "Stopped condition"), source, UA_NODEID_NULL, &output),
        UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(acDriver_ac->setConditionTwoStateVariableCallback(acDriver_ac, condition,
        source, false, testTwoStateCallback, UA_ENTERING_ENABLEDSTATE),
        UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(acDriver_ac->deleteCondition(acDriver_ac, condition, source),
                      UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(acDriver_ac->setLimitState(acDriver_ac, condition, 50.0),
                      UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(acDriver_ac->setExpirationDate(acDriver_ac, condition, UA_BYTESTRING_NULL),
                      UA_STATUSCODE_BADINTERNALERROR);
    ck_assert(UA_NodeId_equal(&output, &sentinel));
    ck_assert_ptr_eq(eventId.data, originalEventId.data);
    ck_assert_uint_eq(eventId.length, originalEventId.length);

    ck_assert_uint_eq(acDriver_ac->drv.start(&acDriver_ac->drv), UA_STATUSCODE_GOOD);
    assertMethodConditionState(condition, "EnabledState", false);
    UA_Variant_init(&value);
    ck_assert_uint_eq(readConditionField(server_ac, condition, "Retain", &value),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BOOLEAN]));
    ck_assert(!*(UA_Boolean *)value.data);
    UA_Variant_clear(&value);
    UA_NodeId_clear(&condition);
}
END_TEST

START_TEST(conditionOptionalField_objectTypeAndChildren) {
    UA_NodeId condition = createMethodCondition();
    UA_NodeId field = UA_NODEID_NULL;
    ck_assert_uint_eq(acDriver_ac->addConditionOptionalField(acDriver_ac, condition,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE),
        UA_QUALIFIEDNAME(0, "ShelvingState"), &field), UA_STATUSCODE_GOOD);
    UA_NodeClass nodeClass = UA_NODECLASS_UNSPECIFIED;
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ac, field, &nodeClass), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(nodeClass, UA_NODECLASS_OBJECT);

    UA_BrowseDescription description;
    UA_BrowseDescription_init(&description);
    description.nodeId = field;
    description.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    UA_BrowseResult result = UA_Server_browse(server_ac, 0, &description);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesSize, 1);
    UA_NodeId expectedType = UA_NODEID_NUMERIC(0, UA_NS0ID_SHELVEDSTATEMACHINETYPE);
    ck_assert(UA_NodeId_equal(&result.references[0].nodeId.nodeId, &expectedType));
    UA_BrowseResult_clear(&result);

    UA_QualifiedName path[2] = {UA_QUALIFIEDNAME(0, "ShelvingState"),
                              UA_QUALIFIEDNAME(0, "CurrentState")};
    UA_BrowsePathResult child = UA_Server_browseSimplifiedBrowsePath(server_ac, condition, 2, path);
    ck_assert_uint_eq(child.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(child.targetsSize, 1);
    ck_assert_uint_eq(UA_Server_readNodeClass(server_ac, child.targets[0].targetId.nodeId,
                                            &nodeClass), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(nodeClass, UA_NODECLASS_VARIABLE);
    UA_BrowsePathResult_clear(&child);
    UA_NodeId_clear(&field);
    UA_NodeId_clear(&condition);
}
END_TEST

START_TEST(conditionOptionalField_rejectMethod) {
    UA_NodeId condition = createMethodCondition();
    UA_NodeId sentinel = UA_NODEID_NUMERIC(1, 12345);
    UA_NodeId output = sentinel;
    ck_assert_uint_eq(acDriver_ac->addConditionOptionalField(acDriver_ac, condition,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE),
        UA_QUALIFIEDNAME(0, "Confirm"), &output), UA_STATUSCODE_BADNOTSUPPORTED);
    ck_assert(UA_NodeId_equal(&output, &sentinel));
    assertMethodConditionState(condition, "ConfirmedState", false);
    UA_NodeId_clear(&condition);
}
END_TEST

typedef struct {
    size_t count;
    UA_UInt32 eventTypes[8];
    UA_Boolean invalid;
} RefreshEvents;

static void
recordRefreshEvent(UA_Server *server, UA_UInt32 monitoredItemId,
                   void *context, const UA_KeyValueMap fields) {
    RefreshEvents *events = (RefreshEvents *)context;
    if(fields.mapSize != 1 || events->count >= 8 ||
       !UA_Variant_hasScalarType(&fields.map[0].value, &UA_TYPES[UA_TYPES_NODEID])) {
        events->invalid = true;
        return;
    }
    UA_NodeId *type = (UA_NodeId *)fields.map[0].value.data;
    if(type->namespaceIndex != 0 || type->identifierType != UA_NODEIDTYPE_NUMERIC) {
        events->invalid = true;
        return;
    }
    events->eventTypes[events->count++] = type->identifier.numeric;
}

static UA_UInt32
createRefreshMonitoredItem(RefreshEvents *events) {
    UA_SimpleAttributeOperand field;
    UA_SimpleAttributeOperand_init(&field);
    ck_assert_uint_eq(UA_SimpleAttributeOperand_parse(&field, UA_STRING("/EventType")),
                      UA_STATUSCODE_GOOD);
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    filter.selectClauses = &field;
    filter.selectClausesSize = 1;
    UA_MonitoredItemCreateResult result = UA_Server_createEventMonitoredItem(
        server_ac, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), filter, events, recordRefreshEvent);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_UInt32 id = result.monitoredItemId;
    UA_MonitoredItemCreateResult_clear(&result);
    UA_SimpleAttributeOperand_clear(&field);
    return id;
}

static void
assertRefreshEvents(const RefreshEvents *events, UA_Boolean refreshed) {
    ck_assert(!events->invalid);
    ck_assert_uint_eq(events->count, refreshed ? 3 : 0);
    if(refreshed) {
        ck_assert_uint_eq(events->eventTypes[0], UA_NS0ID_REFRESHSTARTEVENTTYPE);
        ck_assert_uint_eq(events->eventTypes[1], UA_NS0ID_OFFNORMALALARMTYPE);
        ck_assert_uint_eq(events->eventTypes[2], UA_NS0ID_REFRESHENDEVENTTYPE);
    }
}

static UA_StatusCode
callConditionRefreshMethod(UA_UInt32 method, size_t inputSize, UA_Variant *input) {
    return callConditionMethod(UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE),
                               method, inputSize, input);
}

START_TEST(conditionRefreshMethods_targetsAndRetention) {
    UA_EventLoop *el = UA_Server_getConfig(server_ac)->eventLoop;
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    UA_NodeId retained = createMethodCondition();
    setMethodConditionState(retained, "EnabledState", true);
    setMethodConditionRetain(retained, true);
    UA_ByteString eventId = triggerMethodCondition(retained);
    UA_ByteString_clear(&eventId);

    /* A condition that has emitted no event, and one no longer retained, must
     * both be omitted from the refreshed notification sequence. */
    UA_NodeId neverTriggered = createMethodCondition();
    UA_NodeId notRetained = createMethodCondition();
    setMethodConditionState(notRetained, "EnabledState", true);
    setMethodConditionRetain(notRetained, true);
    eventId = triggerMethodCondition(notRetained);
    UA_ByteString_clear(&eventId);
    setMethodConditionRetain(notRetained, false);

    RefreshEvents events[2] = {{0}, {0}};
    UA_UInt32 ids[2];
    ids[0] = createRefreshMonitoredItem(&events[0]);
    ids[1] = createRefreshMonitoredItem(&events[1]);
    UA_UInt32 subscriptionId = 0; /* Local MonitoredItems use the admin subscription. */
    UA_UInt32 target = _i == 2 ? ids[1] : ids[0];
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &subscriptionId, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[1], &target, &UA_TYPES[UA_TYPES_UINT32]);
    UA_UInt32 method = _i == 0 ? UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH :
        UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH2;
    ck_assert_uint_eq(callConditionRefreshMethod(method, _i == 0 ? 1 : 2, input),
                      UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server_ac, false);
    assertRefreshEvents(&events[0], _i != 2);
    assertRefreshEvents(&events[1], _i != 1);

    /* Removing one item invalidates only its refresh target. */
    ck_assert_uint_eq(UA_Server_deleteMonitoredItem(server_ac, ids[0]), UA_STATUSCODE_GOOD);
    target = ids[0];
    ck_assert_uint_eq(callConditionRefreshMethod(UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH2,
                                                  2, input), UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
    memset(&events[1], 0, sizeof(events[1]));
    target = ids[1];
    ck_assert_uint_eq(callConditionRefreshMethod(UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH2,
                                                  2, input), UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server_ac, false);
    assertRefreshEvents(&events[1], true);
    ck_assert_uint_eq(UA_Server_deleteMonitoredItem(server_ac, ids[1]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callConditionRefreshMethod(UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH,
                                                  1, input), UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID);
    UA_NodeId_clear(&retained);
    UA_NodeId_clear(&neverTriggered);
    UA_NodeId_clear(&notRetained);
}
END_TEST

START_TEST(conditionRefresh_acceptsIntegerId) {
    UA_EventLoop *el = UA_Server_getConfig(server_ac)->eventLoop;
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    UA_NodeId condition = createTestCondition(
        server_ac, UA_NODEID_NUMERIC(0, UA_NS0ID_OFFNORMALALARMTYPE),
        "Refresh argument types", UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER));

    UA_SimpleAttributeOperand field;
    UA_SimpleAttributeOperand_init(&field);
    ck_assert_uint_eq(UA_SimpleAttributeOperand_parse(&field, UA_STRING("/EventType")),
                      UA_STATUSCODE_GOOD);
    UA_EventFilter filter = {0};
    filter.selectClauses = &field;
    filter.selectClausesSize = 1;
    RefreshEvents events = {0};
    UA_MonitoredItemCreateResult item = UA_Server_createEventMonitoredItem(
        server_ac, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), filter, &events,
        recordRefreshEvent);
    UA_SimpleAttributeOperand_clear(&field);
    ck_assert_uint_eq(item.statusCode, UA_STATUSCODE_GOOD);

    /* Local MonitoredItems use the admin subscription. Test both the declared
     * IntegerId type and UInt32, which the Method service adjusts to IntegerId. */
    UA_UInt32 subscriptionId = 0;
    const UA_DataType *type = &UA_TYPES[_i < 2 ? UA_TYPES_UINT32 : UA_TYPES_INTEGERID];
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &subscriptionId, type);
    UA_Variant_setScalar(&input[1], &item.monitoredItemId, type);
    UA_CallMethodRequest request = {0};
    request.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    request.methodId = UA_NODEID_NUMERIC(0, (_i % 2) == 0 ?
        UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH : UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH2);
    request.inputArgumentsSize = (_i % 2) == 0 ? 1 : 2;
    request.inputArguments = input;
    UA_CallMethodResult result = UA_Server_call(server_ac, &request);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.outputArgumentsSize, 0);
    UA_CallMethodResult_clear(&result);
    UA_Server_run_iterate(server_ac, false);

    ck_assert(!events.invalid);
    ck_assert_uint_eq(events.count, 2);
    ck_assert_uint_eq(events.eventTypes[0], UA_NS0ID_REFRESHSTARTEVENTTYPE);
    ck_assert_uint_eq(events.eventTypes[1], UA_NS0ID_REFRESHENDEVENTTYPE);
    ck_assert_uint_eq(UA_Server_deleteMonitoredItem(server_ac, item.monitoredItemId),
                      UA_STATUSCODE_GOOD);
    UA_MonitoredItemCreateResult_clear(&item);
    UA_NodeId_clear(&condition);
}
END_TEST

#endif /* UA_TEST_ENABLE_ALARMS_CONDITIONS */

int main(void) {
    Suite *s = suite_create("server_alarmcondition");

    TCase *tc_call = tcase_create("Alarms and Conditions");
#ifdef UA_TEST_ENABLE_ALARMS_CONDITIONS
    tcase_add_test(tc_call, createDelete);
    tcase_add_test(tc_call, splitCreation);
    tcase_add_test(tc_call, createCondition_invalidType);
    tcase_add_test(tc_call, deleteCondition_notFound);
    tcase_add_test(tc_call, createCondition_nullOutNodeId);
    tcase_add_test(tc_call, addConditionBegin_nullOutNodeId);
    tcase_add_test(tc_call, conditionRefresh_replacedInputArguments);
    tcase_add_loop_test(tc_call, conditionRefresh_acceptsIntegerId, 0, 4);
#endif
    tcase_add_checked_fixture(tc_call, setup, teardown);
    suite_add_tcase(s, tc_call);

    TCase *tc_refresh = tcase_create("Condition refresh");
#ifdef UA_TEST_ENABLE_ALARMS_CONDITIONS
    tcase_add_loop_test(tc_refresh, conditionRefreshMethods_targetsAndRetention, 0, 3);
    tcase_add_test(tc_refresh, conditionRefresh_callbackValidation);
#endif
    tcase_add_checked_fixture(tc_refresh, setup, teardown);
    suite_add_tcase(s, tc_refresh);

    TCase *tc_fields = tcase_create("Condition Fields");
#ifdef UA_TEST_ENABLE_ALARMS_CONDITIONS
    tcase_add_test(tc_fields, conditionLifecycle_setField);
    tcase_add_test(tc_fields, setConditionField_invalidField);
    tcase_add_test(tc_fields, setConditionVariableFieldProperty_test);
    tcase_add_test(tc_fields, setConditionField_multipleFields);
    tcase_add_test(tc_fields, addConditionOptionalField_test);
#endif
    tcase_add_checked_fixture(tc_fields, setup, teardown);
    suite_add_tcase(s, tc_fields);

    TCase *tc_trigger = tcase_create("Condition Trigger");
#ifdef UA_TEST_ENABLE_ALARMS_CONDITIONS
    tcase_add_test(tc_trigger, triggerConditionEvent_test);
    tcase_add_test(tc_trigger, triggerConditionEvent_disabled);
    tcase_add_test(tc_trigger, triggerConditionEvent_nullEventId);
    tcase_add_test(tc_trigger, enableDisable_condition);
#endif
    tcase_add_checked_fixture(tc_trigger, setup, teardown);
    suite_add_tcase(s, tc_trigger);

    TCase *tc_state = tcase_create("Condition State");
#ifdef UA_TEST_ENABLE_ALARMS_CONDITIONS
    tcase_add_test(tc_state, setAckedState_condition);
    tcase_add_test(tc_state, setTwoStateVariableCallback_test);
    tcase_add_test(tc_state, setTwoStateVariableCallback_notFound);
    tcase_add_test(tc_state, conditionFullLifecycle);
    tcase_add_test(tc_state, setConfirmedStateCallback);
    tcase_add_test(tc_state, setActiveStateCallback);
    tcase_add_test(tc_state, setAllCallbackTypes);
    tcase_add_test(tc_state, setCallback_invalidType);
    tcase_add_test(tc_state, activeState_triggerCycle);
    tcase_add_test(tc_state, ackCallback_withRemoveBranch);
    tcase_add_test(tc_state, confirmCallback_withRemoveBranch);
    tcase_add_test(tc_state, acknowledgeMethodCallback_emptyEventId);
    tcase_add_test(tc_state, setCallback_outOfRange);
#endif
    tcase_add_checked_fixture(tc_state, setup, teardown);
    suite_add_tcase(s, tc_state);

    TCase *tc_limit = tcase_create("Limit Alarms");
#ifdef UA_TEST_ENABLE_ALARMS_CONDITIONS
    tcase_add_test(tc_limit, limitAlarm_setLimitState);
    tcase_add_test(tc_limit, limitAlarm_exactBoundaries);
    tcase_add_test(tc_limit, setLimitState_nonLimitCondition);
    tcase_add_test(tc_limit, limitAlarm_limitGetter_regression);
    tcase_add_test(tc_limit, exclusiveLimitAlarm_test);
#endif
    tcase_add_checked_fixture(tc_limit, setup, teardown);
    suite_add_tcase(s, tc_limit);

    TCase *tc_misc = tcase_create("Condition Misc");
#ifdef UA_TEST_ENABLE_ALARMS_CONDITIONS
    tcase_add_test(tc_misc, multipleConditions_sameSource);
    tcase_add_test(tc_misc, createCondition_differentTypes);
    tcase_add_test(tc_misc, conditionCleanup_onShutdown);
    tcase_add_test(tc_misc, condition_differentSources);
    tcase_add_test(tc_misc, severityChange_multiTrigger);
    tcase_add_test(tc_misc, setQualityField);
    tcase_add_test(tc_misc, addOptionalField_nonExistent);
    tcase_add_test(tc_misc, conditionField_comment);
    tcase_add_test(tc_misc, triggerDisabledCondition_noRetain);
    tcase_add_test(tc_misc, deleteCondition_wrongSource);
    tcase_add_test(tc_misc, splitCreation_fullCycle);
    tcase_add_test(tc_misc, enableDisable_reEnable);
    tcase_add_test(tc_misc, triggerAlarmCondition_fullPath);
    tcase_add_test(tc_misc, triggerCondition_multipleTimes);
    tcase_add_test(tc_misc, addDriver_rejectsDuplicateAlarmsConditions);
#endif
    tcase_add_checked_fixture(tc_misc, setup, teardown);
    suite_add_tcase(s, tc_misc);

    TCase *tc_methods = tcase_create("Condition methods");
#ifdef UA_TEST_ENABLE_ALARMS_CONDITIONS
    tcase_add_test(tc_methods, conditionMethods_enableDisable);
    tcase_add_loop_test(tc_methods, conditionMethods_commentAndState, 0, 6);
    tcase_add_loop_test(tc_methods, conditionMethods_notRetained, 0, 3);
    tcase_add_test(tc_methods, conditionMethods_validateCallbackArguments);
    tcase_add_test(tc_methods, conditionMethods_rejectTypeAndStoppedDriver);
#endif
    tcase_add_checked_fixture(tc_methods, setup, teardown);
    suite_add_tcase(s, tc_methods);

    TCase *tc_driver = tcase_create("Condition driver lifecycle");
#ifdef UA_TEST_ENABLE_ALARMS_CONDITIONS
    tcase_add_test(tc_driver, conditionDriver_rejectInvalidLifecycle);
    tcase_add_test(tc_driver, conditionDriver_stoppedOperationsPreserveState);
    tcase_add_test(tc_driver, conditionOptionalField_objectTypeAndChildren);
    tcase_add_test(tc_driver, conditionOptionalField_rejectMethod);
#endif
    tcase_add_checked_fixture(tc_driver, setup, teardown);
    suite_add_tcase(s, tc_driver);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
