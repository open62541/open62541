/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/types.h>

#include <stdlib.h>
#include <check.h>

#include "test_helpers.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

static const size_t usernamePasswordsSize = 2;
static UA_UsernamePasswordLogin usernamePasswords[2] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")},
    {UA_STRING_STATIC("user2"), UA_STRING_STATIC("password1")}};

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static UA_NodeId readWriteNodeId;
static UA_NodeId readOnlyNodeId;
static UA_NodeId writeOnlyNodeId;
static UA_NodeId executableNodeId;
static UA_NodeId notExecutableNodeId;

static UA_UInt32 methodCallCount = 0;
static UA_Byte userAccessLevel = UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_READ;
static UA_Boolean userExecutable = true;

static UA_UInt32
getUserRightsMask(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId,
                  void *sessionContext, const UA_NodeId *nodeId, void *nodeContext) {
    return userAccessLevel;
}

static UA_Byte
getUserAccessLevel(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId,
                   void *sessionContext, const UA_NodeId *nodeId, void *nodeContext) 
{
    return userAccessLevel;
}

static UA_Boolean
getUserExecutable(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId,
                  void *sessionContext, const UA_NodeId *methodId, void *methodContext) {
    return userExecutable;
}

static UA_Boolean
getUserExecutableOnObject(UA_Server *server, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext) {
    return userExecutable;
}

static void
addVariables(UA_Server *server) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US", "Description");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "DisplayName");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attr.valueRank = UA_VALUERANK_SCALAR;

    UA_NodeId parentNodeId = UA_NS0ID(OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NS0ID(ORGANIZES);
    UA_NodeId typeDefinition = UA_NS0ID(BASEDATAVARIABLETYPE);

    /* Add the variable nodes to the information model */
    UA_StatusCode retval;
    retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                                       parentReferenceNodeId,
                                       UA_QUALIFIEDNAME(1, "ReadWriteVariable"),
                                       typeDefinition, attr, NULL, &readWriteNodeId);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                                       parentReferenceNodeId,
                                       UA_QUALIFIEDNAME(1, "ReadOnlyVariable"),
                                       typeDefinition, attr, NULL, &readOnlyNodeId);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    attr.accessLevel = UA_ACCESSLEVELMASK_WRITE;
    retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                                       parentReferenceNodeId,
                                       UA_QUALIFIEDNAME(1, "WriteOnlyVariable"),
                                       typeDefinition, attr, NULL, &writeOnlyNodeId);
    ck_assert(retval == UA_STATUSCODE_GOOD);
}

static UA_StatusCode
writeVariable(UA_Client *client, UA_NodeId nodeId) {
    UA_WriteRequest req;
    UA_WriteRequest_init(&req);
    req.nodesToWriteSize = 1;

    UA_WriteValue writeValues[1] = {0};
    UA_WriteValue_init(&writeValues[0]);
    writeValues[0].attributeId = UA_ATTRIBUTEID_VALUE;
    writeValues[0].nodeId = nodeId;

    UA_DataValue dv;
    UA_Int32 value = 0;
    UA_DataValue_init(&dv);
    dv.hasValue = true;
    dv.value.type = &UA_TYPES[UA_TYPES_INT32];
    dv.value.storageType = UA_VARIANT_DATA_NODELETE;
    dv.value.data = &value;
    writeValues[0].value = dv;

    req.nodesToWrite = writeValues;
    UA_WriteResponse resp = UA_Client_Service_write(client, req);
    ck_assert(resp.resultsSize == 1);
    UA_StatusCode ret = *resp.results;
    UA_WriteResponse_clear(&resp);
    return ret;
}

static UA_StatusCode
readVariable(UA_Client *client, UA_NodeId nodeId) {
    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.nodesToReadSize = 1;

    UA_ReadValueId readValues[1] = {0};
    UA_ReadValueId_init(&readValues[0]);
    readValues[0].attributeId = UA_ATTRIBUTEID_VALUE;
    readValues[0].nodeId = nodeId;

    req.nodesToRead = readValues;
    UA_ReadResponse resp = UA_Client_Service_read(client, req);
    ck_assert(resp.resultsSize == 1);
    UA_StatusCode ret = resp.results->status;
    UA_ReadResponse_clear(&resp);
    return ret;
}

static UA_StatusCode
executeMethod(UA_Client *client, UA_NodeId methodId) {
    UA_CallRequest req;
    UA_CallRequest_init(&req);
    UA_CallMethodRequest cmr;
    UA_CallMethodRequest_init(&cmr);
    cmr.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    cmr.methodId = methodId;
    req.methodsToCall = &cmr;
    req.methodsToCallSize = 1;

    UA_CallResponse resp = UA_Client_Service_call(client, req);
    ck_assert(resp.resultsSize == 1);
    UA_StatusCode ret = resp.results[0].statusCode;
    UA_CallResponse_clear(&resp);
    return ret;
}

