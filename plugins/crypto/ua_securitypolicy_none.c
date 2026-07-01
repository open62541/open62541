/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2017-2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/securitypolicy_default.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
#include "mbedtls/securitypolicy_common.h"
#endif

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
#include "openssl/securitypolicy_common.h"
#endif

#ifndef UA_ENABLE_ENCRYPTION
/* The URI-derived SecurityPolicy property helpers are normally provided by the
 * crypto backend's securitypolicy_common.c. With no crypto backend the only
 * SecurityPolicy is #None, so they reduce to constants. */
UA_Boolean
UA_SecurityPolicy_isEnhancedSecurity(const UA_SecurityPolicy *policy) {
    (void)policy;
    return false;
}

UA_Boolean
UA_SecurityPolicy_useLegacySequenceNumbers(const UA_SecurityPolicy *policy) {
    (void)policy;
    return true;
}
#endif

static UA_StatusCode
verify_none(const UA_SecurityPolicy *policy, void *channelContext,
            const UA_ByteString *message, const UA_ByteString *signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sign_none(const UA_SecurityPolicy *policy, void *channelContext,
          const UA_ByteString *message, UA_ByteString *signature) {
    return UA_STATUSCODE_GOOD;
}

static size_t
length_none(const UA_SecurityPolicy *policy, const void *channelContext) {
    return 0;
}

static UA_StatusCode
encrypt_none(const UA_SecurityPolicy *policy, void *channelContext,
             UA_ByteString *data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
decrypt_none(const UA_SecurityPolicy *policy, void *channelContext,
             UA_ByteString *data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
makeThumbprint_none(const UA_SecurityPolicy *policy,
                    const UA_ByteString *certificate,
                    UA_ByteString *thumbprint) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
compareThumbprint_none(const UA_SecurityPolicy *policy,
                       const UA_ByteString *thumbprint) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
generateKey_none(const UA_SecurityPolicy *policy,
                 void *channelContext, const UA_ByteString *secret,
                 const UA_ByteString *seed, UA_ByteString *out) {
    return UA_STATUSCODE_GOOD;
}

/* Use the non-cryptographic RNG to set the nonce */
static UA_StatusCode
generateNonce_none(const UA_SecurityPolicy *policy,
                   void *channelContext, UA_ByteString *out) {
    if(out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(out->length == 0)
        return UA_STATUSCODE_GOOD;

    /* Fill blocks of four byte */
    size_t i = 0;
    while(i + 3 < out->length) {
        UA_UInt32 randNumber = UA_UInt32_random();
        memcpy(&out->data[i], &randNumber, 4);
        i = i+4;
    }

    /* Fill the remaining byte */
    UA_UInt32 randNumber = UA_UInt32_random();
    memcpy(&out->data[i], &randNumber, out->length % 4);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
newContext_none(const UA_SecurityPolicy *securityPolicy,
                const UA_ByteString *remoteCertificate,
                void **channelContext) {
    return UA_STATUSCODE_GOOD;
}

static void
deleteContext_none(const UA_SecurityPolicy *policy, void *channelContext) {
}

static UA_StatusCode
setContextValue_none(const UA_SecurityPolicy *policy,
                     void *channelContext,
                     const UA_ByteString *iv) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
compareCertificate_none(const UA_SecurityPolicy *policy,
                        const void *channelContext,
                        const UA_ByteString *certificate) {
    return UA_STATUSCODE_GOOD;
}

static void policy_clear_none(UA_SecurityPolicy *policy);

static UA_StatusCode
updateCertificate_none(UA_SecurityPolicy *policy,
                       const UA_ByteString certificate,
                       const UA_ByteString privateKey) {
    UA_ByteString_clear(&policy->localCertificate);
    /* An empty certificate is valid for the #None policy (no crypto). Only
     * treat an actual copy failure of a provided certificate as an error. */
    if(certificate.length == 0)
        return UA_STATUSCODE_GOOD;
    UA_StatusCode retval = UA_ByteString_copy(&certificate, &policy->localCertificate);
    if(retval != UA_STATUSCODE_GOOD)
        policy_clear_none(policy);
    return retval;
}


static void
policy_clear_none(UA_SecurityPolicy *policy) {
    UA_ByteString_clear(&policy->localCertificate);
}

UA_StatusCode
UA_SecurityPolicy_None(UA_SecurityPolicy *sp, const UA_ByteString localCertificate,
                       const UA_Logger *logger) {

    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = logger;
    sp->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    sp->certificateGroupId = UA_NODEID_NULL;
    sp->certificateTypeId = UA_NODEID_NULL;
    sp->securityLevel = 0;
    sp->policyType = UA_SECURITYPOLICYTYPE_NONE;

    /* Symmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = UA_STRING_NULL;
    symSig->verify = verify_none;
    symSig->sign = sign_none;
    symSig->getLocalSignatureSize = length_none;
    symSig->getRemoteSignatureSize = length_none;
    symSig->getLocalKeyLength = length_none;
    symSig->getRemoteKeyLength = length_none;

    /* Symmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = UA_STRING_NULL;
    symEnc->encrypt = encrypt_none;
    symEnc->decrypt = decrypt_none;
    symEnc->getLocalKeyLength = length_none;
    symEnc->getRemoteKeyLength = length_none;
    symEnc->getRemoteBlockSize = length_none;
    symEnc->getRemotePlainTextBlockSize = length_none;

    /* This only works for none since symmetric and asymmetric crypto modules do
     * the same i.e. nothing */
    sp->asymSignatureAlgorithm = sp->symSignatureAlgorithm;
    sp->asymEncryptionAlgorithm = sp->symEncryptionAlgorithm;

    /* Use the same signing algorithm as for asymmetric signing */
    sp->certSignatureAlgorithm = sp->asymSignatureAlgorithm;

    /* Direct Method Pointers */
    sp->newChannelContext = newContext_none;
    sp->deleteChannelContext = deleteContext_none;
    sp->setLocalSymEncryptingKey = setContextValue_none;
    sp->setLocalSymSigningKey = setContextValue_none;
    sp->setLocalSymIv = setContextValue_none;
    sp->setRemoteSymEncryptingKey = setContextValue_none;
    sp->setRemoteSymSigningKey = setContextValue_none;
    sp->setRemoteSymIv = setContextValue_none;
    sp->compareCertificate = compareCertificate_none;
    sp->generateKey = generateKey_none;
    sp->generateNonce = generateNonce_none;
    sp->nonceLength = 0;
    sp->makeCertThumbprint = makeThumbprint_none;
    sp->compareCertThumbprint = compareThumbprint_none;
    sp->updateCertificate = updateCertificate_none;
    sp->createSigningRequest = NULL;
    sp->clear = policy_clear_none;

    /* The #None policy does no crypto, so an empty local certificate is valid
     * and is the common case (e.g. UA_ServerConfig_setMinimal with NULL cert).
     * Only fail when a non-empty certificate was provided but could not be
     * loaded. Otherwise the crypto backends return an error for empty input
     * and server creation fails. */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(localCertificate.length > 0) {
#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
        retval = UA_mbedTLS_LoadLocalCertificate(&localCertificate, &sp->localCertificate);
#elif defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
        retval = UA_OpenSSL_LoadLocalCertificate(&localCertificate, &sp->localCertificate,
                                                 EVP_PKEY_NONE);
#else
        retval = UA_ByteString_copy(&localCertificate, &sp->localCertificate);
#endif
        if(retval != UA_STATUSCODE_GOOD)
            policy_clear_none(sp);
    }

    return retval;
}
