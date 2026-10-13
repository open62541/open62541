/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "securitypolicy_common.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

typedef UA_StatusCode (*AeadOperation)(const UA_ByteString *, const UA_ByteString *,
                                      const UA_ByteString *, UA_ByteString *, UA_Boolean);

static AeadOperation encryptOperations[] = {
    UA_OpenSSL_AES_128_GCM_Encrypt, UA_OpenSSL_ChaCha20Poly1305_Encrypt
};
static AeadOperation decryptOperations[] = {
    UA_OpenSSL_AES_128_GCM_Decrypt, UA_OpenSSL_ChaCha20Poly1305_Decrypt
};

/* NIST GCM-AES128 examples 2 and 3:
 * https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Standards-and-Guidelines/documents/examples/AES_GCM.pdf */
static UA_Byte gcmKey[] = {
    0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
    0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08
};
static UA_Byte gcmIv[] = {
    0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88
};
static UA_Byte gcmAad[] = {
    0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
    0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97,
    0xf5, 0xd3, 0xd5, 0x85, 0x03, 0xb9, 0x69, 0x9d,
    0xe7, 0x85, 0x89, 0x5a, 0x96, 0xfd, 0xba, 0xaf,
    0x43, 0xb1, 0xcd, 0x7f, 0x59, 0x8e, 0xce, 0x23,
    0x88, 0x1b, 0x00, 0xe3, 0xed, 0x03, 0x06, 0x88,
    0x7b, 0x0c, 0x78, 0x5e, 0x27, 0xe8, 0xad, 0x3f,
    0x82, 0x23, 0x20, 0x71, 0x04, 0x72, 0x5d, 0xd4
};

START_TEST(aesGcm_signOnly_knownAnswer) {
    const size_t splits[] = {0, 20, 64};
    size_t split = splits[_i];
    UA_ByteString key = {sizeof(gcmKey), gcmKey};
    UA_ByteString iv = {sizeof(gcmIv), gcmIv};
    UA_ByteString aad = {split, gcmAad};
    UA_Byte buffer[80];
    size_t plainLength = sizeof(gcmAad) - split;
    memcpy(buffer, gcmAad + split, plainLength);
    UA_ByteString data = {plainLength + 16, buffer};
    const UA_Byte tag[] = {
        0x5f, 0x91, 0xd7, 0x71, 0x23, 0xef, 0x5e, 0xb9,
        0x99, 0x79, 0x13, 0x84, 0x9b, 0x8d, 0xc1, 0xe9
    };
    ck_assert_uint_eq(UA_OpenSSL_AES_128_GCM_Encrypt(&iv, &key, &aad, &data, false),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, plainLength + sizeof(tag));
    ck_assert_mem_eq(data.data, gcmAad + split, plainLength);
    ck_assert_mem_eq(data.data + plainLength, tag, sizeof(tag));
    ck_assert_uint_eq(UA_OpenSSL_AES_128_GCM_Decrypt(&iv, &key, &aad, &data, false),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, plainLength);
    ck_assert_mem_eq(data.data, gcmAad + split, plainLength);
}
END_TEST

START_TEST(aesGcm_encrypt_knownAnswer) {
    UA_ByteString key = {sizeof(gcmKey), gcmKey};
    UA_ByteString iv = {sizeof(gcmIv), gcmIv};
    UA_ByteString plain = UA_BYTESTRING(
        "\xd9\x31\x32\x25\xf8\x84\x06\xe5\xa5\x59\x09\xc5\xaf\xf5\x26\x9a"
        "\x86\xa7\xa9\x53\x15\x34\xf7\xda\x2e\x4c\x30\x3d\x8a\x31\x8a\x72"
        "\x1c\x3c\x0c\x95\x95\x68\x09\x53\x2f\xcf\x0e\x24\x49\xa6\xb5\x25"
        "\xb1\x6a\xed\xf5\xaa\x0d\xe6\x57\xba\x63\x7b\x39\x1a\xaf\xd2\x55");
    UA_ByteString expected = UA_BYTESTRING(
        "\x42\x83\x1e\xc2\x21\x77\x74\x24\x4b\x72\x21\xb7\x84\xd0\xd4\x9c"
        "\xe3\xaa\x21\x2f\x2c\x02\xa4\xe0\x35\xc1\x7e\x23\x29\xac\xa1\x2e"
        "\x21\xd5\x14\xb2\x54\x66\x93\x1c\x7d\x8f\x6a\x5a\xac\x84\xaa\x05"
        "\x1b\xa3\x0b\x39\x6a\x0a\xac\x97\x3d\x58\xe0\x91\x47\x3f\x59\x85"
        "\x4d\x5c\x2a\xf3\x27\xcd\x64\xa6\x2c\xf3\x5a\xbd\x2b\xa6\xfa\xb4");
    UA_Byte buffer[80];
    memcpy(buffer, plain.data, plain.length);
    UA_ByteString data = {sizeof(buffer), buffer};
    ck_assert_uint_eq(UA_OpenSSL_AES_128_GCM_Encrypt(&iv, &key, NULL, &data, true),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_ByteString_equal(&data, &expected));
    ck_assert_uint_eq(UA_OpenSSL_AES_128_GCM_Decrypt(&iv, &key, NULL, &data, true),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_ByteString_equal(&data, &plain));
}
END_TEST

