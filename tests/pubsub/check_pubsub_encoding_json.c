/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Lukas Meling)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/client.h>
#include <open62541/types.h>
#include <open62541/util.h>
#include <open62541/pubsub.h>

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

START_TEST(UA_PubSub_CompactJsonOptions) {
    UA_FieldMetaData field = {0};
    field.name = UA_STRING("Field");
    UA_DataSetMessage_EncodingMetaData metadata = {0};
    metadata.dataSetWriterId = 1;
    metadata.fieldsSize = 1;
    metadata.fields = &field;
    UA_NetworkMessage_EncodingOptions encodingOptions = {0};
    encodingOptions.metaDataSize = 1;
    encodingOptions.metaData = &metadata;

    UA_NetworkMessage message;
    memset(&message, 0, sizeof(message));
    message.version = 1;
    message.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    message.payloadHeaderEnabled = true;
    message.messageCount = 1;
    message.dataSetWriterIds[0] = 1;
    message.payload.dataSetMessages = (UA_DataSetMessage*)
        UA_calloc(1, sizeof(UA_DataSetMessage));
    ck_assert_ptr_nonnull(message.payload.dataSetMessages);

    UA_DataSetMessage *dataSetMessage = message.payload.dataSetMessages;
    dataSetMessage->header.dataSetMessageValid = true;
    dataSetMessage->header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    dataSetMessage->header.dataSetMessageType =
        UA_DATASETMESSAGE_DATAKEYFRAME;
    dataSetMessage->fieldCount = 1;
    dataSetMessage->data.keyFrameFields = UA_DataValue_new();
    ck_assert_ptr_nonnull(dataSetMessage->data.keyFrameFields);

    UA_ReadValueId value;
    UA_ReadValueId_init(&value);
    ck_assert_uint_eq(
        UA_Variant_setScalarCopy(&dataSetMessage->data.keyFrameFields[0].value,
                                 &value, &UA_TYPES[UA_TYPES_READVALUEID]),
        UA_STATUSCODE_GOOD);
    dataSetMessage->data.keyFrameFields[0].hasValue = true;

    UA_EncodeJsonOptions jsonOptions = {0};
    jsonOptions.useCompactEncoding = true;
    UA_ByteString encoded = UA_BYTESTRING_NULL;
    UA_StatusCode result = UA_NetworkMessage_encodeJson(
        &message, &encoded, &encodingOptions, &jsonOptions);
    ck_assert_uint_eq(result, UA_STATUSCODE_GOOD);
    const char *expected =
        "{\"MessageId\":null,\"MessageType\":\"ua-data\",\"Messages\":[{"
        "\"DataSetWriterId\":1,\"MessageType\":\"ua-keyframe\","
        "\"Payload\":{\"Field\":{\"UaType\":22,\"Value\":{"
        "\"UaTypeId\":\"i=626\"}}}}]}";
    ck_assert_msg(encoded.length == strlen(expected) &&
                  memcmp(encoded.data, expected, encoded.length) == 0,
                  "Unexpected JSON: %.*s", (int)encoded.length,
                  (char*)encoded.data);
    ck_assert_uint_eq(UA_NetworkMessage_calcSizeJson(
                          &message, &encodingOptions, &jsonOptions),
                      encoded.length);

    UA_ByteString_clear(&encoded);
    UA_NetworkMessage_clear(&message);
} END_TEST

