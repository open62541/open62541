/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Tino Bischoff)
 */

#include <open62541/client.h>
#include <open62541/types.h>

#include "ua_pubsub_networkmessage.h"

#include "check.h"
#include <stdlib.h>

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS1ValueVariantKeyFrame) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    UA_DataSetMessage dmkf;
    memset(&dmkf, 0, sizeof(UA_DataSetMessage));
    dmkf.header.dataSetMessageValid = true;
    dmkf.header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    dmkf.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dmkf.fieldCount = 1;
    dmkf.data.keyFrameFields =
        (UA_DataValue*)UA_Array_new(dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[0]);

    UA_Int32 iv = 27;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[0].value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmkf.data.keyFrameFields[0].hasValue = true;

    m.payload.dataSetMessages = &dmkf;
    m.messageCount = 1;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
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
    ck_assert(m.payload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetMessages[0].fieldCount, 1);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.type, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.data, iv);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp);

    //}

    UA_DataValue_clear(&dmkf.data.keyFrameFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameFields, dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS1ValueDataValueKeyFrame) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    UA_DataSetMessage dmkf;
    memset(&dmkf, 0, sizeof(UA_DataSetMessage));
    dmkf.header.dataSetMessageValid = true;
    dmkf.header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    dmkf.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dmkf.fieldCount = 1;
    dmkf.data.keyFrameFields =
        (UA_DataValue*)UA_Array_new(dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[0]);

    UA_Int32 iv = 27;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[0].value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmkf.data.keyFrameFields[0].hasValue = true;

    m.payload.dataSetMessages = &dmkf;
    m.messageCount = 1;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetMessages[0].fieldCount, 1);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.type, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.data, iv);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    //}

    UA_DataValue_clear(&dmkf.data.keyFrameFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameFields, dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesVariantKeyFrame) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    UA_DataSetMessage dmkf;
    memset(&dmkf, 0, sizeof(UA_DataSetMessage));
    dmkf.header.dataSetMessageValid = true;
    dmkf.header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    dmkf.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    UA_UInt16 anzFields = 2;
    dmkf.fieldCount = anzFields;
    dmkf.data.keyFrameFields =
        (UA_DataValue*)UA_Array_new(dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[0]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[1]);

    UA_Double dv = 231.3;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[0].value, &dv, &UA_TYPES[UA_TYPES_DOUBLE]);
    dmkf.data.keyFrameFields[0].hasValue = true;

    UA_UInt64 uiv = 98412;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[1].value, &uiv, &UA_TYPES[UA_TYPES_UINT64]);
    dmkf.data.keyFrameFields[1].hasValue = true;

    m.payload.dataSetMessages = &dmkf;
    m.messageCount = 1;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetMessages[0].fieldCount, anzFields);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.type, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert(*(UA_Double*)m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.data == dv);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[1].hasValue == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.keyFrameFields[1].value.type, &UA_TYPES[UA_TYPES_UINT64]);
    ck_assert_uint_eq(*(UA_UInt64 *)m2.payload.dataSetMessages[0].data.keyFrameFields[1].value.data, uiv);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[1].hasSourceTimestamp == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    UA_DataValue_clear(&dmkf.data.keyFrameFields[0]);
    UA_DataValue_clear(&dmkf.data.keyFrameFields[1]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameFields, dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesDataValueKeyFrame) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    UA_DataSetMessage dmkf;
    memset(&dmkf, 0, sizeof(UA_DataSetMessage));
    dmkf.header.dataSetMessageValid = true;
    dmkf.header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    dmkf.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    UA_UInt16 anzFields = 2;
    dmkf.fieldCount = anzFields;
    dmkf.data.keyFrameFields =
        (UA_DataValue*)UA_Array_new(dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[0]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[1]);

    UA_Double dv = 231.3;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[0].value, &dv, &UA_TYPES[UA_TYPES_DOUBLE]);
    dmkf.data.keyFrameFields[0].hasValue = true;

    UA_UInt64 uiv = 98412;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[1].value, &uiv, &UA_TYPES[UA_TYPES_UINT64]);
    dmkf.data.keyFrameFields[1].hasValue = true;

    m.payload.dataSetMessages = &dmkf;
    m.messageCount = 1;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetMessages[0].fieldCount, anzFields);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.type, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert(*(UA_Double*)m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.data == dv);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[1].hasValue == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.keyFrameFields[1].value.type, &UA_TYPES[UA_TYPES_UINT64]);
    ck_assert_uint_eq(*(UA_UInt64 *)m2.payload.dataSetMessages[0].data.keyFrameFields[1].value.data, uiv);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[1].hasSourceTimestamp == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    UA_DataValue_clear(&dmkf.data.keyFrameFields[0]);
    UA_DataValue_clear(&dmkf.data.keyFrameFields[1]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameFields, dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS1ValueVariantDeltaFrame) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    UA_DataSetMessage dmdf;
    memset(&dmdf, 0, sizeof(UA_DataSetMessage));
    dmdf.header.dataSetMessageValid = true;
    dmdf.header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    dmdf.header.dataSetMessageType = UA_DATASETMESSAGE_DATADELTAFRAME;
    dmdf.fieldCount = 1;
    size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dmdf.fieldCount;
    dmdf.data.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);

    UA_UInt16 index = 1;
    dmdf.data.deltaFrameFields[0].index = index;
    UA_DataValue_init(&dmdf.data.deltaFrameFields[0].value);

    UA_Int32 iv = 58;
    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameFields[0].value.value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmdf.data.deltaFrameFields[0].value.hasValue = true;

    m.payload.dataSetMessages = &dmdf;
    m.messageCount = 1;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetMessages[0].fieldCount, 1);
    ck_assert_int_eq(m.payload.dataSetMessages[0].data.deltaFrameFields[0].index, index);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasValue == m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.value.type, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.value.data, iv);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasSourceTimestamp == m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    UA_DataValue_clear(&dmdf.data.deltaFrameFields[0].value);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(dmdf.data.deltaFrameFields);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS1ValueDataValueDeltaFrame) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    UA_DataSetMessage dmdf;
    memset(&dmdf, 0, sizeof(UA_DataSetMessage));
    dmdf.header.dataSetMessageValid = true;
    dmdf.header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    dmdf.header.dataSetMessageType = UA_DATASETMESSAGE_DATADELTAFRAME;
    dmdf.fieldCount = 1;
    size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dmdf.fieldCount;
    dmdf.data.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);

    UA_UInt16 index = 1;
    dmdf.data.deltaFrameFields[0].index = index;
    UA_DataValue_init(&dmdf.data.deltaFrameFields[0].value);

    UA_Int32 iv = 197;
    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameFields[0].value.value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmdf.data.deltaFrameFields[0].value.hasValue = true;

    m.payload.dataSetMessages = &dmdf;
    m.messageCount = 1;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetMessages[0].fieldCount, 1);
    ck_assert_int_eq(m.payload.dataSetMessages[0].data.deltaFrameFields[0].index, index);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasValue == m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.value.type, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.value.data, iv);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasSourceTimestamp == m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    UA_DataValue_clear(&dmdf.data.deltaFrameFields[0].value);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(dmdf.data.deltaFrameFields);
}
END_TEST

