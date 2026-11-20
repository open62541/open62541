/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>

#include <check.h>
#include <stdlib.h>

#include "test_helpers.h"

typedef struct {
    unsigned count;
    UA_StatusCode status;
    UA_Double value;
} AnalogDataChange;

static UA_Server *server;
/* The fixture owns the callback context through MonitoredItem deletion. */
static AnalogDataChange change;
static UA_UInt32 monitoredItemId;
static UA_BrowsePathResult propertyPath;

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->httpEnabled = false;
    config->webSocketEnabled = false;
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);

    change = (AnalogDataChange){0};
    monitoredItemId = 0;
    UA_BrowsePathResult_init(&propertyPath);
}

static void
teardown(void) {
    if(monitoredItemId != 0)
        ck_assert_uint_eq(UA_Server_deleteMonitoredItem(server, monitoredItemId),
                          UA_STATUSCODE_GOOD);
    UA_BrowsePathResult_clear(&propertyPath);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
}

static void
analogDataChangeCallback(UA_Server *s, UA_UInt32 id, void *monitoredItemContext,
                         const UA_NodeId *nodeId, void *nodeContext,
                         UA_UInt32 attributeId, const UA_DataValue *value) {
    AnalogDataChange *observation = (AnalogDataChange*)monitoredItemContext;
    ck_assert(value->hasValue);
    ck_assert(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_DOUBLE]));
    observation->count++;
    observation->status = value->hasStatus ? value->status : UA_STATUSCODE_GOOD;
    observation->value = *(const UA_Double*)value->value.data;
}

static UA_Boolean
createEngineeringUnitsChild(UA_Server *s, const UA_NodeId *sessionId,
                            void *sessionContext, const UA_NodeId *sourceNodeId,
                            const UA_NodeId *targetParentNodeId,
                            const UA_NodeId *referenceTypeId) {
    const UA_NodeId declaration = UA_NS0ID(BASEANALOGTYPE_ENGINEERINGUNITS);
    const UA_NodeId analog = UA_NODEID_NUMERIC(1, 1340);
    const UA_NodeId hasProperty = UA_NS0ID(HASPROPERTY);
    return UA_NodeId_equal(sourceNodeId, &declaration) &&
           UA_NodeId_equal(targetParentNodeId, &analog) &&
           UA_NodeId_equal(referenceTypeId, &hasProperty);
}

static UA_GlobalNodeLifecycle engineeringUnitsLifecycle = {
    .createOptionalChild = createEngineeringUnitsChild
};

