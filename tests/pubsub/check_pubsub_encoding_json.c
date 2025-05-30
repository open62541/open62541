/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Lukas Meling)
 */

#include <open62541/client.h>
#include <open62541/types.h>
#include <open62541/util.h>
#include <open62541/pubsub.h>

#include <check.h>
#include <stdlib.h>

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
    char* result = "{\"MessageId\":\"ABCDEFGH\",\"MessageType\":\"ua-data\",\"PublisherId\":\"65535\",\"DataSetClassId\":\"00000001-0002-0003-0000-000000000000\",\"Messages\":[{\"DataSetWriterId\":12345,\"SequenceNumber\":4711,\"MetaDataVersion\":{\"MajorVersion\":42,\"MinorVersion\":7},\"Timestamp\":\"1601-01-13T20:38:31.1111111Z\",\"Status\":12345,\"MessageType\":\"ua-keyframe\",\"Payload\":{\"Field1\":{\"UaType\":7,\"Value\":27}}}]}";
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
    fmd1[0].name = UA_STRING("Field2.2");
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
    ck_assert(m.payload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetMessages[0].header.fieldEncoding);
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
    ck_assert_int_eq(out.payload.dataSetMessages[0].header.fieldEncoding, UA_FIELDENCODING_VARIANT);

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
    UA_ByteString buf = UA_STRING("{\"MessageId\":\"5ED82C10-50BB-CD07-0120-22521081E8EE\",\"MessageType\":\"ua-data\",\"Messages\":[{\"MetaDataVersion\":{\"MajorVersion\": 47, \"MinorVersion\": 47},\"DataSetWriterId\":62541,\"Status\":22,\"SequenceNumber\":4711,\"Payload\":{\"Test\":{\"UaType\":5,\"Value\":42},\"Server localtime\":{\"UaType\":1,\"Value\":true}}}]}");
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
    ck_assert_int_eq(out.payload.dataSetMessages[0].header.fieldEncoding, UA_FIELDENCODING_VARIANT);

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


static Suite *testSuite_networkmessage(void) {
    Suite *s = suite_create("Built-in Data Types 62541-6 Json");
    TCase *tc_json_networkmessage = tcase_create("networkmessage_json");

    tcase_add_test(tc_json_networkmessage, UA_PubSub_EncodeAllOptionalFields);
    tcase_add_test(tc_json_networkmessage, UA_PubSub_EnDecode);
    tcase_add_test(tc_json_networkmessage, UA_NetworkMessage_oneMessage_twoFields_json_decode);
    tcase_add_test(tc_json_networkmessage, UA_NetworkMessage_json_decode);
    tcase_add_test(tc_json_networkmessage, UA_NetworkMessage_json_decode_messageObject);
    tcase_add_test(tc_json_networkmessage, UA_Networkmessage_DataSetFieldsNull_json_decode);

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
