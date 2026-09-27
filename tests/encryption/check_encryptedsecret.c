/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/log_stdout.h>

#include "ua_securechannel.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

static UA_Boolean verifyCalled;

static size_t
getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                       const void *channelContext) {
    return 96; /* Raw ECDSA P-384 signature */
}

static UA_StatusCode
verifySignature(const UA_SecurityPolicy *policy, void *channelContext,
                const UA_ByteString *message,
                const UA_ByteString *signature) {
    verifyCalled = true;
    return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
}

static UA_ByteString
malformedEncryptedSecret(const UA_String policyUri) {
    UA_NodeId typeId = UA_NS0ID(ECCENCRYPTEDSECRET);
    UA_Byte encodingMask = 0x01;
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_DateTime signingTime = 0;
    UA_UInt16 keyDataLen = 0;

    size_t headerPrefix =
        UA_calcSizeBinary(&typeId, &UA_TYPES[UA_TYPES_NODEID], NULL) +
        UA_calcSizeBinary(&encodingMask, &UA_TYPES[UA_TYPES_BYTE], NULL) +
        UA_calcSizeBinary(&(UA_UInt32){0}, &UA_TYPES[UA_TYPES_UINT32], NULL);
    size_t commonRemainder =
        UA_calcSizeBinary(&policyUri, &UA_TYPES[UA_TYPES_STRING], NULL) +
        UA_calcSizeBinary(&certificate, &UA_TYPES[UA_TYPES_BYTESTRING], NULL) +
        UA_calcSizeBinary(&signingTime, &UA_TYPES[UA_TYPES_DATETIME], NULL) +
        UA_calcSizeBinary(&keyDataLen, &UA_TYPES[UA_TYPES_UINT16], NULL);

    /* One byte follows the common header. This satisfies the outer Length
     * check but cannot contain the policy header, payload and 96-byte
     * signature. */
    UA_UInt32 length = (UA_UInt32)(commonRemainder + 1);
    UA_ByteString secret;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&secret,
                                                headerPrefix + length),
                      UA_STATUSCODE_GOOD);
    UA_Byte *pos = secret.data;
    const UA_Byte *end = secret.data + secret.length;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_NodeId_encodeBinary(&typeId, &pos, end);
    res |= UA_Byte_encodeBinary(&encodingMask, &pos, end);
    res |= UA_UInt32_encodeBinary(&length, &pos, end);
    res |= UA_String_encodeBinary(&policyUri, &pos, end);
    res |= UA_ByteString_encodeBinary(&certificate, &pos, end);
    res |= UA_DateTime_encodeBinary(&signingTime, &pos, end);
    res |= UA_UInt16_encodeBinary(&keyDataLen, &pos, end);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(pos + 1, end);
    *pos = 0;
    return secret;
}

START_TEST(rejectSignatureLengthLargerThanSecret) {
    UA_SecurityPolicy policy;
    memset(&policy, 0, sizeof(policy));
    policy.policyType = UA_SECURITYPOLICYTYPE_ECC;
    policy.policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384");
    policy.asymSignatureAlgorithm.getRemoteSignatureSize =
        getRemoteSignatureSize;
    policy.asymSignatureAlgorithm.verify = verifySignature;

    UA_ByteString secret = malformedEncryptedSecret(policy.policyUri);
    ck_assert_uint_lt(secret.length, 96);

    UA_SecureChannel channel;
    memset(&channel, 0, sizeof(channel));
    UA_Logger logger = *UA_Log_Stdout;
    verifyCalled = false;
    UA_StatusCode res = decryptUserTokenEcc(&logger, &channel, &policy,
                                            NULL, UA_BYTESTRING_NULL, &secret);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADIDENTITYTOKENINVALID);
    ck_assert(!verifyCalled);
    UA_ByteString_clear(&secret);
}
END_TEST

static Suite *
encryptedSecretSuite(void) {
    Suite *s = suite_create("encrypted-secret");
    TCase *tc = tcase_create("ECC bounds");
    tcase_add_test(tc, rejectSignatureLengthLargerThanSecret);
    suite_add_tcase(s, tc);
    return s;
}

int
main(void) {
    Suite *s = encryptedSecretSuite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
