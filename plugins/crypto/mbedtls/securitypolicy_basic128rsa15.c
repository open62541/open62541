/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018-2019 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2018 (c) HMS Industrial Networks AB (Author: Jonas Green)
 *    Copyright 2020 (c) Wind River Systems, Inc.
 *    Copyright 2020 (c) basysKom GmbH
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS

#include "securitypolicy_common.h"

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
#define UA_SECURITYPOLICY_BASIC128RSA15_MAXASYMKEYLENGTH 512

static UA_StatusCode
asym_verify_basic128rsa15(const UA_SecurityPolicy *policy, void *channelContext,
                          const UA_ByteString *message,
                          const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_ChannelContext *cc = (mbedtls_ChannelContext *)channelContext;
    return mbedtls_verifySig_sha1(&cc->remoteCertificate, message, signature);
}

static UA_StatusCode
asym_sign_basic128rsa15(const UA_SecurityPolicy *policy,
                        void *channelContext, const UA_ByteString *message,
                        UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_PolicyContext *pc =
        (mbedtls_PolicyContext*)policy->policyContext;
    return mbedtls_sign_sha1(&pc->localPrivateKey, &pc->drbgContext, message, signature);
}

static size_t
asym_getLocalSignatureSize_basic128rsa15(const UA_SecurityPolicy *policy,
                                         const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    mbedtls_PolicyContext *pc =
        (mbedtls_PolicyContext*)policy->policyContext;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    return mbedtls_pk_rsa(pc->localPrivateKey)->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(pc->localPrivateKey));
#endif
}

static UA_StatusCode
asym_encrypt_basic128rsa15(const UA_SecurityPolicy *policy,
                           void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_ChannelContext *cc = (mbedtls_ChannelContext *)channelContext;
    mbedtls_PolicyContext *pc =
        (mbedtls_PolicyContext*)policy->policyContext;

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
asym_decrypt_basic128rsa15(const UA_SecurityPolicy *policy,
                           void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_PolicyContext *pc =
        (mbedtls_PolicyContext*)policy->policyContext;

    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(pc->localPrivateKey);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    size_t keylen = rsaContext->len;
#else
    size_t keylen = mbedtls_rsa_get_len(rsaContext);
#endif
    if(data->length % keylen != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t inOffset = 0;
    size_t outOffset = 0;
    size_t outLength = 0;
    unsigned char buf[512];

    while(inOffset < data->length) {
        int mbedErr = mbedtls_pk_decrypt(&pc->localPrivateKey,
                                         data->data + inOffset, keylen,
                                         buf, &outLength, 512,
                                         mbedtls_ctr_drbg_random,
                                         &pc->drbgContext);
        if(mbedErr)
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

        memcpy(data->data + outOffset, buf, outLength);
        inOffset += keylen;
        outOffset += outLength;
    }

    data->length = outOffset;
    return UA_STATUSCODE_GOOD;
}

static size_t
asym_getRemoteEncryptionKeyLength_basic128rsa15(const UA_SecurityPolicy *policy,
                                                const void *channelContext) {
    const mbedtls_ChannelContext *cc =
        (const mbedtls_ChannelContext *)channelContext;
    return mbedtls_pk_get_len(&cc->remoteCertificate.pk) * 8;
}

static size_t
asym_getRemotePlainTextBlockSize_basic128rsa15(const UA_SecurityPolicy *policy,
                                               const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    const mbedtls_ChannelContext *cc =
        (const mbedtls_ChannelContext *)channelContext;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    mbedtls_rsa_context *const rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    return rsaContext->len - UA_SECURITYPOLICY_BASIC128RSA15_RSAPADDING_LEN;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk)) -
        UA_SECURITYPOLICY_BASIC128RSA15_RSAPADDING_LEN;
#endif
}

