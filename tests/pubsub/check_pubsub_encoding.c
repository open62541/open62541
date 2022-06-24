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
    dmkf.data.keyFrameData.fieldCount = 1;
    dmkf.data.keyFrameData.dataSetFields =
        (UA_DataValue*)UA_Array_new(dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameData.dataSetFields[0]);

    UA_Int32 iv = 27;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    //if (rv == UA_STATUSCODE_GOOD) {
    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    size_t offset = 0;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &offset, &m2);
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
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.fieldCount, 1);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data, iv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);

    //}

    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
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
    dmkf.data.keyFrameData.fieldCount = 1;
    dmkf.data.keyFrameData.dataSetFields =
        (UA_DataValue*)UA_Array_new(dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameData.dataSetFields[0]);

    UA_Int32 iv = 27;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    //if (rv == UA_STATUSCODE_GOOD) {
    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    size_t offset = 0;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &offset, &m2);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.fieldCount, 1);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data, iv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    //}

    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
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
    dmkf.data.keyFrameData.fieldCount = anzFields;
    dmkf.data.keyFrameData.dataSetFields =
        (UA_DataValue*)UA_Array_new(dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_DataValue_init(&dmkf.data.keyFrameData.dataSetFields[1]);

    UA_Double dv = 231.3;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, &dv, &UA_TYPES[UA_TYPES_DOUBLE]);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    UA_UInt64 uiv = 98412;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[1].value, &uiv, &UA_TYPES[UA_TYPES_UINT64]);
    dmkf.data.keyFrameData.dataSetFields[1].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    size_t offset = 0;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &offset, &m2);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.fieldCount, anzFields);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert(*(UA_Double*)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data == dv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_UINT64]);
    ck_assert_uint_eq(*(UA_UInt64 *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].value.data, uiv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[1]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
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
    dmkf.data.keyFrameData.fieldCount = anzFields;
    dmkf.data.keyFrameData.dataSetFields =
        (UA_DataValue*)UA_Array_new(dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_DataValue_init(&dmkf.data.keyFrameData.dataSetFields[1]);

    UA_Double dv = 231.3;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, &dv, &UA_TYPES[UA_TYPES_DOUBLE]);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    UA_UInt64 uiv = 98412;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[1].value, &uiv, &UA_TYPES[UA_TYPES_UINT64]);
    dmkf.data.keyFrameData.dataSetFields[1].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    size_t offset = 0;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &offset, &m2);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.fieldCount, anzFields);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert(*(UA_Double*)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data == dv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_UINT64]);
    ck_assert_uint_eq(*(UA_UInt64 *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].value.data, uiv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[1]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
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
    dmdf.data.deltaFrameData.fieldCount = 1;
    size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dmdf.data.deltaFrameData.fieldCount;
    dmdf.data.deltaFrameData.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);

    UA_UInt16 fieldIndex = 1;
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldIndex = fieldIndex;
    UA_DataValue_init(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue);

    UA_Int32 iv = 58;
    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmdf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    size_t offset = 0;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &offset, &m2);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.fieldCount, 1);
    ck_assert_int_eq(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldIndex, fieldIndex);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, (uintptr_t)&UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data, iv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    UA_DataValue_clear(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(dmdf.data.deltaFrameData.deltaFrameFields);
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
    dmdf.data.deltaFrameData.fieldCount = 1;
    size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dmdf.data.deltaFrameData.fieldCount;
    dmdf.data.deltaFrameData.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);

    UA_UInt16 fieldIndex = 1;
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldIndex = fieldIndex;
    UA_DataValue_init(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue);

    UA_Int32 iv = 197;
    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmdf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    size_t offset = 0;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &offset, &m2);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.fieldCount, 1);
    ck_assert_int_eq(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldIndex, fieldIndex);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, (uintptr_t)&UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data, iv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    UA_DataValue_clear(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(dmdf.data.deltaFrameData.deltaFrameFields);
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
    dmkf.data.keyFrameData.fieldCount = 1;
    dmkf.data.keyFrameData.dataSetFields =
        (UA_DataValue*)UA_Array_new(dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameData.dataSetFields[0]);

    UA_Int32 iv = 27;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);

    // to generate an error we make the buffer too small
    msgSize -= 5;
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);

    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
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
    dmkf.data.keyFrameData.fieldCount = 1;
    dmkf.data.keyFrameData.dataSetFields =
        (UA_DataValue*)UA_Array_new(dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameData.dataSetFields[0]);

    UA_Int32 iv = 27;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    size_t offset = 0;
    UA_ByteString buffer2;
    UA_ByteString_init(&buffer2);
    size_t shortLength = buffer.length - 4;
    UA_ByteString_allocBuffer(&buffer2, shortLength);
    for (size_t i = 0; i < shortLength; i++)
    {
        buffer2.data[i] = buffer.data[i];
    }

    rv = UA_NetworkMessage_decodeBinary(&buffer2, &offset, &m2);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);

    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_ByteString_clear(&buffer2);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
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
    dmdf.data.deltaFrameData.fieldCount = 2;
    size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dmdf.data.deltaFrameData.fieldCount;
    dmdf.data.deltaFrameData.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);

    UA_UInt16 fieldIndex1 = 1;
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldIndex = fieldIndex1;
    UA_DataValue_init(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue);
    UA_UInt16 fieldIndex2 = 3;
    dmdf.data.deltaFrameData.deltaFrameFields[1].fieldIndex = fieldIndex2;
    UA_DataValue_init(&dmdf.data.deltaFrameData.deltaFrameFields[1].fieldValue);

    UA_Int32 iv = 58;
    UA_Variant_setScalar(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue = true;

    UA_String testString = (UA_String) { 5, (UA_Byte*)"OPCUA" };
    UA_Variant value;
    UA_Variant_init(&value);
    value.data = UA_malloc(sizeof(UA_String));
    *((UA_String*)value.data) = testString;
    value.type = &UA_TYPES[UA_TYPES_STRING];
    dmdf.data.deltaFrameData.deltaFrameFields[1].fieldValue.value = value;
    dmdf.data.deltaFrameData.deltaFrameFields[1].fieldValue.hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmdf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    size_t offset = 0;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &offset, &m2);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.fieldCount, 2);
    ck_assert_int_eq(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldIndex, fieldIndex1);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, (uintptr_t)&UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data, iv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp);

    ck_assert_int_eq(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldIndex, fieldIndex2);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasValue);
    // compare string value
    UA_String decodedString = *(UA_String*)(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.value.data);
    for (UA_Int32 i = 0; i < 5; i++)
    {
        ck_assert_int_eq(decodedString.data[i], testString.data[i]);
    }
    ck_assert_int_eq(decodedString.length, testString.length);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasSourceTimestamp);

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
    UA_free(dmdf.data.deltaFrameData.deltaFrameFields);
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
    dmdf.data.deltaFrameData.fieldCount = 2;
    size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dmdf.data.deltaFrameData.fieldCount;
    dmdf.data.deltaFrameData.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);

    UA_UInt16 fieldIndex1 = 1;
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldIndex = fieldIndex1;
    UA_DataValue_init(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue);
    UA_UInt16 fieldIndex2 = 3;
    dmdf.data.deltaFrameData.deltaFrameFields[1].fieldIndex = fieldIndex2;
    UA_DataValue_init(&dmdf.data.deltaFrameData.deltaFrameFields[1].fieldValue);

    UA_Int32 iv = 193;
    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue = true;
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.sourceTimestamp = UA_DateTime_now();
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp = true;

    UA_String testString = (UA_String) { 5, (UA_Byte*)"OPCUA" };
    UA_Variant value;
    UA_Variant_init(&value);
    value.data = UA_malloc(sizeof(UA_String));
    *((UA_String*)value.data) = testString;
    value.type = &UA_TYPES[UA_TYPES_STRING];
    dmdf.data.deltaFrameData.deltaFrameFields[1].fieldValue.value = value;
    dmdf.data.deltaFrameData.deltaFrameFields[1].fieldValue.hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmdf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    size_t offset = 0;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &offset, &m2);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.fieldCount, 2);
    ck_assert_int_eq(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldIndex, fieldIndex1);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, (uintptr_t)&UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data, iv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp);

    ck_assert_int_eq(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldIndex, fieldIndex2);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasValue);
    // compare string value
    UA_String decodedString = *(UA_String*)(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.value.data);
    for (UA_Int32 i = 0; i < 5; i++)
    {
        ck_assert_int_eq(decodedString.data[i], testString.data[i]);
    }
    ck_assert_int_eq(decodedString.length, testString.length);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasSourceTimestamp);

    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    ((UA_String*)value.data)->data = NULL; // the string is statically allocated. do not free it.
    UA_Variant_clear(&value);
    UA_DataValue_clear(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(dmdf.data.deltaFrameData.deltaFrameFields);
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
    dmkf.data.keyFrameData.fieldCount = anzFields;
    dmkf.data.keyFrameData.dataSetFields =
        (UA_DataValue*)UA_Array_new(dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_DataValue_init(&dmkf.data.keyFrameData.dataSetFields[1]);

    UA_Double dv = 231.3;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, &dv, &UA_TYPES[UA_TYPES_DOUBLE]);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    UA_UInt64 uiv = 98412;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[1].value, &uiv, &UA_TYPES[UA_TYPES_UINT64]);
    dmkf.data.keyFrameData.dataSetFields[1].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    size_t offset = 0;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &offset, &m2);
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
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.fieldCount, anzFields);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue);
    ck_assert_uint_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert(*(UA_Double*)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data == dv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue);
    ck_assert_uint_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_UINT64]);
    ck_assert_uint_eq(*(UA_UInt64 *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].value.data, uiv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);

    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[1]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesVariantDeltaFramePublDSCID) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.publisherIdEnabled = true;
    m.publisherIdType = UA_PUBLISHERDATATYPE_UINT32;
    UA_UInt32 publisherId = 1469;
    m.publisherId.publisherIdUInt32 = publisherId;
    m.dataSetClassIdEnabled = true;
    UA_Guid dataSetClassId = UA_Guid_random();
    m.dataSetClassId = dataSetClassId;

    UA_DataSetMessage dmdf;
    memset(&dmdf, 0, sizeof(UA_DataSetMessage));
    dmdf.header.dataSetMessageValid = true;
    dmdf.header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    dmdf.header.dataSetMessageType = UA_DATASETMESSAGE_DATADELTAFRAME;
    dmdf.data.deltaFrameData.fieldCount = 2;
    size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dmdf.data.deltaFrameData.fieldCount;
    dmdf.data.deltaFrameData.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);

    UA_UInt16 fieldIndex1 = 1;
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldIndex = fieldIndex1;
    UA_DataValue_init(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue);
    UA_UInt16 fieldIndex2 = 3;
    dmdf.data.deltaFrameData.deltaFrameFields[1].fieldIndex = fieldIndex2;
    UA_DataValue_init(&dmdf.data.deltaFrameData.deltaFrameFields[1].fieldValue);

    UA_Int32 iv = 58;
    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue = true;

    UA_String testString = (UA_String) { 5, (UA_Byte*)"OPCUA" };
    UA_Variant value;
    UA_Variant_init(&value);
    value.data = UA_malloc(sizeof(UA_String));
    *((UA_String*)value.data) = testString;
    value.type = &UA_TYPES[UA_TYPES_STRING];
    dmdf.data.deltaFrameData.deltaFrameFields[1].fieldValue.value = value;
    dmdf.data.deltaFrameData.deltaFrameFields[1].fieldValue.hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmdf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    size_t offset = 0;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &offset, &m2);
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
    ck_assert_int_eq(m2.publisherIdType, m.publisherIdType);
    ck_assert_uint_eq(m2.publisherId.publisherIdUInt32, publisherId);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.fieldCount, 2);
    ck_assert_int_eq(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldIndex, fieldIndex1);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, (uintptr_t)&UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data, iv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp);

    ck_assert_int_eq(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldIndex, fieldIndex2);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasValue);
    // compare string value
    UA_String decodedString = *(UA_String*)(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.value.data);
    for (UA_Int32 i = 0; i < 5; i++)
    {
        ck_assert_int_eq(decodedString.data[i], testString.data[i]);
    }
    ck_assert_int_eq(decodedString.length, testString.length);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasSourceTimestamp);

    ((UA_String*)value.data)->data = NULL; // the string is statically allocated. do not free it.
    UA_Variant_clear(&value);
    UA_DataValue_clear(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(dmdf.data.deltaFrameData.deltaFrameFields);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS2ValuesDataValueKeyFramePH) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.payloadHeaderEnabled = true;
    m.payloadHeader.dataSetPayloadHeader.count = 1;
    m.payloadHeader.dataSetPayloadHeader.dataSetWriterIds = (UA_UInt16 *)UA_Array_new(m.payloadHeader.dataSetPayloadHeader.count, &UA_TYPES[UA_TYPES_UINT16]);
    UA_UInt16 dataSetWriterId = 1698;
    m.payloadHeader.dataSetPayloadHeader.dataSetWriterIds[0] = dataSetWriterId;

    UA_DataSetMessage dmkf;
    memset(&dmkf, 0, sizeof(UA_DataSetMessage));
    dmkf.header.dataSetMessageValid = true;
    dmkf.header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    dmkf.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    UA_UInt16 anzFields = 2;
    dmkf.data.keyFrameData.fieldCount = anzFields;
    dmkf.data.keyFrameData.dataSetFields =
        (UA_DataValue*)UA_Array_new(dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_DataValue_init(&dmkf.data.keyFrameData.dataSetFields[1]);

    UA_Double dv = 231.3;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, &dv, &UA_TYPES[UA_TYPES_DOUBLE]);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    UA_UInt64 uiv = 98412;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[1].value, &uiv, &UA_TYPES[UA_TYPES_UINT64]);
    dmkf.data.keyFrameData.dataSetFields[1].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    size_t offset = 0;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &offset, &m2);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(m.version == m2.version);
    ck_assert(m.networkMessageType == m2.networkMessageType);
    ck_assert(m.timestampEnabled == m2.timestampEnabled);
    ck_assert(m.payloadHeaderEnabled == m2.payloadHeaderEnabled);
    ck_assert_uint_eq(m.payloadHeader.dataSetPayloadHeader.count, 1);
    ck_assert_uint_eq(m.payloadHeader.dataSetPayloadHeader.dataSetWriterIds[0], dataSetWriterId);

    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.fieldCount, anzFields);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert(*(UA_Double*)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data == dv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_UINT64]);
    ck_assert_uint_eq(*(UA_UInt64 *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].value.data, uiv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    UA_Array_delete(m.payloadHeader.dataSetPayloadHeader.dataSetWriterIds, m.payloadHeader.dataSetPayloadHeader.count, &UA_TYPES[UA_TYPES_UINT16]);
    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[1]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
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
    dmkf.data.keyFrameData.fieldCount = anzFields;
    dmkf.data.keyFrameData.dataSetFields =
        (UA_DataValue*)UA_Array_new(dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_DataValue_init(&dmkf.data.keyFrameData.dataSetFields[1]);

    UA_Double dv = 231.3;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, &dv, &UA_TYPES[UA_TYPES_DOUBLE]);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    UA_Variant_copy(&dmkf.data.keyFrameData.dataSetFields[0].value, &m.promotedFields[0]);

    UA_UInt64 uiv = 98412;
    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[1].value, &uiv, &UA_TYPES[UA_TYPES_UINT64]);
    dmkf.data.keyFrameData.dataSetFields[1].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    size_t offset = 0;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &offset, &m2);
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
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.fieldCount, anzFields);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert(*(UA_Double*)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data == dv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_UINT64]);
    ck_assert_uint_eq(*(UA_UInt64 *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].value.data, uiv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[1].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);

    UA_Array_delete(m.promotedFields, m.promotedFieldsSize, &UA_TYPES[UA_TYPES_VARIANT]);
    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[1]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
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
    dmdf.data.deltaFrameData.fieldCount = 2;
    size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dmdf.data.deltaFrameData.fieldCount;
    dmdf.data.deltaFrameData.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);

    UA_UInt16 fieldIndex1 = 1;
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldIndex = fieldIndex1;
    UA_DataValue_init(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue);
    UA_UInt16 fieldIndex2 = 3;
    dmdf.data.deltaFrameData.deltaFrameFields[1].fieldIndex = fieldIndex2;
    UA_DataValue_init(&dmdf.data.deltaFrameData.deltaFrameFields[1].fieldValue);

    UA_Int32 iv = 1573;
    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &iv, &UA_TYPES[UA_TYPES_INT32]);
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue = true;
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.sourceTimestamp = UA_DateTime_now();
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp = true;

    UA_Variant_copy(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &m.promotedFields[0]);

    UA_Float fv = 197.34f;
    UA_Variant_setScalarCopy(&m.promotedFields[1], &fv, &UA_TYPES[UA_TYPES_FLOAT]);

    UA_String testString = (UA_String) { 5, (UA_Byte*)"OPC24" };
    UA_Variant value;
    UA_Variant_init(&value);
    value.data = UA_malloc(sizeof(UA_String));
    *((UA_String*)value.data) = testString;
    value.type = &UA_TYPES[UA_TYPES_STRING];
    dmdf.data.deltaFrameData.deltaFrameFields[1].fieldValue.value = value;
    dmdf.data.deltaFrameData.deltaFrameFields[1].fieldValue.hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmdf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    size_t offset = 0;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &offset, &m2);
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
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.fieldCount, 2);
    ck_assert_int_eq(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldIndex, fieldIndex1);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue);
    ck_assert_int_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, (uintptr_t)&UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data, iv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp);

    ck_assert_int_eq(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldIndex, fieldIndex2);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasValue);
    // compare string value
    UA_String decodedString = *(UA_String*)(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.value.data);
    for (UA_Int32 i = 0; i < 5; i++)
    {
        ck_assert_int_eq(decodedString.data[i], testString.data[i]);
    }
    ck_assert_int_eq(decodedString.length, testString.length);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasSourceTimestamp);

    UA_Array_delete(m.promotedFields, m.promotedFieldsSize, &UA_TYPES[UA_TYPES_VARIANT]);
    ((UA_String*)value.data)->data = NULL; // the string is statically allocated. do not free it.
    UA_Variant_clear(&value);
    UA_DataValue_clear(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(dmdf.data.deltaFrameData.deltaFrameFields);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_ShallWorkOn2DSVariant) {
    UA_NetworkMessage m;
    memset(&m, 0, sizeof(UA_NetworkMessage));
    m.version = 1;
    m.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    m.payloadHeaderEnabled = true;
    m.payloadHeader.dataSetPayloadHeader.count = 2;
    UA_UInt16 dsWriter1 = 4;
    UA_UInt16 dsWriter2 = 7;
    m.payloadHeader.dataSetPayloadHeader.dataSetWriterIds = (UA_UInt16 *)UA_Array_new(m.payloadHeader.dataSetPayloadHeader.count, &UA_TYPES[UA_TYPES_UINT16]);
    m.payloadHeader.dataSetPayloadHeader.dataSetWriterIds[0] = dsWriter1;
    m.payloadHeader.dataSetPayloadHeader.dataSetWriterIds[1] = dsWriter2;

    size_t memsize = m.payloadHeader.dataSetPayloadHeader.count * sizeof(UA_DataSetMessage);
    m.payload.dataSetPayload.dataSetMessages = (UA_DataSetMessage*)UA_malloc(memsize);
    memset(m.payload.dataSetPayload.dataSetMessages, 0, memsize);

    //UA_DataSetMessage dmkf;
    //memset(&dmkf, 0, sizeof(UA_DataSetMessage));
    m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid = true;
    m.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    UA_UInt16 fieldCountDS1 = 1;
    m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.fieldCount = fieldCountDS1;
    m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields =
        (UA_DataValue*)UA_Array_new(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_DataValue_init(&m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0]);

    UA_UInt32 iv = 27;
    UA_Variant_setScalarCopy(&m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value, &iv, &UA_TYPES[UA_TYPES_UINT32]);
    m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages[1].header.dataSetMessageValid = true;
    m.payload.dataSetPayload.dataSetMessages[1].header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    m.payload.dataSetPayload.dataSetMessages[1].header.dataSetMessageType = UA_DATASETMESSAGE_DATADELTAFRAME;
    UA_UInt16 fieldCountDS2 = 2;
    m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.fieldCount = fieldCountDS2;
    memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.fieldCount;
    m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);

    UA_Guid gv = UA_Guid_random();
    UA_UInt16 fieldIndex1 = 2;
    m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[0].fieldIndex = fieldIndex1;
    UA_DataValue_init(&m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[0].fieldValue);
    UA_Variant_setScalar(&m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &gv, &UA_TYPES[UA_TYPES_GUID]);
    m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue = true;

    UA_UInt16 fieldIndex2 = 5;
    m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[1].fieldIndex = fieldIndex2;
    UA_DataValue_init(&m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[1].fieldValue);
    UA_Int64 iv64 = 152478978534;
    UA_Variant_setScalar(&m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[1].fieldValue.value, &iv64, &UA_TYPES[UA_TYPES_INT64]);
    m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasValue = true;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m, NULL);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buffer.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &(buffer.data[buffer.length]);
    rv = UA_NetworkMessage_encodeBinary(&m, &bufPos, bufEnd, NULL);

    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    size_t offset = 0;
    rv = UA_NetworkMessage_decodeBinary(&buffer, &offset, &m2);
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
    ck_assert_uint_eq(m2.payloadHeader.dataSetPayloadHeader.dataSetWriterIds[0], dsWriter1);
    ck_assert_uint_eq(m2.payloadHeader.dataSetPayloadHeader.dataSetWriterIds[1], dsWriter2);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid == m2.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding == m2.payload.dataSetPayload.dataSetMessages[0].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.fieldCount, fieldCountDS1);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasValue);
    ck_assert_uint_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, (uintptr_t)&UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(*(UA_UInt32 *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data, iv);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);

    ck_assert(m.payload.dataSetPayload.dataSetMessages[1].header.dataSetMessageValid == m2.payload.dataSetPayload.dataSetMessages[1].header.dataSetMessageValid);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[1].header.fieldEncoding == m2.payload.dataSetPayload.dataSetMessages[1].header.fieldEncoding);
    ck_assert_int_eq(m2.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.fieldCount, fieldCountDS2);

    ck_assert(m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue == m2.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue);
    ck_assert_uint_eq(m2.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[0].fieldIndex, fieldIndex1);
    ck_assert_uint_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, (uintptr_t)&UA_TYPES[UA_TYPES_GUID]);
    ck_assert(UA_Guid_equal((UA_Guid*)m2.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data, &gv) == true);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp);

    ck_assert(m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasValue == m2.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasValue);
    ck_assert_uint_eq(m2.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[1].fieldIndex, fieldIndex2);
    ck_assert_uint_eq((uintptr_t)m2.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[1].fieldValue.value.type, (uintptr_t)&UA_TYPES[UA_TYPES_INT64]);
    ck_assert_int_eq(*(UA_Int64 *)m2.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[1].fieldValue.value.data, iv64);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields[1].fieldValue.hasSourceTimestamp);

    UA_Array_delete(m.payloadHeader.dataSetPayloadHeader.dataSetWriterIds, m.payloadHeader.dataSetPayloadHeader.count, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Array_delete(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields, m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_free(m.payload.dataSetPayload.dataSetMessages[1].data.deltaFrameData.deltaFrameFields);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(m.payload.dataSetPayload.dataSetMessages);
    //UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

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

    Suite *s = suite_create("PubSub NetworkMessage");
    suite_add_tcase(s, tc_encode);
    suite_add_tcase(s, tc_decode);
    suite_add_tcase(s, tc_ende1);
    suite_add_tcase(s, tc_ende2);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
