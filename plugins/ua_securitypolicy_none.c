/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include "ua_types.h"
#include "ua_securitypolicy_none.h"
#include "ua_types_generated_handling.h"

static UA_StatusCode
verify_none(const UA_SecurityPolicy *securityPolicy,
            const void *channelContext,
            const UA_ByteString *message,
            const UA_ByteString *signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sign_none(const UA_SecurityPolicy *securityPolicy,
          const void *channelContext,
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
             const void *channelContext,
             UA_ByteString *data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
decrypt_none(const UA_SecurityPolicy *securityPolicy,
             const void *channelContext,
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

static UA_StatusCode
generateNonce_none(const UA_SecurityPolicy *securityPolicy,
                   UA_ByteString *out) {
    if(securityPolicy == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(out->length != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(out->data != NULL)
        UA_ByteString_deleteMembers(out);

    out->data = (UA_Byte *) UA_EMPTY_ARRAY_SENTINEL;
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

static size_t
getRemoteAsymPlainTextBlockSize_none(const void *channelContext) {
    return 0;
}

static size_t
getRemoteAsymEncryptionBufferLengthOverhead_none(const void *channelContext,
                                                 size_t maxEncryptionLength) {
    return 0;
}

static UA_StatusCode
compareCertificate_none(const void *channelContext,
                        const UA_ByteString *certificate) {
    return UA_STATUSCODE_GOOD;
}

static void
policy_deletemembers_none(UA_SecurityPolicy *policy) {
    UA_ByteString_deleteMembers(&policy->localCertificate);
}

UA_StatusCode
UA_SecurityPolicy_None(UA_SecurityPolicy *policy, const UA_ByteString localCertificate,
                       UA_Logger logger) {
    policy->policyContext = (void *) (uintptr_t) logger;
    policy->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    policy->logger = logger;
    UA_ByteString_copy(&localCertificate, &policy->localCertificate);

    policy->asymmetricModule.makeCertificateThumbprint = makeThumbprint_none;
    policy->asymmetricModule.compareCertificateThumbprint = compareThumbprint_none;
    policy->asymmetricModule.cryptoModule.signatureAlgorithmUri = UA_STRING_NULL;
    policy->asymmetricModule.cryptoModule.verify = verify_none;
    policy->asymmetricModule.cryptoModule.sign = sign_none;
    policy->asymmetricModule.cryptoModule.getLocalSignatureSize = length_none;
    policy->asymmetricModule.cryptoModule.getRemoteSignatureSize = length_none;
    policy->asymmetricModule.cryptoModule.encrypt = encrypt_none;
    policy->asymmetricModule.cryptoModule.decrypt = decrypt_none;
    policy->asymmetricModule.cryptoModule.getLocalEncryptionKeyLength = length_none;
    policy->asymmetricModule.cryptoModule.getRemoteEncryptionKeyLength = length_none;

    policy->symmetricModule.generateKey = generateKey_none;
    policy->symmetricModule.generateNonce = generateNonce_none;
    policy->symmetricModule.cryptoModule.signatureAlgorithmUri = UA_STRING_NULL;
    policy->symmetricModule.cryptoModule.verify = verify_none;
    policy->symmetricModule.cryptoModule.sign = sign_none;
    policy->symmetricModule.cryptoModule.getLocalSignatureSize = length_none;
    policy->symmetricModule.cryptoModule.getRemoteSignatureSize = length_none;
    policy->symmetricModule.cryptoModule.encrypt = encrypt_none;
    policy->symmetricModule.cryptoModule.decrypt = decrypt_none;
    policy->symmetricModule.cryptoModule.getLocalEncryptionKeyLength = length_none;
    policy->symmetricModule.cryptoModule.getRemoteEncryptionKeyLength = length_none;
    policy->symmetricModule.encryptionBlockSize = 0;
    policy->symmetricModule.signingKeyLength = 0;

    policy->channelModule.newContext = newContext_none;
    policy->channelModule.deleteContext = deleteContext_none;
    policy->channelModule.setLocalSymEncryptingKey = setContextValue_none;
    policy->channelModule.setLocalSymSigningKey = setContextValue_none;
    policy->channelModule.setLocalSymIv = setContextValue_none;
    policy->channelModule.setRemoteSymEncryptingKey = setContextValue_none;
    policy->channelModule.setRemoteSymSigningKey = setContextValue_none;
    policy->channelModule.setRemoteSymIv = setContextValue_none;
    policy->channelModule.compareCertificate = compareCertificate_none;
    policy->channelModule.getRemoteAsymPlainTextBlockSize = getRemoteAsymPlainTextBlockSize_none;
    policy->channelModule.getRemoteAsymEncryptionBufferLengthOverhead =
        getRemoteAsymEncryptionBufferLengthOverhead_none;
    policy->deleteMembers = policy_deletemembers_none;

    return UA_STATUSCODE_GOOD;
}
