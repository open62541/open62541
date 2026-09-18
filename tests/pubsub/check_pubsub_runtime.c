/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include "ua_pubsub_internal.h"
#include "test_helpers.h"
#include "testing_networklayers.h"
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


static UA_Boolean
processSequence(UA_UInt32 number, UA_Int32 value, UA_Boolean network,
                UA_UInt16 writerId, UA_UInt16 publisherId, UA_Boolean delta) {
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(nm));
    nm.publisherIdEnabled = true;
    nm.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    nm.publisherId.id.uint16 = publisherId;
    nm.payloadHeaderEnabled = true;
    nm.messageCount = 1;
    nm.dataSetWriterIds[0] = writerId;
    nm.groupHeaderEnabled = network;
    nm.groupHeader.sequenceNumberEnabled = network;
    nm.groupHeader.sequenceNumber = (UA_UInt16)number;
    UA_DataSetMessage dsm;
    memset(&dsm, 0, sizeof(dsm));
    dsm.header.dataSetMessageValid = true;
    dsm.header.dataSetMessageSequenceNrEnabled = !network;
    dsm.header.dataSetMessageSequenceNr = number;
    dsm.fieldCount = 1;
    UA_DataValue field;
    UA_DataValue_init(&field);
    UA_Variant_setScalar(&field.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    field.hasValue = true;
    UA_DataSetMessage_DeltaFrameField deltaField;
    memset(&deltaField, 0, sizeof(deltaField));
    deltaField.value = field;
    if(delta) {
        dsm.header.dataSetMessageType = UA_DATASETMESSAGE_DATADELTAFRAME;
        dsm.data.deltaFrameFields = &deltaField;
    } else {
        dsm.data.keyFrameFields = &field;
    }
    nm.payload.dataSetMessages = &dsm;
    lockServer(server);
    UA_Boolean processed = UA_ReaderGroup_process(getPSM(server), readerGroup, &nm);
    unlockServer(server);
    return processed;
}

