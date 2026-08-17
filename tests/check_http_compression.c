/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "eventloop_posix_http_compression.h"

#include <check.h>
#include <stdlib.h>

START_TEST(contentEncodingNegotiation) {
    ck_assert_uint_eq(
        UA_HTTP_parseContentEncoding(NULL),
        UA_HTTP_CONTENT_ENCODING_IDENTITY);
    UA_String br = UA_STRING("br");
    ck_assert_uint_eq(
        UA_HTTP_parseContentEncoding(&br),
        UA_HTTP_CONTENT_ENCODING_UNSUPPORTED);

    UA_String none = UA_STRING("gzip;q=0, deflate;q=0, identity;q=0");
    UA_HTTPCompressionPreference preference =
        UA_HTTP_selectResponseEncoding(&none);
    ck_assert(!preference.acceptable);
    ck_assert(!preference.identityAllowed);

    UA_String identity = UA_STRING("br, identity;q=0.5");
    preference = UA_HTTP_selectResponseEncoding(&identity);
    ck_assert(preference.acceptable);
    ck_assert_uint_eq(preference.encoding,
                      UA_HTTP_CONTENT_ENCODING_IDENTITY);

#ifdef UA_ENABLE_HTTP_COMPRESSION
    UA_String gzip = UA_STRING("deflate;q=0.2, gzip;q=0.9, identity;q=0.1");
    preference = UA_HTTP_selectResponseEncoding(&gzip);
    ck_assert(preference.acceptable);
    ck_assert_uint_eq(preference.encoding, UA_HTTP_CONTENT_ENCODING_GZIP);
#endif
} END_TEST

#ifdef UA_ENABLE_HTTP_COMPRESSION
START_TEST(compressionRoundtripAndLimit) {
    UA_ByteString input = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&input, 16384),
                      UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < input.length; i++)
        input.data[i] = (UA_Byte)(i % 7);

    const UA_HTTPContentEncoding encodings[] = {
        UA_HTTP_CONTENT_ENCODING_GZIP,
        UA_HTTP_CONTENT_ENCODING_DEFLATE
    };
    for(size_t i = 0; i < 2; i++) {
        UA_ByteString compressed = UA_BYTESTRING_NULL;
        ck_assert_uint_eq(UA_HTTP_compress(encodings[i], &input, &compressed),
                          UA_STATUSCODE_GOOD);
        ck_assert_uint_lt(compressed.length, input.length);

        UA_ByteString output = UA_BYTESTRING_NULL;
        ck_assert_uint_eq(UA_HTTP_decompress(encodings[i], &compressed,
                                             input.length, &output),
                          UA_STATUSCODE_GOOD);
        ck_assert(UA_ByteString_equal(&input, &output));
        UA_ByteString_clear(&output);

        ck_assert_uint_eq(UA_HTTP_decompress(encodings[i], &compressed,
                                             input.length - 1, &output),
                          UA_STATUSCODE_BADREQUESTTOOLARGE);
        ck_assert_ptr_null(output.data);
        UA_ByteString_clear(&compressed);
    }

    UA_ByteString malformed = UA_BYTESTRING("not-a-compressed-stream");
    UA_ByteString output = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_HTTP_decompress(UA_HTTP_CONTENT_ENCODING_GZIP,
                                         &malformed, 1024, &output),
                      UA_STATUSCODE_BADDECODINGERROR);
    ck_assert_ptr_null(output.data);
    UA_ByteString_clear(&input);
} END_TEST
#endif

int main(void) {
    Suite *suite = suite_create("HTTP content encoding");
    TCase *tc = tcase_create("provider");
    tcase_add_test(tc, contentEncodingNegotiation);
#ifdef UA_ENABLE_HTTP_COMPRESSION
    tcase_add_test(tc, compressionRoundtripAndLimit);
#endif
    suite_add_tcase(suite, tc);
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