static UA_StatusCode
methodCallback(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *methodId, void *methodContext, const UA_NodeId *objectId,
               void *objectContext, size_t inputSize, const UA_Variant *input,
               size_t outputSize, UA_Variant *output) {
    methodCallCount++;
    return UA_STATUSCODE_GOOD;
}

static void
addMethod(UA_Server *server) {
    /* Add a method to the address space */
    UA_MethodAttributes attr = UA_MethodAttributes_default;
    attr.executable = true;
    UA_StatusCode retval;
    retval = UA_Server_addMethodNode(
        server, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "ExecutableMethod"), attr, &methodCallback, 0, NULL, 0, NULL,
        NULL,
        &executableNodeId);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    attr.executable = false;
    retval = UA_Server_addMethodNode(
        server, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "NotExecutableMethod"), attr, &methodCallback, 0, NULL, 0,
        NULL, NULL, &notExecutableNodeId);
    ck_assert(retval == UA_STATUSCODE_GOOD);
}

static void setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->allowNonePolicyPassword = true;

    /* Instantiate a new AccessControl plugin that knows username/pw */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_SecurityPolicy *sp = &config->securityPolicies[config->securityPoliciesSize-1];
    UA_AccessControl_default(config, true, &sp->policyUri,
                             usernamePasswordsSize, usernamePasswords);

    sc->accessControl.getUserAccessLevel = &getUserAccessLevel;
    sc->accessControl.getUserExecutable = &getUserExecutable;
    sc->accessControl.getUserExecutableOnObject = &getUserExecutableOnObject;
    sc->accessControl.getUserRightsMask = &getUserRightsMask;

    addVariables(server);
    addMethod(server);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Client_anonymous) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Client_user_pass_ok) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Client_user_fail) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user0", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Client_pass_fail) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "secret");
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Client_useraccess_variable_write) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Check proper error codes */
    userAccessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    retval = writeVariable(client, readWriteNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = writeVariable(client, writeOnlyNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = writeVariable(client, readOnlyNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTWRITABLE);

    userAccessLevel = UA_ACCESSLEVELMASK_READ;
    retval = writeVariable(client, readWriteNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    retval = writeVariable(client, writeOnlyNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    retval = writeVariable(client, readOnlyNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTWRITABLE);

    userAccessLevel = UA_ACCESSLEVELMASK_WRITE;
    retval = writeVariable(client, readWriteNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = writeVariable(client, writeOnlyNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = writeVariable(client, readOnlyNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTWRITABLE);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_useraccess_variable_read) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840",
                                                     "user2", "password1");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read with read and write user access level */
    userAccessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    retval = readVariable(client, readWriteNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = readVariable(client, writeOnlyNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTREADABLE);
    retval = readVariable(client, readOnlyNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read with read only user access level */
    userAccessLevel = UA_ACCESSLEVELMASK_READ;
    retval = readVariable(client, readWriteNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = readVariable(client, writeOnlyNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTREADABLE);
    retval = readVariable(client, readOnlyNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read with write only user access level */
    userAccessLevel = UA_ACCESSLEVELMASK_WRITE;
    retval = readVariable(client, readWriteNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    retval = readVariable(client, writeOnlyNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTREADABLE);
    retval = readVariable(client, readOnlyNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_useraccess_method) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840",
                                                     "user1", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Executable user access */
    userExecutable = true;
    retval = executeMethod(client, executableNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = executeMethod(client, notExecutableNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTEXECUTABLE);

    /* Non-executable user access */
    userExecutable = false;
    retval = executeMethod(client, executableNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    retval = executeMethod(client, notExecutableNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTEXECUTABLE);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client_user = tcase_create("Client User/Password");
    tcase_add_checked_fixture(tc_client_user, setup, teardown);
    tcase_add_test(tc_client_user, Client_anonymous);
    tcase_add_test(tc_client_user, Client_user_pass_ok);
    tcase_add_test(tc_client_user, Client_user_fail);
    tcase_add_test(tc_client_user, Client_pass_fail);
    tcase_add_test(tc_client_user, Client_useraccess_variable_write);
    tcase_add_test(tc_client_user, Client_useraccess_variable_read);
    tcase_add_test(tc_client_user, Client_useraccess_method);
    suite_add_tcase(s,tc_client_user);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
