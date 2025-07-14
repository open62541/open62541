/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Wind River Systems, Inc.
 *    Copyright 2020 (c) basysKom GmbH
 *    Copyright 2022 (c) Wind River Systems, Inc.
 *    Copyright 2022 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2024 (c) Siemens AG (Authors: Tin Raic, Thomas Zeschg)
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
#include <openssl/x509v3.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/ecdsa.h>
#include <openssl/kdf.h>

#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
#include <openssl/core_names.h>
#endif

#include "securitypolicy_common.h"

#define SHA1_DIGEST_LENGTH 20          /* 160 bits */
#define RSA_DECRYPT_BUFFER_LENGTH 2048 /* bytes */

/* Strings for ECC policies */
UA_String serverLabel = UA_STRING_STATIC("opcua-server");
UA_String clientLabel = UA_STRING_STATIC("opcua-client");

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

static UA_StatusCode
UA_OpenSSL_X509_AddSubjectAttributes(const UA_String* subject, X509_NAME* name) {
    char *subj = (char *)UA_malloc(subject->length + 1);
    if(!subj)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    memset(subj, 0x00, subject->length + 1);
    strncpy(subj, (char *)subject->data, subject->length);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* split string into tokens */
    char *token = strtok(subj, "/,");
    while(token != NULL) {
        /* find delimiter in attribute */
        size_t delim = 0;
        for(size_t idx = 0; idx < strlen(token); idx++) {
            if(token[idx] == '=') {
                delim = idx;
                break;
            }
        }
        if(delim == 0 || delim == strlen(token)-1) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }

        token[delim] = '\0';
        const unsigned char *para = (const unsigned char*)&token[delim+1];

        /* add attribute to X509_NAME */
        int result = X509_NAME_add_entry_by_txt(name, token, MBSTRING_UTF8, para, -1, -1, 0);
        if(!result) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }

        /* get next token */
        token = strtok(NULL, "/,");
    }

cleanup:
    UA_free(subj);
    return retval;
}

