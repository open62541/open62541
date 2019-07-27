/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>
#include <time.h>

#include "check.h"
#include "testing_clock.h"
#include "unistd.h"

#include <open62541/plugin/nodesetLoader.h>

#define BIGXML NODESETPATH "/100kNodes.xml"

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

static UA_UInt16 incrementNamespace(UA_Server* srv, const char* ur)
{
    static UA_UInt16 idx = 1;
    idx++;
    return idx;
}

START_TEST(Server_ImportNodeset) {
    FileHandler f;
    f.addNamespace = incrementNamespace;
    f.server = server;
    f.file = BIGXML;
    clock_t begin, finish;
    begin = clock();
    for(size_t cnt = 0; cnt < 10; cnt++)
    {
        UA_StatusCode retval = UA_XmlImport_loadFile(&f);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }        
    finish = clock();
    double time_spent = (double)(finish - begin) / CLOCKS_PER_SEC;
    printf("duration was %f s\n", time_spent);
    UA_Boolean running = true;
    UA_Server_run(server, &running);
}
END_TEST

static Suite *testSuite_Client(void) {
    Suite *s = suite_create("Server Nodeset Import speed");
    TCase *tc_server = tcase_create("Server Import speed");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_ImportNodeset);
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
