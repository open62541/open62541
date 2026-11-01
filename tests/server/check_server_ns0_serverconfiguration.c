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
#include "ua_server_internal.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* The ServerCapabilities are read from the started server. The expected
 * identifiers are a comma-separated list in the order of the bits. */
static void
assertServerCapabilities(const char *expected) {
    UA_Variant value;
    readServerConfigurationValue(UA_NS0ID_SERVERCONFIGURATION_SERVERCAPABILITIES, &value);
    ck_assert(UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_STRING]));
    char actual[256] = {0};
    size_t pos = 0;
    const UA_String *capabilities = (const UA_String*)value.data;
    for(size_t i = 0; i < value.arrayLength; i++) {
        ck_assert_uint_lt(pos + capabilities[i].length + 1, sizeof(actual));
        if(i > 0)
            actual[pos++] = ',';
        memcpy(actual + pos, capabilities[i].data, capabilities[i].length);
        pos += capabilities[i].length;
    }
    ck_assert_str_eq(actual, expected);
    UA_Variant_clear(&value);
}

/* No capabilities are configured by default */
START_TEST(serverCapabilities) {
    assertServerCapabilities("NA");
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

/* ServerCapabilities derived from the config at startup */

static void
startWithCapabilities(UA_ApplicationType applicationType, UA_UInt64 capabilities) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->applicationDescription.applicationType = applicationType;
    config->serverCapabilities = capabilities;
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
}

START_TEST(capabilitiesConfigured) {
    startWithCapabilities(UA_APPLICATIONTYPE_SERVER,
                          UA_SERVERCAPABILITY_DI | UA_SERVERCAPABILITY_DA |
                          UA_SERVERCAPABILITY_HD);
    assertServerCapabilities("DA,HD,DI");
    teardown();
} END_TEST

START_TEST(capabilitiesClientAndServer) {
    startWithCapabilities(UA_APPLICATIONTYPE_CLIENTANDSERVER,
                          UA_SERVERCAPABILITY_DA | UA_SERVERCAPABILITY_RCP);
    assertServerCapabilities("DA,RCP");
    teardown();
} END_TEST

/* NA cannot be combined with other capabilities */
START_TEST(capabilitiesNACombined) {
    startWithCapabilities(UA_APPLICATIONTYPE_SERVER,
                          UA_SERVERCAPABILITY_NA | UA_SERVERCAPABILITY_DA);
    assertServerCapabilities("DA");
    teardown();
} END_TEST

/* A Server cannot have the LDS capability */
START_TEST(capabilitiesLDSRemoved) {
    startWithCapabilities(UA_APPLICATIONTYPE_SERVER,
                          UA_SERVERCAPABILITY_LDS | UA_SERVERCAPABILITY_DA);
    assertServerCapabilities("DA");
    teardown();
} END_TEST

START_TEST(capabilitiesOnlyLDSRemoved) {
    startWithCapabilities(UA_APPLICATIONTYPE_SERVER, UA_SERVERCAPABILITY_LDS);
    assertServerCapabilities("NA");
    teardown();
} END_TEST

/* A DiscoveryServer has only the LDS capability */
START_TEST(capabilitiesDiscoveryServer) {
    startWithCapabilities(UA_APPLICATIONTYPE_DISCOVERYSERVER, 0);
    assertServerCapabilities("LDS");
    teardown();
} END_TEST

START_TEST(capabilitiesDiscoveryServerOthersRemoved) {
    startWithCapabilities(UA_APPLICATIONTYPE_DISCOVERYSERVER,
                          UA_SERVERCAPABILITY_DA | UA_SERVERCAPABILITY_GDS);
    assertServerCapabilities("LDS");
    teardown();
} END_TEST

/* A Client has only the NA or RCP capability */
START_TEST(capabilitiesClient) {
    startWithCapabilities(UA_APPLICATIONTYPE_CLIENT,
                          UA_SERVERCAPABILITY_DA | UA_SERVERCAPABILITY_RCP);
    assertServerCapabilities("RCP");
    teardown();
} END_TEST

