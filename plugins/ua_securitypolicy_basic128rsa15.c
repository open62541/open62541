/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#include "ua_config.h"

#ifdef UA_ENABLE_ENCRYPTION

#include <mbedtls/aes.h>
#include <mbedtls/md.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/entropy_poll.h>
#include <mbedtls/error.h>
#include <mbedtls/version.h>
#include <mbedtls/sha1.h>

#include "ua_plugin_pki.h"
#include "ua_plugin_securitypolicy.h"
#include "ua_securitypolicy_basic128rsa15.h"
#include "ua_types.h"
#include "ua_types_generated_handling.h"

/* Notes:
 * mbedTLS' AES allows in-place encryption and decryption. Sow we don't have to
 * allocate temp buffers.
 * https://tls.mbed.org/discussions/generic/in-place-decryption-with-aes256-same-input-output-buffer
 */

#define UA_SECURITYPOLICY_BASIC128RSA15_RSAPADDING_LEN 11
#define UA_SHA1_LENGTH 20
#define UA_SECURITYPOLICY_BASIC128RSA15_SYM_KEY_LENGTH 16
#define UA_BASIC128RSA15_SYM_SIGNING_KEY_LENGTH 16
#define UA_SECURITYPOLICY_BASIC128RSA15_SYM_ENCRYPTION_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_BASIC128RSA15_SYM_PLAIN_TEXT_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_BASIC128RSA15_MINASYMKEYLENGTH 128
#define UA_SECURITYPOLICY_BASIC128RSA15_MAXASYMKEYLENGTH 256

#define UA_LOG_MBEDERR                                                  \
    char errBuff[300];                                                  \
    mbedtls_strerror(mbedErr, errBuff, 300);                            \
    UA_LOG_WARNING(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY, \
                   "mbedTLS returned an error: %s", errBuff);           \

#define UA_MBEDTLS_ERRORHANDLING(errorcode)                             \
    if(mbedErr) {                                                       \
        UA_LOG_MBEDERR                                                  \
        retval = errorcode;                                             \
    }

#define UA_MBEDTLS_ERRORHANDLING_RETURN(errorcode)                      \
    if(mbedErr) {                                                       \
        UA_LOG_MBEDERR                                                  \
        return errorcode;                                               \
    }

typedef struct {
    const UA_SecurityPolicy *securityPolicy;
    UA_ByteString localCertThumbprint;

    mbedtls_ctr_drbg_context drbgContext;
    mbedtls_entropy_context entropyContext;
    mbedtls_md_context_t sha1MdContext;
    mbedtls_pk_context localPrivateKey;
} Basic128Rsa15_PolicyContext;

typedef struct {
    Basic128Rsa15_PolicyContext *policyContext;

    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;

    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    mbedtls_x509_crt remoteCertificate;
} Basic128Rsa15_ChannelContext;


/********************/
/* AsymmetricModule */
/********************/

static UA_StatusCode
asym_verify_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                             Basic128Rsa15_ChannelContext *cc,
                             const UA_ByteString *message,
                             const UA_ByteString *signature) {
    if(securityPolicy == NULL || message == NULL || signature == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Compute the sha1 hash */
    unsigned char hash[UA_SHA1_LENGTH];
#if MBEDTLS_VERSION_NUMBER >= 0x02070000
    mbedtls_sha1_ret(message->data, message->length, hash);
#else
    mbedtls_sha1(message->data, message->length, hash);
#endif

    /* Set the RSA settings */
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V15, 0);

    /* Verify */
    int mbedErr = mbedtls_pk_verify(&cc->remoteCertificate.pk,
                                    MBEDTLS_MD_SHA1, hash, UA_SHA1_LENGTH,
                                    signature->data, signature->length);
    UA_MBEDTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_sign_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                           Basic128Rsa15_ChannelContext *cc,
                           const UA_ByteString *message,
                           UA_ByteString *signature) {
    if(securityPolicy == NULL || message == NULL || signature == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    unsigned char hash[UA_SHA1_LENGTH];
#if MBEDTLS_VERSION_NUMBER >= 0x02070000
    mbedtls_sha1_ret(message->data, message->length, hash);
#else
    mbedtls_sha1(message->data, message->length, hash);
#endif

    Basic128Rsa15_PolicyContext *pc = cc->policyContext;
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(pc->localPrivateKey);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V15, 0);

    size_t sigLen = 0;
    int mbedErr = mbedtls_pk_sign(&pc->localPrivateKey,
                                  MBEDTLS_MD_SHA1, hash,
                                  UA_SHA1_LENGTH, signature->data,
                                  &sigLen, mbedtls_ctr_drbg_random,
                                  &pc->drbgContext);
    UA_MBEDTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADINTERNALERROR);
    return UA_STATUSCODE_GOOD;
}

