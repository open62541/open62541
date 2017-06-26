/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this 
*  file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include "ua_server.h"
#include "server/ua_server_internal.h"
#include "ua_config_standard.h"

#include "check.h"
#include "testing_clock.h"

UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_new(UA_ServerConfig_standard);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

UA_Boolean *executed;

static void
dummyJob(UA_Server *serverPtr, void *data) {
    *executed = true;
}

START_TEST(Server_addRemoveRepeatedJob) {
    executed = UA_Boolean_new();
    UA_Guid id;
    UA_Job rj = (UA_Job){
        .type = UA_JOBTYPE_METHODCALL,
        .job.methodCall = {.data = NULL, .method = dummyJob}
    };
    /* The job is added to the main queue only upon the next run_iterate */
    UA_Server_addRepeatedJob(server, rj, 10, &id);
    UA_Server_run_iterate(server, false);

    /* Wait until the job has surely timed out */
    UA_sleep(15);
    UA_Server_run_iterate(server, false);

    /* Wait a bit longer until the workers have picked up the dispatched job */
    UA_sleep(15);
    ck_assert_uint_eq(*executed, true);

    UA_Server_removeRepeatedJob(server, id);
    UA_Boolean_delete(executed);
}
END_TEST

UA_Guid *jobId;

static void
removeItselfJob(UA_Server *serverPtr, void *data) {
    UA_Server_removeRepeatedJob(serverPtr, *jobId);
}

START_TEST(Server_repeatedJobRemoveItself) {
    jobId = UA_Guid_new();
    UA_Job rj = (UA_Job){
        .type = UA_JOBTYPE_METHODCALL,
        .job.methodCall = {.data = NULL, .method = removeItselfJob}
    };
    UA_Server_addRepeatedJob(server, rj, 10, jobId);

    UA_sleep(15);
    UA_Server_run_iterate(server, false);

    UA_Guid_delete(jobId);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server Jobs");
    TCase *tc_server = tcase_create("Server Repeated Jobs");
    tcase_add_checked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_addRemoveRepeatedJob);
    tcase_add_test(tc_server, Server_repeatedJobRemoveItself);
    suite_add_tcase(s, tc_server);
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