START_TEST(chachaPoly_encrypt_knownAnswer) {
    /* RFC 8439 section 2.8.2: https://www.rfc-editor.org/rfc/rfc8439#section-2.8.2 */
    UA_Byte keyBytes[32];
    for(size_t i = 0; i < sizeof(keyBytes); i++)
        keyBytes[i] = (UA_Byte)(0x80 + i);
    UA_ByteString key = {sizeof(keyBytes), keyBytes};
    UA_ByteString iv = UA_BYTESTRING("\x07\x00\x00\x00\x40\x41\x42\x43\x44\x45\x46\x47");
    /* ByteString literals containing NUL bytes need an explicit length. */
    iv.length = 12;
    UA_ByteString aad = UA_BYTESTRING("\x50\x51\x52\x53\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7");
    UA_ByteString plain = UA_BYTESTRING(
        "Ladies and Gentlemen of the class of '99: If I could offer you only one tip "
        "for the future, sunscreen would be it.");
    UA_ByteString expected = UA_BYTESTRING(
        "\xd3\x1a\x8d\x34\x64\x8e\x60\xdb\x7b\x86\xaf\xbc\x53\xef\x7e\xc2"
        "\xa4\xad\xed\x51\x29\x6e\x08\xfe\xa9\xe2\xb5\xa7\x36\xee\x62\xd6"
        "\x3d\xbe\xa4\x5e\x8c\xa9\x67\x12\x82\xfa\xfb\x69\xda\x92\x72\x8b"
        "\x1a\x71\xde\x0a\x9e\x06\x0b\x29\x05\xd6\xa5\xb6\x7e\xcd\x3b\x36"
        "\x92\xdd\xbd\x7f\x2d\x77\x8b\x8c\x98\x03\xae\xe3\x28\x09\x1b\x58"
        "\xfa\xb3\x24\xe4\xfa\xd6\x75\x94\x55\x85\x80\x8b\x48\x31\xd7\xbc"
        "\x3f\xf4\xde\xf0\x8e\x4b\x7a\x9d\xe5\x76\xd2\x65\x86\xce\xc6\x4b\x61\x16"
        "\x1a\xe1\x0b\x59\x4f\x09\xe2\x6a\x7e\x90\x2e\xcb\xd0\x60\x06\x91");
    UA_Byte buffer[130];
    memcpy(buffer, plain.data, plain.length);
    UA_ByteString data = {sizeof(buffer), buffer};
    ck_assert_uint_eq(UA_OpenSSL_ChaCha20Poly1305_Encrypt(&iv, &key, &aad, &data, true),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_ByteString_equal(&data, &expected));
    ck_assert_uint_eq(UA_OpenSSL_ChaCha20Poly1305_Decrypt(&iv, &key, &aad, &data, true),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_ByteString_equal(&data, &plain));
}
END_TEST