static UA_StatusCode
UA_OpenSSL_writePrivateKeyDer(EVP_PKEY *key, UA_ByteString *outPrivateKey) {
    unsigned char *p = NULL;
    const int len = i2d_PrivateKey(key, NULL);
    if(len <= 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(UA_ByteString_allocBuffer(outPrivateKey, len) != UA_STATUSCODE_GOOD) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    outPrivateKey->length = len;
    memset(outPrivateKey->data, 0, outPrivateKey->length);
    p = outPrivateKey->data;
    if(i2d_PrivateKey(key, &p) <= 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_OpenSSL_CreateSigningRequest(EVP_PKEY *localPrivateKey,
                                EVP_PKEY **csrLocalPrivateKey,
                                UA_SecurityPolicy *securityPolicy,
                                const UA_String *subjectName,
                                const UA_ByteString *nonce,
                                UA_ByteString *csr,
                                UA_ByteString *newPrivateKey) {
    /* Check parameter */
    if(!securityPolicy || !csr || !localPrivateKey) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    EVP_PKEY_free(*csrLocalPrivateKey);
    /* CSR has already been generated and private key only needs to be set
     * if a new one has been generated. */
    if(newPrivateKey && newPrivateKey->length > 0) {
        /* Set the private key */
        *csrLocalPrivateKey = UA_OpenSSL_LoadPrivateKey(newPrivateKey);
        if(!(*csrLocalPrivateKey))
            return UA_STATUSCODE_BADINTERNALERROR;

        return UA_STATUSCODE_GOOD;
    }

    if(csr && csr->length > 0)
        return UA_STATUSCODE_GOOD;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Get X509 certificate */
    X509 *x509Certificate = UA_OpenSSL_LoadCertificate(&securityPolicy->localCertificate);
    if(!x509Certificate)
        return UA_STATUSCODE_BADCERTIFICATEINVALID;

    /* Create X509 certificate request */
    X509_REQ *request = X509_REQ_new();
    if(request == NULL) {
        X509_free(x509Certificate);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Set version in X509 certificate request */
    if(X509_REQ_set_version(request, 0) != 1) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    /* For request extensions they are all packed in a single attribute.
     * We save them in a STACK and add them all at once later. */
    STACK_OF(X509_EXTENSION)*exts = sk_X509_EXTENSION_new_null();
    if(!exts) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    /* Set key usage in CSR context */
    X509_EXTENSION *key_usage_ext = NULL;
    key_usage_ext = X509V3_EXT_conf_nid(NULL, NULL, NID_key_usage, "digitalSignature,nonRepudiation,keyEncipherment,dataEncipherment");
    if(!key_usage_ext) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        sk_X509_EXTENSION_free(exts);
        goto cleanup;
    }
    sk_X509_EXTENSION_push(exts, key_usage_ext);

    /* Add entropy */
    if(nonce && nonce->length > 0) {
        RAND_seed(nonce->data, nonce->length);
    }

    /* Get subject alternate name field from certificate */
    X509_EXTENSION *subject_alt_name_ext = NULL;
    int pos = X509_get_ext_by_NID(x509Certificate, NID_subject_alt_name, -1);
    if(pos >= 0) {
        subject_alt_name_ext = X509_get_ext(x509Certificate, pos);
        if(subject_alt_name_ext) {
            /* Set subject alternate name in CSR context */
            sk_X509_EXTENSION_push(exts, subject_alt_name_ext);
        }
    }

    /* Now we've created the extensions we add them to the request */
    X509_REQ_add_extensions(request, exts);
    sk_X509_EXTENSION_free(exts);
    X509_EXTENSION_free(key_usage_ext);

    /* Get subject from argument or read it from certificate */
    X509_NAME *name = NULL;
    if(subjectName && subjectName->length > 0) {
        name = X509_NAME_new();
        if(!name) {
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        /* add subject attributes to name */
        if(UA_OpenSSL_X509_AddSubjectAttributes(subjectName, name) != UA_STATUSCODE_GOOD) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            X509_NAME_free(name);
            goto cleanup;
        }
    } else {
        /* Get subject name from certificate */
        X509_NAME *tmpName = X509_get_subject_name(x509Certificate);
        if(!tmpName) {
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        name = X509_NAME_dup(tmpName);
    }

    /* Set the subject in CSR context */
    if(!X509_REQ_set_subject_name(request, name)) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        X509_NAME_free(name);
        goto cleanup;
    }
    X509_NAME_free(name);

    if(newPrivateKey) {
        size_t keySize = 0;
        UA_CertificateUtils_getKeySize(&securityPolicy->localCertificate, &keySize);
#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
        *csrLocalPrivateKey = EVP_RSA_gen(keySize);
        if(!*csrLocalPrivateKey) {
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
#else
        BIGNUM *exponent = BN_new();
        *csrLocalPrivateKey = EVP_PKEY_new();
        RSA *rsa = RSA_new();
        if(!*csrLocalPrivateKey || !exponent || !rsa) {
            if(*csrLocalPrivateKey)
                EVP_PKEY_free(*csrLocalPrivateKey);
            if(exponent)
                BN_free(exponent);
            if(rsa)
                RSA_free(rsa);
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }

        if(BN_set_word(exponent, RSA_F4) != 1) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            EVP_PKEY_free(*csrLocalPrivateKey);
            BN_free(exponent);
            RSA_free(rsa);
            goto cleanup;
        }

        if(RSA_generate_key_ex(rsa, (int) keySize, exponent, NULL) != 1) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            EVP_PKEY_free(*csrLocalPrivateKey);
            BN_free(exponent);
            RSA_free(rsa);
            goto cleanup;
        }

        if(EVP_PKEY_assign_RSA(*csrLocalPrivateKey, rsa) != 1) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            EVP_PKEY_free(*csrLocalPrivateKey);
            BN_free(exponent);
            RSA_free(rsa);
            goto cleanup;
        }
        BN_free(exponent);
        /* rsa will be freed by pkey */
        rsa = NULL;

#endif  /* end of OPENSSL_VERSION_NUMBER >= 0x30000000L */
        if(UA_OpenSSL_writePrivateKeyDer(*csrLocalPrivateKey, newPrivateKey) != UA_STATUSCODE_GOOD) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            EVP_PKEY_free(*csrLocalPrivateKey);
            goto cleanup;
        }

        if(!X509_REQ_set_pubkey(request, *csrLocalPrivateKey)) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            EVP_PKEY_free(*csrLocalPrivateKey);
            goto cleanup;
        }

        if(!X509_REQ_sign(request, *csrLocalPrivateKey, EVP_sha256())) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            EVP_PKEY_free(*csrLocalPrivateKey);
            goto cleanup;
        }
    } else {
        /* Set public key in CSR context */
        EVP_PKEY *pubkey = X509_get_pubkey(x509Certificate);
        if(!pubkey) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
        if(!X509_REQ_set_pubkey(request, pubkey)) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            EVP_PKEY_free(pubkey);
            goto cleanup;
        }
        if(!X509_REQ_sign(request, localPrivateKey, EVP_sha256())) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            EVP_PKEY_free(pubkey);
            goto cleanup;
        }
        EVP_PKEY_free(pubkey);
    }

    /* Determine necessary length for CSR buffer */
    const int csrBufferLength = i2d_X509_REQ(request, 0);
    if(csrBufferLength < 0) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* create CSR buffer */
    UA_ByteString_init(csr);
    UA_ByteString_allocBuffer(csr, csrBufferLength);

    /* Create CSR buffer (DER format) */
    char *ptr = (char*)csr->data;
    i2d_X509_REQ(request, (unsigned char**)&ptr);

