/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/nodesetloader.h>

#include <check.h>
#include <stdlib.h>

#include "testing_clock.h"
#include "test_helpers.h"

UA_Server *server = NULL;
char **nodesetPaths = NULL;
int nodesetsNum = 0;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Server_loadInputNodesets) {
    for (int cnt = 0; cnt < nodesetsNum; cnt++) {
        UA_StatusCode retVal = UA_Server_loadNodeset(server, nodesetPaths[cnt], NULL);
        ck_assert(UA_StatusCode_isGood(retVal));
    }
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server Nodeset Loader");
    TCase *tc_server = tcase_create("Server load input nodesets");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_loadInputNodesets);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        nodesetPaths = (char**)malloc(sizeof(char*));
        nodesetPaths[0] = OPEN62541_NODESET_DIR "DI/Opc.Ua.Di.NodeSet2.xml";
        nodesetsNum = 1;
    }
    else {
        nodesetPaths = &argv[1];
        nodesetsNum = argc - 1;
    }
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
