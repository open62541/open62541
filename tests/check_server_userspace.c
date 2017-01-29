#include <stdio.h>
#include <stdlib.h>

#include "ua_types.h"
#include "ua_config_standard.h"
#include "check.h"

START_TEST(Server_addNamespace_ShallWork)
{
    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_Server *server = UA_Server_new(config);

    UA_UInt16 a = UA_Server_addNamespace(server, "http://nameOfNamespace");
    UA_UInt16 b = UA_Server_addNamespace(server, "http://nameOfNamespace");
    UA_UInt16 c = UA_Server_addNamespace(server, "http://nameOfNamespace2");

    UA_Server_delete(server);

    ck_assert_uint_gt(a, 0);
    ck_assert_uint_eq(a,b);
    ck_assert_uint_ne(a,c);
}
END_TEST

START_TEST(Server_addNamespace_writeService)
{
    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_Server *server = UA_Server_new(config);

    UA_Variant namespaces;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval = UA_Server_readValue(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY),
                        &namespaces);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(namespaces.type, &UA_TYPES[UA_TYPES_STRING]);

    namespaces.data = realloc(namespaces.data, (namespaces.arrayLength + 1) * sizeof(UA_String));
    ++namespaces.arrayLength;
    UA_String *ns = namespaces.data;
    ns[namespaces.arrayLength-1] = UA_STRING_ALLOC("test");
    size_t nsSize = namespaces.arrayLength;

    retval = UA_Server_writeValue(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY),
                                  namespaces);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_deleteMembers(&namespaces);

    /* Now read again */
    retval = UA_Server_readValue(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY),
                        &namespaces);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(namespaces.arrayLength, nsSize);

    UA_Variant_deleteMembers(&namespaces);
	UA_Server_delete(server);
}
END_TEST

static Suite* testSuite_ServerUserspace(void) {
    Suite *s = suite_create("ServerUserspace");
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, Server_addNamespace_ShallWork);
    tcase_add_test(tc_core, Server_addNamespace_writeService);

    suite_add_tcase(s,tc_core);
    return s;
}

int main(void) {
    int number_failed = 0;

    Suite *s;
    SRunner *sr;

    s = testSuite_ServerUserspace();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


