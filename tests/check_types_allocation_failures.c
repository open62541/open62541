/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <open62541/types.h>
#include <open62541/util.h>
#include <check.h>
#include <stdlib.h>

#if defined(UA_ENABLE_MALLOC_SINGLETON) && defined(UA_TYPES_STRUCTUREDESCRIPTION)
static size_t allocationIndex, failingAllocation, live;
static void *owned[1024];

static void *
remember(void *ptr) {
    if(ptr) {
        for(size_t i = 0; i < 1024; i++) {
            if(!owned[i]) { owned[i] = ptr; live++; return ptr; }
        }
        ck_abort_msg("allocation tracker exhausted");
    }
    return ptr;
}
static void *testMalloc(size_t size) {
    return allocationIndex++ == failingAllocation ? NULL : remember(malloc(size));
}
static void *testCalloc(size_t count, size_t size) {
    return allocationIndex++ == failingAllocation ? NULL : remember(calloc(count, size));
}
static void testFree(void *ptr) {
    if(!ptr) return;
    for(size_t i = 0; i < 1024; i++) {
        if(owned[i] == ptr) { owned[i] = NULL; live--; free(ptr); return; }
    }
    ck_abort_msg("attempt to free borrowed datatype storage");
}
static void *testRealloc(void *ptr, size_t size) {
    if(allocationIndex++ == failingAllocation) return NULL;
    if(!ptr) return remember(realloc(NULL, size));
    for(size_t i = 0; i < 1024; i++) {
        if(owned[i] == ptr) {
            void *next = realloc(ptr, size);
            if(next || size == 0) { owned[i] = next; if(!next) live--; }
            return next;
        }
    }
    ck_abort_msg("attempt to resize borrowed datatype storage");
    return NULL;
}
static void setup(void) {
    allocationIndex = 0; failingAllocation = (size_t)-1; live = 0;
    memset(owned, 0, sizeof(owned));
    UA_mallocSingleton = testMalloc; UA_callocSingleton = testCalloc;
    UA_reallocSingleton = testRealloc; UA_freeSingleton = testFree;
}
static void teardown(void) {
    UA_mallocSingleton = malloc; UA_callocSingleton = calloc;
    UA_reallocSingleton = realloc; UA_freeSingleton = free;
    for(size_t i = 0; i < 1024; i++) free(owned[i]);
}

static UA_StatusCode
operation(int mode) {
    typedef struct { UA_UInt32 number; UA_String text; } Record;
    UA_DataTypeMember members[] = {
        {UA_TYPENAME("Number") &UA_TYPES[UA_TYPES_UINT32], 0, false, false},
        {UA_TYPENAME("Text") &UA_TYPES[UA_TYPES_STRING],
         offsetof(Record, text) - sizeof(UA_UInt32), false, false}};
    UA_DataType type;
    memset(&type, 0, sizeof(type));
#ifdef UA_ENABLE_TYPEDESCRIPTION
    type.typeName = "AllocationProbe";
#endif
    type.typeId = UA_NODEID_STRING(1, "ProbeType");
    type.binaryEncodingId = UA_NODEID_STRING(1, "Probe.Binary");
    type.xmlEncodingId = UA_NODEID_STRING(1, "Probe.XML");
    type.typeKind = UA_DATATYPEKIND_STRUCTURE; type.memSize = sizeof(Record);
    type.membersSize = 2; type.members = members;
    if(mode == 0) {
        UA_DataType copy;
        UA_StatusCode res = UA_DataType_copy(&type, &copy);
        if(res == UA_STATUSCODE_GOOD) {
#ifdef UA_ENABLE_TYPEDESCRIPTION
            ck_assert_str_eq(copy.typeName, "AllocationProbe");
            ck_assert_str_eq(copy.members[1].memberName, "Text");
#endif
            ck_assert(UA_NodeId_equal(&type.typeId, &copy.typeId));
            ck_assert_ptr_ne(type.typeId.identifier.string.data, copy.typeId.identifier.string.data);
        }
        UA_DataType_clear(&copy);
        return res;
    }
    if(mode == 3) {
        UA_StructureDescription description;
        UA_StructureDescription_init(&description);
        description.dataTypeId = type.typeId;
        description.name = UA_QUALIFIEDNAME(0, "Unsupported");
        description.structureDefinition.structureType = UA_STRUCTURETYPE_STRUCTUREWITHSUBTYPEDVALUES;
        UA_ExtensionObject object;
        UA_ExtensionObject_setValueNoDelete(&object, &description, &UA_TYPES[UA_TYPES_STRUCTUREDESCRIPTION]);
        UA_DataType output;
        UA_StatusCode res = UA_DataType_fromDescription(&output, &object, NULL);
        ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);
        ck_assert(UA_NodeId_isNull(&output.typeId));
        UA_DataType_clear(&output);
        return res;
    }
    if(mode == 2) {
        type.typeKind = UA_DATATYPEKIND_ENUM; type.memSize = sizeof(UA_Int32);
        members[0].memberType = (const UA_DataType*)(intptr_t)-7;
        members[1].memberType = (const UA_DataType*)(intptr_t)42;
    }
    UA_ExtensionObject description;
    UA_StatusCode res = UA_DataType_toDescription(&type, &description);
    if(res == UA_STATUSCODE_GOOD) {
#ifdef UA_ENABLE_TYPEDESCRIPTION
        UA_String expected = UA_STRING("Text");
        if(mode == 2) {
            UA_EnumDescription *d = (UA_EnumDescription*)description.content.decoded.data;
            ck_assert(UA_String_equal(&d->enumDefinition.fields[1].name, &expected));
        } else {
            UA_StructureDescription *d = (UA_StructureDescription*)description.content.decoded.data;
            ck_assert(UA_String_equal(&d->structureDefinition.fields[1].name, &expected));
        }
#endif
    }
    UA_ExtensionObject_clear(&description);
    return res;
}

START_TEST(everyAllocationCanFail) {
    UA_StatusCode result = operation(_i);
    ck_assert_uint_eq(result, _i == 3 ? UA_STATUSCODE_BADNOTIMPLEMENTED : UA_STATUSCODE_GOOD);
    size_t count = allocationIndex;
    ck_assert_uint_gt(count, 0);
    ck_assert_uint_eq(live, 0);
    for(size_t failure = 0; failure < count; failure++) {
        allocationIndex = 0; failingAllocation = failure;
        result = operation(_i);
        ck_assert_uint_eq(result, UA_STATUSCODE_BADOUTOFMEMORY);
        ck_assert_uint_eq(live, 0);
    }
} END_TEST
#endif

int main(void) {
    Suite *s = suite_create("Datatype allocation failures");
    TCase *tc = tcase_create("Ownership and description conversion");
#if defined(UA_ENABLE_MALLOC_SINGLETON) && defined(UA_TYPES_STRUCTUREDESCRIPTION)
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_loop_test(tc, everyAllocationCanFail, 0, 4);
#endif
    suite_add_tcase(s, tc);
    SRunner *runner = srunner_create(s);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
