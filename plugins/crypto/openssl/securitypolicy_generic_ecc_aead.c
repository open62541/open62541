/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Siemens AG (Authors: Tin Raic, Thomas Zeschg)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 *    Copyright 2026 (c) Andreas Hellenbrand
 *
 * The ECC AEAD SecurityPolicies of OPC UA Part 7, implemented once.
 *
 * The profiles differ only in the elliptic curve, the hash, and the AEAD
 * cipher. Everything else - the masked IV, the ephemeral-key nonce protocol,
 * the channel context lifecycle, CSR generation, certificate update - is the
 * same code for all of them, so each policy is a UA_EccAeadParams row and a
 * constructor that hands that row to eccAead_policy().
 *
 * The policies implemented here supersede the deprecated ECC_* policies:
 * symmetric signing and encryption (HMAC-SHA2 + AES-CBC there) become a single
 * AEAD pass with a 16-byte tag and a 12-byte IV, which is why they carry
 * DerivedSignatureKeyLength = 0 and UA_SECURITYPOLICYTYPE_ECC_AEAD. The
 * asymmetric signature and the key derivation are unchanged from their
 * deprecated counterparts.
 *
 * The policy context is the shared openssl_EccPolicyContext; the parameter row
 * and the resolved key-agreement id travel in its aeadParams/keyId fields.
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)

#include "securitypolicy_common.h"

#include <openssl/rand.h>
#include <openssl/ec.h>
#include <openssl/objects.h>

/**************/
/* Parameters */
/**************/

/* How the ephemeral key agreement of a policy is named to OpenSSL. The first
 * two resolve to a curve NID, the third to an EVP_PKEY type. */
typedef enum {
    ECC_AEAD_KEX_ECDH_NIST,  /* EC_curve_nist2nid(curveName) */
    ECC_AEAD_KEX_ECDH_OBJ,   /* OBJ_txt2nid(curveName), the brainpool curves */
    ECC_AEAD_KEX_XDH         /* xdhKeyType, X25519 and X448 */
} EccAeadKex;

/* What distinguishes the ECC AEAD policies from each other */
typedef struct {
    UA_String policyUri;
    UA_UInt32 certificateTypeId;   /* numeric identifier in namespace 0 */
    UA_Byte securityLevel;

    /* Ephemeral key agreement */
    EccAeadKex kexKind;
    const char *curveName;         /* NULL for ECC_AEAD_KEX_XDH */
    int xdhKeyType;                /* EVP_PKEY_X25519/X448, 0 otherwise */
    const char *hkdfHash;          /* "SHA256" or "SHA384" */
    size_t nonceLength;            /* the ephemeral public key on the wire */
    size_t secretLength;           /* the shared secret, i.e. the IKM width */

    /* Certificate and asymmetric signature */
    int certKeyType;               /* EVP_PKEY_EC/ED25519/ED448 */
    size_t certThumbprintLength;   /* 32 for SHA-256, 48 for SHA-384 */
    size_t asymSignatureLength;
    UA_StatusCode (*asymSign)(const UA_ByteString *message, EVP_PKEY *privateKey,
                              UA_ByteString *signature);
    UA_StatusCode (*asymVerify)(const UA_ByteString *message, X509 *certificate,
                                const UA_ByteString *signature);

    /* Symmetric AEAD */
    const EVP_CIPHER *(*aeadCipher)(void);
    UA_String symUri;
    size_t symEncryptionKeyLength;
} UA_EccAeadParams;

/* http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256_AesGcm */
static const UA_EccAeadParams eccNistP256AesGcmParams = {
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256_AesGcm"),
    UA_NS0ID_ECCNISTP256APPLICATIONCERTIFICATETYPE,
    10,                                        /* securityLevel */
    ECC_AEAD_KEX_ECDH_NIST, "P-256", 0,        /* ECDH on P-256 */
    "SHA256", 64, 32,                          /* HKDF, nonce, shared secret */
    EVP_PKEY_EC, 32, 64,                       /* certificate, thumbprint, signature */
    UA_Openssl_ECDSA_SHA256_Sign, UA_Openssl_ECDSA_SHA256_Verify,
    EVP_aes_128_gcm,
    UA_STRING_STATIC("http://opcfoundation.org/UA/security/aes128-gcm"), 16
};

/* http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256_ChaChaPoly */
static const UA_EccAeadParams eccNistP256ChaChaPolyParams = {
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256_ChaChaPoly"),
    UA_NS0ID_ECCNISTP256APPLICATIONCERTIFICATETYPE,
    10,                                        /* securityLevel */
    ECC_AEAD_KEX_ECDH_NIST, "P-256", 0,        /* ECDH on P-256 */
    "SHA256", 64, 32,                          /* HKDF, nonce, shared secret */
    EVP_PKEY_EC, 32, 64,                       /* certificate, thumbprint, signature */
    UA_Openssl_ECDSA_SHA256_Sign, UA_Openssl_ECDSA_SHA256_Verify,
    EVP_chacha20_poly1305,
    UA_STRING_STATIC("http://opcfoundation.org/UA/security/chacha20-poly1305"), 32
};

/* http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP256r1_AesGcm */
static const UA_EccAeadParams eccBrainpoolP256r1AesGcmParams = {
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP256r1_AesGcm"),
    UA_NS0ID_ECCBRAINPOOLP256R1APPLICATIONCERTIFICATETYPE,
    10,                                        /* securityLevel */
    ECC_AEAD_KEX_ECDH_OBJ, "brainpoolP256r1", 0,
    "SHA256", 64, 32,                          /* HKDF, nonce, shared secret */
    EVP_PKEY_EC, 32, 64,                       /* certificate, thumbprint, signature */
    UA_Openssl_ECDSA_SHA256_Sign, UA_Openssl_ECDSA_SHA256_Verify,
    EVP_aes_128_gcm,
    UA_STRING_STATIC("http://opcfoundation.org/UA/security/aes128-gcm"), 16
};

/* http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP256r1_ChaChaPoly */
static const UA_EccAeadParams eccBrainpoolP256r1ChaChaPolyParams = {
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP256r1_ChaChaPoly"),
    UA_NS0ID_ECCBRAINPOOLP256R1APPLICATIONCERTIFICATETYPE,
    10,                                        /* securityLevel */
    ECC_AEAD_KEX_ECDH_OBJ, "brainpoolP256r1", 0,
    "SHA256", 64, 32,                          /* HKDF, nonce, shared secret */
    EVP_PKEY_EC, 32, 64,                       /* certificate, thumbprint, signature */
    UA_Openssl_ECDSA_SHA256_Sign, UA_Openssl_ECDSA_SHA256_Verify,
    EVP_chacha20_poly1305,
    UA_STRING_STATIC("http://opcfoundation.org/UA/security/chacha20-poly1305"), 32
};

