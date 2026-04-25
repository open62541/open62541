/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)

#include "securitypolicy_common.h"
#include <openssl/rand.h>

#define UA_ECCCURVE448_ASYM_SIGNATURE_LENGTH 114
#define UA_ECCCURVE448_SYM_SIGNING_KEY_LENGTH 32
#define UA_ECCCURVE448_SYM_ENCRYPTION_KEY_LENGTH 32
#define UA_ECCCURVE448_SYM_IV_LENGTH 12
#define UA_ECCCURVE448_SYM_SIGNATURE_LENGTH 16  /* Poly1305 tag */
#define UA_ECCCURVE448_NONCE_LENGTH_BYTES 56    /* X448 public key */

typedef struct {
    EVP_PKEY *localPrivateKey;
    EVP_PKEY *csrLocalPrivateKey;
    UA_ByteString localCertThumbprint;
    UA_ApplicationType applicationType;
} Policy_Context_EccCurve448;

typedef struct {
    EVP_PKEY *localEphemeralKeyPair;
    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;
    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    UA_ByteString remoteCertificate;
    X509 *remoteCertificateX509;

    /* AEAD message context */
    UA_UInt32 tokenId;
    UA_UInt32 previousSequenceNumber;
    UA_ByteString additionalAuthData;
} Channel_Context_EccCurve448;