START_TEST(UA_PubSub_EncodeAllOptionalFields) {
    UA_UInt16 dsWriter1 = 12345;

    /* Set up the metadata for decoding */
    UA_NetworkMessage_EncodingOptions eo = {0};
    UA_DataSetMessage_EncodingMetaData emd[1] = {0};
    UA_FieldMetaData fmd[1] = {0};
    fmd[0].name = UA_STRING("Field1");

    eo.metaData = emd;
    eo.metaDataSize = 1;

    emd[0].dataSetWriterId = dsWriter1;
    emd[0].fields = fmd;
    emd[0].fieldsSize = 1;

    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.payloadHeaderEnabled = true;
    m.payload.dataSetMessages = (UA_DataSetMessage*)
        UA_calloc(1, sizeof(UA_DataSetMessage));
    m.messageCount = 1;
    m.dataSetWriterIds[0] = dsWriter1;

    /* enable messageId */
    m.messageIdEnabled = true;
    m.messageId = UA_STRING_ALLOC("ABCDEFGH");

    /* enable publisherId */
    m.publisherIdEnabled = true;
    m.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    m.publisherId.id.uint16 = 65535;

    /* enable dataSetClassId */
    m.dataSetClassIdEnabled = true;
    m.dataSetClassId.data1 = 1;
    m.dataSetClassId.data2 = 2;
    m.dataSetClassId.data3 = 3;

    UA_DataSetMessage *dsm = m.payload.dataSetMessages;

    /* DatasetMessage */
    dsm->header.dataSetMessageValid = true;
    dsm->header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    dsm->header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    UA_UInt16 fieldCountDS1 = 1;
    dsm->fieldCount = fieldCountDS1;
    dsm->data.keyFrameFields = UA_DataValue_new();

    /* enable DataSetMessageSequenceNr */
    dsm->header.dataSetMessageSequenceNrEnabled = true;
    dsm->header.dataSetMessageSequenceNr = 4711;

    /* enable metaDataVersion */
    dsm->header.configVersionMajorVersionEnabled = true;
    dsm->header.configVersionMinorVersionEnabled = true;
    dsm->header.configVersionMajorVersion = 42;
    dsm->header.configVersionMinorVersion = 7;

    /* enable timestamp */
    dsm->header.timestampEnabled = true;
    dsm->header.timestamp = 11111111111111;

    /* enable status */
    dsm->header.statusEnabled = true;
    dsm->header.status = 12345;

    UA_UInt32 iv = 27;
    UA_Variant_setScalarCopy(&dsm->data.keyFrameFields[0].value,
                             &iv, &UA_TYPES[UA_TYPES_UINT32]);
    dsm->data.keyFrameFields[0].hasValue = true;

    size_t size = UA_NetworkMessage_calcSizeJson(&m, &eo, NULL);

    UA_ByteString buffer = UA_BYTESTRING_NULL;
    UA_StatusCode rv = UA_ByteString_allocBuffer(&buffer, size+1);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    memset(buffer.data, 0, size+1);

    rv = UA_NetworkMessage_encodeJson(&m, &buffer, &eo, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    // then
    char* result = "{\"MessageId\":\"ABCDEFGH\",\"MessageType\":\"ua-data\",\"PublisherId\":\"65535\",\"DataSetClassId\":\"00000001-0002-0003-0000-000000000000\",\"Messages\":[{\"DataSetWriterId\":12345,\"SequenceNumber\":4711,\"MetaDataVersion\":{\"MajorVersion\":42,\"MinorVersion\":7},\"Timestamp\":\"1601-01-13T20:38:31.1111111Z\",\"Status\":{\"Code\":12345,\"Symbol\":\"Good\"},\"MessageType\":\"ua-keyframe\",\"Payload\":{\"Field1\":{\"UaType\":7,\"Value\":27}}}]}";
    ck_assert_str_eq(result, (char*)buffer.data);

    UA_ByteString_clear(&buffer);
    UA_NetworkMessage_clear(&m);
}
END_TEST

START_TEST(UA_PubSub_EnDecode) {
    UA_UInt16 dsWriter1 = 4;
    UA_UInt16 dsWriter2 = 7;

    /* Set up the metadata for decoding */
    UA_NetworkMessage_EncodingOptions eo = {0};
    UA_DataSetMessage_EncodingMetaData emd[2]= {0};
    UA_FieldMetaData fmd0[1] = {0};
    UA_FieldMetaData fmd1[2] = {0};

    fmd0[0].name = UA_STRING("Field1");
    emd[0].fields = fmd0;
    emd[0].fieldsSize = 1;
    emd[0].dataSetWriterId = dsWriter1;

    fmd1[0].name = UA_STRING("Field2.1");
    fmd1[1].name = UA_STRING("Field2.2");
    emd[1].fields = fmd1;
    emd[1].fieldsSize = 2;
    emd[1].dataSetWriterId = dsWriter2;

    eo.metaData = emd;
    eo.metaDataSize = 2;

    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.payloadHeaderEnabled = true;
    m.payload.dataSetMessages = (UA_DataSetMessage*)
        UA_calloc(2, sizeof(UA_DataSetMessage));
    m.messageCount = 2;

    UA_DataSetMessage *dsm0 = &m.payload.dataSetMessages[0];
    UA_DataSetMessage *dsm1 = &m.payload.dataSetMessages[1];

    m.dataSetWriterIds[0] = dsWriter1;
    m.dataSetWriterIds[1] = dsWriter2;

    dsm0->header.dataSetMessageValid = true;
    dsm0->header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    dsm0->header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    UA_UInt16 fieldCountDS1 = 1;
    dsm0->fieldCount = fieldCountDS1;
    dsm0->data.keyFrameFields = UA_DataValue_new();

    UA_UInt32 iv = 27;
    UA_Variant_setScalarCopy(&dsm0->data.keyFrameFields[0].value, &iv, &UA_TYPES[UA_TYPES_UINT32]);
    dsm0->data.keyFrameFields[0].hasValue = true;

    dsm1->header.dataSetMessageValid = true;
    dsm1->header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    dsm1->header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    UA_UInt16 fieldCountDS2 = 2;
    dsm1->fieldCount = fieldCountDS2;
    dsm1->data.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)
        UA_calloc(dsm1->fieldCount, sizeof(UA_DataSetMessage_DeltaFrameField));

    UA_Guid gv = UA_Guid_random();
    UA_DataValue_init(&dsm1->data.keyFrameFields[0]);
    UA_Variant_setScalarCopy(&dsm1->data.keyFrameFields[0].value, &gv, &UA_TYPES[UA_TYPES_GUID]);
    dsm1->data.keyFrameFields[0].hasValue = true;

    UA_DataValue_init(&dsm1->data.keyFrameFields[1]);
    UA_Int64 iv64 = 152478978534;
    UA_Variant_setScalarCopy(&dsm1->data.keyFrameFields[1].value, &iv64, &UA_TYPES[UA_TYPES_INT64]);
    dsm1->data.keyFrameFields[1].hasValue = true;

    size_t size = UA_NetworkMessage_calcSizeJson(&m, &eo, NULL);

    UA_ByteString buffer;
    UA_StatusCode rv = UA_ByteString_allocBuffer(&buffer, size+1);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    memset(buffer.data, 0, size+1);

    rv = UA_NetworkMessage_encodeJson(&m, &buffer, &eo, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    // then

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeJson(&buffer, &m2, &eo, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert_uint_eq(m2.dataSetWriterIds[0], dsWriter1);
    ck_assert_uint_eq(m2.dataSetWriterIds[1], dsWriter2);
    ck_assert(m.payload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert_int_eq(m2.payload.dataSetMessages[0].header.fieldEncoding,
                     UA_FIELDENCODING_DATAVALUE);
    ck_assert_int_eq(m2.payload.dataSetMessages[0].fieldCount, fieldCountDS1);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue);
    ck_assert_uint_eq((uintptr_t)m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(*(UA_UInt32 *)m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.data, iv);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp);

    ck_assert(m.payload.dataSetMessages[1].header.dataSetMessageValid == m2.payload.dataSetMessages[1].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetMessages[1].data.deltaFrameFields[0].value.hasSourceTimestamp == m2.payload.dataSetMessages[1].data.deltaFrameFields[0].value.hasSourceTimestamp);
    UA_ByteString_clear(&buffer);
    UA_NetworkMessage_clear(&m);
    UA_NetworkMessage_clear(&m2);
}
END_TEST


START_TEST(UA_NetworkMessage_oneMessage_twoFields_json_decode) {
    /* Set up the metadata for decoding */
    UA_NetworkMessage_EncodingOptions eo = {0};
    UA_DataSetMessage_EncodingMetaData emd[1]= {0};
    UA_FieldMetaData fmd[2] = {0};

    fmd[0].name = UA_STRING("Test");
    fmd[1].name = UA_STRING("Server localtime");
    emd[0].fields = fmd;
    emd[0].fieldsSize = 2;
    emd[0].dataSetWriterId = 62541;

    eo.metaData = emd;
    eo.metaDataSize = 2;

    UA_NetworkMessage out;
    UA_ByteString buf = UA_STRING("{\"MessageId\":\"5ED82C10-50BB-CD07-0120-22521081E8EE\",\"MessageType\":\"ua-data\",\"Messages\":[{\"DataSetWriterId\":62541,\"MetaDataVersion\":{\"MajorVersion\":1478393530,\"MinorVersion\":12345},\"SequenceNumber\":4711,\"Payload\":{\"Test\":{\"UaType\":5,\"Value\":42},\"Server localtime\":{\"UaType\":13,\"Value\":\"2018-06-05T05:58:36.000Z\"}}}]}");
    UA_StatusCode retval = UA_NetworkMessage_decodeJson(&buf, &out, &eo, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

     //NetworkMessage
    ck_assert_int_eq(out.chunkMessage, false);
    ck_assert_int_eq(out.dataSetClassIdEnabled, false);
    ck_assert_int_eq(out.groupHeaderEnabled, false);
    ck_assert_int_eq(out.networkMessageType, UA_NETWORKMESSAGE_DATASET);
    ck_assert_int_eq(out.picosecondsEnabled, false);
    ck_assert_int_eq(out.promotedFieldsEnabled, false);
    ck_assert_int_eq(out.securityEnabled, false);
    ck_assert_int_eq(out.timestampEnabled, false);
    ck_assert_int_eq(out.publisherIdEnabled, false);

    ck_assert_int_eq(out.payloadHeaderEnabled, true);
    ck_assert_int_eq(out.messageCount, 1);
    ck_assert_int_eq(out.dataSetWriterIds[0], 62541);

    //dataSetMessage
    ck_assert_int_eq(out.payload.dataSetMessages[0].header.dataSetMessageSequenceNrEnabled, true);
    ck_assert_int_eq(out.payload.dataSetMessages[0].header.dataSetMessageSequenceNr, 4711);

    ck_assert_int_eq(out.payload.dataSetMessages[0].header.dataSetMessageType, UA_DATASETMESSAGE_DATAKEYFRAME);
    ck_assert_int_eq(out.payload.dataSetMessages[0].header.fieldEncoding,
                     UA_FIELDENCODING_DATAVALUE);

    ck_assert_int_eq(out.payload.dataSetMessages[0].header.picoSecondsIncluded, false);

    ck_assert_int_eq(out.payload.dataSetMessages[0].header.configVersionMinorVersionEnabled, true);
    ck_assert_int_eq(out.payload.dataSetMessages[0].header.configVersionMajorVersionEnabled, true);
    ck_assert_int_eq(out.payload.dataSetMessages[0].header.configVersionMinorVersion, 12345);
    ck_assert_int_eq(out.payload.dataSetMessages[0].header.configVersionMajorVersion, 1478393530);

    ck_assert_int_eq(out.payload.dataSetMessages[0].header.dataSetMessageSequenceNr, 4711);
    //ck_assert_int_eq(out.payload.dataSetMessages[0].header.dataSetWriterId, 62541);
    ck_assert_int_eq(out.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue, 1);
    ck_assert_int_eq(*((UA_UInt16*)out.payload.dataSetMessages[0].data.keyFrameFields[0].value.data), 42);
    ck_assert_int_eq(out.payload.dataSetMessages[0].data.keyFrameFields[1].hasValue, 1);
    UA_DateTime *dt = (UA_DateTime*)out.payload.dataSetMessages[0].data.keyFrameFields[1].value.data;
    UA_DateTimeStruct dts = UA_DateTime_toStruct(*dt);
    ck_assert_int_eq(dts.year, 2018);
    ck_assert_int_eq(dts.month, 6);
    ck_assert_int_eq(dts.day, 5);
    ck_assert_int_eq(dts.hour, 5);
    ck_assert_int_eq(dts.min, 58);
    ck_assert_int_eq(dts.sec, 36);
    ck_assert_int_eq(dts.milliSec, 0);
    ck_assert_int_eq(dts.microSec, 0);
    ck_assert_int_eq(dts.nanoSec, 0);

    UA_NetworkMessage_clear(&out);
}
END_TEST


// Decode without metadata
// The values still come out in order
START_TEST(UA_NetworkMessage_json_decode) {
    // given
    UA_NetworkMessage out;
    memset(&out,0,sizeof(UA_NetworkMessage));
    UA_ByteString buf = UA_STRING("{\"MessageId\":\"5ED82C10-50BB-CD07-0120-22521081E8EE\",\"MessageType\":\"ua-data\",\"WriterGroupName\":\"wg\",\"Messages\":[{\"MetaDataVersion\":{\"MajorVersion\": 47, \"MinorVersion\": 47},\"DataSetWriterId\":62541,\"DataSetWriterName\":\"dsw\",\"MinorVersion\":2,\"Status\":22,\"SequenceNumber\":4711,\"Payload\":{\"Test\":{\"UaType\":5,\"Value\":42},\"Server localtime\":{\"UaType\":1,\"Value\":true}}}]}");
    // when
    UA_StatusCode retval = UA_NetworkMessage_decodeJson(&buf, &out, NULL, NULL);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    //NetworkMessage
    ck_assert_int_eq(out.chunkMessage, false);
    ck_assert_int_eq(out.dataSetClassIdEnabled, false);
    ck_assert_int_eq(out.groupHeaderEnabled, false);
    ck_assert_int_eq(out.networkMessageType, UA_NETWORKMESSAGE_DATASET);
    ck_assert_int_eq(out.picosecondsEnabled, false);
    ck_assert_int_eq(out.promotedFieldsEnabled, false);
    ck_assert_int_eq(out.securityEnabled, false);
    ck_assert_int_eq(out.timestampEnabled, false);
    ck_assert_int_eq(out.publisherIdEnabled, false);

    ck_assert_int_eq(out.payloadHeaderEnabled, true);
    ck_assert_int_eq(out.messageCount, 1);
    ck_assert_int_eq(out.dataSetWriterIds[0], 62541);

    //dataSetMessage
    ck_assert_int_eq(out.payload.dataSetMessages[0].header.dataSetMessageSequenceNrEnabled, true);
    ck_assert_int_eq(out.payload.dataSetMessages[0].header.dataSetMessageSequenceNr, 4711);

    ck_assert_int_eq(out.payload.dataSetMessages[0].header.dataSetMessageType, UA_DATASETMESSAGE_DATAKEYFRAME);
    ck_assert_int_eq(out.payload.dataSetMessages[0].header.fieldEncoding,
                     UA_FIELDENCODING_DATAVALUE);

    ck_assert_int_eq(out.payload.dataSetMessages[0].header.picoSecondsIncluded, false);

    ck_assert_int_eq(out.payload.dataSetMessages[0].header.statusEnabled, true);
    ck_assert_int_eq(out.payload.dataSetMessages[0].header.status, 22);

    ck_assert_int_eq(out.payload.dataSetMessages[0].header.configVersionMinorVersionEnabled, true);
    ck_assert_int_eq(out.payload.dataSetMessages[0].header.configVersionMajorVersionEnabled, true);
    ck_assert_int_eq(out.payload.dataSetMessages[0].header.configVersionMinorVersion, 47);
    ck_assert_int_eq(out.payload.dataSetMessages[0].header.configVersionMajorVersion, 47);

    //dataSetFields
    ck_assert_int_eq(out.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue, true);
    ck_assert_int_eq(*((UA_UInt16*)out.payload.dataSetMessages[0].data.keyFrameFields[0].value.data), 42);
    ck_assert_int_eq(out.payload.dataSetMessages[0].data.keyFrameFields[1].hasValue, true);
    ck_assert_int_eq(*((UA_Boolean*)out.payload.dataSetMessages[0].data.keyFrameFields[1].value.data), 1);

    UA_NetworkMessage_clear(&out);
}
END_TEST

/* Messages are a single object and not an array */
START_TEST(UA_NetworkMessage_json_decode_messageObject) {
    // given
    UA_NetworkMessage out;
    memset(&out,0,sizeof(UA_NetworkMessage));
    UA_ByteString buf = UA_STRING("{\"MessageId\":\"5ED82C10-50BB-CD07-0120-22521081E8EE\",\"MessageType\":\"ua-data\",\"Messages\":{\"MetaDataVersion\":{\"MajorVersion\": 47, \"MinorVersion\": 47},\"DataSetWriterId\":62541,\"Status\":22,\"SequenceNumber\":4711,\"Payload\":{\"Test\":{\"UaType\":5,\"Value\":42},\"Server localtime\":{\"UaType\":1,\"Value\":true}}}}");
    // when
    UA_StatusCode retval = UA_NetworkMessage_decodeJson(&buf, &out, NULL, NULL);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_NetworkMessage_clear(&out);
}
END_TEST

START_TEST(UA_Networkmessage_DataSetFieldsNull_json_decode) {
    // given
    UA_NetworkMessage out;
    memset(&out, 0, sizeof(UA_NetworkMessage));
    UA_ByteString buf = UA_STRING("{ \"MessageId\": \"32235546-05d9-4fd7-97df-ea3ff3408574\",  "
            "\"MessageType\": \"ua-data\",  \"PublisherId\": \"MQTT-Localhost\",  "
            "\"DataSetClassId\": \"00000005-cab9-4470-8f8a-2c1ead207e0e\",  \"Messages\": "
            "[    {      \"DataSetWriterId\": 1,      \"SequenceNumber\": 224,     \"MetaDataVersion\": "
            "{        \"MajorVersion\": 1,        \"MinorVersion\": 1      },\"Payload\":null}]}");
    // when
    UA_StatusCode retval = UA_NetworkMessage_decodeJson(&buf, &out, NULL, NULL);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.dataSetClassId.data1, 5);
    ck_assert_int_eq(out.payload.dataSetMessages->header.dataSetMessageSequenceNr, 224);
    ck_assert_ptr_eq(out.payload.dataSetMessages->data.keyFrameFields, NULL);

    UA_NetworkMessage_clear(&out);
}
END_TEST

START_TEST(UA_Networkmessage_metaData_oob) {
    /* Typical metadata: 8 named fields at indices 0..7 */
#define METADATA_FIELDS 8
    static char *names[METADATA_FIELDS] = {
        "Temperature", "Pressure", "Humidity", "Vibration",
        "Voltage", "Current", "Status", "Energy"
    };

    UA_FieldMetaData fmd[METADATA_FIELDS];
    memset(fmd, 0, sizeof(UA_FieldMetaData)*METADATA_FIELDS);
    for(size_t i = 0; i < METADATA_FIELDS; i++) {
        fmd[i].name = UA_STRING(names[i]);
    }

    /* Initialize EncodingMetaData */
    UA_DataSetMessage_EncodingMetaData emd;
    memset(&emd, 0, sizeof(emd));
    emd.dataSetWriterId = 1;
    emd.fields = fmd;
    emd.fieldsSize = METADATA_FIELDS;

    /* Setup encoding options */
    UA_NetworkMessage_EncodingOptions eo;
    memset(&eo, 0, sizeof(eo));
    eo.metaData = &emd;
    eo.metaDataSize = 1;

    /* An incomplete key frame must not index past its payload allocation. */
    static char *json =
        "{"
          "\"MessageType\":\"ua-data\","
          "\"Messages\":[{"
            "\"DataSetWriterId\":1,"
            "\"MessageType\":\"ua-keyframe\","
            "\"Payload\":{\"Energy\":null}"
          "}]"
        "}";

    /* Decode JSON with error checking */
    UA_NetworkMessage nm;
    UA_ByteString bs = UA_STRING(json);
    UA_StatusCode rv = UA_NetworkMessage_decodeJson(&bs, &nm, &eo, NULL);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage_clear(&nm);
}
END_TEST


START_TEST(UA_Networkmessage_malformed_array) {
    UA_NetworkMessage nm;
    UA_ByteString bs = UA_STRING("[]");
    UA_StatusCode rv = UA_NetworkMessage_decodeJson(&bs, &nm, NULL, NULL);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);
    UA_NetworkMessage_clear(&nm);
}
END_TEST

START_TEST(UA_Networkmessage_json_uint32_and_datavalue_roundtrip) {
    static const UA_UInt32 sequenceNumbers[] = {
        65535u, 65536u, 70000u, UA_UINT32_MAX
    };

    UA_FieldMetaData field = {0};
    field.name = UA_STRING("Field");
    UA_DataSetMessage_EncodingMetaData emd = {0};
    emd.dataSetWriterId = 1;
    emd.fields = &field;
    emd.fieldsSize = 1;
    UA_NetworkMessage_EncodingOptions eo = {0};
    eo.metaData = &emd;
    eo.metaDataSize = 1;

    for(size_t i = 0; i < sizeof(sequenceNumbers) / sizeof(sequenceNumbers[0]); i++) {
        char json[1024];
        int len = snprintf(json, sizeof(json),
            "{\"MessageType\":\"ua-data\",\"Messages\":[{"
            "\"DataSetWriterId\":1,\"SequenceNumber\":%u,"
            "\"Status\":2147483648,\"Payload\":{\"Field\":{"
            "\"UaType\":7,\"Value\":27,"
            "\"Status\":{\"Code\":1073741824},"
            "\"SourceTimestamp\":\"1970-01-15T06:56:07Z\","
            "\"ServerTimestamp\":\"1970-01-15T06:56:08Z\"}}}]}" ,
            sequenceNumbers[i]);
        ck_assert_int_gt(len, 0);
        ck_assert_int_lt(len, (int)sizeof(json));

        UA_ByteString input = {(size_t)len, (UA_Byte*)json};
        UA_NetworkMessage decoded;
        UA_StatusCode res = UA_NetworkMessage_decodeJson(&input, &decoded, &eo, NULL);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

        UA_DataSetMessage *dsm = &decoded.payload.dataSetMessages[0];
        ck_assert_uint_eq(dsm->header.dataSetMessageSequenceNr,
                          sequenceNumbers[i]);
        ck_assert_uint_eq(dsm->header.status, 0x80000000u);
        ck_assert_int_eq(dsm->header.fieldEncoding, UA_FIELDENCODING_DATAVALUE);
        UA_DataValue *dv = &dsm->data.keyFrameFields[0];
        ck_assert(dv->hasStatus);
        ck_assert_uint_eq(dv->status, 0x40000000u);
        ck_assert(dv->hasSourceTimestamp);
        ck_assert(dv->hasServerTimestamp);

        UA_ByteString encoded = UA_BYTESTRING_NULL;
        res = UA_NetworkMessage_encodeJson(&decoded, &encoded, &eo, NULL);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

        UA_NetworkMessage roundtrip;
        res = UA_NetworkMessage_decodeJson(&encoded, &roundtrip, &eo, NULL);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        UA_DataSetMessage *roundtripDsm = &roundtrip.payload.dataSetMessages[0];
        ck_assert_uint_eq(roundtripDsm->header.dataSetMessageSequenceNr,
                          sequenceNumbers[i]);
        ck_assert_uint_eq(roundtripDsm->header.status, 0x80000000u);
        UA_DataValue *roundtripDv = &roundtripDsm->data.keyFrameFields[0];
        ck_assert_uint_eq(roundtripDv->status, 0x40000000u);
        ck_assert_int_eq(roundtripDv->sourceTimestamp, dv->sourceTimestamp);
        ck_assert_int_eq(roundtripDv->serverTimestamp, dv->serverTimestamp);

        UA_NetworkMessage_clear(&roundtrip);
        UA_ByteString_clear(&encoded);
        UA_NetworkMessage_clear(&decoded);
    }
}
END_TEST

START_TEST(UA_Networkmessage_json_publisherid_roundtrip) {
    UA_DataSetMessage_EncodingMetaData emd = {0};
    emd.dataSetWriterId = 1;
    UA_NetworkMessage_EncodingOptions eo = {0};
    eo.metaData = &emd;
    eo.metaDataSize = 1;

    UA_DataSetMessage dsm = {0};
    dsm.header.dataSetMessageValid = true;
    dsm.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dsm.header.fieldEncoding = UA_FIELDENCODING_VARIANT;

    UA_NetworkMessage message = {0};
    message.version = 1;
    message.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    message.publisherIdEnabled = true;
    message.payloadHeaderEnabled = true;
    message.messageCount = 1;
    message.dataSetWriterIds[0] = 1;
    message.payload.dataSetMessages = &dsm;

    static const UA_PublisherId numericIds[] = {
        {UA_PUBLISHERIDTYPE_BYTE, {.byte = 17}},
        {UA_PUBLISHERIDTYPE_UINT16, {.uint16 = 65000}},
        {UA_PUBLISHERIDTYPE_UINT32, {.uint32 = 4000000000u}},
        {UA_PUBLISHERIDTYPE_UINT64, {.uint64 = UA_UINT64_MAX}}
    };
    static const UA_UInt64 expected[] = {
        17u, 65000u, 4000000000u, UA_UINT64_MAX
    };

    for(size_t i = 0; i < sizeof(numericIds) / sizeof(numericIds[0]); i++) {
        message.publisherId = numericIds[i];
        UA_ByteString encoded = UA_BYTESTRING_NULL;
        UA_StatusCode res =
            UA_NetworkMessage_encodeJson(&message, &encoded, &eo, NULL);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        UA_NetworkMessage decoded;
        res = UA_NetworkMessage_decodeJson(&encoded, &decoded, &eo, NULL);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_msg(decoded.publisherId.idType == UA_PUBLISHERIDTYPE_UINT64,
                      "numeric PublisherId case %zu decoded as type %u from %.*s",
                      i, (unsigned)decoded.publisherId.idType,
                      (int)encoded.length, (char*)encoded.data);
        ck_assert_uint_eq(decoded.publisherId.id.uint64, expected[i]);
        UA_NetworkMessage_clear(&decoded);
        UA_ByteString_clear(&encoded);
    }

    message.publisherId.idType = UA_PUBLISHERIDTYPE_STRING;
    message.publisherId.id.string = UA_STRING("publisher-alpha");
    UA_ByteString encoded = UA_BYTESTRING_NULL;
    UA_StatusCode res =
        UA_NetworkMessage_encodeJson(&message, &encoded, &eo, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NetworkMessage decoded;
    res = UA_NetworkMessage_decodeJson(&encoded, &decoded, &eo, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(decoded.publisherId.idType, UA_PUBLISHERIDTYPE_STRING);
    ck_assert(UA_String_equal(&decoded.publisherId.id.string,
                              &message.publisherId.id.string));
    UA_NetworkMessage_clear(&decoded);
    UA_ByteString_clear(&encoded);
}
END_TEST

START_TEST(UA_Networkmessage_rawFieldsWithMetadata) {
    UA_FieldMetaData fields[6] = {0};
    const char *names[] = {"Counter", "Random", "ActiveState", "Severity", "Time", "Samples"};
    UA_Byte types[] = {UA_NS0ID_INT32, UA_NS0ID_DOUBLE, UA_NS0ID_LOCALIZEDTEXT,
                       UA_NS0ID_UINT16, UA_NS0ID_DATETIME, UA_NS0ID_INT32};
    for(size_t i = 0; i < 6; i++) {
        fields[i].name = UA_STRING((char*)(uintptr_t)names[i]);
        fields[i].builtInType = types[i];
        fields[i].dataType = UA_NODEID_NUMERIC(0, types[i]);
        fields[i].valueRank = UA_VALUERANK_SCALAR;
    }
    fields[4].dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_UTCTIME);
    fields[5].valueRank = 1;
    UA_DataSetMessage_EncodingMetaData metadata = {0};
    metadata.dataSetWriterId = 1;
    metadata.fields = fields;
    metadata.fieldsSize = 6;
    UA_NetworkMessage_EncodingOptions options = {0};
    options.metaData = &metadata;
    options.metaDataSize = 1;
    UA_ByteString json = UA_STRING(
        "{\"MessageType\":\"ua-data\",\"PublisherId\":\"urn:simulation\",\"Messages\":[{"
        "\"Payload\":{\"Random\":0.5,\"Counter\":10,\"ActiveState\":\"Active\","
        "\"Severity\":500,\"Time\":\"2026-10-10T17:12:50.831Z\",\"Samples\":[1,2,3]},"
        "\"DataSetWriterId\":1}]}");
    UA_NetworkMessage nm;
    ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&json, &nm, &options, NULL), UA_STATUSCODE_GOOD);
    UA_DataValue *values = nm.payload.dataSetMessages[0].data.keyFrameFields;
    ck_assert(UA_Variant_hasScalarType(&values[0].value, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert_int_eq(*(UA_Int32*)values[0].value.data, 10);
    ck_assert_double_eq(*(UA_Double*)values[1].value.data, 0.5);
    UA_String active = UA_STRING("Active");
    ck_assert(UA_String_equal(&((UA_LocalizedText*)values[2].value.data)->text, &active));
    ck_assert_uint_eq(*(UA_UInt16*)values[3].value.data, 500);
    ck_assert_uint_eq(values[4].value.type->typeKind, UA_DATATYPEKIND_DATETIME);
    ck_assert_uint_eq(values[5].value.arrayLength, 3);
    ck_assert_int_eq(((UA_Int32*)values[5].value.data)[2], 3);
    UA_NetworkMessage_clear(&nm);
    ck_assert_uint_ne(UA_NetworkMessage_decodeJson(&json, &nm, NULL, NULL), UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(UA_Networkmessage_rawInvalidFields) {
    UA_FieldMetaData fields[2] = {0};
    fields[0].name = UA_STRING("A");
    fields[1].name = UA_STRING("B");
    for(size_t i = 0; i < 2; i++) {
        fields[i].dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_UINT16);
        fields[i].builtInType = UA_NS0ID_UINT16;
        fields[i].valueRank = UA_VALUERANK_SCALAR;
    }
    UA_DataSetMessage_EncodingMetaData metadata = {0};
    metadata.dataSetWriterId = 1;
    metadata.fieldsSize = 2;
    metadata.fields = fields;
    UA_NetworkMessage_EncodingOptions options = {0};
    options.metaDataSize = 1;
    options.metaData = &metadata;
    const char *payloads[] = {
        "\"A\":1,\"Unknown\":2", "\"A\":1,\"A\":2", "\"A\":null,\"A\":2",
        "\"A\":-1,\"B\":2", "\"A\":[1],\"B\":2", "\"A\":65536,\"B\":2"
    };
    for(size_t i = 0; i < sizeof(payloads) / sizeof(payloads[0]); i++) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "{\"MessageType\":\"ua-data\",\"Messages\":[{"
                 "\"DataSetWriterId\":1,\"Payload\":{%s}}]}", payloads[i]);
        UA_ByteString json = UA_STRING(buffer);
        UA_NetworkMessage nm;
        ck_assert_uint_ne(UA_NetworkMessage_decodeJson(&json, &nm, &options, NULL), UA_STATUSCODE_GOOD);
    }
    UA_ByteString withStatus = UA_STRING("{\"MessageType\":\"ua-data\",\"Messages\":[{"
        "\"DataSetWriterId\":1,\"Payload\":{\"A\":{\"UaType\":5,\"Value\":42,\"Status\":{\"Code\":2147483648}},\"B\":2}}]}");
    UA_NetworkMessage nm;
    ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&withStatus, &nm, &options, NULL), UA_STATUSCODE_GOOD);
    ck_assert(nm.payload.dataSetMessages[0].data.keyFrameFields[0].hasStatus);
    ck_assert(nm.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue);
    UA_NetworkMessage_clear(&nm);
} END_TEST

