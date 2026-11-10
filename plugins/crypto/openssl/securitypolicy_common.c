/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Wind River Systems, Inc.
 *    Copyright 2020 (c) basysKom GmbH
 *    Copyright 2022 (c) Wind River Systems, Inc.
 *    Copyright 2022 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2024 (c) Siemens AG (Authors: Tin Raic, Thomas Zeschg)
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
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
#include <openssl/params.h>
#endif

#include "securitypolicy_common.h"

#include <limits.h>

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
UA_Openssl_Init(void) {
    /* VxWorks7 has initialized the openssl. */
#ifndef __VXWORKS__
    static UA_Int16 bInit = 0;
    if(bInit == 1)
        return;
#if defined(OPENSSL_API_COMPAT) && (OPENSSL_API_COMPAT < 0x10100000L)
    /* only needed, if OpenSSL < V1.1 */
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
#endif
    bInit = 1;
#endif
}

#if(OPENSSL_VERSION_NUMBER >= 0x30000000L)
/* Algorithms used per message or per key are fetched once per process and
 * passed to the operations */
static CRYPTO_ONCE fetchedOnce = CRYPTO_ONCE_STATIC_INIT;
static EVP_CIPHER *fetchedAes128Cbc;
static EVP_CIPHER *fetchedAes256Cbc;
static EVP_CIPHER *fetchedAes128Gcm;
static EVP_CIPHER *fetchedChaCha20Poly1305;
static EVP_KDF *fetchedHkdf;
static EVP_MD *fetchedSha1;
/* HMAC with the digest set but no key */
static EVP_MAC_CTX *fetchedHmacSha1;
static EVP_MAC_CTX *fetchedHmacSha256;
static EVP_MAC_CTX *fetchedHmacSha384;

static void
freeFetchedAlgorithms(void) {
    EVP_CIPHER_free(fetchedAes128Cbc);
    EVP_CIPHER_free(fetchedAes256Cbc);
    EVP_CIPHER_free(fetchedAes128Gcm);
    EVP_CIPHER_free(fetchedChaCha20Poly1305);
    EVP_KDF_free(fetchedHkdf);
    EVP_MD_free(fetchedSha1);
    EVP_MAC_CTX_free(fetchedHmacSha1);
    EVP_MAC_CTX_free(fetchedHmacSha256);
    EVP_MAC_CTX_free(fetchedHmacSha384);
}

static EVP_MAC_CTX *
newHmacCtx(EVP_MAC *mac, const char *digest) {
    EVP_MAC_CTX *ctx = EVP_MAC_CTX_new(mac);
    OSSL_PARAM params[2];
    params[0] = OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST,
                                                 (char *)(uintptr_t)digest, 0);
    params[1] = OSSL_PARAM_construct_end();
    if(ctx && EVP_MAC_CTX_set_params(ctx, params) != 1) {
        EVP_MAC_CTX_free(ctx);
        ctx = NULL;
    }
    return ctx;
}

static void
fetchAlgorithms(void) {
    if(OPENSSL_atexit(freeFetchedAlgorithms) != 1)
        return;
    /* On a failed fetch, the callers fall back to the constant */
    fetchedAes128Cbc = EVP_CIPHER_fetch(NULL, "AES-128-CBC", NULL);
    fetchedAes256Cbc = EVP_CIPHER_fetch(NULL, "AES-256-CBC", NULL);
    fetchedAes128Gcm = EVP_CIPHER_fetch(NULL, "AES-128-GCM", NULL);
    fetchedChaCha20Poly1305 = EVP_CIPHER_fetch(NULL, "ChaCha20-Poly1305", NULL);
    fetchedHkdf = EVP_KDF_fetch(NULL, "HKDF", NULL);
    fetchedSha1 = EVP_MD_fetch(NULL, "SHA1", NULL);
    EVP_MAC *mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
    if(!mac)
        return;
    fetchedHmacSha1 = newHmacCtx(mac, "SHA1");
    fetchedHmacSha256 = newHmacCtx(mac, "SHA256");
    fetchedHmacSha384 = newHmacCtx(mac, "SHA384");
    EVP_MAC_free(mac);
}

const EVP_CIPHER *
UA_OpenSSL_fetchedCipher(const EVP_CIPHER *cipher) {
    if(CRYPTO_THREAD_run_once(&fetchedOnce, fetchAlgorithms) != 1)
        return cipher;
    const EVP_CIPHER *fetched = NULL;
    if(cipher == EVP_aes_128_cbc())
        fetched = fetchedAes128Cbc;
    else if(cipher == EVP_aes_256_cbc())
        fetched = fetchedAes256Cbc;
    else if(cipher == EVP_aes_128_gcm())
        fetched = fetchedAes128Gcm;
    else if(cipher == EVP_chacha20_poly1305())
        fetched = fetchedChaCha20Poly1305;
    return fetched ? fetched : cipher;
}

static const EVP_MAC_CTX *
fetchedHmac(const EVP_MD *md) {
    if(CRYPTO_THREAD_run_once(&fetchedOnce, fetchAlgorithms) != 1)
        return NULL;
    if(md == EVP_sha1())
        return fetchedHmacSha1;
    if(md == EVP_sha256())
        return fetchedHmacSha256;
    if(md == EVP_sha384())
        return fetchedHmacSha384;
    return NULL;
}

static const EVP_MD *
fetchedDigest(const EVP_MD *md) {
    if(CRYPTO_THREAD_run_once(&fetchedOnce, fetchAlgorithms) != 1)
        return md;
    if(md == EVP_sha1() && fetchedSha1)
        return fetchedSha1;
    return md;
}

static EVP_KDF *
fetchedKdfHkdf(void) {
    if(CRYPTO_THREAD_run_once(&fetchedOnce, fetchAlgorithms) != 1)
        return NULL;
    return fetchedHkdf;
}

/* HMAC context without a key, duplicated from the prefetched template */
static EVP_MAC_CTX *
newHmacCtxForDigest(const EVP_MD *md) {
    const EVP_MAC_CTX *prepared = fetchedHmac(md);
    if(prepared)
        return EVP_MAC_CTX_dup(prepared);
    EVP_MAC *mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
    if(!mac)
        return NULL;
    EVP_MAC_CTX *ctx = newHmacCtx(mac, EVP_MD_get0_name(md));
    EVP_MAC_free(mac); /* the context holds its own reference */
    return ctx;
}

/* EVP_MAC_init treats a NULL key as "keep the current key", so an empty key
 * needs a valid pointer */
static const unsigned char *
macKeyPointer(const UA_ByteString *key) {
    static const unsigned char emptyKey = 0;
    return key->length > 0 ? key->data : &emptyKey;
}
#else
#define fetchedDigest(md) (md)
#endif

