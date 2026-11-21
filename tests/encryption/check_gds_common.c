/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Bastian Engel)
 */

#include <open62541/types.h>
#include <open62541/plugin/certificategroup.h>

#include "gds_common.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "certificates.h"

/* Helper: concatenate the certificates to a chain */
static UA_ByteString
concatCertificates(const UA_ByteString *certificates, size_t certificatesSize) {
    size_t length = 0;
    for(size_t i = 0; i < certificatesSize; i++)
        length += certificates[i].length;
    UA_ByteString chain;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&chain, length), UA_STATUSCODE_GOOD);
    size_t pos = 0;
    for(size_t i = 0; i < certificatesSize; i++) {
        memcpy(&chain.data[pos], certificates[i].data, certificates[i].length);
        pos += certificates[i].length;
    }
    return chain;
}

/* Helper: the leaf is the expected certificate and the crypto backend parses
 * it */
static void
assertLeaf(UA_ByteString leaf, const UA_ByteString *expected) {
    ck_assert(UA_ByteString_equal(&leaf, expected));
    size_t keySize = 0;
    ck_assert_uint_eq(UA_CertificateUtils_getKeySize(&leaf, &keySize),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(keySize, 0);
}

START_TEST(derSingle) {
    UA_ByteString cert = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString leaf = UA_GDSReceiver_getLeafCertificate(cert);
    ck_assert_ptr_eq(leaf.data, cert.data);
    assertLeaf(leaf, &cert);
} END_TEST

START_TEST(derChain) {
    UA_ByteString cert = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString certs[2] = {cert, cert};
    UA_ByteString chain = concatCertificates(certs, 2);
    UA_ByteString leaf = UA_GDSReceiver_getLeafCertificate(chain);
    ck_assert_ptr_eq(leaf.data, chain.data);
    assertLeaf(leaf, &cert);
    UA_ByteString_clear(&chain);
} END_TEST

/* The encoded length exceeds the ByteString */
START_TEST(derTruncated) {
    UA_ByteString cert = {CERT_DER_LENGTH - 1, CERT_DER_DATA};
    UA_ByteString leaf = UA_GDSReceiver_getLeafCertificate(cert);
    ck_assert_uint_eq(leaf.length, 0);
    ck_assert_ptr_null(leaf.data);
} END_TEST

START_TEST(pemSingle) {
    UA_ByteString cert = {APPLICATION_CERT_DER_LENGTH, APPLICATION_CERT_DER_DATA};
    UA_ByteString leaf = UA_GDSReceiver_getLeafCertificate(cert);
    ck_assert_ptr_eq(leaf.data, cert.data);
    assertLeaf(leaf, &cert);
} END_TEST

/* The issuers of the leaf are appended */
START_TEST(pemChain) {
    UA_ByteString certs[3] = {
        {APPLICATION_CERT_DER_LENGTH, APPLICATION_CERT_DER_DATA},
        {INTERMEDIATE_CERT_DER_LENGTH, INTERMEDIATE_CERT_DER_DATA},
        {ROOT_CERT_DER_LENGTH, ROOT_CERT_DER_DATA}};
    UA_ByteString chain = concatCertificates(certs, 3);
    UA_ByteString leaf = UA_GDSReceiver_getLeafCertificate(chain);
    ck_assert_ptr_eq(leaf.data, chain.data);
    assertLeaf(leaf, &certs[0]);
    UA_ByteString_clear(&chain);
} END_TEST

/* Text before the PEM header is cut off */
START_TEST(pemLeadingText) {
    UA_ByteString certs[2] = {
        UA_BYTESTRING("Bag Attributes\n"),
        {APPLICATION_CERT_DER_LENGTH, APPLICATION_CERT_DER_DATA}};
    UA_ByteString chain = concatCertificates(certs, 2);
    UA_ByteString leaf = UA_GDSReceiver_getLeafCertificate(chain);
    ck_assert_ptr_eq(leaf.data, &chain.data[certs[0].length]);
    assertLeaf(leaf, &certs[1]);
    UA_ByteString_clear(&chain);
} END_TEST

/* The PEM footer is missing */
START_TEST(pemTruncated) {
    UA_ByteString cert = {APPLICATION_CERT_DER_LENGTH - 10, APPLICATION_CERT_DER_DATA};
    UA_ByteString leaf = UA_GDSReceiver_getLeafCertificate(cert);
    ck_assert_uint_eq(leaf.length, 0);
    ck_assert_ptr_null(leaf.data);
} END_TEST

/* Neither DER nor PEM. The crypto backend reports the parse error. */
START_TEST(unknownFormat) {
    UA_ByteString cert = UA_BYTESTRING("not a certificate");
    UA_ByteString leaf = UA_GDSReceiver_getLeafCertificate(cert);
    ck_assert_ptr_eq(leaf.data, cert.data);
    ck_assert_uint_eq(leaf.length, cert.length);
} END_TEST

int main(void) {
    Suite *s = suite_create("GDS Common");
    TCase *tc = tcase_create("GetLeafCertificate");
    tcase_add_test(tc, derSingle);
    tcase_add_test(tc, derChain);
    tcase_add_test(tc, derTruncated);
    tcase_add_test(tc, pemSingle);
    tcase_add_test(tc, pemChain);
    tcase_add_test(tc, pemLeadingText);
    tcase_add_test(tc, pemTruncated);
    tcase_add_test(tc, unknownFormat);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