START_TEST(SequenceOrderingAndRollover) {
    UA_Boolean network = _i == 0;
    if(_i == 2)
        readerGroup->config.encodingMimeType = UA_PUBSUB_ENCODING_JSON;
    UA_UInt32 max = _i == 2 ? UA_UINT32_MAX : UA_UINT16_MAX;
    ck_assert(processSequence(max - 1, 10, network, 17, 1, false));
    ck_assert(!processSequence(max - 1, 99, network, 17, 1, false));
    ck_assert(!processSequence(max - 2, 99, network, 17, 1, false));
    assertTarget(true, 10, UA_STATUSCODE_GOOD);
    ck_assert(processSequence(max, 20, network, 17, 1, false));
    ck_assert(processSequence(0, 30, network, 17, 1, false));
    ck_assert(!processSequence(max, 99, network, 17, 1, false));
    ck_assert(!processSequence((max >> 2) + 2, 99, network, 17, 1, false));
    assertTarget(true, 30, UA_STATUSCODE_GOOD);
    ck_assert(processSequence(1, 40, network, 17, 1, false));
    assertTarget(true, 40, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(SequenceHistoriesArePerStream) {
    reader->config.dataSetWriterId = 0;
    ck_assert(processSequence(100, 10, false, 17, 1, false));
    ck_assert(processSequence(0, 20, false, 18, 1, false));
    ck_assert(processSequence(0, 30, false, 17, 2, false));
    ck_assert(!processSequence(99, 99, false, 17, 1, false));
    ck_assert(!processSequence(0, 99, false, 18, 1, false));
    assertTarget(true, 30, UA_STATUSCODE_GOOD);
    ck_assert(processSequence(101, 40, false, 17, 1, false));
    assertTarget(true, 40, UA_STATUSCODE_GOOD);
} END_TEST

static UA_DateTime sequenceTime;
static UA_DateTime sequenceNow(UA_EventLoop *el) { return sequenceTime; }

START_TEST(SequenceHistoryExpiresAfterTwoTimeouts) {
    UA_EventLoop *el = UA_Server_getConfig(server)->eventLoop;
    UA_DateTime (*originalNow)(UA_EventLoop*) = el->dateTime_nowMonotonic;
    el->dateTime_nowMonotonic = sequenceNow;
    sequenceTime = UA_DATETIME_SEC;
    reader->config.messageReceiveTimeout = 100;
    UA_Boolean first = processSequence(100, 10, false, 17, 1, false);
    sequenceTime += 150 * UA_DATETIME_MSEC;
    UA_Boolean premature = processSequence(0, 20, false, 17, 1, false);
    sequenceTime += 201 * UA_DATETIME_MSEC;
    UA_Boolean recovered = processSequence(0, 30, false, 17, 1, false);
    el->dateTime_nowMonotonic = originalNow;
    ck_assert(first);
    ck_assert(!premature);
    ck_assert(recovered);
    assertTarget(true, 30, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(DeltaGapRequiresNewKeyFrame) {
    reader->config.keyFrameCount = 10;
    ck_assert(processSequence(1, 10, false, 17, 1, false));
    ck_assert(processSequence(2, 20, false, 17, 1, true));
    assertTarget(true, 20, UA_STATUSCODE_GOOD);
    processSequence(4, 40, false, 17, 1, true);
    assertTarget(true, 20, UA_STATUSCODE_GOOD);
    processSequence(5, 50, false, 17, 1, true);
    assertTarget(true, 20, UA_STATUSCODE_GOOD);
    ck_assert(processSequence(6, 60, false, 17, 1, false));
    ck_assert(processSequence(7, 70, false, 17, 1, true));
    assertTarget(true, 70, UA_STATUSCODE_GOOD);
} END_TEST


typedef struct {
    UA_ByteString messages[16];
    size_t count;
} PublishCapture;

static void connectionCallback(UA_ConnectionManager *cm, uintptr_t id,
    void *app, void **context, UA_ConnectionState state,
    const UA_KeyValueMap *params, UA_ByteString msg) { }

static UA_StatusCode captureMessage(UA_ConnectionManager *cm, uintptr_t id,
    const UA_KeyValueMap *params, UA_ByteString *msg) {
    PublishCapture *capture = (PublishCapture*)TestConnectionManager_getContext(cm);
    ck_assert_uint_lt(capture->count, 16);
    UA_StatusCode res = UA_ByteString_copy(msg, &capture->messages[capture->count++]);
    cm->freeNetworkBuffer(cm, id, msg);
    return res;
}

static UA_WriterGroup *createPublisher(UA_UInt16 writers, UA_UInt16 configuredSize) {
    UA_PublishedDataSetConfig pc;
    memset(&pc, 0, sizeof(pc));
    pc.name = UA_STRING("source");
    UA_NodeId pdsId, wgId;
    ck_assert_uint_eq(UA_Server_addPublishedDataSet(server, &pc, &pdsId).addResult,
                      UA_STATUSCODE_GOOD);
    UA_DataSetFieldConfig fc;
    memset(&fc, 0, sizeof(fc));
    fc.field.variable.fieldNameAlias = UA_STRING("value");
    fc.field.variable.publishParameters.publishedVariable = targetId;
    fc.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert_uint_eq(UA_Server_addDataSetField(server, pdsId, &fc, NULL).result, UA_STATUSCODE_GOOD);
    UA_UadpWriterGroupMessageDataType ms;
    UA_UadpWriterGroupMessageDataType_init(&ms);
    ms.networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER |
        UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER | UA_UADPNETWORKMESSAGECONTENTMASK_NETWORKMESSAGENUMBER;
    UA_WriterGroupConfig wc;
    memset(&wc, 0, sizeof(wc));
    wc.name = UA_STRING("publisher");
    wc.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    wc.maxEncapsulatedDataSetMessageCount = 1;
    UA_ExtensionObject_setValue(&wc.messageSettings, &ms,
                               &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]);
    ck_assert_uint_eq(UA_Server_addWriterGroup(server, readerGroup->linkedConnection->head.identifier,
                                              &wc, &wgId), UA_STATUSCODE_GOOD);
    UA_UadpDataSetWriterMessageDataType ds;
    UA_UadpDataSetWriterMessageDataType_init(&ds);
    ds.configuredSize = configuredSize;
    for(UA_UInt16 i = 0; i < writers; i++) {
        UA_DataSetWriterConfig dc;
        memset(&dc, 0, sizeof(dc));
        dc.name = UA_STRING("writer");
        dc.dataSetWriterId = 17 + i;
        UA_ExtensionObject_setValue(&dc.messageSettings, &ds,
                                   &UA_TYPES[UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE]);
        ck_assert_uint_eq(UA_Server_addDataSetWriter(server, wgId, pdsId, &dc, NULL), UA_STATUSCODE_GOOD);
    }
    UA_WriterGroup *wg = UA_WriterGroup_find(getPSM(server), wgId);
    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &wg->writers, listEntry)
        dsw->head.state = UA_PUBSUBSTATE_OPERATIONAL;
    wg->head.state = UA_PUBSUBSTATE_OPERATIONAL;
    return wg;
}

static PublishCapture capturePublishes(UA_WriterGroup *wg, size_t cycles) {
    PublishCapture capture;
    memset(&capture, 0, sizeof(capture));
    UA_PubSubConnection *connection = wg->linkedConnection;
    UA_ConnectionManager *originalCm = connection->cm;
    uintptr_t originalChannel = connection->sendChannel;
    UA_ConnectionManager *cm = TestConnectionManager_new("udp", NULL);
    ck_assert_ptr_nonnull(cm);
    uintptr_t channel;
    ck_assert_uint_eq(TestConnectionManager_createConnection(cm, NULL, NULL,
        connectionCallback, &channel), UA_STATUSCODE_GOOD);
    TestConnectionManager_setContext(cm, &capture);
    cm->sendWithConnection = captureMessage;
    connection->cm = cm;
    connection->sendChannel = channel;
    for(size_t i = 0; i < cycles; i++)
        UA_Server_triggerWriterGroupPublish(server, wg->head.identifier);
    connection->cm = originalCm;
    connection->sendChannel = originalChannel;
    cm->eventSource.free(&cm->eventSource);
    return capture;
}

static void clearCapture(PublishCapture *capture) {
    for(size_t i = 0; i < capture->count; i++)
        UA_ByteString_clear(&capture->messages[i]);
}

START_TEST(NetworkMessageNumbersRestartEachCycle) {
    UA_WriterGroup *wg = createPublisher(3, 0);
    PublishCapture capture = capturePublishes(wg, 2);
    ck_assert_uint_eq(capture.count, 6);
    for(size_t i = 0; i < capture.count; i++) {
        UA_NetworkMessage nm;
        memset(&nm, 0, sizeof(nm));
        ck_assert_uint_eq(UA_NetworkMessage_decodeBinary(&capture.messages[i], &nm, NULL, NULL),
                          UA_STATUSCODE_GOOD);
        ck_assert(nm.groupHeader.networkMessageNumberEnabled);
        ck_assert_uint_eq(nm.groupHeader.networkMessageNumber, i % 3 + 1);
        UA_NetworkMessage_clear(&nm);
    }
    clearCapture(&capture);
} END_TEST


START_TEST(ConfiguredSizeReachesRuntimeEncoder) {
    UA_UInt16 size = _i == 0 ? 32 : 1;
    UA_WriterGroup *wg = createPublisher(2, size);
    wg->config.maxEncapsulatedDataSetMessageCount = 2;
    PublishCapture capture = capturePublishes(wg, 1);
    ck_assert_uint_eq(capture.count, 1);
    const UA_ByteString *buf = &capture.messages[0];
    /* UADP flags, GroupFlags/NetworkMessageNumber, payload count/writer ids,
     * then two UInt16 DataSetMessage sizes. */
    size_t sizesOffset = 9;
    ck_assert_uint_gt(buf->length, sizesOffset + 4);
    UA_UInt16 firstSize = (UA_UInt16)(buf->data[sizesOffset] | buf->data[sizesOffset + 1] << 8);
    UA_UInt16 secondSize = (UA_UInt16)(buf->data[sizesOffset + 2] | buf->data[sizesOffset + 3] << 8);
    ck_assert_uint_eq(firstSize, _i == 0 ? 32 : 8);
    ck_assert_uint_eq(secondSize, firstSize);
    size_t payload = sizesOffset + 4;
    ck_assert_int_eq((buf->data[payload] & 1) != 0, _i == 0);
    ck_assert_int_eq((buf->data[payload + firstSize] & 1) != 0, _i == 0);
    if(_i == 0) {
        for(size_t i = 8; i < 32; i++) {
            ck_assert_uint_eq(buf->data[payload + i], 0);
            ck_assert_uint_eq(buf->data[payload + 32 + i], 0);
        }
    }
    clearCapture(&capture);
} END_TEST

START_TEST(UnsupportedFixedPlacementIsRejected) {
    UA_WriterGroup *wg = createPublisher(1, 0);
    wg->head.state = UA_PUBSUBSTATE_DISABLED;
    UA_DataSetWriter *existing = LIST_FIRST(&wg->writers);
    UA_UadpDataSetWriterMessageDataType ms;
    UA_UadpDataSetWriterMessageDataType_init(&ms);
    if(_i == 0) ms.networkMessageNumber = 2;
    else ms.dataSetOffset = 40;
    UA_DataSetWriterConfig dc;
    memset(&dc, 0, sizeof(dc));
    dc.name = UA_STRING("fixed");
    dc.dataSetWriterId = 42;
    UA_ExtensionObject_setValue(&dc.messageSettings, &ms,
                               &UA_TYPES[UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE]);
    ck_assert_uint_eq(UA_Server_addDataSetWriter(server, wg->head.identifier,
        existing->connectedDataSet->head.identifier, &dc, NULL), UA_STATUSCODE_BADNOTSUPPORTED);
    readerGroup->head.state = UA_PUBSUBSTATE_DISABLED;
    UA_UadpDataSetReaderMessageDataType rm;
    UA_UadpDataSetReaderMessageDataType_init(&rm);
    rm.dataSetOffset = 40;
    UA_DataSetReaderConfig rc;
    memset(&rc, 0, sizeof(rc));
    rc.name = UA_STRING("fixed reader");
    UA_ExtensionObject_setValue(&rc.messageSettings, &rm,
                               &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE]);
    ck_assert_uint_eq(UA_Server_addDataSetReader(server, readerGroup->head.identifier,
                                                &rc, NULL), UA_STATUSCODE_BADNOTSUPPORTED);
} END_TEST

START_TEST(ConfiguredSizePadsEmptyMessages) {
    UA_DataSetMessage dsm;
    memset(&dsm, 0, sizeof(dsm));
    dsm.header.dataSetMessageValid = true;
    if(_i % 3 == 1) dsm.header.fieldEncoding = UA_FIELDENCODING_RAWDATA;
    if(_i % 3 == 2) dsm.header.dataSetMessageType = UA_DATASETMESSAGE_KEEPALIVE;
    UA_DataSetMessage_EncodingMetaData emd;
    memset(&emd, 0, sizeof(emd));
    emd.dataSetWriterId = 17;
    emd.configuredSize = _i < 3 ? 32 : 0;
    UA_NetworkMessage_EncodingOptions eo;
    memset(&eo, 0, sizeof(eo));
    eo.metaDataSize = 1;
    eo.metaData = &emd;
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(nm));
    nm.version = 1;
    nm.payloadHeaderEnabled = true;
    nm.messageCount = 1;
    nm.dataSetWriterIds[0] = 17;
    nm.payload.dataSetMessages = &dsm;
    size_t payloadSize = _i < 3 ? 32 : (_i % 3 == 0 ? 3 : (_i % 3 == 1 ? 1 : 2));
    ck_assert_uint_eq(UA_NetworkMessage_calcSizeBinary(&nm, &eo), 4 + payloadSize);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_NetworkMessage_encodeBinary(&nm, &buf, &eo), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(buf.length, 4 + payloadSize);
    if(_i < 3) {
        for(size_t i = 7; i < buf.length; i++)
            ck_assert_uint_eq(buf.data[i], 0);
    }
    UA_ByteString_clear(&buf);
} END_TEST

