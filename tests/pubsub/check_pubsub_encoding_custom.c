/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2023 basysKom GmbH (Author: Marius Dege)
 */

#include <open62541/types.h>

#include "ua_pubsub.h"
#include "ua_pubsub_networkmessage.h"

#include "math.h"
#include "check.h"

/* The custom datatype for describing a 3d position */

typedef struct {
    UA_Float x;
    UA_Float y;
    UA_Float z;
} Point;

/* The datatype description for the Point datatype */

#define padding_y offsetof(Point,y) - offsetof(Point,x) - sizeof(UA_Float)
#define padding_z offsetof(Point,z) - offsetof(Point,y) - sizeof(UA_Float)

static UA_DataTypeMember members[3] = {
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
    members
};

const UA_DataTypeArray customDataTypes = {NULL, 1, &PointType, UA_FALSE};

typedef struct {
    UA_Int16 a;
    UA_Float *b;
    UA_Float *c;
    UA_String *d;
} Opt;

static UA_DataTypeMember Opt_members[4] = {
        /* a */
        {
                UA_TYPENAME("a")           /* .memberName */
                &UA_TYPES[UA_TYPES_INT16], /* .memberType */
                0,                         /* .padding */
                false,                     /* .isArray */
                false                      /* .isOptional */
        },
        /* b */
        {
                UA_TYPENAME("b")
                &UA_TYPES[UA_TYPES_FLOAT], /* .memberType */
                offsetof(Opt,b) - offsetof(Opt,a) - sizeof(UA_Int16),
                false,
                true        /* b is an optional field */
        },
        /* c */
        {
                UA_TYPENAME("c")
                &UA_TYPES[UA_TYPES_FLOAT], /* .memberType */
                offsetof(Opt,c) - offsetof(Opt,b) - sizeof(void *),
                false,
                true        /* b is an optional field */
        },
        /* d */
        {
                UA_TYPENAME("d")
                &UA_TYPES[UA_TYPES_STRING], /* .memberType */
                offsetof(Opt,d) - offsetof(Opt,c) - sizeof(void *),
                false,
                true        /* d is an optional field */
        }
};

static const UA_DataType OptType = {
        UA_TYPENAME("Opt")             /* .typeName */
        {1, UA_NODEIDTYPE_NUMERIC, {4242}}, /* .typeId */
        {1, UA_NODEIDTYPE_NUMERIC, {5}}, /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
        sizeof(Opt),                     /* .memSize */
        UA_DATATYPEKIND_OPTSTRUCT,       /* .typeKind */
        false,                            /* .pointerFree */
        false,                           /* .overlayable (depends on endianness and
                                         the absence of padding) */
        4,                               /* .membersSize */
        Opt_members
};

const UA_DataTypeArray customDataTypesOptStruct = {&customDataTypes, 2, &OptType, UA_FALSE};

typedef struct {
    UA_String description;
    size_t bSize;
    UA_String *b;
    size_t cSize;
    UA_Float *c;
    size_t dSize;
    UA_Float *d;
} OptArray;

static UA_DataTypeMember ArrayOptStruct_members[4] = {
    {
        UA_TYPENAME("Measurement description") /* .memberName */
        &UA_TYPES[UA_TYPES_STRING],            /* .memberType */
        0,                                     /* .padding */
        false,                                 /* .isArray */
        false
    },
    {
        UA_TYPENAME("TestArray1") /* .memberName */
        &UA_TYPES[UA_TYPES_STRING], /* .memberType */
        offsetof(OptArray, bSize) - offsetof(OptArray, description) - sizeof(UA_String),               /* .padding */
        true,                      /* .isArray */
        false
    },
    {
        UA_TYPENAME("TestArray2")  /* .memberName */
        &UA_TYPES[UA_TYPES_FLOAT], /* .memberType */
        offsetof(OptArray, cSize) - offsetof(OptArray, b) - sizeof(void *),               /* .padding */
        true,                      /* .isArray */
        true
    },
    {
        UA_TYPENAME("TestArray3")  /* .memberName */
        &UA_TYPES[UA_TYPES_FLOAT], /* .memberType */
        offsetof(OptArray, dSize) - offsetof(OptArray, c) - sizeof(void *),               /* .padding */
        true,                      /* .isArray */
        false
    }
};

