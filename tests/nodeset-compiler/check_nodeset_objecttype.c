/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "open62541.h"
#include "check_nodeset_objecttype_generated.h"
#include "check.h"

static UA_Server *server = NULL;
static UA_ServerConfig *config = NULL;

static void setup(void) {
    config = UA_ServerConfig_new_default();
    server = UA_Server_new(config);
    UA_StatusCode retval = check_nodeset_objecttype_generated(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
}

START_TEST(checkObjectTypeExists) {
    UA_NodeClass nc = UA_NODECLASS_UNSPECIFIED;
    UA_StatusCode retval = UA_Server_readNodeClass(server, UA_NODEID_NUMERIC(2, 1001), &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECTTYPE);
} END_TEST

int main(void) {
    Suite *s = suite_create("XML-Generated ObjectType");
    TCase *tc_objecttype = tcase_create("objecttype tests");
    tcase_add_checked_fixture(tc_objecttype, setup, teardown);
    tcase_add_test(tc_objecttype, checkObjectTypeExists);
    suite_add_tcase(s, tc_objecttype);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
