/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 */

#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/securitypolicy_default.h>

#include <check.h>
#include "test_helpers.h"

UA_Server *server;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
}

static void teardown(void) {
    UA_Server_delete(server);
}

START_TEST(certificate_generation) {
    UA_ByteString derPrivKey = UA_BYTESTRING_NULL;
    UA_ByteString derCert = UA_BYTESTRING_NULL;
    UA_String subject[3] = {UA_STRING_STATIC("C=DE"),
                            UA_STRING_STATIC("O=SampleOrganization"),
                            UA_STRING_STATIC("CN=Open62541Server@localhost")};
    UA_UInt32 lenSubject = 3;
    UA_String subjectAltName[] = {UA_STRING_STATIC("DNS:localhost")};
    UA_UInt32 lenSubjectAltName = 1;
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();
    UA_UInt16 expiresIn = 14;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "expires-in-days"),
                             (void *)&expiresIn, &UA_TYPES[UA_TYPES_UINT16]);
    UA_UInt16 keyLength = 2048;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "key-size-bits"),
                             (void *)&keyLength, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode status = UA_CreateCertificate(
        UA_Log_Stdout, subject, lenSubject, subjectAltName, lenSubjectAltName,
        UA_CERTIFICATEFORMAT_DER, kvm, &derPrivKey, &derCert);
    UA_KeyValueMap_delete(kvm);
    ck_assert(status == UA_STATUSCODE_GOOD);
    ck_assert(derPrivKey.length > 0);
    ck_assert(derCert.length > 0);

    UA_String emptyUri = UA_STRING_NULL;
    status = UA_CertificateUtils_verifyApplicationUri(&derCert, &emptyUri);
    ck_assert(status == UA_STATUSCODE_BADCERTIFICATEURIINVALID);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    status = UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840, &derCert, &derPrivKey,
                                                            NULL, 0, NULL, 0, NULL, 0);
    config->tcpReuseAddr = true;
    ck_assert(status == UA_STATUSCODE_GOOD);

    UA_ByteString_clear(&derCert);
    UA_ByteString_clear(&derPrivKey);
}
END_TEST

static void
createRsaCertificate(UA_UInt16 keyLength, UA_ByteString *derPrivKey,
                     UA_ByteString *derCert) {
    UA_String subject[1] = {UA_STRING_STATIC("CN=Open62541Test@localhost")};
    UA_String subjectAltName[1] = {UA_STRING_STATIC("DNS:localhost")};
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "key-size-bits"),
                             (void *)&keyLength, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode status =
        UA_CreateCertificate(UA_Log_Stdout, subject, 1, subjectAltName, 1,
                             UA_CERTIFICATEFORMAT_DER, kvm, derPrivKey, derCert);
    UA_KeyValueMap_delete(kvm);
    ck_assert_uint_eq(status, UA_STATUSCODE_GOOD);
}

/* Basic256Sha256 requires an asymmetric key of 2048 to 4096 bits. A remote
 * certificate outside of these bounds is refused. */
START_TEST(remote_key_length_basic256sha256) {
    UA_ByteString localKey, localCert;
    createRsaCertificate(2048, &localKey, &localCert);
    UA_SecurityPolicy sp;
    memset(&sp, 0, sizeof(UA_SecurityPolicy));
    UA_StatusCode res =
        UA_SecurityPolicy_Basic256Sha256(&sp, localCert, localKey, UA_Log_Stdout);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_ByteString remoteKey, remoteCert;
    void *channelContext = NULL;
    createRsaCertificate(1024, &remoteKey, &remoteCert);
    res = sp.newChannelContext(&sp, &remoteCert, &channelContext);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED);
    UA_ByteString_clear(&remoteKey);
    UA_ByteString_clear(&remoteCert);

    createRsaCertificate(2048, &remoteKey, &remoteCert);
    res = sp.newChannelContext(&sp, &remoteCert, &channelContext);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    sp.deleteChannelContext(&sp, channelContext);
    UA_ByteString_clear(&remoteKey);
    UA_ByteString_clear(&remoteCert);

    sp.clear(&sp);
    UA_ByteString_clear(&localKey);
    UA_ByteString_clear(&localCert);
}
END_TEST

static Suite* testSuite_create_certificate(void) {
    Suite *s = suite_create("Create Certificate");
    TCase *tc_cert = tcase_create("Certificate Create");
    tcase_add_checked_fixture(tc_cert, setup, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_cert, certificate_generation);
    tcase_add_test(tc_cert, remote_key_length_basic256sha256);
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
