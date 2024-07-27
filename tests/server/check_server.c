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
#include "test_helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "check.h"

static UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
}

static void teardown(void) {
    UA_Server_delete(server);
}

START_TEST(checkGetConfig) {
    ck_assert_ptr_eq(UA_Server_getConfig(NULL), NULL);
    ck_assert_ptr_ne(UA_Server_getConfig(server), NULL);
} END_TEST

START_TEST(checkGetNamespaceByName) {
    size_t notFoundIndex = 62541;
    UA_StatusCode notFound = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/invalid"), &notFoundIndex);
    ck_assert_uint_eq(notFoundIndex, 62541); // not changed
    ck_assert_uint_eq(notFound, UA_STATUSCODE_BADNOTFOUND);

    size_t foundIndex = 62541;
    UA_StatusCode found = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/"), &foundIndex);
    ck_assert_uint_eq(foundIndex, 0); // this namespace always has index 0 (defined by the standard)
    ck_assert_uint_eq(found, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(checkGetNamespaceById) {
    UA_String searchResultNamespace;
    UA_StatusCode notFound = UA_Server_getNamespaceByIndex(server, 10, &searchResultNamespace);
    ck_assert_uint_eq(notFound, UA_STATUSCODE_BADNOTFOUND);

    UA_StatusCode found1 = UA_Server_getNamespaceByIndex(server, 1, &searchResultNamespace);
    ck_assert_uint_eq(found1, UA_STATUSCODE_GOOD);
    UA_String_clear(&searchResultNamespace);

    UA_StatusCode notFound2 = UA_Server_getNamespaceByIndex(server, 2, &searchResultNamespace);
    ck_assert_uint_eq(notFound2, UA_STATUSCODE_BADNOTFOUND);

    UA_String compareNamespace = UA_STRING("http://opcfoundation.org/UA/");
    UA_StatusCode found = UA_Server_getNamespaceByIndex(server, 0, &searchResultNamespace);
    ck_assert(UA_String_equal(&compareNamespace, &searchResultNamespace));
    ck_assert_uint_eq(found, UA_STATUSCODE_GOOD);
    UA_String_clear(&searchResultNamespace);
} END_TEST

static void timedCallbackHandler(UA_Server *s, void *data) {
    *((UA_Boolean*)data) = false;  // stop the server via a timedCallback
}

START_TEST(checkServer_run) {
    UA_Boolean running = true;
    // 0 is in the past so the server will terminate on the first iteration
    UA_StatusCode ret;
    ret = UA_Server_addTimedCallback(server, &timedCallbackHandler, &running, 0, NULL);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
    ret = UA_Server_run(server, &running);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
} END_TEST

int main(void) {
    Suite *s = suite_create("server");

    TCase *tc_call = tcase_create("server - basics");
    tcase_add_checked_fixture(tc_call, setup, teardown);
    tcase_add_test(tc_call, checkGetConfig);
    tcase_add_test(tc_call, checkGetNamespaceByName);
    tcase_add_test(tc_call, checkGetNamespaceById);
    tcase_add_test(tc_call, checkServer_run);
    suite_add_tcase(s, tc_call);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
