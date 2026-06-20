/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Siemens AG (Authors: Tin Raic, Thomas Zeschg)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 *
 * SecurityPolicy [ECC A] – ECC-nistP256-ChaChaPoly Profile
 *   http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256_ChaChaPoly
 *
 * This is the new (non-deprecated) profile, supersedes
 *   http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256
 * Differences from the deprecated policy:
 *   - Symmetric signing/encryption: HMAC-SHA2-256 + AES-128-CBC
 *     replaced by ChaCha20-Poly1305 AEAD (16-byte tag, 12-byte nonce).
 *   - Policy type: UA_SECURITYPOLICYTYPE_ECC_AEAD (was _ECC).
 *   - Adds setMessageSecurityParameters for AEAD nonce masking.
 * Asymmetric signature (ECDSA-SHA2-256) and key derivation
 * (HKDF-SHA2-256 over P-256 ECDHE) are unchanged.
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)

#include "securitypolicy_common.h"
#include <openssl/rand.h>

#define UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_ASYM_SIGNING_KEY_LENGTH 32
#define UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_ASYM_SIGNATURE_LENGTH 64
#define UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_ENCRYPTION_KEY_LENGTH 32
/* ChaCha20-Poly1305 is an AEAD cipher: there is NO separate symmetric signing
 * key. The derived key material is only [EncryptionKey(16) | IV(12)] =
 * 28 bytes, and the GCM authentication tag is produced with the
 * encryption key. (OPC UA Part 6 v1.05: ECC_nistP256_ChaChaPoly has
 * DerivedSignatureKeyLength = 0; cf. UA-.NETStandard SecurityPolicyInfo.) */
#define UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_SIGNING_KEY_LENGTH 0
#define UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_SIGNATURE_LENGTH 16
#define UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_IV_LENGTH 12
#define UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_NONCE_LENGTH_BYTES 64

typedef struct {
    EVP_PKEY *localPrivateKey;
    EVP_PKEY *csrLocalPrivateKey;
    UA_ByteString localCertThumbprint;
    UA_ApplicationType applicationType;
} Policy_Context_EccNistP256ChaChaPoly;

typedef struct Channel_Context_EccNistP256ChaChaPoly {
    EVP_PKEY *    localEphemeralKeyPair;
    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;
    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    UA_ByteString remoteCertificate;
    X509 *remoteCertificateX509; /* X509 */

    /* AEAD message context (mirrors securitypolicy_ecccurve448.c) */
    UA_UInt32 tokenId;
    UA_UInt32 previousSequenceNumber;
    UA_ByteString additionalAuthData;
} Channel_Context_EccNistP256ChaChaPoly;

/* Compute the per-message ChaCha20-Poly1305 nonce.
 * For GCM, the 12-byte IV is formed by XORing the tokenId into the first
 * 4 bytes and the previous sequence number into the next 4 bytes of the
 * base (local/remote) Sym IV. The last 4 bytes are unchanged. */
static void
computeMaskedIvChaChaPoly(UA_Byte *iv_out, const UA_ByteString *base_iv,
                      UA_UInt32 tokenId, UA_UInt32 seqNo) {
    memcpy(iv_out, base_iv->data, 12);
    iv_out[0] ^= (UA_Byte)(tokenId & 0xFF);
    iv_out[1] ^= (UA_Byte)((tokenId >> 8) & 0xFF);
    iv_out[2] ^= (UA_Byte)((tokenId >> 16) & 0xFF);
    iv_out[3] ^= (UA_Byte)((tokenId >> 24) & 0xFF);
    iv_out[4] ^= (UA_Byte)(seqNo & 0xFF);
    iv_out[5] ^= (UA_Byte)((seqNo >> 8) & 0xFF);
    iv_out[6] ^= (UA_Byte)((seqNo >> 16) & 0xFF);
    iv_out[7] ^= (UA_Byte)((seqNo >> 24) & 0xFF);
}

/* --- Policy Context --- */

