/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/certificategroup.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/types.h>

#include <check.h>
#include <stdlib.h>

START_TEST(AcceptAll_InitFromZeroAcceptsAnyCertificate) {
    UA_CertificateGroup g;
    memset(&g, 0, sizeof(g));
    g.logging = UA_Log_Stdout;
    UA_CertificateGroup_AcceptAll(&g);

    /* All trust-list operations must be cleared / left NULL */
    ck_assert(g.getTrustList == NULL);
    ck_assert(g.setTrustList == NULL);
    ck_assert(g.addToTrustList == NULL);
    ck_assert(g.removeFromTrustList == NULL);
    ck_assert(g.getRejectedList == NULL);
    ck_assert(g.getCertificateCrls == NULL);

    /* verifyCertificate must accept any (including empty/garbage) cert */
    ck_assert(g.verifyCertificate != NULL);
    UA_ByteString empty = UA_BYTESTRING_NULL;
    UA_StatusCode rv = g.verifyCertificate(&g, &empty);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte rawData[] = { 0xde, 0xad, 0xbe, 0xef };
    UA_ByteString junk = { sizeof(rawData), rawData };
    rv = g.verifyCertificate(&g, &junk);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    /* clear must be safe to call */
    ck_assert(g.clear != NULL);
    g.clear(&g);
}
END_TEST

START_TEST(AcceptAll_PreservesCertificateGroupId) {
    UA_CertificateGroup g;
    memset(&g, 0, sizeof(g));
    g.logging = UA_Log_Stdout;
    g.certificateGroupId = UA_NODEID_NUMERIC(0, 4711);
    UA_CertificateGroup_AcceptAll(&g);
    ck_assert_int_eq(g.certificateGroupId.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_uint_eq(g.certificateGroupId.identifier.numeric, 4711);
    if(g.clear) g.clear(&g);
    UA_NodeId_clear(&g.certificateGroupId);
}
END_TEST

START_TEST(AcceptAll_ReinitializeOverridesPriorClear) {
    /* Calling AcceptAll twice on the same struct must invoke the previously
     * installed clear() and rewire to the accept-all behaviour. */
    UA_CertificateGroup g;
    memset(&g, 0, sizeof(g));
    g.logging = UA_Log_Stdout;
    UA_CertificateGroup_AcceptAll(&g);
    UA_CertificateGroup_AcceptAll(&g);
    UA_ByteString empty = UA_BYTESTRING_NULL;
    UA_StatusCode rv = g.verifyCertificate(&g, &empty);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    g.clear(&g);
}
END_TEST

#ifndef UA_ENABLE_ENCRYPTION
/* Without encryption, the certificate-utils API still has to provide stable
 * (typically BADNOTSUPPORTED) return values for callers. */
START_TEST(CertificateUtils_StubsAreSafeWithoutEncryption) {
    UA_ByteString cert = UA_BYTESTRING_NULL;
    UA_String uri = UA_STRING_NULL;
    UA_StatusCode rv = UA_CertificateUtils_verifyApplicationUri(&cert, &uri);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_DateTime exp = 0;
    rv = UA_CertificateUtils_getExpirationDate(&cert, &exp);
    ck_assert_int_eq(rv, UA_STATUSCODE_BADNOTSUPPORTED);

    UA_String subj = UA_STRING_NULL;
    rv = UA_CertificateUtils_getSubjectName(&cert, &subj);
    ck_assert_int_eq(rv, UA_STATUSCODE_BADNOTSUPPORTED);

    UA_String thumb = UA_STRING_NULL;
    rv = UA_CertificateUtils_getThumbprint(&cert, &thumb);
    ck_assert_int_eq(rv, UA_STATUSCODE_BADNOTSUPPORTED);

    rv = UA_CertificateUtils_comparePublicKeys(&cert, &cert);
    ck_assert_int_eq(rv, UA_STATUSCODE_BADNOTSUPPORTED);

    UA_ByteString key = UA_BYTESTRING_NULL;
    rv = UA_CertificateUtils_checkKeyPair(&cert, &key);
    ck_assert_int_eq(rv, UA_STATUSCODE_BADNOTSUPPORTED);

    rv = UA_CertificateUtils_checkCA(&cert);
    ck_assert_int_eq(rv, UA_STATUSCODE_BADNOTSUPPORTED);
}
END_TEST
#endif

int main(void) {
    Suite *s = suite_create("CertificateGroup AcceptAll (none backend)");
    TCase *tc = tcase_create("none");
    tcase_add_test(tc, AcceptAll_InitFromZeroAcceptsAnyCertificate);
    tcase_add_test(tc, AcceptAll_PreservesCertificateGroupId);
    tcase_add_test(tc, AcceptAll_ReinitializeOverridesPriorClear);
#ifndef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc, CertificateUtils_StubsAreSafeWithoutEncryption);
#endif
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