static UA_StatusCode
asym_makeThumbprint_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                  const UA_ByteString *certificate,
                                  UA_ByteString *thumbprint) {
    if(securityPolicy == NULL || certificate == NULL || thumbprint == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return mbedtls_thumbprint_sha1(certificate, thumbprint);
}

static UA_StatusCode
compareCertificateThumbprint_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                           const UA_ByteString *certificateThumbprint) {
    if(securityPolicy == NULL || certificateThumbprint == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_PolicyContext *pc =
        (mbedtls_PolicyContext *)securityPolicy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint))
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_verify_basic128rsa15(const UA_SecurityPolicy *policy, void *channelContext,
                         const UA_ByteString *message,
                         const UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Compute MAC */
    if(signature->length != UA_SHA1_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    mbedtls_PolicyContext *pc =
        (mbedtls_PolicyContext*)policy->policyContext;
    mbedtls_ChannelContext *cc = (mbedtls_ChannelContext *)channelContext;

    unsigned char mac[UA_SHA1_LENGTH];
    if(mbedtls_hmac(&pc->mdContext, &cc->remoteSymSigningKey,
                    message, mac) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Compare with Signature */
    if(!UA_constantTimeEqual(signature->data, mac, UA_SHA1_LENGTH))
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_sign_basic128rsa15(const UA_SecurityPolicy *policy,
                       void *channelContext, const UA_ByteString *message,
                       UA_ByteString *signature) {
    if(signature->length != UA_SHA1_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_PolicyContext *pc =
        (mbedtls_PolicyContext*)policy->policyContext;
    mbedtls_ChannelContext *cc = (mbedtls_ChannelContext *)channelContext;
    if(mbedtls_hmac(&pc->mdContext, &cc->localSymSigningKey,
                    message, signature->data) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

static size_t
sym_getSignatureSize_basic128rsa15(const UA_SecurityPolicy *policy,
                                   const void *channelContext) {
    return UA_SHA1_LENGTH;
}

static size_t
sym_getSigningKeyLength_basic128rsa15(const UA_SecurityPolicy *policy,
                                      const void *channelContext) {
    return UA_BASIC128RSA15_SYM_SIGNING_KEY_LENGTH;
}

static size_t
sym_getEncryptionKeyLength_basic128rsa15(const UA_SecurityPolicy *policy,
                                            const void *channelContext) {
    return UA_SECURITYPOLICY_BASIC128RSA15_SYM_KEY_LENGTH;
}

static size_t
sym_getEncryptionBlockSize_basic128rsa15(const UA_SecurityPolicy *policy,
                                            const void *channelContext) {
    return UA_SECURITYPOLICY_BASIC128RSA15_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
sym_getPlainTextBlockSize_basic128rsa15(const UA_SecurityPolicy *policy,
                                           const void *channelContext) {
    return UA_SECURITYPOLICY_BASIC128RSA15_SYM_PLAIN_TEXT_BLOCK_SIZE;
}

static UA_StatusCode
sym_encrypt_basic128rsa15(const UA_SecurityPolicy *policy,
                          void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_ChannelContext *cc = (mbedtls_ChannelContext *)channelContext;

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
sym_decrypt_basic128rsa15(const UA_SecurityPolicy *policy,
                          void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_ChannelContext *cc = (mbedtls_ChannelContext *)channelContext;

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
sym_generateKey_basic128rsa15(const UA_SecurityPolicy *policy,
                              void *channelContext, const UA_ByteString *secret,
                              const UA_ByteString *seed, UA_ByteString *out) {
    if(secret == NULL || seed == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_PolicyContext *pc =
        (mbedtls_PolicyContext *)policy->policyContext;
    return mbedtls_generateKey(&pc->mdContext, secret, seed, out);
}

static UA_StatusCode
sym_generateNonce_basic128rsa15(const UA_SecurityPolicy *policy,
                                void *channelContext, UA_ByteString *out) {
    if(out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_PolicyContext *pc =
        (mbedtls_PolicyContext *)policy->policyContext;
    int mbedErr = mbedtls_ctr_drbg_random(&pc->drbgContext, out->data, out->length);
    if(mbedErr)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

/* Assumes that the certificate has been verified externally */
static UA_StatusCode
parseRemoteCertificate_basic128rsa15(mbedtls_ChannelContext *cc,
                                        const UA_ByteString *remoteCertificate) {
    if(remoteCertificate == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Parse the certificate */
    int mbedErr = mbedtls_x509_crt_parse(&cc->remoteCertificate, remoteCertificate->data,
                                         remoteCertificate->length);
    if(mbedErr)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Check the key length */
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    if(rsaContext->len < UA_SECURITYPOLICY_BASIC128RSA15_MINASYMKEYLENGTH ||
       rsaContext->len > UA_SECURITYPOLICY_BASIC128RSA15_MAXASYMKEYLENGTH)
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
#else
    size_t keylen = mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk));
    if(keylen < UA_SECURITYPOLICY_BASIC128RSA15_MINASYMKEYLENGTH ||
       keylen > UA_SECURITYPOLICY_BASIC128RSA15_MAXASYMKEYLENGTH)
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
#endif
    return UA_STATUSCODE_GOOD;
}

static void
deleteContext_basic128rsa15(const UA_SecurityPolicy *policy,
                            void *channelContext) {
    mbedtls_ChannelContext *cc = (mbedtls_ChannelContext *)channelContext;
    UA_ByteString_clear(&cc->localSymSigningKey);
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    UA_ByteString_clear(&cc->localSymIv);
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    UA_ByteString_clear(&cc->remoteSymIv);
    mbedtls_x509_crt_free(&cc->remoteCertificate);
    UA_free(cc);
}

static UA_StatusCode
newContext_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                         const UA_ByteString *remoteCertificate,
                         void **channelContext) {
    if(securityPolicy == NULL || remoteCertificate == NULL || channelContext  == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the channel context */
    *channelContext = UA_malloc(sizeof(mbedtls_ChannelContext));
    if(*channelContext == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    mbedtls_ChannelContext *cc = (mbedtls_ChannelContext *)*channelContext;

    /* Initialize the channel context */
    UA_ByteString_init(&cc->localSymSigningKey);
    UA_ByteString_init(&cc->localSymEncryptingKey);
    UA_ByteString_init(&cc->localSymIv);
    UA_ByteString_init(&cc->remoteSymSigningKey);
    UA_ByteString_init(&cc->remoteSymEncryptingKey);
    UA_ByteString_init(&cc->remoteSymIv);
    mbedtls_x509_crt_init(&cc->remoteCertificate);

    // TODO: this can be optimized so that we dont allocate memory before parsing the certificate
    UA_StatusCode retval = parseRemoteCertificate_basic128rsa15(cc, remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        deleteContext_basic128rsa15(securityPolicy, cc);
        *channelContext = NULL;
    }
    return retval;
}

static void
clear_basic128rsa15(UA_SecurityPolicy *securityPolicy) {
    if(securityPolicy == NULL)
        return;

    UA_ByteString_clear(&securityPolicy->localCertificate);

    if(securityPolicy->policyContext == NULL)
        return;

    /* delete all allocated members in the context */
    mbedtls_PolicyContext *pc = (mbedtls_PolicyContext *)
        securityPolicy->policyContext;

    mbedtls_ctr_drbg_free(&pc->drbgContext);
    mbedtls_entropy_free(&pc->entropyContext);
    mbedtls_pk_free(&pc->localPrivateKey);
    mbedtls_pk_free(&pc->csrLocalPrivateKey);
    mbedtls_md_free(&pc->mdContext);
    UA_ByteString_clear(&pc->localCertThumbprint);

    UA_LOG_DEBUG(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Deleted members of EndpointContext for basic128rsa15");

    UA_free(pc);
    securityPolicy->policyContext = NULL;
}

static UA_StatusCode
updateCertificateAndPrivateKey_basic128rsa15(UA_SecurityPolicy *securityPolicy,
                                                const UA_ByteString newCertificate,
                                                const UA_ByteString newPrivateKey) {
    if(securityPolicy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(securityPolicy->policyContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_PolicyContext *pc =
        (mbedtls_PolicyContext *)securityPolicy->policyContext;

    UA_Boolean isLocalKey = false;
    if(newPrivateKey.length <= 0) {
        if(UA_CertificateUtils_comparePublicKeys(&newCertificate, &securityPolicy->localCertificate) == 0)
            isLocalKey = true;
    }

    UA_ByteString_clear(&securityPolicy->localCertificate);

    UA_StatusCode retval = UA_mbedTLS_LoadLocalCertificate(&newCertificate, &securityPolicy->localCertificate);

    if (retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the new private key */
    if(newPrivateKey.length > 0) {
        mbedtls_pk_free(&pc->localPrivateKey);
        mbedtls_pk_init(&pc->localPrivateKey);
        if(UA_mbedTLS_LoadPrivateKey(&newPrivateKey, &pc->localPrivateKey, &pc->entropyContext)) {
            retval = UA_STATUSCODE_BADNOTSUPPORTED;
            goto error;
        }
    } else {
        if(!isLocalKey) {
            mbedtls_pk_free(&pc->localPrivateKey);
            pc->localPrivateKey = pc->csrLocalPrivateKey;
            mbedtls_pk_init(&pc->csrLocalPrivateKey);
        }
    }

    retval = asym_makeThumbprint_basic128rsa15(securityPolicy,
                                                  &securityPolicy->localCertificate,
                                                  &pc->localCertThumbprint);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    return retval;

    error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not update certificate and private key");
    if(securityPolicy->policyContext != NULL)
        clear_basic128rsa15(securityPolicy);
    return retval;
}

static UA_StatusCode
createSigningRequest_basic128rsa15(UA_SecurityPolicy *securityPolicy,
                                       const UA_String *subjectName,
                                       const UA_ByteString *nonce,
                                       const UA_KeyValueMap *params,
                                       UA_ByteString *csr,
                                       UA_ByteString *newPrivateKey) {
    /* Check parameter */
    if (securityPolicy == NULL || csr == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if(securityPolicy->policyContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_PolicyContext *pc =
            (mbedtls_PolicyContext *) securityPolicy->policyContext;

    return mbedtls_createSigningRequest(&pc->localPrivateKey, &pc->csrLocalPrivateKey,
                                        &pc->entropyContext, &pc->drbgContext,
                                        securityPolicy, subjectName, nonce,
                                        csr, newPrivateKey);
}

static UA_StatusCode
policyContext_newContext_basic128rsa15(UA_SecurityPolicy *securityPolicy,
                                       const UA_ByteString localPrivateKey) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(securityPolicy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if (localPrivateKey.length == 0) {
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
    mbedtls_ctr_drbg_init(&pc->drbgContext);
    mbedtls_entropy_init(&pc->entropyContext);
    mbedtls_pk_init(&pc->localPrivateKey);
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

    /* Set the private key */
    mbedErr = UA_mbedTLS_LoadPrivateKey(&localPrivateKey, &pc->localPrivateKey, &pc->entropyContext);

    if(mbedErr) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    /* Set the local certificate thumbprint */
    retval = UA_ByteString_allocBuffer(&pc->localCertThumbprint, UA_SHA1_LENGTH);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;
    retval = asym_makeThumbprint_basic128rsa15(securityPolicy,
                                                  &securityPolicy->localCertificate,
                                                  &pc->localCertThumbprint);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    return UA_STATUSCODE_GOOD;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not create securityContext: %s", UA_StatusCode_name(retval));
    if(securityPolicy->policyContext != NULL)
        clear_basic128rsa15(securityPolicy);
    return retval;
}

UA_StatusCode
UA_SecurityPolicy_Basic128Rsa15(UA_SecurityPolicy *sp,
                                const UA_ByteString localCertificate,
                                const UA_ByteString localPrivateKey,
                                const UA_Logger *logger) {
    UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                   "!! WARNING !! The Basic128Rsa15 SecurityPolicy is unsecure. "
                   "There are known attacks that break the encryption.");

    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = logger;
    sp->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15");
    sp->certificateGroupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    sp->certificateTypeId = UA_NS0ID(RSAMINAPPLICATIONCERTIFICATETYPE);
    sp->securityLevel = 0;
    sp->policyType = UA_SECURITYPOLICYTYPE_RSA;

    /* Asymmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *asymSig = &sp->asymSignatureAlgorithm;
    asymSig->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#rsa-sha1");
    asymSig->verify = asym_verify_basic128rsa15;
    asymSig->sign = asym_sign_basic128rsa15;
    asymSig->getLocalSignatureSize = asym_getLocalSignatureSize_basic128rsa15;
    asymSig->getRemoteSignatureSize = UA_mbedTLS_asym_getRemoteSignatureSize_generic;
    asymSig->getLocalKeyLength = NULL;
    asymSig->getRemoteKeyLength = NULL;

    /* Asymmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#rsa-1_5");
    asymEnc->encrypt = asym_encrypt_basic128rsa15;
    asymEnc->decrypt = asym_decrypt_basic128rsa15;
    asymEnc->getLocalKeyLength = UA_mbedTLS_getRemoteCertificatePrivateKeyLength;
    asymEnc->getRemoteKeyLength = asym_getRemoteEncryptionKeyLength_basic128rsa15;
    asymEnc->getRemoteBlockSize = UA_mbedTLS_getRemoteCertificatePrivateKeyLength;
    asymEnc->getRemotePlainTextBlockSize = asym_getRemotePlainTextBlockSize_basic128rsa15;

    /* Symmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha1\0");
    symSig->verify = sym_verify_basic128rsa15;
    symSig->sign = sym_sign_basic128rsa15;
    symSig->getLocalSignatureSize = sym_getSignatureSize_basic128rsa15;
    symSig->getRemoteSignatureSize = sym_getSignatureSize_basic128rsa15;
    symSig->getLocalKeyLength = sym_getSigningKeyLength_basic128rsa15;
    symSig->getRemoteKeyLength = sym_getSigningKeyLength_basic128rsa15;

    /* Symmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#aes128-cbc");
    symEnc->encrypt = sym_encrypt_basic128rsa15;
    symEnc->decrypt = sym_decrypt_basic128rsa15;
    symEnc->getLocalKeyLength = sym_getEncryptionKeyLength_basic128rsa15;
    symEnc->getRemoteKeyLength = sym_getEncryptionKeyLength_basic128rsa15;
    symEnc->getRemoteBlockSize = sym_getEncryptionBlockSize_basic128rsa15;
    symEnc->getRemotePlainTextBlockSize = sym_getPlainTextBlockSize_basic128rsa15;

    /* Certificate Signing
     * Use the same signature algorithm as the asymmetric component for
     * certificate signing (see standard). */
    sp->certSignatureAlgorithm = sp->asymSignatureAlgorithm;

    /* Direct Method Pointers */
    sp->newChannelContext = newContext_basic128rsa15;
    sp->deleteChannelContext = deleteContext_basic128rsa15;
    sp->setLocalSymEncryptingKey = UA_mbedTLS_setLocalSymEncryptingKey_generic;
    sp->setLocalSymSigningKey = UA_mbedTLS_setLocalSymSigningKey_generic;
    sp->setLocalSymIv = UA_mbedTLS_setLocalSymIv_generic;
    sp->setRemoteSymEncryptingKey = UA_mbedTLS_setRemoteSymEncryptingKey_generic;
    sp->setRemoteSymSigningKey = UA_mbedTLS_setRemoteSymSigningKey_generic;
    sp->setRemoteSymIv = UA_mbedTLS_setRemoteSymIv_generic;
    sp->compareCertificate = UA_mbedTLS_compareCertificate_generic;
    sp->generateKey = sym_generateKey_basic128rsa15;
    sp->generateNonce = sym_generateNonce_basic128rsa15;
    sp->nonceLength = 16;
    sp->makeCertThumbprint = asym_makeThumbprint_basic128rsa15;
    sp->compareCertThumbprint = compareCertificateThumbprint_basic128rsa15;
    sp->updateCertificate = updateCertificateAndPrivateKey_basic128rsa15;
    sp->createSigningRequest = createSigningRequest_basic128rsa15;
    sp->clear = clear_basic128rsa15;

    /* Load the certificate */
    UA_StatusCode res =
        UA_mbedTLS_LoadLocalCertificate(&localCertificate, &sp->localCertificate);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Create the policy context */
    res = policyContext_newContext_basic128rsa15(sp, localPrivateKey);
    if(res != UA_STATUSCODE_GOOD)
        clear_basic128rsa15(sp);

    return res;
}

#endif