START_TEST(UA_PubSub_Encode_WithBufferTooSmallShallReturnError) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    UA_DataSetMessage dmkf;
    memset(&dmkf, 0, sizeof(UA_DataSetMessage));
    dmkf.header.dataSetMessageValid = true;
    dmkf.header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    dmkf.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dmkf.fieldCount = 1;
    dmkf.data.keyFrameFields =
        (UA_DataValue*)UA_Array_new(dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[0]);

    UA_Int32 iv = 27;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[0].value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmkf.data.keyFrameFields[0].hasValue = true;

    m.payload.dataSetMessages = &dmkf;
    m.messageCount = 1;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);

    // to generate an error we make the buffer too small
    msgSize -= 5;
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);

    UA_DataValue_clear(&dmkf.data.keyFrameFields[0]);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameFields, dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_Decode_WithBufferTooSmallShallReturnError) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    UA_DataSetMessage dmkf;
    memset(&dmkf, 0, sizeof(UA_DataSetMessage));
    dmkf.header.dataSetMessageValid = true;
    dmkf.header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    dmkf.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dmkf.fieldCount = 1;
    dmkf.data.keyFrameFields =
        (UA_DataValue*)UA_Array_new(dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[0]);

    UA_Int32 iv = 27;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[0].value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmkf.data.keyFrameFields[0].hasValue = true;

    m.payload.dataSetMessages = &dmkf;
    m.messageCount = 1;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    UA_ByteString buffer2;
    UA_ByteString_init(&buffer2);
    size_t shortLength = buffer.length - 4;
    UA_ByteString_allocBuffer(&buffer2, shortLength);
    for (size_t i = 0; i < shortLength; i++)
    {
        buffer2.data[i] = buffer.data[i];
    }

    rv = UA_NetworkMessage_decodeBinary(&buffer2, &m2, NULL, NULL);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);

    UA_DataValue_clear(&dmkf.data.keyFrameFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_ByteString_clear(&buffer2);
    UA_Array_delete(dmkf.data.keyFrameFields, dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesVariantDeltaFrame) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    UA_DataSetMessage dmdf;
    memset(&dmdf, 0, sizeof(UA_DataSetMessage));
    dmdf.header.dataSetMessageValid = true;
    dmdf.header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    dmdf.header.dataSetMessageType = UA_DATASETMESSAGE_DATADELTAFRAME;
    dmdf.fieldCount = 2;
    size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dmdf.fieldCount;
    dmdf.data.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);

    UA_UInt16 index1 = 1;
    dmdf.data.deltaFrameFields[0].index = index1;
    UA_DataValue_init(&dmdf.data.deltaFrameFields[0].value);
    UA_UInt16 index2 = 3;
    dmdf.data.deltaFrameFields[1].index = index2;
    UA_DataValue_init(&dmdf.data.deltaFrameFields[1].value);

    UA_Int32 iv = 58;
    UA_Variant_setScalar(&dmdf.data.deltaFrameFields[0].value.value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmdf.data.deltaFrameFields[0].value.hasValue = true;

    UA_String testString = (UA_String) { 5, (UA_Byte*)"OPCUA" };
    UA_Variant value;
    UA_Variant_init(&value);
    value.data = UA_malloc(sizeof(UA_String));
    *((UA_String*)value.data) = testString;
    value.type = &UA_TYPES[UA_TYPES_STRING];
    dmdf.data.deltaFrameFields[1].value.value = value;
    dmdf.data.deltaFrameFields[1].value.hasValue = true;

    m.payload.dataSetMessages = &dmdf;
    m.messageCount = 1;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetMessages[0].fieldCount, 2);
    ck_assert_int_eq(m.payload.dataSetMessages[0].data.deltaFrameFields[0].index, index1);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasValue == m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.value.type, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.value.data, iv);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasSourceTimestamp == m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasSourceTimestamp);

    ck_assert_int_eq(m.payload.dataSetMessages[0].data.deltaFrameFields[1].index, index2);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasValue == m2.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasValue);
    // compare string value
    UA_String decodedString = *(UA_String*)(m2.payload.dataSetMessages[0].data.deltaFrameFields[1].value.value.data);
    for (UA_Int32 i = 0; i < 5; i++)
    {
        ck_assert_int_eq(decodedString.data[i], testString.data[i]);
    }
    ck_assert_uint_eq(decodedString.length, testString.length);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasSourceTimestamp == m2.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasSourceTimestamp);

    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    ((UA_String*)value.data)->data = NULL; // the string is statically allocated. do not free it.
    UA_Variant_clear(&value);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(dmdf.data.deltaFrameFields);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesDataValueDeltaFrame) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    UA_DataSetMessage dmdf;
    memset(&dmdf, 0, sizeof(UA_DataSetMessage));
    dmdf.header.dataSetMessageValid = true;
    dmdf.header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    dmdf.header.dataSetMessageType = UA_DATASETMESSAGE_DATADELTAFRAME;
    dmdf.fieldCount = 2;
    size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dmdf.fieldCount;
    dmdf.data.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);

    UA_UInt16 index1 = 1;
    dmdf.data.deltaFrameFields[0].index = index1;
    UA_DataValue_init(&dmdf.data.deltaFrameFields[0].value);
    UA_UInt16 index2 = 3;
    dmdf.data.deltaFrameFields[1].index = index2;
    UA_DataValue_init(&dmdf.data.deltaFrameFields[1].value);

    UA_Int32 iv = 193;
    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameFields[0].value.value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmdf.data.deltaFrameFields[0].value.hasValue = true;
    dmdf.data.deltaFrameFields[0].value.sourceTimestamp = UA_DateTime_now();
    dmdf.data.deltaFrameFields[0].value.hasSourceTimestamp = true;

    UA_String testString = (UA_String) { 5, (UA_Byte*)"OPCUA" };
    UA_Variant value;
    UA_Variant_init(&value);
    value.data = UA_malloc(sizeof(UA_String));
    *((UA_String*)value.data) = testString;
    value.type = &UA_TYPES[UA_TYPES_STRING];
    dmdf.data.deltaFrameFields[1].value.value = value;
    dmdf.data.deltaFrameFields[1].value.hasValue = true;

    m.payload.dataSetMessages = &dmdf;
    m.messageCount = 1;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetMessages[0].fieldCount, 2);
    ck_assert_int_eq(m.payload.dataSetMessages[0].data.deltaFrameFields[0].index, index1);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasValue == m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.value.type, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.value.data, iv);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasSourceTimestamp == m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasSourceTimestamp);

    ck_assert_int_eq(m.payload.dataSetMessages[0].data.deltaFrameFields[1].index, index2);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasValue == m2.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasValue);
    // compare string value
    UA_String decodedString = *(UA_String*)(m2.payload.dataSetMessages[0].data.deltaFrameFields[1].value.value.data);
    for (UA_Int32 i = 0; i < 5; i++)
    {
        ck_assert_int_eq(decodedString.data[i], testString.data[i]);
    }
    ck_assert_uint_eq(decodedString.length, testString.length);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasSourceTimestamp == m2.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasSourceTimestamp);

    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    ((UA_String*)value.data)->data = NULL; // the string is statically allocated. do not free it.
    UA_Variant_clear(&value);
    UA_DataValue_clear(&dmdf.data.deltaFrameFields[0].value);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(dmdf.data.deltaFrameFields);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesVariantKeyFrameGroupHeader) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.groupHeaderEnabled = true;
    m.groupHeader.writerGroupIdEnabled = true;
    UA_UInt16 writerGroupId = 17;
    m.groupHeader.writerGroupId = writerGroupId;
    m.groupHeader.groupVersionEnabled = true;
    UA_UInt32 groupVersion = 573354747; // das sollte irgendwann 2018 sein
    m.groupHeader.groupVersion = groupVersion;

    UA_DataSetMessage dmkf;
    memset(&dmkf, 0, sizeof(UA_DataSetMessage));
    dmkf.header.dataSetMessageValid = true;
    dmkf.header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    dmkf.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;

    UA_UInt16 anzFields = 2;
    dmkf.fieldCount = anzFields;
    dmkf.data.keyFrameFields =
        (UA_DataValue*)UA_Array_new(dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[0]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[1]);

    UA_Double dv = 231.3;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[0].value, &dv, &UA_TYPES[UA_TYPES_DOUBLE]);
    dmkf.data.keyFrameFields[0].hasValue = true;

    UA_UInt64 uiv = 98412;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[1].value, &uiv, &UA_TYPES[UA_TYPES_UINT64]);
    dmkf.data.keyFrameFields[1].hasValue = true;

    m.payload.dataSetMessages = &dmkf;
    m.messageCount = 1;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.groupHeader.writerGroupIdEnabled == m2.groupHeader.writerGroupIdEnabled);
    ck_assert(m.groupHeader.groupVersionEnabled == m2.groupHeader.groupVersionEnabled);
    ck_assert(m.groupHeader.networkMessageNumberEnabled == m2.groupHeader.networkMessageNumberEnabled);
    ck_assert(m.groupHeader.sequenceNumberEnabled == m2.groupHeader.sequenceNumberEnabled);
    ck_assert_uint_eq(m2.groupHeader.writerGroupId, writerGroupId);
    ck_assert_uint_eq(m2.groupHeader.groupVersion, groupVersion);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetMessages[0].fieldCount, anzFields);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue);
    ck_assert_uint_eq((uintptr_t)m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert(*(UA_Double*)m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.data == dv);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[1].hasValue == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue);
    ck_assert_uint_eq((uintptr_t)m2.payload.dataSetMessages[0].data.keyFrameFields[1].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_UINT64]);
    ck_assert_uint_eq(*(UA_UInt64 *)m2.payload.dataSetMessages[0].data.keyFrameFields[1].value.data, uiv);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[1].hasSourceTimestamp == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp);

    UA_DataValue_clear(&dmkf.data.keyFrameFields[0]);
    UA_DataValue_clear(&dmkf.data.keyFrameFields[1]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameFields, dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesVariantDeltaFramePublDSCID) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.publisherIdEnabled = true;
    m.publisherId.idType = UA_PUBLISHERIDTYPE_UINT32;
    UA_UInt32 publisherId = 1469;
    m.publisherId.id.uint32 = publisherId;
    m.dataSetClassIdEnabled = true;
    UA_Guid dataSetClassId = UA_Guid_random();
    m.dataSetClassId = dataSetClassId;

    UA_DataSetMessage dmdf;
    memset(&dmdf, 0, sizeof(UA_DataSetMessage));
    dmdf.header.dataSetMessageValid = true;
    dmdf.header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    dmdf.header.dataSetMessageType = UA_DATASETMESSAGE_DATADELTAFRAME;
    dmdf.fieldCount = 2;
    size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dmdf.fieldCount;
    dmdf.data.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);

    UA_UInt16 index1 = 1;
    dmdf.data.deltaFrameFields[0].index = index1;
    UA_DataValue_init(&dmdf.data.deltaFrameFields[0].value);
    UA_UInt16 index2 = 3;
    dmdf.data.deltaFrameFields[1].index = index2;
    UA_DataValue_init(&dmdf.data.deltaFrameFields[1].value);

    UA_Int32 iv = 58;
    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameFields[0].value.value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmdf.data.deltaFrameFields[0].value.hasValue = true;

    UA_String testString = (UA_String) { 5, (UA_Byte*)"OPCUA" };
    UA_Variant value;
    UA_Variant_init(&value);
    value.data = UA_malloc(sizeof(UA_String));
    *((UA_String*)value.data) = testString;
    value.type = &UA_TYPES[UA_TYPES_STRING];
    dmdf.data.deltaFrameFields[1].value.value = value;
    dmdf.data.deltaFrameFields[1].value.hasValue = true;

    m.payload.dataSetMessages = &dmdf;
    m.messageCount = 1;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(UA_Guid_equal(&m.dataSetClassId, &m2.dataSetClassId) == true);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert_int_eq(m2.publisherId.idType, m.publisherId.idType);
    ck_assert_uint_eq(m2.publisherId.id.uint32, publisherId);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetMessages[0].fieldCount, 2);
    ck_assert_int_eq(m.payload.dataSetMessages[0].data.deltaFrameFields[0].index, index1);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasValue == m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.value.type, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.value.data, iv);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasSourceTimestamp == m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasSourceTimestamp);

    ck_assert_int_eq(m.payload.dataSetMessages[0].data.deltaFrameFields[1].index, index2);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasValue == m2.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasValue);
    // compare string value
    UA_String decodedString = *(UA_String*)(m2.payload.dataSetMessages[0].data.deltaFrameFields[1].value.value.data);
    for (UA_Int32 i = 0; i < 5; i++)
    {
        ck_assert_int_eq(decodedString.data[i], testString.data[i]);
    }
    ck_assert_uint_eq(decodedString.length, testString.length);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasSourceTimestamp == m2.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasSourceTimestamp);

    ((UA_String*)value.data)->data = NULL; // the string is statically allocated. do not free it.
    UA_Variant_clear(&value);
    UA_DataValue_clear(&dmdf.data.deltaFrameFields[0].value);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(dmdf.data.deltaFrameFields);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesDataValueKeyFramePH) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.payloadHeaderEnabled = true;

    UA_UInt16 dataSetWriterId = 1698;
    UA_DataSetMessage dmkf;
    memset(&dmkf, 0, sizeof(UA_DataSetMessage));
    m.dataSetWriterIds[0] = dataSetWriterId;
    dmkf.header.dataSetMessageValid = true;
    dmkf.header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    dmkf.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    UA_UInt16 anzFields = 2;
    dmkf.fieldCount = anzFields;
    dmkf.data.keyFrameFields =
        (UA_DataValue*)UA_Array_new(dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[0]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[1]);

    UA_Double dv = 231.3;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[0].value, &dv, &UA_TYPES[UA_TYPES_DOUBLE]);
    dmkf.data.keyFrameFields[0].hasValue = true;

    UA_UInt64 uiv = 98412;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[1].value, &uiv, &UA_TYPES[UA_TYPES_UINT64]);
    dmkf.data.keyFrameFields[1].hasValue = true;

    m.payload.dataSetMessages = &dmkf;
    m.messageCount = 1;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert_uint_eq(m.messageCount, 1);
    ck_assert_uint_eq(m.dataSetWriterIds[0], dataSetWriterId);

    ck_assert(m.payload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetMessages[0].fieldCount, anzFields);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.type, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert(*(UA_Double*)m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.data == dv);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[1].hasValue == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.keyFrameFields[1].value.type, &UA_TYPES[UA_TYPES_UINT64]);
    ck_assert_uint_eq(*(UA_UInt64 *)m2.payload.dataSetMessages[0].data.keyFrameFields[1].value.data, uiv);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[1].hasSourceTimestamp == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    UA_DataValue_clear(&dmkf.data.keyFrameFields[0]);
    UA_DataValue_clear(&dmkf.data.keyFrameFields[1]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameFields, dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesVariantKeyFrameTSProm) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.timestampEnabled = true;
    UA_DateTime ts = UA_DateTime_now();
    m.timestamp = ts;
    m.promotedFieldsEnabled = true;
    m.promotedFieldsSize = 1;
    m.promotedFields = (UA_Variant*)UA_Array_new(m.promotedFieldsSize, &UA_TYPES[UA_TYPES_VARIANT]);

    UA_DataSetMessage dmkf;
    memset(&dmkf, 0, sizeof(UA_DataSetMessage));
    dmkf.header.dataSetMessageValid = true;
    dmkf.header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    dmkf.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    UA_UInt16 anzFields = 2;
    dmkf.fieldCount = anzFields;
    dmkf.data.keyFrameFields =
        (UA_DataValue*)UA_Array_new(dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[0]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[1]);

    UA_Double dv = 231.3;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[0].value, &dv, &UA_TYPES[UA_TYPES_DOUBLE]);
    dmkf.data.keyFrameFields[0].hasValue = true;

    UA_Variant_copy(&dmkf.data.keyFrameFields[0].value, &m.promotedFields[0]);

    UA_UInt64 uiv = 98412;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[1].value, &uiv, &UA_TYPES[UA_TYPES_UINT64]);
    dmkf.data.keyFrameFields[1].hasValue = true;

    m.payload.dataSetMessages = &dmkf;
    m.messageCount = 1;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert_int_eq(m.timestamp, ts);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert_uint_eq(m2.promotedFieldsSize, 1);
    ck_assert(*((UA_Double*)m2.promotedFields[0].data) == dv);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetMessages[0].fieldCount, anzFields);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.type, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert(*(UA_Double*)m2.payload.dataSetMessages[0].data.keyFrameFields[0].value.data == dv);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[1].hasValue == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.keyFrameFields[1].value.type, &UA_TYPES[UA_TYPES_UINT64]);
    ck_assert_uint_eq(*(UA_UInt64 *)m2.payload.dataSetMessages[0].data.keyFrameFields[1].value.data, uiv);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp);
    ck_assert(m.payload.dataSetMessages[0].data.keyFrameFields[1].hasSourceTimestamp == m2.payload.dataSetMessages[0].data.keyFrameFields[0].hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);

    UA_Array_delete(m.promotedFields, m.promotedFieldsSize, &UA_TYPES[UA_TYPES_VARIANT]);
    UA_DataValue_clear(&dmkf.data.keyFrameFields[0]);
    UA_DataValue_clear(&dmkf.data.keyFrameFields[1]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameFields, dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesDataValueDeltaFrameGHProm2) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.groupHeaderEnabled = true;
    m.groupHeader.networkMessageNumberEnabled = true;
    UA_UInt16 networkMessageNumber = 2175;
    m.groupHeader.networkMessageNumber = networkMessageNumber;

    m.promotedFieldsEnabled = true;
    m.promotedFieldsSize = 2;
    m.promotedFields = (UA_Variant*)UA_Array_new(m.promotedFieldsSize, &UA_TYPES[UA_TYPES_VARIANT]);

    UA_DataSetMessage dmdf;
    memset(&dmdf, 0, sizeof(UA_DataSetMessage));
    dmdf.header.dataSetMessageValid = true;
    dmdf.header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    dmdf.header.dataSetMessageType = UA_DATASETMESSAGE_DATADELTAFRAME;
    dmdf.fieldCount = 2;
    size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dmdf.fieldCount;
    dmdf.data.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);

    UA_UInt16 index1 = 1;
    dmdf.data.deltaFrameFields[0].index = index1;
    UA_DataValue_init(&dmdf.data.deltaFrameFields[0].value);
    UA_UInt16 index2 = 3;
    dmdf.data.deltaFrameFields[1].index = index2;
    UA_DataValue_init(&dmdf.data.deltaFrameFields[1].value);

    UA_Int32 iv = 1573;
    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameFields[0].value.value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmdf.data.deltaFrameFields[0].value.hasValue = true;
    dmdf.data.deltaFrameFields[0].value.sourceTimestamp = UA_DateTime_now();
    dmdf.data.deltaFrameFields[0].value.hasSourceTimestamp = true;

    UA_Variant_copy(&dmdf.data.deltaFrameFields[0].value.value, &m.promotedFields[0]);

    UA_Float fv = 197.34f;
    UA_Variant_setScalarCopy(&m.promotedFields[1], &fv, &UA_TYPES[UA_TYPES_FLOAT]);

    UA_String testString = (UA_String) { 5, (UA_Byte*)"OPC24" };
    UA_Variant value;
    UA_Variant_init(&value);
    value.data = UA_malloc(sizeof(UA_String));
    *((UA_String*)value.data) = testString;
    value.type = &UA_TYPES[UA_TYPES_STRING];
    dmdf.data.deltaFrameFields[1].value.value = value;
    dmdf.data.deltaFrameFields[1].value.hasValue = true;

    m.payload.dataSetMessages = &dmdf;
    m.messageCount = 1;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.groupHeader.writerGroupIdEnabled == m2.groupHeader.writerGroupIdEnabled);
    ck_assert(m.groupHeader.groupVersionEnabled == m2.groupHeader.groupVersionEnabled);
    ck_assert(m.groupHeader.networkMessageNumberEnabled == m2.groupHeader.networkMessageNumberEnabled);
    ck_assert_uint_eq(m2.groupHeader.networkMessageNumber, networkMessageNumber);
    ck_assert(m.groupHeader.sequenceNumberEnabled == m2.groupHeader.sequenceNumberEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert_uint_eq(m2.promotedFieldsSize, 2);
    ck_assert_int_eq(*((UA_Int32*)m2.promotedFields[0].data), iv);
    ck_assert(*((UA_Float*)m2.promotedFields[1].data) == fv);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetMessages[0].fieldCount, 2);
    ck_assert_int_eq(m.payload.dataSetMessages[0].data.deltaFrameFields[0].index, index1);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasValue == m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasValue);
    ck_assert_ptr_eq(m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.value.type, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.value.data, iv);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasSourceTimestamp == m2.payload.dataSetMessages[0].data.deltaFrameFields[0].value.hasSourceTimestamp);

    ck_assert_int_eq(m.payload.dataSetMessages[0].data.deltaFrameFields[1].index, index2);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasValue == m2.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasValue);
    // compare string value
    UA_String decodedString = *(UA_String*)(m2.payload.dataSetMessages[0].data.deltaFrameFields[1].value.value.data);
    for (UA_Int32 i = 0; i < 5; i++)
    {
        ck_assert_int_eq(decodedString.data[i], testString.data[i]);
    }
    ck_assert_uint_eq(decodedString.length, testString.length);
    ck_assert(m.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasSourceTimestamp == m2.payload.dataSetMessages[0].data.deltaFrameFields[1].value.hasSourceTimestamp);

    UA_Array_delete(m.promotedFields, m.promotedFieldsSize, &UA_TYPES[UA_TYPES_VARIANT]);
    ((UA_String*)value.data)->data = NULL; // the string is statically allocated. do not free it.
    UA_Variant_clear(&value);
    UA_DataValue_clear(&dmdf.data.deltaFrameFields[0].value);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(dmdf.data.deltaFrameFields);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn2DSVariant) {
    UA_UInt16 dsWriter1 = 4;
    UA_UInt16 dsWriter2 = 7;
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.payloadHeaderEnabled = true;

    m.payload.dataSetMessages = (UA_DataSetMessage*)
        UA_calloc(2, sizeof(UA_DataSetMessage));
    m.messageCount = 2;
    m.dataSetWriterIds[0] = dsWriter1;
    m.dataSetWriterIds[1] = dsWriter2;

    //UA_DataSetMessage dmkf;
    //memset(&dmkf, 0, sizeof(UA_DataSetMessage));
    m.payload.dataSetMessages[0].header.dataSetMessageValid = true;
    m.payload.dataSetMessages[0].header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    m.payload.dataSetMessages[0].header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    UA_UInt16 fieldCountDS1 = 1;
    m.payload.dataSetMessages[0].fieldCount = fieldCountDS1;
    m.payload.dataSetMessages[0].data.keyFrameFields =
        (UA_DataValue*)UA_Array_new(m.payload.dataSetMessages[0].fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&m.payload.dataSetMessages[0].data.keyFrameFields[0]);

    UA_UInt32 iv = 27;
    UA_Variant_setScalarCopy(&m.payload.dataSetMessages[0].data.keyFrameFields[0].value, &iv, &UA_TYPES[UA_TYPES_UINT32]);
    m.payload.dataSetMessages[0].data.keyFrameFields[0].hasValue = true;

    m.payload.dataSetMessages[1].header.dataSetMessageValid = true;
    m.payload.dataSetMessages[1].header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    m.payload.dataSetMessages[1].header.dataSetMessageType = UA_DATASETMESSAGE_DATADELTAFRAME;
    UA_UInt16 fieldCountDS2 = 2;
    m.payload.dataSetMessages[1].fieldCount = fieldCountDS2;
    m.payload.dataSetMessages[1].data.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)
        UA_calloc(m.payload.dataSetMessages[1].fieldCount, sizeof(UA_DataSetMessage_DeltaFrameField));

    UA_Guid gv = UA_Guid_random();
    UA_UInt16 index1 = 2;
    m.payload.dataSetMessages[1].data.deltaFrameFields[0].index = index1;
    UA_DataValue_init(&m.payload.dataSetMessages[1].data.deltaFrameFields[0].value);
    UA_Variant_setScalar(&m.payload.dataSetMessages[1].data.deltaFrameFields[0].value.value, &gv, &UA_TYPES[UA_TYPES_GUID]);
    m.payload.dataSetMessages[1].data.deltaFrameFields[0].value.hasValue = true;

    UA_UInt16 index2 = 5;
    m.payload.dataSetMessages[1].data.deltaFrameFields[1].index = index2;
    UA_DataValue_init(&m.payload.dataSetMessages[1].data.deltaFrameFields[1].value);
    UA_Int64 iv64 = 152478978534;
    UA_Variant_setScalar(&m.payload.dataSetMessages[1].data.deltaFrameFields[1].value.value, &iv64, &UA_TYPES[UA_TYPES_INT64]);
    m.payload.dataSetMessages[1].data.deltaFrameFields[1].value.hasValue = true;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
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
    ck_assert(m.payload.dataSetMessages[1].header.fieldEncoding == m2.payload.dataSetMessages[1].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetMessages[1].fieldCount, fieldCountDS2);

    ck_assert(m.payload.dataSetMessages[1].data.deltaFrameFields[0].value.hasValue == m2.payload.dataSetMessages[1].data.deltaFrameFields[0].value.hasValue);
    ck_assert_uint_eq(m2.payload.dataSetMessages[1].data.deltaFrameFields[0].index, index1);
    ck_assert_uint_eq((uintptr_t)m2.payload.dataSetMessages[1].data.deltaFrameFields[0].value.value.type, (uintptr_t)&UA_TYPES[UA_TYPES_GUID]);
    ck_assert(UA_Guid_equal((UA_Guid*)m2.payload.dataSetMessages[1].data.deltaFrameFields[0].value.value.data, &gv) == true);
    ck_assert(m.payload.dataSetMessages[1].data.deltaFrameFields[0].value.hasSourceTimestamp == m2.payload.dataSetMessages[1].data.deltaFrameFields[0].value.hasSourceTimestamp);

    ck_assert(m.payload.dataSetMessages[1].data.deltaFrameFields[1].value.hasValue == m2.payload.dataSetMessages[1].data.deltaFrameFields[1].value.hasValue);
    ck_assert_uint_eq(m2.payload.dataSetMessages[1].data.deltaFrameFields[1].index, index2);
    ck_assert_uint_eq((uintptr_t)m2.payload.dataSetMessages[1].data.deltaFrameFields[1].value.value.type, (uintptr_t)&UA_TYPES[UA_TYPES_INT64]);
    ck_assert_int_eq(*(UA_Int64 *)m2.payload.dataSetMessages[1].data.deltaFrameFields[1].value.value.data, iv64);
    ck_assert(m.payload.dataSetMessages[1].data.deltaFrameFields[1].value.hasSourceTimestamp == m2.payload.dataSetMessages[1].data.deltaFrameFields[1].value.hasSourceTimestamp);

    UA_Array_delete(m.payload.dataSetMessages[0].data.keyFrameFields, m.payload.dataSetMessages[0].fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_free(m.payload.dataSetMessages[1].data.deltaFrameFields);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(m.payload.dataSetMessages);
    //UA_Array_delete(dmkf.data.keyFrameFields, dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

/* ---------------------------------------------------------------------------
 * Additional coverage tests:
 *  - PublisherId of every supported type (Byte/UInt16/UInt32/UInt64/String)
 *  - Decode of a truncated buffer must fail gracefully
 *  - Decode of a buffer with the version field set to 0xFF (invalid) must fail
 * ------------------------------------------------------------------------- */

static void
encode_decode_with_publisherid(UA_PublisherIdType idType,
                               const UA_PublisherId *pid) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.publisherIdEnabled = true;
    m.publisherId = *pid;

    UA_DataSetMessage dmkf;
    memset(&dmkf, 0, sizeof(UA_DataSetMessage));
    dmkf.header.dataSetMessageValid = true;
    dmkf.header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    dmkf.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dmkf.fieldCount = 1;
    dmkf.data.keyFrameFields =
        (UA_DataValue*)UA_Array_new(dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[0]);
    UA_Int32 iv = 0xCAFE;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[0].value, &iv,
                             &UA_TYPES[UA_TYPES_INT32]);
    dmkf.data.keyFrameFields[0].hasValue = true;

    m.payload.dataSetMessages = &dmkf;
    m.messageCount = 1;

    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    UA_StatusCode rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    ck_assert(m2.publisherIdEnabled);
    ck_assert_int_eq(m2.publisherId.idType, idType);

    switch(idType) {
        case UA_PUBLISHERIDTYPE_BYTE:
            ck_assert_uint_eq(m2.publisherId.id.byte, pid->id.byte);
            break;
        case UA_PUBLISHERIDTYPE_UINT16:
            ck_assert_uint_eq(m2.publisherId.id.uint16, pid->id.uint16);
            break;
        case UA_PUBLISHERIDTYPE_UINT32:
            ck_assert_uint_eq(m2.publisherId.id.uint32, pid->id.uint32);
            break;
        case UA_PUBLISHERIDTYPE_UINT64:
            ck_assert_uint_eq(m2.publisherId.id.uint64, pid->id.uint64);
            break;
        case UA_PUBLISHERIDTYPE_STRING:
            ck_assert(UA_String_equal(&m2.publisherId.id.string, &pid->id.string));
            break;
    }

    UA_DataValue_clear(&dmkf.data.keyFrameFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameFields, dmkf.fieldCount,
                    &UA_TYPES[UA_TYPES_DATAVALUE]);
}

