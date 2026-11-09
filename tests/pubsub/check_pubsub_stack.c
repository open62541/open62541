/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_pubsub_internal.h"
#include "test_helpers.h"

#include <check.h>
#include <stdlib.h>

#if defined(__unix__) || defined(__APPLE__)
# include <pthread.h>
#endif

static UA_Server *server;
static UA_NodeId writerGroupId;
static UA_NodeId dataSetId;
static UA_ByteString sentMessage;
static size_t sentCount;

static UA_StatusCode
allocBuffer(UA_ConnectionManager *cm, uintptr_t id, UA_ByteString *buf, size_t size) {
    return UA_ByteString_allocBuffer(buf, size);
}

static void
freeBuffer(UA_ConnectionManager *cm, uintptr_t id, UA_ByteString *buf) {
    UA_ByteString_clear(buf);
}

static UA_StatusCode
captureMessage(UA_ConnectionManager *cm, uintptr_t id,
               const UA_KeyValueMap *params, UA_ByteString *buf) {
    UA_ByteString_clear(&sentMessage);
    sentMessage = *buf;
    *buf = UA_BYTESTRING_NULL;
    sentCount++;
    return UA_STATUSCODE_GOOD;
}

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
    server->config.pubSubConfig.enableDeltaFrames = false;
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    sentCount = 0;
    UA_PubSubConnectionConfig cc;
    memset(&cc, 0, sizeof(cc));
    cc.name = UA_STRING("Stack test connection");
    cc.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType address = {UA_STRING_NULL,
                                           UA_STRING("opc.udp://127.0.0.1:4840")};
    UA_Variant_setScalar(&cc.address, &address,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    UA_NodeId connectionId;
    ck_assert_uint_eq(UA_Server_addPubSubConnection(server, &cc, &connectionId),
                      UA_STATUSCODE_GOOD);
    UA_WriterGroupConfig wc;
    memset(&wc, 0, sizeof(wc));
    wc.name = UA_STRING("Stack test WriterGroup");
    wc.publishingInterval = 100;
    UA_UadpWriterGroupMessageDataType settings;
    UA_UadpWriterGroupMessageDataType_init(&settings);
    settings.networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
    UA_ExtensionObject_setValueNoDelete(&wc.messageSettings, &settings,
                                        &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]);
    ck_assert_uint_eq(UA_Server_addWriterGroup(server, connectionId, &wc, &writerGroupId),
                      UA_STATUSCODE_GOOD);
    UA_PublishedDataSetConfig dc;
    memset(&dc, 0, sizeof(dc));
    dc.name = UA_STRING("Stack test DataSet");
    ck_assert_uint_eq(UA_Server_addPublishedDataSet(server, &dc, &dataSetId).addResult,
                      UA_STATUSCODE_GOOD);
    UA_DataSetFieldConfig fc;
    memset(&fc, 0, sizeof(fc));
    fc.field.variable.fieldNameAlias = UA_STRING("State");
    fc.field.variable.publishParameters.publishedVariable =
        UA_NS0ID(SERVER_SERVERSTATUS_STATE);
    fc.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert_uint_eq(UA_Server_addDataSetField(server, dataSetId, &fc, NULL).result,
                      UA_STATUSCODE_GOOD);
}

