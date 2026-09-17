/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include "ua_pubsub_internal.h"
#include "test_helpers.h"
#include <check.h>

static UA_Server *server;
static UA_NodeId targetId;
static UA_DataSetReader *reader;
static UA_ReaderGroup *readerGroup;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_nonnull(server);
    UA_PubSubConnectionConfig cc;
    memset(&cc, 0, sizeof(cc));
    cc.name = UA_STRING("connection");
    cc.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType address = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&cc.address, &address, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    UA_NodeId connectionId, groupId, readerId;
    ck_assert_uint_eq(UA_Server_addPubSubConnection(server, &cc, &connectionId), UA_STATUSCODE_GOOD);
    UA_ReaderGroupConfig gc;
    memset(&gc, 0, sizeof(gc));
    gc.name = UA_STRING("group");
    ck_assert_uint_eq(UA_Server_addReaderGroup(server, connectionId, &gc, &groupId), UA_STATUSCODE_GOOD);
    UA_DataSetReaderConfig rc;
    memset(&rc, 0, sizeof(rc));
    rc.name = UA_STRING("reader");
    rc.dataSetWriterId = 17;
    UA_FieldMetaData fmd;
    UA_FieldMetaData_init(&fmd);
    fmd.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    fmd.builtInType = UA_NS0ID_INT32;
    fmd.valueRank = UA_VALUERANK_SCALAR;
    rc.dataSetMetaData.fields = &fmd;
    rc.dataSetMetaData.fieldsSize = 1;
    ck_assert_uint_eq(UA_Server_addDataSetReader(server, groupId, &rc, &readerId), UA_STATUSCODE_GOOD);
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.valueRank = UA_VALUERANK_SCALAR;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE |
        UA_ACCESSLEVELMASK_STATUSWRITE | UA_ACCESSLEVELMASK_TIMESTAMPWRITE;
    UA_Int32 initial = 123;
    UA_Variant_setScalar(&attr.value, &initial, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(UA_Server_addVariableNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "target"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, &targetId), UA_STATUSCODE_GOOD);
    UA_FieldTargetDataType target;
    UA_FieldTargetDataType_init(&target);
    target.targetNodeId = targetId;
    target.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_Int32 override = 99;
    UA_Variant_setScalar(&target.overrideValue, &override, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(UA_Server_DataSetReader_createTargetVariables(server, readerId, 1, &target),
                      UA_STATUSCODE_GOOD);
    reader = UA_DataSetReader_find(getPSM(server), readerId);
    readerGroup = reader->linkedReaderGroup;
    readerGroup->head.state = UA_PUBSUBSTATE_OPERATIONAL;
    readerGroup->hasReceived = true;
    reader->head.state = UA_PUBSUBSTATE_OPERATIONAL;
}

static void teardown(void) {
    UA_Server_delete(server);
}

