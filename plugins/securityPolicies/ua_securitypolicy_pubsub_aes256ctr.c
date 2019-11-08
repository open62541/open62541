#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/plugin/securitypolicy_mbedtls_common.h>
#include <open62541/util.h>

#ifdef UA_ENABLE_ENCRYPTION

#include <mbedtls/aes.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/entropy_poll.h>
#include <mbedtls/error.h>
#include <mbedtls/md.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/version.h>
#include <mbedtls/x509_crt.h>

/* Orinigal Notes:
 * mbedTLS' AES allows in-place encryption and decryption. Sow we don't have to
 * allocate temp buffers.
 * https://tls.mbed.org/discussions/generic/in-place-decryption-with-aes256-same-input-output-buffer
 */

//#define UA_SECURITYPOLICY_PUBSUB_AES256CTR_RSAPADDING_LEN 42
//#define UA_SHA1_LENGTH 20
#define UA_SHA256_LENGTH 32
#define UA_SECURITYPOLICY_PUBSUB_AES256CTR_SYM_SIGNING_KEY_LENGTH 32
#define UA_SECURITYPOLICY_PUBSUB_AES256CTR_SYM_KEY_LENGTH 32
#define UA_SECURITYPOLICY_PUBSUB_AES256CTR_KEYNONCE_LENGTH 4
#define UA_SECURITYPOLICY_PUBSUB_AES256CTR_SYM_ENCRYPTION_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_PUBSUB_AES256CTR_SYM_PLAIN_TEXT_BLOCK_SIZE 16
//#define UA_SECURITYPOLICY_PUBSUB_AES256CTR_MINASYMKEYLENGTH 256
//#define UA_SECURITYPOLICY_PUBSUB_AES256CTR_MAXASYMKEYLENGTH 512
// counter block=keynonce(4Byte)+Messagenonce(8Byte)+counter(4Byte) see Part14 7.2.2.2.3.2
// for details
#define UA_SECURITYPOLICY_PUBSUB_AES256CTR_COUNTERBLOCK_SIZE 16

typedef struct {
    const UA_SecurityPolicy *securityPolicy;
    UA_ByteString localCertThumbprint;

    mbedtls_ctr_drbg_context drbgContext;
    mbedtls_entropy_context entropyContext;
    mbedtls_md_context_t sha256MdContext;
    mbedtls_pk_context localPrivateKey;
} PUBSUB_AES256CTR_PolicyContext;

typedef struct {
    PUBSUB_AES256CTR_PolicyContext *policyContext;

    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;
    /* now use localSymIV as counterBlock to keep consistent  with the policy function
     "setLocalSymIv"*/
    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    mbedtls_x509_crt remoteCertificate;
} PUBSUB_AES256CTR_ChannelContext;

