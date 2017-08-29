/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include "ua_securitypolicy_none.h"
#include "ua_types_generated_handling.h"

/*******************************/
/* Asymmetric module functions */
/*******************************/

static UA_StatusCode
asym_verify_sp_none(const UA_SecurityPolicy *const securityPolicy,
                    const void *const channelContext,
                    const UA_ByteString *const message,
                    const UA_ByteString *const signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_sign_sp_none(const UA_SecurityPolicy *const securityPolicy,
                  const void *const channelContext,
                  const UA_ByteString *const message,
                  UA_ByteString *const signature) {
    return UA_STATUSCODE_GOOD;
}

static size_t
asym_getLocalSignatureSize_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                   const void *const channelContext) {
    return 0;
}

static size_t
asym_getRemoteSignatureSize_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                    const void *const channelContext) {
    return 0;
}

static UA_StatusCode
asym_encrypt_sp_none(const UA_SecurityPolicy *const securityPolicy,
                     const void *const channelContext,
                     UA_ByteString *const data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_decrypt_sp_none(const UA_SecurityPolicy *const securityPolicy,
                     const void *const channelContext,
                     UA_ByteString *const data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_makeThumbprint_sp_none(const UA_SecurityPolicy *const securityPolicy,
                            const UA_ByteString *const certificate,
                            UA_ByteString *const thumbprint) {
    if(certificate == NULL || thumbprint == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

/* Only a stub function for now */
static UA_StatusCode
asym_compareThumbprint_sp_none(const UA_SecurityPolicy *securityPolicy,
                               const UA_ByteString *certificateThumbprint) {
    if(!securityPolicy || !certificateThumbprint)
        return UA_STATUSCODE_BADINTERNALERROR;

    // The certificatethumbprint doesn't exist if the security policy is none
    if(certificateThumbprint->length != 0)
        return UA_STATUSCODE_BADCERTIFICATEINVALID;

    return UA_STATUSCODE_GOOD;
}

/******************************/
/* Symmetric module functions */
/******************************/

static UA_StatusCode
sym_verify_sp_none(const UA_SecurityPolicy *const securityPolicy,
                   const void *const channelContext,
                   const UA_ByteString *const message,
                   const UA_ByteString *const signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_sign_sp_none(const UA_SecurityPolicy *const securityPolicy,
                 const void *const channelContext,
                 const UA_ByteString *const message,
                 UA_ByteString *const signature) {
    return UA_STATUSCODE_GOOD;
}

static size_t
sym_getLocalSignatureSize_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                  const void *const channelContext) {
    return 0;
}

static size_t
sym_getRemoteSignatureSize_sp_none(const UA_SecurityPolicy *const securityPolicy,
                                   const void *const channelContext) {
    return 0;
}

static UA_StatusCode
sym_encrypt_sp_none(const UA_SecurityPolicy *const securityPolicy,
                    const void *const channelContext,
                    UA_ByteString *const data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_decrypt_sp_none(const UA_SecurityPolicy *const securityPolicy,
                    const void *const channelContext,
                    UA_ByteString *const data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_generateKey_sp_none(const UA_SecurityPolicy *const securityPolicy,
                        const UA_ByteString *const secret,
                        const UA_ByteString *const seed,
                        UA_ByteString *const out) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_generateNonce_sp_none(const UA_SecurityPolicy *const securityPolicy,
                          UA_ByteString *const out) {
    out->length = securityPolicy->symmetricModule.encryptingKeyLength;
    out->data[0] = 'a';
    return UA_STATUSCODE_GOOD;
}

/***************************/
/* ChannelModule functions */
/***************************/

// this is not really needed in security policy none because no context is required
// it is there to serve as a small example for policies that need context per channel
typedef struct {
    const UA_SecurityPolicy *securityPolicy;
    int callCounter;
} UA_SP_NONE_ChannelContextData;

static UA_StatusCode
channelContext_newContext_sp_none(const UA_SecurityPolicy *securityPolicy,
                                  const UA_ByteString *remoteCertificate,
                                  void **channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    *channelContext = UA_malloc(sizeof(UA_SP_NONE_ChannelContextData));
    if(*channelContext == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    // Initialize the channelcontext data here to sensible values
    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)*channelContext;

    data->callCounter = 0;
    data->securityPolicy = securityPolicy;

    return UA_STATUSCODE_GOOD;
}

static void
channelContext_deleteContext_sp_none(void *const channelContext) {
    if(channelContext != NULL)
        UA_free(channelContext);
}

static UA_StatusCode
channelContext_setLocalSymEncryptingKey_sp_none(void *const channelContext,
                                                const UA_ByteString *const key) {
    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)channelContext;
    data->callCounter++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setLocalSymSigningKey_sp_none(void *const channelContext,
                                             const UA_ByteString *const key) {
    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)channelContext;
    data->callCounter++;
    return UA_STATUSCODE_GOOD;
}


static UA_StatusCode
channelContext_setLocalSymIv_sp_none(void *const channelContext,
                                     const UA_ByteString *const iv) {
    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)channelContext;
    data->callCounter++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setRemoteSymEncryptingKey_sp_none(void *const channelContext,
                                                 const UA_ByteString *const key) {
    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)channelContext;
    data->callCounter++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setRemoteSymSigningKey_sp_none(void *const channelContext,
                                              const UA_ByteString *const key) {
    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)channelContext;
    data->callCounter++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setRemoteSymIv_sp_none(void *const channelContext,
                                      const UA_ByteString *const iv) {
    UA_SP_NONE_ChannelContextData* const data = (UA_SP_NONE_ChannelContextData*)channelContext;
    data->callCounter++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_compareCertificate_sp_none(const void *const channelContext,
                                          const UA_ByteString *const certificate) {
    if(channelContext == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_STATUSCODE_GOOD;
}

static size_t
channelContext_getRemoteAsymPlainTextBlockSize_sp_none(const void *const channelContext) {
    return 0;
}

static size_t
channelContext_getRemoteAsymEncryptionBufferLengthOverhead_sp_none(const void *const channelContext,
                                                                   const size_t maxEncryptionLength) {
    return 0;
}

/*****************************/
/* Security Policy functions */
/*****************************/

static void
policy_deletemembers_sp_none(UA_SecurityPolicy *policy) {
    UA_ByteString_deleteMembers(&policy->localCertificate);
}

UA_StatusCode
UA_SecurityPolicy_None(UA_SecurityPolicy *policy,
                       const UA_ByteString localCertificate,
                       UA_Logger logger) {
    policy->policyContext = (void*)(uintptr_t)logger;
    policy->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    UA_ByteString_copy(&localCertificate, &policy->localCertificate);

    policy->asymmetricModule.makeCertificateThumbprint = asym_makeThumbprint_sp_none;
    policy->asymmetricModule.compareCertificateThumbprint = asym_compareThumbprint_sp_none;
    policy->asymmetricModule.minAsymmetricKeyLength = 0;
    policy->asymmetricModule.maxAsymmetricKeyLength = 0;
    policy->asymmetricModule.thumbprintLength = 20;
    policy->asymmetricModule.encryptingModule.encrypt = asym_encrypt_sp_none;
    policy->asymmetricModule.encryptingModule.decrypt = asym_decrypt_sp_none;
    policy->asymmetricModule.signingModule.verify = asym_verify_sp_none;
    policy->asymmetricModule.signingModule.sign = asym_sign_sp_none;
    policy->asymmetricModule.signingModule.getLocalSignatureSize = asym_getLocalSignatureSize_sp_none;
    policy->asymmetricModule.signingModule.getRemoteSignatureSize = asym_getRemoteSignatureSize_sp_none;
    policy->asymmetricModule.signingModule.signatureAlgorithmUri = UA_STRING_NULL;

    policy->symmetricModule.generateKey = sym_generateKey_sp_none;
    policy->symmetricModule.generateNonce = sym_generateNonce_sp_none;
    policy->symmetricModule.encryptingModule.encrypt = sym_encrypt_sp_none;
    policy->symmetricModule.encryptingModule.decrypt = sym_decrypt_sp_none;
    policy->symmetricModule.signingModule.verify = sym_verify_sp_none;
    policy->symmetricModule.signingModule.sign = sym_sign_sp_none;
    policy->symmetricModule.signingModule.getLocalSignatureSize = sym_getLocalSignatureSize_sp_none;
    policy->symmetricModule.signingModule.getRemoteSignatureSize = sym_getRemoteSignatureSize_sp_none;
    policy->symmetricModule.signingModule.signatureAlgorithmUri = UA_STRING_NULL;
    policy->symmetricModule.signingKeyLength = 0;
    policy->symmetricModule.encryptingKeyLength = 1;
    policy->symmetricModule.encryptingBlockSize = 0;

    policy->channelModule.newContext = channelContext_newContext_sp_none;
    policy->channelModule.deleteContext = channelContext_deleteContext_sp_none;
    policy->channelModule.setLocalSymEncryptingKey = channelContext_setLocalSymEncryptingKey_sp_none;
    policy->channelModule.setLocalSymSigningKey = channelContext_setLocalSymSigningKey_sp_none;
    policy->channelModule.setLocalSymIv = channelContext_setLocalSymIv_sp_none;
    policy->channelModule.setRemoteSymEncryptingKey = channelContext_setRemoteSymEncryptingKey_sp_none;
    policy->channelModule.setRemoteSymSigningKey = channelContext_setRemoteSymSigningKey_sp_none;
    policy->channelModule.setRemoteSymIv = channelContext_setRemoteSymIv_sp_none;
    policy->channelModule.compareCertificate = channelContext_compareCertificate_sp_none;
    policy->channelModule.getRemoteAsymPlainTextBlockSize =
        channelContext_getRemoteAsymPlainTextBlockSize_sp_none;
    policy->channelModule.getRemoteAsymEncryptionBufferLengthOverhead =
        channelContext_getRemoteAsymEncryptionBufferLengthOverhead_sp_none;

    policy->deleteMembers = policy_deletemembers_sp_none;

    return UA_STATUSCODE_GOOD;
}