static const UA_DataType ArrayOptType = {
    UA_TYPENAME("OptArray")             /* .tyspeName */
    {1, UA_NODEIDTYPE_NUMERIC, {4243}},     /* .typeId */
    {1, UA_NODEIDTYPE_NUMERIC, {1337}}, /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
    sizeof(OptArray),                   /* .memSize */
    UA_DATATYPEKIND_OPTSTRUCT,       /* .typeKind */
    false,                            /* .pointerFree */
    false,                           /* .overlayable (depends on endianness and
                                         the absence of padding) */
    4,                               /* .membersSize */
    ArrayOptStruct_members
};

const UA_DataTypeArray customDataTypesOptArrayStruct = {&customDataTypesOptStruct, 3, &ArrayOptType, UA_FALSE};

typedef enum {UA_UNISWITCH_NONE = 0, UA_UNISWITCH_OPTIONA = 1, UA_UNISWITCH_OPTIONB = 2} UA_UniSwitch;

typedef struct {
    UA_UniSwitch switchField;
    union {
        UA_Double optionA;
        UA_String optionB;
    } fields;
} Uni;

static UA_DataTypeMember Uni_members[2] = {
        {
                UA_TYPENAME("optionA")
                &UA_TYPES[UA_TYPES_DOUBLE], /* .memberType */
                offsetof(Uni, fields.optionA),
                false,
                false
        },
        {
                UA_TYPENAME("optionB")
                &UA_TYPES[UA_TYPES_STRING], /* .memberType */
                offsetof(Uni, fields.optionB),
                false,
                false
        }
};

static const UA_DataType UniType = {
        UA_TYPENAME("Uni")
        {1, UA_NODEIDTYPE_NUMERIC, {4245}},
        {1, UA_NODEIDTYPE_NUMERIC, {13338}},
        sizeof(Uni),
        UA_DATATYPEKIND_UNION,
        false,
        false,
        2,
        Uni_members
};

const UA_DataTypeArray customDataTypesUnion = {&customDataTypesOptArrayStruct, 2, &UniType, UA_FALSE};

typedef enum {
    UA_SELFCONTAININGUNIONSWITCH_NONE = 0,
    UA_SELFCONTAININGUNIONSWITCH_DOUBLE = 1,
    UA_SELFCONTAININGUNIONSWITCH_ARRAY = 2,
    __UA_SELFCONTAININGUNIONSWITCH_FORCE32BIT = 0x7fffffff
} UA_SelfContainingUnionSwitch;

typedef struct UA_SelfContainingUnion UA_SelfContainingUnion;
struct UA_SelfContainingUnion {
    UA_SelfContainingUnionSwitch switchField;
    union {
        UA_Double _double;
        struct {
            size_t arraySize;
            UA_SelfContainingUnion *array;
        } array;
    } fields;
};

extern const UA_DataType selfContainingUnionType;

static UA_DataTypeMember SelfContainingUnion_members[2] = {
{
    UA_TYPENAME("_double")                            /* .memberName */
    &UA_TYPES[UA_TYPES_DOUBLE],                       /* .memberType */
    offsetof(UA_SelfContainingUnion, fields._double), /* .padding */
    false,                                            /* .isArray */
    false                                             /* .isOptional */
},
{
    UA_TYPENAME("Array")                              /* .memberName */
    &selfContainingUnionType,                         /* .memberType */
    offsetof(UA_SelfContainingUnion, fields.array),   /* .padding */
    true,                                             /* .isArray */
    false                                             /* .isOptional */
},};

const UA_DataType selfContainingUnionType = {
    UA_TYPENAME("SelfContainingStruct") /* .typeName */
    {2, UA_NODEIDTYPE_NUMERIC, {4002LU}}, /* .typeId */
    {2, UA_NODEIDTYPE_NUMERIC, {0}}, /* .binaryEncodingId */
    sizeof(UA_SelfContainingUnion), /* .memSize */
    UA_DATATYPEKIND_UNION, /* .typeKind */
    false, /* .pointerFree */
    false, /* .overlayable */
    2, /* .membersSize */
    SelfContainingUnion_members  /* .members */
};

const UA_DataTypeArray customDataTypesSelfContainingUnion = {NULL, 1, &selfContainingUnionType, UA_FALSE};