/* The 384-bit tier. OPC UA Part 7 rates these as the equivalent of
 * Aes256-Sha256-RsaPss, so they take its security level of 30 - above the 15 of
 * the deprecated ECC_nistP384/ECC_brainpoolP384r1 they supersede, which predate
 * that equivalence.
 *
 * The AES-256-GCM algorithm URI follows the scheme of the other AEAD rows,
 * http://opcfoundation.org/UA/security/<algorithm> (aes128-gcm and
 * chacha20-poly1305, as upstream ships them). The profile pages name the
 * algorithm (SymmetricEncryptionAlgorithm_AES256-GCM) but not its URI. The URI
 * is metadata only: the policy is selected by its policyUri. */

/* http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384_AesGcm */
static const UA_EccAeadParams eccNistP384AesGcmParams = {
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384_AesGcm"),
    UA_NS0ID_ECCNISTP384APPLICATIONCERTIFICATETYPE,
    30,                                        /* securityLevel */
    ECC_AEAD_KEX_ECDH_NIST, "P-384", 0,        /* ECDH on P-384 */
    "SHA384", 96, 48,                          /* HKDF, nonce, shared secret */
    EVP_PKEY_EC, 48, 96,                       /* certificate, thumbprint, signature */
    UA_Openssl_ECDSA_SHA384_Sign, UA_Openssl_ECDSA_SHA384_Verify,
    EVP_aes_256_gcm,
    UA_STRING_STATIC("http://opcfoundation.org/UA/security/aes256-gcm"), 32
};

/* http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384_ChaChaPoly */
static const UA_EccAeadParams eccNistP384ChaChaPolyParams = {
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384_ChaChaPoly"),
    UA_NS0ID_ECCNISTP384APPLICATIONCERTIFICATETYPE,
    30,                                        /* securityLevel */
    ECC_AEAD_KEX_ECDH_NIST, "P-384", 0,        /* ECDH on P-384 */
    "SHA384", 96, 48,                          /* HKDF, nonce, shared secret */
    EVP_PKEY_EC, 48, 96,                       /* certificate, thumbprint, signature */
    UA_Openssl_ECDSA_SHA384_Sign, UA_Openssl_ECDSA_SHA384_Verify,
    EVP_chacha20_poly1305,
    UA_STRING_STATIC("http://opcfoundation.org/UA/security/chacha20-poly1305"), 32
};

/* http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP384r1_AesGcm */
static const UA_EccAeadParams eccBrainpoolP384r1AesGcmParams = {
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP384r1_AesGcm"),
    UA_NS0ID_ECCBRAINPOOLP384R1APPLICATIONCERTIFICATETYPE,
    30,                                        /* securityLevel */
    ECC_AEAD_KEX_ECDH_OBJ, "brainpoolP384r1", 0,
    "SHA384", 96, 48,                          /* HKDF, nonce, shared secret */
    EVP_PKEY_EC, 48, 96,                       /* certificate, thumbprint, signature */
    UA_Openssl_ECDSA_SHA384_Sign, UA_Openssl_ECDSA_SHA384_Verify,
    EVP_aes_256_gcm,
    UA_STRING_STATIC("http://opcfoundation.org/UA/security/aes256-gcm"), 32
};

/* http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP384r1_ChaChaPoly */
static const UA_EccAeadParams eccBrainpoolP384r1ChaChaPolyParams = {
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP384r1_ChaChaPoly"),
    UA_NS0ID_ECCBRAINPOOLP384R1APPLICATIONCERTIFICATETYPE,
    30,                                        /* securityLevel */
    ECC_AEAD_KEX_ECDH_OBJ, "brainpoolP384r1", 0,
    "SHA384", 96, 48,                          /* HKDF, nonce, shared secret */
    EVP_PKEY_EC, 48, 96,                       /* certificate, thumbprint, signature */
    UA_Openssl_ECDSA_SHA384_Sign, UA_Openssl_ECDSA_SHA384_Verify,
    EVP_chacha20_poly1305,
    UA_STRING_STATIC("http://opcfoundation.org/UA/security/chacha20-poly1305"), 32
};

/* The Edwards curves. Their certificate carries an Ed25519/Ed448 signing key
 * while the ephemeral key agreement is X25519/X448, so certKeyType and
 * xdhKeyType name different algorithms - unlike the Weierstrass rows above,
 * where one curve serves both. */

/* http://opcfoundation.org/UA/SecurityPolicy#ECC_curve25519_AesGcm */
static const UA_EccAeadParams eccCurve25519AesGcmParams = {
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_curve25519_AesGcm"),
    UA_NS0ID_ECCCURVE25519APPLICATIONCERTIFICATETYPE,
    10,                                        /* securityLevel */
    ECC_AEAD_KEX_XDH, NULL, EVP_PKEY_X25519,   /* X25519 key agreement */
    "SHA256", 32, 32,                          /* HKDF, nonce, shared secret */
    EVP_PKEY_ED25519, 32, 64,                  /* certificate, thumbprint, signature */
    UA_OpenSSL_EdDSA_Ed25519_Sign, UA_OpenSSL_EdDSA_Ed25519_Verify,
    EVP_aes_128_gcm,
    UA_STRING_STATIC("http://opcfoundation.org/UA/security/aes128-gcm"), 16
};

/* http://opcfoundation.org/UA/SecurityPolicy#ECC_curve25519_ChaChaPoly */
static const UA_EccAeadParams eccCurve25519ChaChaPolyParams = {
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_curve25519_ChaChaPoly"),
    UA_NS0ID_ECCCURVE25519APPLICATIONCERTIFICATETYPE,
    10,                                        /* securityLevel */
    ECC_AEAD_KEX_XDH, NULL, EVP_PKEY_X25519,   /* X25519 key agreement */
    "SHA256", 32, 32,                          /* HKDF, nonce, shared secret */
    EVP_PKEY_ED25519, 32, 64,                  /* certificate, thumbprint, signature */
    UA_OpenSSL_EdDSA_Ed25519_Sign, UA_OpenSSL_EdDSA_Ed25519_Verify,
    EVP_chacha20_poly1305,
    UA_STRING_STATIC("http://opcfoundation.org/UA/security/chacha20-poly1305"), 32
};