START_TEST(UA_Networkmessage_deltaDuplicateFields) {
    const size_t fieldCounts[] = {9, 64, 65, UA_UINT16_MAX};
    for(size_t i = 0; i < sizeof(fieldCounts) / sizeof(fieldCounts[0]); i++) {
        size_t fieldCount = fieldCounts[i];
        UA_FieldMetaData *fields = (UA_FieldMetaData*)
            UA_calloc(fieldCount, sizeof(UA_FieldMetaData));
        ck_assert_ptr_nonnull(fields);
        fields[0].name = UA_STRING("First");
        fields[fieldCount - 1].name = UA_STRING("Last");
        fields[0].dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_INT32);
        fields[fieldCount - 1].dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_INT32);
        fields[0].builtInType = UA_NS0ID_INT32;
        fields[fieldCount - 1].builtInType = UA_NS0ID_INT32;
        fields[0].valueRank = UA_VALUERANK_SCALAR;
        fields[fieldCount - 1].valueRank = UA_VALUERANK_SCALAR;

        UA_DataSetMessage_EncodingMetaData metadata = {0};
        metadata.dataSetWriterId = 1;
        metadata.fieldsSize = fieldCount;
        metadata.fields = fields;
        UA_NetworkMessage_EncodingOptions options = {0};
        options.metaDataSize = 1;
        options.metaData = &metadata;

        UA_ByteString valid = UA_STRING("{\"MessageType\":\"ua-data\",\"Messages\":[{"
            "\"DataSetWriterId\":1,\"MessageType\":\"ua-deltaframe\","
            "\"Payload\":{\"Last\":8,\"First\":1}}]}");
        UA_NetworkMessage nm;
        ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&valid, &nm, &options, NULL),
                          UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(nm.payload.dataSetMessages[0].fieldCount, 2);
        ck_assert_uint_eq(nm.payload.dataSetMessages[0].data.deltaFrameFields[0].index,
                          fieldCount - 1);
        ck_assert_uint_eq(nm.payload.dataSetMessages[0].data.deltaFrameFields[1].index, 0);
        UA_NetworkMessage_clear(&nm);

        UA_ByteString duplicate = UA_STRING("{\"MessageType\":\"ua-data\",\"Messages\":[{"
            "\"DataSetWriterId\":1,\"MessageType\":\"ua-deltaframe\","
            "\"Payload\":{\"Last\":8,\"First\":1,\"Last\":9}}]}");
        ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&duplicate, &nm, &options, NULL),
                          UA_STATUSCODE_BADDECODINGERROR);
        UA_free(fields);
    }

    UA_DataSetMessage_EncodingMetaData oversized = {0};
    oversized.dataSetWriterId = 1;
    oversized.fieldsSize = (size_t)UA_UINT16_MAX + 1;
    UA_NetworkMessage_EncodingOptions options = {0};
    options.metaDataSize = 1;
    options.metaData = &oversized;
    UA_ByteString json = UA_STRING("{\"MessageType\":\"ua-data\",\"Messages\":[{"
        "\"DataSetWriterId\":1,\"MessageType\":\"ua-deltaframe\","
        "\"Payload\":{\"First\":1}}]}");
    UA_NetworkMessage nm;
    ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&json, &nm, &options, NULL),
                      UA_STATUSCODE_BADDECODINGERROR);
} END_TEST