#ifdef UA_ENABLE_JSON_ENCODING
static const UA_UInt32 jsonHeaders = UA_JSONNETWORKMESSAGECONTENTMASK_NETWORKMESSAGEHEADER |
    UA_JSONNETWORKMESSAGECONTENTMASK_DATASETMESSAGEHEADER;

static void configureJsonPublisher(UA_WriterGroup *wg, UA_UInt32 mask) {
    wg->head.state = UA_PUBSUBSTATE_DISABLED;
    UA_WriterGroupConfig wc;
    ck_assert_uint_eq(UA_Server_getWriterGroupConfig(server, wg->head.identifier, &wc),
                      UA_STATUSCODE_GOOD);
    UA_ExtensionObject_clear(&wc.messageSettings);
    UA_JsonWriterGroupMessageDataType ms;
    UA_JsonWriterGroupMessageDataType_init(&ms);
    ms.networkMessageContentMask = (UA_JsonNetworkMessageContentMask)mask;
    ck_assert_uint_eq(UA_ExtensionObject_setValueCopy(&wc.messageSettings, &ms,
        &UA_TYPES[UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE]), UA_STATUSCODE_GOOD);
    wc.encodingMimeType = UA_PUBSUB_ENCODING_JSON;
    wc.maxEncapsulatedDataSetMessageCount = 10;
    ck_assert_uint_eq(UA_Server_updateWriterGroupConfig(server, wg->head.identifier, &wc),
                      UA_STATUSCODE_GOOD);
    UA_WriterGroupConfig_clear(&wc);
    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        UA_ExtensionObject_clear(&dsw->config.messageSettings);
        UA_JsonDataSetWriterMessageDataType ds;
        UA_JsonDataSetWriterMessageDataType_init(&ds);
        ds.dataSetMessageContentMask = UA_JSONDATASETMESSAGECONTENTMASK_DATASETWRITERID |
            UA_JSONDATASETMESSAGECONTENTMASK_MESSAGETYPE;
        ck_assert_uint_eq(UA_ExtensionObject_setValueCopy(&dsw->config.messageSettings, &ds,
            &UA_TYPES[UA_TYPES_JSONDATASETWRITERMESSAGEDATATYPE]), UA_STATUSCODE_GOOD);
        dsw->head.state = UA_PUBSUBSTATE_OPERATIONAL;
    }
    wg->head.state = UA_PUBSUBSTATE_OPERATIONAL;
}