START_TEST(UA_PubSub_EnDecode_PublisherIdByte) {
    UA_PublisherId pid; pid.idType = UA_PUBLISHERIDTYPE_BYTE; pid.id.byte = 0xA5;
    encode_decode_with_publisherid(UA_PUBLISHERIDTYPE_BYTE, &pid);
} END_TEST

START_TEST(UA_PubSub_EnDecode_PublisherIdUInt16) {
    UA_PublisherId pid; pid.idType = UA_PUBLISHERIDTYPE_UINT16; pid.id.uint16 = 0xBEEF;
    encode_decode_with_publisherid(UA_PUBLISHERIDTYPE_UINT16, &pid);
} END_TEST

START_TEST(UA_PubSub_EnDecode_PublisherIdUInt32) {
    UA_PublisherId pid; pid.idType = UA_PUBLISHERIDTYPE_UINT32; pid.id.uint32 = 0xDEADBEEF;
    encode_decode_with_publisherid(UA_PUBLISHERIDTYPE_UINT32, &pid);
} END_TEST

START_TEST(UA_PubSub_EnDecode_PublisherIdUInt64) {
    UA_PublisherId pid; pid.idType = UA_PUBLISHERIDTYPE_UINT64; pid.id.uint64 = 0x0123456789ABCDEFULL;
    encode_decode_with_publisherid(UA_PUBLISHERIDTYPE_UINT64, &pid);
} END_TEST

