/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2017-2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include <open62541/plugin/securitypolicy_default.h>

static UA_StatusCode
verify_none(const UA_SecurityPolicy *securityPolicy,
            void *channelContext,
            const UA_ByteString *message,
            const UA_ByteString *signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sign_none(const UA_SecurityPolicy *securityPolicy,
          void *channelContext,
          const UA_ByteString *message,
          UA_ByteString *signature) {
    return UA_STATUSCODE_GOOD;
}

static size_t
length_none(const UA_SecurityPolicy *securityPolicy,
            const void *channelContext) {
    return 0;
}

static UA_StatusCode
encrypt_none(const UA_SecurityPolicy *securityPolicy,
             void *channelContext,
             UA_ByteString *data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
decrypt_none(const UA_SecurityPolicy *securityPolicy,
             void *channelContext,
             UA_ByteString *data) {
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
generateKey_none(const UA_SecurityPolicy *securityPolicy,
                 const UA_ByteString *secret,
                 const UA_ByteString *seed,
                 UA_ByteString *out) {
    return UA_STATUSCODE_GOOD;
}

/* Use the non-cryptographic RNG to set the nonce */
static UA_StatusCode
generateNonce_none(const UA_SecurityPolicy *securityPolicy, UA_ByteString *out) {
    if(securityPolicy == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(out->length == 0)
        return UA_STATUSCODE_GOOD;

    /* Fill blocks of four byte */
    size_t i = 0;
    while(i + 3 < out->length) {
        UA_UInt32 rand = UA_UInt32_random();
        memcpy(&out->data[i], &rand, 4);
        i = i+4;
    }

    /* Fill the remaining byte */
    UA_UInt32 rand = UA_UInt32_random();
    memcpy(&out->data[i], &rand, out->length % 4);

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
    UA_ByteString_deleteMembers(&policy->localCertificate);
    UA_ByteString_copy(&newCertificate, &policy->localCertificate);
    return UA_STATUSCODE_GOOD;
}


static void
policy_deletemembers_none(UA_SecurityPolicy *policy) {
    UA_ByteString_deleteMembers(&policy->localCertificate);
}

UA_StatusCode
UA_SecurityPolicy_None(UA_SecurityPolicy *policy,
                       UA_CertificateVerification *certificateVerification,
                       const UA_ByteString localCertificate, const UA_Logger *logger) {
    policy->policyContext = (void *)(uintptr_t)logger;
    policy->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    policy->logger = logger;
    UA_ByteString_copy(&localCertificate, &policy->localCertificate);

    policy->certificateVerification = certificateVerification;

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
    sym_encryptionAlgorithm->encrypt = encrypt_none;
    sym_encryptionAlgorithm->decrypt = decrypt_none;
    sym_encryptionAlgorithm->getLocalKeyLength = length_none;
    sym_encryptionAlgorithm->getRemoteKeyLength = length_none;
    sym_encryptionAlgorithm->getLocalBlockSize = length_none;
    sym_encryptionAlgorithm->getRemoteBlockSize = length_none;
    sym_encryptionAlgorithm->getLocalPlainTextBlockSize = length_none;
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
    policy->deleteMembers = policy_deletemembers_none;

    return UA_STATUSCODE_GOOD;
}
