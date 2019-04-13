/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2019 (c) basysKom GmbH <opensource@basyskom.com> (Author: Frank Meerkötter)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "check.h"

static UA_Server *server = NULL;

static UA_StatusCode
methodCallback(UA_Server *serverArg,
         const UA_NodeId *sessionId, void *sessionHandle,
         const UA_NodeId *methodId, void *methodContext,
         const UA_NodeId *objectId, void *objectContext,
         size_t inputSize, const UA_Variant *input,
         size_t outputSize, UA_Variant *output)
{
    return UA_STATUSCODE_GOOD;
}

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    UA_MethodAttributes noFpAttr = UA_MethodAttributes_default;
    noFpAttr.description = UA_LOCALIZEDTEXT("en-US","No function pointer attached");
    noFpAttr.displayName = UA_LOCALIZEDTEXT("en-US","No function pointer attached");
    noFpAttr.executable = true;
    noFpAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "nofunctionpointer"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "No function pointer"),
                            noFpAttr, NULL, // no callback
                            0, NULL, 0, NULL, NULL, NULL);

    UA_MethodAttributes nonExecAttr = UA_MethodAttributes_default;
    nonExecAttr.description = UA_LOCALIZEDTEXT("en-US","Not executable");
    nonExecAttr.displayName = UA_LOCALIZEDTEXT("en-US","Not executable");
    nonExecAttr.executable = false;
    nonExecAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "nonexec"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Not executable"),
                            nonExecAttr, &methodCallback,
                            0, NULL, 0, NULL, NULL, NULL);
}

static void teardown(void) {
    UA_Server_delete(server);
}

START_TEST(callUnknownMethod) {
    const UA_UInt32 UA_NS0ID_UNKNOWN_METHOD = 60000;

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_UNKNOWN_METHOD);

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADNODEIDUNKNOWN);
} END_TEST

START_TEST(callKnownMethodOnUnknownObject) {
    const UA_UInt32 UA_NS0ID_UNKNOWN_OBJECT = 60000;

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_REQUESTSERVERSTATECHANGE);
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_UNKNOWN_OBJECT);

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADNODEIDUNKNOWN);
} END_TEST

START_TEST(callMethodAndObjectExistsButMethodHasWrongNodeClass) {
    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);  // not a method
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADNODECLASSINVALID);
} END_TEST

START_TEST(callMethodOnUnrelatedObject) {
    /* Minimal nodeset does not add any method nodes we may call here */
#ifdef UA_GENERATED_NAMESPACE_ZERO
    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS);
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);  // not connected via hasComponent

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADMETHODINVALID);
#endif
} END_TEST

START_TEST(callMethodAndObjectExistsButNoFunctionPointerAttached) {
    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.methodId = UA_NODEID_STRING(1, "nofunctionpointer");
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADINTERNALERROR);
} END_TEST

START_TEST(callMethodNonExecutable) {
    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.methodId = UA_NODEID_STRING(1, "nonexec");
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADNOTEXECUTABLE);
} END_TEST

START_TEST(callMethodWithMissingArguments) {
/* Minimal nodeset does not add any method nodes we may call here */
#ifdef UA_GENERATED_NAMESPACE_ZERO
    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS);
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADARGUMENTSMISSING);
#endif
} END_TEST

START_TEST(callMethodWithTooManyArguments) {
/* Minimal nodeset does not add any method nodes we may call here */
#ifdef UA_GENERATED_NAMESPACE_ZERO
    UA_Variant inputArguments[2];
    UA_Variant_init(&inputArguments[0]);
    UA_Variant_init(&inputArguments[1]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 2;         // 1 would be correct
    callMethodRequest.inputArguments = (UA_Variant*)&inputArguments;
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS);
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADTOOMANYARGUMENTS);
#endif
} END_TEST

START_TEST(callMethodWithWronglyTypedArguments) {
/* Minimal nodeset does not add any method nodes we may call here */
#ifdef UA_GENERATED_NAMESPACE_ZERO
    UA_Variant inputArgument;
    UA_Variant_init(&inputArgument);
    UA_Double wrongType = 1.0;
    UA_Variant_setScalar(&inputArgument, &wrongType, &UA_TYPES[UA_TYPES_DOUBLE]);  // UA_UInt32 would be correct

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 1;
    callMethodRequest.inputArguments = &inputArgument;
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS);
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);

    ck_assert_int_gt(result.inputArgumentResultsSize, 0);
    ck_assert_int_eq(result.inputArgumentResults[0], UA_STATUSCODE_BADTYPEMISMATCH);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_Array_delete(result.inputArgumentResults, result.inputArgumentResultsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
#endif
} END_TEST

int main(void) {
    Suite *s = suite_create("services_call");

    TCase *tc_call = tcase_create("call - error branches");
    tcase_add_checked_fixture(tc_call, setup, teardown);
    tcase_add_test(tc_call, callUnknownMethod);
    tcase_add_test(tc_call, callKnownMethodOnUnknownObject);
    tcase_add_test(tc_call, callMethodAndObjectExistsButMethodHasWrongNodeClass);
    tcase_add_test(tc_call, callMethodAndObjectExistsButNoFunctionPointerAttached);
    tcase_add_test(tc_call, callMethodNonExecutable);
    tcase_add_test(tc_call, callMethodOnUnrelatedObject);
    tcase_add_test(tc_call, callMethodWithMissingArguments);
    tcase_add_test(tc_call, callMethodWithTooManyArguments);
    tcase_add_test(tc_call, callMethodWithWronglyTypedArguments);
    suite_add_tcase(s, tc_call);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
