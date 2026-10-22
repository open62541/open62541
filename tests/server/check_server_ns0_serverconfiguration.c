/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Bastian Engel)
 */

/* Values of the ServerConfiguration Object (OPC UA Part 12, 7.10.3) */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "test_helpers.h"

#include <check.h>
#include <stdlib.h>

static UA_Server *server;

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);

    /* Change the ApplicationDescription after the server was created. The
     * ServerConfiguration has to show the values that are used at startup. */
    UA_ApplicationDescription *ad =
        &UA_Server_getConfig(server)->applicationDescription;
    UA_String_clear(&ad->applicationUri);
    ad->applicationUri = UA_STRING_ALLOC("urn:open62541.unittest.serverconfiguration");
    UA_String_clear(&ad->productUri);
    ad->productUri = UA_STRING_ALLOC("http://open62541.org/unittest/serverconfiguration");
    UA_LocalizedText_clear(&ad->applicationName);
    ad->applicationName = UA_LOCALIZEDTEXT_ALLOC("en-US", "ServerConfiguration Test");

    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
}

static void
teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void
readServerConfigurationValue(UA_UInt32 id, UA_Variant *out) {
    UA_Variant_init(out);
    UA_StatusCode res = UA_Server_readValue(server, UA_NODEID_NUMERIC(0, id), out);
    ck_assert_str_eq(UA_StatusCode_name(res), UA_StatusCode_name(UA_STATUSCODE_GOOD));
}

/* The DataType of the Variable is UriString. A written String value is
 * adjusted to it. */
static UA_Boolean
isUriStringScalar(const UA_Variant *value) {
    return UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_STRING]) ||
           UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_URISTRING]);
}

START_TEST(applicationUri) {
    UA_Variant value;
    readServerConfigurationValue(UA_NS0ID_SERVERCONFIGURATION_APPLICATIONURI, &value);
    ck_assert(isUriStringScalar(&value));
    const UA_ApplicationDescription *ad =
        &UA_Server_getConfig(server)->applicationDescription;
    ck_assert(UA_String_equal((const UA_String*)value.data, &ad->applicationUri));
    UA_Variant_clear(&value);
} END_TEST

START_TEST(productUri) {
    UA_Variant value;
    readServerConfigurationValue(UA_NS0ID_SERVERCONFIGURATION_PRODUCTURI, &value);
    ck_assert(isUriStringScalar(&value));
    const UA_ApplicationDescription *ad =
        &UA_Server_getConfig(server)->applicationDescription;
    ck_assert(UA_String_equal((const UA_String*)value.data, &ad->productUri));
    UA_Variant_clear(&value);
} END_TEST

START_TEST(applicationNames) {
    UA_Variant value;
    readServerConfigurationValue(UA_NS0ID_SERVERCONFIGURATION_APPLICATIONNAMES, &value);
    ck_assert(UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]));
    ck_assert_uint_eq(value.arrayLength, 1);
    const UA_ApplicationDescription *ad =
        &UA_Server_getConfig(server)->applicationDescription;
    ck_assert(UA_equal(value.data, &ad->applicationName,
                       &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]));
    UA_Variant_clear(&value);
} END_TEST

/* The ServerCapabilities Property is mandatory. It contains identifiers from
 * Part 12, Annex D. "NA" cannot be combined with other capabilities. */
START_TEST(serverCapabilities) {
    UA_Variant value;
    readServerConfigurationValue(UA_NS0ID_SERVERCONFIGURATION_SERVERCAPABILITIES, &value);
    ck_assert(UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_STRING]));
    ck_assert_uint_ge(value.arrayLength, 1);
    const UA_String na = UA_STRING_STATIC("NA");
    const UA_String *capabilities = (const UA_String*)value.data;
    for(size_t i = 0; i < value.arrayLength; i++) {
        ck_assert_uint_gt(capabilities[i].length, 0);
        if(UA_String_equal(&capabilities[i], &na))
            ck_assert_uint_eq(value.arrayLength, 1);
    }
    UA_Variant_clear(&value);
} END_TEST

static Suite *
testSuite_serverConfiguration(void) {
    TCase *tc = tcase_create("ServerConfiguration");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, applicationUri);
    tcase_add_test(tc, productUri);
    tcase_add_test(tc, applicationNames);
    tcase_add_test(tc, serverCapabilities);

    Suite *s = suite_create("NS0 ServerConfiguration");
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite_serverConfiguration();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