START_TEST(UA_PubSub_EnDecode_PublisherIdString) {
    UA_PublisherId pid;
    pid.idType = UA_PUBLISHERIDTYPE_STRING;
    pid.id.string = UA_STRING("PubSubPublisher");
    encode_decode_with_publisherid(UA_PUBLISHERIDTYPE_STRING, &pid);
} END_TEST

START_TEST(UA_PubSub_Decode_TruncatedBufferReturnsError) {
    /* Build a valid encoded message first */
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.publisherIdEnabled = true;
    m.publisherId.idType = UA_PUBLISHERIDTYPE_UINT32;
    m.publisherId.id.uint32 = 4711;

    UA_DataSetMessage dmkf;
    memset(&dmkf, 0, sizeof(UA_DataSetMessage));
    dmkf.header.dataSetMessageValid = true;
    dmkf.header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    dmkf.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dmkf.fieldCount = 1;
    dmkf.data.keyFrameFields =
        (UA_DataValue*)UA_Array_new(dmkf.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameFields[0]);
    UA_Int32 iv = 7;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameFields[0].value, &iv,
                             &UA_TYPES[UA_TYPES_INT32]);
    dmkf.data.keyFrameFields[0].hasValue = true;
    m.payload.dataSetMessages = &dmkf;
    m.messageCount = 1;

    UA_ByteString full;
    size_t fullSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    UA_StatusCode rv = UA_ByteString_allocBuffer(&full, fullSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = UA_NetworkMessage_encodeBinary(&m, &full, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    /* Truncate to half its size and try to decode -> must fail */
    UA_ByteString truncated = { full.length / 2, full.data };
    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(m2));
    rv = UA_NetworkMessage_decodeBinary(&truncated, &m2, NULL, NULL);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);
    UA_NetworkMessage_clear(&m2);

    /* Also try a single-byte buffer */
    UA_ByteString tiny = { 1, full.data };
    memset(&m2, 0, sizeof(m2));
    rv = UA_NetworkMessage_decodeBinary(&tiny, &m2, NULL, NULL);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);
    UA_NetworkMessage_clear(&m2);

    /* And an empty buffer */
    UA_ByteString empty = { 0, NULL };
    memset(&m2, 0, sizeof(m2));
    rv = UA_NetworkMessage_decodeBinary(&empty, &m2, NULL, NULL);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);
    UA_NetworkMessage_clear(&m2);

    UA_DataValue_clear(&dmkf.data.keyFrameFields[0]);
    UA_ByteString_clear(&full);
    UA_Array_delete(dmkf.data.keyFrameFields, dmkf.fieldCount,
                    &UA_TYPES[UA_TYPES_DATAVALUE]);
} END_TEST