START_TEST(JsonNetworkContentMaskControlsOutput) {
    UA_WriterGroup *wg = createPublisher(2, 0);
    UA_UInt32 mask = jsonHeaders;
    if(_i & 1) mask |= UA_JSONNETWORKMESSAGECONTENTMASK_PUBLISHERID;
    if(_i & 2) mask |= UA_JSONNETWORKMESSAGECONTENTMASK_DATASETCLASSID;
    if(_i & 4) mask |= UA_JSONNETWORKMESSAGECONTENTMASK_SINGLEDATASETMESSAGE;
    configureJsonPublisher(wg, mask);
    UA_Guid classId = {0x12345678, 0x1234, 0x5678, {1, 2, 3, 4, 5, 6, 7, 8}};
    LIST_FIRST(&wg->writers)->connectedDataSet->dataSetMetaData.dataSetClassId = classId;
    PublishCapture capture = capturePublishes(wg, 1);
    ck_assert_uint_eq(capture.count, (_i & 4) ? 2 : 1);
    for(size_t i = 0; i < capture.count; i++) {
        char json[4096];
        ck_assert_uint_lt(capture.messages[i].length, sizeof(json));
        memcpy(json, capture.messages[i].data, capture.messages[i].length);
        json[capture.messages[i].length] = 0;
        ck_assert_int_eq(strstr(json, "\"PublisherId\":") != NULL, (_i & 1) != 0);
        ck_assert_int_eq(strstr(json, "\"DataSetClassId\":") != NULL, (_i & 2) != 0);
        if(_i & 2)
            ck_assert_ptr_nonnull(strstr(json, "12345678-1234-5678-0102-030405060708"));
        ck_assert_ptr_nonnull(strstr(json, (_i & 4) ? "\"Messages\":{" : "\"Messages\":[{"));
        ck_assert_ptr_nonnull(strstr(json, "\"DataSetWriterId\":"));
        ck_assert_ptr_nonnull(strstr(json, "ua-keyframe"));
        ck_assert_ptr_nonnull(strstr(json, "\"value\":"));
        ck_assert_ptr_null(strstr(json, "\"SequenceNumber\":"));
        ck_assert_ptr_null(strstr(json, "\"Timestamp\":"));
        UA_NetworkMessage decoded;
        memset(&decoded, 0, sizeof(decoded));
        ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&capture.messages[i], &decoded, NULL, NULL),
                          UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(decoded.messageCount, (_i & 4) ? 1 : 2);
        ck_assert_int_eq(decoded.jsonSingleDataSetMessage, (_i & 4) != 0);
        ck_assert_uint_eq(decoded.payload.dataSetMessages[0].fieldCount, 1);
        ck_assert_int_eq(*(UA_Int32*)decoded.payload.dataSetMessages[0].data.keyFrameFields[0].value.data, 123);
        UA_NetworkMessage_clear(&decoded);
    }
    clearCapture(&capture);
} END_TEST

