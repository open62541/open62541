/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/* A PKI folder that cannot be read must make the certificate verification
 * fail with an internal error. Before, the error was ignored and every
 * certificate was reported as untrusted. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pki_default.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "certificates.h"
#include "check.h"

#define MISSING_FOLDER "./check_pki_folder_errors_missing"
#define TRUSTED_FILE "./check_pki_folder_errors_trusted.der"

static UA_StatusCode
setupFolders(UA_CertificateVerification *cv, const char *trustListFolder,
             const char *issuerListFolder, const char *revocationListFolder) {
    memset(cv, 0, sizeof(UA_CertificateVerification));
    cv->logging = UA_Log_Stdout;
#ifdef UA_ENABLE_CERT_REJECTED_DIR
    return UA_CertificateVerification_CertFolders(cv, trustListFolder, issuerListFolder,
                                                  revocationListFolder, NULL);
#else
    return UA_CertificateVerification_CertFolders(cv, trustListFolder, issuerListFolder,
                                                  revocationListFolder);
#endif
}

static UA_StatusCode
verifyTestCertificate(UA_CertificateVerification *cv) {
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;
    return cv->verifyCertificate(cv, &certificate);
}

START_TEST(missingTrustListFolder) {
    UA_CertificateVerification cv;
    UA_StatusCode res = setupFolders(&cv, MISSING_FOLDER, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(verifyTestCertificate(&cv), UA_STATUSCODE_BADINTERNALERROR);
    cv.clear(&cv);
} END_TEST

START_TEST(missingRevocationListFolder) {
    UA_CertificateVerification cv;
    UA_StatusCode res = setupFolders(&cv, NULL, NULL, MISSING_FOLDER);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(verifyTestCertificate(&cv), UA_STATUSCODE_BADINTERNALERROR);
    cv.clear(&cv);
} END_TEST

/* The trust-list folder is read and contains a certificate. Then the
 * issuer-list folder cannot be read. This must not free the directory
 * listing of the trust-list folder a second time. */
START_TEST(missingIssuerListFolderAfterTrustList) {
    FILE *fp = fopen(TRUSTED_FILE, "wb");
    ck_assert_ptr_ne(fp, NULL);
    ck_assert_uint_eq(fwrite(CERT_DER_DATA, 1, CERT_DER_LENGTH, fp), CERT_DER_LENGTH);
    ck_assert_int_eq(fclose(fp), 0);

    UA_CertificateVerification cv;
    UA_StatusCode res = setupFolders(&cv, ".", MISSING_FOLDER, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(verifyTestCertificate(&cv), UA_STATUSCODE_BADINTERNALERROR);
    cv.clear(&cv);

    remove(TRUSTED_FILE);
} END_TEST

/* Readable folders do not produce the internal error */
START_TEST(readableFolders) {
    UA_CertificateVerification cv;
    UA_StatusCode res = setupFolders(&cv, ".", ".", ".");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(verifyTestCertificate(&cv), UA_STATUSCODE_BADINTERNALERROR);
    cv.clear(&cv);
} END_TEST

static Suite *testSuite_pki_folder_errors(void) {
    Suite *s = suite_create("PKI folder errors");
    TCase *tc = tcase_create("PKI folder errors");
    tcase_add_test(tc, missingTrustListFolder);
    tcase_add_test(tc, missingRevocationListFolder);
    tcase_add_test(tc, missingIssuerListFolderAfterTrustList);
    tcase_add_test(tc, readableFolders);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite_pki_folder_errors();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
