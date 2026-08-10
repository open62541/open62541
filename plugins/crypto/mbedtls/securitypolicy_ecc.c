/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/securitypolicy_default.h>

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)

#include "securitypolicy_ecc.h"
#include "securitypolicy_mbedtls_compat.h"

typedef struct {
    mbedtls_PolicyContext common;
    UA_ApplicationType applicationType;
    const UA_mbedTLS_EccPolicyConfig *config;
} EccPolicyContext;

typedef struct {
    mbedtls_ChannelContext common;
    UA_mbedTLS_PsaKey localEphemeralKeyPair;
    UA_ByteString remoteCertificateRaw;
} EccChannelContext;

static const UA_mbedTLS_EccPolicyConfig *
eccGetConfig(const UA_SecurityPolicy *policy) {
    if(!policy || !policy->policyContext)
        return NULL;
    return ((const EccPolicyContext*)policy->policyContext)->config;
}

static UA_StatusCode
eccNewPolicyContext(UA_SecurityPolicy *policy, UA_ByteString localPrivateKey,
                 UA_ApplicationType applicationType,
                 const UA_mbedTLS_EccPolicyConfig *config) {
    EccPolicyContext *context = (EccPolicyContext*)UA_calloc(1, sizeof(*context));
    if(!context)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_mbedTLS_PolicyContext_init(&context->common);
    UA_StatusCode res = UA_mbedTLS_LoadPrivateKey(
        &localPrivateKey, &context->common.localPrivateKey);
    if(res == UA_STATUSCODE_GOOD &&
       !UA_mbedTLS_compat_isEccKeyPair(&context->common.localPrivateKey))
        res = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    if(res != UA_STATUSCODE_GOOD) {
        UA_mbedTLS_PolicyContext_clear(&context->common);
        UA_free(context);
        return res;
    }
    res = UA_ByteString_allocBuffer(
        &context->common.localCertThumbprint, UA_SHA1_LENGTH);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_mbedTLS_thumbprintSha1(
            &policy->localCertificate, &context->common.localCertThumbprint);
    if(res != UA_STATUSCODE_GOOD) {
        UA_mbedTLS_PolicyContext_clear(&context->common);
        UA_free(context);
        return res;
    }
    context->applicationType = applicationType;
    context->config = config;
    policy->policyContext = context;
    return UA_STATUSCODE_GOOD;
}

static void
eccClearPolicyContext(UA_SecurityPolicy *policy) {
    if(!policy)
        return;
    UA_ByteString_clear(&policy->localCertificate);
    EccPolicyContext *context = (EccPolicyContext*)policy->policyContext;
    if(!context)
        return;
    UA_mbedTLS_PolicyContext_clear(&context->common);
    UA_free(context);
    policy->policyContext = NULL;
}

static UA_StatusCode
eccNewChannelContext(const UA_SecurityPolicy *policy,
                  const UA_ByteString *remoteCertificate,
                  void **channelContext) {
    const UA_mbedTLS_EccPolicyConfig *config = eccGetConfig(policy);
    if(!config || !remoteCertificate || !channelContext ||
       (remoteCertificate->length > 0 && !remoteCertificate->data))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    EccChannelContext *context = (EccChannelContext*)UA_calloc(1, sizeof(*context));
    if(!context)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_mbedTLS_ChannelContext_init(&context->common);
    context->common.symmetricMacAlgorithm =
        PSA_ALG_HMAC(config->hashAlgorithm);
    UA_mbedTLS_PsaKey_init(&context->localEphemeralKeyPair);
    UA_StatusCode res = UA_ByteString_copy(
        remoteCertificate, &context->remoteCertificateRaw);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_mbedTLS_LoadCertificate(
            &context->remoteCertificateRaw, &context->common.remoteCertificate);
    if(res != UA_STATUSCODE_GOOD) {
        UA_mbedTLS_ChannelContext_clear(&context->common);
        UA_ByteString_clear(&context->remoteCertificateRaw);
        UA_free(context);
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
    }
    *channelContext = context;
    return UA_STATUSCODE_GOOD;
}

static void
eccDeleteChannelContext(const UA_SecurityPolicy *policy, void *channelContext) {
    (void)policy;
    EccChannelContext *context = (EccChannelContext*)channelContext;
    if(!context)
        return;
    UA_ByteString_clear(&context->remoteCertificateRaw);
    UA_mbedTLS_ChannelContext_clear(&context->common);
    UA_mbedTLS_PsaKey_clear(&context->localEphemeralKeyPair);
    UA_free(context);
}

static size_t
eccGetAsymmetricSignatureSize(const UA_SecurityPolicy *policy,
                           const void *channelContext) {
    (void)channelContext;
    const UA_mbedTLS_EccPolicyConfig *config = eccGetConfig(policy);
    return config ? config->asymmetricSignatureLength : 0;
}