START_TEST(capabilitiesUnknownBits) {
    startWithCapabilities(UA_APPLICATIONTYPE_SERVER,
                          (0x01ULL << 63u) | UA_SERVERCAPABILITY_DA);
    assertServerCapabilities("DA");
    teardown();
} END_TEST

/* The config is not modified. A restart derives the capabilities again. */
START_TEST(capabilitiesRestart) {
    startWithCapabilities(UA_APPLICATIONTYPE_DISCOVERYSERVER, UA_SERVERCAPABILITY_DA);
    assertServerCapabilities("LDS");
    UA_ServerConfig *config = UA_Server_getConfig(server);
    ck_assert_uint_eq(config->serverCapabilities, UA_SERVERCAPABILITY_DA);

    UA_Server_run_shutdown(server);
    config->applicationDescription.applicationType = UA_APPLICATIONTYPE_SERVER;
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    assertServerCapabilities("DA");
    teardown();
} END_TEST

static Suite *
testSuite_serverCapabilities(void) {
    TCase *tc = tcase_create("ServerCapabilities");
    tcase_add_test(tc, capabilitiesConfigured);
    tcase_add_test(tc, capabilitiesClientAndServer);
    tcase_add_test(tc, capabilitiesNACombined);
    tcase_add_test(tc, capabilitiesLDSRemoved);
    tcase_add_test(tc, capabilitiesOnlyLDSRemoved);
    tcase_add_test(tc, capabilitiesDiscoveryServer);
    tcase_add_test(tc, capabilitiesDiscoveryServerOthersRemoved);
    tcase_add_test(tc, capabilitiesClient);
    tcase_add_test(tc, capabilitiesUnknownBits);
    tcase_add_test(tc, capabilitiesRestart);

    Suite *s = suite_create("NS0 ServerCapabilities");
    suite_add_tcase(s, tc);
    return s;
}

/* Every bit maps to at most one identifier and back */
START_TEST(identifiersRoundtrip) {
    UA_String ids[UA_SERVERCAPABILITIES_MAX];
    for(size_t i = 0; i < 64; i++) {
        UA_UInt64 bit = 0x01ULL << i;
        size_t idsSize = getServerCapabilityIdentifiers(bit, ids);
        ck_assert_uint_le(idsSize, 1);
        if(idsSize == 1)
            ck_assert(getServerCapability(&ids[0]) == bit);
    }
} END_TEST

/* The identifiers match the normative CSV of the OPC Foundation */
START_TEST(identifiersMatchCsv) {
    FILE *f = fopen(UA_SERVERCAPABILITIES_CSV, "r");
    ck_assert_ptr_ne(f, NULL);
    UA_UInt64 csvCapabilities = 0;
    char line[512];
    while(fgets(line, sizeof(line), f)) {
        char *id = line;
        if(strncmp(id, "\xEF\xBB\xBF", 3) == 0)
            id += 3; /* Skip the UTF-8 BOM */
        size_t idLen = strcspn(id, ",\r\n");
        if(idLen == 0)
            continue;
        UA_String identifier = {idLen, (UA_Byte*)id};
        UA_UInt64 bit = getServerCapability(&identifier);
        ck_assert_msg(bit != 0, "Unknown ServerCapability %.*s",
                      (int)idLen, id);
        csvCapabilities |= bit;
    }
    fclose(f);

    /* No identifiers beyond the CSV */
    UA_String ids[UA_SERVERCAPABILITIES_MAX];
    ck_assert_uint_eq(getServerCapabilityIdentifiers(~csvCapabilities, ids), 0);
} END_TEST

static Suite *
testSuite_serverCapabilityIdentifiers(void) {
    TCase *tc = tcase_create("ServerCapabilityIdentifiers");
    tcase_add_test(tc, identifiersRoundtrip);
    tcase_add_test(tc, identifiersMatchCsv);

    Suite *s = suite_create("ServerCapability Identifiers");
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite_serverConfiguration();
    SRunner *sr = srunner_create(s);
    srunner_add_suite(sr, testSuite_serverCapabilities());
    srunner_add_suite(sr, testSuite_serverCapabilityIdentifiers());
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
