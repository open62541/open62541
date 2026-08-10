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
    mbedtls_pk_init(&context->common.localPrivateKey);
    mbedtls_pk_init(&context->common.csrLocalPrivateKey);
    if(UA_mbedTLS_LoadPrivateKey(&localPrivateKey,
                                 &context->common.localPrivateKey) != 0 ||
       !UA_mbedTLS_compat_isEccKeyPair(&context->common.localPrivateKey)) {
        mbedtls_pk_free(&context->common.localPrivateKey);
        mbedtls_pk_free(&context->common.csrLocalPrivateKey);
        UA_free(context);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
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
    mbedtls_pk_free(&context->common.localPrivateKey);
    mbedtls_pk_free(&context->common.csrLocalPrivateKey);
    UA_ByteString_clear(&context->common.localCertThumbprint);
    UA_free(context);
    policy->policyContext = NULL;
}

static UA_StatusCode
eccNewChannelContext(const UA_SecurityPolicy *policy,
                  const UA_ByteString *remoteCertificate,
                  void **channelContext) {
    if(!policy || !remoteCertificate || !channelContext ||
       (remoteCertificate->length > 0 && !remoteCertificate->data))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    EccChannelContext *context = (EccChannelContext*)UA_calloc(1, sizeof(*context));
    if(!context)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    mbedtls_x509_crt_init(&context->common.remoteCertificate);
    UA_StatusCode res = UA_ByteString_copy(
        remoteCertificate, &context->remoteCertificateRaw);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_mbedTLS_LoadCertificate(
            &context->remoteCertificateRaw, &context->common.remoteCertificate);
    if(res != UA_STATUSCODE_GOOD) {
        mbedtls_x509_crt_free(&context->common.remoteCertificate);
        UA_ByteString_clear(&context->remoteCertificateRaw);
        UA_free(context);
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
    }
    UA_mbedTLS_ChannelContext_initPsa(&context->common);
    UA_mbedTLS_PsaKey_init(&context->localEphemeralKeyPair);
    *channelContext = context;
    return UA_STATUSCODE_GOOD;
}

static void
eccDeleteChannelContext(const UA_SecurityPolicy *policy, void *channelContext) {
    (void)policy;
    EccChannelContext *context = (EccChannelContext*)channelContext;
    if(!context)
        return;
    mbedtls_x509_crt_free(&context->common.remoteCertificate);
    UA_ByteString_clear(&context->remoteCertificateRaw);
    UA_mbedTLS_ChannelContext_clearPsa(&context->common);
    UA_ByteString_clear(&context->common.localSymIv);
    UA_ByteString_clear(&context->common.remoteSymIv);
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

static size_t
eccGetEncryptionBlockSize(const UA_SecurityPolicy *policy,
                       const void *channelContext) {
    (void)policy;
    (void)channelContext;
    return 16;
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
eccMakeCertificateThumbprint(const UA_SecurityPolicy *policy,
                          const UA_ByteString *certificate,
                          UA_ByteString *thumbprint) {
    (void)policy;
    return UA_mbedTLS_thumbprintSha1(certificate, thumbprint);
}

static UA_StatusCode
eccSymmetricSign(const UA_SecurityPolicy *policy, void *channelContext,
              const UA_ByteString *message, UA_ByteString *signature) {
    const UA_mbedTLS_EccPolicyConfig *config = eccGetConfig(policy);
    EccChannelContext *context = (EccChannelContext*)channelContext;
    if(!config || !context || !signature)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    signature->length = config->hashLength;
    return UA_mbedTLS_PsaMacCompute(
        context->common.localSymSigningKeyPsa.id,
        PSA_ALG_HMAC(config->hashAlgorithm), message, signature);
}

static UA_StatusCode
eccSymmetricVerify(const UA_SecurityPolicy *policy, void *channelContext,
                const UA_ByteString *message,
                const UA_ByteString *signature) {
    const UA_mbedTLS_EccPolicyConfig *config = eccGetConfig(policy);
    EccChannelContext *context = (EccChannelContext*)channelContext;
    if(!config || !context || !signature)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(signature->length != config->hashLength)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_mbedTLS_PsaMacVerify(
        context->common.remoteSymSigningKeyPsa.id,
        PSA_ALG_HMAC(config->hashAlgorithm), message, signature);
}

static UA_StatusCode
eccSymmetricEncrypt(const UA_SecurityPolicy *policy, void *channelContext,
                 UA_ByteString *data) {
    (void)policy;
    EccChannelContext *context = (EccChannelContext*)channelContext;
    if(!context || !data || context->common.localSymIv.length != 16 ||
       data->length % 16 != 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_mbedTLS_PsaCipher(
        context->common.localSymEncryptingKeyPsa.id, PSA_ALG_CBC_NO_PADDING,
        true, &context->common.localSymIv, data);
}

static UA_StatusCode
eccSymmetricDecrypt(const UA_SecurityPolicy *policy, void *channelContext,
                 UA_ByteString *data) {
    (void)policy;
    EccChannelContext *context = (EccChannelContext*)channelContext;
    if(!context || !data || context->common.remoteSymIv.length != 16 ||
       data->length % 16 != 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_mbedTLS_PsaCipher(
        context->common.remoteSymEncryptingKeyPsa.id, PSA_ALG_CBC_NO_PADDING,
        false, &context->common.remoteSymIv, data);
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
    symEnc->encrypt = eccSymmetricEncrypt;
    symEnc->decrypt = eccSymmetricDecrypt;
    symEnc->getLocalKeyLength = eccGetEncryptionKeyLength;
    symEnc->getRemoteKeyLength = eccGetEncryptionKeyLength;
    symEnc->getRemoteBlockSize = eccGetEncryptionBlockSize;
    symEnc->getRemotePlainTextBlockSize = eccGetEncryptionBlockSize;

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
    policy->makeCertThumbprint = eccMakeCertificateThumbprint;
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
