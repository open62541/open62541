/* This Source Code Form is subject to the terms of the Mozilla Public 
 * License, v. 2.0. If a copy of the MPL was not distributed with this 
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <open62541/types_generated_handling.h>

#include "ua_types_encoding_binary.h"

#include "check.h"
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
        UA_TYPENAME("x") /* .memberName */
        UA_TYPES_FLOAT,  /* .memberTypeIndex, points into UA_TYPES since
                            .namespaceZero is true */
        0,               /* .padding */
        true,            /* .namespaceZero, see .memberTypeIndex */
        false,            /* .isArray */
        false           /* .isOptional*/
    },

    /* y */
    {
        UA_TYPENAME("y")
        UA_TYPES_FLOAT, padding_y, true, false, false
    },

    /* z */
    {
        UA_TYPENAME("z")
        UA_TYPES_FLOAT, padding_z, true, false, false
    }
};

static const UA_DataType PointType = {
    UA_TYPENAME("Point")             /* .typeName */
    {1, UA_NODEIDTYPE_NUMERIC, {1}}, /* .typeId */
    sizeof(Point),                   /* .memSize */
    0,                               /* .typeIndex, in the array of custom types */
    UA_DATATYPEKIND_STRUCTURE,       /* .typeKind */
    true,                            /* .pointerFree */
    false,                           /* .overlayable (depends on endianness and
                                         the absence of padding) */
    3,                               /* .membersSize */
    0,                               /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
    members
};

const UA_DataTypeArray customDataTypes = {NULL, 1, &PointType};

/* The custom example datatype for an structure with optional fields */
//typedef struct {
//    UA_Boolean hasB;    /* defining if optional field "b" is defined or not */
//    UA_Int16 a;
//    UA_Float b;
//} Opt;

typedef struct {
    UA_Int16 a;
    UA_Float *b;
    UA_Float *c;
} Opt;

/* flag "hasB" does not count as a member */
static UA_DataTypeMember Opt_members[3] = {
        /* a */
        {
                UA_TYPENAME("a") /* .memberName */
                UA_TYPES_INT16,  /* .memberTypeIndex, points into UA_TYPES since namespaceZero is true */
                0, /* .padding */
                true,       /* .namespaceZero, see .memberTypeIndex */
                false,      /* .isArray */
                false       /* .isOptional */
        },
        /* b */
        {
                UA_TYPENAME("b")
                UA_TYPES_FLOAT,
                offsetof(Opt,b) - offsetof(Opt,a) - sizeof(UA_Int16),
                true,
                false,
                true        /* b is an optional field */
        },
        /* c */
        {
                UA_TYPENAME("c")
                UA_TYPES_FLOAT,
                offsetof(Opt,c) - offsetof(Opt,b) - sizeof(void *),
                true,
                false,
                true        /* b is an optional field */
        }
};

static const UA_DataType OptType = {
        UA_TYPENAME("Opt")             /* .typeName */
        {1, UA_NODEIDTYPE_NUMERIC, {4242}}, /* .typeId */
        sizeof(Opt),                   /* .memSize */
        0,                               /* .typeIndex, in the array of custom types */
        UA_DATATYPEKIND_OPTSTRUCT,       /* .typeKind */
        false,                            /* .pointerFree */
        false,                           /* .overlayable (depends on endianness and
                                         the absence of padding) */
        3,                               /* .membersSize */
        0,                               /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
        Opt_members
};

const UA_DataTypeArray customDataTypesOptStruct = {&customDataTypes, 2, &OptType};

typedef enum {UA_UNISWITCH_OPTIONA = 0, UA_UNISWITCH_OPTIONB = 1} UniSwitch;

typedef struct {
    UniSwitch switchField;
    union {
        UA_Double optionA;
        UA_String optionB;
    } fields;
} Uni;

static UA_DataTypeMember Uni_members[2] = {
        {
                UA_TYPENAME("optionA")
                UA_TYPES_DOUBLE,
                sizeof(UA_UInt32),
                true,
                false,
                false
        },
        {
                UA_TYPENAME("optionB")
                UA_TYPES_STRING,
                sizeof(UA_UInt32),
                true,
                false,
                false
        }
};

static const UA_DataType UniType = {
        UA_TYPENAME("Uni")
        {1, UA_NODEIDTYPE_NUMERIC, {4243}},
        sizeof(Uni),
        1,
        UA_DATATYPEKIND_UNION,
        false,
        false,
        2,
        0,
        Uni_members
};