static UA_StatusCode
UA_Policy_EccNistP256ChaChaPoly_New_Context(UA_SecurityPolicy *securityPolicy,
                                        const UA_ByteString localPrivateKey,
                                        const UA_ApplicationType applicationType,
                                        const UA_Logger *logger) {
    Policy_Context_EccNistP256ChaChaPoly *context = (Policy_Context_EccNistP256ChaChaPoly *)
        UA_malloc(sizeof(Policy_Context_EccNistP256ChaChaPoly));
    if(!context)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    context->localPrivateKey = UA_OpenSSL_LoadPrivateKey(&localPrivateKey);
    if(!context->localPrivateKey) {
        UA_free(context);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    context->csrLocalPrivateKey = NULL;

    UA_StatusCode retval = UA_Openssl_X509_GetCertificateThumbprint(
        &securityPolicy->localCertificate, &context->localCertThumbprint, true);
    if(retval != UA_STATUSCODE_GOOD) {
        EVP_PKEY_free(context->localPrivateKey);
        UA_free(context);
        return retval;
    }

    context->applicationType = applicationType;

    securityPolicy->policyContext = context;
    return UA_STATUSCODE_GOOD;
}

static void
UA_Policy_EccNistP256ChaChaPoly_Clear_Context(UA_SecurityPolicy *policy) {
    if(!policy || !policy->policyContext)
        return;

    UA_ByteString_clear(&policy->localCertificate);

    /* Delete all allocated members in the context */
    Policy_Context_EccNistP256ChaChaPoly *pc =
        (Policy_Context_EccNistP256ChaChaPoly *)policy->policyContext;
    if(!pc)
        return;

    EVP_PKEY_free(pc->localPrivateKey);
    EVP_PKEY_free(pc->csrLocalPrivateKey);
    UA_ByteString_clear(&pc->localCertThumbprint);
    UA_free(pc);
}

static UA_StatusCode
createSigningRequest_sp_eccnistp256chachapoly(UA_SecurityPolicy *securityPolicy,
                                          const UA_String *subjectName,
                                          const UA_ByteString *nonce,
                                          const UA_KeyValueMap *params,
                                          UA_ByteString *csr,
                                          UA_ByteString *newPrivateKey) {
    if(!securityPolicy || !csr)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
     if(!securityPolicy->policyContext)
         return UA_STATUSCODE_BADINTERNALERROR;
    Policy_Context_EccNistP256ChaChaPoly *pc =
        (Policy_Context_EccNistP256ChaChaPoly*)securityPolicy->policyContext;
    return UA_OpenSSL_CreateSigningRequest(pc->localPrivateKey,
                                           &pc->csrLocalPrivateKey,
                                           securityPolicy, subjectName,
                                           nonce, csr, newPrivateKey);
}

static UA_StatusCode
updateCertificateAndPrivateKey_sp_EccNistP256ChaChaPoly(UA_SecurityPolicy *securityPolicy,
                                                    const UA_ByteString newCertificate,
                                                    const UA_ByteString newPrivateKey) {
    if(!securityPolicy || !securityPolicy->policyContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    Policy_Context_EccNistP256ChaChaPoly *pc =
        (Policy_Context_EccNistP256ChaChaPoly *)securityPolicy->policyContext;

    /* Set the certificate */
    UA_ByteString_clear(&securityPolicy->localCertificate);
    UA_StatusCode retval = UA_OpenSSL_LoadLocalCertificate(
        &newCertificate, &securityPolicy->localCertificate,
        EVP_PKEY_EC);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the new private key */
    EVP_PKEY_free(pc->localPrivateKey);
    pc->localPrivateKey = UA_OpenSSL_LoadPrivateKey(&newPrivateKey);
    if(!pc->localPrivateKey) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    /* Update the thumbprint */
    UA_ByteString_clear(&pc->localCertThumbprint);
    retval = UA_Openssl_X509_GetCertificateThumbprint(&securityPolicy->localCertificate,
                                                      &pc->localCertThumbprint, true);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    return UA_STATUSCODE_GOOD;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not update certificate and private key");
    if(securityPolicy->policyContext)
        UA_Policy_EccNistP256ChaChaPoly_Clear_Context(securityPolicy);
    return retval;
}

/* --- Channel Context --- */

static UA_StatusCode
EccNistP256ChaChaPoly_New_Context(const UA_SecurityPolicy *securityPolicy,
                              const UA_ByteString *remoteCertificate,
                              void **channelContext) {
    if(securityPolicy == NULL || remoteCertificate == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Channel_Context_EccNistP256ChaChaPoly *newContext = (Channel_Context_EccNistP256ChaChaPoly *)
        UA_calloc(1, sizeof(Channel_Context_EccNistP256ChaChaPoly));
    if(!newContext)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval =
        UA_copyCertificate(&newContext->remoteCertificate, remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(newContext);
        return retval;
    }

    /* Decode to X509 */
    newContext->remoteCertificateX509 =
        UA_OpenSSL_LoadCertificate(&newContext->remoteCertificate, EVP_PKEY_EC);
    if(newContext->remoteCertificateX509 == NULL) {
        UA_ByteString_clear (&newContext->remoteCertificate);
        UA_free (newContext);
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
    }

    /* Return the new channel context */
    *channelContext = newContext;

    return UA_STATUSCODE_GOOD;
}

static void
EccNistP256ChaChaPoly_Delete_Context(const UA_SecurityPolicy *policy,
                                 void *channelContext) {
    if(!channelContext)
        return;
    Channel_Context_EccNistP256ChaChaPoly *cc =
        (Channel_Context_EccNistP256ChaChaPoly *)channelContext;
    X509_free(cc->remoteCertificateX509);
    UA_ByteString_clear(&cc->remoteCertificate);
    UA_ByteString_clear(&cc->localSymSigningKey);
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    UA_ByteString_clear(&cc->localSymIv);
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    UA_ByteString_clear(&cc->remoteSymIv);
    UA_ByteString_clear(&cc->additionalAuthData);
    EVP_PKEY_free(cc->localEphemeralKeyPair);
    UA_free(cc);
}

/* --- AEAD Message Security Parameters --- */

static UA_StatusCode
EccNistP256ChaChaPoly_setMessageSecurityParameters(const UA_SecurityPolicy *policy,
                                                void *channelContext,
                                                UA_UInt32 tokenId,
                                                UA_UInt32 previousSequenceNumber,
                                                const UA_ByteString *additionalAuthData) {
    if(!channelContext)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256ChaChaPoly *cc =
        (Channel_Context_EccNistP256ChaChaPoly *)channelContext;
    cc->tokenId = tokenId;
    cc->previousSequenceNumber = previousSequenceNumber;

    UA_ByteString_clear(&cc->additionalAuthData);
    if(additionalAuthData && additionalAuthData->length > 0)
        return UA_ByteString_copy(additionalAuthData, &cc->additionalAuthData);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_compareCertificateThumbprint_EccNistP256ChaChaPoly(const UA_SecurityPolicy *policy,
                                                  const UA_ByteString *certificateThumbprint) {
    if(policy == NULL || certificateThumbprint == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Policy_Context_EccNistP256ChaChaPoly *pc =
        (Policy_Context_EccNistP256ChaChaPoly *)policy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint))
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_makeCertificateThumbprint_EccNistP256ChaChaPoly(const UA_SecurityPolicy *securityPolicy,
                                               const UA_ByteString *certificate,
                                               UA_ByteString *thumbprint) {
    return UA_Openssl_X509_GetCertificateThumbprint(certificate, thumbprint, false);
}

static size_t
UA_Asym_EccNistP256ChaChaPoly_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                                 const void *channelContext) {
    /* According to the standard, the server and the client must agree on the
     * security mode and security policy. Therefore, it can be assumed that
     * the remote asymmetric cryptographic tokens have the same sizes as the
     * local ones. */
    return UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_ASYM_SIGNATURE_LENGTH;
}

static size_t
UA_AsySig_EccNistP256ChaChaPoly_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                                                  const void *channelContext) {
    return UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_ASYM_SIGNATURE_LENGTH;
}

static size_t
UA_AsymEn_EccNistP256ChaChaPoly_getRemotePlainTextBlockSize(const UA_SecurityPolicy *policy,
                                                        const void *channelContext) {
    return 1;
}

static size_t
UA_AsymEn_EccNistP256ChaChaPoly_getRemoteBlockSize(const UA_SecurityPolicy *policy,
                                               const void *channelContext) {
    return 1;
}

static size_t
UA_AsymEn_EccNistP256ChaChaPoly_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                               const void *channelContext) {
    /* According to the standard, the server and the client must agree on the
     * security mode and security policy. Therefore, it can be assumed that the
     * remote asymmetric cryptographic tokens have the same sizes as the local
     * ones.
     *
     * No ECC encryption -> key length set to 1 to avoid division or
     * multiplication with 0 */
    return 1;
}

static UA_StatusCode
UA_Sym_EccNistP256ChaChaPoly_generateNonce(const UA_SecurityPolicy *policy,
                                       void *channelContext, UA_ByteString *out) {
    Policy_Context_EccNistP256ChaChaPoly* pctx =
        (Policy_Context_EccNistP256ChaChaPoly*)policy->policyContext;
    if(!pctx)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;

    /* Detect if we want to create an ephemeral key or just cryptographic random
     * data */
    if(out->data[0] == 'e' && out->data[1] == 'p' && out->data[2] == 'h') {
        Channel_Context_EccNistP256ChaChaPoly *cctx =
            (Channel_Context_EccNistP256ChaChaPoly*)channelContext;
        return UA_OpenSSL_ECC_NISTP256_GenerateKey(&cctx->localEphemeralKeyPair, out);
    }

    UA_Int32 rc = RAND_bytes(out->data, (int)out->length);
    return (rc == 1) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADUNEXPECTEDERROR;
}

static size_t
UA_SymEn_EccNistP256ChaChaPoly_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                             const void *channelContext) {
    /* 16 bytes 128 bits */
    return UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymSig_EccNistP256ChaChaPoly_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                              const void *channelContext) {
    /* 16 bytes 128 bits */
    return UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
UA_Sym_EccNistP256ChaChaPoly_generateKey(const UA_SecurityPolicy *policy,
                                     void *channelContext, const UA_ByteString *secret,
                                     const UA_ByteString *seed, UA_ByteString *out) {
    Policy_Context_EccNistP256ChaChaPoly* pctx =
        (Policy_Context_EccNistP256ChaChaPoly *)policy->policyContext;
    Channel_Context_EccNistP256ChaChaPoly *cc =
        (Channel_Context_EccNistP256ChaChaPoly *)channelContext;
    if(!pctx || !cc)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_OpenSSL_ECC_DeriveKeys(EC_curve_nist2nid("P-256"), "SHA256",
                                     pctx->applicationType, cc->localEphemeralKeyPair,
                                     secret, seed, out);
}

static UA_StatusCode
EccNistP256ChaChaPoly_setLocalSymSigningKey(const UA_SecurityPolicy *policy,
                                         void *channelContext,
                                         const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256ChaChaPoly *cc =
        (Channel_Context_EccNistP256ChaChaPoly *)channelContext;
    UA_ByteString_clear(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}

static UA_StatusCode
EccNistP256ChaChaPoly_setLocalSymEncryptingKey(const UA_SecurityPolicy *policy,
                                           void *channelContext,
                                           const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256ChaChaPoly *cc =
        (Channel_Context_EccNistP256ChaChaPoly *)channelContext;
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

static UA_StatusCode
EccNistP256ChaChaPoly_setLocalSymIv(const UA_SecurityPolicy *policy,
                                void *channelContext,
                                const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256ChaChaPoly *cc =
        (Channel_Context_EccNistP256ChaChaPoly *)channelContext;
    UA_ByteString_clear(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

static size_t
UA_SymEn_EccNistP256ChaChaPoly_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                              const void *channelContext) {
    /* 16 bytes 128 bits */
    return UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymEn_EccNistP256ChaChaPoly_getBlockSize(const UA_SecurityPolicy *policy,
                                        const void *channelContext) {
    /* AEAD ciphers are stream-like -> 1 byte */
    return 1;
}

static size_t
UA_SymEn_EccNistP256ChaChaPoly_getIvLength(const UA_SecurityPolicy *policy,
                                       const void *channelContext) {
    return UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_IV_LENGTH;
}

static size_t
UA_SymSig_EccNistP256ChaChaPoly_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                               const void *channelContext) {
    /* 16 bytes 128 bits */
    return UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
EccNistP256ChaChaPoly_setRemoteSymSigningKey(const UA_SecurityPolicy *policy,
                                          void *channelContext,
                                          const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256ChaChaPoly *cc =
        (Channel_Context_EccNistP256ChaChaPoly *)channelContext;
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

static UA_StatusCode
EccNistP256ChaChaPoly_setRemoteSymEncryptingKey(const UA_SecurityPolicy *policy,
                                            void *channelContext,
                                            const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256ChaChaPoly *cc =
        (Channel_Context_EccNistP256ChaChaPoly *)channelContext;
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

static UA_StatusCode
EccNistP256ChaChaPoly_setRemoteSymIv(const UA_SecurityPolicy *policy,
                                 void *channelContext, const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256ChaChaPoly *cc =
        (Channel_Context_EccNistP256ChaChaPoly *)channelContext;
    UA_ByteString_clear(&cc->remoteSymIv);
    return UA_ByteString_copy(iv, &cc->remoteSymIv);
}

/* --- Asymmetric Signature (ECDSA-SHA2-256, unchanged from the deprecated
 *     policy) --- */

static UA_StatusCode
UA_AsymSig_EccNistP256ChaChaPoly_sign(const UA_SecurityPolicy *policy,
                                  void *channelContext, const UA_ByteString *message,
                                  UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Policy_Context_EccNistP256ChaChaPoly *pc =
        (Policy_Context_EccNistP256ChaChaPoly *)policy->policyContext;
    return UA_Openssl_ECDSA_SHA256_Sign(message, pc->localPrivateKey, signature);
}


static UA_StatusCode
UA_AsymSig_EccNistP256ChaChaPoly_verify(const UA_SecurityPolicy *policy, void *channelContext,
                                    const UA_ByteString *message,
                                    const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256ChaChaPoly *cc =
        (Channel_Context_EccNistP256ChaChaPoly *)channelContext;
    return UA_Openssl_ECDSA_SHA256_Verify(message, cc->remoteCertificateX509, signature);
}

static UA_StatusCode
UA_Asym_EccNistP256ChaChaPoly_Dummy(const UA_SecurityPolicy *policy,
                                void *channelContext, UA_ByteString *data) {
    return UA_STATUSCODE_GOOD; /* Do nothing and return true */
}

/* --- Symmetric Signature (ChaCha20-Poly1305 AEAD, sign-only) --- */

static size_t
UA_SymSig_EccNistP256ChaChaPoly_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                                   const void *channelContext) {
    return UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_SIGNATURE_LENGTH;
}

static UA_StatusCode
UA_SymSig_EccNistP256ChaChaPoly_verify(const UA_SecurityPolicy *policy, void *channelContext,
                                   const UA_ByteString *message, const UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(signature->length < UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_SIGNATURE_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    Channel_Context_EccNistP256ChaChaPoly *cc =
        (Channel_Context_EccNistP256ChaChaPoly *)channelContext;

    /* Compute masked IV using the remote IV */
    UA_Byte maskedIv[12];
    computeMaskedIvChaChaPoly(maskedIv, &cc->remoteSymIv,
                          cc->tokenId, cc->previousSequenceNumber);

    /* Use ChaCha20-Poly1305 AEAD to verify the Poly1305 tag.
     * Treat the entire message as AAD (sign-only). The AEAD tag is
     * produced/verified with the encryption key (no separate signing
     * key for AEAD policies). */
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if(!ctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;
    if(EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, NULL, NULL) != 1)
        goto errout;
    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, 12, NULL) != 1)
        goto errout;
    if(EVP_DecryptInit_ex(ctx, NULL, NULL,
                          cc->remoteSymEncryptingKey.data, maskedIv) != 1)
        goto errout;

    /* Set the expected tag for verification */
    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG,
                           UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_SIGNATURE_LENGTH,
                           (void*)(uintptr_t)signature->data) != 1)
        goto errout;

    /* Process the entire message as AAD */
    int outl = 0;
    if(EVP_DecryptUpdate(ctx, NULL, &outl,
                         message->data, (int)message->length) != 1)
        goto errout;

    /* Finalize triggers tag verification */
    UA_Byte dummy;
    if(EVP_DecryptFinal_ex(ctx, &dummy, &outl) != 1) {
        ret = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto errout;
    }
    ret = UA_STATUSCODE_GOOD;

errout:
    EVP_CIPHER_CTX_free(ctx);
    return ret;
}

static UA_StatusCode
UA_SymSig_EccNistP256ChaChaPoly_sign(const UA_SecurityPolicy *policy,
                                 void *channelContext, const UA_ByteString *message,
                                 UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(signature->length < UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_SIGNATURE_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;

    Channel_Context_EccNistP256ChaChaPoly *cc =
        (Channel_Context_EccNistP256ChaChaPoly *)channelContext;

    /* Compute masked IV (same nonce derivation as encrypt mode) */
    UA_Byte maskedIv[12];
    computeMaskedIvChaChaPoly(maskedIv, &cc->localSymIv,
                          cc->tokenId, cc->previousSequenceNumber);

    /* Use ChaCha20-Poly1305 AEAD to compute the Poly1305 tag.
     * For sign-only: treat the entire message as AAD (no encryption).
     * The AEAD tag is produced with the encryption key (no separate
     * signing key for AEAD policies). */
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if(!ctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;
    if(EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, NULL, NULL) != 1)
        goto errout;
    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, 12, NULL) != 1)
        goto errout;
    if(EVP_EncryptInit_ex(ctx, NULL, NULL,
                          cc->localSymEncryptingKey.data, maskedIv) != 1)
        goto errout;

    /* Process the entire message as AAD (authenticated, not encrypted) */
    int outl = 0;
    if(EVP_EncryptUpdate(ctx, NULL, &outl,
                         message->data, (int)message->length) != 1)
        goto errout;

    /* Finalize with no plaintext */
    UA_Byte dummy;
    if(EVP_EncryptFinal_ex(ctx, &dummy, &outl) != 1)
        goto errout;

    /* Get the 16-byte GCM authentication tag */
    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG,
                           UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_SIGNATURE_LENGTH,
                           signature->data) != 1)
        goto errout;
    signature->length = UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_SIGNATURE_LENGTH;
    ret = UA_STATUSCODE_GOOD;

errout:
    EVP_CIPHER_CTX_free(ctx);
    return ret;
}