START_TEST(UA_PubSub_EnDecode_CustomScalarDeltaFrame) {
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
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypes);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, &PointType);
    ck_assert((*(Point *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).x == p.x);
    ck_assert((*(Point *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).y == p.y);
    ck_assert((*(Point *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).z == p.z);
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

START_TEST(UA_PubSub_EnDecode_CustomScalarKeyFrame) {
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

    Point p;
    p.x = 1.0;
    p.y = 2.0;
    p.z = 3.0;

    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, &p, &PointType);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypes);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, &PointType);
    ck_assert((*(Point *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).x == p.x);
    ck_assert((*(Point *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).y == p.y);
    ck_assert((*(Point *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).z == p.z);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);

    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_CustomScalarExtensionObjectDeltaFrame) {
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

    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);

    eo.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    eo.content.decoded.data = &p;
    eo.content.decoded.type = &PointType;

    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmdf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypes);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, &PointType);
    ck_assert((*(Point *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).x == p.x);
    ck_assert((*(Point *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).y == p.y);
    ck_assert((*(Point *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).z == p.z);
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
} END_TEST

START_TEST(UA_PubSub_EnDecode_CustomScalarExtensionObjectKeyFrame) {
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

    Point p;
    p.x = 1.0;
    p.y = 2.0;
    p.z = 3.0;

    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);

    eo.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    eo.content.decoded.data = &p;
    eo.content.decoded.type = &PointType;

    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value,  &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypes);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, &PointType);
    ck_assert((*(Point *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).x == p.x);
    ck_assert((*(Point *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).y == p.y);
    ck_assert((*(Point *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).z == p.z);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);
    
    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_CustomArrayDeltaFrame){
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

    Point ps[10];
    for(size_t i = 0; i < 10; ++i) {
        ps[i].x = (UA_Float)(1*i);
        ps[i].y = (UA_Float)(2*i);
        ps[i].z = (UA_Float)(3*i);
    }

    UA_Variant_setArrayCopy(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, (void*)ps, 10, &PointType);
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmdf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypes);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    for (size_t i = 0; i < 10; i++) {
        ck_assert(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type == &PointType);
        Point *p2 = &((Point*)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data)[i];

        // we need to cast floats to int to avoid comparison of floats
        // which may result into false results
        ck_assert((int)p2->x == (int)ps[i].x);
        ck_assert((int)p2->y == (int)ps[i].y);
        ck_assert((int)p2->z == (int)ps[i].z);
   }

    UA_DataValue_clear(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(dmdf.data.deltaFrameData.deltaFrameFields);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_CustomArrayKeyFrame){
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

    Point ps[10];
    for(size_t i = 0; i < 10; ++i) {
        ps[i].x = (UA_Float)(1*i);
        ps[i].y = (UA_Float)(2*i);
        ps[i].z = (UA_Float)(3*i);
    }

    UA_Variant_setArrayCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, (void*)ps, 10, &PointType);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypes);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    for (size_t i = 0; i < 10; i++) {
        ck_assert(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type == &PointType);
        Point *p2 = &((Point*)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data)[i];

        // we need to cast floats to int to avoid comparison of floats
        // which may result into false results
        ck_assert((int)p2->x == (int)ps[i].x);
        ck_assert((int)p2->y == (int)ps[i].y);
        ck_assert((int)p2->z == (int)ps[i].z);
   }

    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    }
END_TEST