cleanup:
    X509_free(x509Certificate);
    X509_REQ_free(request);

    return retval;
}

EVP_PKEY *
UA_OpenSSL_LoadPrivateKey(const UA_ByteString *privateKey) {
    if(privateKey->length == 0)
        return NULL;

    EVP_PKEY *result = NULL;
    BIO *bio = NULL;

    bio = BIO_new_mem_buf((void *) privateKey->data, (int) privateKey->length);
    /* Try to read DER encoded private key */
    result = d2i_PrivateKey_bio(bio, NULL);

    if (result == NULL) {
        /* Try to read PEM encoded private key */
        BIO_reset(bio);
        result = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
    }
    BIO_free(bio);

    return result;
}

X509 *
UA_OpenSSL_LoadCertificate(const UA_ByteString *certificate) {
    X509 * result = NULL;

    /* Try to decode DER encoded certificate */
    result = UA_OpenSSL_LoadDerCertificate(certificate);

    if (result == NULL) {
        /* Try to decode PEM encoded certificate */
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

X509_CRL *
UA_OpenSSL_LoadCrl(const UA_ByteString *crl) {
    X509_CRL * result = NULL;
    const unsigned char *pData = crl->data;

    if (crl->length > 1 && pData[0] == 0x30 && pData[1] == 0x82) { // Magic number for DER encoded files
        result = UA_OpenSSL_LoadDerCrl(crl);
    } else {
        result = UA_OpenSSL_LoadPemCrl(crl);
    }

    return result;
}

X509_CRL *
UA_OpenSSL_LoadDerCrl(const UA_ByteString *crl) {
    const unsigned char *pData = crl->data;
    return d2i_X509_CRL(NULL, &pData, (long) crl->length);
}

X509_CRL *
UA_OpenSSL_LoadPemCrl(const UA_ByteString *crl) {
    X509_CRL * result = NULL;

    BIO* bio = NULL;
    bio = BIO_new_mem_buf((void *) crl->data, (int) crl->length);
    result = PEM_read_bio_X509_CRL(bio, NULL, NULL, NULL);
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

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)
static UA_StatusCode
UA_OpenSSL_ECDH (const int nid,
                 EVP_PKEY * keyPairLocal,
                 const UA_ByteString * keyPublicRemote,
                 UA_ByteString * sharedSecretOut) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    EVP_PKEY_CTX * pctx = NULL;
    EVP_PKEY_CTX * kctx = NULL;
    EVP_PKEY * remotePubKey = NULL;
    EVP_PKEY * params = NULL;
    UA_ByteString keyPublicRemoteEncoded = UA_BYTESTRING_NULL;

    /* We need one additional byte of memory for the encoding */
    if (UA_STATUSCODE_GOOD != UA_ByteString_allocBuffer(&keyPublicRemoteEncoded, keyPublicRemote->length+1)) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    /* Set the encoding to 0x04 (uncompressed) and copy the public key */
    keyPublicRemoteEncoded.data[0] = 0x04;
    memcpy(&keyPublicRemoteEncoded.data[1], keyPublicRemote->data, keyPublicRemote->length);

    remotePubKey = EVP_PKEY_new();
    if(remotePubKey == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    if (pctx == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    if(EVP_PKEY_paramgen_init(pctx) != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, nid) != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(EVP_PKEY_paramgen(pctx, &params) != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(EVP_PKEY_copy_parameters(remotePubKey, params) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
    if(EVP_PKEY_set1_encoded_public_key(remotePubKey, keyPublicRemoteEncoded.data, keyPublicRemoteEncoded.length) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
#else
    if(EVP_PKEY_set1_tls_encodedpoint(remotePubKey, keyPublicRemoteEncoded.data, keyPublicRemoteEncoded.length) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
#endif

    kctx = EVP_PKEY_CTX_new(keyPairLocal, NULL);
    if(kctx == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(EVP_PKEY_derive_init(kctx) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(EVP_PKEY_derive_set_peer(kctx, remotePubKey) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(EVP_PKEY_derive(kctx, NULL, &sharedSecretOut->length) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(UA_STATUSCODE_GOOD != UA_ByteString_allocBuffer(sharedSecretOut, sharedSecretOut->length)) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    if(EVP_PKEY_derive(kctx, sharedSecretOut->data, &sharedSecretOut->length) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

errout:
    UA_ByteString_clear(&keyPublicRemoteEncoded);
    EVP_PKEY_free(remotePubKey);
    EVP_PKEY_CTX_free(pctx);
    EVP_PKEY_CTX_free(kctx);
    EVP_PKEY_free(params);

    return ret;
}

static UA_StatusCode
UA_OpenSSL_ECC_GenerateSalt (const size_t L,
                             const UA_ByteString * label,
                             const UA_ByteString * key1,
                             const UA_ByteString * key2,
                             UA_ByteString * salt) {
    size_t saltLen = sizeof(uint16_t) + label->length + key1->length + key2->length;

    if (salt == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if (UA_STATUSCODE_GOOD != UA_ByteString_allocBuffer(salt, saltLen)) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    salt->data[0] = (L) & 0xFF;
    salt->data[1] = (L >> 8) & 0xFF;

    UA_Byte * saltPtr = &salt->data[2];

    memcpy(saltPtr, label->data, label->length);
    saltPtr += label->length;
    memcpy(saltPtr, key1->data, key1->length);
    saltPtr += key1->length;
    memcpy(saltPtr, key2->data, key2->length);

    return UA_STATUSCODE_GOOD;
}

#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
static UA_StatusCode
UA_OpenSSL_HKDF (char * hashAlgorithm,
                 const UA_ByteString * ikm,
                 const UA_ByteString * salt,
                 const UA_ByteString * info,
                 UA_ByteString * out) {

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    EVP_KDF * kdf = NULL;
    EVP_KDF_CTX * kctx = NULL;
    OSSL_PARAM params[5];
    OSSL_PARAM * p = params;

    kdf = EVP_KDF_fetch(NULL, "HKDF", NULL);
    if(kdf == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    kctx = EVP_KDF_CTX_new(kdf);
    if(kctx == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    *p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, hashAlgorithm, strlen(hashAlgorithm));
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, ikm->data, ikm->length);
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, info->data, info->length);
    *p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, salt->data, salt->length);
    *p = OSSL_PARAM_construct_end();

    if(EVP_KDF_derive(kctx, out->data, out->length, params) <= 0) {
        ERR_print_errors_fp(stdout);
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

errout:
    EVP_KDF_free(kdf);
    EVP_KDF_CTX_free(kctx);

    return ret;
}
#else

static UA_StatusCode
UA_OpenSSL_HKDF (char * hashAlgorithm,
                 const UA_ByteString * ikm,
                 const UA_ByteString * salt,
                 const UA_ByteString * info,
                 UA_ByteString * out) {

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    EVP_PKEY_CTX *pctx = NULL;
    const EVP_MD *md = NULL;

    /* Retrieve the digest pointer */
    md = EVP_get_digestbyname(hashAlgorithm);
    if (md == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, NULL);
    if (EVP_PKEY_derive_init(pctx) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    if (EVP_PKEY_CTX_set_hkdf_md(pctx, md) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    if (EVP_PKEY_CTX_set1_hkdf_salt(pctx, salt->data, salt->length) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    if (EVP_PKEY_CTX_set1_hkdf_key(pctx, ikm->data, ikm->length) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    if (EVP_PKEY_CTX_add1_hkdf_info(pctx, info->data, info->length) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    if (EVP_PKEY_derive(pctx, out->data, &out->length) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
errout:
    EVP_PKEY_CTX_free(pctx);

    return ret;
}
#endif

UA_StatusCode
UA_OpenSSL_ECC_DeriveKeys (const int curveID,
                           char * hashAlgorithm,
                           const UA_ApplicationType applicationType,
                           EVP_PKEY * localEphemeralKeyPair,
                           const UA_ByteString * key1,
                           const UA_ByteString * key2,
                           UA_ByteString * out) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_ByteString sharedSecret = UA_BYTESTRING_NULL;
    UA_ByteString salt = UA_BYTESTRING_NULL;

    /* The order of ephemeral public keys (key1 and key2) tells us whether we
     * need to generate the local keys or the remote keys. To figure that out,
     * we compare the public part of localEphemeralKeyPair with key1 and key2. */
    UA_Byte * keyPubEnc = NULL;
    size_t keyPubEncSize = 0;

    /* Get the local ephemeral public key to use in comparison */
#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
    keyPubEncSize = EVP_PKEY_get1_encoded_public_key(localEphemeralKeyPair, &keyPubEnc);
#else
    keyPubEncSize = EVP_PKEY_get1_tls_encodedpoint(localEphemeralKeyPair, &keyPubEnc);
#endif
    if (keyPubEncSize <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Determine the label for salt generation, remote ephemeral public key for ECDH, and 
    info for HKDF */
    UA_ByteString* label = NULL;
    const UA_ByteString * remoteEphPubKey = NULL;

    /* Comparing from the second byte since the first byte has 0x04 from the encoding */
    if (memcmp(&keyPubEnc[1], key1->data, key1->length) == 0) {
        /* Key 1 is local ephemeral public key => generating remote keys */
        remoteEphPubKey = key2;
        if(applicationType == UA_APPLICATIONTYPE_SERVER) {
            label = &clientLabel;
        } else {
            label = &serverLabel;
        }
    }
    else if (memcmp(&keyPubEnc[1], key2->data, key2->length) == 0) {
        /* Key 2 is local ephemeral public key => generating local keys */
        remoteEphPubKey = key1;
        if(applicationType == UA_APPLICATIONTYPE_SERVER) {
            label = &serverLabel;
        } else {
            label = &clientLabel;
        }
    }
    else {
        /* invalid */
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Use ECDH to calculate shared secret */
    if (UA_STATUSCODE_GOOD != UA_OpenSSL_ECDH(curveID, localEphemeralKeyPair, remoteEphPubKey, &sharedSecret)) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Calculate salt */
    /* The order of the (ephemeral public) keys (key1, key2) is reversed because the caller sends 
    [remote, local] for local key computation and [local, remote] for remote key computation.
    According to 6.8.1., the local salt computation appends the keys in order [local | remote] and the
    remote salt computation [remote | local]. Therefore, no additional logic is required, reversing the 
    order is sufficient. */
    if (UA_STATUSCODE_GOOD != UA_OpenSSL_ECC_GenerateSalt(out->length, label, key2, key1, &salt)) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Call HKDF to derive keys */
    /* Salt is given as the info argument (check 6.8.1., tables 66 and 67) */
    if (UA_STATUSCODE_GOOD != UA_OpenSSL_HKDF(hashAlgorithm, &sharedSecret, &salt, &salt, out)) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

errout:
    OPENSSL_free(keyPubEnc);
    UA_ByteString_clear(&sharedSecret);
    UA_ByteString_clear(&salt);

    return ret;
}

static UA_StatusCode
UA_OpenSSL_ECC_GenerateKey(const int curveId,
                           EVP_PKEY ** keyPairOut,
                           UA_ByteString * keyPublicEncOut) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    EVP_PKEY_CTX * pctx = NULL;
    EVP_PKEY_CTX * kctx = NULL;
    EVP_PKEY * params = NULL;
    size_t keyPubEncSize = 0;
    UA_Byte * keyPubEnc = NULL;

    pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    if (pctx == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    if(EVP_PKEY_paramgen_init(pctx) != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, curveId) != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if (EVP_PKEY_paramgen(pctx, &params) != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    kctx = EVP_PKEY_CTX_new(params, NULL);
    if (kctx == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if (EVP_PKEY_keygen_init(kctx) != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if (EVP_PKEY_keygen(kctx, keyPairOut) != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
    keyPubEncSize = EVP_PKEY_get1_encoded_public_key(*keyPairOut, &keyPubEnc);
#else
    keyPubEncSize = EVP_PKEY_get1_tls_encodedpoint(*keyPairOut, &keyPubEnc);
#endif
    if (keyPubEncSize <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Omit the first byte (encoding) */
    memcpy(keyPublicEncOut->data, &keyPubEnc[1], keyPubEncSize-1);

errout:
    EVP_PKEY_CTX_free(pctx);
    EVP_PKEY_free(params);
    EVP_PKEY_CTX_free(kctx);
    OPENSSL_free(keyPubEnc);

    return ret;
}

static UA_StatusCode
UA_Openssl_ECDSA_Sign(const UA_ByteString * message,
                      EVP_PKEY * privateKey,
                      const EVP_MD * evpMd,
                      UA_ByteString * outSignature) {
    EVP_MD_CTX * mdctx = NULL;
    int opensslRet = 0;
    EVP_PKEY_CTX * evpKeyCtx = NULL;
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_ByteString signatureEnc = UA_BYTESTRING_NULL;
    ECDSA_SIG* signatureRaw = NULL;
    const BIGNUM * pr = NULL;
    const BIGNUM * ps = NULL;
    size_t sizeEncCoordinate = 0;

    if (privateKey == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    mdctx = EVP_MD_CTX_create();
    if (mdctx == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    opensslRet = EVP_DigestSignInit (mdctx, &evpKeyCtx, evpMd, NULL, privateKey);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    opensslRet = EVP_DigestSignUpdate (mdctx, message->data, message->length);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Length required for internal computing */
#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
    signatureEnc.length = EVP_PKEY_get_size(privateKey);
#else
    signatureEnc.length = ECDSA_size(EVP_PKEY_get0_EC_KEY(privateKey));
#endif

    /* Temporary buffer for OpenSSL-internal signature computation */
    if (UA_STATUSCODE_GOOD != UA_ByteString_allocBuffer(&signatureEnc, signatureEnc.length)) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    opensslRet = EVP_DigestSignFinal(mdctx, signatureEnc.data, &signatureEnc.length);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    // using temp pointer since the function call increments it
    const UA_Byte * ptmpData = signatureEnc.data;

    signatureRaw = d2i_ECDSA_SIG(NULL, &ptmpData, signatureEnc.length);
    if (signatureRaw == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    ECDSA_SIG_get0(signatureRaw, &pr, &ps);
    if (pr == NULL || ps == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    sizeEncCoordinate = outSignature->length / 2;
    if(sizeEncCoordinate <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if (BN_bn2binpad(pr, outSignature->data, sizeEncCoordinate) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if (BN_bn2binpad(ps, outSignature->data + sizeEncCoordinate, sizeEncCoordinate) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

errout:
    EVP_MD_CTX_destroy(mdctx);
    UA_ByteString_clear(&signatureEnc);
    ECDSA_SIG_free(signatureRaw);

    return ret;
}

static UA_StatusCode
UA_Openssl_ECDSA_Verify(const UA_ByteString * message,
                        const EVP_MD * evpMd,
                        X509 * publicKeyX509,
                        const UA_ByteString * signature) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    EVP_MD_CTX *mdctx = NULL;
    BIGNUM *pr = NULL;
    BIGNUM *ps = NULL;
    UA_ByteString signatureEnc = UA_BYTESTRING_NULL;
    ECDSA_SIG *signatureRaw = NULL;
    EVP_PKEY *evpPublicKey = NULL;
    size_t sizeEncCoordinate = 0;

    if (evpMd == EVP_sha256()) {
        sizeEncCoordinate = 32;
    } else if (evpMd == EVP_sha384()) {
        sizeEncCoordinate = 48;
    } else {
        ret = UA_STATUSCODE_BADINVALIDARGUMENT;
        goto errout;
    }

    pr = BN_bin2bn(signature->data, sizeEncCoordinate, NULL);
    ps = BN_bin2bn(signature->data + sizeEncCoordinate, sizeEncCoordinate, NULL);
    if(pr == NULL || ps == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    signatureRaw = ECDSA_SIG_new();
    if (signatureRaw == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(ECDSA_SIG_set0(signatureRaw, pr, ps) != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    // this call is to find out the length of the encoded signature
    signatureEnc.length = i2d_ECDSA_SIG(signatureRaw, NULL);
    if (signatureEnc.length == 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if (UA_STATUSCODE_GOOD != UA_ByteString_allocBuffer(&signatureEnc, signatureEnc.length)) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    // use temp pointer since the function call increments it
    UA_Byte * ptmpData = signatureEnc.data;

    signatureEnc.length = i2d_ECDSA_SIG(signatureRaw, &ptmpData);
    if (signatureEnc.length == 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    mdctx = EVP_MD_CTX_create();
    if(!mdctx) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    evpPublicKey = X509_get_pubkey(publicKeyX509);
    if(!evpPublicKey) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    EVP_PKEY_CTX * evpKeyCtx = NULL;
    int opensslRet = EVP_DigestVerifyInit(mdctx, &evpKeyCtx, evpMd,
                                          NULL, evpPublicKey);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    opensslRet = EVP_DigestVerifyUpdate (mdctx, message->data, message->length);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    opensslRet = EVP_DigestVerifyFinal(mdctx, signatureEnc.data, signatureEnc.length);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    ret = UA_STATUSCODE_GOOD;

errout:
    EVP_PKEY_free(evpPublicKey);
    EVP_MD_CTX_destroy(mdctx);
    ECDSA_SIG_free(signatureRaw);
    UA_ByteString_clear(&signatureEnc);

    return ret;
}

UA_StatusCode
UA_OpenSSL_ECC_NISTP256_GenerateKey (EVP_PKEY ** keyPairOut,
                                     UA_ByteString * keyPublicEncOut) {
    return UA_OpenSSL_ECC_GenerateKey (EC_curve_nist2nid("P-256"), keyPairOut,
                                       keyPublicEncOut);
}

UA_StatusCode
UA_Openssl_ECDSA_SHA256_Sign (const UA_ByteString * message,
                              EVP_PKEY * privateKey,
                              UA_ByteString * outSignature) {
    return UA_Openssl_ECDSA_Sign (message, privateKey, EVP_sha256(),
                                  outSignature);
}

UA_StatusCode
UA_Openssl_ECDSA_SHA256_Verify (const UA_ByteString * message,
                                X509 * publicKeyX509,
                                const UA_ByteString * signature) {
    return UA_Openssl_ECDSA_Verify (message, EVP_sha256(), publicKeyX509,
                                    signature);
}

#endif