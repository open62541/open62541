/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>

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
START_TEST(LoadFromFile_MalformedNestedFieldName_NoCrash) {
    const char *json = "{\"buildInfo\":{\"\\uD800\":0}}";
    UA_ByteString jsonConfig = UA_STRING((char*)(uintptr_t)json);
    jsonConfig.length = strlen(json);

    UA_ServerConfig config;

    /* The malformed field is skipped (logged, not rejected); parsing still
     * reports success. What matters here is that this returns at all,
     * without an OOB read/crash. */
    UA_StatusCode retval = UA_ServerConfig_loadFromFile(&config, jsonConfig);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_ServerConfig_clear(&config);
} END_TEST

START_TEST(NewFromFile_MalformedNestedFieldName_NoCrash) {
    const char *json = "{\"buildInfo\":{\"\\uD800\":0}}";
    UA_ByteString jsonConfig = UA_STRING((char*)(uintptr_t)json);
    jsonConfig.length = strlen(json);

    UA_Server *server = UA_Server_newFromFile(jsonConfig);
    ck_assert(server != NULL); /* malformed field is skipped, server still starts */
    UA_Server_delete(server);
} END_TEST

START_TEST(LoadFromFile_ValidBuildInfo_StillWorks) {
    const char *json = "{\"buildInfo\":{\"productUri\":\"urn:test\"}}";
    UA_ByteString jsonConfig = UA_STRING((char*)(uintptr_t)json);
    jsonConfig.length = strlen(json);

    UA_ServerConfig config;
    UA_StatusCode retval = UA_ServerConfig_loadFromFile(&config, jsonConfig);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_String expected = UA_STRING((char*)(uintptr_t)"urn:test");
    ck_assert(UA_String_equal(&config.buildInfo.productUri, &expected));

    UA_ServerConfig_clear(&config);
} END_TEST

#ifdef UA_ENABLE_LWS
START_TEST(LoadFromFile_HttpListenAddress) {
    const char *json =
        "{\"httpEnabled\":true,\"httpAllowUnencrypted\":true,"
        "\"http\":{\"listenAddress\":\"\","
        "\"maxMsgSize\":4096,\"maxDecompressedMsgSize\":8192,"
        "\"timeout\":15}}";
    UA_ByteString jsonConfig = UA_STRING((char *)(uintptr_t)json);
    jsonConfig.length = strlen(json);

    UA_ServerConfig config;
    UA_StatusCode retval = UA_ServerConfig_loadFromFile(&config, jsonConfig);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(config.httpEnabled);
    ck_assert(config.httpAllowUnencrypted);
    ck_assert_ptr_nonnull(config.httpListenAddress.data);
    ck_assert_uint_eq(config.httpListenAddress.length, 0);
    ck_assert_uint_eq(config.httpMaxMsgSize, 4096);
    ck_assert_uint_eq(config.httpMaxDecompressedMsgSize, 8192);
    ck_assert_uint_eq(config.httpTimeout, 15);

    UA_ServerConfig_clear(&config);
} END_TEST
#endif

#ifdef UA_ENABLE_RBAC

static UA_StatusCode
loadJson(const char *json, UA_ServerConfig *config) {
    UA_ByteString jsonConfig = UA_STRING((char*)(uintptr_t)json);
    jsonConfig.length = strlen(json);
    return UA_ServerConfig_loadFromFile(config, jsonConfig);
}

