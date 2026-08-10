/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2018 (c) Daniel Feist, Precitec GmbH & Co. KG
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2020 (c) Wind River Systems, Inc.
 *    Copyright 2020 (c) basysKom GmbH
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/securitypolicy.h>
#include <open62541/types.h>
#include <open62541/util.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS

#include <open62541/plugin/securitypolicy_default.h>
#include "securitypolicy_common.h"

#include <mbedtls/version.h>

/* Notes:
 * mbedTLS' AES allows in-place encryption and decryption. Sow we don't have to
 * allocate temp buffers.
 * https://tls.mbed.org/discussions/generic/in-place-decryption-with-aes256-same-input-output-buffer
 */

#define UA_SECURITYPOLICY_BASIC256SHA1_RSAPADDING_LEN 42
#define UA_SHA1_LENGTH 20
#define UA_BASIC256_SYM_SIGNING_KEY_LENGTH 24
#define UA_SECURITYPOLICY_BASIC256_SYM_KEY_LENGTH 32
#define UA_SECURITYPOLICY_BASIC256_SYM_ENCRYPTION_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_BASIC256_SYM_PLAIN_TEXT_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_BASIC256_MINASYMKEYLENGTH 128
#define UA_SECURITYPOLICY_BASIC256_MAXASYMKEYLENGTH 512

/********************/
/* AsymmetricModule */
/********************/

static UA_StatusCode
asym_verify_sp_basic256(const UA_SecurityPolicy *policy, void *channelContext,
                        const UA_ByteString *message, const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_ChannelContext *cc = (mbedtls_ChannelContext *)channelContext;
    return UA_mbedTLS_PsaAsymmetricVerify(&cc->remoteCertificate.pk,
        PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_1), PSA_ALG_SHA_1,
        message, signature);
}

/* AsymmetricSignatureAlgorithm_RSA-PKCS15-SHA2-256 */
static UA_StatusCode
asym_sign_sp_basic256(const UA_SecurityPolicy *policy,
                      void *channelContext, const UA_ByteString *message,
                      UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_PolicyContext *pc = (mbedtls_PolicyContext *)policy->policyContext;
    return UA_mbedTLS_PsaAsymmetricSign(&pc->localPrivateKey,
        PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_1), PSA_ALG_SHA_1,
        message, signature);
}

static size_t
asym_getRemotePlainTextBlockSize_sp_basic256(const UA_SecurityPolicy *policy,
                                             const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    const mbedtls_ChannelContext *cc = (const mbedtls_ChannelContext *)channelContext;
    return (mbedtls_pk_get_bitlen(&cc->remoteCertificate.pk) + 7) / 8 -
        UA_SECURITYPOLICY_BASIC256SHA1_RSAPADDING_LEN;
}

/* AsymmetricEncryptionAlgorithm_RSA-OAEP-SHA1 */
static UA_StatusCode
asym_encrypt_sp_basic256(const UA_SecurityPolicy *policy,
                         void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const mbedtls_ChannelContext *cc = (const mbedtls_ChannelContext *)channelContext;
    const size_t plainTextBlockSize =
        asym_getRemotePlainTextBlockSize_sp_basic256(policy, cc);
    return UA_mbedTLS_PsaAsymmetricEncrypt(&cc->remoteCertificate.pk,
        PSA_ALG_RSA_OAEP(PSA_ALG_SHA_1), plainTextBlockSize, data);
}