static void
computeMaskedIv448(UA_Byte *iv_out, const UA_ByteString *base_iv,
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
UA_Policy_EccCurve448_New_Context(UA_SecurityPolicy *securityPolicy,
                                   const UA_ByteString localPrivateKey,
                                   const UA_ApplicationType applicationType,
                                   const UA_Logger *logger) {
    Policy_Context_EccCurve448 *context = (Policy_Context_EccCurve448 *)
        UA_malloc(sizeof(Policy_Context_EccCurve448));
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
UA_Policy_EccCurve448_Clear_Context(UA_SecurityPolicy *policy) {
    if(!policy || !policy->policyContext)
        return;

    UA_ByteString_clear(&policy->localCertificate);

    Policy_Context_EccCurve448 *pc =
        (Policy_Context_EccCurve448 *)policy->policyContext;
    EVP_PKEY_free(pc->localPrivateKey);
    EVP_PKEY_free(pc->csrLocalPrivateKey);
    UA_ByteString_clear(&pc->localCertThumbprint);
    UA_free(pc);
}

static UA_StatusCode
createSigningRequest_sp_ecccurve448(UA_SecurityPolicy *securityPolicy,
                                    const UA_String *subjectName,
                                    const UA_ByteString *nonce,
                                    const UA_KeyValueMap *params,
                                    UA_ByteString *csr,
                                    UA_ByteString *newPrivateKey) {
    if(!securityPolicy || !csr)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(!securityPolicy->policyContext)
        return UA_STATUSCODE_BADINTERNALERROR;
    Policy_Context_EccCurve448 *pc =
        (Policy_Context_EccCurve448 *)securityPolicy->policyContext;
    return UA_OpenSSL_CreateSigningRequest(pc->localPrivateKey,
                                           &pc->csrLocalPrivateKey,
                                           securityPolicy, subjectName,
                                           nonce, csr, newPrivateKey);
}

static UA_StatusCode
updateCertificateAndPrivateKey_sp_EccCurve448(UA_SecurityPolicy *securityPolicy,
                                               const UA_ByteString newCertificate,
                                               const UA_ByteString newPrivateKey) {
    if(!securityPolicy || !securityPolicy->policyContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    Policy_Context_EccCurve448 *pc =
        (Policy_Context_EccCurve448 *)securityPolicy->policyContext;

    UA_ByteString_clear(&securityPolicy->localCertificate);
    UA_StatusCode retval = UA_OpenSSL_LoadLocalCertificate(
        &newCertificate, &securityPolicy->localCertificate, EVP_PKEY_ED448);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    EVP_PKEY_free(pc->localPrivateKey);
    pc->localPrivateKey = UA_OpenSSL_LoadPrivateKey(&newPrivateKey);
    if(!pc->localPrivateKey) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

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
        UA_Policy_EccCurve448_Clear_Context(securityPolicy);
    return retval;
}

/* --- Channel Context --- */

static UA_StatusCode
EccCurve448_New_Context(const UA_SecurityPolicy *securityPolicy,
                        const UA_ByteString *remoteCertificate,
                        void **channelContext) {
    if(!securityPolicy || !remoteCertificate || !channelContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    Channel_Context_EccCurve448 *cc = (Channel_Context_EccCurve448 *)
        UA_calloc(1, sizeof(Channel_Context_EccCurve448));
    if(!cc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval =
        UA_copyCertificate(&cc->remoteCertificate, remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(cc);
        return retval;
    }

    cc->remoteCertificateX509 =
        UA_OpenSSL_LoadCertificate(&cc->remoteCertificate, EVP_PKEY_ED448);
    if(!cc->remoteCertificateX509) {
        UA_ByteString_clear(&cc->remoteCertificate);
        UA_free(cc);
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
    }

    *channelContext = cc;
    return UA_STATUSCODE_GOOD;
}

static void
EccCurve448_Delete_Context(const UA_SecurityPolicy *policy,
                           void *channelContext) {
    if(!channelContext)
        return;
    Channel_Context_EccCurve448 *cc =
        (Channel_Context_EccCurve448 *)channelContext;
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
EccCurve448_setMessageSecurityParameters(const UA_SecurityPolicy *policy,
                                          void *channelContext,
                                          UA_UInt32 tokenId,
                                          UA_UInt32 previousSequenceNumber,
                                          const UA_ByteString *additionalAuthData) {
    if(!channelContext)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccCurve448 *cc =
        (Channel_Context_EccCurve448 *)channelContext;
    cc->tokenId = tokenId;
    cc->previousSequenceNumber = previousSequenceNumber;

    UA_ByteString_clear(&cc->additionalAuthData);
    if(additionalAuthData && additionalAuthData->length > 0)
        return UA_ByteString_copy(additionalAuthData, &cc->additionalAuthData);
    return UA_STATUSCODE_GOOD;
}

/* --- Asymmetric Signature (Ed448) --- */

static UA_StatusCode
UA_AsymSig_EccCurve448_sign(const UA_SecurityPolicy *policy,
                             void *channelContext, const UA_ByteString *message,
                             UA_ByteString *signature) {
    if(!channelContext || !message || !signature)
        return UA_STATUSCODE_BADINTERNALERROR;
    Policy_Context_EccCurve448 *pc =
        (Policy_Context_EccCurve448 *)policy->policyContext;
    return UA_OpenSSL_EdDSA_Ed448_Sign(message, pc->localPrivateKey, signature);
}

static UA_StatusCode
UA_AsymSig_EccCurve448_verify(const UA_SecurityPolicy *policy,
                               void *channelContext,
                               const UA_ByteString *message,
                               const UA_ByteString *signature) {
    if(!message || !signature || !channelContext)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccCurve448 *cc =
        (Channel_Context_EccCurve448 *)channelContext;
    return UA_OpenSSL_EdDSA_Ed448_Verify(message, cc->remoteCertificateX509,
                                          signature);
}

static size_t
UA_AsymSig_EccCurve448_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                                              const void *channelContext) {
    return UA_ECCCURVE448_ASYM_SIGNATURE_LENGTH;
}

static size_t
UA_AsymSig_EccCurve448_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                                const void *channelContext) {
    return UA_ECCCURVE448_ASYM_SIGNATURE_LENGTH;
}

/* --- Asymmetric Encryption (dummy) --- */

static UA_StatusCode
UA_Asym_EccCurve448_Dummy(const UA_SecurityPolicy *policy,
                           void *channelContext, UA_ByteString *data) {
    return UA_STATUSCODE_GOOD;
}

static size_t
UA_AsymEn_EccCurve448_getRemotePlainTextBlockSize(const UA_SecurityPolicy *policy,
                                                   const void *ctx) {
    return 1;
}

static size_t
UA_AsymEn_EccCurve448_getRemoteBlockSize(const UA_SecurityPolicy *policy,
                                          const void *ctx) {
    return 1;
}

static size_t
UA_AsymEn_EccCurve448_getKeyLength(const UA_SecurityPolicy *policy,
                                    const void *ctx) {
    return 1;
}

/* --- Symmetric Signature (Poly1305) --- */

static size_t
UA_SymSig_EccCurve448_getLocalSignatureSize(const UA_SecurityPolicy *p,
                                             const void *ctx) {
    return UA_ECCCURVE448_SYM_SIGNATURE_LENGTH;
}

static size_t
UA_SymSig_EccCurve448_getRemoteSignatureSize(const UA_SecurityPolicy *p,
                                               const void *ctx) {
    return UA_ECCCURVE448_SYM_SIGNATURE_LENGTH;
}

static size_t
UA_SymSig_EccCurve448_getLocalKeyLength(const UA_SecurityPolicy *p,
                                         const void *ctx) {
    return UA_ECCCURVE448_SYM_SIGNING_KEY_LENGTH;
}

static size_t
UA_SymSig_EccCurve448_getRemoteKeyLength(const UA_SecurityPolicy *p,
                                          const void *ctx) {
    return UA_ECCCURVE448_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
UA_SymSig_EccCurve448_sign(const UA_SecurityPolicy *policy,
                            void *channelContext, const UA_ByteString *message,
                            UA_ByteString *signature) {
    if(!channelContext || !message || !signature)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccCurve448 *cc =
        (Channel_Context_EccCurve448 *)channelContext;

    /* Compute masked IV (same nonce derivation as encrypt mode) */
    UA_Byte maskedIv[12];
    computeMaskedIv448(maskedIv, &cc->localSymIv,
                       cc->tokenId, cc->previousSequenceNumber);

    /* Use ChaCha20-Poly1305 AEAD to compute the Poly1305 tag.
     * For sign-only: treat entire message as AAD (no encryption). */
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if(!ctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;
    if(EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, NULL, NULL) != 1)
        goto errout;
    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, 12, NULL) != 1)
        goto errout;
    if(EVP_EncryptInit_ex(ctx, NULL, NULL,
                          cc->localSymSigningKey.data, maskedIv) != 1)
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

    /* Get the 16-byte Poly1305 authentication tag */
    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16,
                           signature->data) != 1)
        goto errout;
    signature->length = 16;
    ret = UA_STATUSCODE_GOOD;

errout:
    EVP_CIPHER_CTX_free(ctx);
    return ret;
}

static UA_StatusCode
UA_SymSig_EccCurve448_verify(const UA_SecurityPolicy *policy,
                              void *channelContext, const UA_ByteString *message,
                              const UA_ByteString *signature) {
    if(!channelContext || !message || !signature)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccCurve448 *cc =
        (Channel_Context_EccCurve448 *)channelContext;

    if(signature->length < 16)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Compute masked IV using remote IV */
    UA_Byte maskedIv[12];
    computeMaskedIv448(maskedIv, &cc->remoteSymIv,
                       cc->tokenId, cc->previousSequenceNumber);

    /* Use ChaCha20-Poly1305 AEAD to verify the Poly1305 tag.
     * Treat entire message as AAD (sign-only). */
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if(!ctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;
    if(EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, NULL, NULL) != 1)
        goto errout;
    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, 12, NULL) != 1)
        goto errout;
    if(EVP_DecryptInit_ex(ctx, NULL, NULL,
                          cc->remoteSymSigningKey.data, maskedIv) != 1)
        goto errout;

    /* Set the expected tag for verification */
    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16,
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

