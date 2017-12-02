/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __clang_analyzer__

#include <ua_types.h>
#include <ua_plugin_securitypolicy.h>
#include "ua_types_generated_handling.h"
#include "testing_policy.h"
#include "check.h"

#define SET_CALLED(func) funcsCalled->func = true

static funcs_called *funcsCalled;
static const key_sizes *keySizes;

static UA_StatusCode
verify_testing(const UA_SecurityPolicy *securityPolicy,
               const void *channelContext,
               const UA_ByteString *message,
               const UA_ByteString *signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_sign_testing(const UA_SecurityPolicy *securityPolicy,
                  const void *channelContext,
                  const UA_ByteString *message,
                  UA_ByteString *signature) {
    SET_CALLED(asym_sign);
    ck_assert(securityPolicy != NULL);
    ck_assert(channelContext != NULL);
    ck_assert(message != NULL);
    ck_assert(signature != NULL);

    ck_assert_msg(signature->length == keySizes->asym_lcl_sig_size,
                  "Expected signature length to be %i but was %i",
                  keySizes->asym_lcl_sig_size,
                  signature->length);

    memset(signature->data, '*', signature->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_sign_testing(const UA_SecurityPolicy *securityPolicy,
                 const void *channelContext,
                 const UA_ByteString *message,
                 UA_ByteString *signature) {
    SET_CALLED(sym_sign);
    ck_assert(securityPolicy != NULL);
    ck_assert(channelContext != NULL);
    ck_assert(message != NULL);
    ck_assert(signature != NULL);
    ck_assert(signature->length != 0);
    ck_assert(signature->data != NULL);

    memset(signature->data, 'S', signature->length);
    return UA_STATUSCODE_GOOD;
}

static size_t
asym_getLocalSignatureSize_testing(const UA_SecurityPolicy *securityPolicy,
                                   const void *channelContext) {
    ck_assert(securityPolicy != NULL);
    ck_assert(channelContext != NULL);

    return keySizes->asym_lcl_sig_size;
}

static size_t
asym_getRemoteSignatureSize_testing(const UA_SecurityPolicy *securityPolicy,
                                    const void *channelContext) {
    ck_assert(securityPolicy != NULL);
    ck_assert(channelContext != NULL);

    return keySizes->asym_rmt_sig_size;
}

static size_t
asym_getLocalEncryptionKeyLength_testing(const UA_SecurityPolicy *securityPolicy,
                                         const void *channelContext) {
    return keySizes->asym_lcl_enc_key_size;
}

static size_t
asym_getRemoteEncryptionKeyLength_testing(const UA_SecurityPolicy *securityPolicy,
                                          const void *channelContext) {
    return keySizes->asym_rmt_enc_key_size;
}

static size_t
sym_getLocalSignatureSize_testing(const UA_SecurityPolicy *securityPolicy,
                                  const void *channelContext) {
    return 0;
}

static size_t
sym_getRemoteSignatureSize_testing(const UA_SecurityPolicy *securityPolicy,
                                   const void *channelContext) {
    return 0;
}

static size_t
sym_getLocalEncryptionKeyLength_testing(const UA_SecurityPolicy *securityPolicy,
                                        const void *channelContext) {
    ck_assert(securityPolicy != NULL);
    ck_assert(channelContext != NULL);
    return keySizes->sym_enc_keyLen;
}

static size_t
sym_getRemoteEncryptionKeyLength_testing(const UA_SecurityPolicy *securityPolicy,
                                         const void *channelContext) {
    return keySizes->sym_enc_keyLen;
}

static UA_StatusCode
sym_encrypt_testing(const UA_SecurityPolicy *securityPolicy,
                    const void *channelContext,
                    UA_ByteString *data) {
    SET_CALLED(sym_enc);
    ck_assert(securityPolicy != NULL);
    ck_assert(channelContext != NULL);
    ck_assert(data != NULL);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_encrypt_testing(const UA_SecurityPolicy *securityPolicy,
                     const void *channelContext,
                     UA_ByteString *data) {
    SET_CALLED(asym_enc);
    ck_assert(securityPolicy != NULL);
    ck_assert(channelContext != NULL);
    ck_assert(data != NULL);

    size_t blockSize = securityPolicy->channelModule.getRemoteAsymPlainTextBlockSize(securityPolicy);
    ck_assert_msg(data->length % blockSize == 0,
                  "Expected the length of the data to be encrypted to be a multiple of the plaintext block size (%i). "
                      "Remainder was %i",
                  blockSize,
                  data->length % blockSize);

    for(size_t i = 0; i < data->length; ++i) {
        data->data[i] = (UA_Byte) ((data->data[i] + 1) % (UA_BYTE_MAX + 1));
    }

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

    ck_assert(securityPolicy != NULL);
    ck_assert(certificate != NULL);
    ck_assert(thumbprint != NULL);

    ck_assert_msg(thumbprint->length == 20, "Thumbprints have to be 20 bytes long (current specification)");
    memset(thumbprint->data, 42, 20);

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
    ck_assert(securityPolicy != NULL);
    ck_assert(secret != NULL);
    ck_assert(seed != NULL);
    ck_assert(out != NULL);
    SET_CALLED(generateKey);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
generateNonce_testing(const UA_SecurityPolicy *securityPolicy,
                      UA_ByteString *out) {
    ck_assert(securityPolicy != NULL);
    ck_assert(out != NULL);
    ck_assert(out->data != NULL);

    memset(out->data, 'N', out->length);

    SET_CALLED(generateNonce);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
newContext_testing(const UA_SecurityPolicy *securityPolicy,
                   const UA_ByteString *remoteCertificate,
                   void **channelContext) {
    SET_CALLED(newContext);
    ck_assert(securityPolicy != NULL);
    ck_assert(remoteCertificate != NULL);
    ck_assert(channelContext != NULL);

    ck_assert(funcsCalled != NULL);
    *channelContext = (void *) funcsCalled;
    return UA_STATUSCODE_GOOD;
}

static void
deleteContext_testing(void *channelContext) {
    SET_CALLED(deleteContext);
    ck_assert(channelContext != NULL);
}

static UA_StatusCode
setLocalSymEncryptingKey_testing(void *channelContext,
                                 const UA_ByteString *val) {
    SET_CALLED(setLocalSymEncryptingKey);
    ck_assert(channelContext != NULL);
    ck_assert(val != NULL);
    ck_assert(val->data != NULL);
    ck_assert_msg(val->length == keySizes->sym_enc_keyLen,
                  "Expected length to be %i but got %i",
                  keySizes->sym_enc_keyLen,
                  val->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setLocalSymSigningKey_testing(void *channelContext,
                              const UA_ByteString *val) {
    SET_CALLED(setLocalSymSigningKey);
    ck_assert(channelContext != NULL);
    ck_assert(val != NULL);
    ck_assert(val->data != NULL);
    ck_assert_msg(val->length == keySizes->sym_sig_keyLen,
                  "Expected length to be %i but got %i",
                  keySizes->sym_sig_keyLen,
                  val->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setLocalSymIv_testing(void *channelContext,
                      const UA_ByteString *val) {
    SET_CALLED(setLocalSymIv);
    ck_assert(channelContext != NULL);
    ck_assert(val != NULL);
    ck_assert(val->data != NULL);
    ck_assert_msg(val->length == keySizes->sym_enc_blockSize,
                  "Expected length to be %i but got %i",
                  keySizes->sym_enc_blockSize,
                  val->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setRemoteSymEncryptingKey_testing(void *channelContext,
                                  const UA_ByteString *val) {
    SET_CALLED(setRemoteSymEncryptingKey);
    ck_assert(channelContext != NULL);
    ck_assert(val != NULL);
    ck_assert(val->data != NULL);
    ck_assert_msg(val->length == keySizes->sym_enc_keyLen,
                  "Expected length to be %i but got %i",
                  keySizes->sym_enc_keyLen,
                  val->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setRemoteSymSigningKey_testing(void *channelContext,
                               const UA_ByteString *val) {
    SET_CALLED(setRemoteSymSigningKey);
    ck_assert(channelContext != NULL);
    ck_assert(val != NULL);
    ck_assert(val->data != NULL);
    ck_assert_msg(val->length == keySizes->sym_sig_keyLen,
                  "Expected length to be %i but got %i",
                  keySizes->sym_sig_keyLen,
                  val->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setRemoteSymIv_testing(void *channelContext,
                       const UA_ByteString *val) {
    SET_CALLED(setRemoteSymIv);
    ck_assert(channelContext != NULL);
    ck_assert(val != NULL);
    ck_assert(val->data != NULL);
    ck_assert_msg(val->length == keySizes->sym_enc_blockSize,
                  "Expected length to be %i but got %i",
                  keySizes->sym_enc_blockSize,
                  val->length);
    return UA_STATUSCODE_GOOD;
}

static size_t
getRemoteAsymPlainTextBlockSize_testing(const void *channelContext) {
    return keySizes->asym_rmt_ptext_blocksize;
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
              funcs_called *fCalled, const key_sizes *kSizes) {
    keySizes = kSizes;
    funcsCalled = fCalled;
    policy->policyContext = (void *) funcsCalled;
    policy->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Testing");
    policy->logger = NULL;
    UA_ByteString_copy(&localCertificate, &policy->localCertificate);

    policy->asymmetricModule.makeCertificateThumbprint = makeThumbprint_testing;
    policy->asymmetricModule.compareCertificateThumbprint = compareThumbprint_testing;
    policy->asymmetricModule.cryptoModule.signatureAlgorithmUri = UA_STRING_NULL;
    policy->asymmetricModule.cryptoModule.verify = verify_testing;
    policy->asymmetricModule.cryptoModule.sign = asym_sign_testing;
    policy->asymmetricModule.cryptoModule.getLocalSignatureSize = asym_getLocalSignatureSize_testing;
    policy->asymmetricModule.cryptoModule.getRemoteSignatureSize = asym_getRemoteSignatureSize_testing;
    policy->asymmetricModule.cryptoModule.encrypt = asym_encrypt_testing;
    policy->asymmetricModule.cryptoModule.decrypt = decrypt_testing;
    policy->asymmetricModule.cryptoModule.getLocalEncryptionKeyLength = asym_getLocalEncryptionKeyLength_testing;
    policy->asymmetricModule.cryptoModule.getRemoteEncryptionKeyLength = asym_getRemoteEncryptionKeyLength_testing;

    policy->symmetricModule.generateKey = generateKey_testing;
    policy->symmetricModule.generateNonce = generateNonce_testing;
    policy->symmetricModule.cryptoModule.signatureAlgorithmUri = UA_STRING_NULL;
    policy->symmetricModule.cryptoModule.verify = verify_testing;
    policy->symmetricModule.cryptoModule.sign = sym_sign_testing;
    policy->symmetricModule.cryptoModule.getLocalSignatureSize = sym_getLocalSignatureSize_testing;
    policy->symmetricModule.cryptoModule.getRemoteSignatureSize = sym_getRemoteSignatureSize_testing;
    policy->symmetricModule.cryptoModule.encrypt = sym_encrypt_testing;
    policy->symmetricModule.cryptoModule.decrypt = decrypt_testing;
    policy->symmetricModule.cryptoModule.getLocalEncryptionKeyLength = sym_getLocalEncryptionKeyLength_testing;
    policy->symmetricModule.cryptoModule.getRemoteEncryptionKeyLength = sym_getRemoteEncryptionKeyLength_testing;
    policy->symmetricModule.encryptionBlockSize = keySizes->sym_enc_blockSize;
    policy->symmetricModule.signingKeyLength = keySizes->sym_sig_keyLen;

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

#endif