/* http://opcfoundation.org/UA/SecurityPolicy#ECC_curve448_AesGcm */
static const UA_EccAeadParams eccCurve448AesGcmParams = {
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_curve448_AesGcm"),
    UA_NS0ID_ECCCURVE448APPLICATIONCERTIFICATETYPE,
    30,                                        /* securityLevel */
    ECC_AEAD_KEX_XDH, NULL, EVP_PKEY_X448,     /* X448 key agreement */
    "SHA384", 56, 56,                          /* HKDF, nonce, shared secret */
    EVP_PKEY_ED448, 48, 114,                   /* certificate, thumbprint, signature */
    UA_OpenSSL_EdDSA_Ed448_Sign, UA_OpenSSL_EdDSA_Ed448_Verify,
    EVP_aes_256_gcm,
    UA_STRING_STATIC("http://opcfoundation.org/UA/security/aes256-gcm"), 32
};

/* http://opcfoundation.org/UA/SecurityPolicy#ECC_curve448_ChaChaPoly */
static const UA_EccAeadParams eccCurve448ChaChaPolyParams = {
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_curve448_ChaChaPoly"),
    UA_NS0ID_ECCCURVE448APPLICATIONCERTIFICATETYPE,
    30,                                        /* securityLevel */
    ECC_AEAD_KEX_XDH, NULL, EVP_PKEY_X448,     /* X448 key agreement */
    "SHA384", 56, 56,                          /* HKDF, nonce, shared secret */
    EVP_PKEY_ED448, 48, 114,                   /* certificate, thumbprint, signature */
    UA_OpenSSL_EdDSA_Ed448_Sign, UA_OpenSSL_EdDSA_Ed448_Verify,
    EVP_chacha20_poly1305,
    UA_STRING_STATIC("http://opcfoundation.org/UA/security/chacha20-poly1305"), 32
};

/************/
/* Contexts */
/************/

/* See securitypolicy_common.h */
typedef openssl_EccPolicyContext Policy_Context_EccAead;
typedef openssl_EccAeadChannelContext Channel_Context_EccAead;

/* The parameter row of a policy */
static const UA_EccAeadParams *
eccAead_params(const UA_SecurityPolicy *policy) {
    if(!policy || !policy->policyContext)
        return NULL;
    return (const UA_EccAeadParams *)
        ((const Policy_Context_EccAead *)policy->policyContext)->aeadParams;
}

/* Compute the per-message AEAD IV.
 * The 12-byte IV is formed by XORing the tokenId into the first 4 bytes and
 * the previous sequence number into the next 4 bytes of the base (local or
 * remote) Sym IV. The last 4 bytes are unchanged. */
static void
eccAead_maskIv(UA_Byte *iv_out, const UA_ByteString *base_iv,
               UA_UInt32 tokenId, UA_UInt32 seqNo) {
    memcpy(iv_out, base_iv->data, UA_OPENSSL_AEAD_IV_LENGTH);
    iv_out[0] ^= (UA_Byte)(tokenId & 0xFF);
    iv_out[1] ^= (UA_Byte)((tokenId >> 8) & 0xFF);
    iv_out[2] ^= (UA_Byte)((tokenId >> 16) & 0xFF);
    iv_out[3] ^= (UA_Byte)((tokenId >> 24) & 0xFF);
    iv_out[4] ^= (UA_Byte)(seqNo & 0xFF);
    iv_out[5] ^= (UA_Byte)((seqNo >> 8) & 0xFF);
    iv_out[6] ^= (UA_Byte)((seqNo >> 16) & 0xFF);
    iv_out[7] ^= (UA_Byte)((seqNo >> 24) & 0xFF);
}

/* The curve NID (ECDH) or EVP_PKEY type (XDH) a parameter row names, 0 if
 * this build of OpenSSL does not know it. */
static int
eccAead_resolveKeyId(const UA_EccAeadParams *params) {
    switch(params->kexKind) {
    case ECC_AEAD_KEX_ECDH_NIST:
        return EC_curve_nist2nid(params->curveName);
    case ECC_AEAD_KEX_ECDH_OBJ:
        return OBJ_txt2nid(params->curveName);
    case ECC_AEAD_KEX_XDH:
        return params->xdhKeyType;
    }
    return 0;
}

/* UA_OpenSSL_LoadCertificate compares only the key type (EVP_PKEY_base_id), so
 * a P-256 certificate passes as the certificate of a P-384 policy, which then
 * advertises an endpoint it cannot serve. Reject the mismatch here.
 *
 * Only the ECDH policies need it: for Ed25519 and Ed448 the key type already
 * identifies the curve. */
static UA_StatusCode
eccAead_checkCertificateCurve(const UA_ByteString *certificate,
                              const UA_EccAeadParams *params, int keyId) {
    if(params->kexKind == ECC_AEAD_KEX_XDH)
        return UA_STATUSCODE_GOOD;

    X509 *cert = UA_OpenSSL_LoadCertificate(certificate, params->certKeyType);
    if(!cert)
        return UA_STATUSCODE_BADCERTIFICATEINVALID;

    EVP_PKEY *pkey = X509_get_pubkey(cert);
    if(!pkey) {
        X509_free(cert);
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    }

    int certNid = NID_undef;
#if(OPENSSL_VERSION_NUMBER >= 0x30000000L)
    char curveName[64];
    size_t curveNameLen = 0;
    if(EVP_PKEY_get_utf8_string_param(pkey, "group", curveName,
                                      sizeof(curveName), &curveNameLen))
        certNid = OBJ_txt2nid(curveName);
#else
    const EC_KEY *ecKey = EVP_PKEY_get0_EC_KEY(pkey);
    if(ecKey)
        certNid = EC_GROUP_get_curve_name(EC_KEY_get0_group(ecKey));
#endif

    EVP_PKEY_free(pkey);
    X509_free(cert);

    if(certNid == NID_undef || certNid != keyId)
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    return UA_STATUSCODE_GOOD;
}

/******************/
/* Policy Context */
/******************/

