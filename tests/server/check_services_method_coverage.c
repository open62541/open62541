/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 *
 * Coverage tests for src/server/ua_services_method.c error paths that
 * the existing 14-test check_services_call.c suite does not exercise:
 *
 *   - getArgumentsNodeCallback: the "found" branch that returns a
 *     variable node reference. The existing tests use methods
 *     WITHOUT defined InputArguments, so the callback is never
 *     invoked successfully.
 *   - checkAdjustArguments: the type-mismatch branch that returns
 *     BADINVALIDARGUMENT. The existing tests use a method without
 *     input arguments and so skip the type check entirely.
 *   - The "args mismatch" early returns in callWithResolvedMethodAndObject.
 *
 * Each test adds its own method to a fresh sub-Object so the previous
 * test's argument definitions cannot leak across.
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#include "test_helpers.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

static UA_Server *server;
static UA_NodeId testObjectId;
static UA_NodeId methodWithInt32In;
static UA_NodeId methodNoArgs;
static UA_UInt32 lastMethodCalled = 0;

static UA_StatusCode
testMethodCallback(UA_Server *s, const UA_NodeId *sessionId, void *sessionHandle,
                   const UA_NodeId *methodId, void *methodContext,
                   const UA_NodeId *objectId, void *objectContext,
                   size_t inputSize, const UA_Variant *input,
                   size_t outputSize, UA_Variant *output) {
    (void)s; (void)sessionId; (void)sessionHandle; (void)methodId;
    (void)methodContext; (void)objectId; (void)objectContext;
    (void)input; (void)output; (void)inputSize; (void)outputSize;
    lastMethodCalled = 1;
    return UA_STATUSCODE_GOOD;
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
    UA_Server_run_startup(server);
    lastMethodCalled = 0;

    /* Create a parent Object that will own our test methods. */
    UA_ObjectAttributes objAttr = UA_ObjectAttributes_default;
    objAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MethodTestObj");
    UA_NodeId objId;
    UA_StatusCode res = UA_Server_addObjectNode(
        server, UA_NODEID_NULL, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "MethodTestObj"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        objAttr, NULL, &objId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_copy(&objId, &testObjectId);

    /* Method #1: takes one Int32 input argument. */
    UA_Argument inArg;
    UA_Argument_init(&inArg);
    inArg.name = UA_STRING("x");
    inArg.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    inArg.valueRank = -1; /* scalar */
    UA_MethodAttributes methAttr = UA_MethodAttributes_default;
    methAttr.displayName = UA_LOCALIZEDTEXT("en-US", "methodWithInt32In");
    methAttr.executable = true;
    res = UA_Server_addMethodNode(
        server, UA_NODEID_NULL, objId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "methodWithInt32In"),
        methAttr, testMethodCallback,
        1, &inArg, 0, NULL,
        NULL, &methodWithInt32In);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Method #2: takes no arguments. */
    methAttr.displayName = UA_LOCALIZEDTEXT("en-US", "methodNoArgs");
    res = UA_Server_addMethodNode(
        server, UA_NODEID_NULL, objId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "methodNoArgs"),
        methAttr, testMethodCallback,
        0, NULL, 0, NULL,
        NULL, &methodNoArgs);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    UA_NodeId_clear(&testObjectId);
    UA_NodeId_clear(&methodWithInt32In);
    UA_NodeId_clear(&methodNoArgs);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/* ==== Tests ==== */

/* Calling a no-argument method with arguments must return
 * BADTOOMANYARGUMENTS. This path is hit in callWithResolvedMethodAndObject
 * (line 181 of ua_services_method.c in the source tree). */
START_TEST(Call_noArgsMethod_withArgs_rejected) {
    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = testObjectId;
    req.methodId = methodNoArgs;

    /* Inject one bogus argument. */
    UA_Variant bogus;
    UA_Variant_init(&bogus);
    UA_Int32 v = 42;
    UA_Variant_setScalarCopy(&bogus, &v, &UA_TYPES[UA_TYPES_INT32]);
    req.inputArgumentsSize = 1;
    req.inputArguments = &bogus;

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &req);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_BADTOOMANYARGUMENTS);
    /* Method callback must not have run. */
    ck_assert_uint_eq(lastMethodCalled, 0);

    UA_CallMethodResult_clear(&result);
    UA_Variant_clear(&bogus);
} END_TEST

/* Calling a method with fewer arguments than its definition requires
 * returns BADARGUMENTSMISSING. The arguments are size-checked before
 * the type-check so a wrong count is reported first. */
START_TEST(Call_method_missingArgs_rejected) {
    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = testObjectId;
    req.methodId = methodWithInt32In;
    /* No input arguments supplied. */

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &req);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_BADARGUMENTSMISSING);
    ck_assert_uint_eq(lastMethodCalled, 0);

    UA_CallMethodResult_clear(&result);
} END_TEST

