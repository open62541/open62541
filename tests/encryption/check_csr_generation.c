/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/create_certificate.h>

#include <check.h>
#include "test_helpers.h"
#include "certificates.h"

UA_Server *server;

static void setup(void) {
    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    server = UA_Server_newForUnitTestWithSecurityPolicies(4840, &certificate, &privateKey,
                                                          NULL, 0,
                                                          NULL, 0,
                                                          NULL, 0);
    ck_assert(server != NULL);
}

static void teardown(void) {
    UA_Server_delete(server);
}

START_TEST(csr_generation_rsaSha) {
    UA_ByteString *csr = UA_ByteString_new();
    UA_NodeId groupId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);
    UA_StatusCode retval =
            UA_Server_createSigningRequest(server, groupId, typeId, NULL, NULL, NULL, csr);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(csr->length, 0);

    UA_ByteString_delete(csr);
}
END_TEST

/* Basic128Rsa15 is deprecated */
/* START_TEST(csr_generation_rsaMin) { */
/*     UA_ByteString *csr = UA_ByteString_new(); */
/*     UA_NodeId groupId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP); */
/*     UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE); */
/*     UA_StatusCode retval = */
/*             UA_Server_createSigningRequest(server, groupId, typeId, NULL, NULL, NULL, csr); */
/*     ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD); */
/*     ck_assert_uint_ne(csr->length, 0); */

/*     UA_ByteString_delete(csr); */
/* } */
/* END_TEST */

START_TEST(csr_generation_new_priv_key) {
    UA_ByteString *csr = UA_ByteString_new();
    UA_NodeId groupId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);
    UA_Boolean regenerateKey = true;
    UA_StatusCode retval =
            UA_Server_createSigningRequest(server, groupId, typeId, NULL, &regenerateKey, NULL, csr);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(csr->length, 0);

    UA_ByteString_delete(csr);
}
END_TEST

START_TEST(csr_generation_add_nonce) {
    UA_ByteString *csr = UA_ByteString_new();
    UA_ByteString nonce = UA_BYTESTRING("08384461199560152606491732662271");
    UA_NodeId groupId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);
    UA_Boolean regenerateKey = false;
    UA_StatusCode retval =
            UA_Server_createSigningRequest(server, groupId, typeId, NULL, &regenerateKey, &nonce, csr);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(csr->length, 0);

    UA_ByteString_delete(csr);
}
END_TEST

START_TEST(csr_generation_add_subject_name) {
    UA_ByteString *csr = UA_ByteString_new();
    UA_String subjectName = UA_STRING("CN=open62541Server@localhost O=open62541 L=Here C=DE");
    UA_NodeId groupId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);
    UA_Boolean regenerateKey = false;
    UA_StatusCode retval =
            UA_Server_createSigningRequest(server, groupId, typeId, &subjectName, &regenerateKey, NULL, csr);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(csr->length, 0);

    UA_ByteString_delete(csr);
}
END_TEST

START_TEST(csr_generation_wrong_typeId) {
    UA_ByteString *csr = UA_ByteString_new();
    UA_String subjectName = UA_STRING("CN=open62541Server@localhost O=open62541 L=Here C=DE");
    UA_NodeId groupId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ECCCURVE448APPLICATIONCERTIFICATETYPE);
    UA_Boolean regenerateKey = false;
    UA_StatusCode retval =
            UA_Server_createSigningRequest(server, groupId, typeId, &subjectName, &regenerateKey, NULL, csr);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(csr->length, 0);

    UA_ByteString_delete(csr);
}
END_TEST

START_TEST(csr_generation_wrong_groupId) {
    UA_ByteString *csr = UA_ByteString_new();
    UA_String subjectName = UA_STRING("CN=open62541Server@localhost O=open62541 L=Here C=DE");
    UA_NodeId groupId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);
    UA_Boolean regenerateKey = false;
    UA_StatusCode retval =
            UA_Server_createSigningRequest(server, groupId, typeId, &subjectName, &regenerateKey, NULL, csr);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(csr->length, 0);

    UA_ByteString_delete(csr);
}
END_TEST

static Suite* testSuite_create_certificate(void) {
    Suite *s = suite_create("Create Csr");
    TCase *tc_cert = tcase_create("Csr Create");
    tcase_add_checked_fixture(tc_cert, setup, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_cert, csr_generation_rsaSha);
    tcase_add_test(tc_cert, csr_generation_new_priv_key);
    tcase_add_test(tc_cert, csr_generation_add_nonce);
    tcase_add_test(tc_cert, csr_generation_add_subject_name);
    tcase_add_test(tc_cert, csr_generation_wrong_typeId);
    tcase_add_test(tc_cert, csr_generation_wrong_groupId);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s,tc_cert);
    return s;
}

int main(void) {
    Suite *s = testSuite_create_certificate();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
