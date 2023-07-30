/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018-2019 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2018 (c) HMS Industrial Networks AB (Author: Jonas Green)
 *    Copyright 2020 (c) Wind River Systems, Inc.
 *    Copyright 2020 (c) basysKom GmbH
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Kai Huebl)
 * 
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS

#include <open62541/nodeids.h>
#include "securitypolicy_mbedtls_common.h"

#include <mbedtls/aes.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/md.h>
#include <mbedtls/sha1.h>
#include <mbedtls/version.h>
#include <mbedtls/x509_crt.h>

/* Notes:
 * mbedTLS' AES allows in-place encryption and decryption. Sow we don't have to
 * allocate temp buffers.
 * https://tls.mbed.org/discussions/generic/in-place-decryption-with-aes256-same-input-output-buffer
 */

#define UA_SECURITYPOLICY_BASIC128RSA15_RSAPADDING_LEN 11
#define UA_SECURITYPOLICY_BASIC128RSA15_SYM_KEY_LENGTH 16
#define UA_BASIC128RSA15_SYM_SIGNING_KEY_LENGTH 16
#define UA_SECURITYPOLICY_BASIC128RSA15_SYM_ENCRYPTION_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_BASIC128RSA15_SYM_PLAIN_TEXT_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_BASIC128RSA15_MINASYMKEYLENGTH 128
#define UA_SECURITYPOLICY_BASIC128RSA15_MAXASYMKEYLENGTH 256

/********************/
/* AsymmetricModule */
/********************/

static UA_StatusCode
asym_verify_sp_basic128rsa15(ChannelContext_mbedtls *cc,
                             const UA_ByteString *message,
                             const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    return mbedtls_verifySig_sha1(&cc->remoteCertificate, message, signature);
}

static UA_StatusCode
asym_sign_sp_basic128rsa15(ChannelContext_mbedtls *cc,
                           const UA_ByteString *message,
                           UA_ByteString *signature) {
    return channelContext_mbedtls_loadKeyThenSign(cc, message, signature, mbedtls_sign_sha1);
}

static UA_StatusCode
asym_signAuth_sp_basic128rsa15(ChannelContext_mbedtls *cc,
                               const UA_ByteString *message,
                               UA_ByteString *signature,
			   			       UA_ByteString *privateKey) {
    return channelContext_mbedtls_parseKeyThenSign(cc, message, signature, privateKey, mbedtls_sign_sha1);
}

static size_t
getSignatureSize_sp_basic128rsa15(const ChannelContext_mbedtls *cc, const mbedtls_pk_context *privateKey) {
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    return mbedtls_pk_rsa(*privateKey)->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(*privateKey));
#endif
}

static size_t
asym_getLocalSignatureSize_sp_basic128rsa15(const ChannelContext_mbedtls *cc) {
    return channelContext_mbedtls_loadKeyThenGetSize(cc, getSignatureSize_sp_basic128rsa15);
}

static size_t
asym_getRemoteSignatureSize_sp_basic128rsa15(const ChannelContext_mbedtls *cc) {
    if(cc == NULL)
        return 0;
    return getSignatureSize_sp_basic128rsa15(cc, &cc->remoteCertificate.pk);
}