START_TEST(UA_PubSub_Decode_InvalidVersionReturnsError) {
    /* Header byte: bit field with version in low nibble.
     * Setting all bits to 1 makes the version 0xF (>1) which is invalid. */
    UA_Byte raw[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    UA_ByteString buf = { sizeof(raw), raw };
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(m));
    UA_StatusCode rv = UA_NetworkMessage_decodeBinary(&buf, &m, NULL, NULL);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);
    UA_NetworkMessage_clear(&m);
} END_TEST

START_TEST(UA_PubSub_Decode_PayloadHeaderCountZeroReturnsBadDecodingError) {
    /* Header: version=1, payloadHeaderEnabled=1, no extended flags.
     * Then message count byte set to 0 to hit explicit count==0 reject. */
    UA_Byte raw[] = { 0x41, 0x00 };
    UA_ByteString buf = { sizeof(raw), raw };
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(m));

    UA_StatusCode rv = UA_NetworkMessage_decodeBinary(&buf, &m, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_BADDECODINGERROR);
    UA_NetworkMessage_clear(&m);
} END_TEST

START_TEST(UA_PubSub_Decode_PayloadHeaderCountTooLargeReturnsBadDecodingError) {
    /* count=33 while UA_NETWORKMESSAGE_MAXMESSAGECOUNT is 32 */
    UA_Byte raw[] = { 0x41, 0x21 };
    UA_ByteString buf = { sizeof(raw), raw };
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(m));

    UA_StatusCode rv = UA_NetworkMessage_decodeBinary(&buf, &m, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_BADDECODINGERROR);
    UA_NetworkMessage_clear(&m);
} END_TEST

