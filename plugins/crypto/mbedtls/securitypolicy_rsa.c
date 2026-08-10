/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/securitypolicy_default.h>

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)

#include "securitypolicy_rsa.h"
#include "securitypolicy_mbedtls_compat.h"

typedef struct {
    mbedtls_PolicyContext common;
    const UA_mbedTLS_RsaPolicyConfig *config;
} RsaPolicyContext;

static const UA_mbedTLS_RsaPolicyConfig *
rsaGetConfig(const UA_SecurityPolicy *policy) {
    if(!policy || !policy->policyContext)
        return NULL;
    return ((const RsaPolicyContext*)policy->policyContext)->config;
}

static UA_StatusCode
rsaMakeCertificateThumbprint(const UA_SecurityPolicy *policy,
                             const UA_ByteString *certificate,
                             UA_ByteString *thumbprint) {
    (void)policy;
    return UA_mbedTLS_thumbprintSha1(certificate, thumbprint);
}

static void
rsaClearPolicyContext(UA_SecurityPolicy *policy) {
    if(!policy)
        return;
    UA_ByteString_clear(&policy->localCertificate);
    RsaPolicyContext *context = (RsaPolicyContext*)policy->policyContext;
    if(!context)
        return;
    mbedtls_pk_free(&context->common.localPrivateKey);
    mbedtls_pk_free(&context->common.csrLocalPrivateKey);
    UA_ByteString_clear(&context->common.localCertThumbprint);
    UA_free(context);
    policy->policyContext = NULL;
}

static UA_StatusCode
rsaNewPolicyContext(UA_SecurityPolicy *policy, UA_ByteString privateKey,
                    const UA_mbedTLS_RsaPolicyConfig *config) {
    if(privateKey.length == 0 || !privateKey.data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    RsaPolicyContext *context = (RsaPolicyContext*)UA_calloc(1, sizeof(*context));
    if(!context)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    mbedtls_pk_init(&context->common.localPrivateKey);
    mbedtls_pk_init(&context->common.csrLocalPrivateKey);
    if(UA_mbedTLS_LoadPrivateKey(
           &privateKey, &context->common.localPrivateKey) != 0 ||
       !UA_mbedTLS_compat_isRsaKeyPair(&context->common.localPrivateKey)) {
        mbedtls_pk_free(&context->common.localPrivateKey);
        mbedtls_pk_free(&context->common.csrLocalPrivateKey);
        UA_free(context);
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }
    UA_StatusCode res = UA_ByteString_allocBuffer(
        &context->common.localCertThumbprint, UA_SHA1_LENGTH);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_mbedTLS_thumbprintSha1(
            &policy->localCertificate, &context->common.localCertThumbprint);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&context->common.localCertThumbprint);
        mbedtls_pk_free(&context->common.localPrivateKey);
        mbedtls_pk_free(&context->common.csrLocalPrivateKey);
        UA_free(context);
        return res;
    }
    context->config = config;
    policy->policyContext = context;
    return UA_STATUSCODE_GOOD;
}

static void
rsaDeleteChannelContext(const UA_SecurityPolicy *policy,
                        void *channelContext) {
    (void)policy;
    mbedtls_ChannelContext *context = (mbedtls_ChannelContext*)channelContext;
    if(!context)
        return;
    UA_ByteString_clear(&context->localSymIv);
    UA_ByteString_clear(&context->remoteSymIv);
    mbedtls_x509_crt_free(&context->remoteCertificate);
    UA_mbedTLS_ChannelContext_clearPsa(context);
    UA_free(context);
}