START_TEST(UA_PubSub_EnDecode_CustomStructureWithOptionalFieldsDeltaFrame){
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
    
    Opt o;
    memset(&o, 0, sizeof(Opt));
    o.a = 3;
    o.c = UA_Float_new();
    *o.c = (UA_Float) 10.10;
    o.d = UA_String_new();
    *o.d = UA_STRING_ALLOC("Test");

    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &o, &OptType);
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmdf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypesOptStruct);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, &OptType);
    ck_assert_int_eq((*(Opt *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).a, o.a);
    ck_assert_ptr_eq((*(Opt *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).b, NULL);
    ck_assert(*(*(Opt *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).c == *o.c);
    UA_String testStr = UA_STRING("Test");
    ck_assert(UA_String_equal(((Opt *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data)->d, &testStr));
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    UA_clear(&o, &OptType);
    UA_DataValue_clear(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(dmdf.data.deltaFrameData.deltaFrameFields);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_CustomStructureWithOptionalFieldsKeyFrame){
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

    Opt o;
    memset(&o, 0, sizeof(Opt));
    o.a = 3;
    o.c = UA_Float_new();
    *o.c = (UA_Float) 10.10;
    o.d = UA_String_new();
    *o.d = UA_STRING_ALLOC("Test");

    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, &o, &OptType);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypesOptStruct);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, &OptType);
    ck_assert_int_eq((*(Opt *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).a, o.a);
    ck_assert_ptr_eq((*(Opt *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).b, NULL);
    ck_assert(*(*(Opt *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).c == *o.c);
    UA_String testStr = UA_STRING("Test");
    ck_assert(UA_String_equal(((Opt *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data)->d, &testStr));
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);

    UA_clear(&o, &OptType);
    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_CustomUnionDeltaFrame){
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

    Uni u;
    u.switchField = UA_UNISWITCH_OPTIONB;
    u.fields.optionB = UA_STRING("test string");

    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &u, &UniType);
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmdf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypesUnion);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, &UniType);
    ck_assert_int_eq((*(Uni *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).switchField, UA_UNISWITCH_OPTIONB);
    UA_String testStr = UA_STRING("test string");
    ck_assert(UA_String_equal(&((Uni *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data)->fields.optionB, &testStr));
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

START_TEST(UA_PubSub_EnDecode_CustomUnionKeyFrame){
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

    Uni u;
    u.switchField = UA_UNISWITCH_OPTIONB;
    u.fields.optionB = UA_STRING("test string");

    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, &u, &UniType);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypesUnion);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, &UniType);
    ck_assert_int_eq((*(Uni *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).switchField, UA_UNISWITCH_OPTIONB);
    UA_String testStr = UA_STRING("test string");
    ck_assert(UA_String_equal(&((Uni *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data)->fields.optionB, &testStr));
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);

    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_SelfContainingUnionNormalMemberDeltaFrame){
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

    UA_SelfContainingUnion s;
    s.switchField = UA_SELFCONTAININGUNIONSWITCH_DOUBLE;
    s.fields._double = 42.0;

    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &s, &selfContainingUnionType);
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmdf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypesSelfContainingUnion);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, &selfContainingUnionType);
    ck_assert_int_eq((*(UA_SelfContainingUnion *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).switchField, UA_SELFCONTAININGUNIONSWITCH_DOUBLE);
    ck_assert(fabs(((UA_SelfContainingUnion *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data)->fields._double - 42.0) < 0.005);
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

START_TEST(UA_PubSub_EnDecode_SelfContainingUnionNormalMemberKeyFrame){
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

    UA_SelfContainingUnion s;
    s.switchField = UA_SELFCONTAININGUNIONSWITCH_DOUBLE;
    s.fields._double = 42.0;

    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value,  &s, &selfContainingUnionType);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypesSelfContainingUnion);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, &selfContainingUnionType);
    ck_assert_int_eq((*(UA_SelfContainingUnion *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).switchField, UA_SELFCONTAININGUNIONSWITCH_DOUBLE);
    ck_assert(fabs(((UA_SelfContainingUnion *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data)->fields._double - 42.0) < 0.005);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);
    
    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_SelfContainingUnionSelfMemberDeltaFrame){
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

    UA_SelfContainingUnion s;
    s.switchField = UA_SELFCONTAININGUNIONSWITCH_ARRAY;
    s.fields.array.arraySize = 2;
    s.fields.array.array = (UA_SelfContainingUnion *)UA_calloc(2, sizeof(UA_SelfContainingUnion));
    s.fields.array.array[0].switchField = UA_SELFCONTAININGUNIONSWITCH_DOUBLE;
    s.fields.array.array[0].fields._double = 23.0;
    s.fields.array.array[1].switchField = UA_SELFCONTAININGUNIONSWITCH_DOUBLE;
    s.fields.array.array[1].fields._double = 42.0;

    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &s, &selfContainingUnionType);
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue = true;
    UA_free(s.fields.array.array);

    m.payload.dataSetPayload.dataSetMessages = &dmdf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypesSelfContainingUnion);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, &selfContainingUnionType);
    ck_assert((*(UA_SelfContainingUnion *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).fields.array.arraySize == 2);
    ck_assert_int_eq((*(UA_SelfContainingUnion *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).fields.array.array[0].switchField, UA_SELFCONTAININGUNIONSWITCH_DOUBLE);
    ck_assert(fabs(((UA_SelfContainingUnion *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data)->fields.array.array[0].fields._double - 23.0) < 0.005);
    ck_assert_int_eq((*(UA_SelfContainingUnion *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).fields.array.array[1].switchField, UA_SELFCONTAININGUNIONSWITCH_DOUBLE);
    ck_assert(fabs(((UA_SelfContainingUnion *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data)->fields.array.array[1].fields._double - 42.0) < 0.005);
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

START_TEST(UA_PubSub_EnDecode_SelfContainingUnionSelfMemberKeyFrame){
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

    UA_SelfContainingUnion s;
    s.switchField = UA_SELFCONTAININGUNIONSWITCH_ARRAY;
    s.fields.array.arraySize = 2;
    s.fields.array.array = (UA_SelfContainingUnion *)UA_calloc(2, sizeof(UA_SelfContainingUnion));
    s.fields.array.array[0].switchField = UA_SELFCONTAININGUNIONSWITCH_DOUBLE;
    s.fields.array.array[0].fields._double = 23.0;
    s.fields.array.array[1].switchField = UA_SELFCONTAININGUNIONSWITCH_DOUBLE;
    s.fields.array.array[1].fields._double = 42.0;

    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, &s, &selfContainingUnionType);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;
    UA_free(s.fields.array.array);

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypesSelfContainingUnion);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, &selfContainingUnionType);
    ck_assert((*(UA_SelfContainingUnion *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).fields.array.arraySize == 2);
    ck_assert_int_eq((*(UA_SelfContainingUnion *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).fields.array.array[0].switchField, UA_SELFCONTAININGUNIONSWITCH_DOUBLE);
    ck_assert(fabs(((UA_SelfContainingUnion *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data)->fields.array.array[0].fields._double - 23.0) < 0.005);
    ck_assert_int_eq((*(UA_SelfContainingUnion *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).fields.array.array[1].switchField, UA_SELFCONTAININGUNIONSWITCH_DOUBLE);
    ck_assert(fabs(((UA_SelfContainingUnion *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data)->fields.array.array[1].fields._double - 42.0) < 0.005);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);

    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_CustomStructureWithOptionalFieldsWithArrayNotContainedDeltaFrame){
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

    OptArray oa;
    oa.description = UA_STRING_ALLOC("TestDesc");
    oa.bSize = 1;
    oa.b = (UA_String *)UA_Array_new(oa.bSize, &UA_TYPES[UA_TYPES_STRING]);
    oa.b[0] = UA_STRING_ALLOC("Test");
    oa.c = NULL;
    oa.dSize = 3;
    oa.d = (UA_Float *) UA_Array_new(oa.dSize, &UA_TYPES[UA_TYPES_FLOAT]);
    oa.d[0] = (UA_Float)1.1;
    oa.d[1] = (UA_Float)1.2;
    oa.d[2] = (UA_Float)1.3;

    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &oa, &ArrayOptType);
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmdf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypesOptArrayStruct);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, &ArrayOptType);
    UA_String testStr = UA_STRING("TestDesc");
    ck_assert(UA_String_equal(&(*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).description, &testStr));
    ck_assert_uint_eq((*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).bSize, 1);
    ck_assert_uint_eq((*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).cSize, 0);
    ck_assert_uint_eq((*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).dSize, 3);
    ck_assert_ptr_eq((*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).c, NULL);
    UA_String testStr2 = UA_STRING("Test");
    ck_assert(UA_String_equal(&(*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).b[0], &testStr2));
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data)->d[0] - 1.1) < 0.005);
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data)->d[1] - 1.2) < 0.005);
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data)->d[2] - 1.3) < 0.005);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    UA_clear(&oa, &ArrayOptType);
    UA_DataValue_clear(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(dmdf.data.deltaFrameData.deltaFrameFields);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_CustomStructureWithOptionalFieldsWithArrayNotContainedKeyFrame){
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

    OptArray oa;
    oa.description = UA_STRING_ALLOC("TestDesc");
    oa.bSize = 1;
    oa.b = (UA_String *)UA_Array_new(oa.bSize, &UA_TYPES[UA_TYPES_STRING]);
    oa.b[0] = UA_STRING_ALLOC("Test");
    oa.c = NULL;
    oa.dSize = 3;
    oa.d = (UA_Float *) UA_Array_new(oa.dSize, &UA_TYPES[UA_TYPES_FLOAT]);
    oa.d[0] = (UA_Float)1.1;
    oa.d[1] = (UA_Float)1.2;
    oa.d[2] = (UA_Float)1.3;

    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, &oa, &ArrayOptType);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypesOptArrayStruct);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, &ArrayOptType);
    UA_String testStr = UA_STRING("TestDesc");
    ck_assert(UA_String_equal(&(*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).description, &testStr));
    ck_assert_uint_eq((*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).bSize, 1);
    ck_assert_uint_eq((*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).cSize, 0);
    ck_assert_uint_eq((*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).dSize, 3);
    ck_assert_ptr_eq((*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).c, NULL);
    UA_String testStr2 = UA_STRING("Test");
    ck_assert(UA_String_equal(&(*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).b[0], &testStr2));
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data)->d[0] - 1.1) < 0.005);
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data)->d[1] - 1.2) < 0.005);
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data)->d[2] - 1.3) < 0.005);
    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);

    UA_clear(&oa, &ArrayOptType);
    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_CustomStructureWithOptionalFieldsWithArrayContainedDeltaFrame){
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

    OptArray oa;
    oa.description = UA_STRING_ALLOC("TestDesc");
    oa.bSize = 1;
    oa.b = (UA_String *)UA_Array_new(oa.bSize, &UA_TYPES[UA_TYPES_STRING]);
    oa.b[0] = UA_STRING_ALLOC("Test");
    oa.cSize = 3;
    oa.c = (UA_Float *) UA_Array_new(oa.cSize, &UA_TYPES[UA_TYPES_FLOAT]);
    oa.c[0] = (UA_Float)1.1;
    oa.c[1] = (UA_Float)1.2;
    oa.c[2] = (UA_Float)1.3;
    oa.dSize = 3;
    oa.d = (UA_Float *)UA_Array_new(oa.dSize, &UA_TYPES[UA_TYPES_FLOAT]);
    oa.d[0] = (UA_Float)2.1;
    oa.d[1] = (UA_Float)2.2;
    oa.d[2] = (UA_Float)2.3;

    UA_Variant_setScalarCopy(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.value, &oa, &ArrayOptType);
    dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue.hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmdf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypesOptArrayStruct);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.type, &ArrayOptType);

    UA_String testStr = UA_STRING("TestDesc");
    ck_assert(UA_String_equal(&(*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).description, &testStr));
    ck_assert_uint_eq((*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).bSize, 1);
    ck_assert_uint_eq((*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).cSize, 3);
    ck_assert_uint_eq((*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).dSize, 3);
    UA_String testStr2 = UA_STRING("Test");
    ck_assert(UA_String_equal(&(*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data).b[0], &testStr2));
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data)->c[0] - 1.1) < 0.005);
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data)->c[1] - 1.2) < 0.005);
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data)->c[2] - 1.3) < 0.005);
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data)->d[0] - 2.1) < 0.005);
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data)->d[1] - 2.2) < 0.005);
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.value.data)->d[2] - 2.3) < 0.005);

    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.deltaFrameData.deltaFrameFields[0].fieldValue.hasSourceTimestamp);
    ck_assert(m.dataSetClassIdEnabled == m2.dataSetClassIdEnabled);
    ck_assert(m.groupHeaderEnabled == m2.groupHeaderEnabled);
    ck_assert(m.picosecondsEnabled == m2.picosecondsEnabled);
    ck_assert(m.promotedFieldsEnabled == m2.promotedFieldsEnabled);
    ck_assert(m.publisherIdEnabled == m2.publisherIdEnabled);
    ck_assert(m.securityEnabled == m2.securityEnabled);
    ck_assert(m.chunkMessage == m2.chunkMessage);

    UA_clear(&oa, &ArrayOptType);
    UA_DataValue_clear(&dmdf.data.deltaFrameData.deltaFrameFields[0].fieldValue);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_free(dmdf.data.deltaFrameData.deltaFrameFields);
}
END_TEST