static size_t
asym_getLocalSignatureSize_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                            const Basic128Rsa15_ChannelContext *cc) {
    if(securityPolicy == NULL || cc == NULL)
        return 0;

    return mbedtls_pk_rsa(cc->policyContext->localPrivateKey)->len;
}

static size_t
asym_getRemoteSignatureSize_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                             const Basic128Rsa15_ChannelContext *cc) {
    if(securityPolicy == NULL || cc == NULL)
        return 0;

    return mbedtls_pk_rsa(cc->remoteCertificate.pk)->len;
}

static UA_StatusCode
asym_encrypt_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                              Basic128Rsa15_ChannelContext *cc,
                              UA_ByteString *data) {
    if(securityPolicy == NULL || cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const size_t plainTextBlockSize = securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm.
        getRemotePlainTextBlockSize(securityPolicy, cc);

    if(data->length % plainTextBlockSize != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_rsa_context *remoteRsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    mbedtls_rsa_set_padding(remoteRsaContext, MBEDTLS_RSA_PKCS_V15, 0);

    UA_ByteString encrypted;
    const size_t bufferOverhead =
        UA_SecurityPolicy_getRemoteAsymEncryptionBufferLengthOverhead(securityPolicy, cc, data->length);
    UA_StatusCode retval = UA_ByteString_allocBuffer(&encrypted, data->length + bufferOverhead);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    size_t lenDataToEncrypt = data->length;
    size_t inOffset = 0;
    size_t offset = 0;
    size_t outLength = 0;
    Basic128Rsa15_PolicyContext *pc = cc->policyContext;
    while(lenDataToEncrypt >= plainTextBlockSize) {
        int mbedErr = mbedtls_pk_encrypt(&cc->remoteCertificate.pk,
                                         data->data + inOffset, plainTextBlockSize,
                                         encrypted.data + offset, &outLength,
                                         encrypted.length - offset,
                                         mbedtls_ctr_drbg_random,
                                         &pc->drbgContext);
        UA_MBEDTLS_ERRORHANDLING(UA_STATUSCODE_BADINTERNALERROR);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_ByteString_deleteMembers(&encrypted);
            return retval;
        }

        inOffset += plainTextBlockSize;
        offset += outLength;
        lenDataToEncrypt -= plainTextBlockSize;
    }

    memcpy(data->data, encrypted.data, offset);
    UA_ByteString_deleteMembers(&encrypted);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_decrypt_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                              Basic128Rsa15_ChannelContext *cc,
                              UA_ByteString *data) {
    if(securityPolicy == NULL || cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_rsa_context *rsaContext =
        mbedtls_pk_rsa(cc->policyContext->localPrivateKey);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V15, 0);

    if(data->length % rsaContext->len != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString decrypted;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&decrypted, data->length);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    size_t lenDataToDecrypt = data->length;
    size_t inOffset = 0;
    size_t offset = 0;
    size_t outLength = 0;
    while(lenDataToDecrypt >= rsaContext->len) {
        int mbedErr = mbedtls_pk_decrypt(&cc->policyContext->localPrivateKey,
                                         data->data + inOffset, rsaContext->len,
                                         decrypted.data + offset, &outLength,
                                         decrypted.length - offset, NULL, NULL);
        if(mbedErr)
            UA_ByteString_deleteMembers(&decrypted); // TODO: Maybe change error macro to jump to cleanup?
        UA_MBEDTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

        inOffset += rsaContext->len;
        offset += outLength;
        lenDataToDecrypt -= rsaContext->len;
    }

    if(lenDataToDecrypt == 0) {
        memcpy(data->data, decrypted.data, offset);
        data->length = offset;
    } else {
        retval = UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_ByteString_deleteMembers(&decrypted);
    return retval;
}

static size_t
asym_getRemoteEncryptionKeyLength_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                                   const Basic128Rsa15_ChannelContext *cc) {
    return mbedtls_pk_get_len(&cc->remoteCertificate.pk) * 8;
}

static size_t
asym_getRemoteBlockSize_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                         const Basic128Rsa15_ChannelContext *cc) {
    mbedtls_rsa_context *const rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    return rsaContext->len;
}