static UA_StatusCode
UA_OpenSSL_HMAC(const EVP_MD *md, const UA_ByteString *key,
                const UA_ByteString *message, UA_ByteString *signature) {
#if(OPENSSL_VERSION_NUMBER >= 0x30000000L)
    UA_StatusCode res = UA_STATUSCODE_BADINTERNALERROR;
    EVP_MAC_CTX *ctx = newHmacCtxForDigest(md);
    if(ctx &&
       EVP_MAC_init(ctx, macKeyPointer(key), key->length, NULL) == 1 &&
       EVP_MAC_update(ctx, message->data, message->length) == 1 &&
       EVP_MAC_final(ctx, signature->data, &signature->length,
                     signature->length) == 1)
        res = UA_STATUSCODE_GOOD;
    EVP_MAC_CTX_free(ctx);
    return res;
#else
    if(HMAC(md, key->data, (int)key->length, message->data, message->length,
            signature->data, (unsigned int *)&signature->length) == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
#endif
}

/* HMAC over raw buffers, for the P_SHA key derivation. out may equal msg. */
static UA_StatusCode
UA_OpenSSL_HMAC_raw(const EVP_MD *md, const UA_Byte *key, size_t keyLength,
                    const UA_Byte *msg, size_t msgLength,
                    UA_Byte *out, size_t outLength) {
    UA_ByteString k = {keyLength, (UA_Byte *)(uintptr_t)key};
    UA_ByteString m = {msgLength, (UA_Byte *)(uintptr_t)msg};
    UA_ByteString o = {outLength, out};
    return UA_OpenSSL_HMAC(md, &k, &m, &o);
}

static int UA_OpenSSL_RSA_Key_Size(EVP_PKEY * key){
#if(OPENSSL_VERSION_NUMBER >= 0x30000000L)
    return EVP_PKEY_get_size(key);
#else
    return RSA_size(get_pkey_rsa(key));
#endif
}

/* UA_copyCertificate - allocate the buffer, copy the certificate and add a NULL
 * to the end */

UA_StatusCode
UA_copyCertificate(UA_ByteString * dst, const UA_ByteString * src) {
    UA_StatusCode retval = UA_ByteString_allocBuffer(dst, src->length + 1);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    (void) memcpy(dst->data, src->data, src->length);
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

    opensslRet = EVP_DigestVerifyUpdate(mdctx, message->data, message->length);
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
UA_OpenSSL_RSA_PKCS1_V15_SHA256_Verify(const UA_ByteString *msg,
                                       X509 *publicKeyX509,
                                       const UA_ByteString *signature) {
    return UA_OpenSSL_RSA_Public_Verify(msg, EVP_sha256(), publicKeyX509,
                                        RSA_PKCS1_PADDING, signature);
}

UA_StatusCode
UA_OpenSSL_RSA_PSS_SHA256_Verify(const UA_ByteString *msg,
                                 X509 *publicKeyX509,
                                 const UA_ByteString *signature) {
    return UA_OpenSSL_RSA_Public_Verify(msg, EVP_sha256(), publicKeyX509,
                                        RSA_PKCS1_PSS_PADDING, signature);
}

/* Get certificate thumbprint, and allocate the buffer. */

UA_StatusCode
UA_Openssl_X509_GetCertificateThumbprint(const UA_ByteString *certficate,
                                         UA_ByteString *pThumbprint,
                                         bool bThumbPrint) {
    if(bThumbPrint) {
        pThumbprint->length = SHA_DIGEST_LENGTH;
        UA_StatusCode ret = UA_ByteString_allocBuffer(pThumbprint, pThumbprint->length);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
    } else {
        if(pThumbprint->length != SHA_DIGEST_LENGTH)
            return UA_STATUSCODE_BADINTERNALERROR;
    }
    X509 * x509Certificate = UA_OpenSSL_LoadCertificate(certficate, EVP_PKEY_NONE);

    if(x509Certificate == NULL) {
        if(bThumbPrint)
            UA_ByteString_clear(pThumbprint);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(X509_digest(x509Certificate, fetchedDigest(EVP_sha1()),
                   pThumbprint->data, NULL) != 1) {
        if(bThumbPrint)
            UA_ByteString_clear(pThumbprint);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    X509_free(x509Certificate);
    return UA_STATUSCODE_GOOD;
}

/* Substring search on a (not necessarily null-terminated) UA_String. */
static UA_Boolean
policyUriContains(const UA_String *uri, const char *token) {
    size_t n = strlen(token);
    if(uri->length < n)
        return false;
    for(size_t i = 0; i + n <= uri->length; i++) {
        if(memcmp(uri->data + i, token, n) == 0)
            return true;
    }
    return false;
}

/* OPC UA Part 6 v1.05.07 (SecureChannelEnhancements): hash a certificate (the
 * leaf, DER) with the hash of the policy's elliptic curve - SHA-256 for the
 * nistP256 curve, SHA-384 for nistP384. Used to build the channel-bound
 * CreateSession / ActivateSession SignatureData.
 *
 * This is NOT the OPN-header Certificate thumbprint: that thumbprint uses the
 * policy's CertificateThumbprintAlgorithm (SHA-1 by default) and is produced by
 * makeCertThumbprint / UA_Openssl_X509_GetCertificateThumbprint. The digest
 * here is selected from the policy URI's curve. */
UA_StatusCode
UA_SecurityPolicy_hashCertificate(const UA_SecurityPolicy *policy,
                                  const UA_ByteString *certificate,
                                  UA_ByteString *hash) {
    if(policy == NULL || certificate == NULL || hash == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Select the digest from the policy's elliptic curve, not a fixed
     * algorithm. */
    const EVP_MD *md;
    size_t hashLen;
    if(policyUriContains(&policy->policyUri, "P384")) {
        md = EVP_sha384();
        hashLen = 48;
    } else if(policyUriContains(&policy->policyUri, "P256")) {
        md = EVP_sha256();
        hashLen = 32;
    } else {
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
    }

    X509 *x509 = UA_OpenSSL_LoadCertificate(certificate, EVP_PKEY_NONE);
    if(x509 == NULL)
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    UA_StatusCode ret = UA_ByteString_allocBuffer(hash, hashLen);
    if(ret != UA_STATUSCODE_GOOD) {
        X509_free(x509);
        return ret;
    }
    unsigned int len = 0;
    if(X509_digest(x509, md, hash->data, &len) != 1 || len != hashLen) {
        UA_ByteString_clear(hash);
        X509_free(x509);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    X509_free(x509);
    return UA_STATUSCODE_GOOD;
}

/* Backend-agnostic SecurityPolicy properties derived from the policy URI. Kept
 * out of the UA_SecurityPolicy struct (so its layout stays stable across the
 * 1.5 release family); see the #None fallback in ua_securitypolicy_none.c used
 * when no crypto backend is built. */

/* True if the policy requires the OPC UA Part 6 v1.05.07
 * SecureChannelEnhancements behavior (the ECC_nistP256_* policies). */
UA_Boolean
UA_SecurityPolicy_isEnhancedSecurity(const UA_SecurityPolicy *policy) {
    if(!policy)
        return false;
    static const UA_String eccNistP256AesGcm =
        UA_STRING_STATIC(UA_SECURITYPOLICY_URI_ECC_NISTP256_AESGCM);
    static const UA_String eccNistP256ChaChaPoly =
        UA_STRING_STATIC(UA_SECURITYPOLICY_URI_ECC_NISTP256_CHACHAPOLY);
    return UA_String_equal(&policy->policyUri, &eccNistP256AesGcm) ||
           UA_String_equal(&policy->policyUri, &eccNistP256ChaChaPoly);
}

/* In the OPC UA reference stack the SequenceNumber handling depends on a
 * SecurityPolicy property `LegacySequenceNumbers`: false for all ECC policies
 * (and RSA-DH, which open62541 does not implement), true for None / RSA
 * (Basic*, Aes*_RsaOaep/RsaPss). Non-legacy starts the channel SequenceNumber
 * at 0 and wraps UA_UINT32_MAX -> 0; legacy starts at 1 with the "< 1024"
 * rollover. Detected from the policy URI (all ECC URIs carry the "ECC_"
 * fragment). A NULL policy is treated as legacy. */
UA_Boolean
UA_SecurityPolicy_useLegacySequenceNumbers(const UA_SecurityPolicy *policy) {
    if(!policy)
        return true;
    return !policyUriContains(&policy->policyUri, "ECC_");
}

static UA_StatusCode
UA_Openssl_RSA_Private_Decrypt(UA_ByteString *data, EVP_PKEY *privateKey,
                               UA_Int16 padding, UA_Boolean withSha256) {
    if(data == NULL || privateKey == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    size_t keySize = (size_t) UA_OpenSSL_RSA_Key_Size(privateKey);
    /* The buffer must consist of an integral number of RSA blocks. Without
     * this check, EVP_PKEY_decrypt is called with keySize bytes from an
     * undersized buffer, which reads past the end of data->data. Also guard
     * against keySize == 0 (e.g. unexpected key type / error from the helper),
     * which would otherwise raise a division-by-zero / undefined behaviour. */
    if(data->length == 0 || keySize == 0 || (data->length % keySize) != 0)
        return UA_STATUSCODE_BADINTERNALERROR;
    size_t cipherOffset = 0;
    size_t outOffset = 0;
    unsigned char buf[RSA_DECRYPT_BUFFER_LENGTH];
    size_t decryptedBytes;
    EVP_PKEY_CTX * ctx;
    int opensslRet;

    ctx = EVP_PKEY_CTX_new(privateKey, NULL);
    if(ctx == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    opensslRet = EVP_PKEY_decrypt_init(ctx);
    if(opensslRet != 1) {
        EVP_PKEY_CTX_free(ctx);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    opensslRet = EVP_PKEY_CTX_set_rsa_padding(ctx, padding);
    if(opensslRet != 1) {
        EVP_PKEY_CTX_free(ctx);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(withSha256) {
        opensslRet = EVP_PKEY_CTX_set_rsa_oaep_md(ctx, SHA256EVP());
        if(opensslRet != 1)
            return UA_STATUSCODE_BADINTERNALERROR;
        opensslRet = EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, SHA256EVP());
        if(opensslRet != 1)
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    while(cipherOffset < data->length) {
        decryptedBytes = RSA_DECRYPT_BUFFER_LENGTH;
        opensslRet = EVP_PKEY_decrypt(ctx, buf, &decryptedBytes,
                                       data->data + cipherOffset, keySize);
        if(opensslRet != 1) {
            EVP_PKEY_CTX_free(ctx);
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        }
        (void) memcpy(data->data + outOffset, buf, decryptedBytes);
        cipherOffset += (size_t) keySize;
        outOffset += decryptedBytes;
    }
    data->length = outOffset;
    EVP_PKEY_CTX_free(ctx);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Openssl_RSA_Oaep_Decrypt(UA_ByteString *data,
                            EVP_PKEY *privateKey) {
    return UA_Openssl_RSA_Private_Decrypt(data, privateKey,
                                          RSA_PKCS1_OAEP_PADDING, false);
}

UA_StatusCode
UA_Openssl_RSA_Oaep_Sha2_Decrypt(UA_ByteString *data,
                             EVP_PKEY *privateKey) {
    return UA_Openssl_RSA_Private_Decrypt(data, privateKey,
                                          RSA_PKCS1_OAEP_PADDING, true);
}

static UA_StatusCode
UA_Openssl_RSA_Public_Encrypt(const UA_ByteString *message, X509 *publicX509,
                              UA_Int16 padding, size_t paddingSize,
                              UA_ByteString *encrypted, UA_Boolean withSha256) {
    EVP_PKEY_CTX *ctx = NULL;
    int opensslRet;
    UA_StatusCode ret;
    size_t encryptedTextLen = 0;
    size_t dataPos =  0;
    size_t encryptedPos = 0;
    size_t bytesToEncrypt = 0;
    size_t encryptedBlockSize = 0;
    size_t keySize = 0;

    EVP_PKEY *evpPublicKey = X509_get_pubkey(publicX509);
    if(evpPublicKey == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }
    ctx = EVP_PKEY_CTX_new(evpPublicKey, NULL);
    if(ctx == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }
    opensslRet = EVP_PKEY_encrypt_init(ctx);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    opensslRet = EVP_PKEY_CTX_set_rsa_padding(ctx, padding);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    if(withSha256) {
        opensslRet = EVP_PKEY_CTX_set_rsa_oaep_md(ctx, SHA256EVP());
        if(opensslRet != 1) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
        opensslRet = EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, SHA256EVP());
        if(opensslRet != 1) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
    }

    /* get the encrypted block size */

    keySize = (size_t) UA_OpenSSL_RSA_Key_Size(evpPublicKey);
    if(keySize == 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    switch(padding) {
        case RSA_PKCS1_OAEP_PADDING:
        case RSA_PKCS1_PADDING:
            if(keySize <= paddingSize) {
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

    while(dataPos > 0) {
        size_t outlen = keySize;
        encryptedPos -= keySize;
        dataPos -= bytesToEncrypt;
        opensslRet = EVP_PKEY_encrypt(ctx, encrypted->data + encryptedPos, &outlen,
                                      message->data + dataPos, bytesToEncrypt);

        if(opensslRet != 1) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
        bytesToEncrypt = encryptedBlockSize;
    }
    encrypted->length = encryptedTextLen;

    ret = UA_STATUSCODE_GOOD;
errout:
    if(evpPublicKey != NULL)
        EVP_PKEY_free(evpPublicKey);
    if(ctx != NULL)
        EVP_PKEY_CTX_free (ctx);
    return ret;
}

UA_StatusCode
UA_Openssl_RSA_OAEP_Encrypt(UA_ByteString *data,
                            size_t paddingSize,
                            X509 *publicX509) {
    UA_ByteString message;
    UA_StatusCode ret = UA_ByteString_copy(data, &message);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;
    ret = UA_Openssl_RSA_Public_Encrypt(&message, publicX509,
                                        RSA_PKCS1_OAEP_PADDING,
                                        paddingSize, data, false);
    UA_ByteString_clear(&message);
    return ret;
}

UA_StatusCode
UA_Openssl_RSA_OAEP_SHA2_Encrypt(UA_ByteString *data,
                                 size_t paddingSize,
                                 X509 *publicX509) {
    UA_ByteString message;
    UA_StatusCode ret = UA_ByteString_copy (data, &message);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;
    ret = UA_Openssl_RSA_Public_Encrypt(&message, publicX509,
                                        RSA_PKCS1_OAEP_PADDING,
                                        paddingSize, data, true);
    UA_ByteString_clear(&message);
    return ret;
}

/* The context holds the secret, the seed and the A(n) chain. Wipe it before
 * it goes back to the heap. */
static void
P_SHA256_Ctx_Free(UA_Openssl_P_SHA256_Ctx *ctx) {
    if(ctx == NULL)
        return;
    OPENSSL_cleanse(ctx, sizeof(UA_Openssl_P_SHA256_Ctx) +
                    ctx->secretLen + ctx->seedLen);
    UA_free(ctx);
}

static UA_Openssl_P_SHA256_Ctx *
P_SHA256_Ctx_Create(const UA_ByteString *secret,
                    const UA_ByteString *seed) {
    size_t size = (UA_Int32)sizeof(UA_Openssl_P_SHA256_Ctx) +
        secret->length + seed->length;
    UA_Openssl_P_SHA256_Ctx * ctx = (UA_Openssl_P_SHA256_Ctx *)UA_malloc(size);
    if(ctx == NULL)
        return NULL;
    ctx->secretLen = secret->length;
    ctx->seedLen = seed->length;
    memcpy(UA_Openssl_P_SHA256_SEED(ctx), seed->data, seed->length);
    memcpy(UA_Openssl_P_SHA256_SECRET(ctx), secret->data, secret->length);
    /* A(0) = seed
       A(n) = HMAC_HASH(secret, A(n-1)) */

    if(UA_OpenSSL_HMAC_raw(EVP_sha256(), secret->data, secret->length,
                           seed->data, seed->length,
                           ctx->A, sizeof(ctx->A)) != UA_STATUSCODE_GOOD) {
        P_SHA256_Ctx_Free(ctx);
        return NULL;
    }

    return ctx;
}

static UA_StatusCode
P_SHA256_Hash_Generate(UA_Openssl_P_SHA256_Ctx *ctx,
                       UA_Byte *pHas) {
    /* Calculate P_SHA256(n) = HMAC_SHA256(secret, A(n)+seed) */
    if(UA_OpenSSL_HMAC_raw(EVP_sha256(), UA_Openssl_P_SHA256_SECRET(ctx),
                           ctx->secretLen, ctx->A, sizeof(ctx->A) + ctx->seedLen,
                           pHas, SHA256_DIGEST_LENGTH) != UA_STATUSCODE_GOOD) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Calculate A(n) = HMAC_SHA256(secret, A(n-1)) */
    if(UA_OpenSSL_HMAC_raw(EVP_sha256(), UA_Openssl_P_SHA256_SECRET(ctx),
                           ctx->secretLen, ctx->A, sizeof(ctx->A),
                           ctx->A, sizeof(ctx->A)) != UA_STATUSCODE_GOOD) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Openssl_Random_Key_PSHA256_Derive(const UA_ByteString *secret,
                                     const UA_ByteString *seed,
                                     UA_ByteString *out) {
    size_t keyLen = out->length;
    size_t iter   = keyLen/32 + ((keyLen%32)?1:0);
    size_t bufferLen = iter * 32;
    size_t i;
    UA_StatusCode st;

    UA_Byte * pBuffer = (UA_Byte *) UA_malloc(bufferLen);
    if(pBuffer == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_Openssl_P_SHA256_Ctx *ctx = P_SHA256_Ctx_Create(secret, seed);
    if(ctx == NULL) {
        UA_free(pBuffer);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    for(i = 0; i < iter; i++) {
        st = P_SHA256_Hash_Generate(ctx, pBuffer + (i * 32));
        if(st != UA_STATUSCODE_GOOD) {
            OPENSSL_cleanse(pBuffer, bufferLen);
            UA_free(pBuffer);
            P_SHA256_Ctx_Free(ctx);
            return st;
        }
    }

    /* The scratch buffer holds derived key material */
    memcpy(out->data, pBuffer, keyLen);
    OPENSSL_cleanse(pBuffer, bufferLen);
    UA_free(pBuffer);
    P_SHA256_Ctx_Free(ctx);
    return UA_STATUSCODE_GOOD;
}

/* return the key bytes */
UA_StatusCode
UA_Openssl_RSA_Public_GetKeyLength(X509 *publicKeyX509,
                                   UA_Int32 *keyLen) {
    EVP_PKEY * evpKey = X509_get_pubkey(publicKeyX509);
    if(evpKey == NULL)
        return  UA_STATUSCODE_BADINTERNALERROR;
    *keyLen = UA_OpenSSL_RSA_Key_Size(evpKey);
    EVP_PKEY_free(evpKey);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Openssl_RSA_Private_GetKeyLength(EVP_PKEY *privateKey,
                                    UA_Int32 *keyLen) {
    if(privateKey == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    *keyLen = UA_OpenSSL_RSA_Key_Size(privateKey);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_Openssl_RSA_Private_Sign(const UA_ByteString *message,
                            EVP_PKEY *privateKey,
                            const EVP_MD *evpMd,
                            UA_Int16 padding,
                            UA_ByteString *outSignature) {
    if(privateKey == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    int opensslRet;
    EVP_PKEY_CTX *evpKeyCtx;
    UA_StatusCode ret;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
    if(mdctx == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    opensslRet = EVP_DigestSignInit(mdctx, &evpKeyCtx, evpMd, NULL, privateKey);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    EVP_PKEY_CTX_set_rsa_padding(evpKeyCtx, padding);
    if(padding == RSA_PKCS1_PSS_PADDING) {
        opensslRet = EVP_PKEY_CTX_set_rsa_pss_saltlen(evpKeyCtx,
                                                      RSA_PSS_SALTLEN_DIGEST); //RSA_PSS_SALTLEN_DIGEST
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

    opensslRet = EVP_DigestSignUpdate(mdctx, message->data, message->length);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    opensslRet = EVP_DigestSignFinal(mdctx, outSignature->data, &outSignature->length);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    ret = UA_STATUSCODE_GOOD;

errout:
    if(mdctx != NULL)
        EVP_MD_CTX_destroy(mdctx);
    return ret;
}

UA_StatusCode
UA_Openssl_RSA_PKCS1_V15_SHA256_Sign(const UA_ByteString *message,
                                     EVP_PKEY *privateKey,
                                     UA_ByteString *outSignature) {
    return UA_Openssl_RSA_Private_Sign(message, privateKey, EVP_sha256(),
                                       RSA_PKCS1_PADDING , outSignature);
}

UA_StatusCode
UA_Openssl_RSA_PSS_SHA256_Sign(const UA_ByteString *message,
                               EVP_PKEY *privateKey,
                               UA_ByteString *outSignature) {
    return UA_Openssl_RSA_Private_Sign(message, privateKey, EVP_sha256(),
                                       RSA_PKCS1_PSS_PADDING, outSignature);
}

UA_StatusCode
UA_OpenSSL_HMAC_SHA256_Verify(const UA_ByteString *message,
                              const UA_ByteString *key,
                              const UA_ByteString *signature) {
    unsigned char buf[SHA256_DIGEST_LENGTH] = {0};
    UA_ByteString mac = {SHA256_DIGEST_LENGTH, buf};

    if(UA_OpenSSL_HMAC(EVP_sha256(), key, message, &mac) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(signature->length != mac.length ||
       !UA_constantTimeEqual(signature->data, mac.data, mac.length))
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_OpenSSL_HMAC_SHA256_Sign(const UA_ByteString *message,
                            const UA_ByteString *key,
                            UA_ByteString *signature) {
    return UA_OpenSSL_HMAC(EVP_sha256(), key, message, signature);
}

static UA_StatusCode
UA_OpenSSL_Decrypt(const UA_ByteString *iv,
                   const UA_ByteString *key,
                   const EVP_CIPHER *cipherAlg,
                   UA_ByteString *data  /* [in/out]*/) {
    UA_ByteString    ivCopy    = {0, NULL};
    UA_ByteString    cipherTxt = {0, NULL};
    EVP_CIPHER_CTX * ctx       = NULL;
    UA_StatusCode    ret;
    int              opensslRet;
    int              outLen;
    int              tmpLen;

    /* copy the IV because the AES_cbc_encrypt function overwrites it. */

    ret = UA_ByteString_copy(iv, &ivCopy);
    if(ret != UA_STATUSCODE_GOOD)
        goto errout;

    ret = UA_ByteString_copy(data, &cipherTxt);
    if(ret != UA_STATUSCODE_GOOD)
        goto errout;

    ctx = EVP_CIPHER_CTX_new();
    if(ctx == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    /* call EVP_* to decrypt */
    opensslRet = EVP_DecryptInit_ex(ctx, UA_OpenSSL_fetchedCipher(cipherAlg), NULL,
                                    key->data, ivCopy.data);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    /* EVP_DecryptFinal() will return an error code if padding is enabled
     * and the final block is not correctly formatted.
     */
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    opensslRet = EVP_DecryptUpdate(ctx, data->data, &outLen,
                                   cipherTxt.data, (int) cipherTxt.length);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    opensslRet = EVP_DecryptFinal_ex(ctx, data->data + outLen, &tmpLen);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    outLen += tmpLen;
    data->length = (size_t) outLen;
    ret = UA_STATUSCODE_GOOD;

errout:
    UA_ByteString_clear(&ivCopy);
    UA_ByteString_clear(&cipherTxt);
    if(ctx != NULL) {
        EVP_CIPHER_CTX_free(ctx);
    }
    return ret;
}

static UA_StatusCode
UA_OpenSSL_Encrypt(const UA_ByteString * iv,
                   const UA_ByteString * key,
                   const EVP_CIPHER *    cipherAlg,
                   UA_ByteString *       data  /* [in/out]*/) {
    UA_ByteString    ivCopy   = {0, NULL};
    UA_ByteString    plainTxt = {0, NULL};
    EVP_CIPHER_CTX * ctx      = NULL;
    int              opensslRet;
    int              outLen;
    int              tmpLen;

    /* copy the IV because the AES_cbc_encrypt function overwrites it. */
    UA_StatusCode ret = UA_ByteString_copy(iv, &ivCopy);
    if(ret != UA_STATUSCODE_GOOD) {
        goto errout;
    }

    ret = UA_ByteString_copy(data, &plainTxt);
    if(ret != UA_STATUSCODE_GOOD) {
        goto errout;
    }

    ctx = EVP_CIPHER_CTX_new();
    if(ctx == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    /* call EVP_* to encrypt */

    opensslRet = EVP_EncryptInit_ex(ctx, UA_OpenSSL_fetchedCipher(cipherAlg), NULL,
                                    key->data, ivCopy.data);
    if(opensslRet != 1) {
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
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Encrypt the data */
    opensslRet = EVP_EncryptUpdate(ctx, data->data, &outLen,
                                    plainTxt.data, (int) plainTxt.length);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Encrypt-final does nothing as padding is disabled */
    opensslRet = EVP_EncryptFinal_ex(ctx, data->data + outLen, &tmpLen);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    outLen += tmpLen;
    data->length = (size_t) outLen;
    ret = UA_STATUSCODE_GOOD;

errout:
    UA_ByteString_clear(&ivCopy);
    UA_ByteString_clear(&plainTxt);
    if(ctx != NULL)
        EVP_CIPHER_CTX_free(ctx);
    return ret;
}

void
UA_OpenSSL_SymContexts_init(UA_OpenSSL_SymContexts *sym) {
    memset(sym, 0, sizeof(*sym));
}

static void
freeCipherCtx(EVP_CIPHER_CTX **ctx) {
    EVP_CIPHER_CTX_free(*ctx);
    *ctx = NULL;
}

/* The contexts are rebuilt with the new key on the next use */
void
UA_OpenSSL_SymContexts_signingKeyChanged(UA_OpenSSL_SymContexts *sym,
                                         UA_Boolean local) {
#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
    EVP_MAC_CTX **mac = local ? &sym->localMac : &sym->remoteMac;
    EVP_MAC_CTX_free(*mac);
    *mac = NULL;
#endif
    freeCipherCtx(local ? &sym->localTag : &sym->remoteTag);
}

void
UA_OpenSSL_SymContexts_encryptingKeyChanged(UA_OpenSSL_SymContexts *sym,
                                            UA_Boolean local) {
    freeCipherCtx(local ? &sym->localCipher : &sym->remoteCipher);
    /* Some AEAD policies compute the tags with the encrypting key */
    freeCipherCtx(local ? &sym->localTag : &sym->remoteTag);
}

/* OpenSSL clears the key material of the contexts when freeing them */
void
UA_OpenSSL_SymContexts_clear(UA_OpenSSL_SymContexts *sym) {
    UA_OpenSSL_SymContexts_signingKeyChanged(sym, true);
    UA_OpenSSL_SymContexts_signingKeyChanged(sym, false);
    UA_OpenSSL_SymContexts_encryptingKeyChanged(sym, true);
    UA_OpenSSL_SymContexts_encryptingKeyChanged(sym, false);
}

/* Takes a prepared context if no other thread uses it */
static UA_Boolean
tryAcquireCtx(UA_atomic(uintptr_t) *busy) {
    uintptr_t wasBusy = 1;
    UA_atomic_xchg(busy, (uintptr_t)1, &wasBusy);
    return wasBusy == 0;
}

static void
releaseCtx(UA_atomic(uintptr_t) *busy) {
    UA_atomic_store(busy, (uintptr_t)0);
}

#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
/* The context absorbs the key once. Every later message only resets it. */
static UA_StatusCode
keyedHmac(EVP_MAC_CTX **slot, const EVP_MD *md, const UA_ByteString *key,
          const UA_ByteString *message, unsigned char *out, size_t outSize,
          size_t *outLength) {
    EVP_MAC_CTX *ctx = *slot;
    int ok;
    if(!ctx) {
        ctx = newHmacCtxForDigest(md);
        if(!ctx)
            return UA_STATUSCODE_BADINTERNALERROR;
        *slot = ctx;
        ok = EVP_MAC_init(ctx, macKeyPointer(key), key->length, NULL);
    } else {
        ok = EVP_MAC_init(ctx, NULL, 0, NULL);
    }
    if(ok == 1)
        ok = EVP_MAC_update(ctx, message->data, message->length);
    if(ok == 1)
        ok = EVP_MAC_final(ctx, out, outLength, outSize);
    if(ok != 1) {
        EVP_MAC_CTX_free(ctx);
        *slot = NULL;
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}
#endif

UA_StatusCode
UA_OpenSSL_Sym_sign(UA_OpenSSL_SymContexts *sym, const EVP_MD *md,
                    const UA_ByteString *key, const UA_ByteString *message,
                    UA_ByteString *signature) {
#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
    if(!tryAcquireCtx(&sym->localMacBusy))
        return UA_OpenSSL_HMAC(md, key, message, signature);
    size_t outLength = 0;
    UA_StatusCode res = keyedHmac(&sym->localMac, md, key, message,
                                  signature->data, signature->length,
                                  &outLength);
    releaseCtx(&sym->localMacBusy);
    if(res == UA_STATUSCODE_GOOD)
        signature->length = outLength;
    return res;
#else
    (void)sym;
    return UA_OpenSSL_HMAC(md, key, message, signature);
#endif
}

UA_StatusCode
UA_OpenSSL_Sym_verify(UA_OpenSSL_SymContexts *sym, const EVP_MD *md,
                      const UA_ByteString *key, const UA_ByteString *message,
                      const UA_ByteString *signature) {
    unsigned char buf[EVP_MAX_MD_SIZE];
    UA_StatusCode res = UA_STATUSCODE_BADINTERNALERROR;
    size_t macLength = 0;
    UA_Boolean prepared = false;
#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
    prepared = tryAcquireCtx(&sym->remoteMacBusy);
    if(prepared) {
        res = keyedHmac(&sym->remoteMac, md, key, message,
                        buf, sizeof(buf), &macLength);
        releaseCtx(&sym->remoteMacBusy);
    }
#else
    (void)sym;
#endif
    if(!prepared) {
        UA_ByteString mac = {sizeof(buf), buf};
        res = UA_OpenSSL_HMAC(md, key, message, &mac);
        macLength = mac.length;
    }
    if(res == UA_STATUSCODE_GOOD &&
       (signature->length != macLength ||
        !UA_constantTimeEqual(signature->data, buf, macLength)))
        res = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    OPENSSL_cleanse(buf, sizeof(buf));
    return res;
}

/* The context keeps the key schedule. Every message only sets the IV and
 * runs in place (OpenSSL allows identical input and output buffers). */
static UA_StatusCode
symCrypt(EVP_CIPHER_CTX **slot, const EVP_CIPHER *cipher,
         const UA_ByteString *key, const UA_ByteString *iv,
         UA_ByteString *data, int encrypt) {
    /* EVP reads exactly the IV length and key length from the pointers */
    if(iv->length != (size_t)EVP_CIPHER_iv_length(cipher) ||
       data->length > INT_MAX)
        return UA_STATUSCODE_BADINTERNALERROR;

    EVP_CIPHER_CTX *ctx = *slot;
    if(!ctx) {
        if(key->length != (size_t)EVP_CIPHER_key_length(cipher))
            return UA_STATUSCODE_BADINTERNALERROR;
        ctx = EVP_CIPHER_CTX_new();
        if(!ctx)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        if(EVP_CipherInit_ex(ctx, UA_OpenSSL_fetchedCipher(cipher), NULL,
                             key->data, NULL, encrypt) != 1 ||
           EVP_CIPHER_CTX_set_padding(ctx, 0) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        *slot = ctx;
    }

    int outLength = 0;
    int finalLength = 0;
    if(EVP_CipherInit_ex(ctx, NULL, NULL, NULL, iv->data, encrypt) != 1 ||
       EVP_CipherUpdate(ctx, data->data, &outLength,
                        data->data, (int)data->length) != 1 ||
       EVP_CipherFinal_ex(ctx, data->data + outLength, &finalLength) != 1 ||
       (size_t)outLength + (size_t)finalLength != data->length) {
        EVP_CIPHER_CTX_free(ctx);
        *slot = NULL;
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_OpenSSL_Sym_encrypt(UA_OpenSSL_SymContexts *sym, const EVP_CIPHER *cipher,
                       const UA_ByteString *key, const UA_ByteString *iv,
                       UA_ByteString *data) {
    if(!tryAcquireCtx(&sym->localCipherBusy))
        return UA_OpenSSL_Encrypt(iv, key, cipher, data);
    UA_StatusCode res = symCrypt(&sym->localCipher, cipher, key, iv, data, 1);
    releaseCtx(&sym->localCipherBusy);
    return res;
}

UA_StatusCode
UA_OpenSSL_Sym_decrypt(UA_OpenSSL_SymContexts *sym, const EVP_CIPHER *cipher,
                       const UA_ByteString *key, const UA_ByteString *iv,
                       UA_ByteString *data) {
    if(!tryAcquireCtx(&sym->remoteCipherBusy))
        return UA_OpenSSL_Decrypt(iv, key, cipher, data);
    UA_StatusCode res = symCrypt(&sym->remoteCipher, cipher, key, iv, data, 0);
    releaseCtx(&sym->remoteCipherBusy);
    return res;
}

UA_StatusCode
UA_OpenSSL_X509_compare(const UA_ByteString * cert,
                        const X509 *          bcert) {
    X509 * acert = UA_OpenSSL_LoadCertificate(cert, EVP_PKEY_NONE);
    if(acert == NULL) {
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    }
    int opensslRet = X509_cmp(acert, bcert);
    X509_free(acert);

    if(opensslRet == 0)
        return UA_STATUSCODE_GOOD;
    return UA_STATUSCODE_UNCERTAINSUBNORMAL;
}

UA_StatusCode
UA_OpenSSL_RSA_PKCS1_V15_SHA1_Verify(const UA_ByteString * msg,
                                     X509 *                publicKeyX509,
                                     const UA_ByteString * signature) {
    return UA_OpenSSL_RSA_Public_Verify(msg, EVP_sha1(), publicKeyX509,
                                        RSA_PKCS1_PADDING, signature);
}

UA_StatusCode
UA_Openssl_RSA_PKCS1_V15_SHA1_Sign(const UA_ByteString * message,
                                   EVP_PKEY * privateKey,
                                   UA_ByteString *       outSignature) {
    return UA_Openssl_RSA_Private_Sign(message, privateKey, EVP_sha1(),
                                       RSA_PKCS1_PADDING, outSignature);
}

static void
P_SHA1_Ctx_Free(UA_Openssl_P_SHA1_Ctx *ctx) {
    if(ctx == NULL)
        return;
    OPENSSL_cleanse(ctx, sizeof(UA_Openssl_P_SHA1_Ctx) +
                    ctx->secretLen + ctx->seedLen);
    UA_free(ctx);
}

static UA_Openssl_P_SHA1_Ctx *
P_SHA1_Ctx_Create(const UA_ByteString *  secret,
                  const UA_ByteString *  seed) {
    size_t size = (UA_Int32)sizeof(UA_Openssl_P_SHA1_Ctx) + secret->length +
                    seed->length;
    UA_Openssl_P_SHA1_Ctx * ctx = (UA_Openssl_P_SHA1_Ctx *) UA_malloc(size);
    if(ctx == NULL)
        return NULL;

    ctx->secretLen = secret->length;
    ctx->seedLen = seed->length;
    memcpy(UA_Openssl_P_SHA1_SEED(ctx), seed->data, seed->length);
    memcpy(UA_Openssl_P_SHA1_SECRET(ctx), secret->data, secret->length);
    /* A(0) = seed
       A(n) = HMAC_HASH(secret, A(n-1)) */

    if(UA_OpenSSL_HMAC_raw(EVP_sha1(), secret->data, secret->length,
                           seed->data, seed->length,
                           ctx->A, sizeof(ctx->A)) != UA_STATUSCODE_GOOD) {
        P_SHA1_Ctx_Free(ctx);
        return NULL;
    }

    return ctx;
}

static UA_StatusCode
P_SHA1_Hash_Generate(UA_Openssl_P_SHA1_Ctx * ctx,
                     UA_Byte *               pHas) {
    /* Calculate P_SHA1(n) = HMAC_SHA1(secret, A(n)+seed) */
    if(UA_OpenSSL_HMAC_raw(EVP_sha1(), UA_Openssl_P_SHA1_SECRET(ctx),
                           ctx->secretLen, ctx->A, sizeof(ctx->A) + ctx->seedLen,
                           pHas, SHA1_DIGEST_LENGTH) != UA_STATUSCODE_GOOD) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Calculate A(n) = HMAC_SHA1(secret, A(n-1)) */
   if(UA_OpenSSL_HMAC_raw(EVP_sha1(), UA_Openssl_P_SHA1_SECRET(ctx),
                          ctx->secretLen, ctx->A, sizeof(ctx->A),
                          ctx->A, sizeof(ctx->A)) != UA_STATUSCODE_GOOD) {
       return UA_STATUSCODE_BADINTERNALERROR;
   }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Openssl_Random_Key_PSHA1_Derive(const UA_ByteString *     secret,
                                   const UA_ByteString *     seed,
                                   UA_ByteString *           out) {
    size_t keyLen     = out->length;
    size_t iter       = keyLen / SHA1_DIGEST_LENGTH + ((keyLen % SHA1_DIGEST_LENGTH)?1:0);
    size_t bufferLen  = iter * SHA1_DIGEST_LENGTH;
    UA_Byte * pBuffer = (UA_Byte *) UA_malloc(bufferLen);
    if(pBuffer == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_Openssl_P_SHA1_Ctx * ctx = P_SHA1_Ctx_Create(secret, seed);
    if(ctx == NULL) {
        UA_free(pBuffer);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    for(size_t i = 0; i < iter; i++) {
        UA_StatusCode st =
            P_SHA1_Hash_Generate(ctx, pBuffer + (i * SHA1_DIGEST_LENGTH));
        if(st != UA_STATUSCODE_GOOD) {
            OPENSSL_cleanse(pBuffer, bufferLen);
            UA_free(pBuffer);
            P_SHA1_Ctx_Free(ctx);
            return st;
        }
    }

    memcpy(out->data, pBuffer, keyLen);
    OPENSSL_cleanse(pBuffer, bufferLen);
    UA_free(pBuffer);
    P_SHA1_Ctx_Free(ctx);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_OpenSSL_HMAC_SHA1_Verify(const UA_ByteString *message,
                            const UA_ByteString *key,
                            const UA_ByteString *signature) {
    unsigned char buf[SHA1_DIGEST_LENGTH] = {0};
    UA_ByteString mac = {SHA1_DIGEST_LENGTH, buf};

    if(UA_OpenSSL_HMAC(EVP_sha1(), key, message, &mac) != UA_STATUSCODE_GOOD) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(signature->length != mac.length ||
       !UA_constantTimeEqual(signature->data, mac.data, mac.length))
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_OpenSSL_HMAC_SHA1_Sign(const UA_ByteString *     message,
                          const UA_ByteString *     key,
                          UA_ByteString *           signature) {
    if(UA_OpenSSL_HMAC(EVP_sha1(), key, message, signature) != UA_STATUSCODE_GOOD) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Openssl_RSA_PKCS1_V15_Decrypt(UA_ByteString *       data,
                                 EVP_PKEY * privateKey) {
    return  UA_Openssl_RSA_Private_Decrypt(data, privateKey,
                                           RSA_PKCS1_PADDING, false);
}

UA_StatusCode
UA_Openssl_RSA_PKCS1_V15_Encrypt(UA_ByteString * data,
                                 size_t          paddingSize,
                                 X509 *          publicX509) {
    UA_ByteString message;
    UA_StatusCode ret = UA_ByteString_copy(data, &message);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;
    ret = UA_Openssl_RSA_Public_Encrypt(&message, publicX509,
                                        RSA_PKCS1_PADDING,
                                        paddingSize, data, false);
    UA_ByteString_clear(&message);
    return ret;
}

#ifdef _WIN32
#define strtok_r strtok_s
#endif

static UA_StatusCode
UA_OpenSSL_X509_AddSubjectAttributes(const UA_String* subject, X509_NAME* name) {
    char *subj = (char *)UA_malloc(subject->length + 1);
    if(!subj)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    memset(subj, 0x00, subject->length + 1);
    strncpy(subj, (char *)subject->data, subject->length);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* split string into tokens */
    char *saveptr;
    char *token = strtok_r(subj, "/,", &saveptr);
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
            retval = UA_STATUSCODE_BADINVALIDARGUMENT;
            goto cleanup;
        }

        token[delim] = '\0';
        const unsigned char *para = (const unsigned char*)&token[delim+1];

        /* add attribute to X509_NAME */
        /* Fails for an unknown attribute name */
        int result = X509_NAME_add_entry_by_txt(name, token, MBSTRING_UTF8, para, -1, -1, 0);
        if(!result) {
            retval = UA_STATUSCODE_BADINVALIDARGUMENT;
            goto cleanup;
        }

        /* get next token */
        token = strtok_r(NULL, "/,", &saveptr);
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

    /* A new CSR discards the private key of the previous CSR. The
     * SecurityPolicy must not keep a pointer to the freed key. */
    EVP_PKEY_free(*csrLocalPrivateKey);
    *csrLocalPrivateKey = NULL;

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
    /* The new key becomes the CSR key of the SecurityPolicy only if the CSR is
     * created. It is freed in all error paths. */
    EVP_PKEY *csrKey = NULL;

    /* Get X509 certificate */
    X509 *x509Certificate =
        UA_OpenSSL_LoadCertificate(&securityPolicy->localCertificate, EVP_PKEY_NONE);
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
        subject_alt_name_ext = X509_EXTENSION_dup(X509_get_ext(x509Certificate, pos));
        if(subject_alt_name_ext) {
            /* Set subject alternate name in CSR context */
            sk_X509_EXTENSION_push(exts, subject_alt_name_ext);
        }
    }

    /* Now we've created the extensions we add them to the request */
    X509_REQ_add_extensions(request, exts);
    sk_X509_EXTENSION_free(exts);
    X509_EXTENSION_free(key_usage_ext);
    X509_EXTENSION_free(subject_alt_name_ext);

    /* Get subject from argument or read it from certificate */
    X509_NAME *name = NULL;
    if(subjectName && subjectName->length > 0) {
        name = X509_NAME_new();
        if(!name) {
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        /* add subject attributes to name. An invalid SubjectName returns
         * Bad_InvalidArgument (Part 12, 7.10.10). */
        retval = UA_OpenSSL_X509_AddSubjectAttributes(subjectName, name);
        if(retval != UA_STATUSCODE_GOOD) {
            X509_NAME_free(name);
            goto cleanup;
        }
    } else {
        /* Get subject name from certificate */
        name = X509_NAME_dup(X509_get_subject_name(x509Certificate));
        if(!name) {
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
    }

    /* Set the subject in CSR context */
    if(!X509_REQ_set_subject_name(request, name)) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        X509_NAME_free(name);
        goto cleanup;
    }
    X509_NAME_free(name);

    if(newPrivateKey) {
        int keyType = EVP_PKEY_base_id(localPrivateKey);
        if(keyType == EVP_PKEY_EC) {
            /* Generate a new EC key on the same curve as the local key */
#if(OPENSSL_VERSION_NUMBER >= 0x30000000L)
            char curveName[64];
            size_t curveNameLen = 0;
            if(!EVP_PKEY_get_utf8_string_param(localPrivateKey, "group",
                                               curveName, sizeof(curveName),
                                               &curveNameLen)) {
                retval = UA_STATUSCODE_BADINTERNALERROR;
                goto cleanup;
            }
            csrKey = EVP_PKEY_Q_keygen(NULL, NULL, "EC", curveName);
#else
            int nid = EC_GROUP_get_curve_name(
                EC_KEY_get0_group(EVP_PKEY_get0_EC_KEY(localPrivateKey)));
            EC_KEY *ecKey = EC_KEY_new_by_curve_name(nid);
            if(!ecKey || !EC_KEY_generate_key(ecKey)) {
                if(ecKey) EC_KEY_free(ecKey);
                retval = UA_STATUSCODE_BADINTERNALERROR;
                goto cleanup;
            }
            csrKey = EVP_PKEY_new();
            if(!csrKey || !EVP_PKEY_assign_EC_KEY(csrKey, ecKey)) {
                EC_KEY_free(ecKey);
                retval = UA_STATUSCODE_BADINTERNALERROR;
                goto cleanup;
            }
#endif
        } else {
            /* Generate a new RSA key */
            size_t keySize = 0;
            UA_CertificateUtils_getKeySize(&securityPolicy->localCertificate, &keySize);
#if(OPENSSL_VERSION_NUMBER >= 0x30000000L)
            csrKey = EVP_RSA_gen(keySize);
#else
            BIGNUM *exponent = BN_new();
            csrKey = EVP_PKEY_new();
            RSA *rsa = RSA_new();
            if(!csrKey || !exponent || !rsa) {
                if(exponent)
                    BN_free(exponent);
                if(rsa)
                    RSA_free(rsa);
                retval = UA_STATUSCODE_BADOUTOFMEMORY;
                goto cleanup;
            }

            if(BN_set_word(exponent, RSA_F4) != 1) {
                retval = UA_STATUSCODE_BADINTERNALERROR;
                BN_free(exponent);
                RSA_free(rsa);
                goto cleanup;
            }

            if(RSA_generate_key_ex(rsa, (int) keySize, exponent, NULL) != 1) {
                retval = UA_STATUSCODE_BADINTERNALERROR;
                BN_free(exponent);
                RSA_free(rsa);
                goto cleanup;
            }

            if(EVP_PKEY_assign_RSA(csrKey, rsa) != 1) {
                retval = UA_STATUSCODE_BADINTERNALERROR;
                BN_free(exponent);
                RSA_free(rsa);
                goto cleanup;
            }
            BN_free(exponent);
            /* rsa will be freed by pkey */
            rsa = NULL;
#endif  /* end of OPENSSL_VERSION_NUMBER >= 0x30000000L */
        }
        if(!csrKey) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
        if(UA_OpenSSL_writePrivateKeyDer(csrKey, newPrivateKey) != UA_STATUSCODE_GOOD) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }

        if(!X509_REQ_set_pubkey(request, csrKey)) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }

        if(!X509_REQ_sign(request, csrKey, EVP_sha256())) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
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
    retval = UA_ByteString_allocBuffer(csr, (size_t)csrBufferLength);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Create CSR buffer (DER format) */
    char *ptr = (char*)csr->data;
    i2d_X509_REQ(request, (unsigned char**)&ptr);

    /* The SecurityPolicy keeps the key of the CSR for UpdateCertificate */
    *csrLocalPrivateKey = csrKey;
    csrKey = NULL;

cleanup:
    EVP_PKEY_free(csrKey);
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

    if(result == NULL) {
        /* Try to read PEM encoded private key */
        (void)BIO_reset(bio);
        result = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
    }
    BIO_free(bio);

    return result;
}

X509 *
UA_OpenSSL_LoadCertificate(const UA_ByteString *certificate,
                           int keyType) {
    X509 * result = NULL;

    /* Try to decode DER encoded certificate */
    result = UA_OpenSSL_LoadDerCertificate(certificate);

    if(result == NULL) {
        /* Try to decode PEM encoded certificate */
        result = UA_OpenSSL_LoadPemCertificate(certificate);
    }

    if(!result)
        return NULL;

    /* Don't compare */
    if(keyType == EVP_PKEY_NONE)
        return result;

    EVP_PKEY *pkey = X509_get_pubkey(result);
    if(!pkey) {
        X509_free(result);
        return NULL;
    }

    UA_Boolean good = (EVP_PKEY_base_id(pkey) == keyType);
    EVP_PKEY_free(pkey);
    if(!good) {
        X509_free(result);
        return NULL;
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

    if(crl->length > 1 && pData[0] == 0x30 && pData[1] == 0x82) { // Magic number for DER encoded files
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
UA_OpenSSL_LoadLocalCertificate(const UA_ByteString *certificate,
                                UA_ByteString *target,
                                int keyType) {
    X509 *cert = UA_OpenSSL_LoadCertificate(certificate, keyType);

    if(!cert) {
        UA_ByteString_init(target);
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    }

    unsigned char *derData = NULL;
    int length = i2d_X509(cert, &derData);
    X509_free(cert);

    if(length > 0) {
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

UA_StatusCode
UA_OpenSSL_setLocalSymSigningKey_generic(const UA_SecurityPolicy *policy,
                                         void *channelContext,
                                         const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    openssl_ChannelContext * cc = (openssl_ChannelContext *) channelContext;
    UA_OpenSSL_SymContexts_signingKeyChanged(&cc->sym, true);
    UA_ByteString_clear(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}

UA_StatusCode
UA_OpenSSL_setLocalSymEncryptingKey_generic(const UA_SecurityPolicy *policy,
                                            void *channelContext,
                                            const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    openssl_ChannelContext * cc = (openssl_ChannelContext *) channelContext;
    UA_OpenSSL_SymContexts_encryptingKeyChanged(&cc->sym, true);
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

UA_StatusCode
UA_OpenSSL_setLocalSymIv_generic(const UA_SecurityPolicy *policy,
                                 void *channelContext,
                                 const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    openssl_ChannelContext * cc = (openssl_ChannelContext *) channelContext;
    UA_ByteString_clear(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

UA_StatusCode
UA_OpenSSL_setRemoteSymSigningKey_generic(const UA_SecurityPolicy *policy,
                                          void *channelContext,
                                          const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    openssl_ChannelContext * cc = (openssl_ChannelContext *) channelContext;
    UA_OpenSSL_SymContexts_signingKeyChanged(&cc->sym, false);
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

UA_StatusCode
UA_OpenSSL_setRemoteSymEncryptingKey_generic(const UA_SecurityPolicy *policy,
                                             void *channelContext,
                                             const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    openssl_ChannelContext * cc = (openssl_ChannelContext *) channelContext;
    UA_OpenSSL_SymContexts_encryptingKeyChanged(&cc->sym, false);
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

UA_StatusCode
UA_OpenSSL_setRemoteSymIv_generic(const UA_SecurityPolicy *policy,
                                  void *channelContext,
                                  const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    openssl_ChannelContext * cc = (openssl_ChannelContext *) channelContext;
    UA_ByteString_clear(&cc->remoteSymIv);
    return UA_ByteString_copy(iv, &cc->remoteSymIv);
}

UA_StatusCode
UA_OpenSSL_compareCertificate_generic(const UA_SecurityPolicy *policy,
                                      const void *channelContext,
                                      const UA_ByteString *certificate) {
    if(channelContext == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const openssl_ChannelContext *cc =
        (const openssl_ChannelContext *) channelContext;
    return UA_OpenSSL_X509_compare(certificate, cc->remoteCertificateX509);
}

void
UA_OpenSSL_Policy_clearContext_generic(UA_SecurityPolicy *policy) {
    if(policy == NULL)
        return;

    UA_ByteString_clear(&policy->localCertificate);
    openssl_PolicyContext * ctx = (openssl_PolicyContext *) policy->policyContext;
    if(ctx == NULL)
        return;

    EVP_PKEY_free(ctx->localPrivateKey);
    EVP_PKEY_free(ctx->csrLocalPrivateKey);
    UA_ByteString_clear(&ctx->localCertThumbprint);
    UA_free (ctx);
}

UA_StatusCode
UA_OpenSSL_Policy_newContext_generic(UA_SecurityPolicy *securityPolicy,
                                     const UA_ByteString localPrivateKey,
                                     int keyType,
                                     const UA_Logger *logger) {
    openssl_PolicyContext *context = (openssl_PolicyContext *)
        UA_malloc(sizeof(openssl_PolicyContext));
    if(context == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    context->keyType = keyType;
    context->localPrivateKey = UA_OpenSSL_LoadPrivateKey(&localPrivateKey);
    if(!context->localPrivateKey) {
        UA_free(context);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    context->csrLocalPrivateKey = NULL;

    UA_StatusCode retval =
        UA_Openssl_X509_GetCertificateThumbprint(&securityPolicy->localCertificate,
                                                 &context->localCertThumbprint, true);
    if(retval != UA_STATUSCODE_GOOD) {
        EVP_PKEY_free(context->localPrivateKey);
        UA_free(context);
        return retval;
    }

    securityPolicy->policyContext = context;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_OpenSSL_SecurityPolicy_updateCertificate_generic(UA_SecurityPolicy *securityPolicy,
                                                    const UA_ByteString newCertificate,
                                                    const UA_ByteString newPrivateKey) {
    if(securityPolicy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(securityPolicy->policyContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    openssl_PolicyContext *pc =
        (openssl_PolicyContext *)securityPolicy->policyContext;

    UA_Boolean isLocalKey = false;
    if(newPrivateKey.length <= 0) {
        if(UA_CertificateUtils_comparePublicKeys(&newCertificate, &securityPolicy->localCertificate) == 0)
            isLocalKey = true;
    }

    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString thumbprint = UA_BYTESTRING_NULL;
    UA_StatusCode retval =
        UA_OpenSSL_LoadLocalCertificate(&newCertificate, &certificate,
                                        pc->keyType);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    EVP_PKEY *privateKey = pc->localPrivateKey;
    if(newPrivateKey.length > 0)
        privateKey = UA_OpenSSL_LoadPrivateKey(&newPrivateKey);
    else if(!isLocalKey)
        privateKey = pc->csrLocalPrivateKey;

    if(!privateKey) {
        retval = UA_STATUSCODE_BADNOTSUPPORTED;
        goto error;
    }

    /* The certificate must be issued for the private key. Otherwise every
     * handshake fails after the update. For example, a certificate for the key
     * of an earlier CSR does not match the key of the last CSR. */
    const unsigned char *certData = certificate.data;
    X509 *x509 = d2i_X509(NULL, &certData, (long)certificate.length);
    int keyMatches = x509 ? X509_check_private_key(x509, privateKey) : 0;
    X509_free(x509);
    if(keyMatches != 1) {
        ERR_clear_error();
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    retval = UA_Openssl_X509_GetCertificateThumbprint(&certificate, &thumbprint, true);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    UA_ByteString_clear(&securityPolicy->localCertificate);
    securityPolicy->localCertificate = certificate;

    UA_ByteString_clear(&pc->localCertThumbprint);
    pc->localCertThumbprint = thumbprint;

    if(newPrivateKey.length > 0) {
        EVP_PKEY_free(pc->localPrivateKey);
        pc->localPrivateKey = privateKey;
    } else if(!isLocalKey) {
        EVP_PKEY_free(pc->localPrivateKey);
        pc->localPrivateKey = pc->csrLocalPrivateKey;
        pc->csrLocalPrivateKey = NULL;
    }

    return UA_STATUSCODE_GOOD;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not update certificate and private key");
    UA_ByteString_clear(&certificate);
    if(newPrivateKey.length > 0)
        EVP_PKEY_free(privateKey);
    return retval;
}

UA_StatusCode
UA_OpenSSL_SecurityPolicy_compareCertThumbprint_generic(const UA_SecurityPolicy *securityPolicy,
                                                const UA_ByteString *certificateThumbprint) {
    if(securityPolicy == NULL || certificateThumbprint == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    openssl_PolicyContext *pc = (openssl_PolicyContext *)
                                       securityPolicy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint)) {
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    }
    return UA_STATUSCODE_GOOD;
}

#endif

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)
static UA_StatusCode
UA_OpenSSL_ECDH(const int nid,
                EVP_PKEY * keyPairLocal,
                const UA_ByteString * keyPublicRemote,
                UA_ByteString * sharedSecretOut) {
    EVP_PKEY_CTX * pctx = NULL;
    EVP_PKEY_CTX * kctx = NULL;
    EVP_PKEY * remotePubKey = NULL;
    EVP_PKEY * params = NULL;
    UA_ByteString keyPublicRemoteEncoded = UA_BYTESTRING_NULL;

    /* We need one additional byte of memory for the encoding */
    UA_StatusCode ret = UA_ByteString_allocBuffer(&keyPublicRemoteEncoded,
                                                  keyPublicRemote->length+1);
    if(ret != UA_STATUSCODE_GOOD)
        goto errout;

    /* Set the encoding to 0x04 (uncompressed) and copy the public key */
    keyPublicRemoteEncoded.data[0] = 0x04;
    memcpy(&keyPublicRemoteEncoded.data[1], keyPublicRemote->data,
           keyPublicRemote->length);

    remotePubKey = EVP_PKEY_new();
    if(remotePubKey == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    if(pctx == NULL) {
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

#if defined(LIBRESSL_VERSION_NUMBER)
    /* LibreSSL does currently not support TLS-encoded point APIs required for ECDH */
    ret = UA_STATUSCODE_BADNOTSUPPORTED;
    goto errout;
#elif(OPENSSL_VERSION_NUMBER >= 0x30000000L)
    if(EVP_PKEY_set1_encoded_public_key(remotePubKey, keyPublicRemoteEncoded.data,
                                        keyPublicRemoteEncoded.length) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
#else
    if(EVP_PKEY_set1_tls_encodedpoint(remotePubKey, keyPublicRemoteEncoded.data,
                                      keyPublicRemoteEncoded.length) <= 0) {
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

    ret = UA_ByteString_allocBuffer(sharedSecretOut, sharedSecretOut->length);
    if(ret != UA_STATUSCODE_GOOD)
        goto errout;

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
UA_OpenSSL_ECC_GenerateSalt(const size_t L,
                            const UA_ByteString * label,
                            const UA_ByteString * key1,
                            const UA_ByteString * key2,
                            UA_ByteString * salt) {
    if(salt == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t saltLen = sizeof(uint16_t) + label->length + key1->length + key2->length;
    if(UA_STATUSCODE_GOOD != UA_ByteString_allocBuffer(salt, saltLen))
        return UA_STATUSCODE_BADOUTOFMEMORY;

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

#if(OPENSSL_VERSION_NUMBER >= 0x30000000L)
static UA_StatusCode
UA_OpenSSL_HKDF(char * hashAlgorithm,
                const UA_ByteString * ikm,
                const UA_ByteString * salt,
                const UA_ByteString * info,
                UA_ByteString * out) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    EVP_KDF * kdf = NULL; /* prefetched, not freed here */
    EVP_KDF_CTX * kctx = NULL;
    OSSL_PARAM params[5];
    OSSL_PARAM * p = params;

    kdf = fetchedKdfHkdf();
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
    EVP_KDF_CTX_free(kctx);

    return ret;
}

#else

static UA_StatusCode
UA_OpenSSL_HKDF(char * hashAlgorithm,
                const UA_ByteString * ikm,
                const UA_ByteString * salt,
                const UA_ByteString * info,
                UA_ByteString * out) {

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    EVP_PKEY_CTX *pctx = NULL;
    const EVP_MD *md = NULL;

    /* Retrieve the digest pointer */
    md = EVP_get_digestbyname(hashAlgorithm);
    if(md == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, NULL);
    if(EVP_PKEY_derive_init(pctx) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    if(EVP_PKEY_CTX_set_hkdf_md(pctx, md) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    if(EVP_PKEY_CTX_set1_hkdf_salt(pctx, salt->data, salt->length) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    if(EVP_PKEY_CTX_set1_hkdf_key(pctx, ikm->data, ikm->length) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    if(EVP_PKEY_CTX_add1_hkdf_info(pctx, info->data, info->length) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
    if(EVP_PKEY_derive(pctx, out->data, &out->length) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }
errout:
    EVP_PKEY_CTX_free(pctx);

    return ret;
}
#endif

UA_StatusCode
UA_OpenSSL_ECC_DeriveKeys(const int curveID, char *hashAlgorithm,
                          const UA_ApplicationType applicationType,
                          EVP_PKEY *localEphemeralKeyPair,
                          const UA_ByteString *key1, const UA_ByteString *key2,
                          UA_ByteString *out) {
#if defined(LIBRESSL_VERSION_NUMBER)
    return UA_STATUSCODE_BADNOTSUPPORTED;
#endif

    UA_ByteString sharedSecret = UA_BYTESTRING_NULL;
    UA_ByteString salt = UA_BYTESTRING_NULL;
    UA_ByteString ikm = UA_BYTESTRING_NULL;
    UA_ByteString ikmPrev = UA_BYTESTRING_NULL;
    UA_ByteString *secret = NULL; /* points to either &sharedSecret or &ikm */

    /* Get the local ephemeral public key to use in comparison. The
     * encoded form begins with a single 0x04 byte (for uncompressed
     * points) followed by the X and Y coordinates. The comparison
     * below uses &keyPubEnc[1] as the start, so the expected length
     * of `key1` is (keyPubEncSize - 1). */
    UA_Byte *keyPubEnc = NULL;
#if(OPENSSL_VERSION_NUMBER >= 0x30000000L)
    size_t keyPubEncSize =
        EVP_PKEY_get1_encoded_public_key(localEphemeralKeyPair, &keyPubEnc);
#else
    size_t keyPubEncSize =
        EVP_PKEY_get1_tls_encodedpoint(localEphemeralKeyPair, &keyPubEnc);
#endif
    if(keyPubEncSize <= 0)
        return UA_STATUSCODE_BADINTERNALERROR;
    size_t expectedKey1Len = keyPubEncSize - 1;

    /* OPC UA Part 6 v1.05.07 §6.8.1 step 2 "Extract" (IKM chaining
     * on renewal): the SecureChannel may prepend the previous IKM
     * accumulator to `key1`. Detect this by checking if `key1` is
     * longer than expected. The prepend length is `key1->length -
     * expectedKey1Len` and the IKM is a chain of XORs with the
     * shared secret. */
    UA_ByteString key1Effective = *key1;
    size_t ikmPrependLength = 0;
    if(key1->length > expectedKey1Len) {
        ikmPrependLength = key1->length - expectedKey1Len;
        ikmPrev.data = (UA_Byte*)(uintptr_t)key1->data;
        ikmPrev.length = ikmPrependLength;
        key1Effective.data += ikmPrependLength;
        key1Effective.length -= ikmPrependLength;
    }

    /* The order of ephemeral public keys (key1 and key2) tells us whether we
     * need to generate the local keys or the remote keys. To figure that out,
     * we compare the public part of localEphemeralKeyPair with key1 and
     * key2. */

    /* Determine the label for salt generation, remote ephemeral public key for
     * ECDH, and info for HKDF */
    UA_ByteString* label = NULL;
    const UA_ByteString * remoteEphPubKey = NULL;
    static UA_String serverLabel = UA_STRING_STATIC("opcua-server");
    static UA_String clientLabel = UA_STRING_STATIC("opcua-client");
    static UA_String sessionLabel = UA_STRING_STATIC("opcua-secret");

    /* (Temporary) measure to signal salt generation for sessions. out should
     * always be allocated before this check, so it shouldn't cause problems. */
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if(out->data[0] == 0x03 && out->data[1] == 0x03 && out->data[2] == 0x04 ) {
        label = &sessionLabel;
        if(applicationType == UA_APPLICATIONTYPE_SERVER) {
            remoteEphPubKey = key2;
        } else {
            remoteEphPubKey = &key1Effective;
        }
    }
    /* Comparing from the second byte since the first byte has 0x04 from the encoding */
    else if(memcmp(&keyPubEnc[1], key1Effective.data, key1Effective.length) == 0) {
        /* Key 1 is local ephemeral public key => generating remote keys */
        remoteEphPubKey = key2;
        if(applicationType == UA_APPLICATIONTYPE_SERVER) {
            label = &clientLabel;
        } else {
            label = &serverLabel;
        }
    }
    else if(memcmp(&keyPubEnc[1], key2->data, key2->length) == 0) {
        /* Key 2 is local ephemeral public key => generating local keys */
        remoteEphPubKey = &key1Effective;
        if(applicationType == UA_APPLICATIONTYPE_SERVER) {
            label = &serverLabel;
        } else {
            label = &clientLabel;
        }
    } else {
        /* invalid */
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Use ECDH to calculate shared secret */
    ret = UA_OpenSSL_ECDH(curveID, localEphemeralKeyPair, remoteEphPubKey, &sharedSecret);
    if(ret != UA_STATUSCODE_GOOD) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* IKM chaining: when the SecureChannel provided a previous
     * accumulator, XOR it with the raw shared secret to form the new
     * IKM. The new IKM is then written back into the prefix slot of
     * `key1` so the SecureChannel can pick it up after this call. */
    if(ikmPrependLength > 0) {
        if(ikmPrev.length != sharedSecret.length) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
        UA_StatusCode alloc = UA_ByteString_allocBuffer(&ikm, sharedSecret.length);
        if(alloc != UA_STATUSCODE_GOOD) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
        for(size_t i = 0; i < sharedSecret.length; i++)
            ikm.data[i] = (UA_Byte)(ikmPrev.data[i] ^ sharedSecret.data[i]);
        /* Write the new accumulator back into the prefix slot. The
         * SecureChannel reads it from there to update its own
         * channel->currentIKM. */
        memcpy(ikmPrev.data, ikm.data, ikm.length);
        secret = &ikm;
    } else {
        secret = &sharedSecret;
    }

    /* Calculate salt. The order of the (ephemeral public) keys (key1, key2) is
     * reversed because the caller sends [remote, local] for local key
     * computation and [local, remote] for remote key computation. According to
     * 6.8.1., the local salt computation appends the keys in order [local |
     * remote] and the remote salt computation [remote | local]. Therefore, no
     * additional logic is required, reversing the order is sufficient. The
     * `key1` argument here is the un-prefixed nonce (key1Effective) so the
     * salt only contains the actual nonce data. */
    ret = UA_OpenSSL_ECC_GenerateSalt(out->length, label, key2, &key1Effective, &salt);
    if(ret != UA_STATUSCODE_GOOD) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Call HKDF to derive keys */
    /* Salt is given as the info argument (check 6.8.1., tables 66 and 67) */
    ret = UA_OpenSSL_HKDF(hashAlgorithm, secret, &salt, &salt, out);
    if(ret != UA_STATUSCODE_GOOD) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

errout:
    OPENSSL_free(keyPubEnc);
    UA_ByteString_clear(&sharedSecret);
    UA_ByteString_clear(&salt);
    UA_ByteString_clear(&ikm);

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
    if(pctx == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    if(EVP_PKEY_paramgen_init(pctx) != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, curveId) != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(EVP_PKEY_paramgen(pctx, &params) != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    kctx = EVP_PKEY_CTX_new(params, NULL);
    if(kctx == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(EVP_PKEY_keygen_init(kctx) != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(EVP_PKEY_keygen(kctx, keyPairOut) != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

#if defined(LIBRESSL_VERSION_NUMBER)
    ret = UA_STATUSCODE_BADNOTSUPPORTED;
    goto errout; /* LibreSSL does currently not support TLS-encoded point APIs
                  * required for ECDH */
#elif(OPENSSL_VERSION_NUMBER >= 0x30000000L)
    keyPubEncSize = EVP_PKEY_get1_encoded_public_key(*keyPairOut, &keyPubEnc);
#else
    keyPubEncSize = EVP_PKEY_get1_tls_encodedpoint(*keyPairOut, &keyPubEnc);
#endif
    if(keyPubEncSize <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    UA_assert(keyPubEncSize-1 == keyPublicEncOut->length);

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

    if(privateKey == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    mdctx = EVP_MD_CTX_create();
    if(mdctx == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto errout;
    }

    opensslRet = EVP_DigestSignInit(mdctx, &evpKeyCtx, evpMd, NULL, privateKey);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    opensslRet = EVP_DigestSignUpdate(mdctx, message->data, message->length);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Length required for internal computing */
#if(OPENSSL_VERSION_NUMBER >= 0x30000000L)
    signatureEnc.length = EVP_PKEY_get_size(privateKey);
#else
    signatureEnc.length = ECDSA_size(EVP_PKEY_get0_EC_KEY(privateKey));
#endif

    /* Temporary buffer for OpenSSL-internal signature computation */
    ret = UA_ByteString_allocBuffer(&signatureEnc, signatureEnc.length);
    if(ret != UA_STATUSCODE_GOOD)
        goto errout;

    opensslRet = EVP_DigestSignFinal(mdctx, signatureEnc.data, &signatureEnc.length);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    // using temp pointer since the function call increments it
    const UA_Byte * ptmpData = signatureEnc.data;

    signatureRaw = d2i_ECDSA_SIG(NULL, &ptmpData, signatureEnc.length);
    if(signatureRaw == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    ECDSA_SIG_get0(signatureRaw, &pr, &ps);
    if(pr == NULL || ps == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    sizeEncCoordinate = outSignature->length / 2;
    if(sizeEncCoordinate <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(BN_bn2binpad(pr, outSignature->data, sizeEncCoordinate) <= 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(BN_bn2binpad(ps, outSignature->data + sizeEncCoordinate, sizeEncCoordinate) <= 0) {
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

    if(evpMd == EVP_sha256()) {
        sizeEncCoordinate = 32;
    } else if(evpMd == EVP_sha384()) {
        sizeEncCoordinate = 48;
    } else {
        ret = UA_STATUSCODE_BADINVALIDARGUMENT;
        goto errout;
    }

    if(!signature->data || signature->length != 2 * sizeEncCoordinate) {
        ret = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto errout;
    }

    pr = BN_bin2bn(signature->data, sizeEncCoordinate, NULL);
    ps = BN_bin2bn(signature->data + sizeEncCoordinate, sizeEncCoordinate, NULL);
    if(pr == NULL || ps == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    signatureRaw = ECDSA_SIG_new();
    if(signatureRaw == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    if(ECDSA_SIG_set0(signatureRaw, pr, ps) != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    // this call is to find out the length of the encoded signature
    signatureEnc.length = i2d_ECDSA_SIG(signatureRaw, NULL);
    if(signatureEnc.length == 0) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    ret = UA_ByteString_allocBuffer(&signatureEnc, signatureEnc.length);
    if(ret != UA_STATUSCODE_GOOD)
        goto errout;

    // use temp pointer since the function call increments it
    UA_Byte * ptmpData = signatureEnc.data;
    signatureEnc.length = i2d_ECDSA_SIG(signatureRaw, &ptmpData);
    if(signatureEnc.length == 0) {
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

    opensslRet = EVP_DigestVerifyUpdate(mdctx, message->data, message->length);
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
UA_OpenSSL_ECC_NISTP256_GenerateKey(EVP_PKEY ** keyPairOut,
                                    UA_ByteString * keyPublicEncOut) {
    return UA_OpenSSL_ECC_GenerateKey(EC_curve_nist2nid("P-256"), keyPairOut,
                                      keyPublicEncOut);
}


UA_StatusCode
UA_OpenSSL_ECC_NISTP384_GenerateKey(EVP_PKEY ** keyPairOut,
                                    UA_ByteString * keyPublicEncOut) {
    return UA_OpenSSL_ECC_GenerateKey(EC_curve_nist2nid("P-384"), keyPairOut,
                                      keyPublicEncOut);
}

UA_StatusCode
UA_OpenSSL_ECC_BRAINPOOLP256R1_GenerateKey(EVP_PKEY ** keyPairOut,
                                            UA_ByteString * keyPublicEncOut) {
    return UA_OpenSSL_ECC_GenerateKey(OBJ_txt2nid("brainpoolP256r1"), keyPairOut,
                                      keyPublicEncOut);
}

UA_StatusCode
UA_OpenSSL_ECC_BRAINPOOLP384R1_GenerateKey(EVP_PKEY ** keyPairOut,
                                            UA_ByteString * keyPublicEncOut) {
    return UA_OpenSSL_ECC_GenerateKey(OBJ_txt2nid("brainpoolP384r1"), keyPairOut,
                                      keyPublicEncOut);
}
UA_StatusCode
UA_Openssl_ECDSA_SHA256_Sign(const UA_ByteString * message,
                             EVP_PKEY * privateKey,
                             UA_ByteString * outSignature) {
    return UA_Openssl_ECDSA_Sign(message, privateKey, EVP_sha256(), outSignature);
}

UA_StatusCode
UA_Openssl_ECDSA_SHA256_Verify(const UA_ByteString * message,
                               X509 * publicKeyX509,
                               const UA_ByteString * signature) {
    return UA_Openssl_ECDSA_Verify(message, EVP_sha256(), publicKeyX509, signature);
}


UA_StatusCode
UA_Openssl_ECDSA_SHA384_Sign(const UA_ByteString * message,
                             EVP_PKEY * privateKey,
                             UA_ByteString * outSignature) {
    return UA_Openssl_ECDSA_Sign(message, privateKey, EVP_sha384(), outSignature);
}

UA_StatusCode
UA_Openssl_ECDSA_SHA384_Verify(const UA_ByteString * message,
                               X509 * publicKeyX509,
                               const UA_ByteString * signature) {
    return UA_Openssl_ECDSA_Verify(message, EVP_sha384(), publicKeyX509, signature);
}

UA_StatusCode
UA_OpenSSL_HMAC_SHA384_Verify(const UA_ByteString *message,
                              const UA_ByteString *key,
                              const UA_ByteString *signature) {
    unsigned char buf[SHA384_DIGEST_LENGTH] = {0};
    UA_ByteString mac = {SHA384_DIGEST_LENGTH, buf};

    if(UA_OpenSSL_HMAC(EVP_sha384(), key, message, &mac) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(signature->length != mac.length ||
       !UA_constantTimeEqual(signature->data, mac.data, mac.length))
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_OpenSSL_HMAC_SHA384_Sign(const UA_ByteString *message,
                            const UA_ByteString *key,
                            UA_ByteString *signature) {
    return UA_OpenSSL_HMAC(EVP_sha384(), key, message, signature);
}

/* EdDSA Ed25519 Sign/Verify */

UA_StatusCode
UA_OpenSSL_EdDSA_Ed25519_Sign(const UA_ByteString *message,
                              EVP_PKEY *privateKey,
                              UA_ByteString *outSignature) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if(!mdctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;
    if(EVP_DigestSignInit(mdctx, NULL, NULL, NULL, privateKey) != 1)
        goto errout;

    size_t sigLen = outSignature->length;
    if(EVP_DigestSign(mdctx, outSignature->data, &sigLen,
                      message->data, message->length) != 1)
        goto errout;

    outSignature->length = sigLen;
    ret = UA_STATUSCODE_GOOD;
errout:
    EVP_MD_CTX_free(mdctx);
    return ret;
}

UA_StatusCode
UA_OpenSSL_EdDSA_Ed25519_Verify(const UA_ByteString *message,
                                X509 *publicKeyX509,
                                const UA_ByteString *signature) {
    EVP_PKEY *pubKey = X509_get0_pubkey(publicKeyX509);
    if(!pubKey)
        return UA_STATUSCODE_BADINTERNALERROR;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if(!mdctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;
    if(EVP_DigestVerifyInit(mdctx, NULL, NULL, NULL, pubKey) != 1)
        goto errout;

    if(EVP_DigestVerify(mdctx, signature->data, signature->length,
                        message->data, message->length) != 1) {
        ret = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto errout;
    }

    ret = UA_STATUSCODE_GOOD;
errout:
    EVP_MD_CTX_free(mdctx);
    return ret;
}

/* EdDSA Ed448 Sign/Verify */

UA_StatusCode
UA_OpenSSL_EdDSA_Ed448_Sign(const UA_ByteString *message,
                            EVP_PKEY *privateKey,
                            UA_ByteString *outSignature) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if(!mdctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;
    if(EVP_DigestSignInit(mdctx, NULL, NULL, NULL, privateKey) != 1)
        goto errout;

    size_t sigLen = outSignature->length;
    if(EVP_DigestSign(mdctx, outSignature->data, &sigLen,
                      message->data, message->length) != 1)
        goto errout;

    outSignature->length = sigLen;
    ret = UA_STATUSCODE_GOOD;
errout:
    EVP_MD_CTX_free(mdctx);
    return ret;
}

UA_StatusCode
UA_OpenSSL_EdDSA_Ed448_Verify(const UA_ByteString *message,
                              X509 *publicKeyX509,
                              const UA_ByteString *signature) {
    EVP_PKEY *pubKey = X509_get0_pubkey(publicKeyX509);
    if(!pubKey)
        return UA_STATUSCODE_BADINTERNALERROR;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if(!mdctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;
    if(EVP_DigestVerifyInit(mdctx, NULL, NULL, NULL, pubKey) != 1)
        goto errout;

    if(EVP_DigestVerify(mdctx, signature->data, signature->length,
                        message->data, message->length) != 1) {
        ret = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto errout;
    }

    ret = UA_STATUSCODE_GOOD;
errout:
    EVP_MD_CTX_free(mdctx);
    return ret;
}

/* X25519 / X448 Key Generation */

UA_StatusCode
UA_OpenSSL_X25519_GenerateKey(EVP_PKEY **keyPairOut,
                              UA_ByteString *keyPublicEncOut) {
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
    if(!pctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;
    if(EVP_PKEY_keygen_init(pctx) != 1)
        goto errout;

    if(EVP_PKEY_keygen(pctx, keyPairOut) != 1)
        goto errout;

    /* Get the raw public key (32 bytes for X25519) */
    size_t pubKeyLen = keyPublicEncOut->length;
    if(EVP_PKEY_get_raw_public_key(*keyPairOut, keyPublicEncOut->data, &pubKeyLen) != 1)
        goto errout;

    keyPublicEncOut->length = pubKeyLen;
    ret = UA_STATUSCODE_GOOD;
errout:
    EVP_PKEY_CTX_free(pctx);
    return ret;
}

UA_StatusCode
UA_OpenSSL_X448_GenerateKey(EVP_PKEY **keyPairOut,
                            UA_ByteString *keyPublicEncOut) {
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X448, NULL);
    if(!pctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;
    if(EVP_PKEY_keygen_init(pctx) != 1)
        goto errout;

    if(EVP_PKEY_keygen(pctx, keyPairOut) != 1)
        goto errout;

    /* Get the raw public key (56 bytes for X448) */
    size_t pubKeyLen = keyPublicEncOut->length;
    if(EVP_PKEY_get_raw_public_key(*keyPairOut, keyPublicEncOut->data, &pubKeyLen) != 1)
        goto errout;

    keyPublicEncOut->length = pubKeyLen;
    ret = UA_STATUSCODE_GOOD;
errout:
    EVP_PKEY_CTX_free(pctx);
    return ret;
}

/* X25519/X448 ECDH Key Agreement (XDHE) + HKDF Key Derivation */

static UA_StatusCode
UA_OpenSSL_XDHE(int keyType,
                EVP_PKEY *localEphemeralKeyPair,
                const UA_ByteString *remoteEphPubKey,
                UA_ByteString *sharedSecretOut) {
    /* Create remote public key from raw bytes */
    EVP_PKEY *remotePubKey = EVP_PKEY_new_raw_public_key(
        keyType, NULL, remoteEphPubKey->data, remoteEphPubKey->length);
    if(!remotePubKey)
        return UA_STATUSCODE_BADINTERNALERROR;

    EVP_PKEY_CTX *kctx = EVP_PKEY_CTX_new(localEphemeralKeyPair, NULL);
    if(!kctx) {
        EVP_PKEY_free(remotePubKey);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;
    if(EVP_PKEY_derive_init(kctx) <= 0)
        goto errout;

    if(EVP_PKEY_derive_set_peer(kctx, remotePubKey) <= 0)
        goto errout;

    /* Get shared secret size */
    size_t secretLen = 0;
    if(EVP_PKEY_derive(kctx, NULL, &secretLen) <= 0)
        goto errout;

    ret = UA_ByteString_allocBuffer(sharedSecretOut, secretLen);
    if(ret != UA_STATUSCODE_GOOD)
        goto errout;

    if(EVP_PKEY_derive(kctx, sharedSecretOut->data, &sharedSecretOut->length) <= 0) {
        UA_ByteString_clear(sharedSecretOut);
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    ret = UA_STATUSCODE_GOOD;
errout:
    EVP_PKEY_CTX_free(kctx);
    EVP_PKEY_free(remotePubKey);
    return ret;
}

UA_StatusCode
UA_OpenSSL_XDHE_DeriveKeys(int keyType,
                            const char *hashAlgorithm,
                            UA_ApplicationType applicationType,
                            EVP_PKEY *localEphemeralKeyPair,
                            const UA_ByteString *key1,
                            const UA_ByteString *key2,
                            UA_ByteString *out) {
    UA_ByteString sharedSecret = UA_BYTESTRING_NULL;
    UA_ByteString salt = UA_BYTESTRING_NULL;
    UA_ByteString ikm = UA_BYTESTRING_NULL;
    UA_ByteString ikmPrev = UA_BYTESTRING_NULL;
    UA_ByteString *secret = NULL;

    /* Get the local ephemeral public key for comparison */
    size_t pubKeyLen = 0;
    if(EVP_PKEY_get_raw_public_key(localEphemeralKeyPair, NULL, &pubKeyLen) != 1)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Byte keyPubEnc[64]; /* Max 56 bytes for X448 */
    if(pubKeyLen > sizeof(keyPubEnc))
        return UA_STATUSCODE_BADINTERNALERROR;
    if(EVP_PKEY_get_raw_public_key(localEphemeralKeyPair, keyPubEnc, &pubKeyLen) != 1)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* OPC UA Part 6 v1.05.07 §6.8.1 step 2 IKM chaining. See the
     * matching comment in UA_OpenSSL_ECC_DeriveKeys. The prepend is
     * detected by `key1` being longer than the expected ephemeral
     * public key length. */
    UA_ByteString key1Effective = *key1;
    size_t ikmPrependLength = 0;
    if(key1->length > pubKeyLen) {
        ikmPrependLength = key1->length - pubKeyLen;
        ikmPrev.data = (UA_Byte*)(uintptr_t)key1->data;
        ikmPrev.length = ikmPrependLength;
        key1Effective.data += ikmPrependLength;
        key1Effective.length -= ikmPrependLength;
    }

    /* Determine label and remote ephemeral public key */
    UA_ByteString *label = NULL;
    const UA_ByteString *remoteEphPubKey = NULL;
    static UA_String serverLabel = UA_STRING_STATIC("opcua-server");
    static UA_String clientLabel = UA_STRING_STATIC("opcua-client");
    static UA_String sessionLabel = UA_STRING_STATIC("opcua-secret");

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if(out->data[0] == 0x03 && out->data[1] == 0x03 && out->data[2] == 0x04) {
        label = &sessionLabel;
        if(applicationType == UA_APPLICATIONTYPE_SERVER) {
            remoteEphPubKey = key2;
        } else {
            remoteEphPubKey = &key1Effective;
        }
    } else if(pubKeyLen == key1Effective.length &&
              memcmp(keyPubEnc, key1Effective.data, key1Effective.length) == 0) {
        /* Key 1 is local ephemeral public key => generating remote keys */
        remoteEphPubKey = key2;
        if(applicationType == UA_APPLICATIONTYPE_SERVER)
            label = &clientLabel;
        else
            label = &serverLabel;
    } else if(pubKeyLen == key2->length &&
              memcmp(keyPubEnc, key2->data, key2->length) == 0) {
        /* Key 2 is local ephemeral public key => generating local keys */
        remoteEphPubKey = &key1Effective;
        if(applicationType == UA_APPLICATIONTYPE_SERVER)
            label = &serverLabel;
        else
            label = &clientLabel;
    } else {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* XDHE key agreement */
    ret = UA_OpenSSL_XDHE(keyType, localEphemeralKeyPair, remoteEphPubKey, &sharedSecret);
    if(ret != UA_STATUSCODE_GOOD)
        goto errout;

    /* IKM chaining */
    if(ikmPrependLength > 0) {
        if(ikmPrev.length != sharedSecret.length) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
        UA_StatusCode alloc = UA_ByteString_allocBuffer(&ikm, sharedSecret.length);
        if(alloc != UA_STATUSCODE_GOOD) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
        for(size_t i = 0; i < sharedSecret.length; i++)
            ikm.data[i] = (UA_Byte)(ikmPrev.data[i] ^ sharedSecret.data[i]);
        memcpy(ikmPrev.data, ikm.data, ikm.length);
        secret = &ikm;
    } else {
        secret = &sharedSecret;
    }

    /* Generate salt */
    ret = UA_OpenSSL_ECC_GenerateSalt(out->length, label, key2, &key1Effective, &salt);
    if(ret != UA_STATUSCODE_GOOD)
        goto errout;

    /* HKDF to derive keys */
    ret = UA_OpenSSL_HKDF((char*)(uintptr_t)hashAlgorithm, secret, &salt, &salt, out);

errout:
    UA_ByteString_clear(&sharedSecret);
    UA_ByteString_clear(&salt);
    UA_ByteString_clear(&ikm);
    return ret;
}

#define UA_OPENSSL_AEAD_IV_LENGTH 12
#define UA_OPENSSL_AEAD_TAG_LENGTH 16

/* A context with the key set up. A message only sets the IV. */
static EVP_CIPHER_CTX *
newAeadCtx(const EVP_CIPHER *cipher, const UA_ByteString *key, int seal) {
    const EVP_CIPHER *fetched = UA_OpenSSL_fetchedCipher(cipher);
    if(key->length != (size_t)EVP_CIPHER_key_length(fetched))
        return NULL;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if(!ctx)
        return NULL;
    if(EVP_CipherInit_ex(ctx, fetched, NULL, NULL, NULL, seal) != 1 ||
       EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN,
                           UA_OPENSSL_AEAD_IV_LENGTH, NULL) != 1 ||
       EVP_CipherInit_ex(ctx, NULL, NULL, key->data, NULL, seal) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    return ctx;
}

/* The tag covers aad and data. data is encrypted or decrypted in place only
 * with crypt; otherwise it is authenticated like aad. */
static UA_StatusCode
aeadRun(EVP_CIPHER_CTX *ctx, int seal, const UA_Byte *iv,
        const UA_ByteString *aad, UA_Byte *data, size_t length,
        UA_Boolean crypt, UA_Byte *tag) {
    if((aad && aad->length > INT_MAX) || length > INT_MAX)
        return UA_STATUSCODE_BADINTERNALERROR;
    int outLength = 0;
    UA_Byte finalOut[EVP_MAX_BLOCK_LENGTH];
    if(EVP_CipherInit_ex(ctx, NULL, NULL, NULL, iv, seal) != 1 ||
       (!seal && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG,
                                     UA_OPENSSL_AEAD_TAG_LENGTH, tag) != 1) ||
       (aad && aad->length > 0 &&
        EVP_CipherUpdate(ctx, NULL, &outLength,
                         aad->data, (int)aad->length) != 1) ||
       (length > 0 &&
        EVP_CipherUpdate(ctx, crypt ? data : NULL, &outLength,
                         data, (int)length) != 1))
        return UA_STATUSCODE_BADINTERNALERROR;
    if(EVP_CipherFinal_ex(ctx, finalOut, &outLength) != 1)
        return seal ? UA_STATUSCODE_BADINTERNALERROR
                    : UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    if(seal && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG,
                                   UA_OPENSSL_AEAD_TAG_LENGTH, tag) != 1)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
aeadTemporary(const EVP_CIPHER *cipher, const UA_ByteString *key, int seal,
              const UA_Byte *iv, const UA_ByteString *aad, UA_Byte *data,
              size_t length, UA_Boolean crypt, UA_Byte *tag) {
    EVP_CIPHER_CTX *ctx = newAeadCtx(cipher, key, seal);
    if(!ctx)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_StatusCode res = aeadRun(ctx, seal, iv, aad, data, length, crypt, tag);
    EVP_CIPHER_CTX_free(ctx);
    return res;
}

/* The prepared context, or a temporary one while another thread uses it */
static UA_StatusCode
aeadPrepared(EVP_CIPHER_CTX **slot, UA_atomic(uintptr_t) *busy,
             const EVP_CIPHER *cipher, const UA_ByteString *key, int seal,
             const UA_ByteString *iv, const UA_ByteString *aad,
             UA_Byte *data, size_t length, UA_Boolean crypt, UA_Byte *tag) {
    if(iv->length != UA_OPENSSL_AEAD_IV_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(!tryAcquireCtx(busy))
        return aeadTemporary(cipher, key, seal, iv->data, aad, data, length,
                             crypt, tag);
    if(!*slot)
        *slot = newAeadCtx(cipher, key, seal);
    UA_StatusCode res = UA_STATUSCODE_BADINTERNALERROR;
    if(*slot)
        res = aeadRun(*slot, seal, iv->data, aad, data, length, crypt, tag);
    if(res == UA_STATUSCODE_BADINTERNALERROR)
        freeCipherCtx(slot); /* rebuilt on the next use */
    releaseCtx(busy);
    return res;
}

/* data ends with the tag */
static UA_StatusCode
aeadOneShot(const EVP_CIPHER *cipher, int seal, const UA_ByteString *iv,
            const UA_ByteString *key, const UA_ByteString *aad,
            UA_ByteString *data, UA_Boolean crypt) {
    if(iv->length != UA_OPENSSL_AEAD_IV_LENGTH ||
       data->length < UA_OPENSSL_AEAD_TAG_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;
    size_t length = data->length - UA_OPENSSL_AEAD_TAG_LENGTH;
    UA_StatusCode res = aeadTemporary(cipher, key, seal, iv->data, aad,
                                      data->data, length, crypt,
                                      data->data + length);
    if(res == UA_STATUSCODE_GOOD && !seal)
        data->length = length;
    return res;
}

UA_StatusCode
UA_OpenSSL_ChaCha20Poly1305_Encrypt(const UA_ByteString *iv,
                                    const UA_ByteString *key,
                                    const UA_ByteString *aad,
                                    UA_ByteString *data,
                                    UA_Boolean encryptData) {
    return aeadOneShot(EVP_chacha20_poly1305(), 1, iv, key, aad, data,
                       encryptData);
}

UA_StatusCode
UA_OpenSSL_ChaCha20Poly1305_Decrypt(const UA_ByteString *iv,
                                    const UA_ByteString *key,
                                    const UA_ByteString *aad,
                                    UA_ByteString *data,
                                    UA_Boolean decryptData) {
    return aeadOneShot(EVP_chacha20_poly1305(), 0, iv, key, aad, data,
                       decryptData);
}

UA_StatusCode
UA_OpenSSL_AES_128_GCM_Encrypt(const UA_ByteString *iv,
                               const UA_ByteString *key,
                               const UA_ByteString *aad,
                               UA_ByteString *data,
                               UA_Boolean encryptData) {
    return aeadOneShot(EVP_aes_128_gcm(), 1, iv, key, aad, data, encryptData);
}

UA_StatusCode
UA_OpenSSL_AES_128_GCM_Decrypt(const UA_ByteString *iv,
                               const UA_ByteString *key,
                               const UA_ByteString *aad,
                               UA_ByteString *data,
                               UA_Boolean decryptData) {
    return aeadOneShot(EVP_aes_128_gcm(), 0, iv, key, aad, data, decryptData);
}

UA_StatusCode
UA_OpenSSL_aeadMessageIv(const UA_ByteString *baseIv, UA_UInt32 tokenId,
                         UA_UInt32 previousSequenceNumber, UA_Byte iv[12]) {
    if(baseIv->length < UA_OPENSSL_AEAD_IV_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;
    memcpy(iv, baseIv->data, UA_OPENSSL_AEAD_IV_LENGTH);
    for(size_t i = 0; i < 4; i++) {
        iv[i] ^= (UA_Byte)(tokenId >> (8 * i));
        iv[4 + i] ^= (UA_Byte)(previousSequenceNumber >> (8 * i));
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_OpenSSL_Sym_aeadEncrypt(UA_OpenSSL_SymContexts *sym, const EVP_CIPHER *cipher,
                           const UA_ByteString *key, const UA_ByteString *iv,
                           const UA_ByteString *aad, UA_ByteString *data) {
    if(data->length < UA_OPENSSL_AEAD_TAG_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;
    size_t length = data->length - UA_OPENSSL_AEAD_TAG_LENGTH;
    return aeadPrepared(&sym->localCipher, &sym->localCipherBusy, cipher, key,
                        1, iv, aad, data->data, length, true,
                        data->data + length);
}

UA_StatusCode
UA_OpenSSL_Sym_aeadDecrypt(UA_OpenSSL_SymContexts *sym, const EVP_CIPHER *cipher,
                           const UA_ByteString *key, const UA_ByteString *iv,
                           const UA_ByteString *aad, UA_ByteString *data) {
    if(data->length < UA_OPENSSL_AEAD_TAG_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;
    size_t length = data->length - UA_OPENSSL_AEAD_TAG_LENGTH;
    UA_StatusCode res =
        aeadPrepared(&sym->remoteCipher, &sym->remoteCipherBusy, cipher, key,
                     0, iv, aad, data->data, length, true, data->data + length);
    if(res == UA_STATUSCODE_GOOD)
        data->length = length;
    return res;
}

UA_StatusCode
UA_OpenSSL_Sym_aeadSign(UA_OpenSSL_SymContexts *sym, const EVP_CIPHER *cipher,
                        const UA_ByteString *key, const UA_ByteString *iv,
                        const UA_ByteString *message, UA_ByteString *signature) {
    if(signature->length < UA_OPENSSL_AEAD_TAG_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_StatusCode res =
        aeadPrepared(&sym->localTag, &sym->localTagBusy, cipher, key, 1, iv,
                     message, NULL, 0, false, signature->data);
    if(res == UA_STATUSCODE_GOOD)
        signature->length = UA_OPENSSL_AEAD_TAG_LENGTH;
    return res;
}

UA_StatusCode
UA_OpenSSL_Sym_aeadVerify(UA_OpenSSL_SymContexts *sym, const EVP_CIPHER *cipher,
                          const UA_ByteString *key, const UA_ByteString *iv,
                          const UA_ByteString *message,
                          const UA_ByteString *signature) {
    if(signature->length < UA_OPENSSL_AEAD_TAG_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return aeadPrepared(&sym->remoteTag, &sym->remoteTagBusy, cipher, key, 0,
                        iv, message, NULL, 0, false, signature->data);
}


#endif