START_TEST(UA_PubSub_EnDecode_CustomStructureWithOptionalFieldsWithArrayContainedKeyFrame){
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

    OptArray oa;
    oa.description = UA_STRING_ALLOC("TestDesc");
    oa.bSize = 1;
    oa.b = (UA_String *)UA_Array_new(oa.bSize, &UA_TYPES[UA_TYPES_STRING]);
    oa.b[0] = UA_STRING_ALLOC("Test");
    oa.cSize = 3;
    oa.c = (UA_Float *) UA_Array_new(oa.cSize, &UA_TYPES[UA_TYPES_FLOAT]);
    oa.c[0] = (UA_Float)1.1;
    oa.c[1] = (UA_Float)1.2;
    oa.c[2] = (UA_Float)1.3;
    oa.dSize = 3;
    oa.d = (UA_Float *)UA_Array_new(oa.dSize, &UA_TYPES[UA_TYPES_FLOAT]);
    oa.d[0] = (UA_Float)2.1;
    oa.d[1] = (UA_Float)2.2;
    oa.d[2] = (UA_Float)2.3;

    UA_Variant_setScalarCopy(&dmkf.data.keyFrameData.dataSetFields[0].value, &oa, &ArrayOptType);
    dmkf.data.keyFrameData.dataSetFields[0].hasValue = true;

    m.payload.dataSetPayload.dataSetMessages = &dmkf;

    UA_StatusCode rv = UA_STATUSCODE_UNCERTAININITIALVALUE;
    UA_ByteString buffer;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&m);
    rv = UA_ByteString_allocBuffer(&buffer, msgSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
   
    rv = UA_NetworkMessage_encodeBinary(&m, &buffer);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_NetworkMessage m2;
    memset(&m2, 0, sizeof(UA_NetworkMessage));
    rv = UA_NetworkMessage_decodeBinary(&buffer, &m2, &customDataTypesOptArrayStruct);

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
    ck_assert_ptr_eq(m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type, &ArrayOptType);

    UA_String testStr = UA_STRING("TestDesc");
    ck_assert(UA_String_equal(&(*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).description,  &testStr));
    ck_assert_uint_eq((*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).bSize, 1);
    ck_assert_uint_eq((*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).cSize, 3);
    ck_assert_uint_eq((*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).dSize, 3);
    UA_String testStr2 = UA_STRING("Test");
    ck_assert(UA_String_equal(&(*(OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data).b[0], &testStr2));
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data)->c[0] - 1.1) < 0.005);
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data)->c[1] - 1.2) < 0.005);
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data)->c[2] - 1.3) < 0.005);
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data)->d[0] - 2.1) < 0.005);
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data)->d[1] - 2.2) < 0.005);
    ck_assert(fabs(((OptArray *)m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.data)->d[2] - 2.3) < 0.005);

    ck_assert(m.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp == m2.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].hasSourceTimestamp);

    UA_clear(&oa, &ArrayOptType);
    UA_DataValue_clear(&dmkf.data.keyFrameData.dataSetFields[0]);
    UA_NetworkMessage_clear(&m2);
    UA_ByteString_clear(&buffer);
    UA_Array_delete(dmkf.data.keyFrameData.dataSetFields, dmkf.data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
}
END_TEST

