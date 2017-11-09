/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <ua_types.h>
#include "ua_securitypolicy_none.h"
#include "ua_types_generated_handling.h"
#include "testing_policy.h"
#include "check.h"

#define SET_CALLED(func) funcsCalled->func = true

#define SYM_ENCRYPTION_BLOCK_SIZE 3
#define SYM_SIGNING_KEY_LENGTH 5
#define SYM_ENCRYPTION_KEY_LENGTH 7

funcs_called *funcsCalled;

static UA_StatusCode
verify_testing(const UA_SecurityPolicy *securityPolicy,
               const void *channelContext,
               const UA_ByteString *message,
               const UA_ByteString *signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sign_testing(const UA_SecurityPolicy *securityPolicy,
             const void *channelContext,
             const UA_ByteString *message,
             UA_ByteString *signature) {
    return UA_STATUSCODE_GOOD;
}

static size_t
length_testing(const UA_SecurityPolicy *securityPolicy,
               const void *channelContext) {
    return ENCRYPTION_KEY_LENGTH;
}

static UA_StatusCode
encrypt_testing(const UA_SecurityPolicy *securityPolicy,
                const void *channelContext,
                UA_ByteString *data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
decrypt_testing(const UA_SecurityPolicy *securityPolicy,
                const void *channelContext,
                UA_ByteString *data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
makeThumbprint_testing(const UA_SecurityPolicy *securityPolicy,
                       const UA_ByteString *certificate,
                       UA_ByteString *thumbprint) {
    SET_CALLED(makeCertificateThumbprint);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
compareThumbprint_testing(const UA_SecurityPolicy *securityPolicy,
                          const UA_ByteString *certificateThumbprint) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
generateKey_testing(const UA_SecurityPolicy *securityPolicy,
                    const UA_ByteString *secret,
                    const UA_ByteString *seed,
                    UA_ByteString *out) {
    SET_CALLED(generateKey);
    ck_assert(securityPolicy);
    ck_assert(secret);
    ck_assert(seed);
    ck_assert(out);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
generateNonce_testing(const UA_SecurityPolicy *securityPolicy,
                      UA_ByteString *out) {
    out->data = UA_Byte_new();
    if(!out->data)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    out->length = 1;
    out->data[0] = 'a';
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
newContext_testing(const UA_SecurityPolicy *securityPolicy,
                   const UA_ByteString *remoteCertificate,
                   void **channelContext) {
    SET_CALLED(newContext);
    ck_assert(securityPolicy);
    ck_assert(remoteCertificate);
    ck_assert(channelContext);

    *channelContext = (void *) funcsCalled;
    return UA_STATUSCODE_GOOD;
}

static void
deleteContext_testing(void *channelContext) {
    SET_CALLED(deleteContext);
    ck_assert(channelContext);
}

static UA_StatusCode
setLocalSymEncryptingKey_testing(void *channelContext,
                                 const UA_ByteString *val) {
    SET_CALLED(setLocalSymEncryptingKey);
    ck_assert(channelContext);
    ck_assert(val);
    ck_assert(val->length == SYM_ENCRYPTION_KEY_LENGTH);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setLocalSymSigningKey_testing(void *channelContext,
                              const UA_ByteString *val) {
    SET_CALLED(setLocalSymSigningKey);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setLocalSymIv_testing(void *channelContext,
                      const UA_ByteString *val) {
    SET_CALLED(setLocalSymIv);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setRemoteSymEncryptingKey_testing(void *channelContext,
                                  const UA_ByteString *val) {
    SET_CALLED(setRemoteSymEncryptingKey);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setRemoteSymSigningKey_testing(void *channelContext,
                               const UA_ByteString *val) {
    SET_CALLED(setRemoteSymSigningKey);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setRemoteSymIv_testing(void *channelContext,
                       const UA_ByteString *val) {
    SET_CALLED(setRemoteSymIv);
    return UA_STATUSCODE_GOOD;
}

static size_t
getRemoteAsymPlainTextBlockSize_testing(const void *channelContext) {
    return 5;
}

static size_t
getRemoteAsymEncryptionBufferLengthOverhead_testing(const void *channelContext,
                                                    size_t maxEncryptionLength) {
    return 0;
}

static UA_StatusCode
compareCertificate_testing(const void *channelContext,
                           const UA_ByteString *certificate) {
    return UA_STATUSCODE_GOOD;
}

static void
policy_deletemembers_testing(UA_SecurityPolicy *policy) {
    UA_ByteString_deleteMembers(&policy->localCertificate);
}

UA_StatusCode
TestingPolicy(UA_SecurityPolicy *policy, const UA_ByteString localCertificate,
              funcs_called *fCalled) {
    funcsCalled = fCalled;
    policy->policyContext = (void *) funcsCalled;
    policy->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Testing");
    policy->logger = NULL;
    UA_ByteString_copy(&localCertificate, &policy->localCertificate);

    policy->asymmetricModule.makeCertificateThumbprint = makeThumbprint_testing;
    policy->asymmetricModule.compareCertificateThumbprint = compareThumbprint_testing;
    policy->asymmetricModule.cryptoModule.signatureAlgorithmUri = UA_STRING_NULL;
    policy->asymmetricModule.cryptoModule.verify = verify_testing;
    policy->asymmetricModule.cryptoModule.sign = sign_testing;
    policy->asymmetricModule.cryptoModule.getLocalSignatureSize = length_testing;
    policy->asymmetricModule.cryptoModule.getRemoteSignatureSize = length_testing;
    policy->asymmetricModule.cryptoModule.encrypt = encrypt_testing;
    policy->asymmetricModule.cryptoModule.decrypt = decrypt_testing;
    policy->asymmetricModule.cryptoModule.getLocalEncryptionKeyLength = length_testing;
    policy->asymmetricModule.cryptoModule.getRemoteEncryptionKeyLength = length_testing;

    policy->symmetricModule.generateKey = generateKey_testing;
    policy->symmetricModule.generateNonce = generateNonce_testing;
    policy->symmetricModule.cryptoModule.signatureAlgorithmUri = UA_STRING_NULL;
    policy->symmetricModule.cryptoModule.verify = verify_testing;
    policy->symmetricModule.cryptoModule.sign = sign_testing;
    policy->symmetricModule.cryptoModule.getLocalSignatureSize = length_testing;
    policy->symmetricModule.cryptoModule.getRemoteSignatureSize = length_testing;
    policy->symmetricModule.cryptoModule.encrypt = encrypt_testing;
    policy->symmetricModule.cryptoModule.decrypt = decrypt_testing;
    policy->symmetricModule.cryptoModule.getLocalEncryptionKeyLength = length_testing;
    policy->symmetricModule.cryptoModule.getRemoteEncryptionKeyLength = length_testing;
    policy->symmetricModule.encryptionBlockSize = SYM_ENCRYPTION_BLOCK_SIZE;
    policy->symmetricModule.signingKeyLength = SYM_SIGNING_KEY_LENGTH;

    policy->channelModule.newContext = newContext_testing;
    policy->channelModule.deleteContext = deleteContext_testing;
    policy->channelModule.setLocalSymEncryptingKey = setLocalSymEncryptingKey_testing;
    policy->channelModule.setLocalSymSigningKey = setLocalSymSigningKey_testing;
    policy->channelModule.setLocalSymIv = setLocalSymIv_testing;
    policy->channelModule.setRemoteSymEncryptingKey = setRemoteSymEncryptingKey_testing;
    policy->channelModule.setRemoteSymSigningKey = setRemoteSymSigningKey_testing;
    policy->channelModule.setRemoteSymIv = setRemoteSymIv_testing;
    policy->channelModule.compareCertificate = compareCertificate_testing;
    policy->channelModule.getRemoteAsymPlainTextBlockSize = getRemoteAsymPlainTextBlockSize_testing;
    policy->channelModule.getRemoteAsymEncryptionBufferLengthOverhead =
        getRemoteAsymEncryptionBufferLengthOverhead_testing;
    policy->deleteMembers = policy_deletemembers_testing;

    return UA_STATUSCODE_GOOD;
}