START_TEST(UA_Networkmessage_dataValueFieldsWithMetadata) {
    UA_FieldMetaData field = {0};
    field.name = UA_STRING("A");
    field.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_INT32);
    field.builtInType = UA_NS0ID_INT32;
    field.valueRank = UA_VALUERANK_SCALAR;
    UA_DataSetMessage_EncodingMetaData metadata = {0};
    metadata.dataSetWriterId = 1;
    metadata.fieldsSize = 1;
    metadata.fields = &field;
    UA_NetworkMessage_EncodingOptions options = {0};
    options.metaDataSize = 1;
    options.metaData = &metadata;
    struct {
        const char *payload;
        UA_Boolean hasValue;
        UA_Boolean hasStatus;
        UA_Boolean hasSourceTimestamp;
    } cases[] = {
        {"{}", false, false, false},
        {"{\"Value\":42,\"Status\":{\"Code\":2147483648},"
         "\"SourceTimestamp\":\"2026-10-10T17:12:50.831Z\"}", true, true, true},
        {"{\"Status\":2147483648}", false, true, false},
        {"{\"Value\":null}", false, false, false},
        {"{\"UaType\":6,\"Value\":42}", true, false, false}
    };
    for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char buffer[512];
        snprintf(buffer, sizeof(buffer), "{\"MessageType\":\"ua-data\",\"Messages\":[{"
                 "\"DataSetWriterId\":1,\"Payload\":{\"A\":%s}}]}", cases[i].payload);
        UA_ByteString json = UA_STRING(buffer);
        UA_NetworkMessage nm;
        ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&json, &nm, &options, NULL),
                          UA_STATUSCODE_GOOD);
        UA_DataValue *value = &nm.payload.dataSetMessages[0].data.keyFrameFields[0];
        ck_assert_msg(value->hasValue == cases[i].hasValue,
                      "payload %zu hasValue=%u expected=%u", i,
                      (unsigned)value->hasValue, (unsigned)cases[i].hasValue);
        ck_assert_int_eq(value->hasStatus, cases[i].hasStatus);
        ck_assert_int_eq(value->hasSourceTimestamp, cases[i].hasSourceTimestamp);
        if(value->hasValue) {
            ck_assert(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_INT32]));
            ck_assert_int_eq(*(UA_Int32*)value->value.data, 42);
        }
        if(value->hasStatus)
            ck_assert_uint_eq(value->status, 2147483648u);
        UA_NetworkMessage_clear(&nm);
    }

    UA_ByteString invalid = UA_STRING("{\"MessageType\":\"ua-data\",\"Messages\":[{"
        "\"DataSetWriterId\":1,\"Payload\":{\"A\":{\"Value\":\"bad\"}}}]}");
    UA_NetworkMessage nm;
    ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&invalid, &nm, &options, NULL),
                      UA_STATUSCODE_BADDECODINGERROR);
} END_TEST

