/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>
#include <open62541/util.h>

#include "open62541/plugin/nodesetloader.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "tests/namespace_tests_autoid_generated.h"
#include "tests/namespace_tests_di_generated.h"
#include "thread_wrapper.h"

static UA_Server *server;
static UA_Client *client;
static UA_NodeId *di;
static size_t diSize;
static UA_NodeId *autoId;
static size_t autoIdSize;
static UA_Boolean running;
static THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setupPrelude(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
}

static void setupRest(void) {
     size_t diIndex = 0;
    ck_assert_uint_eq(UA_STATUSCODE_GOOD, UA_Server_getNamespaceByName(server,
        UA_STRING("http://opcfoundation.org/UA/DI/"), &diIndex));
    size_t autoIdIndex = 0;
    ck_assert_uint_eq(UA_STATUSCODE_GOOD, UA_Server_getNamespaceByName(server,
        UA_STRING("http://opcfoundation.org/UA/AutoID/"), &autoIdIndex));

    // find all NodeId's UA_getRemoteDatatypes() should return
    const UA_DataTypeArray* array = UA_Server_getDataTypes(server);
    diSize = autoIdSize = 0;
    for(const UA_DataTypeArray *currentArray = array; NULL != currentArray; currentArray
        = currentArray->next) {
        for (size_t i = 0; i < currentArray->typesSize; ++i) {
            const UA_DataType dt = currentArray->types[i];
            if (dt.typeId.namespaceIndex == diIndex) {
                diSize++;
            }
            else if (dt.typeId.namespaceIndex == autoIdIndex) {
                autoIdSize++;
            }
        }
    }
    di = (UA_NodeId*)UA_Array_new(diSize, &UA_TYPES[UA_TYPES_NODEID]);
    ck_assert(NULL != di);
    autoId = (UA_NodeId*)UA_Array_new(autoIdSize, &UA_TYPES[UA_TYPES_NODEID]);
    ck_assert(NULL != autoId);
    size_t diPos = 0;
    size_t autoIdPos = 0;
    for(const UA_DataTypeArray *currentArray = array; NULL != currentArray; currentArray
        = currentArray->next) {
        for (size_t i = 0; i < currentArray->typesSize; ++i) {
            const UA_DataType dt = currentArray->types[i];
            ck_assert(dt.typeId.identifierType == UA_NODEIDTYPE_NUMERIC);
            if (dt.typeId.namespaceIndex == diIndex) {
                di[diPos] = UA_NODEID_NUMERIC(diIndex, dt.typeId.identifier.numeric);
                ++diPos;
                printf("DI DataType: ns=%u i=%u %s\n", dt.typeId.namespaceIndex,
                    dt.typeId.identifier.numeric, dt.typeName);
            }
            else if (dt.typeId.namespaceIndex == autoIdIndex) {
                autoId[autoIdPos] = UA_NODEID_NUMERIC(autoIdIndex, dt.typeId.identifier.numeric);
                ++autoIdPos;
                printf("AutoID DataType: ns=%u i=%u %s\n", dt.typeId.namespaceIndex,
                    dt.typeId.identifier.numeric, dt.typeName);
            }
        }
    }

    // start server
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
    client = UA_Client_newForUnitTest();
    ck_assert(NULL != client);
    const UA_StatusCode resConnect = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(resConnect, UA_STATUSCODE_GOOD);
}

#ifdef UA_ENABLE_NODESETLOADER
static void setupNodesetLoader(void) {
    setupPrelude();
    UA_Server_loadNodeset(server, UA_NODESET_DIR "DI/Opc.Ua.Di.NodeSet2.xml", NULL);
    UA_Server_loadNodeset(server, UA_NODESET_DIR "AutoID/Opc.Ua.AutoID.NodeSet2.xml", NULL);
    setupRest();
}
#endif

static void setupNodesetCompiler(void) {
    setupPrelude();
    ck_assert_uint_eq(UA_STATUSCODE_GOOD, namespace_tests_di_generated(server));
    ck_assert_uint_eq(UA_STATUSCODE_GOOD, namespace_tests_autoid_generated(server));
    setupRest();
}


static void
teardown(void) {
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_Array_delete(di, diSize, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Array_delete(autoId, autoIdSize, &UA_TYPES[UA_TYPES_NODEID]);
    di = autoId = NULL;
    diSize = autoIdSize = 0;
}

START_TEST(get_remote_datatypes_di_and_autoid) {
    // Fetch all remote datatypes
    UA_DataTypeArray* array = NULL;
    ck_assert_uint_eq(UA_STATUSCODE_GOOD, UA_Client_getRemoteDataTypes(client, 0, NULL,
        &array));
    ck_assert(NULL != array);
    ck_assert(NULL != UA_Client_getConfig(client));
    UA_Client_getConfig(client)->customDataTypes = array;

    bool allFound = true;
    UA_UInt16 diIdx = 0;
    ck_assert_uint_eq(UA_STATUSCODE_GOOD, UA_Client_getNamespaceIndex(client,
        UA_STRING("http://opcfoundation.org/UA/DI/"), &diIdx));
    for (size_t i = 0; i < diSize; ++i) {
        const UA_NodeId id = UA_NODEID_NUMERIC(diIdx, di[i].identifier.numeric);
        const UA_DataType* dt = UA_Client_findDataType(client, &id);
        allFound = allFound && NULL != dt;
        if (NULL == dt) {
            printf("Failed to find datatype with NodeId ns=%u i=%u\n", id.namespaceIndex,
                id.identifier.numeric);
        }
    }
    UA_UInt16 autoIdIdx = 0;
    ck_assert_uint_eq(UA_STATUSCODE_GOOD, UA_Client_getNamespaceIndex(client,
        UA_STRING("http://opcfoundation.org/UA/AutoID/"), &autoIdIdx));
    for (size_t i = 0; i < autoIdSize; ++i) {
        const UA_NodeId id = UA_NODEID_NUMERIC(autoIdIdx, autoId[i].identifier.numeric);
        const UA_DataType* dt = UA_Client_findDataType(client, &id);
        allFound = allFound && NULL != dt;
        if (NULL == dt) {
            printf("Failed to find datatype with NodeId ns=%u i=%u\n", id.namespaceIndex,
                id.identifier.numeric);
        }
    }
    ck_assert_msg(allFound, "Not all datatypes from DI and AutoID were found in the client after getRemoteDataTypes");
}
END_TEST

static Suite*
testSuite_GetRemoteDatatypes2(void) {
    Suite *s = suite_create("Client - Remote DataTypes DI and AutoID");
    TCase *tcCompiler = tcase_create("nodeset-compiler");
    tcase_add_checked_fixture(tcCompiler, setupNodesetCompiler, teardown);
    tcase_add_test(tcCompiler, get_remote_datatypes_di_and_autoid);
    suite_add_tcase(s, tcCompiler);
#ifdef UA_ENABLE_NODESETLOADER
    TCase *tcLoader = tcase_create("nodeset-loader");
    tcase_add_checked_fixture(tcLoader, setupNodesetLoader, teardown);
    tcase_add_test(tcLoader, get_remote_datatypes_di_and_autoid);
    suite_add_tcase(s, tcLoader);
#endif
    return s;
}

int
main(void) {
    Suite *s = testSuite_GetRemoteDatatypes2();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