START_TEST(JsonClassIdsControlBatching) {
    UA_WriterGroup *wg = createPublisher(2, 0);
    UA_PublishedDataSetConfig pc;
    memset(&pc, 0, sizeof(pc));
    pc.name = UA_STRING("other source");
    UA_NodeId otherId;
    ck_assert_uint_eq(UA_Server_addPublishedDataSet(server, &pc, &otherId).addResult, UA_STATUSCODE_GOOD);
    UA_DataSetFieldConfig fc;
    memset(&fc, 0, sizeof(fc));
    fc.field.variable.fieldNameAlias = UA_STRING("value");
    fc.field.variable.publishParameters.publishedVariable = targetId;
    fc.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert_uint_eq(UA_Server_addDataSetField(server, otherId, &fc, NULL).result, UA_STATUSCODE_GOOD);
    UA_PublishedDataSet *otherPds = UA_PublishedDataSet_find(getPSM(server), otherId);
    otherPds->dataSetMetaData.dataSetClassId.data1 = 1234;
    wg->head.state = UA_PUBSUBSTATE_DISABLED;
    UA_DataSetWriterConfig dc;
    memset(&dc, 0, sizeof(dc));
    dc.name = UA_STRING("other class");
    dc.dataSetWriterId = 42;
    ck_assert_uint_eq(UA_Server_addDataSetWriter(server, wg->head.identifier,
        otherPds->head.identifier, &dc, NULL), UA_STATUSCODE_GOOD);
    configureJsonPublisher(wg, jsonHeaders | (_i ? UA_JSONNETWORKMESSAGECONTENTMASK_DATASETCLASSID : 0));
    PublishCapture capture = capturePublishes(wg, 1);
    ck_assert_uint_eq(capture.count, _i ? 2 : 1);
    for(size_t i = 0; i < capture.count; i++) {
        UA_NetworkMessage nm;
        memset(&nm, 0, sizeof(nm));
        ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&capture.messages[i], &nm, NULL, NULL),
                          UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(nm.messageCount, _i ? (i == 0 ? 2 : 1) : 3);
        ck_assert_int_eq(nm.dataSetClassIdEnabled, _i != 0);
        if(_i)
            ck_assert_uint_eq(nm.dataSetClassId.data1, i == 0 ? 0 : 1234);
        UA_NetworkMessage_clear(&nm);
    }
    clearCapture(&capture);
} END_TEST