static size_t
asym_getRemotePlainTextBlockSize_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                                  const Basic128Rsa15_ChannelContext *cc) {
    mbedtls_rsa_context *const rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    return rsaContext->len - UA_SECURITYPOLICY_BASIC128RSA15_RSAPADDING_LEN;
}

static UA_StatusCode
asym_makeThumbprint_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                     const UA_ByteString *certificate,
                                     UA_ByteString *thumbprint) {
    if(securityPolicy == NULL || certificate == NULL || thumbprint == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(UA_ByteString_equal(certificate, &UA_BYTESTRING_NULL))
        return UA_STATUSCODE_BADINTERNALERROR;

    if(thumbprint->length != UA_SHA1_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;

#if MBEDTLS_VERSION_NUMBER >= 0x02070000
    mbedtls_sha1_ret(certificate->data, certificate->length, thumbprint->data);
#else
    mbedtls_sha1(certificate->data, certificate->length, thumbprint->data);
#endif
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asymmetricModule_compareCertificateThumbprint_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                                               const UA_ByteString *certificateThumbprint) {
    if(securityPolicy == NULL || certificateThumbprint == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Basic128Rsa15_PolicyContext *pc = (Basic128Rsa15_PolicyContext *)securityPolicy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint))
        return UA_STATUSCODE_BADCERTIFICATEINVALID;

    return UA_STATUSCODE_GOOD;
}

/*******************/
/* SymmetricModule */
/*******************/

static void
md_hmac(mbedtls_md_context_t *context, const UA_ByteString *key,
        const UA_ByteString *in, unsigned char out[20]) {
    mbedtls_md_hmac_starts(context, key->data, key->length);
    mbedtls_md_hmac_update(context, in->data, in->length);
    mbedtls_md_hmac_finish(context, out);
}

static UA_StatusCode
sym_verify_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                            Basic128Rsa15_ChannelContext *cc,
                            const UA_ByteString *message,
                            const UA_ByteString *signature) {
    if(securityPolicy == NULL || cc == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Compute MAC */
    if(signature->length != UA_SHA1_LENGTH) {
        UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Signature size does not have the desired size defined by the security policy");
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    Basic128Rsa15_PolicyContext *pc =
        (Basic128Rsa15_PolicyContext *)securityPolicy->policyContext;

    unsigned char mac[UA_SHA1_LENGTH];
    md_hmac(&pc->sha1MdContext, &cc->remoteSymSigningKey, message, mac);

    /* Compare with Signature */
    if(memcmp(signature->data, mac, UA_SHA1_LENGTH) != 0)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_sign_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                          const Basic128Rsa15_ChannelContext *cc,
                          const UA_ByteString *message,
                          UA_ByteString *signature) {
    if(signature->length != UA_SHA1_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;

    md_hmac(&cc->policyContext->sha1MdContext, &cc->localSymSigningKey,
            message, signature->data);
    return UA_STATUSCODE_GOOD;
}

static size_t
sym_getSignatureSize_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                      const void *channelContext) {
    return UA_SHA1_LENGTH;
}

static size_t
sym_getSigningKeyLength_sp_basic128rsa15(const UA_SecurityPolicy *const securityPolicy,
                                         const void *const channelContext) {
    return UA_BASIC128RSA15_SYM_SIGNING_KEY_LENGTH;
}

static size_t
sym_getEncryptionKeyLength_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                            const void *channelContext) {
    return UA_SECURITYPOLICY_BASIC128RSA15_SYM_KEY_LENGTH;
}

static size_t
sym_getEncryptionBlockSize_sp_basic128rsa15(const UA_SecurityPolicy *const securityPolicy,
                                            const void *const channelContext) {
    return UA_SECURITYPOLICY_BASIC128RSA15_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
sym_getPlainTextBlockSize_sp_basic128rsa15(const UA_SecurityPolicy *const securityPolicy,
                                           const void *const channelContext) {
    return UA_SECURITYPOLICY_BASIC128RSA15_SYM_PLAIN_TEXT_BLOCK_SIZE;
}

static UA_StatusCode
sym_encrypt_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                             const Basic128Rsa15_ChannelContext *cc,
                             UA_ByteString *data) {
    if(securityPolicy == NULL || cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(cc->localSymIv.length !=
       securityPolicy->symmetricModule.cryptoModule.encryptionAlgorithm.getLocalBlockSize(securityPolicy, cc))
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t plainTextBlockSize =
        securityPolicy->symmetricModule.cryptoModule.encryptionAlgorithm.getLocalPlainTextBlockSize(securityPolicy, cc);

    if(data->length % plainTextBlockSize != 0) {
        UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Length of data to encrypt is not a multiple of the plain text block size."
                         "Padding might not have been calculated appropriately.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Keylength in bits */
    unsigned int keylength = (unsigned int)(cc->localSymEncryptingKey.length * 8);
    mbedtls_aes_context aesContext;
    int mbedErr = mbedtls_aes_setkey_enc(&aesContext, cc->localSymEncryptingKey.data, keylength);
    UA_MBEDTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADINTERNALERROR);

    UA_ByteString ivCopy;
    UA_StatusCode retval = UA_ByteString_copy(&cc->localSymIv, &ivCopy);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    mbedErr = mbedtls_aes_crypt_cbc(&aesContext, MBEDTLS_AES_ENCRYPT, data->length,
                                    ivCopy.data, data->data, data->data);
    UA_MBEDTLS_ERRORHANDLING(UA_STATUSCODE_BADINTERNALERROR);
    UA_ByteString_deleteMembers(&ivCopy);
    return retval;
}

static UA_StatusCode
sym_decrypt_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                             const Basic128Rsa15_ChannelContext *cc,
                             UA_ByteString *data) {
    if(securityPolicy == NULL || cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t encryptionBlockSize =
        securityPolicy->symmetricModule.cryptoModule.encryptionAlgorithm.getLocalBlockSize(securityPolicy, cc);

    if(cc->remoteSymIv.length != encryptionBlockSize)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(data->length % encryptionBlockSize != 0) {
        UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Length of data to decrypt is not a multiple of the encryptingBlock size.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    unsigned int keylength = (unsigned int)(cc->remoteSymEncryptingKey.length * 8);
    mbedtls_aes_context aesContext;
    int mbedErr = mbedtls_aes_setkey_dec(&aesContext, cc->remoteSymEncryptingKey.data, keylength);
    UA_MBEDTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADINTERNALERROR);

    UA_ByteString ivCopy;
    UA_StatusCode retval = UA_ByteString_copy(&cc->remoteSymIv, &ivCopy);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    mbedErr = mbedtls_aes_crypt_cbc(&aesContext, MBEDTLS_AES_DECRYPT, data->length,
                                    ivCopy.data, data->data, data->data);
    UA_MBEDTLS_ERRORHANDLING(UA_STATUSCODE_BADINTERNALERROR);
    UA_ByteString_deleteMembers(&ivCopy);
    return retval;
}

static void
swapBuffers(UA_ByteString *const bufA, UA_ByteString *const bufB) {
    UA_ByteString tmp = *bufA;
    *bufA = *bufB;
    *bufB = tmp;
}

static UA_StatusCode
sym_generateKey_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                 const UA_ByteString *secret, const UA_ByteString *seed,
                                 UA_ByteString *out) {
    if(securityPolicy == NULL || secret == NULL || seed == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Basic128Rsa15_PolicyContext *pc =
        (Basic128Rsa15_PolicyContext *)securityPolicy->policyContext;

    size_t hashLen = 0;
    const mbedtls_md_info_t *mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    hashLen = (size_t)mbedtls_md_get_size(mdInfo);

    UA_ByteString A_and_seed;
    UA_ByteString_allocBuffer(&A_and_seed, hashLen + seed->length);
    memcpy(A_and_seed.data + hashLen, seed->data, seed->length);

    UA_ByteString ANext_and_seed;
    UA_ByteString_allocBuffer(&ANext_and_seed, hashLen + seed->length);
    memcpy(ANext_and_seed.data + hashLen, seed->data, seed->length);

    UA_ByteString A = {
        hashLen,
        A_and_seed.data
    };

    UA_ByteString ANext = {
        hashLen,
        ANext_and_seed.data
    };

    md_hmac(&pc->sha1MdContext, secret, seed, A.data);

    UA_StatusCode retval = 0;
    for(size_t offset = 0; offset < out->length; offset += hashLen) {
        UA_ByteString outSegment = {
            hashLen,
            out->data + offset
        };
        UA_Boolean bufferAllocated = UA_FALSE;
        // Not enough room in out buffer to write the hash.
        if(offset + hashLen > out->length) {
            outSegment.data = NULL;
            outSegment.length = 0;
            retval |= UA_ByteString_allocBuffer(&outSegment, hashLen);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_ByteString_deleteMembers(&A_and_seed);
                UA_ByteString_deleteMembers(&ANext_and_seed);
                return retval;
            }
            bufferAllocated = UA_TRUE;
        }

        md_hmac(&pc->sha1MdContext, secret, &A_and_seed, outSegment.data);
        md_hmac(&pc->sha1MdContext, secret, &A, ANext.data);

        if(retval != UA_STATUSCODE_GOOD) {
            if(bufferAllocated)
                UA_ByteString_deleteMembers(&outSegment);
            UA_ByteString_deleteMembers(&A_and_seed);
            UA_ByteString_deleteMembers(&ANext_and_seed);
            return retval;
        }

        if(bufferAllocated) {
            memcpy(out->data + offset, outSegment.data, out->length - offset);
            UA_ByteString_deleteMembers(&outSegment);
        }

        swapBuffers(&ANext_and_seed, &A_and_seed);
        swapBuffers(&ANext, &A);
    }

    UA_ByteString_deleteMembers(&A_and_seed);
    UA_ByteString_deleteMembers(&ANext_and_seed);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_generateNonce_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                   UA_ByteString *out) {
    if(securityPolicy == NULL || securityPolicy->policyContext == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Basic128Rsa15_PolicyContext *data =
        (Basic128Rsa15_PolicyContext *)securityPolicy->policyContext;

    int mbedErr = mbedtls_ctr_drbg_random(&data->drbgContext, out->data, out->length);
    UA_MBEDTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADUNEXPECTEDERROR);

    return UA_STATUSCODE_GOOD;
}

/*****************/
/* ChannelModule */
/*****************/

/* Assumes that the certificate has been verified externally */
static UA_StatusCode
parseRemoteCertificate_sp_basic128rsa15(Basic128Rsa15_ChannelContext *cc,
                                        const UA_ByteString *remoteCertificate) {
    if(remoteCertificate == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const UA_SecurityPolicy *securityPolicy = cc->policyContext->securityPolicy;

    /* Parse the certificate */
    int mbedErr = mbedtls_x509_crt_parse(&cc->remoteCertificate, remoteCertificate->data,
                                         remoteCertificate->length);
    UA_MBEDTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    /* Check the key length */
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    if(rsaContext->len < UA_SECURITYPOLICY_BASIC128RSA15_MINASYMKEYLENGTH ||
       rsaContext->len > UA_SECURITYPOLICY_BASIC128RSA15_MAXASYMKEYLENGTH)
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;

    return UA_STATUSCODE_GOOD;
}

static void
channelContext_deleteContext_sp_basic128rsa15(Basic128Rsa15_ChannelContext *cc) {
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
channelContext_newContext_sp_basic128rsa15(const UA_SecurityPolicy *securityPolicy,
                                           const UA_ByteString *remoteCertificate,
                                           void **pp_contextData) {
    if(securityPolicy == NULL || remoteCertificate == NULL || pp_contextData == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the channel context */
    *pp_contextData = UA_malloc(sizeof(Basic128Rsa15_ChannelContext));
    if(*pp_contextData == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    Basic128Rsa15_ChannelContext *cc = (Basic128Rsa15_ChannelContext *)*pp_contextData;

    /* Initialize the channel context */
    cc->policyContext = (Basic128Rsa15_PolicyContext *)securityPolicy->policyContext;

    UA_ByteString_init(&cc->localSymSigningKey);
    UA_ByteString_init(&cc->localSymEncryptingKey);
    UA_ByteString_init(&cc->localSymIv);

    UA_ByteString_init(&cc->remoteSymSigningKey);
    UA_ByteString_init(&cc->remoteSymEncryptingKey);
    UA_ByteString_init(&cc->remoteSymIv);

    mbedtls_x509_crt_init(&cc->remoteCertificate);

    // TODO: this can be optimized so that we dont allocate memory before parsing the certificate
    UA_StatusCode retval = parseRemoteCertificate_sp_basic128rsa15(cc, remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        channelContext_deleteContext_sp_basic128rsa15(cc);
        *pp_contextData = NULL;
    }
    return retval;
}

static UA_StatusCode
channelContext_setLocalSymEncryptingKey_sp_basic128rsa15(Basic128Rsa15_ChannelContext *cc,
                                                         const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_deleteMembers(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

static UA_StatusCode
channelContext_setLocalSymSigningKey_sp_basic128rsa15(Basic128Rsa15_ChannelContext *cc,
                                                      const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_deleteMembers(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}


static UA_StatusCode
channelContext_setLocalSymIv_sp_basic128rsa15(Basic128Rsa15_ChannelContext *cc,
                                              const UA_ByteString *iv) {
    if(iv == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_deleteMembers(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

static UA_StatusCode
channelContext_setRemoteSymEncryptingKey_sp_basic128rsa15(Basic128Rsa15_ChannelContext *cc,
                                                          const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_deleteMembers(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

static UA_StatusCode
channelContext_setRemoteSymSigningKey_sp_basic128rsa15(Basic128Rsa15_ChannelContext *cc,
                                                       const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_deleteMembers(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

static UA_StatusCode
channelContext_setRemoteSymIv_sp_basic128rsa15(Basic128Rsa15_ChannelContext *cc,
                                               const UA_ByteString *iv) {
    if(iv == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_deleteMembers(&cc->remoteSymIv);
    return UA_ByteString_copy(iv, &cc->remoteSymIv);
}

static UA_StatusCode
channelContext_compareCertificate_sp_basic128rsa15(const Basic128Rsa15_ChannelContext *cc,
                                                   const UA_ByteString *certificate) {
    if(cc == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const UA_SecurityPolicy *securityPolicy = cc->policyContext->securityPolicy;

    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    int mbedErr = mbedtls_x509_crt_parse(&cert, certificate->data, certificate->length);
    if(mbedErr) {
        UA_LOG_MBEDERR;
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(cert.raw.len != cc->remoteCertificate.raw.len ||
       memcmp(cert.raw.p, cc->remoteCertificate.raw.p, cert.raw.len) != 0)
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    mbedtls_x509_crt_free(&cert);
    return retval;
}

static void
deleteMembers_sp_basic128rsa15(UA_SecurityPolicy *securityPolicy) {
    if(securityPolicy == NULL)
        return;

    if(securityPolicy->policyContext == NULL)
        return;

    UA_ByteString_deleteMembers(&securityPolicy->localCertificate);

    /* delete all allocated members in the context */
    Basic128Rsa15_PolicyContext *pc = (Basic128Rsa15_PolicyContext *)
        securityPolicy->policyContext;

    mbedtls_ctr_drbg_free(&pc->drbgContext);
    mbedtls_entropy_free(&pc->entropyContext);
    mbedtls_pk_free(&pc->localPrivateKey);
    mbedtls_md_free(&pc->sha1MdContext);
    UA_ByteString_deleteMembers(&pc->localCertThumbprint);

    UA_LOG_DEBUG(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Deleted members of EndpointContext for sp_basic128rsa15");

    UA_free(pc);
    securityPolicy->policyContext = NULL;
}

static UA_StatusCode
policyContext_newContext_sp_basic128rsa15(UA_SecurityPolicy *securityPolicy,
                                          const UA_ByteString localPrivateKey) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(securityPolicy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Basic128Rsa15_PolicyContext *pc = (Basic128Rsa15_PolicyContext *)
        UA_malloc(sizeof(Basic128Rsa15_PolicyContext));
    securityPolicy->policyContext = (void *)pc;
    if(!pc) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    /* Initialize the PolicyContext */
    memset(pc, 0, sizeof(Basic128Rsa15_PolicyContext));
    mbedtls_ctr_drbg_init(&pc->drbgContext);
    mbedtls_entropy_init(&pc->entropyContext);
    mbedtls_pk_init(&pc->localPrivateKey);
    mbedtls_md_init(&pc->sha1MdContext);
    pc->securityPolicy = securityPolicy;

    /* Initialized the message digest */
    const mbedtls_md_info_t *const mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    int mbedErr = mbedtls_md_setup(&pc->sha1MdContext, mdInfo, MBEDTLS_MD_SHA1);
    UA_MBEDTLS_ERRORHANDLING(UA_STATUSCODE_BADOUTOFMEMORY);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    /* Add the system entropy source */
    mbedErr = mbedtls_entropy_add_source(&pc->entropyContext,
                                         mbedtls_platform_entropy_poll, NULL, 0,
                                         MBEDTLS_ENTROPY_SOURCE_STRONG);
    UA_MBEDTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    /* Seed the RNG */
    char *personalization = "open62541-drbg";
    mbedErr = mbedtls_ctr_drbg_seed(&pc->drbgContext, mbedtls_entropy_func,
                                    &pc->entropyContext,
                                    (const unsigned char *)personalization, 14);
    UA_MBEDTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    /* Set the private key */
    mbedErr = mbedtls_pk_parse_key(&pc->localPrivateKey,
                                   localPrivateKey.data, localPrivateKey.length,
                                   NULL, 0);
    UA_MBEDTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    /* Set the local certificate thumbprint */
    retval = UA_ByteString_allocBuffer(&pc->localCertThumbprint, UA_SHA1_LENGTH);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;
    retval = asym_makeThumbprint_sp_basic128rsa15(pc->securityPolicy,
                                                  &securityPolicy->localCertificate,
                                                  &pc->localCertThumbprint);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    return UA_STATUSCODE_GOOD;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not create securityContext");
    if(securityPolicy->policyContext != NULL)
        deleteMembers_sp_basic128rsa15(securityPolicy);
    return retval;
}

UA_StatusCode
UA_SecurityPolicy_Basic128Rsa15(UA_SecurityPolicy *policy, UA_CertificateVerification *certificateVerification,
                                const UA_ByteString localCertificate, const UA_ByteString localPrivateKey,
                                UA_Logger logger) {
    memset(policy, 0, sizeof(UA_SecurityPolicy));
    policy->logger = logger;

    policy->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15");

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
    asym_signatureAlgorithm->uri =
        UA_STRING("http://www.w3.org/2000/09/xmldsig#rsa-sha1\0");
    asym_signatureAlgorithm->verify =
        (UA_StatusCode (*)(const UA_SecurityPolicy *, void *,
                           const UA_ByteString *, const UA_ByteString *))asym_verify_sp_basic128rsa15;
    asym_signatureAlgorithm->sign =
        (UA_StatusCode (*)(const UA_SecurityPolicy *, void *,
                           const UA_ByteString *, UA_ByteString *))asym_sign_sp_basic128rsa15;
    asym_signatureAlgorithm->getLocalSignatureSize =
        (size_t (*)(const UA_SecurityPolicy *, const void *))asym_getLocalSignatureSize_sp_basic128rsa15;
    asym_signatureAlgorithm->getRemoteSignatureSize =
        (size_t (*)(const UA_SecurityPolicy *, const void *))asym_getRemoteSignatureSize_sp_basic128rsa15;
    asym_signatureAlgorithm->getLocalKeyLength = NULL; // TODO: Write function
    asym_signatureAlgorithm->getRemoteKeyLength = NULL; // TODO: Write function

    UA_SecurityPolicyEncryptionAlgorithm *asym_encryptionAlgorithm =
        &asymmetricModule->cryptoModule.encryptionAlgorithm;
    asym_encryptionAlgorithm->uri = UA_STRING("TODO: ALG URI");
    asym_encryptionAlgorithm->encrypt =
        (UA_StatusCode(*)(const UA_SecurityPolicy *, void *, UA_ByteString *))asym_encrypt_sp_basic128rsa15;
    asym_encryptionAlgorithm->decrypt =
        (UA_StatusCode(*)(const UA_SecurityPolicy *, void *, UA_ByteString *))
            asym_decrypt_sp_basic128rsa15;
    asym_encryptionAlgorithm->getLocalKeyLength = NULL; // TODO: Write function
    asym_encryptionAlgorithm->getRemoteKeyLength =
        (size_t (*)(const UA_SecurityPolicy *, const void *))asym_getRemoteEncryptionKeyLength_sp_basic128rsa15;
    asym_encryptionAlgorithm->getLocalBlockSize = NULL; // TODO: Write function
    asym_encryptionAlgorithm->getRemoteBlockSize = (size_t (*)(const UA_SecurityPolicy *,
                                                               const void *))asym_getRemoteBlockSize_sp_basic128rsa15;
    asym_encryptionAlgorithm->getLocalPlainTextBlockSize = NULL; // TODO: Write function
    asym_encryptionAlgorithm->getRemotePlainTextBlockSize =
        (size_t (*)(const UA_SecurityPolicy *, const void *))asym_getRemotePlainTextBlockSize_sp_basic128rsa15;

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
        (UA_StatusCode (*)(const UA_SecurityPolicy *, void *, const UA_ByteString *,
                           const UA_ByteString *))sym_verify_sp_basic128rsa15;
    sym_signatureAlgorithm->sign =
        (UA_StatusCode (*)(const UA_SecurityPolicy *, void *,
                           const UA_ByteString *, UA_ByteString *))sym_sign_sp_basic128rsa15;
    sym_signatureAlgorithm->getLocalSignatureSize = sym_getSignatureSize_sp_basic128rsa15;
    sym_signatureAlgorithm->getRemoteSignatureSize = sym_getSignatureSize_sp_basic128rsa15;
    sym_signatureAlgorithm->getLocalKeyLength =
        (size_t (*)(const UA_SecurityPolicy *,
                    const void *))sym_getSigningKeyLength_sp_basic128rsa15;
    sym_signatureAlgorithm->getRemoteKeyLength =
        (size_t (*)(const UA_SecurityPolicy *,
                    const void *))sym_getSigningKeyLength_sp_basic128rsa15;

    UA_SecurityPolicyEncryptionAlgorithm *sym_encryptionAlgorithm =
        &symmetricModule->cryptoModule.encryptionAlgorithm;
    sym_encryptionAlgorithm->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#aes128-cbc");
    sym_encryptionAlgorithm->encrypt =
        (UA_StatusCode(*)(const UA_SecurityPolicy *, void *, UA_ByteString *))sym_encrypt_sp_basic128rsa15;
    sym_encryptionAlgorithm->decrypt =
        (UA_StatusCode(*)(const UA_SecurityPolicy *, void *, UA_ByteString *))sym_decrypt_sp_basic128rsa15;
    sym_encryptionAlgorithm->getLocalKeyLength = sym_getEncryptionKeyLength_sp_basic128rsa15;
    sym_encryptionAlgorithm->getRemoteKeyLength = sym_getEncryptionKeyLength_sp_basic128rsa15;
    sym_encryptionAlgorithm->getLocalBlockSize =
        (size_t (*)(const UA_SecurityPolicy *, const void *))sym_getEncryptionBlockSize_sp_basic128rsa15;
    sym_encryptionAlgorithm->getRemoteBlockSize =
        (size_t (*)(const UA_SecurityPolicy *, const void *))sym_getEncryptionBlockSize_sp_basic128rsa15;
    sym_encryptionAlgorithm->getLocalPlainTextBlockSize =
        (size_t (*)(const UA_SecurityPolicy *, const void *))sym_getPlainTextBlockSize_sp_basic128rsa15;
    sym_encryptionAlgorithm->getRemotePlainTextBlockSize =
        (size_t (*)(const UA_SecurityPolicy *, const void *))sym_getPlainTextBlockSize_sp_basic128rsa15;
    symmetricModule->secureChannelNonceLength = 16;

    // Use the same signature algorithm as the asymmetric component for certificate signing (see standard)
    policy->certificateSigningAlgorithm = policy->asymmetricModule.cryptoModule.signatureAlgorithm;

    /* ChannelModule */
    channelModule->newContext = channelContext_newContext_sp_basic128rsa15;
    channelModule->deleteContext = (void (*)(void *))
        channelContext_deleteContext_sp_basic128rsa15;

    channelModule->setLocalSymEncryptingKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setLocalSymEncryptingKey_sp_basic128rsa15;
    channelModule->setLocalSymSigningKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setLocalSymSigningKey_sp_basic128rsa15;
    channelModule->setLocalSymIv = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setLocalSymIv_sp_basic128rsa15;

    channelModule->setRemoteSymEncryptingKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setRemoteSymEncryptingKey_sp_basic128rsa15;
    channelModule->setRemoteSymSigningKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setRemoteSymSigningKey_sp_basic128rsa15;
    channelModule->setRemoteSymIv = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setRemoteSymIv_sp_basic128rsa15;

    channelModule->compareCertificate = (UA_StatusCode (*)(const void *, const UA_ByteString *))
        channelContext_compareCertificate_sp_basic128rsa15;

    policy->deleteMembers = deleteMembers_sp_basic128rsa15;

    return policyContext_newContext_sp_basic128rsa15(policy, localPrivateKey);
}

#endif /* UA_ENABLE_ENCRYPTION */