/* A complete "rbac" object round-trips into the UA_ServerConfig RBAC fields. */
START_TEST(Rbac_FullConfigParsed) {
    const char *json =
        "{\"rbac\":{"
        "  \"allPermissionsForAnonymous\": false,"
        "  \"roles\": [{"
        "    \"roleId\": \"ns=1;i=5000\","
        "    \"roleName\": \"JsonOperator\","
        "    \"roleNameNamespaceIndex\": 1,"
        "    \"identityMappingRules\": ["
        "      {\"criteriaType\": \"UserName\", \"criteria\": \"alice\"},"
        "      {\"criteriaType\": \"GroupId\", \"criteria\": \"ops\"}],"
        "    \"applicationsExclude\": false,"
        "    \"applications\": [\"urn:app:one\", \"urn:app:two\"],"
        "    \"endpointsExclude\": false,"
        "    \"endpoints\": [{\"endpointUrl\": \"opc.tcp://localhost:4840\","
        "                     \"securityMode\": \"SignAndEncrypt\","
        "                     \"securityPolicyUri\": \"http://policy\","
        "                     \"transportProfileUri\": \"http://transport\"}],"
        "    \"customConfiguration\": true"
        "  }],"
        "  \"rolePermissionPresets\": [{\"rolePermissions\": ["
        "      {\"roleId\": \"ns=1;i=5000\", \"permissions\": 3},"
        "      {\"roleId\": \"i=15644\", \"permissions\": 7}]}]"
        "},"
        /* Parsed after "rbac" - guards against a desynchronized token walk */
        "\"buildInfo\":{\"productUri\":\"urn:test\"}}";

    UA_ServerConfig config;
    ck_assert_int_eq(loadJson(json, &config), UA_STATUSCODE_GOOD);

    ck_assert(!config.allPermissionsForAnonymous);
    ck_assert_uint_eq(config.rolesSize, 1);

    const UA_Role *role = &config.roles[0];
    UA_NodeId expectedId = UA_NODEID_NUMERIC(1, 5000);
    ck_assert(UA_NodeId_equal(&role->roleId, &expectedId));
    UA_QualifiedName expectedName = UA_QUALIFIEDNAME(1, "JsonOperator");
    ck_assert(UA_QualifiedName_equal(&role->roleName, &expectedName));
    ck_assert(role->customConfiguration);

    ck_assert_uint_eq(role->identityMappingRulesSize, 2);
    ck_assert_uint_eq(role->identityMappingRules[0].criteriaType,
                      UA_IDENTITYCRITERIATYPE_USERNAME);
    UA_String alice = UA_STRING((char*)(uintptr_t)"alice");
    ck_assert(UA_String_equal(&role->identityMappingRules[0].criteria, &alice));
    ck_assert_uint_eq(role->identityMappingRules[1].criteriaType,
                      UA_IDENTITYCRITERIATYPE_GROUPID);

    ck_assert(!role->applicationsExclude);
    ck_assert_uint_eq(role->applicationsSize, 2);
    UA_String appTwo = UA_STRING((char*)(uintptr_t)"urn:app:two");
    ck_assert(UA_String_equal(&role->applications[1], &appTwo));

    ck_assert(!role->endpointsExclude);
    ck_assert_uint_eq(role->endpointsSize, 1);
    ck_assert_uint_eq(role->endpoints[0].securityMode,
                      UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
    UA_String epUrl = UA_STRING((char*)(uintptr_t)"opc.tcp://localhost:4840");
    ck_assert(UA_String_equal(&role->endpoints[0].endpointUrl, &epUrl));

    ck_assert_uint_eq(config.rolePermissionPresetsSize, 1);
    ck_assert_uint_eq(config.rolePermissionPresets[0].rolePermissionsSize, 2);
    ck_assert_uint_eq(config.rolePermissionPresets[0].rolePermissions[0].permissions, 3);
    UA_NodeId wellKnown = UA_NODEID_NUMERIC(0, 15644);
    ck_assert(UA_NodeId_equal(&config.rolePermissionPresets[0].rolePermissions[1].roleId,
                              &wellKnown));

    /* The token walk stayed in sync with the enclosing config object */
    UA_String expectedUri = UA_STRING((char*)(uintptr_t)"urn:test");
    ck_assert(UA_String_equal(&config.buildInfo.productUri, &expectedUri));

    UA_ServerConfig_clear(&config);
} END_TEST

/* The role name and its namespace index are independent fields and may appear
 * in any order. */
START_TEST(Rbac_RoleNameNamespaceIndexOrderIndependent) {
    const char *json =
        "{\"rbac\":{\"roles\":[{\"roleNameNamespaceIndex\": 2,"
        "                       \"roleName\": \"Late\"}]}}";
    UA_ServerConfig config;
    ck_assert_int_eq(loadJson(json, &config), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config.rolesSize, 1);
    UA_QualifiedName expected = UA_QUALIFIEDNAME(2, "Late");
    ck_assert(UA_QualifiedName_equal(&config.roles[0].roleName, &expected));
    UA_ServerConfig_clear(&config);
} END_TEST

/* Empty arrays are valid and must not leave a dangling/sentinel array behind
 * that the UA_Role cleanup would then free. */
START_TEST(Rbac_EmptyArrays) {
    const char *json =
        "{\"rbac\":{\"roles\":[{\"roleName\":\"Empty\","
        "                       \"identityMappingRules\":[],"
        "                       \"applications\":[],"
        "                       \"endpoints\":[]}],"
        "           \"rolePermissionPresets\":[{\"rolePermissions\":[]}]}}";
    UA_ServerConfig config;
    ck_assert_int_eq(loadJson(json, &config), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config.rolesSize, 1);
    ck_assert_uint_eq(config.roles[0].identityMappingRulesSize, 0);
    ck_assert_uint_eq(config.roles[0].applicationsSize, 0);
    ck_assert_uint_eq(config.roles[0].endpointsSize, 0);
    ck_assert_uint_eq(config.rolePermissionPresetsSize, 1);
    ck_assert_uint_eq(config.rolePermissionPresets[0].rolePermissionsSize, 0);
    UA_ServerConfig_clear(&config);
} END_TEST

/* A repeated key replaces the previous value instead of appending to an array
 * that was already sized for the first occurrence. */
START_TEST(Rbac_RepeatedKeyReplacesValue) {
    const char *json =
        "{\"rbac\":{\"roles\":[{\"roleName\":\"First\"}],"
        "           \"roles\":[{\"roleName\":\"Second\"},{\"roleName\":\"Third\"}]}}";
    UA_ServerConfig config;
    ck_assert_int_eq(loadJson(json, &config), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config.rolesSize, 2);
    UA_QualifiedName second = UA_QUALIFIEDNAME(0, "Second");
    ck_assert(UA_QualifiedName_equal(&config.roles[0].roleName, &second));
    UA_ServerConfig_clear(&config);
} END_TEST

/* An unknown field with an object value is skipped as a whole; the fields
 * after it are still parsed. */
START_TEST(Rbac_UnknownObjectFieldSkipped) {
    const char *json =
        "{\"rbac\":{\"roles\":[{\"roleName\":\"R\",\"bogus\":{\"x\":1},"
        "                       \"customConfiguration\":true}]}}";
    UA_ServerConfig config;
    ck_assert_int_eq(loadJson(json, &config), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config.rolesSize, 1);
    ck_assert(config.roles[0].customConfiguration);
    UA_ServerConfig_clear(&config);
} END_TEST

/* Malformed values are rejected instead of silently producing a half-parsed
 * role registry. */
START_TEST(Rbac_MalformedValuesRejected) {
    UA_ServerConfig config;

    /* An array-valued field that is not an array */
    ck_assert_int_eq(loadJson("{\"rbac\":{\"roles\":{\"a\":1}}}", &config),
                     UA_STATUSCODE_BADDECODINGERROR);
    ck_assert_uint_eq(config.rolesSize, 0);
    UA_ServerConfig_clear(&config);

    /* An unknown IdentityCriteriaType name */
    ck_assert(loadJson("{\"rbac\":{\"roles\":[{\"roleName\":\"R\","
                       "\"identityMappingRules\":[{\"criteriaType\":\"Nope\"}]}]}}",
                       &config) != UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config.rolesSize, 0);
    UA_ServerConfig_clear(&config);
} END_TEST

/* The parsed roles reach the running server's role registry. */
START_TEST(Rbac_ConfigRolesReachTheServer) {
    const char *json =
        "{\"rbac\":{\"roles\":[{\"roleId\":\"ns=1;i=6100\",\"roleName\":\"JsonRole\","
        "  \"identityMappingRules\":[{\"criteriaType\":\"AuthenticatedUser\"}]}]}}";
    UA_ByteString jsonConfig = UA_STRING((char*)(uintptr_t)json);
    jsonConfig.length = strlen(json);

    UA_Server *server = UA_Server_newFromFile(jsonConfig);
    ck_assert_ptr_ne(server, NULL);

    UA_Role role;
    ck_assert_int_eq(UA_Server_getRoleById(server, UA_NODEID_NUMERIC(1, 6100), &role),
                     UA_STATUSCODE_GOOD);
    UA_QualifiedName expected = UA_QUALIFIEDNAME(0, "JsonRole");
    ck_assert(UA_QualifiedName_equal(&role.roleName, &expected));
    ck_assert_uint_eq(role.identityMappingRulesSize, 1);
    UA_Role_clear(&role);

    /* Config roles are protected and cannot be removed at runtime */
    ck_assert_int_eq(UA_Server_removeRole(server, expected),
                     UA_STATUSCODE_BADREQUESTNOTALLOWED);

    UA_Server_delete(server);
} END_TEST

#endif /* UA_ENABLE_RBAC */

static Suite *testSuite_ServerConfigJson(void) {
    Suite *s = suite_create("Server config from JSON5 file");

    TCase *tc = tcase_create("Malformed input regressions");
    tcase_add_test(tc, LoadFromFile_MalformedNestedFieldName_NoCrash);
    tcase_add_test(tc, NewFromFile_MalformedNestedFieldName_NoCrash);
    tcase_add_test(tc, LoadFromFile_ValidBuildInfo_StillWorks);
#ifdef UA_ENABLE_LWS
    tcase_add_test(tc, LoadFromFile_HttpListenAddress);
#endif
    suite_add_tcase(s, tc);

#ifdef UA_ENABLE_RBAC
    TCase *tcRbac = tcase_create("RBAC configuration");
    tcase_add_test(tcRbac, Rbac_FullConfigParsed);
    tcase_add_test(tcRbac, Rbac_RoleNameNamespaceIndexOrderIndependent);
    tcase_add_test(tcRbac, Rbac_EmptyArrays);
    tcase_add_test(tcRbac, Rbac_RepeatedKeyReplacesValue);
    tcase_add_test(tcRbac, Rbac_UnknownObjectFieldSkipped);
    tcase_add_test(tcRbac, Rbac_MalformedValuesRejected);
    tcase_add_test(tcRbac, Rbac_ConfigRolesReachTheServer);
    suite_add_tcase(s, tcRbac);
#endif

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