START_TEST(UnsupportedJsonLayoutsAreRejected) {
    UA_JsonWriterGroupMessageDataType ms;
    UA_JsonWriterGroupMessageDataType_init(&ms);
    UA_UInt32 masks[] = {0, UA_JSONNETWORKMESSAGECONTENTMASK_NETWORKMESSAGEHEADER,
        UA_JSONNETWORKMESSAGECONTENTMASK_DATASETMESSAGEHEADER,
        jsonHeaders | UA_JSONNETWORKMESSAGECONTENTMASK_REPLYTO, jsonHeaders | 0x40};
    ms.networkMessageContentMask = (UA_JsonNetworkMessageContentMask)masks[_i];
    UA_WriterGroupConfig wc;
    memset(&wc, 0, sizeof(wc));
    wc.name = UA_STRING("unsupported json layout");
    wc.encodingMimeType = UA_PUBSUB_ENCODING_JSON;
    UA_ExtensionObject_setValue(&wc.messageSettings, &ms,
                               &UA_TYPES[UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE]);
    ck_assert_uint_eq(UA_Server_addWriterGroup(server, readerGroup->linkedConnection->head.identifier,
                                              &wc, NULL), UA_STATUSCODE_BADNOTSUPPORTED);
    UA_WriterGroup *wg = createPublisher(1, 0);
    wg->head.state = UA_PUBSUBSTATE_DISABLED;
    ck_assert_uint_eq(UA_Server_updateWriterGroupConfig(server, wg->head.identifier, &wc),
                      UA_STATUSCODE_BADNOTSUPPORTED);
    ck_assert_uint_eq(wg->config.encodingMimeType, UA_PUBSUB_ENCODING_UADP);
} END_TEST