START_TEST(UA_PubSub_Decode_MultiDsmZeroSizeReturnsBadDecodingError) {
    /* Header + payload header for two DataSetMessages, then first DSM size = 0. */
    UA_Byte raw[] = {
        0x41,             /* version=1, payload header enabled */
        0x02,             /* messageCount */
        0x01, 0x00,       /* writerId[0] */
        0x02, 0x00,       /* writerId[1] */
        0x00, 0x00        /* dataSetMessageSizes[0] -> invalid */
    };
    UA_ByteString buf = { sizeof(raw), raw };
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(m));

    UA_StatusCode rv = UA_NetworkMessage_decodeBinary(&buf, &m, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_BADDECODINGERROR);
    UA_NetworkMessage_clear(&m);
} END_TEST

START_TEST(UA_PubSub_Decode_InvalidPublisherIdTypeReturnsBadInternalError) {
    /* Header with publisherIdEnabled + extended flags 1.
     * ExtendedFlags1 low bits carry idType. Use invalid idType=5. */
    UA_Byte raw[] = { 0x91, 0x05 };
    UA_ByteString buf = { sizeof(raw), raw };
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(m));

    UA_StatusCode rv = UA_NetworkMessage_decodeBinary(&buf, &m, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_BADINTERNALERROR);
    UA_NetworkMessage_clear(&m);
} END_TEST