START_TEST(UA_Networkmessage_partialJsonUpdates) {
    UA_FieldMetaData fields[3] = {0};
    const char *names[] = {"Counter", "Square", "Random"};
    for(size_t i = 0; i < 3; i++) {
        fields[i].name = UA_STRING((char*)(uintptr_t)names[i]);
        fields[i].dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_INT32);
        fields[i].builtInType = UA_NS0ID_INT32;
        fields[i].valueRank = UA_VALUERANK_SCALAR;
    }
    UA_DataSetMessage_EncodingMetaData metadata = {0};
    metadata.dataSetWriterId = 1;
    metadata.fieldsSize = 3;
    metadata.fields = fields;
    UA_NetworkMessage_EncodingOptions options = {0};
    options.metaDataSize = 1;
    options.metaData = &metadata;
    struct {
        const char *messageType;
        UA_Boolean valid;
    } cases[] = {
        {"", false},
        {"\"MessageType\":\"ua-deltaframe\",", true},
        {"\"MessageType\":\"ua-keyframe\",", false},
        {"\"MessageType\":42,", false},
        {"\"MessageType\":\"ua-deltaframe\",\"MessageType\":\"ua-deltaframe\",", false}
    };
    for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "{\"MessageType\":\"ua-data\",\"Messages\":[{"
                 "\"Payload\":{\"Random\":12,\"Counter\":42},%s\"DataSetWriterId\":1}]}",
                 cases[i].messageType);
        UA_ByteString json = UA_STRING(buffer);
        UA_NetworkMessage nm;
        UA_StatusCode ret = UA_NetworkMessage_decodeJson(&json, &nm, &options, NULL);
        if(!cases[i].valid) {
            ck_assert_uint_ne(ret, UA_STATUSCODE_GOOD);
            continue;
        }
        ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
        UA_DataSetMessage *message = &nm.payload.dataSetMessages[0];
        ck_assert_uint_eq(message->header.dataSetMessageType, UA_DATASETMESSAGE_DATADELTAFRAME);
        ck_assert_uint_eq(message->fieldCount, 2);
        ck_assert_uint_eq(message->data.deltaFrameFields[0].index, 2);
        ck_assert_uint_eq(message->data.deltaFrameFields[1].index, 0);
        ck_assert_int_eq(*(UA_Int32*)message->data.deltaFrameFields[1].value.value.data, 42);
        UA_NetworkMessage_clear(&nm);
    }
    UA_ByteString complete = UA_STRING("{\"MessageType\":\"ua-data\",\"Messages\":[{"
        "\"DataSetWriterId\":1,\"Payload\":{\"Counter\":1,\"Square\":2,\"Random\":3}}]}");
    UA_NetworkMessage nm;
    ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&complete, &nm, &options, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(nm.payload.dataSetMessages[0].header.dataSetMessageType,
                      UA_DATASETMESSAGE_DATAKEYFRAME);
    UA_NetworkMessage_clear(&nm);
} END_TEST