static void
teardown(void) {
    UA_ByteString_clear(&sentMessage);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void
runOnSmallStack(void *(*callback)(void *), void *context) {
#if defined(__unix__) || defined(__APPLE__)
    pthread_attr_t attr;
    ck_assert_int_eq(pthread_attr_init(&attr), 0);
    ck_assert_int_eq(pthread_attr_setstacksize(&attr, 128 * 1024), 0);
    pthread_t thread;
    ck_assert_int_eq(pthread_create(&thread, &attr, callback, context), 0);
    ck_assert_int_eq(pthread_attr_destroy(&attr), 0);
    ck_assert_int_eq(pthread_join(thread, NULL), 0);
#else
    callback(context);
#endif
}

/* Populate the internal list directly to avoid thousands of unrelated
 * information-model nodes. All writers reference the same one-field DataSet. */
static UA_DataSetWriter *
addWriters(UA_WriterGroup *wg, size_t count) {
    UA_DataSetWriter *writers = (UA_DataSetWriter*)UA_calloc(count, sizeof(*writers));
    ck_assert_ptr_ne(writers, NULL);
    for(size_t i = 0; i < count; i++) {
        writers[i].linkedWriterGroup = wg;
        writers[i].connectedDataSet = UA_PublishedDataSet_find(getPSM(server), dataSetId);
        writers[i].config.dataSetWriterId = (UA_UInt16)(i + 1);
        LIST_INSERT_HEAD(&wg->writers, &writers[i], listEntry);
    }
    wg->writersCount = (UA_UInt32)count;
    return writers;
}

static void
removeWriters(UA_WriterGroup *wg, UA_DataSetWriter *writers) {
    LIST_INIT(&wg->writers);
    wg->writersCount = 0;
    UA_free(writers);
}

static void *
publish(void *context) {
    UA_WriterGroup_publishCallback(getPSM(server), (UA_WriterGroup*)context);
    return NULL;
}

START_TEST(publishManyWriters) {
    const size_t counts[] = {16, 17, 8192};
    UA_Boolean json = (_i >= 3);
    UA_WriterGroup *wg = UA_WriterGroup_find(getPSM(server), writerGroupId);
    UA_DataSetWriter *writers = addWriters(wg, counts[_i % 3]);
    /* One enabled writer is sufficient to reach the encoding metadata path. */
    writers[0].head.state = UA_PUBSUBSTATE_OPERATIONAL;
    if(!json)
        writers[0].config.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_RAWDATA;
    wg->config.encodingMimeType = json ? UA_PUBSUB_ENCODING_JSON : UA_PUBSUB_ENCODING_UADP;

    UA_ConnectionManager cm;
    memset(&cm, 0, sizeof(cm));
    cm.allocNetworkBuffer = allocBuffer;
    cm.freeNetworkBuffer = freeBuffer;
    cm.sendWithConnection = captureMessage;
    UA_PubSubConnection *connection = wg->linkedConnection;
    UA_ConnectionManager *originalCM = connection->cm;
    connection->cm = &cm;
    connection->sendChannel = 1;
    runOnSmallStack(publish, wg);
    connection->cm = originalCM;
    connection->sendChannel = 0;

    UA_UInt16 generated = writers[0].actualDataSetMessageSequenceCount;
    UA_PublishedDataSet *pds = writers[0].connectedDataSet;
    removeWriters(wg, writers);
    ck_assert_uint_eq(sentCount, 1);
    ck_assert_uint_eq(generated, 1);
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(nm));
    UA_DataSetMessage_EncodingMetaData metadata = {0};
    metadata.dataSetWriterId = 1;
    metadata.fields = pds->dataSetMetaData.fields;
    metadata.fieldsSize = pds->dataSetMetaData.fieldsSize;
    UA_NetworkMessage_EncodingOptions eo = {1, &metadata};
    UA_StatusCode res;
#ifdef UA_ENABLE_JSON_ENCODING
    if(json)
        res = UA_NetworkMessage_decodeJson(&sentMessage, &nm, &eo, NULL);
    else
#endif
        res = UA_NetworkMessage_decodeBinary(&sentMessage, &nm, &eo, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(nm.messageCount, 1);
    ck_assert(nm.payload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert_uint_eq(nm.payload.dataSetMessages[0].fieldCount, 1);
    UA_NetworkMessage_clear(&nm);
} END_TEST

typedef struct {
    UA_StatusCode result;
    UA_PubSubOffsetTable table;
} OffsetContext;

static void *
computeOffsets(void *context) {
    OffsetContext *ctx = (OffsetContext*)context;
    ctx->result = UA_Server_computeWriterGroupOffsetTable(server, writerGroupId, &ctx->table);
    return NULL;
}

START_TEST(offsetTableWriterLimits) {
    const size_t counts[] = {1, UA_NETWORKMESSAGE_MAXMESSAGECOUNT - 1,
                            UA_NETWORKMESSAGE_MAXMESSAGECOUNT, 8192};
    UA_WriterGroup *wg = UA_WriterGroup_find(getPSM(server), writerGroupId);
    UA_DataSetWriter *writers = addWriters(wg, counts[_i]);
    OffsetContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    runOnSmallStack(computeOffsets, &ctx);
    UA_UInt16 generated = writers[0].actualDataSetMessageSequenceCount;
    size_t length = ctx.table.networkMessage.length;
    UA_Boolean empty = (ctx.table.networkMessage.data == NULL);
    UA_PubSubOffsetTable_clear(&ctx.table);
    removeWriters(wg, writers);
    if(_i < 2) {
        ck_assert_uint_eq(ctx.result, UA_STATUSCODE_GOOD);
        ck_assert_uint_gt(length, 0);
    } else {
        ck_assert_uint_eq(ctx.result, UA_STATUSCODE_BADINTERNALERROR);
        ck_assert(empty);
        /* Rejection must precede message generation as well as allocation. */
        ck_assert_uint_eq(generated, 0);
    }
} END_TEST

typedef struct {
    UA_ReaderGroup group;
    UA_ByteString buffer;
    UA_NetworkMessage message;
    UA_StatusCode result;
    UA_Boolean json;
} DecodeContext;

static void *
decode(void *context) {
    DecodeContext *ctx = (DecodeContext*)context;
    lockServer(server);
#ifdef UA_ENABLE_JSON_ENCODING
    if(ctx->json)
        ctx->result = UA_ReaderGroup_decodeNetworkMessageJSON(
            getPSM(server), &ctx->group, ctx->buffer, &ctx->message);
    else
#endif
        ctx->result = UA_ReaderGroup_decodeNetworkMessage(
            getPSM(server), &ctx->group, ctx->buffer, &ctx->message);
    unlockServer(server);
    return NULL;
}

START_TEST(decodeManyReaders) {
    const size_t counts[] = {16, 17, 8192};
    size_t count = counts[_i % 3];
    DecodeContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.json = (_i >= 3);
    ctx.group.readersCount = (UA_UInt32)count;
    UA_DataSetReader *readers = (UA_DataSetReader*)UA_calloc(count, sizeof(*readers));
    ck_assert_ptr_ne(readers, NULL);
    UA_FieldMetaData field;
    UA_FieldMetaData_init(&field);
    field.name = UA_STRING("Value");
    field.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    field.builtInType = UA_NS0ID_UINT32;
    field.valueRank = UA_VALUERANK_SCALAR;
    for(size_t i = 0; i < count; i++) {
        readers[i].linkedReaderGroup = &ctx.group;
        readers[i].config.dataSetWriterId = (UA_UInt16)(i + 1);
        readers[i].config.dataSetMetaData.fields = &field;
        readers[i].config.dataSetMetaData.fieldsSize = 1;
        LIST_INSERT_HEAD(&ctx.group.readers, &readers[i], listEntry);
    }

    UA_UInt32 value = 42;
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    dv.hasValue = true;
    UA_Variant_setScalar(&dv.value, &value, &UA_TYPES[UA_TYPES_UINT32]);
    UA_DataSetMessage dsm;
    memset(&dsm, 0, sizeof(dsm));
    dsm.header.dataSetMessageValid = true;
    dsm.header.fieldEncoding = ctx.json ? UA_FIELDENCODING_VARIANT : UA_FIELDENCODING_RAWDATA;
    dsm.fieldCount = 1;
    dsm.data.keyFrameFields = &dv;
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(nm));
    nm.version = 1;
    nm.payloadHeaderEnabled = true;
    nm.messageCount = 1;
    nm.dataSetWriterIds[0] = 1;
    nm.payload.dataSetMessages = &dsm;
    UA_DataSetMessage_EncodingMetaData metadata = {0};
    metadata.dataSetWriterId = 1;
    metadata.fields = &field;
    metadata.fieldsSize = 1;
    UA_NetworkMessage_EncodingOptions eo = {1, &metadata};
    UA_ByteString encoded = UA_BYTESTRING_NULL;
    UA_StatusCode res;
#ifdef UA_ENABLE_JSON_ENCODING
    if(ctx.json)
        res = UA_NetworkMessage_encodeJson(&nm, &encoded, &eo, NULL);
    else
#endif
        res = UA_NetworkMessage_encodeBinary(&nm, &encoded, &eo);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ctx.buffer = encoded;
    runOnSmallStack(decode, &ctx);
    ck_assert_uint_eq(ctx.result, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ctx.message.messageCount, 1);
    ck_assert_uint_eq(ctx.message.payload.dataSetMessages[0].fieldCount, 1);
    UA_Variant *decoded = &ctx.message.payload.dataSetMessages[0].data.keyFrameFields[0].value;
    ck_assert(UA_Variant_hasScalarType(decoded, &UA_TYPES[UA_TYPES_UINT32]));
    ck_assert_uint_eq(*(UA_UInt32*)decoded->data, value);
    UA_NetworkMessage_clear(&ctx.message);

    /* Fail after metadata allocation to exercise the cleanup path. */
    ctx.buffer.length--;
    runOnSmallStack(decode, &ctx);
    ck_assert_uint_ne(ctx.result, UA_STATUSCODE_GOOD);
    UA_NetworkMessage_clear(&ctx.message);

    if(!ctx.json) {
        /* No payload header: synthesizing one must also release metadata
         * when the configured reader count exceeds the message limit. */
        UA_Byte header = 1;
        ctx.buffer.data = &header;
        ctx.buffer.length = 1;
        runOnSmallStack(decode, &ctx);
        ck_assert_uint_ne(ctx.result, UA_STATUSCODE_GOOD);
        UA_NetworkMessage_clear(&ctx.message);
    }
    UA_ByteString_clear(&encoded);
    UA_free(readers);
} END_TEST

int main(void) {
    Suite *s = suite_create("PubSub bounded stack usage");
    TCase *tc = tcase_create("Publishing, receiving and offset tables");
    tcase_add_checked_fixture(tc, setup, teardown);
    int formats = 1;
#ifdef UA_ENABLE_JSON_ENCODING
    formats = 2;
#endif
    tcase_add_loop_test(tc, publishManyWriters, 0, 3 * formats);
    tcase_add_loop_test(tc, decodeManyReaders, 0, 3 * formats);
    tcase_add_loop_test(tc, offsetTableWriterLimits, 0, 4);
    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