static void
assertReadOnlySemanticProperty(UA_NodeId node) {
    UA_Byte accessLevel = 0;
    ck_assert_uint_eq(UA_Server_readAccessLevel(server, node, &accessLevel),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(accessLevel, UA_ACCESSLEVELMASK_READ |
                      UA_ACCESSLEVELMASK_SEMANTICCHANGE);
}

static void
checkSemanticChange(UA_QualifiedName name, const UA_DataType *type,
                    void *initial, void *revised, UA_Boolean instantiateUnits) {
    UA_NodeId analog = UA_NODEID_NUMERIC(1, 1340);
    UA_Double sample = 40.0;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &sample, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_GlobalNodeLifecycle *previousLifecycle = config->nodeLifecycle;
    if(instantiateUnits)
        config->nodeLifecycle = &engineeringUnitsLifecycle;
    UA_StatusCode result = UA_Server_addVariableNode(
        server, analog, UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
        UA_QUALIFIEDNAME(1, "SemanticAnalog"), UA_NS0ID(ANALOGITEMTYPE),
        attr, NULL, NULL);
    config->nodeLifecycle = previousLifecycle;
    ck_assert_uint_eq(result, UA_STATUSCODE_GOOD);

    propertyPath = UA_Server_browseSimplifiedBrowsePath(server, analog, 1, &name);
    ck_assert_uint_eq(propertyPath.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(propertyPath.targetsSize, 1);
    UA_NodeId property = propertyPath.targets[0].targetId.nodeId;

    UA_Variant value;
    UA_Variant_setScalar(&value, initial, type);
    ck_assert_uint_eq(UA_Server_writeValue(server, property, value),
                      UA_STATUSCODE_GOOD);

    UA_MonitoredItemCreateRequest request;
    UA_MonitoredItemCreateRequest_init(&request);
    request.itemToMonitor.nodeId = analog;
    request.itemToMonitor.attributeId = UA_ATTRIBUTEID_VALUE;
    request.monitoringMode = UA_MONITORINGMODE_REPORTING;
    request.requestedParameters.samplingInterval = 0.0;
    request.requestedParameters.queueSize = 1;
    UA_MonitoredItemCreateResult mon = UA_Server_createDataChangeMonitoredItem(
        server, UA_TIMESTAMPSTORETURN_NEITHER, request, &change, analogDataChangeCallback);
    monitoredItemId = mon.monitoredItemId;
    result = mon.statusCode;
    UA_MonitoredItemCreateResult_clear(&mon);
    ck_assert_uint_eq(result, UA_STATUSCODE_GOOD);

    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(change.count, 1);
    ck_assert(change.value == 40.0);
    ck_assert_uint_eq(change.status, UA_STATUSCODE_GOOD);

    UA_Variant_setScalar(&value, revised, type);
    ck_assert_uint_eq(UA_Server_writeValue(server, property, value),
                      UA_STATUSCODE_GOOD);
    sample = 48.0;
    UA_Variant_setScalar(&value, &sample, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert_uint_eq(UA_Server_writeValue(server, analog, value),
                      UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(change.count, 2);
    ck_assert(change.value == 48.0);
    ck_assert_uint_eq(change.status, UA_STATUSCODE_GOOD | UA_STATUSCODE_SEMANTICSCHANGED);

    sample = 42.0;
    ck_assert_uint_eq(UA_Server_writeValue(server, analog, value),
                      UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(change.count, 3);
    ck_assert(change.value == 42.0);
    ck_assert_uint_eq(change.status, UA_STATUSCODE_GOOD);

    assertReadOnlySemanticProperty(property);
}

START_TEST(semanticChangeInheritedEURange) {
    UA_Range initial = {0.0, 100.0};
    UA_Range revised = {0.0, 50.0};
    checkSemanticChange(UA_QUALIFIEDNAME(0, "EURange"), &UA_TYPES[UA_TYPES_RANGE],
                        &initial, &revised, false);
}
END_TEST

START_TEST(semanticChangeInheritedEngineeringUnits) {
    UA_EUInformation initial = {0};
    initial.namespaceUri = UA_STRING("urn:open62541:test:units");
    initial.unitId = 1;
    initial.displayName = UA_LOCALIZEDTEXT("en", "Unit A");
    UA_EUInformation revised = initial;
    revised.unitId = 2;
    revised.displayName = UA_LOCALIZEDTEXT("en", "Unit B");
    checkSemanticChange(UA_QUALIFIEDNAME(0, "EngineeringUnits"),
                        &UA_TYPES[UA_TYPES_EUINFORMATION], &initial, &revised, true);
}
END_TEST

START_TEST(semanticPropertyTemplateMetadata) {
    static const UA_UInt32 propertyIds[] = {
        UA_NS0ID_ANALOGITEMTYPE_EURANGE,
        UA_NS0ID_BASEANALOGTYPE_ENGINEERINGUNITS,
#ifdef UA_GENERATED_NAMESPACE_ZERO_FULL
        UA_NS0ID_BASEANALOGTYPE_EURANGE,
        UA_NS0ID_ANALOGUNITTYPE_ENGINEERINGUNITS,
        UA_NS0ID_ANALOGUNITRANGETYPE_ENGINEERINGUNITS,
        UA_NS0ID_ARRAYITEMTYPE_EURANGE,
        UA_NS0ID_ARRAYITEMTYPE_ENGINEERINGUNITS,
        UA_NS0ID_IIETFBASENETWORKINTERFACETYPE_SPEED_ENGINEERINGUNITS,
        UA_NS0ID_IIEEEBASEETHERNETPORTTYPE_SPEED_ENGINEERINGUNITS,
        UA_NS0ID_IETFBASENETWORKINTERFACETYPE_SPEED_ENGINEERINGUNITS
#endif
    };
    for(size_t i = 0; i < sizeof(propertyIds) / sizeof(propertyIds[0]); ++i)
        assertReadOnlySemanticProperty(UA_NODEID_NUMERIC(0, propertyIds[i]));
}
END_TEST

int
main(void) {
    Suite *s = suite_create("Semantic notification");
    TCase *tc = tcase_create("Part 8 semantic properties");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, semanticChangeInheritedEURange);
    tcase_add_test(tc, semanticChangeInheritedEngineeringUnits);
    tcase_add_test(tc, semanticPropertyTemplateMetadata);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