START_TEST(UA_Networkmessage_omittedWriterId) {
    UA_FieldMetaData fields[2] = {0};
    fields[0].name = UA_STRING("Counter");
    fields[1].name = UA_STRING("Square");
    for(size_t i = 0; i < 2; i++) {
        fields[i].dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_INT32);
        fields[i].builtInType = UA_NS0ID_INT32;
        fields[i].valueRank = UA_VALUERANK_SCALAR;
    }
    UA_DataSetMessage_EncodingMetaData metadata[2] = {0};
    metadata[0].dataSetWriterId = 42;
    metadata[0].fieldsSize = 2;
    metadata[0].fields = fields;
    metadata[1] = metadata[0];
    metadata[1].dataSetWriterId = 43;
    UA_NetworkMessage_EncodingOptions options = {0};
    options.metaData = metadata;
    options.metaDataSize = 1;

    UA_ByteString json = UA_STRING("{\"MessageType\":\"ua-data\",\"Messages\":[{"
        "\"Payload\":{\"Counter\":12,\"Square\":5}}]}");
    UA_NetworkMessage nm;
    ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&json, &nm, &options, NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(nm.dataSetWriterIds[0], 42);
    ck_assert_int_eq(*(UA_Int32*)nm.payload.dataSetMessages[0].data.keyFrameFields[0].value.data, 12);
    UA_NetworkMessage_clear(&nm);

    UA_ByteString partial = UA_STRING("{\"MessageType\":\"ua-data\",\"Messages\":[{"
        "\"MessageType\":\"ua-deltaframe\",\"Payload\":{\"Square\":7}}]}");
    ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&partial, &nm, &options, NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(nm.dataSetWriterIds[0], 42);
    ck_assert_uint_eq(nm.payload.dataSetMessages[0].header.dataSetMessageType, UA_DATASETMESSAGE_DATADELTAFRAME);
    ck_assert_uint_eq(nm.payload.dataSetMessages[0].data.deltaFrameFields[0].index, 1);
    ck_assert_int_eq(*(UA_Int32*)nm.payload.dataSetMessages[0].data.deltaFrameFields[0].value.value.data, 7);
    UA_NetworkMessage_clear(&nm);

    options.metaDataSize = 2;
    ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&json, &nm, &options, NULL), UA_STATUSCODE_BADDECODINGERROR);
    ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&json, &nm, NULL, NULL), UA_STATUSCODE_BADDECODINGERROR);
    options.metaDataSize = 1;
    UA_ByteString multiple = UA_STRING("{\"MessageType\":\"ua-data\",\"Messages\":[{"
        "\"Payload\":{\"Counter\":12,\"Square\":5}},{\"Payload\":{\"Counter\":13,\"Square\":6}}]}");
    ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&multiple, &nm, &options, NULL), UA_STATUSCODE_BADDECODINGERROR);
    UA_ByteString invalidId = UA_STRING("{\"MessageType\":\"ua-data\",\"Messages\":[{"
        "\"DataSetWriterId\":\"invalid\",\"Payload\":{\"Counter\":12,\"Square\":5}}]}");
    ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&invalidId, &nm, &options, NULL), UA_STATUSCODE_BADDECODINGERROR);
} END_TEST