static UA_StatusCode
rsaNewChannelContext(const UA_SecurityPolicy *policy,
                     const UA_ByteString *remoteCertificate,
                     void **channelContext) {
    const UA_mbedTLS_RsaPolicyConfig *config = rsaGetConfig(policy);
    if(!config || !remoteCertificate || !channelContext ||
       remoteCertificate->length == 0 || !remoteCertificate->data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    mbedtls_ChannelContext *context =
        (mbedtls_ChannelContext*)UA_calloc(1, sizeof(*context));
    if(!context)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    mbedtls_x509_crt_init(&context->remoteCertificate);
    UA_mbedTLS_ChannelContext_initPsa(context);
    UA_StatusCode res = UA_mbedTLS_LoadCertificate(
        remoteCertificate, &context->remoteCertificate);
    if(res == UA_STATUSCODE_GOOD) {
        size_t keyLength =
            (mbedtls_pk_get_bitlen(&context->remoteCertificate.pk) + 7) / 8;
        if(keyLength < config->minimumAsymmetricKeyLength ||
           keyLength > config->maximumAsymmetricKeyLength)
            res = UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
    }
    if(res != UA_STATUSCODE_GOOD) {
        rsaDeleteChannelContext(policy, context);
        return res;
    }
    *channelContext = context;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
rsaAsymmetricSign(const UA_SecurityPolicy *policy, void *channelContext,
                  const UA_ByteString *message, UA_ByteString *signature) {
    (void)channelContext;
    const RsaPolicyContext *context =
        policy ? (const RsaPolicyContext*)policy->policyContext : NULL;
    if(!context)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_mbedTLS_PsaAsymmetricSign(
        &context->common.localPrivateKey, context->config->signatureAlgorithm,
        context->config->hashAlgorithm, message, signature);
}

static UA_StatusCode
rsaAsymmetricVerify(const UA_SecurityPolicy *policy, void *channelContext,
                    const UA_ByteString *message,
                    const UA_ByteString *signature) {
    const UA_mbedTLS_RsaPolicyConfig *config = rsaGetConfig(policy);
    mbedtls_ChannelContext *context = (mbedtls_ChannelContext*)channelContext;
    if(!config || !context)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_mbedTLS_PsaAsymmetricVerify(
        &context->remoteCertificate.pk, config->signatureAlgorithm,
        config->hashAlgorithm, message, signature);
}

static size_t
rsaGetRemotePlainTextBlockSize(const UA_SecurityPolicy *policy,
                               const void *channelContext) {
    const UA_mbedTLS_RsaPolicyConfig *config = rsaGetConfig(policy);
    const mbedtls_ChannelContext *context =
        (const mbedtls_ChannelContext*)channelContext;
    if(!config || !context)
        return 0;
    size_t blockSize =
        (mbedtls_pk_get_bitlen(&context->remoteCertificate.pk) + 7) / 8;
    return blockSize > config->asymmetricPaddingOverhead ?
        blockSize - config->asymmetricPaddingOverhead : 0;
}

static size_t
rsaGetRemoteKeyBitLength(const UA_SecurityPolicy *policy,
                         const void *channelContext) {
    (void)policy;
    const mbedtls_ChannelContext *context =
        (const mbedtls_ChannelContext*)channelContext;
    return context ? mbedtls_pk_get_bitlen(&context->remoteCertificate.pk) : 0;
}

static UA_StatusCode
rsaAsymmetricEncrypt(const UA_SecurityPolicy *policy, void *channelContext,
                     UA_ByteString *data) {
    const UA_mbedTLS_RsaPolicyConfig *config = rsaGetConfig(policy);
    mbedtls_ChannelContext *context = (mbedtls_ChannelContext*)channelContext;
    if(!config || !context)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_mbedTLS_PsaAsymmetricEncrypt(
        &context->remoteCertificate.pk, config->encryptionAlgorithm,
        rsaGetRemotePlainTextBlockSize(policy, context), data);
}

static UA_StatusCode
rsaAsymmetricDecrypt(const UA_SecurityPolicy *policy, void *channelContext,
                     UA_ByteString *data) {
    (void)channelContext;
    const RsaPolicyContext *context =
        policy ? (const RsaPolicyContext*)policy->policyContext : NULL;
    if(!context)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_mbedTLS_PsaAsymmetricDecrypt(
        &context->common.localPrivateKey,
        context->config->encryptionAlgorithm, data);
}

static UA_StatusCode
rsaCertificateSign(const UA_SecurityPolicy *policy, void *channelContext,
                   const UA_ByteString *message, UA_ByteString *signature) {
    (void)channelContext;
    const RsaPolicyContext *context =
        policy ? (const RsaPolicyContext*)policy->policyContext : NULL;
    if(!context)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_mbedTLS_PsaAsymmetricSign(
        &context->common.localPrivateKey,
        context->config->certificateSignatureAlgorithm,
        context->config->hashAlgorithm, message, signature);
}

static UA_StatusCode
rsaCertificateVerify(const UA_SecurityPolicy *policy, void *channelContext,
                     const UA_ByteString *message,
                     const UA_ByteString *signature) {
    const UA_mbedTLS_RsaPolicyConfig *config = rsaGetConfig(policy);
    mbedtls_ChannelContext *context = (mbedtls_ChannelContext*)channelContext;
    if(!config || !context)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_mbedTLS_PsaAsymmetricVerify(
        &context->remoteCertificate.pk, config->certificateSignatureAlgorithm,
        config->hashAlgorithm, message, signature);
}

static size_t
rsaGetHashLength(const UA_SecurityPolicy *policy, const void *channelContext) {
    (void)channelContext;
    const UA_mbedTLS_RsaPolicyConfig *config = rsaGetConfig(policy);
    return config ? config->hashLength : 0;
}

static size_t
rsaGetSigningKeyLength(const UA_SecurityPolicy *policy,
                       const void *channelContext) {
    (void)channelContext;
    const UA_mbedTLS_RsaPolicyConfig *config = rsaGetConfig(policy);
    return config ? config->symmetricSigningKeyLength : 0;
}

static size_t
rsaGetEncryptionKeyLength(const UA_SecurityPolicy *policy,
                          const void *channelContext) {
    (void)channelContext;
    const UA_mbedTLS_RsaPolicyConfig *config = rsaGetConfig(policy);
    return config ? config->symmetricEncryptionKeyLength : 0;
}

static size_t
rsaGetEncryptionBlockSize(const UA_SecurityPolicy *policy,
                          const void *channelContext) {
    (void)policy;
    (void)channelContext;
    return 16;
}

static UA_StatusCode
rsaSymmetricSign(const UA_SecurityPolicy *policy, void *channelContext,
                 const UA_ByteString *message, UA_ByteString *signature) {
    const UA_mbedTLS_RsaPolicyConfig *config = rsaGetConfig(policy);
    mbedtls_ChannelContext *context = (mbedtls_ChannelContext*)channelContext;
    if(!config || !context || !signature || signature->length != config->hashLength)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_mbedTLS_PsaMacCompute(
        context->localSymSigningKeyPsa.id,
        PSA_ALG_HMAC(config->hashAlgorithm), message, signature);
}

static UA_StatusCode
rsaSymmetricVerify(const UA_SecurityPolicy *policy, void *channelContext,
                   const UA_ByteString *message,
                   const UA_ByteString *signature) {
    const UA_mbedTLS_RsaPolicyConfig *config = rsaGetConfig(policy);
    mbedtls_ChannelContext *context = (mbedtls_ChannelContext*)channelContext;
    if(!config || !context || !signature)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(signature->length != config->hashLength)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_mbedTLS_PsaMacVerify(
        context->remoteSymSigningKeyPsa.id,
        PSA_ALG_HMAC(config->hashAlgorithm), message, signature);
}

static UA_StatusCode
rsaSymmetricCrypt(const UA_SecurityPolicy *policy, void *channelContext,
                  UA_ByteString *data, UA_Boolean encrypt) {
    (void)policy;
    mbedtls_ChannelContext *context = (mbedtls_ChannelContext*)channelContext;
    if(!context || !data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const UA_ByteString *iv = encrypt ?
        &context->localSymIv : &context->remoteSymIv;
    if(iv->length != 16 || data->length % 16 != 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_mbedTLS_PsaCipher(
        encrypt ? context->localSymEncryptingKeyPsa.id :
                  context->remoteSymEncryptingKeyPsa.id,
        PSA_ALG_CBC_NO_PADDING, encrypt, iv, data);
}

static UA_StatusCode
rsaSymmetricEncrypt(const UA_SecurityPolicy *policy, void *channelContext,
                    UA_ByteString *data) {
    return rsaSymmetricCrypt(policy, channelContext, data, true);
}

static UA_StatusCode
rsaSymmetricDecrypt(const UA_SecurityPolicy *policy, void *channelContext,
                    UA_ByteString *data) {
    return rsaSymmetricCrypt(policy, channelContext, data, false);
}

UA_StatusCode
UA_mbedTLS_SecurityPolicy_Rsa(UA_SecurityPolicy *policy,
                              UA_ByteString localCertificate,
                              UA_ByteString localPrivateKey,
                              const UA_Logger *logger,
                              const UA_mbedTLS_RsaPolicyConfig *config) {
    if(!policy || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    memset(policy, 0, sizeof(*policy));
    policy->logger = logger;
    if(config->deprecatedName)
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                       "SecurityPolicy %s is deprecated", config->deprecatedName);
    policy->policyUri = UA_STRING(config->policyUri);
    policy->certificateGroupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    policy->certificateTypeId = UA_NODEID_NUMERIC(0, config->certificateTypeId);
    policy->securityLevel = config->securityLevel;
    policy->policyType = UA_SECURITYPOLICYTYPE_RSA;

    UA_SecurityPolicySignatureAlgorithm *asymSig =
        &policy->asymSignatureAlgorithm;
    asymSig->uri = UA_STRING(config->asymmetricSignatureUri);
    asymSig->verify = rsaAsymmetricVerify;
    asymSig->sign = rsaAsymmetricSign;
    asymSig->getLocalSignatureSize = UA_mbedTLS_getLocalPrivateKeyLength;
    asymSig->getRemoteSignatureSize =
        UA_mbedTLS_asym_getRemoteSignatureSize_generic;

    UA_SecurityPolicyEncryptionAlgorithm *asymEnc =
        &policy->asymEncryptionAlgorithm;
    asymEnc->uri = UA_STRING(config->asymmetricEncryptionUri);
    asymEnc->encrypt = rsaAsymmetricEncrypt;
    asymEnc->decrypt = rsaAsymmetricDecrypt;
    asymEnc->getLocalKeyLength = UA_mbedTLS_getLocalPrivateKeyBitLength;
    asymEnc->getRemoteKeyLength = rsaGetRemoteKeyBitLength;
    asymEnc->getRemoteBlockSize = UA_mbedTLS_asym_getRemoteBlockSize_generic;
    asymEnc->getRemotePlainTextBlockSize = rsaGetRemotePlainTextBlockSize;

    UA_SecurityPolicySignatureAlgorithm *symSig =
        &policy->symSignatureAlgorithm;
    symSig->uri = UA_STRING(config->symmetricSignatureUri);
    symSig->verify = rsaSymmetricVerify;
    symSig->sign = rsaSymmetricSign;
    symSig->getLocalSignatureSize = rsaGetHashLength;
    symSig->getRemoteSignatureSize = rsaGetHashLength;
    symSig->getLocalKeyLength = rsaGetSigningKeyLength;
    symSig->getRemoteKeyLength = rsaGetSigningKeyLength;

    UA_SecurityPolicyEncryptionAlgorithm *symEnc =
        &policy->symEncryptionAlgorithm;
    symEnc->uri = UA_STRING(config->symmetricEncryptionUri);
    symEnc->encrypt = rsaSymmetricEncrypt;
    symEnc->decrypt = rsaSymmetricDecrypt;
    symEnc->getLocalKeyLength = rsaGetEncryptionKeyLength;
    symEnc->getRemoteKeyLength = rsaGetEncryptionKeyLength;
    symEnc->getRemoteBlockSize = rsaGetEncryptionBlockSize;
    symEnc->getRemotePlainTextBlockSize = rsaGetEncryptionBlockSize;

    policy->certSignatureAlgorithm = policy->asymSignatureAlgorithm;
    if(config->certificateSignatureAlgorithm != config->signatureAlgorithm) {
        policy->certSignatureAlgorithm.uri =
            UA_STRING(config->certificateSignatureUri);
        policy->certSignatureAlgorithm.verify = rsaCertificateVerify;
        policy->certSignatureAlgorithm.sign = rsaCertificateSign;
    }

    policy->newChannelContext = rsaNewChannelContext;
    policy->deleteChannelContext = rsaDeleteChannelContext;
    policy->setLocalSymEncryptingKey =
        UA_mbedTLS_setLocalSymEncryptingKey_generic;
    policy->setLocalSymSigningKey = UA_mbedTLS_setLocalSymSigningKey_generic;
    policy->setLocalSymIv = UA_mbedTLS_setLocalSymIv_generic;
    policy->setRemoteSymEncryptingKey =
        UA_mbedTLS_setRemoteSymEncryptingKey_generic;
    policy->setRemoteSymSigningKey = UA_mbedTLS_setRemoteSymSigningKey_generic;
    policy->setRemoteSymIv = UA_mbedTLS_setRemoteSymIv_generic;
    policy->compareCertificate = UA_mbedTLS_compareCertificate_generic;
    policy->generateKey = UA_mbedTLS_sym_generateKey_generic;
    policy->generateNonce = UA_mbedTLS_sym_generateNonce_generic;
    policy->nonceLength = config->nonceLength;
    policy->makeCertThumbprint = rsaMakeCertificateThumbprint;
    policy->compareCertThumbprint =
        UA_mbedTLS_compareCertificateThumbprint_generic;
    policy->updateCertificate = UA_mbedTLS_UpdateCertificateAndPrivateKey;
    policy->createSigningRequest = UA_mbedTLS_createSigningRequest_generic;
    policy->clear = rsaClearPolicyContext;

    UA_StatusCode res = UA_mbedTLS_LoadLocalCertificate(
        &localCertificate, &policy->localCertificate);
    if(res == UA_STATUSCODE_GOOD)
        res = rsaNewPolicyContext(policy, localPrivateKey, config);
    if(res != UA_STATUSCODE_GOOD)
        rsaClearPolicyContext(policy);
    return res;
}

#endif
