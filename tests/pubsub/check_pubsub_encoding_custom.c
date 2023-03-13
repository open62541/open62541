/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2023 basysKom GmbH (Author: Marius Dege)
 */

#include <open62541/client.h>
#include <open62541/types.h>
#include <open62541/server.h>

#include "ua_pubsub_networkmessage.h"

#include "check.h"

UA_NodeId pointVariableNode;

const UA_NodeId pointVariableTypeId = {
    1, UA_NODEIDTYPE_NUMERIC, {4243}};

/* The custom datatype for describing a 3d position */

typedef struct {
    UA_Float x;
    UA_Float y;
    UA_Float z;
} Point;

/* The datatype description for the Point datatype */

#define padding_y offsetof(Point,y) - offsetof(Point,x) - sizeof(UA_Float)
#define padding_z offsetof(Point,z) - offsetof(Point,y) - sizeof(UA_Float)

static UA_DataTypeMember Point_members[3] = {
    /* x */
    {
        UA_TYPENAME("x")           /* .memberName */
        &UA_TYPES[UA_TYPES_FLOAT], /* .memberType */
        0,                         /* .padding */
        false,                     /* .isArray */
        false                      /* .isOptional*/
    },

    /* y */
    {
        UA_TYPENAME("y")
        &UA_TYPES[UA_TYPES_FLOAT],
        padding_y,
        false,
        false
    },

    /* z */
    {
        UA_TYPENAME("z")
        &UA_TYPES[UA_TYPES_FLOAT],
        padding_z,
        false,
        false
    }
};

static const UA_DataType PointType = {
    UA_TYPENAME("Point")             /* .typeName */
    {1, UA_NODEIDTYPE_NUMERIC, {1}}, /* .typeId */
    {1, UA_NODEIDTYPE_NUMERIC, {17}}, /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
    sizeof(Point),                   /* .memSize */
    UA_DATATYPEKIND_STRUCTURE,       /* .typeKind */
    true,                            /* .pointerFree */
    false,                           /* .overlayable (depends on endianness and
                                         the absence of padding) */
    3,                               /* .membersSize */
    Point_members
};

const UA_DataTypeArray customDataTypes = {NULL, 1, &PointType, UA_FALSE};

START_TEST(UA_PubSub_EnDecode_ShallWorkOn1DS1CustomTypeDeltaFrame) {
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

    Point p;
    p.x = 1.0;
    p.y = 2.0;
    p.z = 3.0;

    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &p, &PointType);
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
    rv = UA_NetworkMessage_decodeBinary_custom(&buffer, &offset, &m2, &customDataTypes);

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
//    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, &PointType);
    ck_assert_double_eq((*(Point *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).x, p.x);
    ck_assert_double_eq((*(Point *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).y, p.y);
    ck_assert_double_eq((*(Point *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).z, p.z);
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

int main(void) {

    TCase *tc_encode = tcase_create("encode_decode1DS1CustomType");
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_ShallWorkOn1DS1CustomTypeDeltaFrame);

    Suite *s = suite_create("PubSub Custom Types with Raw Encoding");
    suite_add_tcase(s, tc_encode);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