START_TEST(aead_emptyInputsAndModes) {
    size_t algorithm = (size_t)_i / 12;
    UA_Boolean encryptData = (_i / 6) % 2 != 0;
    size_t shape = (size_t)_i % 6;
    UA_Byte keyBytes[32] = {1}, ivBytes[12] = {2};
    UA_ByteString key = {algorithm == 0 ? 16 : 32, keyBytes};
    UA_ByteString iv = {sizeof(ivBytes), ivBytes};
    UA_ByteString aad = shape % 3 == 2 ? UA_BYTESTRING("header") : UA_BYTESTRING_NULL;
    const UA_ByteString *aadPtr = shape % 3 == 0 ? NULL : &aad;
    size_t plainLength = shape < 3 ? 0 : 31;
    UA_Byte guarded[49];
    memset(guarded, 0xa5, sizeof(guarded));
    UA_ByteString data = {plainLength + 16, guarded + 1};
    memset(data.data, 0x42, plainLength);
    UA_Byte plain[31];
    memset(plain, 0x42, sizeof(plain));
    ck_assert_uint_eq(encryptOperations[algorithm](&iv, &key, aadPtr, &data, encryptData),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, plainLength + 16);
    if(!encryptData)
        ck_assert_mem_eq(data.data, plain, plainLength);
    else if(plainLength > 0)
        ck_assert(memcmp(data.data, plain, plainLength) != 0);
    ck_assert_uint_eq(decryptOperations[algorithm](&iv, &key, aadPtr, &data, encryptData),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, plainLength);
    ck_assert_mem_eq(data.data, plain, plainLength);
    ck_assert_uint_eq(guarded[0], 0xa5);
    ck_assert_uint_eq(guarded[plainLength + 17], 0xa5);
}
END_TEST

START_TEST(aead_rejectsTampering) {
    size_t algorithm = (size_t)_i / 12;
    UA_Boolean encryptData = (_i / 6) % 2 != 0;
    size_t corruption = (size_t)_i % 6;
    UA_Byte keyBytes[32] = {1}, ivBytes[12] = {2}, aadBytes[13] = {3};
    UA_ByteString key = {algorithm == 0 ? 16 : 32, keyBytes};
    UA_ByteString iv = {sizeof(ivBytes), ivBytes};
    UA_ByteString aad = {sizeof(aadBytes), aadBytes};
    UA_Byte buffer[47];
    memset(buffer, 0x42, sizeof(buffer));
    UA_ByteString data = {sizeof(buffer), buffer};
    ck_assert_uint_eq(encryptOperations[algorithm](&iv, &key, &aad, &data, encryptData),
                      UA_STATUSCODE_GOOD);
    switch(corruption) {
    case 0: buffer[0] ^= 1; break;
    case 1: buffer[46] ^= 1; break;
    case 2: aadBytes[0] ^= 1; break;
    case 3: keyBytes[0] ^= 1; break;
    case 4: ivBytes[0] ^= 1; break;
    default: data.length--; break;
    }
    UA_Byte before[47];
    memcpy(before, buffer, sizeof(buffer));
    size_t length = data.length;
    ck_assert_uint_eq(decryptOperations[algorithm](&iv, &key, &aad, &data, encryptData),
                      UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    ck_assert_uint_eq(data.length, length);
    if(!encryptData)
        ck_assert_mem_eq(buffer, before, sizeof(buffer));
}
END_TEST

START_TEST(aead_rejectsInvalidSizes) {
    size_t algorithm = (size_t)_i / 6;
    UA_Boolean decrypt = (_i / 3) % 2 != 0;
    size_t invalidField = (size_t)_i % 3;
    UA_Byte keyBytes[32] = {1}, ivBytes[12] = {2}, buffer[32];
    memset(buffer, 0xa5, sizeof(buffer));
    UA_ByteString key = {algorithm == 0 ? 16 : 32, keyBytes};
    UA_ByteString iv = {sizeof(ivBytes), ivBytes};
    UA_ByteString data = {sizeof(buffer), buffer};
    if(invalidField == 0)
        key.length--;
    else if(invalidField == 1)
        iv.length--;
    else
        data.length = 15;
    size_t length = data.length;
    AeadOperation operation = decrypt ? decryptOperations[algorithm] : encryptOperations[algorithm];
    ck_assert_uint_eq(operation(&iv, &key, NULL, &data, false), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(data.length, length);
    ck_assert_ptr_eq(data.data, buffer);
    for(size_t i = 0; i < sizeof(buffer); i++)
        ck_assert_uint_eq(buffer[i], 0xa5);
}
END_TEST

int main(void) {
    Suite *s = suite_create("OpenSSL AEAD");
    TCase *tc = tcase_create("authenticated encryption");
    tcase_add_loop_test(tc, aesGcm_signOnly_knownAnswer, 0, 3);
    tcase_add_test(tc, aesGcm_encrypt_knownAnswer);
    tcase_add_test(tc, chachaPoly_encrypt_knownAnswer);
    tcase_add_loop_test(tc, aead_emptyInputsAndModes, 0, 24);
    tcase_add_loop_test(tc, aead_rejectsTampering, 0, 24);
    tcase_add_loop_test(tc, aead_rejectsInvalidSizes, 0, 12);
    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
