/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Wind River Systems, Inc.
 *    Copyright 2020 (c) basysKom GmbH
 *    Copyright 2022 (c) Wind River Systems, Inc.
 *    Copyright 2022 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

/*
modification history
--------------------
01feb20,lan  written
*/

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)

#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>
#include <openssl/pem.h>

#include "securitypolicy_openssl_common.h"
#include "ua_openssl_version_abstraction.h"

#define SHA1_DIGEST_LENGTH 20          /* 160 bits */
#define RSA_DECRYPT_BUFFER_LENGTH 2048 /* bytes */

/* Cast to prevent warnings in LibreSSL */
#define SHA256EVP() ((EVP_MD *)(uintptr_t)EVP_sha256())


/** P_SHA256 Context */
typedef struct UA_Openssl_P_SHA256_Ctx_ {
    size_t  seedLen;
    size_t  secretLen;
    UA_Byte   A[32]; /* 32 bytes of SHA256 output */
    /*
    char seed[seedLen];
    char secret[secretLen]; */
} UA_Openssl_P_SHA256_Ctx;

#define UA_Openssl_P_SHA256_SEED(ctx)   ((ctx)->A+32)
#define UA_Openssl_P_SHA256_SECRET(ctx) ((ctx)->A+32+(ctx)->seedLen)

/** P_SHA1 Context */
typedef struct UA_Openssl_P_SHA1_Ctx_ {
    size_t  seedLen;
    size_t  secretLen;
    UA_Byte A[SHA1_DIGEST_LENGTH];  /* 20 bytes of SHA1 output */
    /*
    char seed[seedLen];
    char secret[secretLen]; */
} UA_Openssl_P_SHA1_Ctx;

#define UA_Openssl_P_SHA1_SEED(ctx)   ((ctx)->A + SHA1_DIGEST_LENGTH)
#define UA_Openssl_P_SHA1_SECRET(ctx) ((ctx)->A + SHA1_DIGEST_LENGTH +(ctx)->seedLen)

void
UA_Openssl_Init (void) {
    /* VxWorks7 has initialized the openssl. */
#ifndef __VXWORKS__
    static UA_Int16 bInit = 0;
    if (bInit == 1)
        return;
#if defined(OPENSSL_API_COMPAT) && (OPENSSL_API_COMPAT < 0x10100000L)
    /* only needed, if OpenSSL < V1.1 */
    OpenSSL_add_all_algorithms ();
    ERR_load_crypto_strings ();
#endif
    bInit = 1;
#endif
}

static int UA_OpenSSL_RSA_Key_Size (EVP_PKEY * key){
#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
    return EVP_PKEY_get_size (key);
#else
    return RSA_size (get_pkey_rsa(key));
#endif
}

/* UA_copyCertificate - allocalte the buffer, copy the certificate and
 * add a NULL to the end
 */