START_TEST(UnsupportedJsonDataSetHeadersAreRejected) {
    UA_WriterGroup *wg = createPublisher(1, 0);
    configureJsonPublisher(wg, jsonHeaders);
    wg->head.state = UA_PUBSUBSTATE_DISABLED;
    UA_JsonDataSetWriterMessageDataType ms;
    UA_JsonDataSetWriterMessageDataType_init(&ms);
    UA_UInt32 required = UA_JSONDATASETMESSAGECONTENTMASK_DATASETWRITERID |
        UA_JSONDATASETMESSAGECONTENTMASK_MESSAGETYPE;
    UA_UInt32 masks[] = {0, UA_JSONDATASETMESSAGECONTENTMASK_DATASETWRITERID,
        UA_JSONDATASETMESSAGECONTENTMASK_MESSAGETYPE,
        required | UA_JSONDATASETMESSAGECONTENTMASK_DATASETWRITERNAME, required | 0x100};
    ms.dataSetMessageContentMask = (UA_JsonDataSetMessageContentMask)masks[_i];
    UA_DataSetWriterConfig dc;
    memset(&dc, 0, sizeof(dc));
    dc.name = UA_STRING("json writer");
    dc.dataSetWriterId = 42;
    UA_ExtensionObject_setValue(&dc.messageSettings, &ms,
                               &UA_TYPES[UA_TYPES_JSONDATASETWRITERMESSAGEDATATYPE]);
    ck_assert_uint_eq(UA_Server_addDataSetWriter(server, wg->head.identifier,
        LIST_FIRST(&wg->writers)->connectedDataSet->head.identifier, &dc, NULL),
        UA_STATUSCODE_BADNOTSUPPORTED);
    ms.dataSetMessageContentMask = (UA_JsonDataSetMessageContentMask)required;
    ck_assert_uint_eq(UA_Server_addDataSetWriter(server, wg->head.identifier,
        LIST_FIRST(&wg->writers)->connectedDataSet->head.identifier, &dc, NULL), UA_STATUSCODE_GOOD);
} END_TEST
#endif

int main(void) {
    Suite *suite = suite_create("PubSub runtime");
    TCase *tc = tcase_create("Runtime");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_loop_test(tc, FallbackQualityAndInitialDefault, 0, 3);
    tcase_add_loop_test(tc, StateChangesUpdateTargetsOnce, 0, 9);
    tcase_add_loop_test(tc, UadpReaderFiltersAreEnforced, 0, 3);
    tcase_add_loop_test(tc, SequenceOrderingAndRollover, 0, 3);
    tcase_add_test(tc, SequenceHistoriesArePerStream);
    tcase_add_test(tc, SequenceHistoryExpiresAfterTwoTimeouts);
    tcase_add_test(tc, DeltaGapRequiresNewKeyFrame);
    tcase_add_test(tc, NetworkMessageNumbersRestartEachCycle);
    tcase_add_loop_test(tc, ConfiguredSizeReachesRuntimeEncoder, 0, 2);
    tcase_add_loop_test(tc, ConfiguredSizePadsEmptyMessages, 0, 6);
    tcase_add_loop_test(tc, UnsupportedFixedPlacementIsRejected, 0, 2);
#ifdef UA_ENABLE_JSON_ENCODING
    tcase_add_loop_test(tc, JsonNetworkContentMaskControlsOutput, 0, 8);
    tcase_add_loop_test(tc, JsonClassIdsControlBatching, 0, 2);
    tcase_add_loop_test(tc, UnsupportedJsonLayoutsAreRejected, 0, 5);
    tcase_add_loop_test(tc, UnsupportedJsonDataSetHeadersAreRejected, 0, 5);
#endif
    suite_add_tcase(suite, tc);
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