static UA_StatusCode
eccAead_newPolicyContext(UA_SecurityPolicy *securityPolicy,
                         const UA_EccAeadParams *params, int keyId,
                         const UA_ByteString localPrivateKey,
                         const UA_ApplicationType applicationType) {
    Policy_Context_EccAead *context = (Policy_Context_EccAead *)
        UA_calloc(1, sizeof(Policy_Context_EccAead));
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
    context->aeadParams = params;
    context->keyId = keyId;

    securityPolicy->policyContext = context;
    return UA_STATUSCODE_GOOD;
}

static void
eccAead_clearPolicyContext(UA_SecurityPolicy *policy) {
    if(!policy || !policy->policyContext)
        return;

    UA_ByteString_clear(&policy->localCertificate);

    /* Delete all allocated members in the context */
    Policy_Context_EccAead *pc = (Policy_Context_EccAead *)policy->policyContext;
    EVP_PKEY_free(pc->localPrivateKey);
    EVP_PKEY_free(pc->csrLocalPrivateKey);
    UA_ByteString_clear(&pc->localCertThumbprint);
    UA_free(pc);
}

static UA_StatusCode
eccAead_createSigningRequest(UA_SecurityPolicy *securityPolicy,
                             const UA_String *subjectName,
                             const UA_ByteString *nonce,
                             const UA_KeyValueMap *params,
                             UA_ByteString *csr,
                             UA_ByteString *newPrivateKey) {
    if(!securityPolicy || !csr)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(!securityPolicy->policyContext)
        return UA_STATUSCODE_BADINTERNALERROR;
    Policy_Context_EccAead *pc =
        (Policy_Context_EccAead *)securityPolicy->policyContext;
    return UA_OpenSSL_CreateSigningRequest(pc->localPrivateKey,
                                           &pc->csrLocalPrivateKey,
                                           securityPolicy, subjectName,
                                           nonce, csr, newPrivateKey);
}