static void processValue(UA_Int32 value, UA_StatusCode status) {
    UA_DataValue field;
    UA_DataValue_init(&field);
    UA_Variant_setScalar(&field.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    field.hasValue = true;
    field.hasStatus = true;
    field.status = status;
    UA_DataSetMessage message;
    memset(&message, 0, sizeof(message));
    message.header.dataSetMessageValid = true;
    message.fieldCount = 1;
    message.data.keyFrameFields = &field;
    lockServer(server);
    UA_DataSetReader_process(getPSM(server), reader, &message);
    unlockServer(server);
}

static void assertTarget(UA_Boolean hasValue, UA_Int32 value, UA_StatusCode status) {
    UA_ReadValueId rv;
    UA_ReadValueId_init(&rv);
    rv.nodeId = targetId;
    rv.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataValue actual = UA_Server_read(server, &rv, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert_uint_eq(actual.status, status);
    if(!hasValue)
        ck_assert(UA_Variant_isEmpty(&actual.value));
    if(hasValue) {
        ck_assert(actual.hasValue);
        ck_assert_ptr_eq(actual.value.type, &UA_TYPES[UA_TYPES_INT32]);
        ck_assert_int_eq(*(UA_Int32*)actual.value.data, value);
    }
    UA_DataValue_clear(&actual);
}

static void setMode(unsigned mode) {
    static const UA_OverrideValueHandling modes[] = {
        UA_OVERRIDEVALUEHANDLING_DISABLED, UA_OVERRIDEVALUEHANDLING_LASTUSABLEVALUE,
        UA_OVERRIDEVALUEHANDLING_OVERRIDEVALUE
    };
    reader->config.subscribedDataSet.target.targetVariables[0].overrideValueHandling = modes[mode];
}

START_TEST(FallbackQualityAndInitialDefault) {
    setMode((unsigned)_i);
    processValue(20, UA_STATUSCODE_BADNOCOMMUNICATION);
    assertTarget(_i != 0, _i == 1 ? 0 : 99, _i == 0 ? UA_STATUSCODE_BADNOCOMMUNICATION :
        (_i == 1 ? UA_STATUSCODE_UNCERTAINLASTUSABLEVALUE : UA_STATUSCODE_GOODLOCALOVERRIDE));
    processValue(30, UA_STATUSCODE_UNCERTAIN);
    assertTarget(true, 30, UA_STATUSCODE_UNCERTAIN);
    processValue(40, UA_STATUSCODE_BADNOCOMMUNICATION);
    assertTarget(_i != 0, _i == 1 ? 30 : 99, _i == 0 ? UA_STATUSCODE_BADNOCOMMUNICATION :
        (_i == 1 ? UA_STATUSCODE_UNCERTAINLASTUSABLEVALUE : UA_STATUSCODE_GOODLOCALOVERRIDE));
    processValue(50, UA_STATUSCODE_GOOD);
    assertTarget(true, 50, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(StateChangesUpdateTargetsOnce) {
    unsigned mode = (unsigned)_i % 3;
    static const UA_PubSubState states[] = {UA_PUBSUBSTATE_DISABLED, UA_PUBSUBSTATE_PAUSED,
                                         UA_PUBSUBSTATE_ERROR};
    UA_PubSubState state = states[_i / 3];
    setMode(mode);
    processValue(30, UA_STATUSCODE_GOOD);
    if(state == UA_PUBSUBSTATE_PAUSED)
        readerGroup->head.state = UA_PUBSUBSTATE_PAUSED;
    lockServer(server);
    UA_StatusCode res = UA_DataSetReader_setPubSubState(getPSM(server), reader, state,
                                                       UA_STATUSCODE_GOOD);
    unlockServer(server);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_StatusCode status = mode == 1 ? UA_STATUSCODE_UNCERTAINLASTUSABLEVALUE :
        (mode == 2 ? UA_STATUSCODE_GOODLOCALOVERRIDE : (state == UA_PUBSUBSTATE_ERROR ?
        UA_STATUSCODE_BADNOCOMMUNICATION : UA_STATUSCODE_BADOUTOFSERVICE));
    assertTarget(mode != 0, mode == 1 ? 30 : 99, status);
    UA_Int32 changed = 77;
    UA_Variant v;
    UA_Variant_setScalar(&v, &changed, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(UA_Server_writeValue(server, targetId, v), UA_STATUSCODE_GOOD);
    lockServer(server);
    res = UA_DataSetReader_setPubSubState(getPSM(server), reader, state, UA_STATUSCODE_GOOD);
    unlockServer(server);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertTarget(true, 77, UA_STATUSCODE_GOOD);
} END_TEST


START_TEST(UadpReaderFiltersAreEnforced) {
    UA_UadpDataSetReaderMessageDataType settings;
    UA_UadpDataSetReaderMessageDataType_init(&settings);
    UA_Guid classId = UA_GUID("01234567-89ab-cdef-0123-456789abcdef");
    if(_i == 0) settings.groupVersion = 42;
    if(_i == 1) settings.networkMessageNumber = 2;
    if(_i == 2) settings.dataSetClassId = classId;
    ck_assert_uint_eq(UA_ExtensionObject_setValueCopy(&reader->config.messageSettings,
        &settings, &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE]), UA_STATUSCODE_GOOD);
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(nm));
    nm.groupHeaderEnabled = true;
    nm.groupHeader.groupVersionEnabled = true;
    nm.groupHeader.groupVersion = 42;
    nm.groupHeader.networkMessageNumberEnabled = true;
    nm.groupHeader.networkMessageNumber = 2;
    nm.dataSetClassIdEnabled = true;
    nm.dataSetClassId = classId;
    nm.payloadHeaderEnabled = true;
    nm.messageCount = 1;
    nm.dataSetWriterIds[0] = 17;
    UA_Int32 value = 31;
    UA_DataValue field;
    UA_DataValue_init(&field);
    UA_Variant_setScalar(&field.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    field.hasValue = true;
    UA_DataSetMessage dsm;
    memset(&dsm, 0, sizeof(dsm));
    dsm.header.dataSetMessageValid = true;
    dsm.fieldCount = 1;
    dsm.data.keyFrameFields = &field;
    nm.payload.dataSetMessages = &dsm;
    lockServer(server);
    UA_Boolean processed = UA_ReaderGroup_process(getPSM(server), readerGroup, &nm);
    unlockServer(server);
    ck_assert(processed);
    assertTarget(true, 31, UA_STATUSCODE_GOOD);
    value = 88;
    if(_i == 0) nm.groupHeader.groupVersion++;
    if(_i == 1) nm.groupHeader.networkMessageNumber++;
    if(_i == 2) nm.dataSetClassId.data1++;
    lockServer(server);
    processed = UA_ReaderGroup_process(getPSM(server), readerGroup, &nm);
    unlockServer(server);
    ck_assert(!processed);
    assertTarget(true, 31, UA_STATUSCODE_GOOD);
    if(_i == 0) nm.groupHeader.groupVersionEnabled = false;
    if(_i == 1) nm.groupHeader.networkMessageNumberEnabled = false;
    if(_i == 2) nm.dataSetClassIdEnabled = false;
    ck_assert_uint_eq(UA_DataSetReader_checkIdentifier(getPSM(server), reader, &nm),
                      UA_STATUSCODE_BADNOTFOUND);
    /* Null settings disable the filters. */
    UA_UadpDataSetReaderMessageDataType *stored =
        (UA_UadpDataSetReaderMessageDataType*)reader->config.messageSettings.content.decoded.data;
    stored->groupVersion = 0;
    stored->networkMessageNumber = 0;
    stored->dataSetClassId = UA_GUID_NULL;
    ck_assert_uint_eq(UA_DataSetReader_checkIdentifier(getPSM(server), reader, &nm),
                      UA_STATUSCODE_GOOD);
} END_TEST

int main(void) {
    Suite *suite = suite_create("PubSub runtime");
    TCase *tc = tcase_create("Runtime");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_loop_test(tc, FallbackQualityAndInitialDefault, 0, 3);
    tcase_add_loop_test(tc, StateChangesUpdateTargetsOnce, 0, 9);
    tcase_add_loop_test(tc, UadpReaderFiltersAreEnforced, 0, 3);
    suite_add_tcase(suite, tc);
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
