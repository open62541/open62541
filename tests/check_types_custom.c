/* This Source Code Form is subject to the terms of the Mozilla Public 
 * License, v. 2.0. If a copy of the MPL was not distributed with this 
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_types.h"
#include "ua_types_generated_handling.h"
#include "ua_types_encoding_binary.h"
#include "ua_util.h"
#include "check.h"

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
        false            /* .isArray */
    },

    /* y */
    {
        UA_TYPENAME("y")
        UA_TYPES_FLOAT, padding_y, true, false
    },

    /* z */
    {
        UA_TYPENAME("y")
        UA_TYPES_FLOAT, padding_z, true, false
    }
};

static const UA_DataType PointType = {
    UA_TYPENAME("Point")             /* .typeName */
    {1, UA_NODEIDTYPE_NUMERIC, {1}}, /* .typeId */
    sizeof(Point),                   /* .memSize */
    0,                               /* .typeIndex, in the array of custom types */
    3,                               /* .membersSize */
    false,                           /* .builtin */
    true,                            /* .pointerFree */
    false,                           /* .overlayable (depends on endianness and
                                         the absence of padding) */
    0,                               /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
    members
};

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
    retval = UA_decodeBinary(&buf, &offset, &var2, &UA_TYPES[UA_TYPES_VARIANT], 1, &PointType);
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
    retval = UA_decodeBinary(&buf, &offset, &eo2, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], 1, &PointType);
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
    retval = UA_decodeBinary(&buf, &offset, &var2, &UA_TYPES[UA_TYPES_VARIANT], 1, &PointType);
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

int main(void) {
    Suite *s  = suite_create("Test Custom DataType Encoding");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, parseCustomScalar);
    tcase_add_test(tc, parseCustomScalarExtensionObject);
    tcase_add_test(tc, parseCustomArray);
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
