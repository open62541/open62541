/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/server_config_file_based.h>

#include <check.h>
#include <string.h>

/* API-level smoke test related to GHSA-38g6-5hfj-2fj7: a malformed field
 * name inside a nested JSON5 config object (an unpaired UTF-16 surrogate
 * escape) must not crash the process when loaded through the public
 * config-loading API.
 *
 * This is NOT a reliable regression test for the underlying OOB read on
 * this branch: BuildInfo_parseJson() only ever runs strcmp() against the
 * unterminated buffer here (the "Unknown field name." log message on
 * 1.4/1.5 is a static string, not "%s"-formatted with the field content),
 * and strcmp() against a short literal like "productUri" almost always
 * diverges within the first byte or two -- so it does not reliably walk
 * past the end of the allocation. Verified empirically: running this same
 * test against the pre-fix cj5_get_str() does NOT reproduce a crash here.
 * tests/check_cj5.c's getStr*IsTerminated tests are the actual, deterministic
 * regression guard, because they call strlen()/strcmp() on the buffer
 * directly and always exercise the failure. Keep this test for API-level
 * robustness coverage (e.g. against a future reintroduction of "%s"
 * logging, as already happened on the unreleased master branch), not as
 * the primary defense for this bug class.
 *
 * Note on the assertions below: an unrecognized field name is logged as an
 * error and otherwise silently skipped -- it does *not* fail parsing or
 * refuse to start the server. That is the parser's existing (separate,
 * pre-existing) behavior for any unknown field, not something this
 * regression test is meant to change; these tests only guard against a
 * crash/OOB-read regression in cj5_get_str(), not against permissive
 * unknown-field handling. */
START_TEST(UpdateFromFile_MalformedNestedFieldName_NoCrash) {
    const char *json = "{\"buildInfo\":{\"\\uD800\":0}}";
    UA_ByteString jsonConfig = UA_STRING((char*)(uintptr_t)json);
    jsonConfig.length = strlen(json);

    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    UA_StatusCode retval = UA_ServerConfig_setDefault(&config);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* The malformed field is skipped (logged, not rejected); parsing still
     * reports success. What matters here is that this returns at all,
     * without an OOB read/crash. */
    retval = UA_ServerConfig_updateFromFile(&config, jsonConfig);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_ServerConfig_clean(&config);
} END_TEST

START_TEST(NewFromFile_MalformedNestedFieldName_NoCrash) {
    const char *json = "{\"buildInfo\":{\"\\uD800\":0}}";
    UA_ByteString jsonConfig = UA_STRING((char*)(uintptr_t)json);
    jsonConfig.length = strlen(json);

    UA_Server *server = UA_Server_newFromFile(jsonConfig);
    ck_assert(server != NULL); /* malformed field is skipped, server still starts */
    UA_Server_delete(server);
} END_TEST

START_TEST(UpdateFromFile_ValidBuildInfo_StillWorks) {
    const char *json = "{\"buildInfo\":{\"productUri\":\"urn:test\"}}";
    UA_ByteString jsonConfig = UA_STRING((char*)(uintptr_t)json);
    jsonConfig.length = strlen(json);

    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    UA_StatusCode retval = UA_ServerConfig_setDefault(&config);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_ServerConfig_updateFromFile(&config, jsonConfig);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_String expected = UA_STRING((char*)(uintptr_t)"urn:test");
    ck_assert(UA_String_equal(&config.buildInfo.productUri, &expected));

    UA_ServerConfig_clean(&config);
} END_TEST

static Suite *testSuite_ServerConfigJson(void) {
    Suite *s = suite_create("Server config from JSON5 file");

    TCase *tc = tcase_create("Malformed input regressions");
    tcase_add_test(tc, UpdateFromFile_MalformedNestedFieldName_NoCrash);
    tcase_add_test(tc, NewFromFile_MalformedNestedFieldName_NoCrash);
    tcase_add_test(tc, UpdateFromFile_ValidBuildInfo_StillWorks);
    suite_add_tcase(s, tc);

    return s;
}

int main(void) {
    Suite *s = testSuite_ServerConfigJson();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