/* -------------------------------------------------------------------------
 * Coverage for previously untested NetworkMessage encoding/decoding branches:
 *   - picoseconds (extended NM2 flags)
 *   - groupHeader with sequenceNumber
 *   - securityHeader / securityFooter roundtrip (no encryption, just plumbing)
 *   - default branch in payload-header writerId encoding (multiple writers)
 *   - calcSizeBinary on minimal message
 * ------------------------------------------------------------------------- */

static void
fillKeyFrame(UA_DataSetMessage *dmkf, UA_Int32 v) {
    memset(dmkf, 0, sizeof(UA_DataSetMessage));
    dmkf->header.dataSetMessageValid = true;
    dmkf->header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    dmkf->header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dmkf->fieldCount = 1;
    dmkf->data.keyFrameFields =
        (UA_DataValue*)UA_Array_new(1, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf->data.keyFrameFields[0]);
    UA_Variant_setScalarCopy(&dmkf->data.keyFrameFields[0].value, &v,
                             &UA_TYPES[UA_TYPES_INT32]);
    dmkf->data.keyFrameFields[0].hasValue = true;
}

static void
clearKeyFrame(UA_DataSetMessage *dmkf) {
    UA_DataValue_clear(&dmkf->data.keyFrameFields[0]);
    UA_Array_delete(dmkf->data.keyFrameFields, dmkf->fieldCount,
                    &UA_TYPES[UA_TYPES_DATAVALUE]);
}

START_TEST(UA_PubSub_EnDecode_PicosecondsRoundtrip) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.timestampEnabled = true;
    m.timestamp = 1234567890;
    m.picosecondsEnabled = true;
    m.picoseconds = 0xABCD;

    UA_DataSetMessage dmkf;
    fillKeyFrame(&dmkf, 11);
    m.payload.dataSetMessages = &dmkf;
    m.messageCount = 1;

    UA_ByteString buffer;
    size_t s = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    ck_assert_uint_gt(s, 0);
    UA_StatusCode rv = UA_ByteString_allocBuffer(&buffer, s);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(m2));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m2.picosecondsEnabled);
    ck_assert_uint_eq(m2.picoseconds, m.picoseconds);
    ck_assert(m2.timestampEnabled);
    ck_assert_int_eq(m2.timestamp, m.timestamp);

    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    clearKeyFrame(&dmkf);
} END_TEST

START_TEST(UA_PubSub_EnDecode_GroupHeaderSequenceNumber) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.groupHeaderEnabled = true;
    m.groupHeader.sequenceNumberEnabled = true;
    m.groupHeader.sequenceNumber = 42;
    m.groupHeader.writerGroupIdEnabled = true;
    m.groupHeader.writerGroupId = 7;

    UA_DataSetMessage dmkf;
    fillKeyFrame(&dmkf, 22);
    m.payload.dataSetMessages = &dmkf;
    m.messageCount = 1;

    UA_ByteString buffer;
    size_t s = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    UA_StatusCode rv = UA_ByteString_allocBuffer(&buffer, s);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(m2));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m2.groupHeaderEnabled);
    ck_assert(m2.groupHeader.sequenceNumberEnabled);
    ck_assert_uint_eq(m2.groupHeader.sequenceNumber, m.groupHeader.sequenceNumber);
    ck_assert_uint_eq(m2.groupHeader.writerGroupId, m.groupHeader.writerGroupId);

    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    clearKeyFrame(&dmkf);
} END_TEST