START_TEST(UA_Networkmessage_rawArrays) {
    UA_FieldMetaData field = {0};
    field.name = UA_STRING("Samples");
    field.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    field.builtInType = UA_NS0ID_STRING;
    field.valueRank = UA_VALUERANK_ONE_DIMENSION;
    UA_DataSetMessage_EncodingMetaData metadata = {0};
    metadata.dataSetWriterId = 1;
    metadata.fields = &field;
    metadata.fieldsSize = 1;
    UA_NetworkMessage_EncodingOptions options = {0};
    options.metaData = &metadata;
    options.metaDataSize = 1;
    const char *values[] = {"[]", "[\"a\",\"b\"]", "\"a\"", "[\"a\",42]", "[[\"a\"]]"};
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "{\"MessageType\":\"ua-data\",\"Messages\":{"
             "\"DataSetWriterId\":1,\"Payload\":{\"Samples\":%s}}}", values[_i]);
    UA_ByteString json = UA_STRING(buffer);
    UA_NetworkMessage nm;
    UA_StatusCode ret = UA_NetworkMessage_decodeJson(&json, &nm, &options, NULL);
    if(_i >= 2) {
        ck_assert_uint_eq(ret, UA_STATUSCODE_BADDECODINGERROR);
        return;
    }
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    UA_Variant *value = &nm.payload.dataSetMessages[0].data.keyFrameFields[0].value;
    ck_assert(UA_Variant_hasArrayType(value, &UA_TYPES[UA_TYPES_STRING]));
    ck_assert_uint_eq(value->arrayLength, _i == 0 ? 0 : 2);
    if(_i == 1) {
        UA_String expected = UA_STRING("b");
        ck_assert(UA_String_equal(&((UA_String*)value->data)[1], &expected));
    }
    UA_NetworkMessage_clear(&nm);
} END_TEST

START_TEST(UA_Networkmessage_verboseArrayWithMetadata) {
    UA_FieldMetaData field = {0};
    field.name = UA_STRING("Samples");
    field.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    field.builtInType = UA_NS0ID_INT32;
    field.valueRank = UA_VALUERANK_ONE_DIMENSION;
    UA_DataSetMessage_EncodingMetaData metadata = {0};
    metadata.dataSetWriterId = 1;
    metadata.fields = &field;
    metadata.fieldsSize = 1;
    UA_NetworkMessage_EncodingOptions options = {0};
    options.metaData = &metadata;
    options.metaDataSize = 1;

    UA_ByteString json = UA_STRING("{\"MessageType\":\"ua-data\",\"Messages\":[{"
        "\"DataSetWriterId\":1,\"Payload\":{\"Samples\":{\"Value\":[3,5],"
        "\"Status\":{\"Code\":2147483648}}}}]}");
    UA_NetworkMessage nm;
    ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&json, &nm, &options, NULL),
                      UA_STATUSCODE_GOOD);
    UA_DataValue *value = &nm.payload.dataSetMessages[0].data.keyFrameFields[0];
    ck_assert(value->hasValue);
    ck_assert(value->hasStatus);
    ck_assert_uint_eq(value->status, 2147483648u);
    ck_assert(UA_Variant_hasArrayType(&value->value, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert_uint_eq(value->value.arrayLength, 2);
    ck_assert_int_eq(((UA_Int32*)value->value.data)[1], 5);
    UA_NetworkMessage_clear(&nm);

    UA_ByteString invalid = UA_STRING("{\"MessageType\":\"ua-data\",\"Messages\":[{"
        "\"DataSetWriterId\":1,\"Payload\":{\"Samples\":{\"Value\":3}}}]}");
    ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&invalid, &nm, &options, NULL),
                      UA_STATUSCODE_BADDECODINGERROR);
} END_TEST