const UA_DataTypeArray customDataTypesUnion = {&customDataTypesOptStruct, 2, &UniType};

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
    retval = UA_encodeBinary(&var, &UA_TYPES[UA_TYPES_VARIANT],
                             &pos, &end, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant var2;
    size_t offset = 0;
    retval = UA_decodeBinary(&buf, &offset, &var2, &UA_TYPES[UA_TYPES_VARIANT], &customDataTypes);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var2.type == &PointType);

    Point *p2 = (Point*)var2.data;
    ck_assert(p.x == p2->x);
        
    UA_Variant_deleteMembers(&var2);
    UA_ByteString_deleteMembers(&buf);
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
    retval = UA_encodeBinary(&eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &bufPos, &bufEnd, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_ExtensionObject eo2;
    size_t offset = 0;
    retval = UA_decodeBinary(&buf, &offset, &eo2, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &customDataTypes);
    ck_assert_int_eq(offset, (uintptr_t)(bufPos - buf.data));
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(eo2.encoding, UA_EXTENSIONOBJECT_DECODED);
    ck_assert(eo2.content.decoded.type == &PointType);

    Point *p2 = (Point*)eo2.content.decoded.data;
    ck_assert(p.x == p2->x);
        
    UA_ExtensionObject_deleteMembers(&eo2);
    UA_ByteString_deleteMembers(&buf);
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
    retval = UA_encodeBinary(&var, &UA_TYPES[UA_TYPES_VARIANT],
                             &pos, &end, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant var2;
    size_t offset = 0;
    retval = UA_decodeBinary(&buf, &offset, &var2, &UA_TYPES[UA_TYPES_VARIANT], &customDataTypes);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var2.type == &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    ck_assert_int_eq(var2.arrayLength, 10);

    for (size_t i = 0; i < 10; i++) {
        UA_ExtensionObject *eo = &((UA_ExtensionObject*)var2.data)[i];
        ck_assert_int_eq(eo->encoding, UA_EXTENSIONOBJECT_DECODED);
        ck_assert(eo->content.decoded.type == &PointType);
        Point *p2 = (Point*)eo->content.decoded.data;

        // we need to cast floats to int to avoid comparison of floats
        // which may result into false results
        ck_assert((int)p2->x == (int)ps[i].x);
        ck_assert((int)p2->y == (int)ps[i].y);
        ck_assert((int)p2->z == (int)ps[i].z);
    }
        
    UA_Variant_deleteMembers(&var2);
    UA_ByteString_deleteMembers(&buf);
} END_TEST

START_TEST(parseCustomStructureWithOptionalFields) {
        //Opt o;
        //o.hasB = true;      /* means optional field "b" is defined and will be encoded */
        //o.a = 3;
        //o.b = 2.5;

        Opt o;
        o.a = 3;
        o.b = NULL;
        //o.b = UA_Float_new();
        //*o.b = (UA_Float) 8.8;;
        o.c = UA_Float_new();
        *o.c = (UA_Float) 10.10;;

        UA_Variant var;
        UA_Variant_init(&var);
        UA_Variant_setScalarCopy(&var, &o, &OptType);

        size_t buflen = UA_calcSizeBinary(&var, &UA_TYPES[UA_TYPES_VARIANT]);
        UA_ByteString buf;
        UA_StatusCode retval = UA_ByteString_allocBuffer(&buf, buflen);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        UA_Byte *pos = buf.data;
        const UA_Byte *end = &buf.data[buf.length];
        retval = UA_encodeBinary(&var, &UA_TYPES[UA_TYPES_VARIANT],
                                 &pos, &end, NULL, NULL);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        UA_Variant var2;
        size_t offset = 0;
        retval = UA_decodeBinary(&buf, &offset, &var2, &UA_TYPES[UA_TYPES_VARIANT], &customDataTypesOptStruct);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert(var2.type == &OptType);

        Opt *optStruct2 = (Opt *) var2.data;
        //ck_assert(optStruct2->hasB == UA_TRUE);
        ck_assert(optStruct2->a == 3);
        ck_assert((fabs(*optStruct2->c - 10.10)) < 0.005);

        //TODO add magic size number
        UA_Float_delete(o.b);
        UA_Float_delete(o.c);
        UA_Variant_deleteMembers(&var);
        UA_Variant_deleteMembers(&var2);
        UA_ByteString_deleteMembers(&buf);
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
        retval = UA_encodeBinary(&var, &UA_TYPES[UA_TYPES_VARIANT],
                                 &pos, &end, NULL, NULL);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        UA_Variant var2;
        size_t offset = 0;
        retval = UA_decodeBinary(&buf, &offset, &var2, &UA_TYPES[UA_TYPES_VARIANT], &customDataTypesUnion);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert(var2.type == &UniType);

        Uni *uni2 = (Uni *) var2.data;
        ck_assert(uni2->switchField = UA_UNISWITCH_OPTIONB);
        UA_String compare = UA_STRING("test string");
        ck_assert(UA_String_equal(&u.fields.optionB, &compare));
        //ck_assert(uni2->fields.optionA = (fabs(uni2->fields.optionA - 2.5)) < 0.005);

        UA_Variant_deleteMembers(&var);
        UA_Variant_deleteMembers(&var2);
        UA_ByteString_deleteMembers(&buf);
    } END_TEST

int main(void) {
    Suite *s  = suite_create("Test Custom DataType Encoding");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, parseCustomScalar);
    tcase_add_test(tc, parseCustomScalarExtensionObject);
    tcase_add_test(tc, parseCustomArray);
    tcase_add_test(tc, parseCustomStructureWithOptionalFields);
    tcase_add_test(tc, parseCustomUnion);
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