static UA_StatusCode
asym_encrypt_sp_basic128rsa15(ChannelContext_mbedtls *cc,
                              UA_ByteString *data) {
    if(cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_rsa_context *remoteRsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    mbedtls_rsa_set_padding(remoteRsaContext, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);

#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    size_t keylen = remoteRsaContext->len;
#else
    size_t keylen = mbedtls_rsa_get_len(remoteRsaContext);
#endif
    size_t plainTextBlockSize =
        keylen - UA_SECURITYPOLICY_BASIC128RSA15_RSAPADDING_LEN;
    if(data->length % plainTextBlockSize != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t blocks = data->length / plainTextBlockSize;
    UA_ByteString encrypted;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&encrypted, blocks * keylen);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    size_t lenDataToEncrypt = data->length;
    size_t inOffset = 0;
    size_t offset = 0;
    size_t outLength = 0;
    PolicyContext_mbedtls *pc = cc->policyContext;
    while(lenDataToEncrypt >= plainTextBlockSize) {
        int mbedErr = mbedtls_pk_encrypt(&cc->remoteCertificate.pk,
                                         data->data + inOffset, plainTextBlockSize,
                                         encrypted.data + offset, &outLength,
                                         encrypted.length - offset,
                                         mbedtls_ctr_drbg_random,
                                         &pc->drbgContext);
        if(mbedErr) {
            UA_ByteString_clear(&encrypted);
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        inOffset += plainTextBlockSize;
        offset += outLength;
        lenDataToEncrypt -= plainTextBlockSize;
    }

    memcpy(data->data, encrypted.data, offset);
    UA_ByteString_clear(&encrypted);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
decrypt_rsa15_sp_basic128rsa15(ChannelContext_mbedtls *cc,
                               mbedtls_pk_context *privateKey,
                               UA_ByteString *data) {
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(*privateKey);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    size_t keylen = rsaContext->len;
#else
    size_t keylen = mbedtls_rsa_get_len(rsaContext);
#endif
    if(data->length % keylen != 0) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    size_t inOffset = 0;
    size_t outOffset = 0;
    size_t outLength = 0;
    unsigned char buf[512];

    while(inOffset < data->length) {
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
        int mbedErr = mbedtls_pk_decrypt(privateKey,
                                         data->data + inOffset, rsaContext->len,
                                         buf, &outLength, 512, NULL, NULL);
#else
        int mbedErr = mbedtls_pk_decrypt(privateKey,
                                         data->data + inOffset, keylen,
                                         buf, &outLength, 512, NULL, NULL);
#endif

        if(mbedErr)
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

        memcpy(data->data + outOffset, buf, outLength);
        inOffset += keylen;
        outOffset += outLength;
    }

    data->length = outOffset;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_decrypt_sp_basic128rsa15(ChannelContext_mbedtls *cc,
                              UA_ByteString *data) {
    return channelContext_mbedtls_loadKeyThenCrypt(cc, data, decrypt_rsa15_sp_basic128rsa15);
}

static size_t
getKeySize_sp_basic128rsa15(const ChannelContext_mbedtls *cc, const mbedtls_pk_context *privateKey) {
    return mbedtls_pk_get_len(privateKey) * 8;
}

static size_t
asym_getLocalEncryptionKeyLength_sp_basic128rsa15(const ChannelContext_mbedtls *cc) {
    return channelContext_mbedtls_loadKeyThenGetSize(cc, getKeySize_sp_basic128rsa15);
}

static size_t
asym_getRemoteEncryptionKeyLength_sp_basic128rsa15(const ChannelContext_mbedtls *cc) {
    return getKeySize_sp_basic128rsa15(cc, &cc->remoteCertificate.pk);
}

static size_t
asym_getRemoteBlockSize_sp_basic128rsa15(const ChannelContext_mbedtls *cc) {
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    mbedtls_rsa_context *const rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    return rsaContext->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk));
#endif
}

static size_t
asym_getRemotePlainTextBlockSize_sp_basic128rsa15(const ChannelContext_mbedtls *cc) {
    if(cc == NULL)
        return 0;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    mbedtls_rsa_context *const rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    return rsaContext->len - UA_SECURITYPOLICY_BASIC128RSA15_RSAPADDING_LEN;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk)) -
        UA_SECURITYPOLICY_BASIC128RSA15_RSAPADDING_LEN;
#endif
}

