/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"

#include <check.h>
#include <stdlib.h>

#include "test_helpers.h"

/* The Session authenticationToken must not be a pure function of the RNG
 * seed. UA_random_seed_deterministic() pins the RNG to a fixed value with
 * no real-time mixing, so two servers seeded the same way represent two
 * processes whose seed (derived from the start time in normal operation)
 * happened to coincide. If the token were still generated with the plain
 * PCG32 RNG, both would compute the identical first authenticationToken
 * instead of two independent, non-repeating values. */
START_TEST(Session_authenticationToken_notSeedDerivable) {
    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);
    request.requestedSessionTimeout = UA_UINT32_MAX;

    UA_Server *server1 = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server1, NULL);
    UA_Server_run_startup(server1);
    UA_random_seed_deterministic(1234567890ULL);

    UA_Session *session1 = NULL;
    lockServer(server1);
    UA_StatusCode retval = UA_Session_create(server1, NULL, &request, &session1);
    unlockServer(server1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_NodeId token1;
    UA_NodeId_copy(&session1->authenticationToken, &token1);

    UA_Server_run_shutdown(server1);
    UA_Server_delete(server1);

    UA_Server *server2 = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server2, NULL);
    UA_Server_run_startup(server2);
    UA_random_seed_deterministic(1234567890ULL);

    UA_Session *session2 = NULL;
    lockServer(server2);
    retval = UA_Session_create(server2, NULL, &request, &session2);
    unlockServer(server2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Boolean equal = UA_NodeId_equal(&token1, &session2->authenticationToken);
    ck_assert_msg(!equal, "two sessions with the same RNG seed produced the "
                          "same authenticationToken - it must not be a pure "
                          "function of the seed");

    UA_NodeId_clear(&token1);
    UA_Server_run_shutdown(server2);
    UA_Server_delete(server2);
} END_TEST

static Suite* testSuite_SessionAuthTokenRng(void) {
    Suite *s = suite_create("SessionAuthTokenRng");
    TCase *tc = tcase_create("Core");
    tcase_add_test(tc, Session_authenticationToken_notSeedDerivable);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    int number_failed = 0;

    Suite *s = testSuite_SessionAuthTokenRng();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