static size_t
UA_SymSig_EccNistP256ChaChaPoly_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                                                  const void *channelContext) {
    return UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_SYM_SIGNATURE_LENGTH;
}

/* --- Symmetric Encryption (ChaCha20-Poly1305 AEAD) --- */

static UA_StatusCode
UA_SymEn_EccNistP256ChaChaPoly_decrypt(const UA_SecurityPolicy *policy,
                                   void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256ChaChaPoly *cc =
        (Channel_Context_EccNistP256ChaChaPoly *)channelContext;

    UA_Byte maskedIv[12];
    computeMaskedIvChaChaPoly(maskedIv, &cc->remoteSymIv,
                          cc->tokenId, cc->previousSequenceNumber);
    UA_ByteString ivBs = {12, maskedIv};

    return UA_OpenSSL_ChaCha20Poly1305_Decrypt(
        &ivBs, &cc->remoteSymEncryptingKey, &cc->additionalAuthData,
        data, true);
}

static UA_StatusCode
UA_SymEn_EccNistP256ChaChaPoly_encrypt(const UA_SecurityPolicy *policy,
                                   void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256ChaChaPoly *cc =
        (Channel_Context_EccNistP256ChaChaPoly *)channelContext;

    UA_Byte maskedIv[12];
    computeMaskedIvChaChaPoly(maskedIv, &cc->localSymIv,
                          cc->tokenId, cc->previousSequenceNumber);
    UA_ByteString ivBs = {12, maskedIv};

    return UA_OpenSSL_ChaCha20Poly1305_Encrypt(
        &ivBs, &cc->localSymEncryptingKey, &cc->additionalAuthData,
        data, true);
}

