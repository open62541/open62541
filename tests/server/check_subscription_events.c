/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_server.h"
#include "server/ua_services.h"
#include "server/ua_server_internal.h"
#include "server/ua_subscription.h"
#include "ua_config_default.h"

#include "check.h"
#include "testing_clock.h"

static UA_Server *server = NULL;
static UA_ServerConfig *config = NULL;

static void setup(void) {
    config = UA_ServerConfig_new_default();
    server = UA_Server_new(config);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
}

#ifdef UA_ENABLE_SUBSCRIPTIONS
#ifdef UA_ENABLE_EVENTS



#endif //UA_ENABLE_EVENTS
#endif //UA_ENABLE_SUBSCRIPTIONS

//assumes subscriptions work fine with data change because of other unit test
static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server Subscription Events");
    TCase *tc_server = tcase_create("Server Subscription Events");
    tcase_add_checked_fixture(tc_server, setup, teardown);
#ifdef UA_ENABLE_SUBSCRIPTIONS
#ifdef UA_ENABLE_EVENTS
#endif //UA_ENABLE_EVENTS
#endif //UA_ENABLE_SUBSCRIPTIONS
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


#endif //UA_ENABLE_EVENTS
#endif //UA_ENABLE_SUBSCRIPTIONS
