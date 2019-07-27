/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "check.h"
#include "testing_clock.h"
#include "unistd.h"

#include <open62541/plugin/nodesetLoader.h>

#define TESTIMPORTXML NODESETPATH "/testimport.xml"
#define BASICNODECLASSTESTXML NODESETPATH "/basicNodeClassTest.xml"
#define NS0VALUESXML NODESETPATH "/namespaceZeroValues.xml"

UA_Server *server;

static void setup(void) {
    printf("path to testnodesets %s\n", NODESETPATH);
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Server_ImportNodeset) {
    FileHandler f;
	f.addNamespace = UA_Server_addNamespace;
    f.server = server;
    f.file = TESTIMPORTXML;
    UA_StatusCode retval = UA_XmlImport_loadFile(&f);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(Server_ImportNoFile) {
    FileHandler f;
	f.addNamespace = UA_Server_addNamespace;
    f.server = server;
    f.file = "notExistingFile.xml";
    UA_StatusCode retval = UA_XmlImport_loadFile(&f);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTFOUND);
}
END_TEST

START_TEST(Server_EmptyHandler) {
    UA_StatusCode retval = UA_XmlImport_loadFile(NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
}
END_TEST

START_TEST(Server_ImportBasicNodeClassTest) {
    FileHandler f;
	f.addNamespace = UA_Server_addNamespace;
    f.server = server;
    f.file = BASICNODECLASSTESTXML;
    UA_StatusCode retval = UA_XmlImport_loadFile(&f);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

static UA_UInt16 getNamespaceIndex(const char* uri)
{
    UA_Variant namespaceArray;
    UA_Variant_init(&namespaceArray);
    UA_Server_readValue(server, UA_NODEID_NUMERIC(0, 2255), &namespaceArray);
    UA_UInt16 nsidx = 0;
    for(size_t cnt = 0; cnt < namespaceArray.arrayLength; cnt++) {
        if(!strncmp((char*)((UA_String*)namespaceArray.data)[cnt].data, uri, ((UA_String*)namespaceArray.data)[cnt].length))
        {
            
            nsidx =(UA_UInt16)cnt;
            break;
        }
    }
    UA_Variant_clear(&namespaceArray);
    return nsidx;
}

START_TEST(Server_LoadNS0Values) {
    FileHandler f;
	f.addNamespace = UA_Server_addNamespace;
    f.server = server;
    f.file = NS0VALUESXML;
    UA_StatusCode retval = UA_XmlImport_loadFile(&f);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_UInt16 nsIdx =
        getNamespaceIndex("http://open62541.com/nodesetimport/tests/namespaceZeroValues");
    ck_assert_uint_gt(nsIdx, 0);
    UA_Variant var;
    UA_Variant_init(&var);
    //scalar double
    retval = UA_Server_readValue(server, UA_NODEID_NUMERIC(nsIdx, 1003), &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var.type->typeIndex == UA_TYPES_DOUBLE);
    ck_assert(*(UA_Double *)var.data - 3.14 < 0.01);
    UA_Variant_clear(&var);
    //array of Uint32
    retval = UA_Server_readValue(server, UA_NODEID_NUMERIC(nsIdx, 1004), &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var.type->typeIndex == UA_TYPES_UINT32);
    ck_assert_uint_eq(((UA_UInt32 *)var.data)[2], 140);
    UA_Variant_clear(&var);
    //extension object with nested struct
    retval = UA_Server_readValue(server, UA_NODEID_NUMERIC(nsIdx, 1005), &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var.type->typeIndex == UA_TYPES_SERVERSTATUSDATATYPE);
    ck_assert_int_eq(((UA_ServerStatusDataType *)var.data)->state, 5);
    UA_Variant_clear(&var);
    //array of extension object with nested struct
    retval = UA_Server_readValue(server, UA_NODEID_NUMERIC(nsIdx, 1006), &var);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(var.type->typeIndex == UA_TYPES_SERVERSTATUSDATATYPE);
    ck_assert_int_eq(((UA_ServerStatusDataType *)var.data)[1].state, 3);
    UA_Variant_clear(&var);
}
END_TEST

static Suite *testSuite_Client(void) {
    Suite *s = suite_create("server nodeset import");
    TCase *tc_server = tcase_create("server nodeset import");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_ImportNodeset);
    tcase_add_test(tc_server, Server_ImportNoFile);
    tcase_add_test(tc_server, Server_EmptyHandler);
    tcase_add_test(tc_server, Server_ImportBasicNodeClassTest);
    tcase_add_test(tc_server, Server_LoadNS0Values);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(int argc, char*argv[]) {
    printf("%s", argv[0]);
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
