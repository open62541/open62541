#include <stdlib.h>

#include "ua_types.h"
#include "ua_client.h"
#include "check.h"

START_TEST(EndpointUrl_split) {
        // check for null
        ck_assert_uint_eq(UA_EndpointUrl_split(NULL, NULL, NULL, NULL), UA_STATUSCODE_BADINVALIDARGUMENT);

        char hostname[256];
        UA_UInt16 port;
        const char* path;

        // check for max url length
        // this string has 256 chars
        char *overlength = "wEgfH2Sqe8AtFcUqX6VnyvZz6A4AZtbKRvGwQWvtPLrt7aaLb6wtqFzqQ2dLYLhTwJpAuVbsRTGfjvP2kvsVSYQLLeGuPjJyYnMt5e8TqtmYuPTb78uuAx7KyQB9ce95eacs3Jp32KMNtb7BTuKjQ236MnMX3mFWYAkALcj5axpQnFaGyU3HvpYrX24FTEztuZ3zpNnqBWQyHPVa6efGTzmUXMADxjw3AbG5sTGzDca7rucsfQRAZby8ZWKm66pV";
        ck_assert_uint_eq(UA_EndpointUrl_split(overlength, hostname, &port, &path), UA_STATUSCODE_BADOUTOFRANGE);

        // check for too short url
        ck_assert_uint_eq(UA_EndpointUrl_split("inv.ali:/", hostname, &port, &path), UA_STATUSCODE_BADOUTOFRANGE);

        // check for opc.tcp:// protocol
        ck_assert_uint_eq(UA_EndpointUrl_split("inv.ali://", hostname, &port, &path), UA_STATUSCODE_BADATTRIBUTEIDINVALID);

        // empty url
        ck_assert_uint_eq(UA_EndpointUrl_split("opc.tcp://", hostname, &port, &path), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(strlen(hostname), 0);
        ck_assert_uint_eq(port, 0);
        ck_assert_ptr_eq(path, NULL);

        // only hostname
        ck_assert_uint_eq(UA_EndpointUrl_split("opc.tcp://hostname", hostname, &port, &path), UA_STATUSCODE_GOOD);
        ck_assert_str_eq(hostname,"hostname");
        ck_assert_uint_eq(port, 0);
        ck_assert_ptr_eq(path, NULL);

        // empty port
        ck_assert_uint_eq(UA_EndpointUrl_split("opc.tcp://hostname:", hostname, &port, &path), UA_STATUSCODE_GOOD);
        ck_assert_str_eq(hostname,"hostname");
        ck_assert_uint_eq(port, 0);
        ck_assert_ptr_eq(path, NULL);

        // specific port
        ck_assert_uint_eq(UA_EndpointUrl_split("opc.tcp://hostname:1234", hostname, &port, &path), UA_STATUSCODE_GOOD);
        ck_assert_str_eq(hostname,"hostname");
        ck_assert_uint_eq(port, 1234);
        ck_assert_ptr_eq(path, NULL);

        // empty hostname
        ck_assert_uint_eq(UA_EndpointUrl_split("opc.tcp://:", hostname, &port, &path), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(strlen(hostname),0);
        ck_assert_uint_eq(port, 0);
        ck_assert_ptr_eq(path, NULL);

        // empty hostname and no port
        ck_assert_uint_eq(UA_EndpointUrl_split("opc.tcp:///", hostname, &port, &path), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(strlen(hostname),0);
        ck_assert_uint_eq(port, 0);
        ck_assert_str_eq(path,"/");

        // overlength port
        ck_assert_uint_eq(UA_EndpointUrl_split("opc.tcp://hostname:12345678", hostname, &port, &path), UA_STATUSCODE_BADOUTOFRANGE);

        // too high port
        ck_assert_uint_eq(UA_EndpointUrl_split("opc.tcp://hostname:65536", hostname, &port, &path), UA_STATUSCODE_BADOUTOFRANGE);

        // no port but path
        ck_assert_uint_eq(UA_EndpointUrl_split("opc.tcp://hostname/", hostname, &port, &path), UA_STATUSCODE_GOOD);
        ck_assert_str_eq(hostname,"hostname");
        ck_assert_uint_eq(port, 0);
        ck_assert_str_eq(path, "/");

        // port and path
        ck_assert_uint_eq(UA_EndpointUrl_split("opc.tcp://hostname:1234/path", hostname, &port, &path), UA_STATUSCODE_GOOD);
        ck_assert_str_eq(hostname,"hostname");
        ck_assert_uint_eq(port, 1234);
        ck_assert_str_eq(path, "/path");

        // full url, but only hostname required
        ck_assert_uint_eq(UA_EndpointUrl_split("opc.tcp://hostname:1234/path", hostname, NULL, NULL), UA_STATUSCODE_GOOD);
        ck_assert_str_eq(hostname,"hostname");

        // full url, but only hostname and port required
        ck_assert_uint_eq(UA_EndpointUrl_split("opc.tcp://hostname:1234/path", hostname, &port, NULL), UA_STATUSCODE_GOOD);
        ck_assert_str_eq(hostname,"hostname");
        ck_assert_uint_eq(port, 1234);
}
END_TEST

static Suite* testSuite_Utils(void) {
    Suite *s = suite_create("Utils");
    TCase *tc_endpointUrl_split = tcase_create("EndpointUrl_split");
    tcase_add_test(tc_endpointUrl_split, EndpointUrl_split);
    suite_add_tcase(s,tc_endpointUrl_split);
    return s;
}

int main(void) {
    Suite *s = testSuite_Utils();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