static UA_StatusCode
asym_makeThumbprint_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                     const UA_ByteString *certificate,
                                     UA_ByteString *thumbprint) {
    if(securityPolicy == NULL || certificate == NULL || thumbprint == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return mbedtls_thumbprint_sha1(certificate, thumbprint);
}

static UA_StatusCode
asymmetricModule_compareCertificateThumbprint_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                                               UA_PKIStore *pkiStore,
                                                               const UA_ByteString *certificateThumbprint) {
    return mbedtls_compare_thumbprints(securityPolicy,
                                       pkiStore,
                                       certificateThumbprint,
                                       asym_makeThumbprint_sp_basic128rsa15);
}

/*******************/
/* SymmetricModule */
/*******************/

static UA_StatusCode
sym_verify_sp_basic128rsa15(ChannelContext_mbedtls *cc,
                            const UA_ByteString *message,
                            const UA_ByteString *signature) {
    if(cc == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Compute MAC */
    if(signature->length != UA_SHA1_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    PolicyContext_mbedtls *pc = cc->policyContext;

    unsigned char mac[UA_SHA1_LENGTH];
    mbedtls_hmac(&pc->mdContext, &cc->remoteSymSigningKey, message, mac);

    /* Compare with Signature */
    if(!UA_constantTimeEqual(signature->data, mac, UA_SHA1_LENGTH))
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_sign_sp_basic128rsa15(const ChannelContext_mbedtls *cc,
                          const UA_ByteString *message,
                          UA_ByteString *signature) {
    if(signature->length != UA_SHA1_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_hmac(&cc->policyContext->mdContext, &cc->localSymSigningKey,
                 message, signature->data);
    return UA_STATUSCODE_GOOD;
}

static size_t
sym_getSignatureSize_sp_basic128rsa15(const void *channelContext) {
    return UA_SHA1_LENGTH;
}

static size_t
sym_getSigningKeyLength_sp_basic128rsa15(const void *const channelContext) {
    return UA_BASIC128RSA15_SYM_SIGNING_KEY_LENGTH;
}

static size_t
sym_getEncryptionKeyLength_sp_basic128rsa15(const void *channelContext) {
    return UA_SECURITYPOLICY_BASIC128RSA15_SYM_KEY_LENGTH;
}

static size_t
sym_getEncryptionBlockSize_sp_basic128rsa15(const void *const channelContext) {
    return UA_SECURITYPOLICY_BASIC128RSA15_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
sym_getPlainTextBlockSize_sp_basic128rsa15(const void *const channelContext) {
    return UA_SECURITYPOLICY_BASIC128RSA15_SYM_PLAIN_TEXT_BLOCK_SIZE;
}

static UA_StatusCode
sym_encrypt_sp_basic128rsa15(const ChannelContext_mbedtls *cc,
                             UA_ByteString *data) {
    if(cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(cc->localSymIv.length != UA_SECURITYPOLICY_BASIC128RSA15_SYM_ENCRYPTION_BLOCK_SIZE)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t plainTextBlockSize = UA_SECURITYPOLICY_BASIC128RSA15_SYM_PLAIN_TEXT_BLOCK_SIZE;
    if(data->length % plainTextBlockSize != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Keylength in bits */
    unsigned int keylength = (unsigned int)(cc->localSymEncryptingKey.length * 8);
    mbedtls_aes_context aesContext;
    int mbedErr = mbedtls_aes_setkey_enc(&aesContext, cc->localSymEncryptingKey.data, keylength);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString ivCopy;
    UA_StatusCode retval = UA_ByteString_copy(&cc->localSymIv, &ivCopy);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    mbedErr = mbedtls_aes_crypt_cbc(&aesContext, MBEDTLS_AES_ENCRYPT, data->length,
                                    ivCopy.data, data->data, data->data);
    if(mbedErr)
        retval = UA_STATUSCODE_BADINTERNALERROR;
    UA_ByteString_clear(&ivCopy);
    return retval;
}

static UA_StatusCode
sym_decrypt_sp_basic128rsa15(const ChannelContext_mbedtls *cc,
                             UA_ByteString *data) {
    if(cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t encryptionBlockSize = UA_SECURITYPOLICY_BASIC128RSA15_SYM_ENCRYPTION_BLOCK_SIZE;
    if(cc->remoteSymIv.length != encryptionBlockSize)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(data->length % encryptionBlockSize != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    unsigned int keylength = (unsigned int)(cc->remoteSymEncryptingKey.length * 8);
    mbedtls_aes_context aesContext;
    int mbedErr = mbedtls_aes_setkey_dec(&aesContext,
                                         cc->remoteSymEncryptingKey.data, keylength);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString ivCopy;
    UA_StatusCode retval = UA_ByteString_copy(&cc->remoteSymIv, &ivCopy);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    mbedErr = mbedtls_aes_crypt_cbc(&aesContext, MBEDTLS_AES_DECRYPT, data->length,
                                    ivCopy.data, data->data, data->data);
    if(mbedErr)
        retval = UA_STATUSCODE_BADINTERNALERROR;
    UA_ByteString_clear(&ivCopy);
    return retval;
}

static UA_StatusCode
sym_generateKey_sp_basic128rsa15(void *policyContext, const UA_ByteString *secret,
                                 const UA_ByteString *seed, UA_ByteString *out) {
    if(secret == NULL || seed == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    PolicyContext_mbedtls *pc = (PolicyContext_mbedtls *)policyContext;
    return mbedtls_generateKey(&pc->mdContext, secret, seed, out);
}

static UA_StatusCode
sym_generateNonce_sp_basic128rsa15(void *policyContext, UA_ByteString *out) {
    if(out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    PolicyContext_mbedtls *pc = (PolicyContext_mbedtls *)policyContext;
    int mbedErr = mbedtls_ctr_drbg_random(&pc->drbgContext, out->data, out->length);
    if(mbedErr)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_newContext_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                           UA_PKIStore *pkiStore,
                                           const UA_ByteString *remoteCertificate,
                                           void **pp_contextData) {
    return channelContext_mbedtls_newContext(securityPolicy,
                                             pkiStore,
                                             remoteCertificate,
                                             UA_SECURITYPOLICY_BASIC128RSA15_MINASYMKEYLENGTH,
                                             UA_SECURITYPOLICY_BASIC128RSA15_MAXASYMKEYLENGTH,
                                             pp_contextData);
}

static void
clear_sp_basic128rsa15(UA_SecurityPolicy *securityPolicy) {
    if(securityPolicy == NULL)
        return;

    if(securityPolicy->policyContext == NULL)
        return;

    /* delete all allocated members in the context */
    PolicyContext_mbedtls *pc = (PolicyContext_mbedtls *)
        securityPolicy->policyContext;

    mbedtls_ctr_drbg_free(&pc->drbgContext);
    mbedtls_entropy_free(&pc->entropyContext);
    mbedtls_md_free(&pc->mdContext);

    UA_LOG_DEBUG(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Deleted members of EndpointContext for sp_basic128rsa15");

    UA_free(pc);
    securityPolicy->policyContext = NULL;
}

static UA_StatusCode
policyContext_newContext_sp_basic128rsa15(UA_SecurityPolicy *securityPolicy) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    PolicyContext_mbedtls *pc = (PolicyContext_mbedtls *)
        UA_malloc(sizeof(PolicyContext_mbedtls));
    securityPolicy->policyContext = (void *)pc;
    if(!pc) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    /* Initialize the PolicyContext */
    memset(pc, 0, sizeof(PolicyContext_mbedtls));
    mbedtls_ctr_drbg_init(&pc->drbgContext);
    mbedtls_entropy_init(&pc->entropyContext);
    mbedtls_md_init(&pc->mdContext);

    /* Initialized the message digest */
    const mbedtls_md_info_t *const mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    int mbedErr = mbedtls_md_setup(&pc->mdContext, mdInfo, MBEDTLS_MD_SHA1);
    if(mbedErr) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    mbedErr = mbedtls_entropy_self_test(0);

    if(mbedErr) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    /* Seed the RNG */
    char *personalization = "open62541-drbg";
    mbedErr = mbedtls_ctr_drbg_seed(&pc->drbgContext, mbedtls_entropy_func,
                                    &pc->entropyContext,
                                    (const unsigned char *)personalization, 14);
    if(mbedErr) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    return UA_STATUSCODE_GOOD;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not create securityContext: %s", UA_StatusCode_name(retval));
    if(securityPolicy->policyContext != NULL)
        clear_sp_basic128rsa15(securityPolicy);
    return retval;
}

UA_StatusCode
UA_SecurityPolicy_Basic128Rsa15(UA_SecurityPolicy *policy,
		                        const UA_Logger *logger) {
    if(policy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    memset(policy, 0, sizeof(UA_SecurityPolicy));
    policy->logger = logger;

    policy->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15\0");
    policy->certificateTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE);

    UA_SecurityPolicyAsymmetricModule *const asymmetricModule = &policy->asymmetricModule;
    UA_SecurityPolicySymmetricModule *const symmetricModule = &policy->symmetricModule;
    UA_SecurityPolicyChannelModule *const channelModule = &policy->channelModule;

    /* AsymmetricModule */
    UA_SecurityPolicySignatureAlgorithm *asym_signatureAlgorithm =
        &asymmetricModule->cryptoModule.signatureAlgorithm;
    asym_signatureAlgorithm->uri =
        UA_STRING("http://www.w3.org/2000/09/xmldsig#rsa-sha1\0");
    asym_signatureAlgorithm->verify =
        (UA_StatusCode (*)(void *, const UA_ByteString *, const UA_ByteString *))asym_verify_sp_basic128rsa15;
    asym_signatureAlgorithm->sign =
        (UA_StatusCode (*)(void *, const UA_ByteString *, UA_ByteString *))asym_sign_sp_basic128rsa15;
    asym_signatureAlgorithm->signAuth =
        (UA_StatusCode (*)(void *, const UA_ByteString *, UA_ByteString *, UA_ByteString*))asym_signAuth_sp_basic128rsa15;
    asym_signatureAlgorithm->getLocalSignatureSize =
        (size_t (*)(const void *))asym_getLocalSignatureSize_sp_basic128rsa15;
    asym_signatureAlgorithm->getRemoteSignatureSize =
        (size_t (*)(const void *))asym_getRemoteSignatureSize_sp_basic128rsa15;
    asym_signatureAlgorithm->getLocalKeyLength = NULL; // TODO: Write function
    asym_signatureAlgorithm->getRemoteKeyLength = NULL; // TODO: Write function

    UA_SecurityPolicyEncryptionAlgorithm *asym_encryptionAlgorithm =
        &asymmetricModule->cryptoModule.encryptionAlgorithm;
    asym_encryptionAlgorithm->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#rsa-1_5");
    asym_encryptionAlgorithm->encrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *))asym_encrypt_sp_basic128rsa15;
    asym_encryptionAlgorithm->decrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *)) asym_decrypt_sp_basic128rsa15;
    asym_encryptionAlgorithm->getLocalKeyLength =
        (size_t (*)(const void *))asym_getLocalEncryptionKeyLength_sp_basic128rsa15;
    asym_encryptionAlgorithm->getRemoteKeyLength =
        (size_t (*)(const void *))asym_getRemoteEncryptionKeyLength_sp_basic128rsa15;
    asym_encryptionAlgorithm->getRemoteBlockSize =
        (size_t (*)(const void *))asym_getRemoteBlockSize_sp_basic128rsa15;
    asym_encryptionAlgorithm->getRemotePlainTextBlockSize =
        (size_t (*)(const void *))asym_getRemotePlainTextBlockSize_sp_basic128rsa15;

    asymmetricModule->makeCertificateThumbprint = asym_makeThumbprint_sp_basic128rsa15;
    asymmetricModule->compareCertificateThumbprint =
        asymmetricModule_compareCertificateThumbprint_sp_basic128rsa15;

    /* SymmetricModule */
    symmetricModule->generateKey = sym_generateKey_sp_basic128rsa15;
    symmetricModule->generateNonce = sym_generateNonce_sp_basic128rsa15;

    UA_SecurityPolicySignatureAlgorithm *sym_signatureAlgorithm =
        &symmetricModule->cryptoModule.signatureAlgorithm;
    sym_signatureAlgorithm->uri =
        UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha1\0");
    sym_signatureAlgorithm->verify =
        (UA_StatusCode (*)(void *, const UA_ByteString *,
                           const UA_ByteString *))sym_verify_sp_basic128rsa15;
    sym_signatureAlgorithm->sign =
        (UA_StatusCode (*)(void *, const UA_ByteString *, UA_ByteString *))sym_sign_sp_basic128rsa15;
    sym_signatureAlgorithm->getLocalSignatureSize = sym_getSignatureSize_sp_basic128rsa15;
    sym_signatureAlgorithm->getRemoteSignatureSize = sym_getSignatureSize_sp_basic128rsa15;
    sym_signatureAlgorithm->getLocalKeyLength =
        (size_t (*)(const void *))sym_getSigningKeyLength_sp_basic128rsa15;
    sym_signatureAlgorithm->getRemoteKeyLength =
        (size_t (*)(const void *))sym_getSigningKeyLength_sp_basic128rsa15;

    UA_SecurityPolicyEncryptionAlgorithm *sym_encryptionAlgorithm =
        &symmetricModule->cryptoModule.encryptionAlgorithm;
    sym_encryptionAlgorithm->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#aes128-cbc");
    sym_encryptionAlgorithm->encrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *))sym_encrypt_sp_basic128rsa15;
    sym_encryptionAlgorithm->decrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *))sym_decrypt_sp_basic128rsa15;
    sym_encryptionAlgorithm->getLocalKeyLength = sym_getEncryptionKeyLength_sp_basic128rsa15;
    sym_encryptionAlgorithm->getRemoteKeyLength = sym_getEncryptionKeyLength_sp_basic128rsa15;
    sym_encryptionAlgorithm->getRemoteBlockSize =
        (size_t (*)(const void *))sym_getEncryptionBlockSize_sp_basic128rsa15;
    sym_encryptionAlgorithm->getRemotePlainTextBlockSize =
        (size_t (*)(const void *))sym_getPlainTextBlockSize_sp_basic128rsa15;
    symmetricModule->secureChannelNonceLength = 16;

    // Use the same signature algorithm as the asymmetric component for certificate signing (see standard)
    policy->certificateSigningAlgorithm = policy->asymmetricModule.cryptoModule.signatureAlgorithm;

    /* ChannelModule */
    channelModule->newContext = channelContext_newContext_sp_basic128rsa15;
    channelModule->deleteContext = (void (*)(void *))channelContext_mbedtls_deleteContext;

    channelModule->setLocalSymEncryptingKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_mbedtls_setLocalSymEncryptingKey;
    channelModule->setLocalSymSigningKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_mbedtls_setLocalSymSigningKey;
    channelModule->setLocalSymIv = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_mbedtls_setLocalSymIv;

    channelModule->setRemoteSymEncryptingKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_mbedtls_setRemoteSymEncryptingKey;
    channelModule->setRemoteSymSigningKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_mbedtls_setRemoteSymSigningKey;
    channelModule->setRemoteSymIv = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_mbedtls_setRemoteSymIv;

    channelModule->compareCertificate = (UA_StatusCode (*)(const void *, const UA_ByteString *))
        channelContext_mbedtls_compareCertificate;

    policy->getLocalCertificate = UA_mbedTLS_LoadLocalCertificate;
    policy->clear = clear_sp_basic128rsa15;

    UA_StatusCode res = policyContext_newContext_sp_basic128rsa15(policy);
    if(res != UA_STATUSCODE_GOOD)
        clear_sp_basic128rsa15(policy);

    return res;
}

#endif