/* AsymmetricEncryptionAlgorithm_RSA-OAEP-SHA1 */
static UA_StatusCode
asym_decrypt_sp_basic256(const UA_SecurityPolicy *policy,
                         void *channelContext, UA_ByteString *data) {
    if(data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_PolicyContext *pc = (mbedtls_PolicyContext *)policy->policyContext;
    return UA_mbedTLS_PsaAsymmetricDecrypt(&pc->localPrivateKey,
                                            PSA_ALG_RSA_OAEP(PSA_ALG_SHA_1), data);
}

static size_t
asym_getLocalEncryptionKeyLength_sp_basic256(const UA_SecurityPolicy *policy,
                                             const void *channelContext) {
    mbedtls_PolicyContext *pc = (mbedtls_PolicyContext *)policy->policyContext;
    return mbedtls_pk_get_bitlen(&pc->localPrivateKey);
}

static size_t
asym_getRemoteEncryptionKeyLength_sp_basic256(const UA_SecurityPolicy *policy,
                                              const void *channelContext) {
    const mbedtls_ChannelContext *cc = (const mbedtls_ChannelContext *)channelContext;
    return mbedtls_pk_get_bitlen(&cc->remoteCertificate.pk);
}

static UA_StatusCode
asym_makeThumbprint_sp_basic256(const UA_SecurityPolicy *securityPolicy,
                                const UA_ByteString *certificate,
                                UA_ByteString *thumbprint) {
    if(securityPolicy == NULL || certificate == NULL || thumbprint == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_mbedTLS_thumbprintSha1(certificate, thumbprint);
}

static UA_StatusCode
sym_verify_sp_basic256(const UA_SecurityPolicy *policy, void *channelContext,
                       const UA_ByteString *message,
                       const UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Compute MAC */
    if(signature->length != UA_SHA1_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    mbedtls_ChannelContext *cc =
        (mbedtls_ChannelContext *)channelContext;

    return UA_mbedTLS_PsaMacVerify(cc->remoteSymSigningKeyPsa.id,
                                  PSA_ALG_HMAC(PSA_ALG_SHA_1),
                                  message, signature);
}

static UA_StatusCode
sym_sign_sp_basic256(const UA_SecurityPolicy *policy,
                     void *channelContext, const UA_ByteString *message,
                     UA_ByteString *signature) {
    if(signature->length != UA_SHA1_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_ChannelContext *cc =
        (mbedtls_ChannelContext *)channelContext;

    if(UA_mbedTLS_PsaMacCompute(cc->localSymSigningKeyPsa.id,
                               PSA_ALG_HMAC(PSA_ALG_SHA_1),
                               message, signature) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    return UA_STATUSCODE_GOOD;
}

static size_t
sym_getSignatureSize_sp_basic256(const UA_SecurityPolicy *policy,
                                 const void *channelContext) {
    return UA_SHA1_LENGTH;
}

static size_t
sym_getSigningKeyLength_sp_basic256(const UA_SecurityPolicy *policy,
                                    const void *channelContext) {
    return UA_BASIC256_SYM_SIGNING_KEY_LENGTH;
}

static size_t
sym_getEncryptionKeyLength_sp_basic256(const UA_SecurityPolicy *policy,
                                       const void *channelContext) {
    return UA_SECURITYPOLICY_BASIC256_SYM_KEY_LENGTH;
}

static size_t
sym_getEncryptionBlockSize_sp_basic256(const UA_SecurityPolicy *policy,
                                       const void *channelContext) {
    return UA_SECURITYPOLICY_BASIC256_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
sym_getPlainTextBlockSize_sp_basic256(const UA_SecurityPolicy *policy,
                                      const void *channelContext) {
    return UA_SECURITYPOLICY_BASIC256_SYM_PLAIN_TEXT_BLOCK_SIZE;
}

static UA_StatusCode
sym_encrypt_sp_basic256(const UA_SecurityPolicy *policy,
                        void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const mbedtls_ChannelContext *cc =
        (const mbedtls_ChannelContext *)channelContext;

    if(cc->localSymIv.length != UA_SECURITYPOLICY_BASIC256_SYM_ENCRYPTION_BLOCK_SIZE)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t plainTextBlockSize = UA_SECURITYPOLICY_BASIC256_SYM_PLAIN_TEXT_BLOCK_SIZE;
    if(data->length % plainTextBlockSize != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_mbedTLS_PsaCipher(cc->localSymEncryptingKeyPsa.id,
                                PSA_ALG_CBC_NO_PADDING, true,
                                &cc->localSymIv, data);
}

static UA_StatusCode
sym_decrypt_sp_basic256(const UA_SecurityPolicy *policy,
                        void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const mbedtls_ChannelContext *cc =
        (const mbedtls_ChannelContext *)channelContext;

    size_t encryptionBlockSize = UA_SECURITYPOLICY_BASIC256_SYM_ENCRYPTION_BLOCK_SIZE;
    if(cc->remoteSymIv.length != encryptionBlockSize)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(data->length % encryptionBlockSize != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_mbedTLS_PsaCipher(cc->remoteSymEncryptingKeyPsa.id,
                                PSA_ALG_CBC_NO_PADDING, false,
                                &cc->remoteSymIv, data);
}

/* Assumes that the certificate has been verified externally */
static UA_StatusCode
parseRemoteCertificate_sp_basic256(mbedtls_ChannelContext *cc,
                                   const UA_ByteString *remoteCertificate) {
    if(remoteCertificate == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Parse the certificate */
    int mbedErr = mbedtls_x509_crt_parse(&cc->remoteCertificate, remoteCertificate->data,
                                         remoteCertificate->length);
    if(mbedErr)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Check the key length */
    size_t keylen = (mbedtls_pk_get_bitlen(&cc->remoteCertificate.pk) + 7) / 8;
    if(keylen < UA_SECURITYPOLICY_BASIC256_MINASYMKEYLENGTH ||
       keylen > UA_SECURITYPOLICY_BASIC256_MAXASYMKEYLENGTH)
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;

    return UA_STATUSCODE_GOOD;
}

static void
deleteContext_sp_basic256(const UA_SecurityPolicy *policy,
                          void *channelContext) {
    mbedtls_ChannelContext *cc = (mbedtls_ChannelContext *)channelContext;
    UA_ByteString_clear(&cc->localSymIv);
    UA_ByteString_clear(&cc->remoteSymIv);
    mbedtls_x509_crt_free(&cc->remoteCertificate);
    UA_mbedTLS_ChannelContext_clearPsa(cc);
    UA_free(cc);
}

static UA_StatusCode
newContext_sp_basic256(const UA_SecurityPolicy *securityPolicy,
                       const UA_ByteString *remoteCertificate,
                       void **channelContext) {
    if(securityPolicy == NULL || remoteCertificate == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the channel context */
    *channelContext = UA_malloc(sizeof(mbedtls_ChannelContext));
    if(*channelContext == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    mbedtls_ChannelContext *cc = (mbedtls_ChannelContext *)*channelContext;

    /* Initialize the channel context */
    UA_ByteString_init(&cc->localSymIv);
    UA_ByteString_init(&cc->remoteSymIv);
    mbedtls_x509_crt_init(&cc->remoteCertificate);
    UA_mbedTLS_ChannelContext_initPsa(cc);

    // TODO: this can be optimized so that we dont allocate memory before
    // parsing the certificate
    UA_StatusCode retval = parseRemoteCertificate_sp_basic256(cc, remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        deleteContext_sp_basic256(securityPolicy, cc);
        *channelContext = NULL;
    }
    return retval;
}

static void
clear_sp_basic256(UA_SecurityPolicy *securityPolicy) {
    if(securityPolicy == NULL)
        return;

    UA_ByteString_clear(&securityPolicy->localCertificate);

    if(securityPolicy->policyContext == NULL)
        return;

    /* delete all allocated members in the context */
    mbedtls_PolicyContext *pc = (mbedtls_PolicyContext *)
        securityPolicy->policyContext;

    mbedtls_pk_free(&pc->localPrivateKey);
    mbedtls_pk_free(&pc->csrLocalPrivateKey);
    UA_ByteString_clear(&pc->localCertThumbprint);

    UA_LOG_DEBUG(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Deleted members of EndpointContext for sp_basic256");

    UA_free(pc);
    securityPolicy->policyContext = NULL;
}

static UA_StatusCode
updateCertificateAndPrivateKey_sp_basic256(UA_SecurityPolicy *securityPolicy,
                                           const UA_ByteString newCertificate,
                                           const UA_ByteString newPrivateKey) {
    return UA_mbedTLS_UpdateCertificateAndPrivateKey(
        securityPolicy, newCertificate, newPrivateKey);
}

static UA_StatusCode
policyContext_newContext_sp_basic256(UA_SecurityPolicy *securityPolicy,
                                     const UA_ByteString localPrivateKey) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(securityPolicy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(localPrivateKey.length == 0) {
        UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Can not initialize security policy. Private key is empty.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    mbedtls_PolicyContext *pc = (mbedtls_PolicyContext *)
        UA_malloc(sizeof(mbedtls_PolicyContext));
    securityPolicy->policyContext = (void *)pc;
    if(!pc) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    /* Initialize the PolicyContext */
    memset(pc, 0, sizeof(mbedtls_PolicyContext));
    int mbedErr;
    mbedtls_pk_init(&pc->localPrivateKey);


    /* Set the private key */
    mbedErr = UA_mbedTLS_LoadPrivateKey(&localPrivateKey, &pc->localPrivateKey);
    if(mbedErr) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    /* Set the local certificate thumbprint */
    retval = UA_ByteString_allocBuffer(&pc->localCertThumbprint, UA_SHA1_LENGTH);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;
    retval = asym_makeThumbprint_sp_basic256(securityPolicy,
                                             &securityPolicy->localCertificate,
                                             &pc->localCertThumbprint);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    return UA_STATUSCODE_GOOD;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not create securityContext: %s", UA_StatusCode_name(retval));
    if(securityPolicy->policyContext != NULL)
        clear_sp_basic256(securityPolicy);
    return retval;
}

UA_StatusCode
UA_SecurityPolicy_Basic256(UA_SecurityPolicy *sp, const UA_ByteString localCertificate,
                           const UA_ByteString localPrivateKey, const UA_Logger *logger) {
    UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                   "!! WARNING !! The Basic256 SecurityPolicy is unsecure. "
                   "There are known attacks that break the encryption.");

    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = logger;
    sp->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256");
    sp->certificateGroupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    sp->certificateTypeId = UA_NS0ID(RSAMINAPPLICATIONCERTIFICATETYPE);
    sp->securityLevel = 0;
    sp->policyType = UA_SECURITYPOLICYTYPE_RSA;

    /* Asymmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *asymSig = &sp->asymSignatureAlgorithm;
    asymSig->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#rsa-sha1");
    asymSig->verify = asym_verify_sp_basic256;
    asymSig->sign = asym_sign_sp_basic256;
    asymSig->getLocalSignatureSize = UA_mbedTLS_getLocalPrivateKeyLength;
    asymSig->getRemoteSignatureSize = UA_mbedTLS_asym_getRemoteSignatureSize_generic;
    asymSig->getLocalKeyLength = NULL;
    asymSig->getRemoteKeyLength = NULL;

    /* Asymmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#rsa-oaep");
    asymEnc->encrypt = asym_encrypt_sp_basic256;
    asymEnc->decrypt = asym_decrypt_sp_basic256;
    asymEnc->getLocalKeyLength = asym_getLocalEncryptionKeyLength_sp_basic256;
    asymEnc->getRemoteKeyLength = asym_getRemoteEncryptionKeyLength_sp_basic256;
    asymEnc->getRemoteBlockSize = UA_mbedTLS_asym_getRemoteBlockSize_generic;
    asymEnc->getRemotePlainTextBlockSize = asym_getRemotePlainTextBlockSize_sp_basic256;

    /* Symmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha1");
    symSig->verify = sym_verify_sp_basic256;
    symSig->sign = sym_sign_sp_basic256;
    symSig->getLocalSignatureSize = sym_getSignatureSize_sp_basic256;
    symSig->getRemoteSignatureSize = sym_getSignatureSize_sp_basic256;
    symSig->getLocalKeyLength = sym_getSigningKeyLength_sp_basic256;
    symSig->getRemoteKeyLength = sym_getSigningKeyLength_sp_basic256;

    /* Symmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#aes256-cbc");
    symEnc->encrypt = sym_encrypt_sp_basic256;
    symEnc->decrypt = sym_decrypt_sp_basic256;
    symEnc->getLocalKeyLength = sym_getEncryptionKeyLength_sp_basic256;
    symEnc->getRemoteKeyLength = sym_getEncryptionKeyLength_sp_basic256;
    symEnc->getRemoteBlockSize = sym_getEncryptionBlockSize_sp_basic256;
    symEnc->getRemotePlainTextBlockSize = sym_getPlainTextBlockSize_sp_basic256;

    /* Certificate Signing
     * Use the same signature algorithm as the asymmetric component for
     * certificate signing (see standard). */
    sp->certSignatureAlgorithm = sp->asymSignatureAlgorithm;

    /* Direct Method Pointers */
    sp->newChannelContext = newContext_sp_basic256;
    sp->deleteChannelContext = deleteContext_sp_basic256;
    sp->setLocalSymEncryptingKey = UA_mbedTLS_setLocalSymEncryptingKey_generic;
    sp->setLocalSymSigningKey = UA_mbedTLS_setLocalSymSigningKey_generic;
    sp->setLocalSymIv = UA_mbedTLS_setLocalSymIv_generic;
    sp->setRemoteSymEncryptingKey = UA_mbedTLS_setRemoteSymEncryptingKey_generic;
    sp->setRemoteSymSigningKey = UA_mbedTLS_setRemoteSymSigningKey_generic;
    sp->setRemoteSymIv = UA_mbedTLS_setRemoteSymIv_generic;
    sp->compareCertificate = UA_mbedTLS_compareCertificate_generic;
    sp->generateKey = UA_mbedTLS_sym_generateKey_generic;
    sp->generateNonce = UA_mbedTLS_sym_generateNonce_generic;
    sp->nonceLength = 32;
    sp->makeCertThumbprint = asym_makeThumbprint_sp_basic256;
    sp->compareCertThumbprint = UA_mbedTLS_compareCertificateThumbprint_generic;
    sp->updateCertificate = updateCertificateAndPrivateKey_sp_basic256;
    sp->createSigningRequest = UA_mbedTLS_createSigningRequest_generic;
    sp->clear = clear_sp_basic256;

    UA_StatusCode res =
        UA_mbedTLS_LoadLocalCertificate(&localCertificate, &sp->localCertificate);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    res = policyContext_newContext_sp_basic256(sp, localPrivateKey);
    if(res != UA_STATUSCODE_GOOD)
        clear_sp_basic256(sp);

    return res;
}

#endif
