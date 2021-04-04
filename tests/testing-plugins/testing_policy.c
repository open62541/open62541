/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __clang_analyzer__

#include "testing_policy.h"

#include <open62541/plugin/log_stdout.h>

#include <check.h>

#define SET_CALLED(func) funcsCalled->func = true

static funcs_called *funcsCalled;
static const key_sizes *keySizes;

static UA_StatusCode
verify_testing(void *channelContext, const UA_ByteString *message,
               const UA_ByteString *signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_sign_testing(void *channelContext, const UA_ByteString *message,
                  UA_ByteString *signature) {
    SET_CALLED(asym_sign);
    ck_assert(channelContext != NULL);
    ck_assert(message != NULL);
    ck_assert(signature != NULL);

    ck_assert_msg(signature->length == keySizes->asym_lcl_sig_size,
                  "Expected signature length to be %u but was %u",
                  (unsigned int)keySizes->asym_lcl_sig_size,
                  (unsigned int)signature->length);

    memset(signature->data, '*', signature->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_sign_testing(void *channelContext, const UA_ByteString *message,
                 UA_ByteString *signature) {
    SET_CALLED(sym_sign);
    ck_assert(channelContext != NULL);
    ck_assert(message != NULL);
    ck_assert(signature != NULL);
    ck_assert(signature->length != 0);
    ck_assert(signature->data != NULL);

    memset(signature->data, 'S', signature->length);
    return UA_STATUSCODE_GOOD;
}

static size_t
asym_getLocalSignatureSize_testing(const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->asym_lcl_sig_size;
}

static size_t
asym_getRemoteSignatureSize_testing(const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->asym_rmt_sig_size;
}

static size_t
asym_getLocalEncryptionKeyLength_testing(const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->asym_lcl_enc_key_size;
}

static size_t
asym_getRemoteEncryptionKeyLength_testing(const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->asym_rmt_enc_key_size;
}

static size_t
sym_getLocalSignatureSize_testing(const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_sig_size;
}

static size_t
sym_getRemoteSignatureSize_testing(const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_sig_size;
}

static size_t
sym_getLocalSigningKeyLength_testing(const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_sig_keyLen;
}

static size_t
sym_getRemoteSigningKeyLength_testing(const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_sig_keyLen; // TODO: Remote sig key len
}

static size_t
sym_getLocalEncryptionKeyLength_testing(const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_enc_keyLen;
}

static size_t
sym_getRemoteEncryptionKeyLength_testing(const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_enc_keyLen;
}

static size_t
sym_getEncryptionBlockSize_testing(const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_enc_blockSize;
}

static size_t
sym_getPlainTextBlockSize_testing(const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_enc_blockSize;
}

static UA_StatusCode
sym_encrypt_testing(void *channelContext,
                    UA_ByteString *data) {
    SET_CALLED(sym_enc);
    ck_assert(channelContext != NULL);
    ck_assert(data != NULL);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_encrypt_testing(void *channelContext,
                     UA_ByteString *data) {
    SET_CALLED(asym_enc);
    ck_assert(channelContext != NULL);
    ck_assert(data != NULL);

    size_t blockSize = keySizes->sym_enc_blockSize;
    ck_assert_msg(data->length % blockSize == 0,
                  "Expected the length of the data to be encrypted to be a "
                  "multiple of the plaintext block size (%u). Remainder was %u",
                  (unsigned int)blockSize, (unsigned int)(data->length % blockSize));

    for(size_t i = 0; i < data->length; ++i) {
        data->data[i] = (UA_Byte)((data->data[i] + 1) % (UA_BYTE_MAX + 1));
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
decrypt_testing(void *channelContext, UA_ByteString *data) {
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
generateKey_testing(void *policyContext,
                    const UA_ByteString *secret,
                    const UA_ByteString *seed,
                    UA_ByteString *out) {
    ck_assert(secret != NULL);
    ck_assert(seed != NULL);
    ck_assert(out != NULL);
    SET_CALLED(generateKey);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
generateNonce_testing(void *policyContext,
                      UA_ByteString *out) {
    ck_assert(out != NULL);
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
    *channelContext = (void *)funcsCalled;
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
                  "Expected length to be %u but got %u",
                  (unsigned int)keySizes->sym_enc_keyLen,
                  (unsigned int)val->length);
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
                  "Expected length to be %u but got %u",
                  (unsigned int)keySizes->sym_sig_keyLen,
                  (unsigned int)val->length);
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
                  "Expected length to be %u but got %u",
                  (unsigned int)keySizes->sym_enc_blockSize,
                  (unsigned int)val->length);
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
                  "Expected length to be %u but got %u",
                  (unsigned int)keySizes->sym_enc_keyLen,
                  (unsigned int)val->length);
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
                  "Expected length to be %u but got %u",
                  (unsigned int)keySizes->sym_sig_keyLen,
                  (unsigned int)val->length);
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
                  "Expected length to be %u but got %u",
                  (unsigned int)keySizes->sym_enc_blockSize,
                  (unsigned int)val->length);
    return UA_STATUSCODE_GOOD;
}

static size_t
asym_getRemotePlainTextBlockSize_testing(const void *channelContext) {
    return keySizes->asym_rmt_ptext_blocksize;
}

static size_t
asym_getRemoteBlockSize_testing(const void *channelContext) {
    return keySizes->asym_rmt_blocksize;
}

static UA_StatusCode
compareCertificate_testing(const void *channelContext,
                           const UA_ByteString *certificate) {
    return UA_STATUSCODE_GOOD;
}

static void
policy_clear_testing(UA_SecurityPolicy *policy) {
    UA_ByteString_clear(&policy->localCertificate);
}

UA_StatusCode
TestingPolicy(UA_SecurityPolicy *policy, UA_ByteString localCertificate,
              funcs_called *fCalled, const key_sizes *kSizes) {
    keySizes = kSizes;
    funcsCalled = fCalled;
    policy->policyContext = (void *)funcsCalled;
    policy->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Testing");
    policy->logger = UA_Log_Stdout;
    UA_ByteString_copy(&localCertificate, &policy->localCertificate);

    policy->asymmetricModule.makeCertificateThumbprint = makeThumbprint_testing;
    policy->asymmetricModule.compareCertificateThumbprint = compareThumbprint_testing;

    UA_SecurityPolicySignatureAlgorithm *asym_signatureAlgorithm =
        &policy->asymmetricModule.cryptoModule.signatureAlgorithm;
    asym_signatureAlgorithm->uri = UA_STRING_NULL;
    asym_signatureAlgorithm->verify = verify_testing;
    asym_signatureAlgorithm->sign = asym_sign_testing;
    asym_signatureAlgorithm->getLocalSignatureSize = asym_getLocalSignatureSize_testing;
    asym_signatureAlgorithm->getRemoteSignatureSize = asym_getRemoteSignatureSize_testing;

    UA_SecurityPolicyEncryptionAlgorithm *asym_encryptionAlgorithm =
        &policy->asymmetricModule.cryptoModule.encryptionAlgorithm;
    asym_encryptionAlgorithm->encrypt = asym_encrypt_testing;
    asym_encryptionAlgorithm->decrypt = decrypt_testing;
    asym_encryptionAlgorithm->getLocalKeyLength = asym_getLocalEncryptionKeyLength_testing;
    asym_encryptionAlgorithm->getRemoteKeyLength = asym_getRemoteEncryptionKeyLength_testing;
    asym_encryptionAlgorithm->getRemotePlainTextBlockSize = asym_getRemotePlainTextBlockSize_testing;
    asym_encryptionAlgorithm->getRemoteBlockSize = asym_getRemoteBlockSize_testing;

    policy->symmetricModule.generateKey = generateKey_testing;
    policy->symmetricModule.generateNonce = generateNonce_testing;

    UA_SecurityPolicySignatureAlgorithm *sym_signatureAlgorithm =
        &policy->symmetricModule.cryptoModule.signatureAlgorithm;
    sym_signatureAlgorithm->uri = UA_STRING_NULL;
    sym_signatureAlgorithm->verify = verify_testing;
    sym_signatureAlgorithm->sign = sym_sign_testing;
    sym_signatureAlgorithm->getLocalSignatureSize = sym_getLocalSignatureSize_testing;
    sym_signatureAlgorithm->getRemoteSignatureSize = sym_getRemoteSignatureSize_testing;
    sym_signatureAlgorithm->getLocalKeyLength = sym_getLocalSigningKeyLength_testing;
    sym_signatureAlgorithm->getRemoteKeyLength = sym_getRemoteSigningKeyLength_testing;

    UA_SecurityPolicyEncryptionAlgorithm *sym_encryptionAlgorithm =
        &policy->symmetricModule.cryptoModule.encryptionAlgorithm;
    sym_encryptionAlgorithm->encrypt = sym_encrypt_testing;
    sym_encryptionAlgorithm->decrypt = decrypt_testing;
    sym_encryptionAlgorithm->getLocalKeyLength = sym_getLocalEncryptionKeyLength_testing;
    sym_encryptionAlgorithm->getRemoteKeyLength = sym_getRemoteEncryptionKeyLength_testing;
    sym_encryptionAlgorithm->getRemoteBlockSize = sym_getEncryptionBlockSize_testing;
    sym_encryptionAlgorithm->getRemotePlainTextBlockSize = sym_getPlainTextBlockSize_testing;

    policy->channelModule.newContext = newContext_testing;
    policy->channelModule.deleteContext = deleteContext_testing;
    policy->channelModule.setLocalSymEncryptingKey = setLocalSymEncryptingKey_testing;
    policy->channelModule.setLocalSymSigningKey = setLocalSymSigningKey_testing;
    policy->channelModule.setLocalSymIv = setLocalSymIv_testing;
    policy->channelModule.setRemoteSymEncryptingKey = setRemoteSymEncryptingKey_testing;
    policy->channelModule.setRemoteSymSigningKey = setRemoteSymSigningKey_testing;
    policy->channelModule.setRemoteSymIv = setRemoteSymIv_testing;
    policy->channelModule.compareCertificate = compareCertificate_testing;
    policy->clear = policy_clear_testing;

    return UA_STATUSCODE_GOOD;
}

#endif
