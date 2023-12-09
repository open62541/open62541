/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2017-2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include <open62541/plugin/securitypolicy_default.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
#include "mbedtls/securitypolicy_mbedtls_common.h"
#endif

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
#include "openssl/securitypolicy_openssl_common.h"
#endif

static UA_StatusCode
verify_none(void *channelContext,
            const UA_ByteString *message,
            const UA_ByteString *signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sign_none(void *channelContext, const UA_ByteString *message,
          UA_ByteString *signature) {
    return UA_STATUSCODE_GOOD;
}

static size_t
length_none(const void *channelContext) {
    return 0;
}

static UA_StatusCode
encrypt_none(void *channelContext, UA_ByteString *data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
decrypt_none(void *channelContext, UA_ByteString *data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
makeThumbprint_none(const UA_SecurityPolicy *securityPolicy,
                    const UA_ByteString *certificate,
                    UA_ByteString *thumbprint) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
compareThumbprint_none(const UA_SecurityPolicy *securityPolicy,
                       const UA_ByteString *certificateThumbprint) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
generateKey_none(void *policyContext, const UA_ByteString *secret,
                 const UA_ByteString *seed, UA_ByteString *out) {
    return UA_STATUSCODE_GOOD;
}

/* Use the non-cryptographic RNG to set the nonce */
static UA_StatusCode
generateNonce_none(void *policyContext, UA_ByteString *out) {
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
deleteContext_none(void *channelContext) {
}

static UA_StatusCode
setContextValue_none(void *channelContext,
                     const UA_ByteString *key) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
compareCertificate_none(const void *channelContext,
                        const UA_ByteString *certificate) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
updateCertificateAndPrivateKey_none(UA_SecurityPolicy *policy,
                                    const UA_ByteString newCertificate,
                                    const UA_ByteString newPrivateKey) {
    UA_ByteString_clear(&policy->localCertificate);
    UA_ByteString_copy(&newCertificate, &policy->localCertificate);
    return UA_STATUSCODE_GOOD;
}


static void
policy_clear_none(UA_SecurityPolicy *policy) {
    UA_ByteString_clear(&policy->localCertificate);
}

UA_StatusCode
UA_SecurityPolicy_None(UA_SecurityPolicy *policy, const UA_ByteString localCertificate,
                       const UA_Logger *logger) {
    policy->policyContext = (void *)(uintptr_t)logger;
    policy->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    policy->securityLevel = 0;
    policy->logger = logger;

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
    UA_mbedTLS_LoadLocalCertificate(&localCertificate, &policy->localCertificate);
#elif defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
    UA_OpenSSL_LoadLocalCertificate(&localCertificate, &policy->localCertificate);
#else
    UA_ByteString_copy(&localCertificate, &policy->localCertificate);
#endif

    policy->symmetricModule.generateKey = generateKey_none;
    policy->symmetricModule.generateNonce = generateNonce_none;

    UA_SecurityPolicySignatureAlgorithm *sym_signatureAlgorithm =
        &policy->symmetricModule.cryptoModule.signatureAlgorithm;
    sym_signatureAlgorithm->uri = UA_STRING_NULL;
    sym_signatureAlgorithm->verify = verify_none;
    sym_signatureAlgorithm->sign = sign_none;
    sym_signatureAlgorithm->getLocalSignatureSize = length_none;
    sym_signatureAlgorithm->getRemoteSignatureSize = length_none;
    sym_signatureAlgorithm->getLocalKeyLength = length_none;
    sym_signatureAlgorithm->getRemoteKeyLength = length_none;

    UA_SecurityPolicyEncryptionAlgorithm *sym_encryptionAlgorithm =
        &policy->symmetricModule.cryptoModule.encryptionAlgorithm;
    sym_encryptionAlgorithm->uri = UA_STRING_NULL;
    sym_encryptionAlgorithm->encrypt = encrypt_none;
    sym_encryptionAlgorithm->decrypt = decrypt_none;
    sym_encryptionAlgorithm->getLocalKeyLength = length_none;
    sym_encryptionAlgorithm->getRemoteKeyLength = length_none;
    sym_encryptionAlgorithm->getRemoteBlockSize = length_none;
    sym_encryptionAlgorithm->getRemotePlainTextBlockSize = length_none;
    policy->symmetricModule.secureChannelNonceLength = 0;

    policy->asymmetricModule.makeCertificateThumbprint = makeThumbprint_none;
    policy->asymmetricModule.compareCertificateThumbprint = compareThumbprint_none;

    // This only works for none since symmetric and asymmetric crypto modules do the same i.e. nothing
    policy->asymmetricModule.cryptoModule = policy->symmetricModule.cryptoModule;

    // Use the same signing algorithm as for asymmetric signing
    policy->certificateSigningAlgorithm = policy->asymmetricModule.cryptoModule.signatureAlgorithm;

    policy->channelModule.newContext = newContext_none;
    policy->channelModule.deleteContext = deleteContext_none;
    policy->channelModule.setLocalSymEncryptingKey = setContextValue_none;
    policy->channelModule.setLocalSymSigningKey = setContextValue_none;
    policy->channelModule.setLocalSymIv = setContextValue_none;
    policy->channelModule.setRemoteSymEncryptingKey = setContextValue_none;
    policy->channelModule.setRemoteSymSigningKey = setContextValue_none;
    policy->channelModule.setRemoteSymIv = setContextValue_none;
    policy->channelModule.compareCertificate = compareCertificate_none;
    policy->updateCertificateAndPrivateKey = updateCertificateAndPrivateKey_none;
    policy->clear = policy_clear_none;

    return UA_STATUSCODE_GOOD;
}