static size_t
eccGetOne(const UA_SecurityPolicy *policy, const void *channelContext) {
    (void)policy;
    (void)channelContext;
    return 1;
}

static size_t
eccGetHashLength(const UA_SecurityPolicy *policy, const void *channelContext) {
    (void)channelContext;
    const UA_mbedTLS_EccPolicyConfig *config = eccGetConfig(policy);
    return config ? config->hashLength : 0;
}

static size_t
eccGetEncryptionKeyLength(const UA_SecurityPolicy *policy,
                       const void *channelContext) {
    (void)channelContext;
    const UA_mbedTLS_EccPolicyConfig *config = eccGetConfig(policy);
    return config ? config->symmetricEncryptionKeyLength : 0;
}

static UA_StatusCode
eccGenerateNonce(const UA_SecurityPolicy *policy, void *channelContext,
              UA_ByteString *output) {
    const UA_mbedTLS_EccPolicyConfig *config = eccGetConfig(policy);
    EccChannelContext *context = (EccChannelContext*)channelContext;
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_mbedTLS_EccGenerateNonce(
        policy, context ? &context->localEphemeralKeyPair : NULL,
        config->family, config->keyBits, output);
}

static UA_StatusCode
eccGenerateKey(const UA_SecurityPolicy *policy, void *channelContext,
            const UA_ByteString *secret, const UA_ByteString *seed,
            UA_ByteString *output) {
    const EccPolicyContext *policyContext =
        policy ? (const EccPolicyContext*)policy->policyContext : NULL;
    EccChannelContext *context = (EccChannelContext*)channelContext;
    if(!policyContext || !context)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_mbedTLS_PsaEccDerive(
        policyContext->config->hashAlgorithm, policyContext->applicationType,
        &context->localEphemeralKeyPair, secret, seed, output);
}

static UA_StatusCode
eccAsymmetricSign(const UA_SecurityPolicy *policy, void *channelContext,
               const UA_ByteString *message, UA_ByteString *signature) {
    (void)channelContext;
    const EccPolicyContext *context =
        policy ? (const EccPolicyContext*)policy->policyContext : NULL;
    if(!context)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    psa_algorithm_t hash = context->config->hashAlgorithm;
    return UA_mbedTLS_PsaAsymmetricSign(
        &context->common.localPrivateKey, PSA_ALG_ECDSA(hash), hash,
        message, signature);
}

static UA_StatusCode
eccAsymmetricVerify(const UA_SecurityPolicy *policy, void *channelContext,
                 const UA_ByteString *message,
                 const UA_ByteString *signature) {
    const UA_mbedTLS_EccPolicyConfig *config = eccGetConfig(policy);
    EccChannelContext *context = (EccChannelContext*)channelContext;
    if(!config || !context)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_mbedTLS_PsaAsymmetricVerify(
        &context->common.remoteCertificate.pk,
        PSA_ALG_ECDSA(config->hashAlgorithm), config->hashAlgorithm,
        message, signature);
}

