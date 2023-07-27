/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <open62541/types_generated_handling.h>

#include "ua_types_encoding_binary.h"

#include <stdlib.h>
#include <check.h>
#include <math.h>

#ifdef __clang__
//required for ck_assert_ptr_eq and const casting
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#endif

/* The standard-defined datatypes are stored in the global array UA_TYPES. User
 * can create their own UA_CUSTOM_TYPES array (the name doesn't matter) and
 * provide it to the server / client. The type will be automatically decoded if
 * possible.
 */

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

START_TEST(parseCustomScalar) {
    Point p;
    p.x = 1.0;
    p.y = 2.0;
    p.z = 3.0;

    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, &p, &PointType);

    size_t buflen = UA_calcSizeBinary(&var, &UA_TYPES[UA_TYPES_VARIANT]);
    UA_ByteString buf;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&buf, buflen);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_Byte *pos = buf.data;
    const UA_Byte *end = &buf.data[buf.length];
    retval = UA_encodeBinaryInternal(&var, &UA_TYPES[UA_TYPES_VARIANT],
                                     &pos, &end, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant var2;
    size_t offset = 0;
    retval = UA_decodeBinaryInternal(&buf, &offset, &var2, &UA_TYPES[UA_TYPES_VARIANT], &customDataTypes);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var2.type == &PointType);

    Point *p2 = (Point*)var2.data;
    ck_assert(p.x == p2->x);

    UA_Variant_clear(&var2);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(parseCustomScalarExtensionObject) {
    Point p;
    p.x = 1.0;
    p.y = 2.0;
    p.z = 3.0;

    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);

    eo.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    eo.content.decoded.data = &p;
    eo.content.decoded.type = &PointType;

    size_t buflen = UA_calcSizeBinary(&eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    UA_ByteString buf;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&buf, buflen);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buf.data;
    const UA_Byte *bufEnd = &buf.data[buf.length];
    retval = UA_encodeBinaryInternal(&eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &bufPos, &bufEnd, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_ExtensionObject eo2;
    size_t offset = 0;
    retval = UA_decodeBinaryInternal(&buf, &offset, &eo2, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &customDataTypes);
    ck_assert_uint_eq(offset, (uintptr_t)(bufPos - buf.data));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(eo2.encoding, UA_EXTENSIONOBJECT_DECODED);
    ck_assert(eo2.content.decoded.type == &PointType);

    Point *p2 = (Point*)eo2.content.decoded.data;
    ck_assert(p.x == p2->x);

    UA_ExtensionObject_clear(&eo2);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(parseCustomArray) {
    Point ps[10];
    for(size_t i = 0; i < 10; ++i) {
        ps[i].x = (UA_Float)(1*i);
        ps[i].y = (UA_Float)(2*i);
        ps[i].z = (UA_Float)(3*i);
    }

    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setArray(&var, (void*)ps, 10, &PointType);

    size_t buflen = UA_calcSizeBinary(&var, &UA_TYPES[UA_TYPES_VARIANT]);
    UA_ByteString buf;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&buf, buflen);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_Byte *pos = buf.data;
    const UA_Byte *end = &buf.data[buf.length];
    retval = UA_encodeBinaryInternal(&var, &UA_TYPES[UA_TYPES_VARIANT],
                             &pos, &end, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant var2;
    size_t offset = 0;
    retval = UA_decodeBinaryInternal(&buf, &offset, &var2, &UA_TYPES[UA_TYPES_VARIANT], &customDataTypes);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var2.type == &PointType);
    ck_assert_uint_eq(var2.arrayLength, 10);

    for (size_t i = 0; i < 10; i++) {
        Point *p2 = &((Point*)var2.data)[i];

        // we need to cast floats to int to avoid comparison of floats
        // which may result into false results
        ck_assert((int)p2->x == (int)ps[i].x);
        ck_assert((int)p2->y == (int)ps[i].y);
        ck_assert((int)p2->z == (int)ps[i].z);
    }

    UA_Variant_clear(&var2);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(parseCustomStructureWithOptionalFields) {
        Opt o;
        memset(&o, 0, sizeof(Opt));
        o.a = 3;
        o.b = NULL;
        o.c = UA_Float_new();
        *o.c = (UA_Float) 10.10;
        o.d = UA_String_new();
        *o.d = UA_STRING_ALLOC("Test");

        UA_Variant var;
        UA_Variant_init(&var);
        UA_Variant_setScalarCopy(&var, &o, &OptType);

        size_t buflen = UA_calcSizeBinary(&var, &UA_TYPES[UA_TYPES_VARIANT]);
        UA_ByteString buf;
        UA_StatusCode retval = UA_ByteString_allocBuffer(&buf, buflen);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        UA_Byte *pos = buf.data;
        const UA_Byte *end = &buf.data[buf.length];
        retval = UA_encodeBinaryInternal(&var, &UA_TYPES[UA_TYPES_VARIANT],
                                 &pos, &end, NULL, NULL);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        UA_Variant var2;
        size_t offset = 0;
        retval = UA_decodeBinaryInternal(&buf, &offset, &var2, &UA_TYPES[UA_TYPES_VARIANT], &customDataTypesOptStruct);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert(var2.type == &OptType);
        Opt *optStruct2 = (Opt *) var2.data;
        ck_assert(optStruct2->a == 3);

        UA_clear(&o, &OptType);
        UA_Variant_clear(&var);
        UA_Variant_clear(&var2);
        UA_ByteString_clear(&buf);
} END_TEST

START_TEST(parseCustomStructureWithOptionalFieldsWithArrayNotContained) {
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

        UA_StatusCode retval;
        UA_Variant var;
        UA_Variant_init(&var);
        retval = UA_Variant_setScalarCopy(&var, &oa, &ArrayOptType);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        size_t buflen = UA_calcSizeBinary(&var, &UA_TYPES[UA_TYPES_VARIANT]);
        UA_ByteString buf;
        retval = UA_ByteString_allocBuffer(&buf, buflen);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        size_t binSize = UA_calcSizeBinary(&oa, &ArrayOptType);
        ck_assert_uint_eq(binSize, 44);
        UA_Byte *pos = buf.data;
        const UA_Byte *end = &buf.data[buf.length];
        retval = UA_encodeBinaryInternal(&var, &UA_TYPES[UA_TYPES_VARIANT],
                                 &pos, &end, NULL, NULL);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        UA_Variant var2;
        size_t offset = 0;
        retval = UA_decodeBinaryInternal(&buf, &offset, &var2, &UA_TYPES[UA_TYPES_VARIANT], &customDataTypesOptArrayStruct);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert(var2.type == &ArrayOptType);

        OptArray *optStruct2 = (OptArray *) var2.data;
        UA_String compare = UA_STRING("TestDesc");
        ck_assert(UA_String_equal(&optStruct2->description, &compare));
        ck_assert(optStruct2->bSize == 1);
        compare = UA_STRING("Test");
        ck_assert(UA_String_equal(&optStruct2->b[0], &compare));
        ck_assert(optStruct2->cSize == 0);
        ck_assert(optStruct2->c == NULL);
        ck_assert(optStruct2->dSize == 3);
        ck_assert((fabs(optStruct2->d[0] - 1.1)) < 0.005);
        ck_assert((fabs(optStruct2->d[1] - 1.2)) < 0.005);
        ck_assert((fabs(optStruct2->d[2] - 1.3)) < 0.005);

        UA_clear(&oa, &ArrayOptType);
        UA_Variant_clear(&var);
        UA_Variant_clear(&var2);
        UA_ByteString_clear(&buf);
} END_TEST

START_TEST(parseCustomStructureWithOptionalFieldsWithArrayContained) {
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

        UA_StatusCode retval;
        UA_Variant var;
        UA_Variant_init(&var);
        retval = UA_Variant_setScalarCopy(&var, &oa, &ArrayOptType);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        size_t buflen = UA_calcSizeBinary(&var, &UA_TYPES[UA_TYPES_VARIANT]);
        UA_ByteString buf;
        retval = UA_ByteString_allocBuffer(&buf, buflen);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        size_t binSize = UA_calcSizeBinary(&oa, &ArrayOptType);
        ck_assert_uint_eq(binSize, 60);
        UA_Byte *pos = buf.data;
        const UA_Byte *end = &buf.data[buf.length];
        retval = UA_encodeBinaryInternal(&var, &UA_TYPES[UA_TYPES_VARIANT],
                                 &pos, &end, NULL, NULL);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        UA_Variant var2;
        size_t offset = 0;
        retval = UA_decodeBinaryInternal(&buf, &offset, &var2, &UA_TYPES[UA_TYPES_VARIANT], &customDataTypesOptArrayStruct);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert(var2.type == &ArrayOptType);

        OptArray *optStruct2 = (OptArray *) var2.data;
        UA_String compare = UA_STRING("TestDesc");
        ck_assert(UA_String_equal(&optStruct2->description, &compare));
        ck_assert(optStruct2->bSize == 1);
        compare = UA_STRING("Test");
        ck_assert(UA_String_equal(&optStruct2->b[0], &compare));
        ck_assert(optStruct2->cSize == 3);
        ck_assert((fabs(optStruct2->c[0] - 1.1)) < 0.005);
        ck_assert((fabs(optStruct2->c[1] - 1.2)) < 0.005);
        ck_assert((fabs(optStruct2->c[2] - 1.3)) < 0.005);
        ck_assert(optStruct2->dSize == 3);
        ck_assert((fabs(optStruct2->d[0] - 2.1)) < 0.005);
        ck_assert((fabs(optStruct2->d[1] - 2.2)) < 0.005);
        ck_assert((fabs(optStruct2->d[2] - 2.3)) < 0.005);

        UA_clear(&oa, &ArrayOptType);
        UA_Variant_clear(&var);
        UA_Variant_clear(&var2);
        UA_ByteString_clear(&buf);
    } END_TEST

START_TEST(parseCustomUnion) {
        UA_StatusCode retval;
        Uni u;
        u.switchField = UA_UNISWITCH_OPTIONB;
        u.fields.optionB = UA_STRING("test string");

        UA_Variant var;
        UA_Variant_init(&var);
        retval = UA_Variant_setScalarCopy(&var, &u, &UniType);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        size_t lengthOfUnion = UA_calcSizeBinary(&u, &UniType);
        //check if 19 is the right size
        ck_assert_uint_eq(lengthOfUnion, 19);

        size_t buflen = UA_calcSizeBinary(&var, &UA_TYPES[UA_TYPES_VARIANT]);
        UA_ByteString buf;
        retval = UA_ByteString_allocBuffer(&buf, buflen);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        UA_Byte *pos = buf.data;
        const UA_Byte *end = &buf.data[buf.length];
        retval = UA_encodeBinaryInternal(&var, &UA_TYPES[UA_TYPES_VARIANT],
                                 &pos, &end, NULL, NULL);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        UA_Variant var2;
        size_t offset = 0;
        retval = UA_decodeBinaryInternal(&buf, &offset, &var2, &UA_TYPES[UA_TYPES_VARIANT], &customDataTypesUnion);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert(var2.type == &UniType);

        Uni *uni2 = (Uni *) var2.data;
        ck_assert(uni2->switchField = UA_UNISWITCH_OPTIONB);
        UA_String compare = UA_STRING("test string");
        ck_assert(UA_String_equal(&u.fields.optionB, &compare));
        //ck_assert(uni2->fields.optionA = (fabs(uni2->fields.optionA - 2.5)) < 0.005);

        UA_Variant_clear(&var);
        UA_Variant_clear(&var2);
        UA_ByteString_clear(&buf);
    } END_TEST

START_TEST(parseSelfContainingUnionNormalMember) {
        UA_StatusCode retval;
        UA_SelfContainingUnion s;
        s.switchField = UA_SELFCONTAININGUNIONSWITCH_DOUBLE;
        s.fields._double = 42.0;

        UA_Variant var;
        UA_Variant_init(&var);
        retval = UA_Variant_setScalarCopy(&var, &s, &selfContainingUnionType);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        size_t lengthOfUnion = UA_calcSizeBinary(&s, &selfContainingUnionType);
        //check if 12 is the right size
        ck_assert_uint_eq(lengthOfUnion, 12);

        size_t buflen = UA_calcSizeBinary(&var, &UA_TYPES[UA_TYPES_VARIANT]);
        UA_ByteString buf;
        retval = UA_ByteString_allocBuffer(&buf, buflen);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        UA_Byte *pos = buf.data;
        const UA_Byte *end = &buf.data[buf.length];
        retval = UA_encodeBinaryInternal(&var, &UA_TYPES[UA_TYPES_VARIANT],
                                 &pos, &end, NULL, NULL);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        UA_Variant var2;
        size_t offset = 0;
        retval = UA_decodeBinaryInternal(&buf, &offset, &var2, &UA_TYPES[UA_TYPES_VARIANT], &customDataTypesSelfContainingUnion);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert(var2.type == &selfContainingUnionType);

        UA_SelfContainingUnion *s2 = (UA_SelfContainingUnion *) var2.data;
        ck_assert(s2->switchField = UA_SELFCONTAININGUNIONSWITCH_DOUBLE);
        ck_assert(fabs(s2->fields._double - 42.0) < 0.005);

        UA_Variant_clear(&var);
        UA_Variant_clear(&var2);
        UA_ByteString_clear(&buf);
    } END_TEST

START_TEST(parseSelfContainingUnionSelfMember) {
        UA_StatusCode retval;
        UA_SelfContainingUnion s;
        s.switchField = UA_SELFCONTAININGUNIONSWITCH_ARRAY;
        s.fields.array.arraySize = 2;
        s.fields.array.array = (UA_SelfContainingUnion *)UA_calloc(2, sizeof(UA_SelfContainingUnion));
        s.fields.array.array[0].switchField = UA_SELFCONTAININGUNIONSWITCH_DOUBLE;
        s.fields.array.array[0].fields._double = 23.0;
        s.fields.array.array[1].switchField = UA_SELFCONTAININGUNIONSWITCH_DOUBLE;
        s.fields.array.array[1].fields._double = 42.0;

        UA_Variant var;
        UA_Variant_init(&var);
        retval = UA_Variant_setScalarCopy(&var, &s, &selfContainingUnionType);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        size_t lengthOfUnion = UA_calcSizeBinary(&s, &selfContainingUnionType);
        //check if 32 is the right size
        ck_assert_uint_eq(lengthOfUnion, 32);

        UA_free(s.fields.array.array);

        size_t buflen = UA_calcSizeBinary(&var, &UA_TYPES[UA_TYPES_VARIANT]);
        UA_ByteString buf;
        retval = UA_ByteString_allocBuffer(&buf, buflen);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        UA_Byte *pos = buf.data;
        const UA_Byte *end = &buf.data[buf.length];
        retval = UA_encodeBinaryInternal(&var, &UA_TYPES[UA_TYPES_VARIANT],
                                 &pos, &end, NULL, NULL);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        UA_Variant var2;
        size_t offset = 0;
        retval = UA_decodeBinaryInternal(&buf, &offset, &var2, &UA_TYPES[UA_TYPES_VARIANT], &customDataTypesSelfContainingUnion);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert(var2.type == &selfContainingUnionType);

        UA_SelfContainingUnion *s2 = (UA_SelfContainingUnion *) var2.data;
        ck_assert(s2->switchField = UA_SELFCONTAININGUNIONSWITCH_ARRAY);
        ck_assert(s2->fields.array.arraySize == 2);
        ck_assert(s2->fields.array.array[0].switchField == UA_SELFCONTAININGUNIONSWITCH_DOUBLE);
        ck_assert(fabs(s2->fields.array.array[0].fields._double - 23.0) < 0.005);
        ck_assert(s2->fields.array.array[1].switchField == UA_SELFCONTAININGUNIONSWITCH_DOUBLE);
        ck_assert(fabs(s2->fields.array.array[1].fields._double - 42.0) < 0.005);

        UA_Variant_clear(&var);
        UA_Variant_clear(&var2);
        UA_ByteString_clear(&buf);
    } END_TEST

int main(void) {
    Suite *s  = suite_create("Test Custom DataType Encoding");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, parseCustomScalar);
    tcase_add_test(tc, parseCustomScalarExtensionObject);
    tcase_add_test(tc, parseCustomArray);
    tcase_add_test(tc, parseCustomStructureWithOptionalFields);
    tcase_add_test(tc, parseCustomUnion);
    tcase_add_test(tc, parseSelfContainingUnionNormalMember);
    tcase_add_test(tc, parseSelfContainingUnionSelfMember);
    tcase_add_test(tc, parseCustomStructureWithOptionalFieldsWithArrayNotContained);
    tcase_add_test(tc, parseCustomStructureWithOptionalFieldsWithArrayContained);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