static UA_StatusCode
eccAead_updateCertificateAndPrivateKey(UA_SecurityPolicy *securityPolicy,
                                       const UA_ByteString newCertificate,
                                       const UA_ByteString newPrivateKey) {
    if(!securityPolicy || !securityPolicy->policyContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    Policy_Context_EccAead *pc =
        (Policy_Context_EccAead *)securityPolicy->policyContext;
    const UA_EccAeadParams *params = (const UA_EccAeadParams *)pc->aeadParams;
    if(!params)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* The new certificate must be on the curve of the policy, like the one the
     * constructor accepted */
    UA_StatusCode retval =
        eccAead_checkCertificateCurve(&newCertificate, params, pc->keyId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the certificate */
    UA_ByteString_clear(&securityPolicy->localCertificate);
    retval = UA_OpenSSL_LoadLocalCertificate(&newCertificate,
                                             &securityPolicy->localCertificate,
                                             params->certKeyType);
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
    /* Through the policy's own clear. The context is gone afterwards: a later
     * clear must not free it a second time. */
    if(securityPolicy->policyContext) {
        securityPolicy->clear(securityPolicy);
        securityPolicy->policyContext = NULL;
    }
    return retval;
}

/*******************/
/* Channel Context */
/*******************/

static UA_StatusCode
eccAead_newChannelContext(const UA_SecurityPolicy *securityPolicy,
                          const UA_ByteString *remoteCertificate,
                          void **channelContext) {
    if(securityPolicy == NULL || remoteCertificate == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const UA_EccAeadParams *params = eccAead_params(securityPolicy);
    if(!params)
        return UA_STATUSCODE_BADINTERNALERROR;

    Channel_Context_EccAead *newContext = (Channel_Context_EccAead *)
        UA_calloc(1, sizeof(Channel_Context_EccAead));
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
        UA_OpenSSL_LoadCertificate(&newContext->remoteCertificate,
                                   params->certKeyType);
    if(newContext->remoteCertificateX509 == NULL) {
        UA_ByteString_clear(&newContext->remoteCertificate);
        UA_free(newContext);
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
    }

    /* Return the new channel context */
    *channelContext = newContext;

    return UA_STATUSCODE_GOOD;
}

static void
eccAead_deleteChannelContext(const UA_SecurityPolicy *policy,
                             void *channelContext) {
    if(!channelContext)
        return;
    Channel_Context_EccAead *cc = (Channel_Context_EccAead *)channelContext;
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

/***********************************/
/* AEAD Message Security Parameters */
/***********************************/

static UA_StatusCode
eccAead_setMessageSecurityParameters(const UA_SecurityPolicy *policy,
                                     void *channelContext,
                                     UA_UInt32 tokenId,
                                     UA_UInt32 previousSequenceNumber,
                                     const UA_ByteString *additionalAuthData) {
    if(!channelContext)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccAead *cc = (Channel_Context_EccAead *)channelContext;
    cc->tokenId = tokenId;
    cc->previousSequenceNumber = previousSequenceNumber;

    UA_ByteString_clear(&cc->additionalAuthData);
    if(additionalAuthData && additionalAuthData->length > 0)
        return UA_ByteString_copy(additionalAuthData, &cc->additionalAuthData);
    return UA_STATUSCODE_GOOD;
}

/****************/
/* Thumbprints  */
/****************/

static UA_StatusCode
eccAead_compareCertificateThumbprint(const UA_SecurityPolicy *policy,
                                     const UA_ByteString *certificateThumbprint) {
    if(policy == NULL || certificateThumbprint == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Policy_Context_EccAead *pc = (Policy_Context_EccAead *)policy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint))
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
eccAead_makeCertificateThumbprint(const UA_SecurityPolicy *securityPolicy,
                                  const UA_ByteString *certificate,
                                  UA_ByteString *thumbprint) {
    return UA_Openssl_X509_GetCertificateThumbprint(certificate, thumbprint, false);
}

/**************************/
/* Asymmetric Key Lengths */
/**************************/

static size_t
eccAead_asymGetRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                   const void *channelContext) {
    /* According to the standard, the server and the client must agree on the
     * security mode and security policy. Therefore, it can be assumed that
     * the remote asymmetric cryptographic tokens have the same sizes as the
     * local ones. */
    const UA_EccAeadParams *params = eccAead_params(policy);
    return params ? params->asymSignatureLength : 0;
}

static size_t
eccAead_asymGetLocalSignatureSize(const UA_SecurityPolicy *policy,
                                  const void *channelContext) {
    const UA_EccAeadParams *params = eccAead_params(policy);
    return params ? params->asymSignatureLength : 0;
}

/* No ECC encryption -> the key and block lengths are set to 1 to avoid
 * division or multiplication with 0 */

static size_t
eccAead_asymGetRemotePlainTextBlockSize(const UA_SecurityPolicy *policy,
                                        const void *channelContext) {
    return 1;
}

static size_t
eccAead_asymGetRemoteBlockSize(const UA_SecurityPolicy *policy,
                               const void *channelContext) {
    return 1;
}

static size_t
eccAead_asymGetRemoteKeyLength(const UA_SecurityPolicy *policy,
                               const void *channelContext) {
    return 1;
}

static size_t
eccAead_asymGetLocalKeyLength(const UA_SecurityPolicy *policy,
                              const void *channelContext) {
    return 1;
}

/******************************/
/* Nonce and Key Derivation   */
/******************************/

static UA_StatusCode
eccAead_generateNonce(const UA_SecurityPolicy *policy,
                      void *channelContext, UA_ByteString *out) {
    Policy_Context_EccAead *pctx = (Policy_Context_EccAead *)policy->policyContext;
    const UA_EccAeadParams *params = eccAead_params(policy);
    if(!pctx || !params)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;

    /* Detect if we want to create an ephemeral key or just cryptographic random
     * data */
    if(out->data[0] == 'e' && out->data[1] == 'p' && out->data[2] == 'h') {
        Channel_Context_EccAead *cctx = (Channel_Context_EccAead *)channelContext;
        if(!cctx)
            return UA_STATUSCODE_BADUNEXPECTEDERROR;
        if(params->kexKind == ECC_AEAD_KEX_XDH) {
            if(params->xdhKeyType == EVP_PKEY_X448)
                return UA_OpenSSL_X448_GenerateKey(&cctx->localEphemeralKeyPair, out);
            return UA_OpenSSL_X25519_GenerateKey(&cctx->localEphemeralKeyPair, out);
        }
        return UA_OpenSSL_ECC_GenerateKey(pctx->keyId,
                                          &cctx->localEphemeralKeyPair, out);
    }

    UA_Int32 rc = RAND_bytes(out->data, (int)out->length);
    return (rc == 1) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADUNEXPECTEDERROR;
}

static UA_StatusCode
eccAead_generateKey(const UA_SecurityPolicy *policy,
                    void *channelContext, const UA_ByteString *secret,
                    const UA_ByteString *seed, UA_ByteString *out) {
    Policy_Context_EccAead *pctx = (Policy_Context_EccAead *)policy->policyContext;
    Channel_Context_EccAead *cc = (Channel_Context_EccAead *)channelContext;
    const UA_EccAeadParams *params = eccAead_params(policy);
    if(!pctx || !cc || !params)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;

    /* The SecureChannel prepends the IKM accumulator to `secret` on an enhanced
     * policy. Peel it off here, where policy->secretLength says how wide it is,
     * instead of letting the derivation infer it from a length mismatch. */
    UA_ByteString ikmInOut, nonce;
    UA_OpenSSL_splitIKM(policy, secret, &ikmInOut, &nonce);

    if(params->kexKind == ECC_AEAD_KEX_XDH)
        return UA_OpenSSL_XDHE_DeriveKeys(pctx->keyId, params->hkdfHash,
                                          pctx->applicationType,
                                          cc->localEphemeralKeyPair,
                                          &nonce, seed, &ikmInOut, out);

    return UA_OpenSSL_ECC_DeriveKeys(pctx->keyId,
                                     (char *)(uintptr_t)params->hkdfHash,
                                     pctx->applicationType,
                                     cc->localEphemeralKeyPair,
                                     &nonce, seed, &ikmInOut, out);
}

/* The key of an EccEncryptedSecret is derived without IKM chaining, so the
 * input is not split like in eccAead_generateKey */
static UA_StatusCode
eccAead_generateEncryptedSecretKey(const UA_SecurityPolicy *policy,
                                   void *channelContext,
                                   const UA_ByteString *receiverPublicKey,
                                   const UA_ByteString *senderPublicKey,
                                   UA_ByteString *out) {
    Policy_Context_EccAead *pctx = (Policy_Context_EccAead *)policy->policyContext;
    Channel_Context_EccAead *cc = (Channel_Context_EccAead *)channelContext;
    const UA_EccAeadParams *params = eccAead_params(policy);
    if(!pctx || !cc || !params)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;

    if(params->kexKind == ECC_AEAD_KEX_XDH)
        return UA_OpenSSL_XDHE_DeriveSessionKey(pctx->keyId, params->hkdfHash,
                                                pctx->applicationType,
                                                cc->localEphemeralKeyPair,
                                                receiverPublicKey,
                                                senderPublicKey, out);

    return UA_OpenSSL_ECC_DeriveSessionKey(pctx->keyId, params->hkdfHash,
                                           pctx->applicationType,
                                           cc->localEphemeralKeyPair,
                                           receiverPublicKey,
                                           senderPublicKey, out);
}

/************************/
/* Symmetric Key Setup  */
/************************/

static size_t
eccAead_symEncGetLocalKeyLength(const UA_SecurityPolicy *policy,
                                const void *channelContext) {
    const UA_EccAeadParams *params = eccAead_params(policy);
    return params ? params->symEncryptionKeyLength : 0;
}

static size_t
eccAead_symEncGetRemoteKeyLength(const UA_SecurityPolicy *policy,
                                 const void *channelContext) {
    const UA_EccAeadParams *params = eccAead_params(policy);
    return params ? params->symEncryptionKeyLength : 0;
}

/* An AEAD cipher has no separate symmetric signing key: the derived key
 * material is [EncryptionKey | IV] and the authentication tag is produced with
 * the encryption key. (OPC UA Part 6 v1.05: the ECC AEAD policies have
 * DerivedSignatureKeyLength = 0; cf. UA-.NETStandard SecurityPolicyInfo.) */

static size_t
eccAead_symSigGetLocalKeyLength(const UA_SecurityPolicy *policy,
                                const void *channelContext) {
    return 0;
}

static size_t
eccAead_symSigGetRemoteKeyLength(const UA_SecurityPolicy *policy,
                                 const void *channelContext) {
    return 0;
}

static size_t
eccAead_symEncGetBlockSize(const UA_SecurityPolicy *policy,
                           const void *channelContext) {
    /* AEAD ciphers are stream-like -> 1 byte */
    return 1;
}

static size_t
eccAead_symEncGetIvLength(const UA_SecurityPolicy *policy,
                          const void *channelContext) {
    return UA_OPENSSL_AEAD_IV_LENGTH;
}

static UA_StatusCode
eccAead_setLocalSymSigningKey(const UA_SecurityPolicy *policy,
                              void *channelContext, const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccAead *cc = (Channel_Context_EccAead *)channelContext;
    UA_ByteString_clear(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}

static UA_StatusCode
eccAead_setLocalSymEncryptingKey(const UA_SecurityPolicy *policy,
                                 void *channelContext, const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccAead *cc = (Channel_Context_EccAead *)channelContext;
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

static UA_StatusCode
eccAead_setLocalSymIv(const UA_SecurityPolicy *policy,
                      void *channelContext, const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccAead *cc = (Channel_Context_EccAead *)channelContext;
    UA_ByteString_clear(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

static UA_StatusCode
eccAead_setRemoteSymSigningKey(const UA_SecurityPolicy *policy,
                               void *channelContext, const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccAead *cc = (Channel_Context_EccAead *)channelContext;
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

static UA_StatusCode
eccAead_setRemoteSymEncryptingKey(const UA_SecurityPolicy *policy,
                                  void *channelContext, const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccAead *cc = (Channel_Context_EccAead *)channelContext;
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

static UA_StatusCode
eccAead_setRemoteSymIv(const UA_SecurityPolicy *policy,
                       void *channelContext, const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccAead *cc = (Channel_Context_EccAead *)channelContext;
    UA_ByteString_clear(&cc->remoteSymIv);
    return UA_ByteString_copy(iv, &cc->remoteSymIv);
}

/*************************/
/* Asymmetric Signature  */
/*************************/

static UA_StatusCode
eccAead_asymSign(const UA_SecurityPolicy *policy,
                 void *channelContext, const UA_ByteString *message,
                 UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Policy_Context_EccAead *pc = (Policy_Context_EccAead *)policy->policyContext;
    const UA_EccAeadParams *params = eccAead_params(policy);
    if(!pc || !params)
        return UA_STATUSCODE_BADINTERNALERROR;
    return params->asymSign(message, pc->localPrivateKey, signature);
}

static UA_StatusCode
eccAead_asymVerify(const UA_SecurityPolicy *policy, void *channelContext,
                   const UA_ByteString *message, const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccAead *cc = (Channel_Context_EccAead *)channelContext;
    const UA_EccAeadParams *params = eccAead_params(policy);
    if(!params)
        return UA_STATUSCODE_BADINTERNALERROR;
    return params->asymVerify(message, cc->remoteCertificateX509, signature);
}

static UA_StatusCode
eccAead_asymDummy(const UA_SecurityPolicy *policy,
                  void *channelContext, UA_ByteString *data) {
    return UA_STATUSCODE_GOOD; /* Do nothing and return true */
}

/**********************************/
/* Symmetric Signature (AEAD tag) */
/**********************************/

/* The sign-only message security mode. The entire message is additional
 * authenticated data and the signature is the AEAD tag over it, produced with
 * the encryption key. Unlike UA_OpenSSL_AEAD_Encrypt/Decrypt, the tag lives in
 * its own buffer here instead of trailing the message. */

static size_t
eccAead_symSigGetLocalSignatureSize(const UA_SecurityPolicy *policy,
                                    const void *channelContext) {
    return UA_OPENSSL_AEAD_TAG_LENGTH;
}

static size_t
eccAead_symSigGetRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                     const void *channelContext) {
    return UA_OPENSSL_AEAD_TAG_LENGTH;
}

static UA_StatusCode
eccAead_symSign(const UA_SecurityPolicy *policy,
                void *channelContext, const UA_ByteString *message,
                UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(signature->length < UA_OPENSSL_AEAD_TAG_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;

    Channel_Context_EccAead *cc = (Channel_Context_EccAead *)channelContext;
    const UA_EccAeadParams *params = eccAead_params(policy);
    if(!params)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Compute masked IV (same nonce derivation as encrypt mode) */
    UA_Byte maskedIv[UA_OPENSSL_AEAD_IV_LENGTH];
    eccAead_maskIv(maskedIv, &cc->localSymIv,
                   cc->tokenId, cc->previousSequenceNumber);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if(!ctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;
    if(EVP_EncryptInit_ex(ctx, params->aeadCipher(), NULL, NULL, NULL) != 1)
        goto errout;
    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN,
                           UA_OPENSSL_AEAD_IV_LENGTH, NULL) != 1)
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

    /* Get the 16-byte authentication tag */
    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG,
                           UA_OPENSSL_AEAD_TAG_LENGTH, signature->data) != 1)
        goto errout;
    signature->length = UA_OPENSSL_AEAD_TAG_LENGTH;
    ret = UA_STATUSCODE_GOOD;

errout:
    EVP_CIPHER_CTX_free(ctx);
    return ret;
}

static UA_StatusCode
eccAead_symVerify(const UA_SecurityPolicy *policy, void *channelContext,
                  const UA_ByteString *message, const UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(signature->length < UA_OPENSSL_AEAD_TAG_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    Channel_Context_EccAead *cc = (Channel_Context_EccAead *)channelContext;
    const UA_EccAeadParams *params = eccAead_params(policy);
    if(!params)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Compute masked IV using the remote IV */
    UA_Byte maskedIv[UA_OPENSSL_AEAD_IV_LENGTH];
    eccAead_maskIv(maskedIv, &cc->remoteSymIv,
                   cc->tokenId, cc->previousSequenceNumber);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if(!ctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;
    if(EVP_DecryptInit_ex(ctx, params->aeadCipher(), NULL, NULL, NULL) != 1)
        goto errout;
    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN,
                           UA_OPENSSL_AEAD_IV_LENGTH, NULL) != 1)
        goto errout;
    if(EVP_DecryptInit_ex(ctx, NULL, NULL,
                          cc->remoteSymEncryptingKey.data, maskedIv) != 1)
        goto errout;

    /* Set the expected tag for verification */
    if(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, UA_OPENSSL_AEAD_TAG_LENGTH,
                           (void *)(uintptr_t)signature->data) != 1)
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

/**************************/
/* Symmetric Encryption   */
/**************************/

static UA_StatusCode
eccAead_symEncrypt(const UA_SecurityPolicy *policy,
                   void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccAead *cc = (Channel_Context_EccAead *)channelContext;
    const UA_EccAeadParams *params = eccAead_params(policy);
    if(!params)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Byte maskedIv[UA_OPENSSL_AEAD_IV_LENGTH];
    eccAead_maskIv(maskedIv, &cc->localSymIv,
                   cc->tokenId, cc->previousSequenceNumber);
    UA_ByteString ivBs = {UA_OPENSSL_AEAD_IV_LENGTH, maskedIv};

    return UA_OpenSSL_AEAD_Encrypt(params->aeadCipher(), &ivBs,
                                   &cc->localSymEncryptingKey,
                                   &cc->additionalAuthData, data, true);
}

static UA_StatusCode
eccAead_symDecrypt(const UA_SecurityPolicy *policy,
                   void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccAead *cc = (Channel_Context_EccAead *)channelContext;
    const UA_EccAeadParams *params = eccAead_params(policy);
    if(!params)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Byte maskedIv[UA_OPENSSL_AEAD_IV_LENGTH];
    eccAead_maskIv(maskedIv, &cc->remoteSymIv,
                   cc->tokenId, cc->previousSequenceNumber);
    UA_ByteString ivBs = {UA_OPENSSL_AEAD_IV_LENGTH, maskedIv};

    return UA_OpenSSL_AEAD_Decrypt(params->aeadCipher(), &ivBs,
                                   &cc->remoteSymEncryptingKey,
                                   &cc->additionalAuthData, data, true);
}

static UA_StatusCode
eccAead_compareCertificate(const UA_SecurityPolicy *policy,
                           const void *channelContext,
                           const UA_ByteString *certificate) {
    if(channelContext == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const Channel_Context_EccAead *cc = (const Channel_Context_EccAead *)channelContext;
    return UA_OpenSSL_X509_compare(certificate, cc->remoteCertificateX509);
}

/***************/
/* Constructor */
/***************/

static UA_StatusCode
eccAead_policy(UA_SecurityPolicy *sp, const UA_EccAeadParams *params,
               const UA_ApplicationType applicationType,
               const UA_ByteString localCertificate,
               const UA_ByteString localPrivateKey,
               const UA_Logger *logger) {
    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = logger;
    sp->policyUri = params->policyUri;
    sp->certificateGroupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    sp->certificateTypeId = UA_NODEID_NUMERIC(0, params->certificateTypeId);
    sp->securityLevel = params->securityLevel;
    sp->policyType = UA_SECURITYPOLICYTYPE_ECC_AEAD;

    int keyId = eccAead_resolveKeyId(params);
    if(keyId == 0 || keyId == NID_undef)
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;

    /* Asymmetric Signature (ECDSA or PureEdDSA).
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
    asymSig->uri = params->policyUri;
    asymSig->verify = eccAead_asymVerify;
    asymSig->sign = eccAead_asymSign;
    asymSig->getLocalSignatureSize = eccAead_asymGetLocalSignatureSize;
    asymSig->getRemoteSignatureSize = eccAead_asymGetRemoteSignatureSize;

    /* Asymmetric Encryption (none) */
    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri = UA_STRING("https://profiles.opcfoundation.org/conformanceunit/2720\0");
    asymEnc->encrypt = eccAead_asymDummy;
    asymEnc->decrypt = eccAead_asymDummy;
    asymEnc->getLocalKeyLength = eccAead_asymGetLocalKeyLength;
    asymEnc->getRemoteKeyLength = eccAead_asymGetRemoteKeyLength;
    asymEnc->getRemoteBlockSize = eccAead_asymGetRemoteBlockSize;
    asymEnc->getRemotePlainTextBlockSize = eccAead_asymGetRemotePlainTextBlockSize;

    /* Symmetric Signature (the AEAD tag, 16 bytes) */
    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = params->symUri;
    symSig->verify = eccAead_symVerify;
    symSig->sign = eccAead_symSign;
    symSig->getLocalSignatureSize = eccAead_symSigGetLocalSignatureSize;
    symSig->getRemoteSignatureSize = eccAead_symSigGetRemoteSignatureSize;
    symSig->getLocalKeyLength = eccAead_symSigGetLocalKeyLength;
    symSig->getRemoteKeyLength = eccAead_symSigGetRemoteKeyLength;

    /* Symmetric Encryption (the AEAD cipher) */
    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = params->symUri;
    symEnc->encrypt = eccAead_symEncrypt;
    symEnc->decrypt = eccAead_symDecrypt;
    symEnc->getLocalKeyLength = eccAead_symEncGetLocalKeyLength;
    symEnc->getRemoteKeyLength = eccAead_symEncGetRemoteKeyLength;
    symEnc->getRemoteBlockSize = eccAead_symEncGetBlockSize;
    symEnc->getRemotePlainTextBlockSize = eccAead_symEncGetBlockSize;
    symEnc->getLocalIvLength = eccAead_symEncGetIvLength;

    /* Use the same signature algorithm as the asymmetric component for
     * certificate signing (see standard) */
    sp->certSignatureAlgorithm = sp->asymSignatureAlgorithm;

    /* Direct Method Pointers */
    sp->newChannelContext = eccAead_newChannelContext;
    sp->deleteChannelContext = eccAead_deleteChannelContext;
    sp->setLocalSymEncryptingKey = eccAead_setLocalSymEncryptingKey;
    sp->setLocalSymSigningKey = eccAead_setLocalSymSigningKey;
    sp->setLocalSymIv = eccAead_setLocalSymIv;
    sp->setRemoteSymEncryptingKey = eccAead_setRemoteSymEncryptingKey;
    sp->setRemoteSymSigningKey = eccAead_setRemoteSymSigningKey;
    sp->setRemoteSymIv = eccAead_setRemoteSymIv;
    sp->setMessageSecurityParameters = eccAead_setMessageSecurityParameters;
    sp->compareCertificate = eccAead_compareCertificate;
    sp->generateKey = eccAead_generateKey;
    sp->generateEncryptedSecretKey = eccAead_generateEncryptedSecretKey;
    sp->generateNonce = eccAead_generateNonce;
    sp->nonceLength = params->nonceLength;
    sp->secretLength = params->secretLength;
    sp->certThumbprintLength = params->certThumbprintLength;
    sp->secureChannelEnhancements = true;
    sp->makeCertThumbprint = eccAead_makeCertificateThumbprint;
    sp->compareCertThumbprint = eccAead_compareCertificateThumbprint;
    sp->updateCertificate = eccAead_updateCertificateAndPrivateKey;
    sp->createSigningRequest = eccAead_createSigningRequest;
    sp->clear = eccAead_clearPolicyContext;

    /* Parse the certificate */
    UA_Openssl_Init();

    /* The certificate must be on the curve of the policy */
    UA_StatusCode res =
        eccAead_checkCertificateCurve(&localCertificate, params, keyId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    res = UA_OpenSSL_LoadLocalCertificate(&localCertificate, &sp->localCertificate,
                                          params->certKeyType);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Create the policy context */
    res = eccAead_newPolicyContext(sp, params, keyId, localPrivateKey,
                                   applicationType);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&sp->localCertificate);
        return res;
    }

    return UA_STATUSCODE_GOOD;
}

/***********************/
/* Public Entry Points */
/***********************/

UA_StatusCode
UA_SecurityPolicy_EccNistP256AesGcm(UA_SecurityPolicy *sp,
                                    const UA_ApplicationType applicationType,
                                    const UA_ByteString localCertificate,
                                    const UA_ByteString localPrivateKey,
                                    const UA_Logger *logger) {
    return eccAead_policy(sp, &eccNistP256AesGcmParams, applicationType,
                          localCertificate, localPrivateKey, logger);
}

UA_StatusCode
UA_SecurityPolicy_EccNistP256ChaChaPoly(UA_SecurityPolicy *sp,
                                        const UA_ApplicationType applicationType,
                                        const UA_ByteString localCertificate,
                                        const UA_ByteString localPrivateKey,
                                        const UA_Logger *logger) {
    return eccAead_policy(sp, &eccNistP256ChaChaPolyParams, applicationType,
                          localCertificate, localPrivateKey, logger);
}

UA_StatusCode
UA_SecurityPolicy_EccBrainpoolP256r1AesGcm(UA_SecurityPolicy *sp,
                                           const UA_ApplicationType applicationType,
                                           const UA_ByteString localCertificate,
                                           const UA_ByteString localPrivateKey,
                                           const UA_Logger *logger) {
    return eccAead_policy(sp, &eccBrainpoolP256r1AesGcmParams, applicationType,
                          localCertificate, localPrivateKey, logger);
}

UA_StatusCode
UA_SecurityPolicy_EccBrainpoolP256r1ChaChaPoly(UA_SecurityPolicy *sp,
                                               const UA_ApplicationType applicationType,
                                               const UA_ByteString localCertificate,
                                               const UA_ByteString localPrivateKey,
                                               const UA_Logger *logger) {
    return eccAead_policy(sp, &eccBrainpoolP256r1ChaChaPolyParams, applicationType,
                          localCertificate, localPrivateKey, logger);
}

UA_StatusCode
UA_SecurityPolicy_EccNistP384AesGcm(UA_SecurityPolicy *sp,
                                    const UA_ApplicationType applicationType,
                                    const UA_ByteString localCertificate,
                                    const UA_ByteString localPrivateKey,
                                    const UA_Logger *logger) {
    return eccAead_policy(sp, &eccNistP384AesGcmParams, applicationType,
                          localCertificate, localPrivateKey, logger);
}

UA_StatusCode
UA_SecurityPolicy_EccNistP384ChaChaPoly(UA_SecurityPolicy *sp,
                                        const UA_ApplicationType applicationType,
                                        const UA_ByteString localCertificate,
                                        const UA_ByteString localPrivateKey,
                                        const UA_Logger *logger) {
    return eccAead_policy(sp, &eccNistP384ChaChaPolyParams, applicationType,
                          localCertificate, localPrivateKey, logger);
}

UA_StatusCode
UA_SecurityPolicy_EccBrainpoolP384r1AesGcm(UA_SecurityPolicy *sp,
                                           const UA_ApplicationType applicationType,
                                           const UA_ByteString localCertificate,
                                           const UA_ByteString localPrivateKey,
                                           const UA_Logger *logger) {
    return eccAead_policy(sp, &eccBrainpoolP384r1AesGcmParams, applicationType,
                          localCertificate, localPrivateKey, logger);
}

UA_StatusCode
UA_SecurityPolicy_EccBrainpoolP384r1ChaChaPoly(UA_SecurityPolicy *sp,
                                               const UA_ApplicationType applicationType,
                                               const UA_ByteString localCertificate,
                                               const UA_ByteString localPrivateKey,
                                               const UA_Logger *logger) {
    return eccAead_policy(sp, &eccBrainpoolP384r1ChaChaPolyParams, applicationType,
                          localCertificate, localPrivateKey, logger);
}

UA_StatusCode
UA_SecurityPolicy_EccCurve25519AesGcm(UA_SecurityPolicy *sp,
                                      const UA_ApplicationType applicationType,
                                      const UA_ByteString localCertificate,
                                      const UA_ByteString localPrivateKey,
                                      const UA_Logger *logger) {
    return eccAead_policy(sp, &eccCurve25519AesGcmParams, applicationType,
                          localCertificate, localPrivateKey, logger);
}

UA_StatusCode
UA_SecurityPolicy_EccCurve25519ChaChaPoly(UA_SecurityPolicy *sp,
                                          const UA_ApplicationType applicationType,
                                          const UA_ByteString localCertificate,
                                          const UA_ByteString localPrivateKey,
                                          const UA_Logger *logger) {
    return eccAead_policy(sp, &eccCurve25519ChaChaPolyParams, applicationType,
                          localCertificate, localPrivateKey, logger);
}

UA_StatusCode
UA_SecurityPolicy_EccCurve448AesGcm(UA_SecurityPolicy *sp,
                                    const UA_ApplicationType applicationType,
                                    const UA_ByteString localCertificate,
                                    const UA_ByteString localPrivateKey,
                                    const UA_Logger *logger) {
    return eccAead_policy(sp, &eccCurve448AesGcmParams, applicationType,
                          localCertificate, localPrivateKey, logger);
}

UA_StatusCode
UA_SecurityPolicy_EccCurve448ChaChaPoly(UA_SecurityPolicy *sp,
                                        const UA_ApplicationType applicationType,
                                        const UA_ByteString localCertificate,
                                        const UA_ByteString localPrivateKey,
                                        const UA_Logger *logger) {
    return eccAead_policy(sp, &eccCurve448ChaChaPolyParams, applicationType,
                          localCertificate, localPrivateKey, logger);
}

#endif