static UA_StatusCode
eccAsymmetricNoop(const UA_SecurityPolicy *policy, void *channelContext,
               UA_ByteString *data) {
    (void)policy;
    (void)channelContext;
    (void)data;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
eccSymmetricSign(const UA_SecurityPolicy *policy, void *channelContext,
              const UA_ByteString *message, UA_ByteString *signature) {
    const UA_mbedTLS_EccPolicyConfig *config = eccGetConfig(policy);
    EccChannelContext *context = (EccChannelContext*)channelContext;
    if(!config || !context)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_mbedTLS_symmetricSign(&context->common, config->hashLength,
                                    message, signature);
}

static UA_StatusCode
eccSymmetricVerify(const UA_SecurityPolicy *policy, void *channelContext,
                const UA_ByteString *message,
                const UA_ByteString *signature) {
    const UA_mbedTLS_EccPolicyConfig *config = eccGetConfig(policy);
    EccChannelContext *context = (EccChannelContext*)channelContext;
    if(!config || !context)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_mbedTLS_symmetricVerify(&context->common, config->hashLength,
                                      message, signature);
}

UA_StatusCode
UA_mbedTLS_SecurityPolicy_Ecc(UA_SecurityPolicy *policy,
                              UA_ApplicationType applicationType,
                              UA_ByteString localCertificate,
                              UA_ByteString localPrivateKey,
                              const UA_Logger *logger,
                              const UA_mbedTLS_EccPolicyConfig *config) {
    if(!policy || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    memset(policy, 0, sizeof(*policy));
    policy->logger = logger;
    UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                   "SecurityPolicy %s is deprecated (OPC UA Part 7); use "
                   "ECC_nistP256_AesGcm or ECC_nistP256_ChaChaPoly instead",
                   config->deprecatedName);
    policy->policyUri = UA_STRING(config->policyUri);
    policy->certificateGroupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    policy->certificateTypeId = UA_NODEID_NUMERIC(0, config->certificateTypeId);
    policy->securityLevel = config->securityLevel;
    policy->policyType = UA_SECURITYPOLICYTYPE_ECC;

    UA_SecurityPolicySignatureAlgorithm *asymSig =
        &policy->asymSignatureAlgorithm;
    asymSig->uri = UA_STRING_NULL;
    asymSig->verify = eccAsymmetricVerify;
    asymSig->sign = eccAsymmetricSign;
    asymSig->getLocalSignatureSize = eccGetAsymmetricSignatureSize;
    asymSig->getRemoteSignatureSize = eccGetAsymmetricSignatureSize;

    UA_SecurityPolicyEncryptionAlgorithm *asymEnc =
        &policy->asymEncryptionAlgorithm;
    asymEnc->uri = UA_STRING(
        "https://profiles.opcfoundation.org/conformanceunit/2720");
    asymEnc->encrypt = eccAsymmetricNoop;
    asymEnc->decrypt = eccAsymmetricNoop;
    asymEnc->getLocalKeyLength = eccGetOne;
    asymEnc->getRemoteKeyLength = eccGetOne;
    asymEnc->getRemoteBlockSize = eccGetOne;
    asymEnc->getRemotePlainTextBlockSize = eccGetOne;

    UA_SecurityPolicySignatureAlgorithm *symSig =
        &policy->symSignatureAlgorithm;
    symSig->uri = UA_STRING(config->symmetricSignatureUri);
    symSig->verify = eccSymmetricVerify;
    symSig->sign = eccSymmetricSign;
    symSig->getLocalSignatureSize = eccGetHashLength;
    symSig->getRemoteSignatureSize = eccGetHashLength;
    symSig->getLocalKeyLength = eccGetHashLength;
    symSig->getRemoteKeyLength = eccGetHashLength;

    UA_SecurityPolicyEncryptionAlgorithm *symEnc =
        &policy->symEncryptionAlgorithm;
    symEnc->uri = UA_STRING(config->symmetricEncryptionUri);
    symEnc->encrypt = UA_mbedTLS_symmetricEncrypt;
    symEnc->decrypt = UA_mbedTLS_symmetricDecrypt;
    symEnc->getLocalKeyLength = eccGetEncryptionKeyLength;
    symEnc->getRemoteKeyLength = eccGetEncryptionKeyLength;
    symEnc->getRemoteBlockSize = UA_mbedTLS_symmetricEncryptionBlockSize;
    symEnc->getRemotePlainTextBlockSize = UA_mbedTLS_symmetricEncryptionBlockSize;

    policy->certSignatureAlgorithm = policy->asymSignatureAlgorithm;
    policy->newChannelContext = eccNewChannelContext;
    policy->deleteChannelContext = eccDeleteChannelContext;
    policy->setLocalSymEncryptingKey =
        UA_mbedTLS_setLocalSymEncryptingKey_generic;
    policy->setLocalSymSigningKey = UA_mbedTLS_setLocalSymSigningKey_generic;
    policy->setLocalSymIv = UA_mbedTLS_setLocalSymIv_generic;
    policy->setRemoteSymEncryptingKey =
        UA_mbedTLS_setRemoteSymEncryptingKey_generic;
    policy->setRemoteSymSigningKey = UA_mbedTLS_setRemoteSymSigningKey_generic;
    policy->setRemoteSymIv = UA_mbedTLS_setRemoteSymIv_generic;
    policy->compareCertificate = UA_mbedTLS_compareCertificate_generic;
    policy->generateKey = eccGenerateKey;
    policy->generateNonce = eccGenerateNonce;
    policy->nonceLength = config->nonceLength;
    policy->makeCertThumbprint = UA_mbedTLS_makeCertificateThumbprint_generic;
    policy->compareCertThumbprint =
        UA_mbedTLS_compareCertificateThumbprint_generic;
    policy->updateCertificate = UA_mbedTLS_UpdateCertificateAndPrivateKey;
    policy->createSigningRequest = UA_mbedTLS_createSigningRequest_generic;
    policy->clear = eccClearPolicyContext;

    UA_StatusCode res = UA_mbedTLS_LoadLocalCertificate(
        &localCertificate, &policy->localCertificate);
    if(res == UA_STATUSCODE_GOOD)
        res = eccNewPolicyContext(policy, localPrivateKey, applicationType, config);
    if(res != UA_STATUSCODE_GOOD)
        eccClearPolicyContext(policy);
    return res;
}

#endif
