#include "ua_server.h"
#include "server/ua_server_internal.h"
#include "ua_config_standard.h"

#include "check.h"
#include <unistd.h>

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
    UA_Server_addRepeatedJob(server, rj, 10, &id);

    usleep(15*1000);
    UA_Server_run_iterate(server, false);

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

    usleep(15*1000);
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