static UA_StatusCode
EccNistP256ChaChaPoly_compareCertificate(const UA_SecurityPolicy *policy,
                                     const void *channelContext,
                                     const UA_ByteString *certificate) {
    if(channelContext == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const Channel_Context_EccNistP256ChaChaPoly *cc =
        (const Channel_Context_EccNistP256ChaChaPoly *)channelContext;
    return UA_OpenSSL_X509_compare(certificate, cc->remoteCertificateX509);
}

static size_t
UA_AsymEn_EccNistP256ChaChaPoly_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                              const void *channelContext) {
    /* No ECC encryption -> key length set to 1 to avoid division or
     * multiplication with 0 */
    return 1;
}

/* --- Public Entry Point --- */

UA_StatusCode
UA_SecurityPolicy_EccNistP256ChaChaPoly(UA_SecurityPolicy *sp,
                                    const UA_ApplicationType applicationType,
                                    const UA_ByteString localCertificate,
                                    const UA_ByteString localPrivateKey,
                                    const UA_Logger *logger) {
    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = logger;
    sp->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256_ChaChaPoly\0");
    sp->certificateGroupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    sp->certificateTypeId = UA_NS0ID(ECCNISTP256APPLICATIONCERTIFICATETYPE);
    sp->securityLevel = 10;
    sp->policyType = UA_SECURITYPOLICYTYPE_ECC_AEAD;

    /* Asymmetric Signature (ECDSA-SHA2-256).
     *
     * The URI here is what goes into the SignatureData.algorithm of the
     * CreateSession/ActivateSession signatures. For the ECC SecurityPolicies
     * the OPC UA reference stack (UA-.NETStandard) signs with an empty
     * algorithm but its verifier accepts EITHER an empty string OR the
     * SecurityPolicy URI (SecurityPolicies.cs, AsymmetricSignatureAlgorithm
     * EcdsaSha256: `IsNullOrEmpty(signature.Algorithm) || signature.Algorithm
     * == securityPolicy.Uri`). Empty is accepted by .NET/Softing but rejected
     * by stricter servers (e.g. CodeSys), so we send the SecurityPolicy URI -
     * the canonical non-empty value that interoperates with both. The
     * dedicated ECDSA URIs (xmldsig-more#ecdsa-sha256, conformanceunit/2473)
     * are NOT accepted and must not be used. */
    UA_SecurityPolicySignatureAlgorithm *asymSig = &sp->asymSignatureAlgorithm;
    asymSig->uri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256_ChaChaPoly\0");
    asymSig->verify = UA_AsymSig_EccNistP256ChaChaPoly_verify;
    asymSig->sign = UA_AsymSig_EccNistP256ChaChaPoly_sign;
    asymSig->getLocalSignatureSize = UA_AsySig_EccNistP256ChaChaPoly_getLocalSignatureSize;
    asymSig->getRemoteSignatureSize = UA_Asym_EccNistP256ChaChaPoly_getRemoteSignatureSize;

    /* Asymmetric Encryption (none) */
    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri = UA_STRING("https://profiles.opcfoundation.org/conformanceunit/2720\0");
    asymEnc->encrypt = UA_Asym_EccNistP256ChaChaPoly_Dummy;
    asymEnc->decrypt = UA_Asym_EccNistP256ChaChaPoly_Dummy;
    asymEnc->getLocalKeyLength = UA_AsymEn_EccNistP256ChaChaPoly_getLocalKeyLength;
    asymEnc->getRemoteKeyLength = UA_AsymEn_EccNistP256ChaChaPoly_getRemoteKeyLength;
    asymEnc->getRemoteBlockSize = UA_AsymEn_EccNistP256ChaChaPoly_getRemoteBlockSize;
    asymEnc->getRemotePlainTextBlockSize = UA_AsymEn_EccNistP256ChaChaPoly_getRemotePlainTextBlockSize;

    /* Symmetric Signature (ChaCha20-Poly1305, 16-byte tag) */
    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = UA_STRING("http://opcfoundation.org/UA/security/chacha20-poly1305\0");
    symSig->verify = UA_SymSig_EccNistP256ChaChaPoly_verify;
    symSig->sign = UA_SymSig_EccNistP256ChaChaPoly_sign;
    symSig->getLocalSignatureSize = UA_SymSig_EccNistP256ChaChaPoly_getLocalSignatureSize;
    symSig->getRemoteSignatureSize = UA_SymSig_EccNistP256ChaChaPoly_getRemoteSignatureSize;
    symSig->getLocalKeyLength = UA_SymSig_EccNistP256ChaChaPoly_getLocalKeyLength;
    symSig->getRemoteKeyLength = UA_SymSig_EccNistP256ChaChaPoly_getRemoteKeyLength;

    /* Symmetric Encryption (ChaCha20-Poly1305 AEAD) */
    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = UA_STRING("http://opcfoundation.org/UA/security/chacha20-poly1305\0");
    symEnc->encrypt = UA_SymEn_EccNistP256ChaChaPoly_encrypt;
    symEnc->decrypt = UA_SymEn_EccNistP256ChaChaPoly_decrypt;
    symEnc->getLocalKeyLength = UA_SymEn_EccNistP256ChaChaPoly_getLocalKeyLength;
    symEnc->getRemoteKeyLength = UA_SymEn_EccNistP256ChaChaPoly_getRemoteKeyLength;
    symEnc->getRemoteBlockSize = UA_SymEn_EccNistP256ChaChaPoly_getBlockSize;
    symEnc->getRemotePlainTextBlockSize = UA_SymEn_EccNistP256ChaChaPoly_getBlockSize;
    symEnc->getLocalIvLength = UA_SymEn_EccNistP256ChaChaPoly_getIvLength;

    /* Use the same signature algorithm as the asymmetric component for
     * certificate signing (see standard) */
    sp->certSignatureAlgorithm = sp->asymSignatureAlgorithm;

    /* Direct Method Pointers */
    sp->newChannelContext = EccNistP256ChaChaPoly_New_Context;
    sp->deleteChannelContext = EccNistP256ChaChaPoly_Delete_Context;
    sp->setLocalSymEncryptingKey = EccNistP256ChaChaPoly_setLocalSymEncryptingKey;
    sp->setLocalSymSigningKey = EccNistP256ChaChaPoly_setLocalSymSigningKey;
    sp->setLocalSymIv = EccNistP256ChaChaPoly_setLocalSymIv;
    sp->setRemoteSymEncryptingKey = EccNistP256ChaChaPoly_setRemoteSymEncryptingKey;
    sp->setRemoteSymSigningKey = EccNistP256ChaChaPoly_setRemoteSymSigningKey;
    sp->setRemoteSymIv = EccNistP256ChaChaPoly_setRemoteSymIv;
    sp->setMessageSecurityParameters = EccNistP256ChaChaPoly_setMessageSecurityParameters;
    sp->compareCertificate = EccNistP256ChaChaPoly_compareCertificate;
    sp->generateKey = UA_Sym_EccNistP256ChaChaPoly_generateKey;
    sp->generateNonce = UA_Sym_EccNistP256ChaChaPoly_generateNonce;
    sp->nonceLength = UA_SECURITYPOLICY_ECCNISTP256_CHACHAPOLY_NONCE_LENGTH_BYTES;
    sp->makeCertThumbprint = UA_makeCertificateThumbprint_EccNistP256ChaChaPoly;
    sp->compareCertThumbprint = UA_compareCertificateThumbprint_EccNistP256ChaChaPoly;
    sp->updateCertificate = updateCertificateAndPrivateKey_sp_EccNistP256ChaChaPoly;
    sp->createSigningRequest = createSigningRequest_sp_eccnistp256chachapoly;
    sp->clear = UA_Policy_EccNistP256ChaChaPoly_Clear_Context;

    /* Parse the certificate */
    UA_Openssl_Init();
    UA_StatusCode res =
        UA_OpenSSL_LoadLocalCertificate(&localCertificate, &sp->localCertificate,
                                        EVP_PKEY_EC);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Create the policy context */
    res = UA_Policy_EccNistP256ChaChaPoly_New_Context(sp, localPrivateKey,
                                                  applicationType, logger);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&sp->localCertificate);
        return res;
    }

    return UA_STATUSCODE_GOOD;
}

#endif