/* --- Symmetric Encryption (ChaCha20-Poly1305 AEAD) --- */

static size_t
UA_SymEn_EccCurve448_getLocalKeyLength(const UA_SecurityPolicy *p,
                                        const void *ctx) {
    return UA_ECCCURVE448_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymEn_EccCurve448_getRemoteKeyLength(const UA_SecurityPolicy *p,
                                         const void *ctx) {
    return UA_ECCCURVE448_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymEn_EccCurve448_getBlockSize(const UA_SecurityPolicy *p,
                                   const void *ctx) {
    return 1;
}

static size_t
UA_SymEn_EccCurve448_getIvLength(const UA_SecurityPolicy *p,
                                  const void *ctx) {
    return UA_ECCCURVE448_SYM_IV_LENGTH;
}

static UA_StatusCode
UA_SymEn_EccCurve448_encrypt(const UA_SecurityPolicy *policy,
                              void *channelContext, UA_ByteString *data) {
    if(!channelContext || !data)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccCurve448 *cc =
        (Channel_Context_EccCurve448 *)channelContext;

    UA_Byte maskedIv[12];
    computeMaskedIv448(maskedIv, &cc->localSymIv,
                       cc->tokenId, cc->previousSequenceNumber);
    UA_ByteString ivBs = {12, maskedIv};

    return UA_OpenSSL_ChaCha20Poly1305_Encrypt(
        &ivBs, &cc->localSymEncryptingKey, &cc->additionalAuthData,
        data, true);
}

static UA_StatusCode
UA_SymEn_EccCurve448_decrypt(const UA_SecurityPolicy *policy,
                              void *channelContext, UA_ByteString *data) {
    if(!channelContext || !data)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccCurve448 *cc =
        (Channel_Context_EccCurve448 *)channelContext;

    UA_Byte maskedIv[12];
    computeMaskedIv448(maskedIv, &cc->remoteSymIv,
                       cc->tokenId, cc->previousSequenceNumber);
    UA_ByteString ivBs = {12, maskedIv};

    return UA_OpenSSL_ChaCha20Poly1305_Decrypt(
        &ivBs, &cc->remoteSymEncryptingKey, &cc->additionalAuthData,
        data, true);
}

/* --- Key Setters --- */

static UA_StatusCode
EccCurve448_setLocalSymSigningKey(const UA_SecurityPolicy *p,
                                   void *ctx, const UA_ByteString *key) {
    if(!key || !ctx) return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccCurve448 *cc = (Channel_Context_EccCurve448 *)ctx;
    UA_ByteString_clear(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}

static UA_StatusCode
EccCurve448_setLocalSymEncryptingKey(const UA_SecurityPolicy *p,
                                      void *ctx, const UA_ByteString *key) {
    if(!key || !ctx) return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccCurve448 *cc = (Channel_Context_EccCurve448 *)ctx;
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

static UA_StatusCode
EccCurve448_setLocalSymIv(const UA_SecurityPolicy *p,
                           void *ctx, const UA_ByteString *iv) {
    if(!iv || !ctx) return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccCurve448 *cc = (Channel_Context_EccCurve448 *)ctx;
    UA_ByteString_clear(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

static UA_StatusCode
EccCurve448_setRemoteSymSigningKey(const UA_SecurityPolicy *p,
                                    void *ctx, const UA_ByteString *key) {
    if(!key || !ctx) return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccCurve448 *cc = (Channel_Context_EccCurve448 *)ctx;
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

static UA_StatusCode
EccCurve448_setRemoteSymEncryptingKey(const UA_SecurityPolicy *p,
                                       void *ctx, const UA_ByteString *key) {
    if(!key || !ctx) return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccCurve448 *cc = (Channel_Context_EccCurve448 *)ctx;
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

static UA_StatusCode
EccCurve448_setRemoteSymIv(const UA_SecurityPolicy *p,
                            void *ctx, const UA_ByteString *iv) {
    if(!iv || !ctx) return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccCurve448 *cc = (Channel_Context_EccCurve448 *)ctx;
    UA_ByteString_clear(&cc->remoteSymIv);
    return UA_ByteString_copy(iv, &cc->remoteSymIv);
}

/* --- Compare Certificate --- */

static UA_StatusCode
EccCurve448_compareCertificate(const UA_SecurityPolicy *policy,
                                const void *channelContext,
                                const UA_ByteString *certificate) {
    if(!channelContext || !certificate)
        return UA_STATUSCODE_BADINTERNALERROR;
    const Channel_Context_EccCurve448 *cc =
        (const Channel_Context_EccCurve448 *)channelContext;
    return UA_OpenSSL_X509_compare(certificate, cc->remoteCertificateX509);
}

/* --- Nonce Generation (X448 ephemeral key) --- */

static UA_StatusCode
UA_Sym_EccCurve448_generateNonce(const UA_SecurityPolicy *policy,
                                  void *channelContext, UA_ByteString *out) {
    if(!policy->policyContext)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;

    if(out->data[0] == 'e' && out->data[1] == 'p' && out->data[2] == 'h') {
        Channel_Context_EccCurve448 *cc =
            (Channel_Context_EccCurve448 *)channelContext;
        return UA_OpenSSL_X448_GenerateKey(&cc->localEphemeralKeyPair, out);
    }

    UA_Int32 rc = RAND_bytes(out->data, (int)out->length);
    return (rc == 1) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADUNEXPECTEDERROR;
}

/* --- Key Derivation (XDHE + HKDF) --- */

static UA_StatusCode
UA_Sym_EccCurve448_generateKey(const UA_SecurityPolicy *policy,
                                void *channelContext,
                                const UA_ByteString *secret,
                                const UA_ByteString *seed,
                                UA_ByteString *out) {
    Policy_Context_EccCurve448 *pctx =
        (Policy_Context_EccCurve448 *)policy->policyContext;
    Channel_Context_EccCurve448 *cc =
        (Channel_Context_EccCurve448 *)channelContext;
    if(!pctx || !cc)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_OpenSSL_XDHE_DeriveKeys(EVP_PKEY_X448, "SHA256",
                                       pctx->applicationType,
                                       cc->localEphemeralKeyPair,
                                       secret, seed, out);
}

/* --- Thumbprint --- */

static UA_StatusCode
UA_makeCertificateThumbprint_EccCurve448(const UA_SecurityPolicy *policy,
                                          const UA_ByteString *certificate,
                                          UA_ByteString *thumbprint) {
    return UA_Openssl_X509_GetCertificateThumbprint(certificate, thumbprint, false);
}

static UA_StatusCode
UA_compareCertificateThumbprint_EccCurve448(const UA_SecurityPolicy *policy,
                                             const UA_ByteString *certThumb) {
    if(!policy || !certThumb)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Policy_Context_EccCurve448 *pc =
        (Policy_Context_EccCurve448 *)policy->policyContext;
    if(!UA_ByteString_equal(certThumb, &pc->localCertThumbprint))
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    return UA_STATUSCODE_GOOD;
}

/* --- Public Entry Point --- */

UA_StatusCode
UA_SecurityPolicy_EccCurve448(UA_SecurityPolicy *sp,
                               const UA_ApplicationType applicationType,
                               const UA_ByteString localCertificate,
                               const UA_ByteString localPrivateKey,
                               const UA_Logger *logger) {
    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = logger;
    sp->policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#ECC_curve448\0");
    sp->certificateGroupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    sp->certificateTypeId = UA_NODEID_NUMERIC(0, 23543);
    sp->securityLevel = 15;
    sp->policyType = UA_SECURITYPOLICYTYPE_ECC_AEAD;

    /* Asymmetric Signature (Ed448) */
    UA_SecurityPolicySignatureAlgorithm *asymSig = &sp->asymSignatureAlgorithm;
    asymSig->uri = UA_STRING_NULL;
    asymSig->verify = UA_AsymSig_EccCurve448_verify;
    asymSig->sign = UA_AsymSig_EccCurve448_sign;
    asymSig->getLocalSignatureSize = UA_AsymSig_EccCurve448_getLocalSignatureSize;
    asymSig->getRemoteSignatureSize = UA_AsymSig_EccCurve448_getRemoteSignatureSize;

    /* Asymmetric Encryption (dummy) */
    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri =
        UA_STRING("https://profiles.opcfoundation.org/conformanceunit/2720\0");
    asymEnc->encrypt = UA_Asym_EccCurve448_Dummy;
    asymEnc->decrypt = UA_Asym_EccCurve448_Dummy;
    asymEnc->getLocalKeyLength = UA_AsymEn_EccCurve448_getKeyLength;
    asymEnc->getRemoteKeyLength = UA_AsymEn_EccCurve448_getKeyLength;
    asymEnc->getRemoteBlockSize = UA_AsymEn_EccCurve448_getRemoteBlockSize;
    asymEnc->getRemotePlainTextBlockSize =
        UA_AsymEn_EccCurve448_getRemotePlainTextBlockSize;

    /* Symmetric Signature (Poly1305) */
    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri =
        UA_STRING("http://opcfoundation.org/UA/security/poly1305\0");
    symSig->verify = UA_SymSig_EccCurve448_verify;
    symSig->sign = UA_SymSig_EccCurve448_sign;
    symSig->getLocalSignatureSize = UA_SymSig_EccCurve448_getLocalSignatureSize;
    symSig->getRemoteSignatureSize = UA_SymSig_EccCurve448_getRemoteSignatureSize;
    symSig->getLocalKeyLength = UA_SymSig_EccCurve448_getLocalKeyLength;
    symSig->getRemoteKeyLength = UA_SymSig_EccCurve448_getRemoteKeyLength;

    /* Symmetric Encryption (ChaCha20-Poly1305 AEAD) */
    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri =
        UA_STRING("http://opcfoundation.org/UA/security/chacha20-poly1305\0");
    symEnc->encrypt = UA_SymEn_EccCurve448_encrypt;
    symEnc->decrypt = UA_SymEn_EccCurve448_decrypt;
    symEnc->getLocalKeyLength = UA_SymEn_EccCurve448_getLocalKeyLength;
    symEnc->getRemoteKeyLength = UA_SymEn_EccCurve448_getRemoteKeyLength;
    symEnc->getRemoteBlockSize = UA_SymEn_EccCurve448_getBlockSize;
    symEnc->getRemotePlainTextBlockSize = UA_SymEn_EccCurve448_getBlockSize;
    symEnc->getLocalIvLength = UA_SymEn_EccCurve448_getIvLength;

    /* Certificate signing uses the same algorithm as asymmetric */
    sp->certSignatureAlgorithm = sp->asymSignatureAlgorithm;

    /* Direct Method Pointers */
    sp->newChannelContext = EccCurve448_New_Context;
    sp->deleteChannelContext = EccCurve448_Delete_Context;
    sp->setLocalSymEncryptingKey = EccCurve448_setLocalSymEncryptingKey;
    sp->setLocalSymSigningKey = EccCurve448_setLocalSymSigningKey;
    sp->setLocalSymIv = EccCurve448_setLocalSymIv;
    sp->setRemoteSymEncryptingKey = EccCurve448_setRemoteSymEncryptingKey;
    sp->setRemoteSymSigningKey = EccCurve448_setRemoteSymSigningKey;
    sp->setRemoteSymIv = EccCurve448_setRemoteSymIv;
    sp->setMessageSecurityParameters = EccCurve448_setMessageSecurityParameters;
    sp->compareCertificate = EccCurve448_compareCertificate;
    sp->generateKey = UA_Sym_EccCurve448_generateKey;
    sp->generateNonce = UA_Sym_EccCurve448_generateNonce;
    sp->nonceLength = UA_ECCCURVE448_NONCE_LENGTH_BYTES;
    sp->makeCertThumbprint = UA_makeCertificateThumbprint_EccCurve448;
    sp->compareCertThumbprint = UA_compareCertificateThumbprint_EccCurve448;
    sp->updateCertificate = updateCertificateAndPrivateKey_sp_EccCurve448;
    sp->createSigningRequest = createSigningRequest_sp_ecccurve448;
    sp->clear = UA_Policy_EccCurve448_Clear_Context;

    /* Parse the certificate */
    UA_Openssl_Init();
    UA_StatusCode res =
        UA_OpenSSL_LoadLocalCertificate(&localCertificate, &sp->localCertificate,
                                         EVP_PKEY_ED448);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Create the policy context */
    res = UA_Policy_EccCurve448_New_Context(sp, localPrivateKey, applicationType,
                                             logger);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&sp->localCertificate);
        return res;
    }

    return UA_STATUSCODE_GOOD;
}

#endif
