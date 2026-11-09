/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define UA_CreateCertificate test_UA_CreateCertificate
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/log_stdout.h>
#include <mbedtls/platform_time.h>
#include <check.h>
#include <stdlib.h>

/* Reproduce the Linux second-boundary race without changing the process clock:
 * gettimeofday() has advanced, but the verifier's time() has not yet. */
static mbedtls_time_t
certificateTime(mbedtls_time_t *out) {
    mbedtls_time_t now = 1700000000;
    if(out)
        *out = now;
    return now;
}

static UA_DateTime
highResolutionTime(void) {
    return UA_DateTime_fromUnixTime((UA_Int64)certificateTime(NULL) + 1);
}

/* Compile the generator with controlled clocks, leaving the library's clocks
 * and the other certificate tests unchanged. */
#undef mbedtls_time
#define mbedtls_time certificateTime
#define UA_DateTime_now highResolutionTime
#define mbedtls_x509write_crt_set_subject_alt_name test_set_subject_alt_name
#include "../../plugins/crypto/mbedtls/create_certificate.c"
#undef mbedtls_x509write_crt_set_subject_alt_name
#undef UA_CreateCertificate
#undef UA_DateTime_now
#undef mbedtls_time

START_TEST(certificate_uses_verifier_clock) {
    UA_String subject = UA_STRING_STATIC("CN=localhost");
    UA_String altName = UA_STRING_STATIC("URI:urn:test.clock");
    UA_String keyType = UA_STRING_STATIC("EC");
    UA_UInt16 expires = 7;
    UA_KeyValueMap params = UA_KEYVALUEMAP_NULL;
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(
        &params, UA_QUALIFIEDNAME(0, "key-type"),
        &keyType, &UA_TYPES[UA_TYPES_STRING]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(
        &params, UA_QUALIFIEDNAME(0, "expires-in-days"),
        &expires, &UA_TYPES[UA_TYPES_UINT16]), UA_STATUSCODE_GOOD);
    UA_ByteString key = UA_BYTESTRING_NULL, cert = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(test_UA_CreateCertificate(
        UA_Log_Stdout, &subject, 1, &altName, 1, UA_CERTIFICATEFORMAT_DER,
        &params, &key, &cert), UA_STATUSCODE_GOOD);
    UA_KeyValueMap_clear(&params);

    mbedtls_x509_crt parsed;
    mbedtls_x509_crt_init(&parsed);
    ck_assert_int_eq(mbedtls_x509_crt_parse_der(&parsed, cert.data, cert.length), 0);
    UA_DateTime now = UA_DateTime_fromUnixTime((UA_Int64)certificateTime(NULL));
    UA_DateTimeStruct expected = UA_DateTime_toStruct(now);
    ck_assert_int_eq(parsed.valid_from.year, expected.year);
    ck_assert_int_eq(parsed.valid_from.mon, expected.month);
    ck_assert_int_eq(parsed.valid_from.day, expected.day);
    ck_assert_int_eq(parsed.valid_from.hour, expected.hour);
    ck_assert_int_eq(parsed.valid_from.min, expected.min);
    ck_assert_int_eq(parsed.valid_from.sec, expected.sec);
    expected = UA_DateTime_toStruct(now + (UA_DateTime)expires * 86400 * UA_DATETIME_SEC);
    ck_assert_int_eq(parsed.valid_to.year, expected.year);
    ck_assert_int_eq(parsed.valid_to.mon, expected.month);
    ck_assert_int_eq(parsed.valid_to.day, expected.day);
    ck_assert_int_eq(parsed.valid_to.hour, expected.hour);
    ck_assert_int_eq(parsed.valid_to.min, expected.min);
    ck_assert_int_eq(parsed.valid_to.sec, expected.sec);
    mbedtls_x509_crt_free(&parsed);
    UA_ByteString_clear(&key);
    UA_ByteString_clear(&cert);
}
END_TEST

int main(void) {
    Suite *s = suite_create("Certificate clock");
    TCase *tc = tcase_create("Validity");
    tcase_add_test(tc, certificate_uses_verifier_clock);
    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