/********************/
/* AsymmetricModule */
/********************/
static UA_StatusCode
asym_verify_sp_pubsub_none_256ctr(const UA_SecurityPolicy *securityPolicy,
                           PUBSUB_AES256CTR_ChannelContext *cc,
                           const UA_ByteString *message, const UA_ByteString *signature) {

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_sign_sp_pubsub_none_256ctr(const UA_SecurityPolicy *securityPolicy,
                         PUBSUB_AES256CTR_ChannelContext *cc,
                         const UA_ByteString *message, UA_ByteString *signature) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_encrypt_sp_pubsub_none_256ctr(const UA_SecurityPolicy *securityPolicy,
                            PUBSUB_AES256CTR_ChannelContext *cc, UA_ByteString *data) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_decrypt_sp_pubsub_none_256ctr(const UA_SecurityPolicy *securityPolicy,
                            PUBSUB_AES256CTR_ChannelContext *cc, UA_ByteString *data) {
    return UA_STATUSCODE_GOOD;
}

/*Get Size place holder not used */
/*
static size_t
asym_getRemoteEncryptionKeyLength_sp_pubsub_length_none(
    const UA_SecurityPolicy *securityPolicy, const PUBSUB_AES256CTR_ChannelContext *cc) {
    return 0;
}
static size_t
asym_getRemoteBlockSize_sp_pubsub_length_none(const UA_SecurityPolicy *securityPolicy,
                                              const PUBSUB_AES256CTR_ChannelContext *cc) {
    return 0;
}

static size_t
asym_getRemotePlainTextBlockSize_sp_pubsub_length_none(
    const UA_SecurityPolicy *securityPolicy, const PUBSUB_AES256CTR_ChannelContext *cc) {
    return 0;
}
static size_t
asym_getLocalSignatureSize_sp_pubsub_length_none(
    const UA_SecurityPolicy *securityPolicy, const PUBSUB_AES256CTR_ChannelContext *cc) {

    return 0;
}

static size_t
asym_getRemoteSignatureSize_sp_pubsub_length_none(
    const UA_SecurityPolicy *securityPolicy, const PUBSUB_AES256CTR_ChannelContext *cc) {
    return 0;
}
*/

static UA_StatusCode
asym_makeThumbprint_sp_pubsub_none_256ctr(const UA_SecurityPolicy *securityPolicy,
                                   const UA_ByteString *certificate,
                                   UA_ByteString *thumbprint) {
    return UA_STATUSCODE_GOOD;
}
static size_t
asym_sp_pubsub_length_none_256ctr(const UA_SecurityPolicy *securityPolicy,
                           const PUBSUB_AES256CTR_ChannelContext *cc) {
    return 0;
}

static UA_StatusCode
asymmetricModule_compareCertificateThumbprint_sp_pubsub_none_256ctr(
    const UA_SecurityPolicy *securityPolicy, const UA_ByteString *certificateThumbprint) {

    return UA_STATUSCODE_GOOD;
}

/*******************/
/* SymmetricModule */
/*******************/

/*Signature and verify all using HMAC-SHA2-256, nothing to change*/
static UA_StatusCode
sym_verify_sp_pubsub_aes256ctr(const UA_SecurityPolicy *securityPolicy,
                               PUBSUB_AES256CTR_ChannelContext *cc,
                               const UA_ByteString *message,
                               const UA_ByteString *signature) {
    if(securityPolicy == NULL || cc == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Compute MAC */
    if(signature->length != UA_SHA256_LENGTH) {
        UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Signature size does not have the desired size defined by the "
                     "security policy");
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    PUBSUB_AES256CTR_PolicyContext *pc =
        (PUBSUB_AES256CTR_PolicyContext *)securityPolicy->policyContext;

    unsigned char mac[UA_SHA256_LENGTH];
    mbedtls_hmac(&pc->sha256MdContext, &cc->localSymSigningKey, message, mac);

    /* Compare with Signature */
    if(!UA_constantTimeEqual(signature->data, mac, UA_SHA256_LENGTH))
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_sign_sp_pubsub_aes256ctr(const UA_SecurityPolicy *securityPolicy,
                             const PUBSUB_AES256CTR_ChannelContext *cc,
                             const UA_ByteString *message, UA_ByteString *signature) {
    if(signature->length != UA_SHA256_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_hmac(&cc->policyContext->sha256MdContext, &cc->localSymSigningKey, message,
                 signature->data);
    return UA_STATUSCODE_GOOD;
}

static size_t
sym_getSignatureSize_sp_pubsub_aes256ctr(const UA_SecurityPolicy *securityPolicy,
                                         const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static size_t
sym_getSigningKeyLength_sp_pubsub_aes256ctr(const UA_SecurityPolicy *const securityPolicy,
                                            const void *const channelContext) {
    return UA_SECURITYPOLICY_PUBSUB_AES256CTR_SYM_SIGNING_KEY_LENGTH;
}

static size_t
sym_getEncryptionKeyLength_sp_pubsub_aes256ctr(const UA_SecurityPolicy *securityPolicy,
                                               const void *channelContext) {
    return UA_SECURITYPOLICY_PUBSUB_AES256CTR_SYM_KEY_LENGTH;
}

static size_t
sym_getEncryptionBlockSize_sp_pubsub_aes256ctr(
    const UA_SecurityPolicy *const securityPolicy, const void *const channelContext) {
    return UA_SECURITYPOLICY_PUBSUB_AES256CTR_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
sym_getPlainTextBlockSize_sp_pubsub_aes256ctr(
    const UA_SecurityPolicy *const securityPolicy, const void *const channelContext) {
    return UA_SECURITYPOLICY_PUBSUB_AES256CTR_SYM_PLAIN_TEXT_BLOCK_SIZE;
}

static UA_StatusCode
sym_encrypt_sp_pubsub_aes256ctr(const UA_SecurityPolicy *securityPolicy,
                                const PUBSUB_AES256CTR_ChannelContext *cc,
                                UA_ByteString *data) {
    if(securityPolicy == NULL || cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_LOG_INFO(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                "StartEncryption/DecryptionModule");

    if(cc->localSymIv.length !=
       securityPolicy->symmetricModule.cryptoModule.encryptionAlgorithm.getLocalBlockSize(
           securityPolicy, cc))
        return UA_STATUSCODE_BADINTERNALERROR;

    /*Tested, CTR mode does not need padding */
    /* Keylength in bits */
    unsigned int keylength = (unsigned int)(cc->localSymEncryptingKey.length * 8);
    mbedtls_aes_context aesContext;
    int mbedErr =
        mbedtls_aes_setkey_enc(&aesContext, cc->localSymEncryptingKey.data, keylength);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /*prepare buffer and counterBlock required for encryption/decryption*/
    UA_Byte
        counterBlockCopy[UA_SECURITYPOLICY_PUBSUB_AES256CTR_SYM_ENCRYPTION_BLOCK_SIZE];
    UA_Byte aesBuffer[UA_SECURITYPOLICY_PUBSUB_AES256CTR_SYM_ENCRYPTION_BLOCK_SIZE];
    memcpy(counterBlockCopy, cc->localSymIv.data,
           UA_SECURITYPOLICY_PUBSUB_AES256CTR_SYM_ENCRYPTION_BLOCK_SIZE);

    size_t counterblockoffset = 0;

    mbedErr = mbedtls_aes_crypt_ctr(&aesContext, data->length, &counterblockoffset,
                                    counterBlockCopy, aesBuffer, data->data, data->data);

    if(mbedErr)
        retval = UA_STATUSCODE_BADINTERNALERROR;
    return retval;
}

/* a decryption function is exactly the same as an encryption one, since they all do XOR
 * operations*/
static UA_StatusCode
sym_decrypt_sp_pubsub_aes256ctr(const UA_SecurityPolicy *securityPolicy,
                                const PUBSUB_AES256CTR_ChannelContext *cc,
                                UA_ByteString *data) {
    return sym_encrypt_sp_pubsub_aes256ctr(securityPolicy, cc, data);
}

/*Tested, meeting  Profile*/
static UA_StatusCode
sym_generateKey_sp_pubsub_aes256ctr(const UA_SecurityPolicy *securityPolicy,
                                    const UA_ByteString *secret,
                                    const UA_ByteString *seed, UA_ByteString *out) {
    if(securityPolicy == NULL || secret == NULL || seed == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    PUBSUB_AES256CTR_PolicyContext *pc =
        (PUBSUB_AES256CTR_PolicyContext *)securityPolicy->policyContext;

    return mbedtls_generateKey(&pc->sha256MdContext, secret, seed, out);
}
/* This nonce does not to be  a
cryptographically random number, it can be pseudo-random */
static UA_StatusCode
sym_generateNonce_sp_pubsub_aes256ctr(const UA_SecurityPolicy *securityPolicy,
                                      UA_ByteString *out) {
    if(securityPolicy == NULL || securityPolicy->policyContext == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    PUBSUB_AES256CTR_PolicyContext *pc =
        (PUBSUB_AES256CTR_PolicyContext *)securityPolicy->policyContext;
    int mbedErr = mbedtls_ctr_drbg_random(&pc->drbgContext, out->data, out->length);
    if(mbedErr)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

/*****************/
/* ChannelModule */
/*****************/

/* Assumes that the certificate has been verified externally */
static UA_StatusCode
parseRemoteCertificate_sp_pubsub_none_256ctr(PUBSUB_AES256CTR_ChannelContext *cc,
                                      const UA_ByteString *remoteCertificate) {

    return UA_STATUSCODE_GOOD;
}

static void
channelContext_deleteContext_sp_pubsub_aes256ctr(PUBSUB_AES256CTR_ChannelContext *cc) {

    UA_ByteString_deleteMembers(&cc->localSymSigningKey);
    UA_ByteString_deleteMembers(&cc->localSymEncryptingKey);
    UA_ByteString_deleteMembers(&cc->localSymIv);

    UA_ByteString_deleteMembers(&cc->remoteSymSigningKey);
    UA_ByteString_deleteMembers(&cc->remoteSymEncryptingKey);
    UA_ByteString_deleteMembers(&cc->remoteSymIv);

    mbedtls_x509_crt_free(&cc->remoteCertificate);

    UA_free(cc);
}

static UA_StatusCode
channelContext_newContext_sp_pubsub_aes256ctr(const UA_SecurityPolicy *securityPolicy,
                                              const UA_ByteString *remoteCertificate,
                                              void **pp_contextData) {
    // if(securityPolicy == NULL || remoteCertificate == NULL || pp_contextData ==
    // NULL)
    //     return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the channel context */
    *pp_contextData = UA_malloc(sizeof(PUBSUB_AES256CTR_ChannelContext));
    if(*pp_contextData == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    PUBSUB_AES256CTR_ChannelContext *cc =
        (PUBSUB_AES256CTR_ChannelContext *)*pp_contextData;

    /* Initialize the channel context */
    cc->policyContext = (PUBSUB_AES256CTR_PolicyContext *)securityPolicy->policyContext;

    UA_ByteString_init(&cc->localSymSigningKey);
    UA_ByteString_init(&cc->localSymEncryptingKey);
    UA_ByteString_init(&cc->localSymIv);

    UA_ByteString_init(&cc->remoteSymSigningKey);
    UA_ByteString_init(&cc->remoteSymEncryptingKey);
    UA_ByteString_init(&cc->remoteSymIv);

    mbedtls_x509_crt_init(&cc->remoteCertificate);

    UA_StatusCode retval = parseRemoteCertificate_sp_pubsub_none_256ctr(cc, remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        channelContext_deleteContext_sp_pubsub_aes256ctr(cc);
        *pp_contextData = NULL;
    }
    return retval;
}

static UA_StatusCode
channelContext_setLocalSymEncryptingKey_sp_pubsub_aes256ctr(
    PUBSUB_AES256CTR_ChannelContext *cc, const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_deleteMembers(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

static UA_StatusCode
channelContext_setLocalSymSigningKey_sp_pubsub_aes256ctr(
    PUBSUB_AES256CTR_ChannelContext *cc, const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_deleteMembers(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}

static UA_StatusCode
channelContext_setLocalSymIv_sp_pubsub_aes256ctr(PUBSUB_AES256CTR_ChannelContext *cc,
                                                 const UA_ByteString *iv) {
    if(iv == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_deleteMembers(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

static UA_StatusCode
channelContext_setRemoteSymEncryptingKey_sp_pubsub_aes256ctr(
    PUBSUB_AES256CTR_ChannelContext *cc, const UA_ByteString *key) {
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

static UA_StatusCode
channelContext_setRemoteSymSigningKey_sp_pubsub_aes256ctr(
    PUBSUB_AES256CTR_ChannelContext *cc, const UA_ByteString *key) {
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

static UA_StatusCode
channelContext_setRemoteSymIv_sp_pubsub_aes256ctr(PUBSUB_AES256CTR_ChannelContext *cc,
                                                  const UA_ByteString *iv) {
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
} 

static UA_StatusCode
channelContext_compareCertificate_sp_pubsub_none_256ctr(
    const PUBSUB_AES256CTR_ChannelContext *cc, const UA_ByteString *certificate) {

    return UA_STATUSCODE_GOOD;
}

static void
deleteMembers_sp_pubsub_aes256ctr(UA_SecurityPolicy *securityPolicy) {
    if(securityPolicy == NULL)
        return;

    if(securityPolicy->policyContext == NULL)
        return;

    UA_ByteString_deleteMembers(&securityPolicy->localCertificate);

    /* delete all allocated members in the context */
    PUBSUB_AES256CTR_PolicyContext *pc =
        (PUBSUB_AES256CTR_PolicyContext *)securityPolicy->policyContext;

    mbedtls_ctr_drbg_free(&pc->drbgContext);
    mbedtls_entropy_free(&pc->entropyContext);

    mbedtls_md_free(&pc->sha256MdContext);
    UA_LOG_DEBUG(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Deleted members of EndpointContext for sp_PUBSUB_AES256CTR");

    mbedtls_pk_free(&pc->localPrivateKey);

    UA_free(pc);
    securityPolicy->policyContext = NULL;
}

static UA_StatusCode
updateCertificateAndPrivateKey_sp_pubsub_none_256ctr(UA_SecurityPolicy *securityPolicy,
                                              const UA_ByteString newCertificate,
                                              const UA_ByteString newPrivateKey) {

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
policyContext_newContext_sp_pubsub_aes256ctr(UA_SecurityPolicy *securityPolicy,
                                             const UA_ByteString localPrivateKey) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(securityPolicy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    PUBSUB_AES256CTR_PolicyContext *pc = (PUBSUB_AES256CTR_PolicyContext *)UA_malloc(
        sizeof(PUBSUB_AES256CTR_PolicyContext));
    securityPolicy->policyContext = (void *)pc;
    if(!pc) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    /* Initialize the PolicyContext */
    memset(pc, 0, sizeof(PUBSUB_AES256CTR_PolicyContext));
    mbedtls_ctr_drbg_init(&pc->drbgContext);
    mbedtls_entropy_init(&pc->entropyContext);
    mbedtls_pk_init(&pc->localPrivateKey);
    mbedtls_md_init(&pc->sha256MdContext);
    pc->securityPolicy = securityPolicy;

    /* Initialized the message digest */
    const mbedtls_md_info_t *const mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    int mbedErr = mbedtls_md_setup(&pc->sha256MdContext, mdInfo, MBEDTLS_MD_SHA256);
    if(mbedErr) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    /* Add the system entropy source */
    mbedErr =
        mbedtls_entropy_add_source(&pc->entropyContext, mbedtls_platform_entropy_poll,
                                   NULL, 0, MBEDTLS_ENTROPY_SOURCE_STRONG);
    if(mbedErr) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    /* Seed the RNG */
    char *personalization = "open62541-drbg";
    mbedErr =
        mbedtls_ctr_drbg_seed(&pc->drbgContext, mbedtls_entropy_func, &pc->entropyContext,
                              (const unsigned char *)personalization, 14);
    if(mbedErr) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    /* Set the private key */
    if(localPrivateKey.length) {
        mbedErr = mbedtls_pk_parse_key(&pc->localPrivateKey, localPrivateKey.data,
                                       localPrivateKey.length, NULL, 0);
        if(mbedErr) {
            retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
            goto error;
        }
    }

    return UA_STATUSCODE_GOOD;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not create securityContext");
    if(securityPolicy->policyContext != NULL)
        deleteMembers_sp_pubsub_aes256ctr(securityPolicy);
    return retval;
}

UA_StatusCode
UA_SecurityPolicy_Pubsub_Aes256ctr(UA_SecurityPolicy *policy,
                                   UA_CertificateVerification *certificateVerification,
                                   const UA_ByteString localCertificate,
                                   const UA_ByteString localPrivateKey,
                                   const UA_Logger *logger) {
    memset(policy, 0, sizeof(UA_SecurityPolicy));
    policy->logger = logger;

    policy->policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR");

    UA_SecurityPolicyAsymmetricModule *const asymmetricModule = &policy->asymmetricModule;
    UA_SecurityPolicySymmetricModule *const symmetricModule = &policy->symmetricModule;
    UA_SecurityPolicyChannelModule *const channelModule = &policy->channelModule;

    /* Copy the certificate and add a NULL to the end */
    UA_StatusCode retval =
        UA_ByteString_allocBuffer(&policy->localCertificate, localCertificate.length + 1);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    memcpy(policy->localCertificate.data, localCertificate.data, localCertificate.length);
    policy->localCertificate.data[localCertificate.length] = '\0';
    policy->localCertificate.length--;
    policy->certificateVerification = certificateVerification;

    /* AsymmetricModule */
    UA_SecurityPolicySignatureAlgorithm *asym_signatureAlgorithm =
        &asymmetricModule->cryptoModule.signatureAlgorithm;
    asym_signatureAlgorithm->uri = UA_STRING_NULL;
    asym_signatureAlgorithm->verify =
        (UA_StatusCode(*)(const UA_SecurityPolicy *, void *, const UA_ByteString *,
                          const UA_ByteString *))asym_verify_sp_pubsub_none_256ctr;
    asym_signatureAlgorithm->sign =
        (UA_StatusCode(*)(const UA_SecurityPolicy *, void *, const UA_ByteString *,
                          UA_ByteString *))asym_sign_sp_pubsub_none_256ctr;
    asym_signatureAlgorithm->getLocalSignatureSize =
        (size_t(*)(const UA_SecurityPolicy *, const void *))asym_sp_pubsub_length_none_256ctr;
    asym_signatureAlgorithm->getRemoteSignatureSize =
        (size_t(*)(const UA_SecurityPolicy *, const void *))asym_sp_pubsub_length_none_256ctr;
    asym_signatureAlgorithm->getLocalKeyLength =
        (size_t(*)(const UA_SecurityPolicy *,
                   const void *))asym_sp_pubsub_length_none_256ctr;  /* TODO: Write function */
    asym_signatureAlgorithm->getRemoteKeyLength =
        (size_t(*)(const UA_SecurityPolicy *,
                   const void *))asym_sp_pubsub_length_none_256ctr;  /* TODO: Write function */

    UA_SecurityPolicyEncryptionAlgorithm *asym_encryptionAlgorithm =
        &asymmetricModule->cryptoModule.encryptionAlgorithm;
    asym_encryptionAlgorithm->uri = UA_STRING_NULL;
    asym_encryptionAlgorithm->encrypt = (UA_StatusCode(*)(
        const UA_SecurityPolicy *, void *, UA_ByteString *))asym_encrypt_sp_pubsub_none_256ctr;
    asym_encryptionAlgorithm->decrypt = (UA_StatusCode(*)(
        const UA_SecurityPolicy *, void *, UA_ByteString *))asym_decrypt_sp_pubsub_none_256ctr;
    asym_encryptionAlgorithm->getLocalKeyLength =
        (size_t(*)(const UA_SecurityPolicy *,
                   const void *))asym_sp_pubsub_length_none_256ctr;  /* TODO: Write function */
    asym_encryptionAlgorithm->getRemoteKeyLength =
        (size_t(*)(const UA_SecurityPolicy *, const void *))asym_sp_pubsub_length_none_256ctr;
    asym_encryptionAlgorithm->getLocalBlockSize =
        (size_t(*)(const UA_SecurityPolicy *,
                   const void *))asym_sp_pubsub_length_none_256ctr;  /* TODO: Write function */
    asym_encryptionAlgorithm->getRemoteBlockSize =
        (size_t(*)(const UA_SecurityPolicy *, const void *))asym_sp_pubsub_length_none_256ctr;
    asym_encryptionAlgorithm->getLocalPlainTextBlockSize =
        (size_t(*)(const UA_SecurityPolicy *,
                   const void *))asym_sp_pubsub_length_none_256ctr;  /* TODO: Write function */
    asym_encryptionAlgorithm->getRemotePlainTextBlockSize =
        (size_t(*)(const UA_SecurityPolicy *, const void *))asym_sp_pubsub_length_none_256ctr;
    asymmetricModule->makeCertificateThumbprint = asym_makeThumbprint_sp_pubsub_none_256ctr;
    asymmetricModule->compareCertificateThumbprint =
        asymmetricModule_compareCertificateThumbprint_sp_pubsub_none_256ctr;

    /* SymmetricModule */
    symmetricModule->generateKey = sym_generateKey_sp_pubsub_aes256ctr;
    symmetricModule->generateNonce = sym_generateNonce_sp_pubsub_aes256ctr;

    UA_SecurityPolicySignatureAlgorithm *sym_signatureAlgorithm =
        &symmetricModule->cryptoModule.signatureAlgorithm;
    sym_signatureAlgorithm->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#sha256");
    sym_signatureAlgorithm->verify =
        (UA_StatusCode(*)(const UA_SecurityPolicy *, void *, const UA_ByteString *,
                          const UA_ByteString *))sym_verify_sp_pubsub_aes256ctr;
    sym_signatureAlgorithm->sign =
        (UA_StatusCode(*)(const UA_SecurityPolicy *, void *, const UA_ByteString *,
                          UA_ByteString *))sym_sign_sp_pubsub_aes256ctr;
    sym_signatureAlgorithm->getLocalSignatureSize =
        sym_getSignatureSize_sp_pubsub_aes256ctr;
    sym_signatureAlgorithm->getRemoteSignatureSize =
        sym_getSignatureSize_sp_pubsub_aes256ctr;
    sym_signatureAlgorithm->getLocalKeyLength =
        (size_t(*)(const UA_SecurityPolicy *,
                   const void *))sym_getSigningKeyLength_sp_pubsub_aes256ctr;
    sym_signatureAlgorithm->getRemoteKeyLength =
        (size_t(*)(const UA_SecurityPolicy *,
                   const void *))sym_getSigningKeyLength_sp_pubsub_aes256ctr;

    UA_SecurityPolicyEncryptionAlgorithm *sym_encryptionAlgorithm =
        &symmetricModule->cryptoModule.encryptionAlgorithm;
    sym_encryptionAlgorithm->uri =
        UA_STRING("https://tools.ietf.org/html/rfc3686");  /* temp solution */
    sym_encryptionAlgorithm->encrypt =
        (UA_StatusCode(*)(const UA_SecurityPolicy *, void *,
                          UA_ByteString *))sym_encrypt_sp_pubsub_aes256ctr;
    sym_encryptionAlgorithm->decrypt =
        (UA_StatusCode(*)(const UA_SecurityPolicy *, void *,
                          UA_ByteString *))sym_decrypt_sp_pubsub_aes256ctr;
    sym_encryptionAlgorithm->getLocalKeyLength =
        sym_getEncryptionKeyLength_sp_pubsub_aes256ctr;
    sym_encryptionAlgorithm->getRemoteKeyLength =
        sym_getEncryptionKeyLength_sp_pubsub_aes256ctr;
    sym_encryptionAlgorithm->getLocalBlockSize =
        (size_t(*)(const UA_SecurityPolicy *,
                   const void *))sym_getEncryptionBlockSize_sp_pubsub_aes256ctr;
    sym_encryptionAlgorithm->getRemoteBlockSize =
        (size_t(*)(const UA_SecurityPolicy *,
                   const void *))sym_getEncryptionBlockSize_sp_pubsub_aes256ctr;
    sym_encryptionAlgorithm->getLocalPlainTextBlockSize =
        (size_t(*)(const UA_SecurityPolicy *,
                   const void *))sym_getPlainTextBlockSize_sp_pubsub_aes256ctr;
    sym_encryptionAlgorithm->getRemotePlainTextBlockSize =
        (size_t(*)(const UA_SecurityPolicy *,
                   const void *))sym_getPlainTextBlockSize_sp_pubsub_aes256ctr;
    symmetricModule->secureChannelNonceLength =
        UA_SECURITYPOLICY_PUBSUB_AES256CTR_SYM_SIGNING_KEY_LENGTH +
        UA_SECURITYPOLICY_PUBSUB_AES256CTR_SYM_KEY_LENGTH +
        UA_SECURITYPOLICY_PUBSUB_AES256CTR_KEYNONCE_LENGTH;

    /* Use the same signature algorithm as the asymmetric component for certificate
     signing (see standard) */
    policy->certificateSigningAlgorithm =
        policy->asymmetricModule.cryptoModule.signatureAlgorithm;

    /* ChannelModule */
    channelModule->newContext = channelContext_newContext_sp_pubsub_aes256ctr;
    channelModule->deleteContext =
        (void (*)(void *))channelContext_deleteContext_sp_pubsub_aes256ctr;

    channelModule->setLocalSymEncryptingKey =
        (UA_StatusCode(*)(void *, const UA_ByteString *))
            channelContext_setLocalSymEncryptingKey_sp_pubsub_aes256ctr;
    channelModule->setLocalSymSigningKey =
        (UA_StatusCode(*)(void *, const UA_ByteString *))
            channelContext_setLocalSymSigningKey_sp_pubsub_aes256ctr;
    channelModule->setLocalSymIv = (UA_StatusCode(*)(
        void *, const UA_ByteString *))channelContext_setLocalSymIv_sp_pubsub_aes256ctr;

    channelModule->setRemoteSymEncryptingKey =
        (UA_StatusCode(*)(void *, const UA_ByteString *))
            channelContext_setRemoteSymEncryptingKey_sp_pubsub_aes256ctr;
    channelModule->setRemoteSymSigningKey =
        (UA_StatusCode(*)(void *, const UA_ByteString *))
            channelContext_setRemoteSymSigningKey_sp_pubsub_aes256ctr;
    channelModule->setRemoteSymIv = (UA_StatusCode(*)(
        void *, const UA_ByteString *))channelContext_setRemoteSymIv_sp_pubsub_aes256ctr;

    channelModule->compareCertificate =
        (UA_StatusCode(*)(const void *, const UA_ByteString *))
            channelContext_compareCertificate_sp_pubsub_none_256ctr;

    policy->updateCertificateAndPrivateKey =
        updateCertificateAndPrivateKey_sp_pubsub_none_256ctr;

    policy->clear = deleteMembers_sp_pubsub_aes256ctr;

    return policyContext_newContext_sp_pubsub_aes256ctr(policy, localPrivateKey);
}

#endif