UA_StatusCode
UA_copyCertificate (UA_ByteString * dst,
                    const UA_ByteString * src) {
    UA_StatusCode retval = UA_ByteString_allocBuffer (dst, src->length + 1);
    if (retval != UA_STATUSCODE_GOOD)
        return retval;
    (void) memcpy (dst->data, src->data, src->length);
    dst->data[dst->length - 1] = '\0';
    dst->length--;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_OpenSSL_RSA_Public_Verify(const UA_ByteString *message,
                             const EVP_MD *evpMd, X509 *publicKeyX509,
                             UA_Int16 padding, const UA_ByteString *signature) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
    if(!mdctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    EVP_PKEY *evpPublicKey = X509_get_pubkey(publicKeyX509);
    if(!evpPublicKey) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    EVP_PKEY_CTX *evpKeyCtx;
    int opensslRet = EVP_DigestVerifyInit(mdctx, &evpKeyCtx, evpMd,
                                          NULL, evpPublicKey);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    opensslRet = EVP_PKEY_CTX_set_rsa_padding(evpKeyCtx, padding);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(padding == RSA_PKCS1_PSS_PADDING) {
        opensslRet = EVP_PKEY_CTX_set_rsa_pss_saltlen(evpKeyCtx, RSA_PSS_SALTLEN_DIGEST);
        if(opensslRet != 1) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
        opensslRet = EVP_PKEY_CTX_set_rsa_mgf1_md(evpKeyCtx, SHA256EVP());
        if(opensslRet != 1) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
    }

    opensslRet = EVP_DigestVerifyUpdate (mdctx, message->data, message->length);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    opensslRet = EVP_DigestVerifyFinal(mdctx, signature->data, signature->length);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

errout:
    if(evpPublicKey)
        EVP_PKEY_free(evpPublicKey);
    if(mdctx)
        EVP_MD_CTX_destroy(mdctx);
    return ret;
}

UA_StatusCode
UA_OpenSSL_RSA_PKCS1_V15_SHA256_Verify (const UA_ByteString * msg,
                                        X509 *                publicKeyX509,
                                        const UA_ByteString * signature
                                       ) {
    return UA_OpenSSL_RSA_Public_Verify (msg, EVP_sha256(), publicKeyX509,
                                         RSA_PKCS1_PADDING, signature);
}

UA_StatusCode
UA_OpenSSL_RSA_PSS_SHA256_Verify (const UA_ByteString * msg,
                                        X509 *                publicKeyX509,
                                        const UA_ByteString * signature
) {
    return UA_OpenSSL_RSA_Public_Verify (msg, EVP_sha256(), publicKeyX509,
                                         RSA_PKCS1_PSS_PADDING, signature);
}

/* Get certificate thumbprint, and allocate the buffer. */

UA_StatusCode
UA_Openssl_X509_GetCertificateThumbprint (const UA_ByteString * certficate,
                                          UA_ByteString *       pThumbprint,
                                          bool                  bThumbPrint) {
    if (bThumbPrint) {
        pThumbprint->length = SHA_DIGEST_LENGTH;
        UA_StatusCode ret = UA_ByteString_allocBuffer (pThumbprint, pThumbprint->length);
        if (ret != UA_STATUSCODE_GOOD) {
            return ret;
            }
    }
    else {
        if (pThumbprint->length != SHA_DIGEST_LENGTH) {
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }
    X509 * x509Certificate = UA_OpenSSL_LoadCertificate(certficate);

    if (x509Certificate == NULL) {
        if (bThumbPrint) {
            UA_ByteString_clear (pThumbprint);
        }
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if (X509_digest (x509Certificate, EVP_sha1(), pThumbprint->data, NULL)
        != 1) {
        if (bThumbPrint) {
            UA_ByteString_clear (pThumbprint);
        }
    return UA_STATUSCODE_BADINTERNALERROR;
    }
    X509_free(x509Certificate);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_Openssl_RSA_Private_Decrypt (UA_ByteString *      data,
                               EVP_PKEY *            privateKey,
                               UA_Int16              padding,
                               UA_Boolean            withSha256) {
    if (data == NULL || privateKey == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if (privateKey == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    size_t keySize = (size_t) UA_OpenSSL_RSA_Key_Size (privateKey);
    size_t cipherOffset = 0;
    size_t outOffset = 0;
    unsigned char buf[RSA_DECRYPT_BUFFER_LENGTH];
    size_t decryptedBytes;
    EVP_PKEY_CTX * ctx;
    int opensslRet;

    ctx = EVP_PKEY_CTX_new (privateKey, NULL);
    if (ctx == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    opensslRet = EVP_PKEY_decrypt_init (ctx);
    if (opensslRet != 1) {
        EVP_PKEY_CTX_free (ctx);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    opensslRet = EVP_PKEY_CTX_set_rsa_padding (ctx, padding);
    if (opensslRet != 1) {
        EVP_PKEY_CTX_free (ctx);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(withSha256) {
        opensslRet = EVP_PKEY_CTX_set_rsa_oaep_md(ctx, SHA256EVP());
        if (opensslRet != 1) {
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        opensslRet = EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, SHA256EVP());
        if (opensslRet != 1) {
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }

    while (cipherOffset < data->length) {
        decryptedBytes = RSA_DECRYPT_BUFFER_LENGTH;
        opensslRet = EVP_PKEY_decrypt (ctx,
                           buf,                       /* where to decrypt */
                           &decryptedBytes,
                           data->data + cipherOffset, /* what to decrypt  */
                           keySize
                           );
        if (opensslRet != 1) {
            EVP_PKEY_CTX_free (ctx);
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        }
        (void) memcpy(data->data + outOffset, buf, decryptedBytes);
        cipherOffset += (size_t) keySize;
        outOffset += decryptedBytes;
    }
    data->length = outOffset;
    EVP_PKEY_CTX_free (ctx);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Openssl_RSA_Oaep_Decrypt (UA_ByteString *       data,
                             EVP_PKEY * privateKey) {
    return  UA_Openssl_RSA_Private_Decrypt (data, privateKey,
                                            RSA_PKCS1_OAEP_PADDING, false);
}

UA_StatusCode
UA_Openssl_RSA_Oaep_Sha2_Decrypt (UA_ByteString *       data,
                             EVP_PKEY * privateKey) {
    return  UA_Openssl_RSA_Private_Decrypt (data, privateKey,
                                            RSA_PKCS1_OAEP_PADDING, true);
}

static UA_StatusCode
UA_Openssl_RSA_Public_Encrypt  (const UA_ByteString * message,
                                X509 *                publicX509,
                                UA_Int16              padding,
                                size_t                paddingSize,
                                UA_ByteString *       encrypted,
                                UA_Boolean withSha256) {
    EVP_PKEY_CTX *   ctx          = NULL;
    EVP_PKEY *       evpPublicKey = NULL;
    int              opensslRet;
    UA_StatusCode    ret;
    size_t encryptedTextLen = 0;
    size_t dataPos =  0;
    size_t encryptedPos = 0;
    size_t bytesToEncrypt = 0;
    size_t encryptedBlockSize = 0;
    size_t keySize = 0;

    evpPublicKey = X509_get_pubkey (publicX509);
    if (evpPublicKey == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }
    ctx = EVP_PKEY_CTX_new (evpPublicKey, NULL);
    if (ctx == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }
    opensslRet = EVP_PKEY_encrypt_init (ctx);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    opensslRet = EVP_PKEY_CTX_set_rsa_padding (ctx, padding);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    if(withSha256) {
        opensslRet = EVP_PKEY_CTX_set_rsa_oaep_md(ctx, SHA256EVP());
        if (opensslRet != 1) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
        opensslRet = EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, SHA256EVP());
        if (opensslRet != 1) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
    }

    /* get the encrypted block size */

    keySize = (size_t) UA_OpenSSL_RSA_Key_Size (evpPublicKey);
    if (keySize == 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    switch (padding) {
        case RSA_PKCS1_OAEP_PADDING:
        case RSA_PKCS1_PADDING:
            if (keySize <= paddingSize) {
                ret = UA_STATUSCODE_BADINTERNALERROR;
                goto errout;
            }
            encryptedBlockSize = keySize - paddingSize;
            break;
        default:
            ret = UA_STATUSCODE_BADNOTSUPPORTED;
            goto errout;
            break;
    }

    /* encrypt in reverse order so that [data] may alias [encrypted] */

    dataPos =  message->length;
    encryptedPos = ((dataPos - 1) / encryptedBlockSize + 1) * keySize;
    bytesToEncrypt = (dataPos - 1) % encryptedBlockSize + 1;
    encryptedTextLen = encryptedPos;

    while (dataPos > 0) {
        size_t outlen = keySize;
        encryptedPos -= keySize;
        dataPos -= bytesToEncrypt;
        opensslRet = EVP_PKEY_encrypt (ctx, encrypted->data + encryptedPos, &outlen,
                                       message->data + dataPos, bytesToEncrypt);

        if (opensslRet != 1) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
        bytesToEncrypt = encryptedBlockSize;
    }
    encrypted->length = encryptedTextLen;

    ret = UA_STATUSCODE_GOOD;
errout:
    if (evpPublicKey != NULL) {
        EVP_PKEY_free (evpPublicKey);
    }
    if (ctx != NULL) {
        EVP_PKEY_CTX_free (ctx);
    }
    return ret;
}

UA_StatusCode
UA_Openssl_RSA_OAEP_Encrypt (UA_ByteString * data,
                             size_t          paddingSize,
                             X509 *          publicX509) {
    UA_ByteString message;
    UA_StatusCode ret;

    ret = UA_ByteString_copy (data, &message);
    if (ret != UA_STATUSCODE_GOOD) {
        return ret;
    }
    ret = UA_Openssl_RSA_Public_Encrypt (&message, publicX509,
                                            RSA_PKCS1_OAEP_PADDING,
                                            paddingSize,
                                            data, false);
    UA_ByteString_clear (&message);
    return ret;
}

UA_StatusCode
UA_Openssl_RSA_OAEP_SHA2_Encrypt (UA_ByteString * data,
                             size_t          paddingSize,
                             X509 *          publicX509) {
    UA_ByteString message;
    UA_StatusCode ret;

    ret = UA_ByteString_copy (data, &message);
    if (ret != UA_STATUSCODE_GOOD) {
        return ret;
    }
    ret = UA_Openssl_RSA_Public_Encrypt (&message, publicX509,
                                         RSA_PKCS1_OAEP_PADDING,
                                         paddingSize,
                                         data, true);
    UA_ByteString_clear (&message);
    return ret;
}

static UA_Openssl_P_SHA256_Ctx *
P_SHA256_Ctx_Create (const UA_ByteString *  secret,
                     const UA_ByteString *  seed) {
    size_t size = (UA_Int32)sizeof (UA_Openssl_P_SHA256_Ctx) + secret->length +
                    seed->length;
    UA_Openssl_P_SHA256_Ctx * ctx = (UA_Openssl_P_SHA256_Ctx *) UA_malloc (size);
    if (ctx == NULL) {
        return NULL;
    }
    ctx->secretLen = secret->length;
    ctx->seedLen = seed->length;
    (void) memcpy (UA_Openssl_P_SHA256_SEED(ctx), seed->data, seed->length);
    (void) memcpy (UA_Openssl_P_SHA256_SECRET(ctx), secret->data, secret->length);
    /* A(0) = seed
       A(n) = HMAC_HASH(secret, A(n-1)) */

    if (HMAC (EVP_sha256(), secret->data, (int) secret->length, seed->data,
        seed->length, ctx->A, NULL) == NULL) {
        UA_free (ctx);
        return NULL;
    }

    return ctx;
}

static UA_StatusCode
P_SHA256_Hash_Generate (UA_Openssl_P_SHA256_Ctx * ctx,
                        UA_Byte *                 pHas
                        ) {
    /* Calculate P_SHA256(n) = HMAC_SHA256(secret, A(n)+seed) */
    if (HMAC (EVP_sha256(),UA_Openssl_P_SHA256_SECRET(ctx), (int) ctx->secretLen,
        ctx->A, sizeof (ctx->A) + ctx->seedLen, pHas, NULL) == NULL) {
            return UA_STATUSCODE_BADINTERNALERROR;
        }

    /* Calculate A(n) = HMAC_SHA256(secret, A(n-1)) */
   if (HMAC (EVP_sha256(),UA_Openssl_P_SHA256_SECRET(ctx), (int) ctx->secretLen,
        ctx->A, sizeof (ctx->A), ctx->A, NULL) == NULL) {
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Openssl_Random_Key_PSHA256_Derive (const UA_ByteString *     secret,
                                      const UA_ByteString *     seed,
                                      UA_ByteString *           out) {
    size_t keyLen = out->length;
    size_t iter   = keyLen/32 + ((keyLen%32)?1:0);
    size_t bufferLen = iter * 32;
    size_t i;
    UA_StatusCode st;

    UA_Byte * pBuffer = (UA_Byte *) UA_malloc (bufferLen);
    if (pBuffer == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_Openssl_P_SHA256_Ctx * ctx = P_SHA256_Ctx_Create (secret, seed);
    if (ctx == NULL) {
        UA_free (pBuffer);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    for (i = 0; i < iter; i++) {
        st = P_SHA256_Hash_Generate (ctx, pBuffer + (i * 32));
        if (st != UA_STATUSCODE_GOOD) {
            UA_free (pBuffer);
            UA_free (ctx);
            return st;
        }
    }

    (void) memcpy (out->data, pBuffer, keyLen);
    UA_free (pBuffer);
    UA_free (ctx);
    return UA_STATUSCODE_GOOD;
}

/* return the key bytes */
UA_StatusCode
UA_Openssl_RSA_Public_GetKeyLength (X509 *     publicKeyX509,
                                    UA_Int32 * keyLen) {
    EVP_PKEY * evpKey = X509_get_pubkey (publicKeyX509);
    if (evpKey == NULL) {
        return  UA_STATUSCODE_BADINTERNALERROR;
    }
    *keyLen = UA_OpenSSL_RSA_Key_Size (evpKey);
    
    EVP_PKEY_free (evpKey);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Openssl_RSA_Private_GetKeyLength (EVP_PKEY * privateKey,
                                     UA_Int32 *            keyLen) {
    if (privateKey == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    *keyLen = UA_OpenSSL_RSA_Key_Size (privateKey);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_Openssl_RSA_Private_Sign (const UA_ByteString * message,
                     EVP_PKEY * privateKey,
                     const EVP_MD *        evpMd,
                     UA_Int16              padding,
                     UA_ByteString *       outSignature) {
    EVP_MD_CTX *     mdctx        = NULL;
    int              opensslRet;
    EVP_PKEY_CTX *   evpKeyCtx;
    UA_StatusCode    ret;

    mdctx = EVP_MD_CTX_create ();
    if (mdctx == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    if (privateKey == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    opensslRet = EVP_DigestSignInit (mdctx, &evpKeyCtx, evpMd, NULL, privateKey);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    EVP_PKEY_CTX_set_rsa_padding (evpKeyCtx, padding);
    if(padding == RSA_PKCS1_PSS_PADDING) {
        opensslRet = EVP_PKEY_CTX_set_rsa_pss_saltlen(evpKeyCtx, RSA_PSS_SALTLEN_DIGEST); //RSA_PSS_SALTLEN_DIGEST
        if (opensslRet != 1) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
        opensslRet = EVP_PKEY_CTX_set_rsa_mgf1_md(evpKeyCtx, SHA256EVP());
        if (opensslRet != 1) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
    }
    opensslRet = EVP_DigestSignUpdate (mdctx, message->data, message->length);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    opensslRet = EVP_DigestSignFinal (mdctx, outSignature->data, &outSignature->length);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    ret = UA_STATUSCODE_GOOD;
errout:
    if (mdctx != NULL) {
        EVP_MD_CTX_destroy (mdctx);
    }
    return ret;
}

UA_StatusCode
UA_Openssl_RSA_PKCS1_V15_SHA256_Sign (const UA_ByteString * message,
                                      EVP_PKEY * privateKey,
                                      UA_ByteString *       outSignature) {
    return UA_Openssl_RSA_Private_Sign (message, privateKey, EVP_sha256(),
                                        RSA_PKCS1_PADDING , outSignature);
}

UA_StatusCode
UA_Openssl_RSA_PSS_SHA256_Sign (const UA_ByteString * message,
                                      EVP_PKEY * privateKey,
                                      UA_ByteString *       outSignature) {
    return UA_Openssl_RSA_Private_Sign (message, privateKey, EVP_sha256(),
                                        RSA_PKCS1_PSS_PADDING, outSignature);
}

UA_StatusCode
UA_OpenSSL_HMAC_SHA256_Verify (const UA_ByteString *     message,
                               const UA_ByteString *     key,
                               const UA_ByteString *     signature
                              ) {
    unsigned char buf[SHA256_DIGEST_LENGTH] = {0};
    UA_ByteString mac = {SHA256_DIGEST_LENGTH, buf};

    if (HMAC (EVP_sha256(), key->data, (int) key->length, message->data, message->length,
              mac.data, (unsigned int *) &mac.length) == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if (UA_ByteString_equal (signature, &mac)) {
        return UA_STATUSCODE_GOOD;
    }
    else {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
}

UA_StatusCode
UA_OpenSSL_HMAC_SHA256_Sign (const UA_ByteString *     message,
                             const UA_ByteString *     key,
                             UA_ByteString *           signature
                             ) {
    if (HMAC (EVP_sha256(), key->data, (int) key->length, message->data,
              message->length,
              signature->data, (unsigned int *) &(signature->length)) == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_OpenSSL_Decrypt (const UA_ByteString * iv,
                    const UA_ByteString * key,
                    const EVP_CIPHER *    cipherAlg,
                    UA_ByteString *       data  /* [in/out]*/) {
    UA_ByteString    ivCopy    = {0, NULL};
    UA_ByteString    cipherTxt = {0, NULL};
    EVP_CIPHER_CTX * ctx       = NULL;
    UA_StatusCode    ret;
    int              opensslRet;
    int              outLen;
    int              tmpLen;

    /* copy the IV because the AES_cbc_encrypt function overwrites it. */

    ret = UA_ByteString_copy (iv, &ivCopy);
    if (ret != UA_STATUSCODE_GOOD) {
        goto errout;
    }

    ret = UA_ByteString_copy (data, &cipherTxt);
    if (ret != UA_STATUSCODE_GOOD) {
        goto errout;
    }

    ctx = EVP_CIPHER_CTX_new ();
    if (ctx == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    /* call EVP_* to decrypt */

    opensslRet = EVP_DecryptInit_ex (ctx, cipherAlg, NULL, key->data, ivCopy.data);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    /* EVP_DecryptFinal() will return an error code if padding is enabled
     * and the final block is not correctly formatted.
     */
    EVP_CIPHER_CTX_set_padding (ctx, 0);
    opensslRet = EVP_DecryptUpdate (ctx, data->data, &outLen,
                                    cipherTxt.data, (int) cipherTxt.length);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    opensslRet = EVP_DecryptFinal_ex (ctx, data->data + outLen, &tmpLen);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    outLen += tmpLen;
    data->length = (size_t) outLen;
    ret = UA_STATUSCODE_GOOD;

errout:
    UA_ByteString_clear (&ivCopy);
    UA_ByteString_clear (&cipherTxt);
    if (ctx != NULL) {
        EVP_CIPHER_CTX_free(ctx);
    }
    return ret;
}

static UA_StatusCode
UA_OpenSSL_Encrypt (const UA_ByteString * iv,
                    const UA_ByteString * key,
                    const EVP_CIPHER *    cipherAlg,
                    UA_ByteString *       data  /* [in/out]*/
                    ) {

    UA_ByteString    ivCopy   = {0, NULL};
    UA_ByteString    plainTxt = {0, NULL};
    EVP_CIPHER_CTX * ctx      = NULL;
    UA_StatusCode    ret;
    int              opensslRet;
    int              outLen;
    int              tmpLen;

    /* copy the IV because the AES_cbc_encrypt function overwrites it. */

    ret = UA_ByteString_copy (iv, &ivCopy);
    if (ret != UA_STATUSCODE_GOOD) {
        goto errout;
    }

    ret = UA_ByteString_copy (data, &plainTxt);
    if (ret != UA_STATUSCODE_GOOD) {
        goto errout;
    }

    ctx = EVP_CIPHER_CTX_new ();
    if (ctx == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    /* call EVP_* to encrypt */

    opensslRet = EVP_EncryptInit_ex (ctx, cipherAlg, NULL, key->data, ivCopy.data);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Disable padding. Padding is done in the stack before calling encryption.
     * Ensure that we have a multiple of the block size */
    if(data->length % (size_t)EVP_CIPHER_CTX_block_size(ctx)) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    opensslRet = EVP_CIPHER_CTX_set_padding(ctx, 0);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Encrypt the data */
    opensslRet = EVP_EncryptUpdate (ctx, data->data, &outLen,
                                    plainTxt.data, (int) plainTxt.length);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Encrypt-final does nothing as padding is disabled */
    opensslRet = EVP_EncryptFinal_ex(ctx, data->data + outLen, &tmpLen);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    outLen += tmpLen;
    data->length = (size_t) outLen;
    ret = UA_STATUSCODE_GOOD;

errout:
    UA_ByteString_clear (&ivCopy);
    UA_ByteString_clear (&plainTxt);
    if (ctx != NULL) {
        EVP_CIPHER_CTX_free(ctx);
    }
    return ret;
}

UA_StatusCode
UA_OpenSSL_AES_256_CBC_Decrypt (const UA_ByteString * iv,
                                const UA_ByteString * key,
                                UA_ByteString *       data  /* [in/out]*/
                                ) {
    return UA_OpenSSL_Decrypt (iv, key, EVP_aes_256_cbc (), data);
}

UA_StatusCode
UA_OpenSSL_AES_256_CBC_Encrypt (const UA_ByteString * iv,
                            const UA_ByteString * key,
                            UA_ByteString *       data  /* [in/out]*/
                            ) {
    return UA_OpenSSL_Encrypt (iv, key, EVP_aes_256_cbc (), data);
}

UA_StatusCode
UA_OpenSSL_X509_compare (const UA_ByteString * cert,
                         const X509 *          bcert) {
    X509 * acert = UA_OpenSSL_LoadCertificate(cert);
    if (acert == NULL) {
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    }
    int opensslRet = X509_cmp (acert, bcert);
    X509_free (acert);

    if (opensslRet == 0)
        return UA_STATUSCODE_GOOD;
    return UA_STATUSCODE_UNCERTAINSUBNORMAL;
}

UA_StatusCode
UA_OpenSSL_RSA_PKCS1_V15_SHA1_Verify (const UA_ByteString * msg,
                                      X509 *                publicKeyX509,
                                      const UA_ByteString * signature) {
    return UA_OpenSSL_RSA_Public_Verify(msg, EVP_sha1(), publicKeyX509,
                                        RSA_PKCS1_PADDING, signature);
}

UA_StatusCode
UA_Openssl_RSA_PKCS1_V15_SHA1_Sign (const UA_ByteString * message,
                                    EVP_PKEY * privateKey,
                                    UA_ByteString *       outSignature) {
    return UA_Openssl_RSA_Private_Sign(message, privateKey, EVP_sha1(),
                                       RSA_PKCS1_PADDING, outSignature);
}

static UA_Openssl_P_SHA1_Ctx *
P_SHA1_Ctx_Create (const UA_ByteString *  secret,
                   const UA_ByteString *  seed) {
    size_t size = (UA_Int32)sizeof (UA_Openssl_P_SHA1_Ctx) + secret->length +
                    seed->length;
    UA_Openssl_P_SHA1_Ctx * ctx = (UA_Openssl_P_SHA1_Ctx *) UA_malloc (size);
    if (ctx == NULL) {
        return NULL;
    }

    ctx->secretLen = secret->length;
    ctx->seedLen = seed->length;
    (void) memcpy (UA_Openssl_P_SHA1_SEED(ctx), seed->data, seed->length);
    (void) memcpy (UA_Openssl_P_SHA1_SECRET(ctx), secret->data, secret->length);
    /* A(0) = seed
       A(n) = HMAC_HASH(secret, A(n-1)) */

    if (HMAC (EVP_sha1(), secret->data, (int) secret->length, seed->data,
        seed->length, ctx->A, NULL) == NULL) {
        UA_free (ctx);
        return NULL;
    }

    return ctx;
}

static UA_StatusCode
P_SHA1_Hash_Generate (UA_Openssl_P_SHA1_Ctx * ctx,
                      UA_Byte *               pHas
                      ) {
    /* Calculate P_SHA1(n) = HMAC_SHA1(secret, A(n)+seed) */
    if (HMAC (EVP_sha1 (), UA_Openssl_P_SHA1_SECRET(ctx), (int) ctx->secretLen,
        ctx->A, sizeof (ctx->A) + ctx->seedLen, pHas, NULL) == NULL) {
            return UA_STATUSCODE_BADINTERNALERROR;
        }

    /* Calculate A(n) = HMAC_SHA1(secret, A(n-1)) */
   if (HMAC (EVP_sha1(), UA_Openssl_P_SHA1_SECRET(ctx), (int) ctx->secretLen,
        ctx->A, sizeof (ctx->A), ctx->A, NULL) == NULL) {
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Openssl_Random_Key_PSHA1_Derive (const UA_ByteString *     secret,
                                   const UA_ByteString *     seed,
                                   UA_ByteString *           out) {
    size_t keyLen     = out->length;
    size_t iter       = keyLen / SHA1_DIGEST_LENGTH + ((keyLen % SHA1_DIGEST_LENGTH)?1:0);
    size_t bufferLen  = iter * SHA1_DIGEST_LENGTH;
    UA_Byte * pBuffer = (UA_Byte *) UA_malloc (bufferLen);
    if (pBuffer == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_Openssl_P_SHA1_Ctx * ctx = P_SHA1_Ctx_Create (secret, seed);
    if (ctx == NULL) {
        UA_free (pBuffer);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    size_t i;
    UA_StatusCode st;

    for (i = 0; i < iter; i++) {
        st = P_SHA1_Hash_Generate (ctx, pBuffer + (i * SHA1_DIGEST_LENGTH));
        if (st != UA_STATUSCODE_GOOD) {
            UA_free (pBuffer);
            UA_free (ctx);
            return st;
        }
    }

    (void) memcpy (out->data, pBuffer, keyLen);
    UA_free (pBuffer);
    UA_free (ctx);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_OpenSSL_HMAC_SHA1_Verify (const UA_ByteString *     message,
                             const UA_ByteString *     key,
                             const UA_ByteString *     signature
                             ) {
    unsigned char buf[SHA1_DIGEST_LENGTH] = {0};
    UA_ByteString mac = {SHA1_DIGEST_LENGTH, buf};

    if(HMAC (EVP_sha1(), key->data, (int) key->length, message->data, message->length,
             mac.data, (unsigned int *) &mac.length) == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if (UA_ByteString_equal (signature, &mac)) {
        return UA_STATUSCODE_GOOD;
    }
    else {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
}

UA_StatusCode
UA_OpenSSL_HMAC_SHA1_Sign (const UA_ByteString *     message,
                           const UA_ByteString *     key,
                           UA_ByteString *           signature
                           ) {
    if (HMAC (EVP_sha1(), key->data, (int) key->length, message->data,
              message->length,
              signature->data, (unsigned int *) &(signature->length)) == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Openssl_RSA_PKCS1_V15_Decrypt (UA_ByteString *       data,
                                  EVP_PKEY * privateKey) {
    return  UA_Openssl_RSA_Private_Decrypt (data, privateKey,
                                            RSA_PKCS1_PADDING, false);
}

UA_StatusCode
UA_Openssl_RSA_PKCS1_V15_Encrypt (UA_ByteString * data,
                                  size_t          paddingSize,
                                  X509 *          publicX509) {
    UA_ByteString message;
    UA_StatusCode ret = UA_ByteString_copy (data, &message);
    if (ret != UA_STATUSCODE_GOOD) {
        return ret;
    }
    ret = UA_Openssl_RSA_Public_Encrypt (&message, publicX509,
                                         RSA_PKCS1_PADDING,
                                         paddingSize,
                                         data, false);
    UA_ByteString_clear (&message);
    return ret;
}

UA_StatusCode
UA_OpenSSL_AES_128_CBC_Decrypt (const UA_ByteString * iv,
                                const UA_ByteString * key,
                                UA_ByteString *       data  /* [in/out]*/
                                ) {
    return UA_OpenSSL_Decrypt (iv, key, EVP_aes_128_cbc (), data);
}

UA_StatusCode
UA_OpenSSL_AES_128_CBC_Encrypt (const UA_ByteString * iv,
                                const UA_ByteString * key,
                                UA_ByteString *       data  /* [in/out]*/
                                ) {
    return UA_OpenSSL_Encrypt (iv, key, EVP_aes_128_cbc (), data);
}

EVP_PKEY *
UA_OpenSSL_LoadPrivateKey(const UA_ByteString *privateKey) {
    const unsigned char * pkData = privateKey->data;
    long len = (long) privateKey->length;
    if(len == 0)
        return NULL;

    EVP_PKEY *result = NULL;

    if (len > 1 && pkData[0] == 0x30 && pkData[1] == 0x82) { // Magic number for DER encoded keys
        result = d2i_PrivateKey(EVP_PKEY_RSA, NULL,
                                          &pkData, len);
    } else {
        BIO *bio = NULL;
        bio = BIO_new_mem_buf((void *) privateKey->data, (int) privateKey->length);
        result = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
        BIO_free(bio);
    }

    return result;
}

X509 *
UA_OpenSSL_LoadCertificate(const UA_ByteString *certificate) {
    X509 * result = NULL;
    const unsigned char *pData = certificate->data;

    if (certificate->length > 1 && pData[0] == 0x30 && pData[1] == 0x82) { // Magic number for DER encoded files
        result = UA_OpenSSL_LoadDerCertificate(certificate);
    } else {
        result = UA_OpenSSL_LoadPemCertificate(certificate);
    }

    return result;
}

X509 *
UA_OpenSSL_LoadDerCertificate(const UA_ByteString *certificate) {
    const unsigned char *pData = certificate->data;
    return d2i_X509(NULL, &pData, (long) certificate->length);
}

X509 *
UA_OpenSSL_LoadPemCertificate(const UA_ByteString *certificate) {
    X509 * result = NULL;

    BIO* bio = NULL;
    bio = BIO_new_mem_buf((void *) certificate->data, (int) certificate->length);
    result = PEM_read_bio_X509(bio, NULL, NULL, NULL);
    BIO_free(bio);

    return result;
}

UA_StatusCode
UA_OpenSSL_LoadLocalCertificate(const UA_ByteString *certificate, UA_ByteString *target) {
    X509 *cert = UA_OpenSSL_LoadCertificate(certificate);

    if (!cert) {
        UA_ByteString_init(target);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    unsigned char *derData = NULL;
    int length = i2d_X509(cert, &derData);
    X509_free(cert);

    if (length > 0) {
        UA_ByteString temp;
        temp.length = (size_t) length;
        temp.data = derData;
        UA_ByteString_copy(&temp, target);
        OPENSSL_free(derData);
        return UA_STATUSCODE_GOOD;
    } else {
        UA_ByteString_init(target);
    }

    return UA_STATUSCODE_BADINVALIDARGUMENT;
}

#endif
