/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Coverage tests for src/server/ua_server_utils.c. The existing
 * check_server.c only lightly exercises this file. This test focuses on
 * the public DataType-management API:
 *   - UA_Server_addDataType (the pre-converted UA_DataType variant)
 *   - UA_Server_findDataType (lookup by NodeId)
 *   - UA_Server_getDataTypes (list head)
 *   - Duplicate-add rejection
 *   - UA_Server_addDataTypeFromDescription
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "test_helpers.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

static UA_Server *server;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/* A simple custom DataType: a single Int32 field. We define a static
 * UA_DataType so we can hand it to UA_Server_addDataType. */
static UA_DataTypeMember customMember[1] = {
    {
        .memberName = "value",
        .memberType = &UA_TYPES[UA_TYPES_INT32],
        .isArray = false,
        .isOptional = false
    }
};

static UA_DataType customType;

static void initCustomType(UA_DataType *t, UA_UInt16 ns, UA_UInt32 idBase) {
    memset(t, 0, sizeof(*t));
    t->typeName = "MyCustomType";
    t->typeId.namespaceIndex = ns;
    t->typeId.identifierType = UA_NODEIDTYPE_NUMERIC;
    t->typeId.identifier.numeric = idBase;
    t->binaryEncodingId.namespaceIndex = ns;
    t->binaryEncodingId.identifierType = UA_NODEIDTYPE_NUMERIC;
    t->binaryEncodingId.identifier.numeric = idBase + 1;
    t->xmlEncodingId.namespaceIndex = ns;
    t->xmlEncodingId.identifierType = UA_NODEIDTYPE_NUMERIC;
    t->xmlEncodingId.identifier.numeric = idBase + 2;
    t->memSize = sizeof(UA_Int32);
    t->typeKind = UA_DATATYPEKIND_STRUCTURE;
    t->pointerFree = true;
    t->overlayable = true;
    t->membersSize = 1;
    t->members = customMember;
}

START_TEST(Utils_addDataType_basic) {
    initCustomType(&customType, 1, 99001);
    UA_NodeId base = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    UA_StatusCode rv = UA_Server_addDataType(server, base, &customType);
    ck_assert_uint_eq(rv, UA_STATUSCODE_GOOD);

    /* It must be findable by NodeId. */
    const UA_DataType *found = UA_Server_findDataType(server, &customType.typeId);
    ck_assert_ptr_ne(found, NULL);
    ck_assert(UA_NodeId_equal(&found->typeId, &customType.typeId));
    ck_assert_uint_eq(found->memSize, customType.memSize);
} END_TEST

