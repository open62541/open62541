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
verify_testing(const UA_SecurityPolicy *policy, void *channelContext,
               const UA_ByteString *message, const UA_ByteString *signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_sign_testing(const UA_SecurityPolicy *policy,
                  void *channelContext, const UA_ByteString *message,
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
sym_sign_testing(const UA_SecurityPolicy *policy,
                 void *channelContext, const UA_ByteString *message,
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
asym_getLocalSignatureSize_testing(const UA_SecurityPolicy *policy,
                                   const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->asym_lcl_sig_size;
}

static size_t
asym_getRemoteSignatureSize_testing(const UA_SecurityPolicy *policy,
                                    const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->asym_rmt_sig_size;
}

static size_t
asym_getLocalEncryptionKeyLength_testing(const UA_SecurityPolicy *policy,
                                         const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->asym_lcl_enc_key_size;
}

static size_t
asym_getRemoteEncryptionKeyLength_testing(const UA_SecurityPolicy *policy,
                                          const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->asym_rmt_enc_key_size;
}

static size_t
sym_getLocalSignatureSize_testing(const UA_SecurityPolicy *policy,
                                  const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_sig_size;
}

static size_t
sym_getRemoteSignatureSize_testing(const UA_SecurityPolicy *policy,
                                   const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_sig_size;
}

static size_t
sym_getLocalSigningKeyLength_testing(const UA_SecurityPolicy *policy,
                                     const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_sig_keyLen;
}

static size_t
sym_getRemoteSigningKeyLength_testing(const UA_SecurityPolicy *policy,
                                      const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_sig_keyLen; // TODO: Remote sig key len
}

static size_t
sym_getLocalEncryptionKeyLength_testing(const UA_SecurityPolicy *policy,
                                        const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_enc_keyLen;
}

static size_t
sym_getRemoteEncryptionKeyLength_testing(const UA_SecurityPolicy *policy,
                                         const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_enc_keyLen;
}

static size_t
sym_getEncryptionBlockSize_testing(const UA_SecurityPolicy *policy,
                                   const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_enc_blockSize;
}

static size_t
sym_getPlainTextBlockSize_testing(const UA_SecurityPolicy *policy,
                                  const void *channelContext) {
    ck_assert(channelContext != NULL);
    return keySizes->sym_enc_blockSize;
}

static UA_StatusCode
sym_encrypt_testing(const UA_SecurityPolicy *policy,
                    void *channelContext, UA_ByteString *data) {
    SET_CALLED(sym_enc);
    ck_assert(channelContext != NULL);
    ck_assert(data != NULL);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_encrypt_testing(const UA_SecurityPolicy *policy,
                     void *channelContext, UA_ByteString *data) {
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
decrypt_testing(const UA_SecurityPolicy *policy,
                void *channelContext, UA_ByteString *data) {
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
generateKey_testing(const UA_SecurityPolicy *policy,
                    void *channelContext, const UA_ByteString *secret,
                    const UA_ByteString *seed, UA_ByteString *out) {
    ck_assert(secret != NULL);
    ck_assert(seed != NULL);
    ck_assert(out != NULL);
    SET_CALLED(generateKey);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
generateNonce_testing(const UA_SecurityPolicy *policy,
                      void *channelContext, UA_ByteString *out) {
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
deleteContext_testing(const UA_SecurityPolicy *policy,
                      void *channelContext) {
    SET_CALLED(deleteContext);
    ck_assert(channelContext != NULL);
}

static UA_StatusCode
setLocalSymEncryptingKey_testing(const UA_SecurityPolicy *policy,
                                 void *channelContext, const UA_ByteString *key) {
    SET_CALLED(setLocalSymEncryptingKey);
    ck_assert(channelContext != NULL);
    ck_assert(key != NULL);
    ck_assert(key->data != NULL);
    ck_assert_msg(key->length == keySizes->sym_enc_keyLen,
                  "Expected length to be %u but got %u",
                  (unsigned int)keySizes->sym_enc_keyLen,
                  (unsigned int)key->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setLocalSymSigningKey_testing(const UA_SecurityPolicy *policy,
                              void *channelContext, const UA_ByteString *key) {
    SET_CALLED(setLocalSymSigningKey);
    ck_assert(channelContext != NULL);
    ck_assert(key != NULL);
    ck_assert(key->data != NULL);
    ck_assert_msg(key->length == keySizes->sym_sig_keyLen,
                  "Expected length to be %u but got %u",
                  (unsigned int)keySizes->sym_sig_keyLen,
                  (unsigned int)key->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setLocalSymIv_testing(const UA_SecurityPolicy *policy,
                      void *channelContext, const UA_ByteString *iv) {
    SET_CALLED(setLocalSymIv);
    ck_assert(channelContext != NULL);
    ck_assert(iv != NULL);
    ck_assert(iv->data != NULL);
    ck_assert_msg(iv->length == keySizes->sym_enc_blockSize,
                  "Expected length to be %u but got %u",
                  (unsigned int)keySizes->sym_enc_blockSize,
                  (unsigned int)iv->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setRemoteSymEncryptingKey_testing(const UA_SecurityPolicy *policy,
                                  void *channelContext, const UA_ByteString *key) {
    SET_CALLED(setRemoteSymEncryptingKey);
    ck_assert(channelContext != NULL);
    ck_assert(key != NULL);
    ck_assert(key->data != NULL);
    ck_assert_msg(key->length == keySizes->sym_enc_keyLen,
                  "Expected length to be %u but got %u",
                  (unsigned int)keySizes->sym_enc_keyLen,
                  (unsigned int)key->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setRemoteSymSigningKey_testing(const UA_SecurityPolicy *policy,
                               void *channelContext, const UA_ByteString *key) {
    SET_CALLED(setRemoteSymSigningKey);
    ck_assert(channelContext != NULL);
    ck_assert(key != NULL);
    ck_assert(key->data != NULL);
    ck_assert_msg(key->length == keySizes->sym_sig_keyLen,
                  "Expected length to be %u but got %u",
                  (unsigned int)keySizes->sym_sig_keyLen,
                  (unsigned int)key->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setRemoteSymIv_testing(const UA_SecurityPolicy *policy,
                       void *channelContext, const UA_ByteString *iv) {
    SET_CALLED(setRemoteSymIv);
    ck_assert(channelContext != NULL);
    ck_assert(iv != NULL);
    ck_assert(iv->data != NULL);
    ck_assert_msg(iv->length == keySizes->sym_enc_blockSize,
                  "Expected length to be %u but got %u",
                  (unsigned int)keySizes->sym_enc_blockSize,
                  (unsigned int)iv->length);
    return UA_STATUSCODE_GOOD;
}

static size_t
asym_getRemotePlainTextBlockSize_testing(const UA_SecurityPolicy *policy,
                                         const void *channelContext) {
    return keySizes->asym_rmt_ptext_blocksize;
}

static size_t
asym_getRemoteBlockSize_testing(const UA_SecurityPolicy *policy,
                                const void *channelContext) {
    return keySizes->asym_rmt_blocksize;
}

static UA_StatusCode
compareCertificate_testing(const UA_SecurityPolicy *policy,
                           const void *channelContext,
                           const UA_ByteString *certificate) {
    return UA_STATUSCODE_GOOD;
}

static void
policy_clear_testing(UA_SecurityPolicy *policy) {
    UA_ByteString_clear(&policy->localCertificate);
}

UA_StatusCode
TestingPolicy(UA_SecurityPolicy *sp, UA_ByteString localCertificate,
              funcs_called *fCalled, const key_sizes *kSizes) {
    keySizes = kSizes;
    funcsCalled = fCalled;

    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = UA_Log_Stdout;
    sp->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Testing");
    sp->certificateGroupId = UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    sp->certificateTypeId = UA_NS0ID(RSASHA256APPLICATIONCERTIFICATETYPE);
    sp->securityLevel = 20;
    sp->policyType = UA_SECURITYPOLICYTYPE_RSA;

    /* Asymmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *asymSig = &sp->asymSignatureAlgorithm;
    asymSig->uri = UA_STRING_NULL;
    asymSig->verify = verify_testing;
    asymSig->sign = asym_sign_testing;
    asymSig->getLocalSignatureSize = asym_getLocalSignatureSize_testing;
    asymSig->getRemoteSignatureSize = asym_getRemoteSignatureSize_testing;

    /* Asymmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri = UA_STRING_NULL;
    asymEnc->encrypt = asym_encrypt_testing;
    asymEnc->decrypt = decrypt_testing;
    asymEnc->getLocalKeyLength = asym_getLocalEncryptionKeyLength_testing;
    asymEnc->getRemoteKeyLength = asym_getRemoteEncryptionKeyLength_testing;
    asymEnc->getRemoteBlockSize = asym_getRemoteBlockSize_testing;
    asymEnc->getRemotePlainTextBlockSize = asym_getRemotePlainTextBlockSize_testing;

    /* Symmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = UA_STRING_NULL;
    symSig->verify = verify_testing;
    symSig->sign = sym_sign_testing;
    symSig->getLocalSignatureSize = sym_getLocalSignatureSize_testing;
    symSig->getRemoteSignatureSize = sym_getRemoteSignatureSize_testing;
    symSig->getLocalKeyLength = sym_getLocalSigningKeyLength_testing;
    symSig->getRemoteKeyLength = sym_getRemoteSigningKeyLength_testing;

    /* Symmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = UA_STRING_NULL;
    symEnc->encrypt = sym_encrypt_testing;
    symEnc->decrypt = decrypt_testing;
    symEnc->getLocalKeyLength = sym_getLocalEncryptionKeyLength_testing;
    symEnc->getRemoteKeyLength = sym_getRemoteEncryptionKeyLength_testing;
    symEnc->getRemoteBlockSize = sym_getEncryptionBlockSize_testing;
    symEnc->getRemotePlainTextBlockSize = sym_getPlainTextBlockSize_testing;

    /* Use the same signature algorithm as the asymmetric component for
     * certificate signing (see standard) */
    sp->certSignatureAlgorithm = sp->asymSignatureAlgorithm;

    /* Direct Method Pointers */
    sp->newChannelContext = newContext_testing;
    sp->deleteChannelContext = deleteContext_testing;
    sp->setLocalSymEncryptingKey = setLocalSymEncryptingKey_testing;
    sp->setLocalSymSigningKey = setLocalSymSigningKey_testing;
    sp->setLocalSymIv = setLocalSymIv_testing;
    sp->setRemoteSymEncryptingKey = setRemoteSymEncryptingKey_testing;
    sp->setRemoteSymSigningKey = setRemoteSymSigningKey_testing;
    sp->setRemoteSymIv = setRemoteSymIv_testing;
    sp->compareCertificate = compareCertificate_testing;
    sp->generateKey = generateKey_testing;
    sp->generateNonce = generateNonce_testing;
    sp->nonceLength = 32;
    sp->makeCertThumbprint = makeThumbprint_testing;
    sp->compareCertThumbprint = compareThumbprint_testing;
    sp->updateCertificate = NULL;
    sp->createSigningRequest = NULL;
    sp->clear = policy_clear_testing;

    UA_ByteString_copy(&localCertificate, &sp->localCertificate);
    sp->policyContext = (void *)funcsCalled;

    return UA_STATUSCODE_GOOD;
}

#endif
