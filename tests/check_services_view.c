#include <stdio.h>
#include <stdlib.h>

#include "check.h"
#include "ua_server.h"
#include "ua_config_standard.h"

START_TEST(Service_Browse_WithBrowseName)
{
    UA_Server * server = UA_Server_new(UA_ServerConfig_standard);

    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.resultMask = UA_BROWSERESULTMASK_BROWSENAME;
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);

    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert(br.referencesSize > 0);
    ck_assert(!UA_String_equal(&br.references[0].browseName.name, &UA_STRING_NULL));

    UA_BrowseResult_deleteMembers(&br);
    UA_Server_delete(server);
}
END_TEST

static Suite* testSuite_Service_TranslateBrowsePathsToNodeIds(void) {
    Suite *s = suite_create("Service_TranslateBrowsePathsToNodeIds");
    TCase *tc_browse = tcase_create("Browse Service");
    tcase_add_test(tc_browse, Service_Browse_WithBrowseName);

    suite_add_tcase(s,tc_browse);
    return s;
}

int main(void) {
    int number_failed = 0;

    Suite *s;
    SRunner *sr;

    s = testSuite_Service_TranslateBrowsePathsToNodeIds();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


