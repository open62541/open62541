/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_plugin_ca.h"
#include "ua_types_generated_handling.h"
#include "check.h"
#include "ua_ca_gnutls.h"
#include <gnutls/x509.h>

#ifdef __clang__
//required for ck_assert_ptr_eq and const casting
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#endif

START_TEST(Init_CA) {
        UA_GDS_CA ca;
        UA_String caDomainName = UA_STRING("O=open62541,CN=GDS@localhost");
        char caSerialNumber[2] = {0, 127}; //Serial number of the CA certificate is 127 (0x7f)
        UA_initCA(&ca, caDomainName, (60 * 60 * 24 * 365 * 10), 2, caSerialNumber, 2048, NULL);
        UA_TrustListDataType tl;
        ca.getTrustList(&ca, &tl);

        ck_assert_uint_eq(1, tl.trustedCertificatesSize);
        ck_assert_uint_eq(1, tl.trustedCrlsSize);
        ck_assert_uint_eq(0, tl.issuerCertificatesSize);
        ck_assert_uint_eq(0, tl.issuerCrlsSize);

        gnutls_x509_crt_t cert;
        gnutls_datum_t data = {NULL, 0};
        data.data = tl.trustedCertificates[0].data;
        data.size = (unsigned int) tl.trustedCertificates[0].length;
        gnutls_x509_crt_init(&cert);
        int gnuErr = gnutls_x509_crt_import(cert, &data, GNUTLS_X509_FMT_DER);
        ck_assert_int_eq(gnuErr, GNUTLS_E_SUCCESS);

        //Check domain name
        char dn[512];
        size_t dn_size = sizeof(dn);
        gnuErr = gnutls_x509_crt_get_dn(cert, dn, &dn_size);
        ck_assert_int_eq(gnuErr, GNUTLS_E_SUCCESS);
        ck_assert_str_eq(dn, (char*) caDomainName.data);

        //Check serial number
        char serialNumber[1];
        size_t serialNumberSize = sizeof(serialNumber);
        gnuErr = gnutls_x509_crt_get_serial(cert, serialNumber, &serialNumberSize);
        ck_assert_int_eq(gnuErr, GNUTLS_E_SUCCESS);
        ck_assert(serialNumber[0] == caSerialNumber[1]);

        //Check key length
        unsigned int bits = 0;
        gnuErr = gnutls_x509_crt_get_pk_algorithm(cert, &bits);
        ck_assert_int_eq(gnuErr, GNUTLS_PK_RSA);
        ck_assert_uint_eq(bits, 2048);

        gnutls_x509_crt_deinit(cert);
        UA_TrustListDataType_deleteMembers(&tl);
        ca.deleteMembers(&ca);
}
END_TEST

int main(void) {
    Suite *s  = suite_create("Test GnuTLS implementation");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, Init_CA);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