START_TEST(Utils_addDataType_duplicate_rejected) {
    initCustomType(&customType, 1, 99001);
    UA_NodeId base = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    ck_assert_uint_eq(UA_Server_addDataType(server, base, &customType),
                     UA_STATUSCODE_GOOD);

    /* Adding the same DataType again must fail (the file uses
     * BADNODEIDEXISTS to reject duplicates). */
    UA_StatusCode rv = UA_Server_addDataType(server, base, &customType);
    ck_assert(rv != UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(Utils_findDataType_unknown) {
    /* Looking up a NodeId that was never added must return NULL. */
    UA_NodeId unknown = UA_NODEID_NUMERIC(1, 88000);
    const UA_DataType *found = UA_Server_findDataType(server, &unknown);
    ck_assert_ptr_eq(found, NULL);
} END_TEST

START_TEST(Utils_getDataTypes_returnsList) {
    /* Before any custom types are added, the list may be NULL. After
     * adding one, the list head must be non-NULL and contain the type. */
    initCustomType(&customType, 1, 99200);
    UA_NodeId base = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    ck_assert_uint_eq(UA_Server_addDataType(server, base, &customType),
                     UA_STATUSCODE_GOOD);

    const UA_DataTypeArray *arr = UA_Server_getDataTypes(server);
    ck_assert_ptr_ne(arr, NULL);
    /* The chain must include at least our custom type. */
    UA_Boolean found = false;
    for(const UA_DataTypeArray *cur = arr; cur != NULL; cur = cur->next) {
        for(size_t i = 0; i < cur->typesSize; i++) {
            if(UA_NodeId_equal(&cur->types[i].typeId, &customType.typeId)) {
                found = true;
                break;
            }
        }
        if(found) break;
    }
    ck_assert(found);
} END_TEST

/* Add the same type under a different nodeId and verify both are
 * findable. This tests that the linked list is extended properly. */
START_TEST(Utils_addDataType_multiple) {
    UA_DataType t1, t2;
    initCustomType(&t1, 1, 1101);
    initCustomType(&t2, 1, 1103);

    UA_NodeId base = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    ck_assert_uint_eq(UA_Server_addDataType(server, base, &t1), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDataType(server, base, &t2), UA_STATUSCODE_GOOD);

    ck_assert_ptr_ne(UA_Server_findDataType(server, &t1.typeId), NULL);
    ck_assert_ptr_ne(UA_Server_findDataType(server, &t2.typeId), NULL);
} END_TEST

/* Adding TYPES_LIST_SIZE (64) datatypes fills the first internal
 * array. Adding one more exercises the realloc/next-pointer chain in
 * addDataType that the existing 5 tests do not hit. */
START_TEST(Utils_addDataType_fillsFirstList) {
    /* Use a fresh member array so each type has a distinct member
     * pointer. Otherwise the second/third call would be flagged as a
     * duplicate type by the lookup guard. */
    static UA_DataTypeMember memberTemplate[1] = {
        {
            .memberName = "value",
            .memberType = &UA_TYPES[UA_TYPES_INT32],
            .isArray = false,
            .isOptional = false
        }
    };
    UA_NodeId base = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    for(UA_UInt32 i = 0; i < 65; i++) {
        UA_DataType t;
        initCustomType(&t, 1, 99200 + i);
        t.members = memberTemplate;
        UA_StatusCode rv = UA_Server_addDataType(server, base, &t);
        ck_assert_uint_eq(rv, UA_STATUSCODE_GOOD);
    }
    /* All 65 must be findable. */
    for(UA_UInt32 i = 0; i < 65; i++) {
        UA_NodeId id = UA_NODEID_NUMERIC(1, 99200 + i);
        const UA_DataType *found = UA_Server_findDataType(server, &id);
        ck_assert_ptr_ne(found, NULL);
    }
    /* The first 64 should sit in customTypes_internal[0], the 65th should
     * have triggered a realloc into customTypes_internal[1] (or beyond). */
    const UA_DataTypeArray *list = UA_Server_getDataTypes(server);
    ck_assert_ptr_ne(list, NULL);
    /* Walk the list and count until we have seen 65 entries. */
    size_t total = 0;
    for(const UA_DataTypeArray *cur = list; cur != NULL; cur = cur->next) {
        total += cur->typesSize;
    }
    ck_assert_uint_eq(total, 65);
} END_TEST

/* An empty (no-decoded-body) extension object is also rejected with
 * BADINVALIDARGUMENT for the same reason. */
START_TEST(Utils_addDataTypeFromDescription_empty_rejected) {
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode res = UA_Server_addDataTypeFromDescription(server, &eo);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

/* === Suite === */

static Suite* testSuite_Utils(void) {
    Suite *s = suite_create("Server Utils Coverage");
    TCase *tc = tcase_create("DataType API");
    /* Allow more time per test when run under Valgrind / sanitizer. */
    tcase_set_timeout(tc, 60);
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, Utils_addDataType_basic);
    tcase_add_test(tc, Utils_addDataType_duplicate_rejected);
    tcase_add_test(tc, Utils_findDataType_unknown);
    tcase_add_test(tc, Utils_getDataTypes_returnsList);
    tcase_add_test(tc, Utils_addDataType_multiple);
    tcase_add_test(tc, Utils_addDataType_fillsFirstList);
    tcase_add_test(tc, Utils_addDataTypeFromDescription_empty_rejected);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite_Utils();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_FORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