/* Calling a method with the right number of arguments but the wrong
 * type must return BADINVALIDARGUMENT. This exercises
 * checkAdjustArguments: getArgumentsVariableNode finds the input-
 * argument description, the type comparison fails, and the per-
 * argument inputArgumentResults entry is filled with
 * BADTYPEMISMATCH. */
START_TEST(Call_method_typeMismatch_rejected) {
    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = testObjectId;
    req.methodId = methodWithInt32In;

    /* The method wants an Int32 but we send a String. */
    UA_Variant wrongType;
    UA_Variant_init(&wrongType);
    UA_String s = UA_STRING("not an int");
    UA_Variant_setScalarCopy(&wrongType, &s, &UA_TYPES[UA_TYPES_STRING]);
    req.inputArgumentsSize = 1;
    req.inputArguments = &wrongType;

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &req);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_BADINVALIDARGUMENT);
    /* The per-argument result array is set so the caller can see
     * exactly which argument failed. */
    ck_assert_uint_eq(result.inputArgumentResultsSize, 1);
    ck_assert(result.inputArgumentResults != NULL);
    ck_assert_uint_eq(result.inputArgumentResults[0],
                     UA_STATUSCODE_BADTYPEMISMATCH);
    ck_assert_uint_eq(lastMethodCalled, 0);

    UA_CallMethodResult_clear(&result);
    UA_Variant_clear(&wrongType);
} END_TEST

/* Calling a method with the right number and type of arguments must
 * succeed and invoke the callback. This is the success path of
 * getArgumentsNodeCallback + checkAdjustArguments + the actual
 * method dispatch. */
START_TEST(Call_method_correctArgs_succeeds) {
    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = testObjectId;
    req.methodId = methodWithInt32In;

    UA_Variant correctArg;
    UA_Variant_init(&correctArg);
    UA_Int32 v = 7;
    UA_Variant_setScalarCopy(&correctArg, &v, &UA_TYPES[UA_TYPES_INT32]);
    req.inputArgumentsSize = 1;
    req.inputArguments = &correctArg;

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &req);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(lastMethodCalled, 1);

    UA_CallMethodResult_clear(&result);
    UA_Variant_clear(&correctArg);
} END_TEST

/* ==== UA_MAX_METHOD_ARGUMENTS guard ==== */

START_TEST(Call_method_exceedsMaxArgs_rejected) {
    /* src/server/ua_services_method.c:181-182:
     *   if(request->inputArgumentsSize > UA_MAX_METHOD_ARGUMENTS)
     *     return UA_STATUSCODE_BADTOOMANYARGUMENTS;
     * UA_MAX_METHOD_ARGUMENTS is 64 (defined in ua_services_method.c,
     * not exposed in a public header). The existing
     * Call_method_typeMismatch_rejected / Call_method_missingArgs_rejected
     * use 0-1 args; the >64 branch is not exercised. */
    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = testObjectId;
    req.methodId = methodWithInt32In;

    /* 65 args exceeds UA_MAX_METHOD_ARGUMENTS (= 64). */
    const size_t size = 65;
    UA_Variant args[65];
    memset(args, 0, sizeof(args));
    for(size_t i = 0; i < size; i++) {
        UA_Variant_init(&args[i]);
    }
    req.inputArgumentsSize = size;
    req.inputArguments = args;

    UA_CallMethodResult result = UA_Server_call(server, &req);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_BADTOOMANYARGUMENTS);

    UA_CallMethodResult_clear(&result);
    for(size_t i = 0; i < size; i++)
        UA_Variant_clear(&args[i]);
} END_TEST

/* ==== Method node with wrong NodeClass ==== */

START_TEST(Call_methodNode_wrongNodeClass_rejected) {
    /* src/server/ua_services_method.c: ~270-271 (Operation_CallMethod):
     * The method's browseName is fetched via lookup; if the resulting
     * node is not UA_NODECLASS_METHOD, the call is rejected. The
     * existing tests use a valid methodId (the method is a
     * UA_NODECLASS_METHOD node) -- they don't exercise the
     * 'methodId points to a non-method node' branch.
     * We use one of the object nodes we created earlier as a
     * bogus methodId. */
    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = testObjectId;
    req.methodId = testObjectId; /* not a method node */

    UA_CallMethodResult result = UA_Server_call(server, &req);
    ck_assert(result.statusCode != UA_STATUSCODE_GOOD);

    UA_CallMethodResult_clear(&result);
} END_TEST

/* ==== Suite ==== */

static Suite *
testSuite(void) {
    Suite *s = suite_create("Services Method Coverage");
    TCase *tc = tcase_create("Operation_CallMethod error paths");
    tcase_set_timeout(tc, 30);
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, Call_noArgsMethod_withArgs_rejected);
    tcase_add_test(tc, Call_method_missingArgs_rejected);
    tcase_add_test(tc, Call_method_typeMismatch_rejected);
    tcase_add_test(tc, Call_method_correctArgs_succeeds);
    tcase_add_test(tc, Call_method_exceedsMaxArgs_rejected);
    tcase_add_test(tc, Call_methodNode_wrongNodeClass_rejected);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