int main(void) {

    TCase *tc_encode = tcase_create("encode_decode_CustomType");

    tcase_add_test(tc_encode, UA_PubSub_EnDecode_CustomScalarDeltaFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_CustomScalarKeyFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_CustomScalarExtensionObjectDeltaFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_CustomScalarExtensionObjectKeyFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_CustomArrayDeltaFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_CustomArrayKeyFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_CustomStructureWithOptionalFieldsDeltaFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_CustomStructureWithOptionalFieldsKeyFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_CustomUnionDeltaFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_CustomUnionKeyFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_SelfContainingUnionNormalMemberDeltaFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_SelfContainingUnionNormalMemberKeyFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_SelfContainingUnionSelfMemberDeltaFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_SelfContainingUnionSelfMemberKeyFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_CustomStructureWithOptionalFieldsWithArrayNotContainedDeltaFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_CustomStructureWithOptionalFieldsWithArrayNotContainedKeyFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_CustomStructureWithOptionalFieldsWithArrayContainedDeltaFrame);
    tcase_add_test(tc_encode, UA_PubSub_EnDecode_CustomStructureWithOptionalFieldsWithArrayContainedKeyFrame);

    Suite *s = suite_create("PubSub Custom Types EnDecode");
    suite_add_tcase(s, tc_encode);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