START_TEST(UA_Networkmessage_rawObjectBesideVerboseValue) {
    UA_FieldMetaData fields[2] = {0};
    fields[0].name = UA_STRING("Label");
    fields[0].dataType = UA_TYPES[UA_TYPES_LOCALIZEDTEXT].typeId;
    fields[0].builtInType = UA_NS0ID_LOCALIZEDTEXT;
    fields[0].valueRank = UA_VALUERANK_SCALAR;
    fields[1].name = UA_STRING("Counter");
    fields[1].dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    fields[1].builtInType = UA_NS0ID_INT32;
    fields[1].valueRank = UA_VALUERANK_SCALAR;
    UA_DataSetMessage_EncodingMetaData metadata = {0};
    metadata.dataSetWriterId = 1;
    metadata.fields = fields;
    metadata.fieldsSize = 2;
    UA_NetworkMessage_EncodingOptions options = {0};
    options.metaData = &metadata;
    options.metaDataSize = 1;

    UA_ByteString json = UA_STRING("{\"MessageType\":\"ua-data\",\"Messages\":[{"
        "\"DataSetWriterId\":1,\"Payload\":{\"Label\":{\"Locale\":\"en\",\"Text\":\"Ready\"},"
        "\"Counter\":{\"Status\":2147483648,\"Value\":42}}}]}");
    UA_NetworkMessage nm;
    ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&json, &nm, &options, NULL),
                      UA_STATUSCODE_GOOD);
    UA_DataValue *values = nm.payload.dataSetMessages[0].data.keyFrameFields;
    ck_assert(UA_Variant_hasScalarType(&values[0].value, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]));
    UA_String expected = UA_STRING("Ready");
    ck_assert(UA_String_equal(&((UA_LocalizedText*)values[0].value.data)->text, &expected));
    ck_assert(!values[0].hasStatus);
    ck_assert(values[1].hasStatus);
    ck_assert_uint_eq(values[1].status, 2147483648u);
    ck_assert_int_eq(*(UA_Int32*)values[1].value.data, 42);
    UA_NetworkMessage_clear(&nm);
} END_TEST

START_TEST(UA_Networkmessage_multipleWritersWithMetadata) {
    UA_FieldMetaData fields[2] = {0};
    UA_DataSetMessage_EncodingMetaData metadata[2] = {0};
    for(size_t i = 0; i < 2; i++) {
        fields[i].name = UA_STRING("Value");
        fields[i].valueRank = UA_VALUERANK_SCALAR;
        metadata[i].dataSetWriterId = (UA_UInt16)(i + 1);
        metadata[i].fields = &fields[i];
        metadata[i].fieldsSize = 1;
    }
    fields[0].dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    fields[1].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    UA_NetworkMessage_EncodingOptions options = {0};
    options.metaData = metadata;
    options.metaDataSize = 2;
    UA_ByteString json = UA_STRING("{\"MessageType\":\"ua-data\",\"Messages\":["
        "{\"Payload\":{\"Value\":\"second\"},\"DataSetWriterId\":2},"
        "{\"DataSetWriterId\":1,\"Payload\":{\"Value\":42}}]}");
    UA_NetworkMessage nm;
    ck_assert_uint_eq(UA_NetworkMessage_decodeJson(&json, &nm, &options, NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(nm.messageCount, 2);
    ck_assert_uint_eq(nm.dataSetWriterIds[0], 2);
    ck_assert_uint_eq(nm.dataSetWriterIds[1], 1);
    UA_Variant *value = &nm.payload.dataSetMessages[0].data.keyFrameFields[0].value;
    ck_assert(UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_STRING]));
    UA_String expected = UA_STRING("second");
    ck_assert(UA_String_equal((UA_String*)value->data, &expected));
    value = &nm.payload.dataSetMessages[1].data.keyFrameFields[0].value;
    ck_assert(UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert_int_eq(*(UA_Int32*)value->data, 42);
    UA_NetworkMessage_clear(&nm);
} END_TEST

static Suite *testSuite_networkmessage(void) {
    Suite *s = suite_create("Built-in Data Types 62541-6 Json");
    TCase *tc_json_networkmessage = tcase_create("networkmessage_json");

    tcase_add_test(tc_json_networkmessage, UA_Networkmessage_partialJsonUpdates);
    tcase_add_test(tc_json_networkmessage, UA_Networkmessage_omittedWriterId);
    tcase_add_test(tc_json_networkmessage, UA_Networkmessage_rawFieldsWithMetadata);
    tcase_add_test(tc_json_networkmessage, UA_Networkmessage_rawInvalidFields);
    tcase_add_test(tc_json_networkmessage, UA_Networkmessage_deltaDuplicateFields);
    tcase_add_test(tc_json_networkmessage, UA_Networkmessage_dataValueFieldsWithMetadata);
    tcase_add_loop_test(tc_json_networkmessage, UA_Networkmessage_rawArrays, 0, 5);
    tcase_add_test(tc_json_networkmessage, UA_Networkmessage_verboseArrayWithMetadata);
    tcase_add_test(tc_json_networkmessage, UA_Networkmessage_rawObjectBesideVerboseValue);
    tcase_add_test(tc_json_networkmessage, UA_Networkmessage_multipleWritersWithMetadata);
    tcase_add_test(tc_json_networkmessage, UA_PubSub_CompactJsonOptions);
    tcase_add_test(tc_json_networkmessage, UA_PubSub_EncodeAllOptionalFields);
    tcase_add_test(tc_json_networkmessage, UA_PubSub_EnDecode);
    tcase_add_test(tc_json_networkmessage, UA_NetworkMessage_oneMessage_twoFields_json_decode);
    tcase_add_test(tc_json_networkmessage, UA_NetworkMessage_json_decode);
    tcase_add_test(tc_json_networkmessage, UA_NetworkMessage_json_decode_messageObject);
    tcase_add_test(tc_json_networkmessage, UA_Networkmessage_DataSetFieldsNull_json_decode);
    tcase_add_test(tc_json_networkmessage, UA_Networkmessage_metaData_oob);
    tcase_add_test(tc_json_networkmessage, UA_Networkmessage_malformed_array);
    tcase_add_test(tc_json_networkmessage,
                   UA_Networkmessage_json_uint32_and_datavalue_roundtrip);
    tcase_add_test(tc_json_networkmessage,
                   UA_Networkmessage_json_publisherid_roundtrip);

    suite_add_tcase(s, tc_json_networkmessage);
    return s;
}

int main(void) {
    int      number_failed = 0;
    Suite   *s;
    SRunner *sr;

    s  = testSuite_networkmessage();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