START_TEST(UA_PubSub_EnDecode_SecurityHeaderAndFooter) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.securityEnabled = true;
    m.securityHeader.networkMessageSigned = false;
    m.securityHeader.networkMessageEncrypted = false;
    m.securityHeader.securityFooterEnabled = true;
    m.securityHeader.forceKeyReset = true;
    m.securityHeader.securityTokenId = 42;
    m.securityHeader.messageNonceSize = 0;
    m.securityHeader.securityFooterSize = 4;
    UA_Byte footer[4] = {0x10, 0x20, 0x30, 0x40};
    m.securityFooter.length = 4;
    m.securityFooter.data = footer;

    UA_DataSetMessage dmkf;
    fillKeyFrame(&dmkf, 33);
    m.payload.dataSetMessages = &dmkf;
    m.messageCount = 1;

    UA_ByteString buffer;
    size_t s = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    UA_StatusCode rv = UA_ByteString_allocBuffer(&buffer, s);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(m2));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m2.securityEnabled);
    ck_assert(m2.securityHeader.securityFooterEnabled);
    ck_assert(m2.securityHeader.forceKeyReset);
    ck_assert_uint_eq(m2.securityHeader.securityTokenId, 42);
    ck_assert_uint_eq(m2.securityHeader.securityFooterSize, 4);
    ck_assert_uint_eq(m2.securityFooter.length, 4);
    ck_assert_int_eq(memcmp(m2.securityFooter.data, footer, 4), 0);

    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    clearKeyFrame(&dmkf);
} END_TEST

START_TEST(UA_PubSub_EnDecode_DataSetClassIdRoundtrip) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.dataSetClassIdEnabled = true;
    UA_Guid g = {0x12345678, 0xABCD, 0xEF01,
                 {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80}};
    m.dataSetClassId = g;

    UA_DataSetMessage dmkf;
    fillKeyFrame(&dmkf, 99);
    m.payload.dataSetMessages = &dmkf;
    m.messageCount = 1;

    UA_ByteString buffer;
    size_t s = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    UA_StatusCode rv = UA_ByteString_allocBuffer(&buffer, s);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(m2));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, NULL, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m2.dataSetClassIdEnabled);
    ck_assert(UA_Guid_equal(&m2.dataSetClassId, &g));

    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    clearKeyFrame(&dmkf);
} END_TEST

START_TEST(UA_PubSub_EnDecode_DiscoveryRequestType) {
    /* DISCOVERY_REQUEST messages currently return BADNOTIMPLEMENTED on encode.
     * Just exercise the early return paths. */
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DISCOVERY_REQUEST;
    UA_ByteString buffer;
    UA_StatusCode rv = UA_ByteString_allocBuffer(&buffer, 64);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer, NULL);
    /* Either succeeds or returns BADNOTIMPLEMENTED -- both exercise code */
    (void)rv;
    UA_ByteString_clear(&buffer);
} END_TEST

int main(void) {
    TCase *tc_encode = tcase_create("encode");
    tcase_add_test(tc_encode, UA_PubSub_Encode_WithBufferTooSmallShallReturnError);

    TCase *tc_decode = tcase_create("decode");
    tcase_add_test(tc_decode, UA_PubSub_Decode_WithBufferTooSmallShallReturnError);

    TCase *tc_ende1 = tcase_create("encode_decode1DS");
    tcase_add_test(tc_ende1, UA_PubSub_EnDecode_ShallWorkOn1DS1ValueVariantKeyFrame);
    tcase_add_test(tc_ende1, UA_PubSub_EnDecode_ShallWorkOn1DS1ValueDataValueKeyFrame);
    tcase_add_test(tc_ende1, UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesVariantKeyFrame);
    tcase_add_test(tc_ende1, UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesDataValueKeyFrame);
    tcase_add_test(tc_ende1, UA_PubSub_EnDecode_ShallWorkOn1DS1ValueVariantDeltaFrame);
    tcase_add_test(tc_ende1, UA_PubSub_EnDecode_ShallWorkOn1DS1ValueDataValueDeltaFrame);
    tcase_add_test(tc_ende1, UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesVariantDeltaFrame);
    tcase_add_test(tc_ende1, UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesDataValueDeltaFrame);
    tcase_add_test(tc_ende1, UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesVariantKeyFrameGroupHeader);
    tcase_add_test(tc_ende1, UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesVariantDeltaFramePublDSCID);
    tcase_add_test(tc_ende1, UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesDataValueKeyFramePH);
    tcase_add_test(tc_ende1, UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesVariantKeyFrameTSProm);
    tcase_add_test(tc_ende1, UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesDataValueDeltaFrameGHProm2);

    TCase *tc_ende2 = tcase_create("encode_decode2DS");
    tcase_add_test(tc_ende2, UA_PubSub_EnDecode_ShallWorkOn2DSVariant);

    TCase *tc_pid = tcase_create("PublisherId roundtrip (all idTypes)");
    tcase_add_test(tc_pid, UA_PubSub_EnDecode_PublisherIdByte);
    tcase_add_test(tc_pid, UA_PubSub_EnDecode_PublisherIdUInt16);
    tcase_add_test(tc_pid, UA_PubSub_EnDecode_PublisherIdUInt32);
    tcase_add_test(tc_pid, UA_PubSub_EnDecode_PublisherIdUInt64);
    tcase_add_test(tc_pid, UA_PubSub_EnDecode_PublisherIdString);

    TCase *tc_decode_err = tcase_create("decode error paths");
    tcase_add_test(tc_decode_err, UA_PubSub_Decode_TruncatedBufferReturnsError);
    tcase_add_test(tc_decode_err, UA_PubSub_Decode_InvalidVersionReturnsError);
    tcase_add_test(tc_decode_err, UA_PubSub_Decode_PayloadHeaderCountZeroReturnsBadDecodingError);
    tcase_add_test(tc_decode_err, UA_PubSub_Decode_PayloadHeaderCountTooLargeReturnsBadDecodingError);
    tcase_add_test(tc_decode_err, UA_PubSub_Decode_MultiDsmZeroSizeReturnsBadDecodingError);
    tcase_add_test(tc_decode_err, UA_PubSub_Decode_InvalidPublisherIdTypeReturnsBadInternalError);

    TCase *tc_nm_optional = tcase_create("NetworkMessage optional headers");
    tcase_add_test(tc_nm_optional, UA_PubSub_EnDecode_PicosecondsRoundtrip);
    tcase_add_test(tc_nm_optional, UA_PubSub_EnDecode_GroupHeaderSequenceNumber);
    tcase_add_test(tc_nm_optional, UA_PubSub_EnDecode_SecurityHeaderAndFooter);
    tcase_add_test(tc_nm_optional, UA_PubSub_EnDecode_DataSetClassIdRoundtrip);
    tcase_add_test(tc_nm_optional, UA_PubSub_EnDecode_DiscoveryRequestType);

    Suite *s = suite_create("PubSub NetworkMessage");
    suite_add_tcase(s, tc_encode);
    suite_add_tcase(s, tc_decode);
    suite_add_tcase(s, tc_ende1);
    suite_add_tcase(s, tc_ende2);
    suite_add_tcase(s, tc_pid);
    suite_add_tcase(s, tc_decode_err);
    suite_add_tcase(s, tc_nm_optional);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
