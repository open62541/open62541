/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Wind River Systems, Inc.
 *    Copyright 2020 (c) basysKom GmbH
 *    Copyright 2024 (c) Siemens AG (Authors: Tin Raic, Thomas Zeschg)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#ifndef SECURITYPOLICY_OPENSSL_COMMON_H_
#define SECURITYPOLICY_OPENSSL_COMMON_H_

#include <open62541/util.h>
#include <open62541/plugin/securitypolicy.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)

#include <openssl/x509.h>
#include <openssl/evp.h>

#define UA_SHA1_LENGTH 20

_UA_BEGIN_DECLS


#if !defined(OPENSSL_VERSION_NUMBER)
#error "OPENSSL_VERSION_NUMBER is not defined."
#endif

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
#define X509_STORE_CTX_set0_trusted_stack(STORE_CTX, CTX_SKTRUSTED) X509_STORE_CTX_trusted_stack(STORE_CTX, CTX_SKTRUSTED)
#endif

#if OPENSSL_VERSION_NUMBER < 0x1010000fL || ( defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x3050000fL)
#define X509_STORE_CTX_get_check_issued(STORE_CTX) STORE_CTX->check_issued
#endif

#if OPENSSL_VERSION_NUMBER < 0x1010000fL || ( defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x3050000fL)
#define get_pkey_rsa(evp) ((evp)->pkey.rsa)
#else
#define get_pkey_rsa(evp) EVP_PKEY_get0_RSA(evp)
#endif

#if OPENSSL_VERSION_NUMBER < 0x1010000fL || defined(LIBRESSL_VERSION_NUMBER)
#define X509_get0_subject_key_id(PX509_CERT) (const ASN1_OCTET_STRING *)X509_get_ext_d2i(PX509_CERT, NID_subject_key_identifier, NULL, NULL);
#endif

#if OPENSSL_VERSION_NUMBER < 0x2000000fL || defined(LIBRESSL_VERSION_NUMBER)
#define get_error_line_data(pFile, pLine, pData, pFlags) ERR_get_error_line_data(pFile, pLine, pData, pFlags)
#else
#define get_error_line_data(pFile, pLine, pData, pFlags) ERR_get_error_all(pFile, pLine, NULL, pData, pFlags)
#endif

typedef struct {
    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;
    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    UA_ByteString remoteCertificate;
    X509 *remoteCertificateX509; /* X509 */
} openssl_ChannelContext;

typedef struct {
    EVP_PKEY *localPrivateKey;
    EVP_PKEY *csrLocalPrivateKey;
    UA_ByteString localCertThumbprint;

    /* keyType == EVP_PKEY_NONE omits the check */
    /* EVP_PKEY_RSA */
    /* EVP_PKEY_EC */
    /* EVP_PKEY_ED25519 */
    /* EVP_PKEY_ED448 */
    int keyType;

    /* Set by the generic RSA module: its UA_RsaParams row. NULL for the
     * hand-written policies that do not use that module. */
    const void *rsaParams;
} openssl_PolicyContext;

void saveDataToFile(const char *fileName, const UA_ByteString *str);
void UA_Openssl_Init(void);

UA_StatusCode
UA_copyCertificate(UA_ByteString *dst, const UA_ByteString *src);

UA_StatusCode
UA_OpenSSL_RSA_PKCS1_V15_SHA256_Verify(const UA_ByteString *msg,
                                       X509 *publicKeyX509,
                                       const UA_ByteString *signature);

UA_StatusCode
UA_OpenSSL_RSA_PSS_SHA256_Verify (const UA_ByteString * msg,
                                  X509 *publicKeyX509,
                                  const UA_ByteString * signature);

UA_StatusCode
UA_Openssl_X509_GetCertificateThumbprint(const UA_ByteString *certficate,
                                         UA_ByteString *pThumbprint,
                                         bool bThumbPrint);
UA_StatusCode
UA_Openssl_RSA_Oaep_Decrypt(UA_ByteString *data,
                            EVP_PKEY *privateKey);

UA_StatusCode
UA_Openssl_RSA_Oaep_Sha2_Decrypt (UA_ByteString *data,
                            EVP_PKEY *privateKey);

UA_StatusCode
UA_Openssl_RSA_OAEP_Encrypt(UA_ByteString *data, /* The data that is encrypted.
                                                    The encrypted data will overwrite
                                                    the data that was supplied.  */
                             size_t paddingSize, X509 *publicX509);

UA_StatusCode
UA_Openssl_RSA_OAEP_SHA2_Encrypt (UA_ByteString * data,
                                  size_t          paddingSize,
                                  X509 *          publicX509);

UA_StatusCode
UA_Openssl_Random_Key_PSHA256_Derive(const UA_ByteString *secret,
                                     const UA_ByteString *seed,
                                     UA_ByteString *out);

UA_StatusCode
UA_Openssl_RSA_Public_GetKeyLength(X509 *publicKeyX509, UA_Int32 *keyLen);

UA_StatusCode
UA_Openssl_RSA_PKCS1_V15_SHA256_Sign(const UA_ByteString *data,
                                     EVP_PKEY *privateKey,
                                     UA_ByteString *outSignature);

UA_StatusCode
UA_Openssl_RSA_PSS_SHA256_Sign (const UA_ByteString * message,
                                EVP_PKEY * privateKey,
                                UA_ByteString *       outSignature);

/* Generate an ephemeral ECDH key pair on the given curve and write its
 * encoded public key (without the leading 0x04 point-format byte) to
 * `keyPublicEncOut`. The named wrappers are the common curves; the generic ECC
 * AEAD module passes the curve NID it resolved from its policy parameters. */
UA_StatusCode
UA_OpenSSL_ECC_GenerateKey(const int curveId,
                           EVP_PKEY **keyPairOut,
                           UA_ByteString *keyPublicEncOut);

UA_StatusCode
UA_OpenSSL_ECC_NISTP256_GenerateKey (EVP_PKEY ** keyPairOut,
                                     UA_ByteString * keyPublicEncOut);

/* ECDH key agreement and HKDF key derivation. `key1` and `key2` are the
 * nonces (ephemeral public keys). `ikmInOut` is the v1.05.07 IKM accumulator of
 * an enhanced policy: IKM_n-1 on entry, IKM_n on return. NULL for policies
 * without IKM chaining and for derivations outside a SecureChannel. */
UA_StatusCode
UA_OpenSSL_ECC_DeriveKeys (const int curveID,
                           char * hashAlgorithm,
                           const UA_ApplicationType applicationType,
                           EVP_PKEY * localEphemeralKeyPair,
                           const UA_ByteString * key1,
                           const UA_ByteString * key2,
                           UA_ByteString * ikmInOut,
                           UA_ByteString * out);

/* Split the `secret` argument of UA_SecurityPolicy.generateKey into the IKM
 * accumulator the SecureChannel prepends for an enhanced policy and the nonce.
 * Without a prepend, `ikmInOut` is empty and `nonce` is the whole `secret`.
 * `ikmInOut` aliases `secret`: the updated accumulator is written back into
 * the SecureChannel's buffer. */
void
UA_OpenSSL_splitIKM(const UA_SecurityPolicy *policy, const UA_ByteString *secret,
                    UA_ByteString *ikmInOut, UA_ByteString *nonce);

UA_StatusCode
UA_Openssl_ECDSA_SHA256_Sign (const UA_ByteString * message,
                              EVP_PKEY * privateKey,
                              UA_ByteString * outSignature);

UA_StatusCode
UA_Openssl_ECDSA_SHA256_Verify (const UA_ByteString * message,
                                X509 * publicKeyX509,
                                const UA_ByteString * signature);

UA_StatusCode
UA_OpenSSL_HMAC_SHA256_Verify(const UA_ByteString *message,
                              const UA_ByteString *key,
                              const UA_ByteString *signature);

UA_StatusCode
UA_OpenSSL_HMAC_SHA256_Sign(const UA_ByteString *message,
                            const UA_ByteString *key,
                            UA_ByteString *signature);

UA_StatusCode
UA_OpenSSL_AES_256_CBC_Decrypt(const UA_ByteString *iv,
                               const UA_ByteString *key,
                               UA_ByteString *data  /* [in/out]*/);

UA_StatusCode
UA_OpenSSL_AES_256_CBC_Encrypt(const UA_ByteString *iv,
                               const UA_ByteString *key,
                               UA_ByteString *data  /* [in/out]*/);

UA_StatusCode
UA_OpenSSL_X509_compare(const UA_ByteString *cert, const X509 *b);

UA_StatusCode
UA_Openssl_RSA_Private_GetKeyLength(EVP_PKEY *privateKey,
                                    UA_Int32 *keyLen) ;

UA_StatusCode
UA_OpenSSL_RSA_PKCS1_V15_SHA1_Verify(const UA_ByteString *msg,
                                      X509 *publicKeyX509,
                                      const UA_ByteString *signature);

UA_StatusCode
UA_Openssl_RSA_PKCS1_V15_SHA1_Sign(const UA_ByteString *message,
                                   EVP_PKEY *privateKey,
                                   UA_ByteString *outSignature);
UA_StatusCode
UA_Openssl_Random_Key_PSHA1_Derive(const UA_ByteString *secret,
                                   const UA_ByteString *seed,
                                   UA_ByteString *out);
UA_StatusCode
UA_OpenSSL_HMAC_SHA1_Verify(const UA_ByteString *message,
                            const UA_ByteString *key,
                            const UA_ByteString *signature);

UA_StatusCode
UA_OpenSSL_HMAC_SHA1_Sign(const UA_ByteString *message,
                          const UA_ByteString *key,
                          UA_ByteString *signature);

UA_StatusCode
UA_Openssl_RSA_PKCS1_V15_Decrypt(UA_ByteString *data,
                                 EVP_PKEY *privateKey);

UA_StatusCode
UA_Openssl_RSA_PKCS1_V15_Encrypt(UA_ByteString *data,
                                 size_t paddingSize,
                                 X509 *publicX509);

UA_StatusCode
UA_OpenSSL_AES_128_CBC_Decrypt(const UA_ByteString *iv,
                               const UA_ByteString *key,
                               UA_ByteString *data  /* [in/out]*/);

UA_StatusCode
UA_OpenSSL_AES_128_CBC_Encrypt(const UA_ByteString *iv,
                               const UA_ByteString *key,
                               UA_ByteString *data  /* [in/out]*/);

UA_StatusCode
UA_OpenSSL_CreateSigningRequest(EVP_PKEY *localPrivateKey,
                                EVP_PKEY **csrLocalPrivateKey,
                                UA_SecurityPolicy *securityPolicy,
                                const UA_String *subjectName,
                                const UA_ByteString *nonce,
                                UA_ByteString *csr,
                                UA_ByteString *newPrivateKey);

EVP_PKEY *
UA_OpenSSL_LoadPrivateKey(const UA_ByteString *privateKey);

/* Check for the correct RSA/ECC certificate */
/* keyType == EVP_PKEY_NONE omits the check */
/* EVP_PKEY_RSA */
/* EVP_PKEY_EC */
/* EVP_PKEY_ED25519 */
/* EVP_PKEY_ED448 */
X509 *
UA_OpenSSL_LoadCertificate(const UA_ByteString *certificate,
                           int keyType);

X509 *
UA_OpenSSL_LoadDerCertificate(const UA_ByteString *certificate);

X509 *
UA_OpenSSL_LoadPemCertificate(const UA_ByteString *certificate);

X509_CRL *
UA_OpenSSL_LoadCrl(const UA_ByteString *crl);

X509_CRL *
UA_OpenSSL_LoadDerCrl(const UA_ByteString *crl);

X509_CRL *
UA_OpenSSL_LoadPemCrl(const UA_ByteString *crl);

UA_StatusCode
UA_OpenSSL_LoadLocalCertificate(const UA_ByteString *certificate,
                                UA_ByteString *target,
                                int keyType);

UA_StatusCode
UA_OpenSSL_setLocalSymSigningKey_generic(const UA_SecurityPolicy *policy,
                                        void *channelContext,
                                        const UA_ByteString *key);

UA_StatusCode
UA_OpenSSL_setLocalSymEncryptingKey_generic(const UA_SecurityPolicy *policy,
                                           void *channelContext,
                                           const UA_ByteString *key);

UA_StatusCode
UA_OpenSSL_setLocalSymIv_generic(const UA_SecurityPolicy *policy,
                                void *channelContext,
                                const UA_ByteString *iv);

UA_StatusCode
UA_OpenSSL_setRemoteSymSigningKey_generic(const UA_SecurityPolicy *policy,
                                         void *channelContext,
                                         const UA_ByteString *key);

UA_StatusCode
UA_OpenSSL_setRemoteSymEncryptingKey_generic(const UA_SecurityPolicy *policy,
                                            void *channelContext,
                                            const UA_ByteString *key);

UA_StatusCode
UA_OpenSSL_setRemoteSymIv_generic(const UA_SecurityPolicy *policy,
                                 void *channelContext,
                                 const UA_ByteString *iv);

UA_StatusCode
UA_OpenSSL_compareCertificate_generic(const UA_SecurityPolicy *policy,
                                      const void *channelContext,
                                      const UA_ByteString *certificate);

void
UA_OpenSSL_Policy_clearContext_generic(UA_SecurityPolicy *policy);

UA_StatusCode
UA_OpenSSL_Policy_newContext_generic(UA_SecurityPolicy *securityPolicy,
                                     const UA_ByteString localPrivateKey,
                                     int keyType,
                                     const UA_Logger *logger);

UA_StatusCode
UA_OpenSSL_SecurityPolicy_updateCertificate_generic(UA_SecurityPolicy *securityPolicy,
                                                    const UA_ByteString newCertificate,
                                                    const UA_ByteString newPrivateKey);

UA_StatusCode
UA_OpenSSL_SecurityPolicy_compareCertThumbprint_generic(const UA_SecurityPolicy *securityPolicy,
                                                        const UA_ByteString *certificateThumbprint);

UA_StatusCode
UA_OpenSSL_ECC_NISTP384_GenerateKey (EVP_PKEY ** keyPairOut,
                                     UA_ByteString * keyPublicEncOut);

UA_StatusCode
UA_Openssl_ECDSA_SHA384_Sign (const UA_ByteString * message,
                              EVP_PKEY * privateKey,
                              UA_ByteString * outSignature);

UA_StatusCode
UA_Openssl_ECDSA_SHA384_Verify (const UA_ByteString * message,
                                X509 * publicKeyX509,
                                const UA_ByteString * signature);

UA_StatusCode
UA_OpenSSL_HMAC_SHA384_Verify(const UA_ByteString *message,
                              const UA_ByteString *key,
                              const UA_ByteString *signature);

UA_StatusCode
UA_OpenSSL_HMAC_SHA384_Sign(const UA_ByteString *message,
                            const UA_ByteString *key,
                            UA_ByteString *signature);


UA_StatusCode
UA_OpenSSL_ECC_BRAINPOOLP256R1_GenerateKey (EVP_PKEY ** keyPairOut,
                                             UA_ByteString * keyPublicEncOut);


UA_StatusCode
UA_OpenSSL_ECC_BRAINPOOLP384R1_GenerateKey (EVP_PKEY ** keyPairOut,
                                             UA_ByteString * keyPublicEncOut);


/* EdDSA Ed25519 signing and verification */
UA_StatusCode
UA_OpenSSL_EdDSA_Ed25519_Sign(const UA_ByteString *message,
                              EVP_PKEY *privateKey,
                              UA_ByteString *outSignature);

UA_StatusCode
UA_OpenSSL_EdDSA_Ed25519_Verify(const UA_ByteString *message,
                                X509 *publicKeyX509,
                                const UA_ByteString *signature);

/* X25519/X448 key generation */
UA_StatusCode
UA_OpenSSL_X25519_GenerateKey(EVP_PKEY **keyPairOut,
                              UA_ByteString *keyPublicEncOut);

/* X25519/X448 ECDH key agreement and HKDF key derivation */
UA_StatusCode
UA_OpenSSL_XDHE_DeriveKeys(int keyType,
                            const char *hashAlgorithm,
                            UA_ApplicationType applicationType,
                            EVP_PKEY *localEphemeralKeyPair,
                            const UA_ByteString *key1,
                            const UA_ByteString *key2,
                            UA_ByteString *ikmInOut,
                            UA_ByteString *out);

/* Every ECC AEAD SecurityPolicy uses a 12-byte IV and a 16-byte tag
 * (OPC UA Part 6: InitializationVectorLength 96 bits, SignatureLength 128 bits). */
#define UA_OPENSSL_AEAD_IV_LENGTH 12
#define UA_OPENSSL_AEAD_TAG_LENGTH 16

/* Generic AEAD encrypt/decrypt for the ECC AEAD SecurityPolicies.
 *
 * The cipher selects the algorithm and, through EVP_CIPHER_key_length(), the
 * expected key length: AES-128-GCM (16), AES-256-GCM (32) and
 * ChaCha20-Poly1305 (32) all share this code path. The IV is always 12 bytes
 * and the authentication tag is the last 16 bytes of `data`.
 *
 * If encryptData/decryptData is true, the data part is encrypted or decrypted.
 * Otherwise all of it is treated as additional authenticated data, so only the
 * tag is computed/verified (the sign-only message security mode). On decrypt
 * the tag is stripped from data->length. */
UA_StatusCode
UA_OpenSSL_AEAD_Encrypt(const EVP_CIPHER *cipher,
                        const UA_ByteString *iv,
                        const UA_ByteString *key,
                        const UA_ByteString *aad,
                        UA_ByteString *data,
                        UA_Boolean encryptData);

UA_StatusCode
UA_OpenSSL_AEAD_Decrypt(const EVP_CIPHER *cipher,
                        const UA_ByteString *iv,
                        const UA_ByteString *key,
                        const UA_ByteString *aad,
                        UA_ByteString *data,
                        UA_Boolean decryptData);

/* ChaCha20-Poly1305 AEAD encrypt/decrypt. Thin wrappers over
 * UA_OpenSSL_AEAD_Encrypt/Decrypt. */
UA_StatusCode
UA_OpenSSL_ChaCha20Poly1305_Encrypt(const UA_ByteString *iv,
                                    const UA_ByteString *key,
                                    const UA_ByteString *aad,
                                    UA_ByteString *data,
                                    UA_Boolean encryptData);

UA_StatusCode
UA_OpenSSL_ChaCha20Poly1305_Decrypt(const UA_ByteString *iv,
                                    const UA_ByteString *key,
                                    const UA_ByteString *aad,
                                    UA_ByteString *data,
                                    UA_Boolean decryptData);

/* AES-128-GCM AEAD encrypt/decrypt. Thin wrappers over
 * UA_OpenSSL_AEAD_Encrypt/Decrypt. */
UA_StatusCode
UA_OpenSSL_AES_128_GCM_Encrypt(const UA_ByteString *iv,
                               const UA_ByteString *key,
                               const UA_ByteString *aad,
                               UA_ByteString *data,
                               UA_Boolean encryptData);

UA_StatusCode
UA_OpenSSL_AES_128_GCM_Decrypt(const UA_ByteString *iv,
                               const UA_ByteString *key,
                               const UA_ByteString *aad,
                               UA_ByteString *data,
                               UA_Boolean decryptData);


/* EdDSA Ed448 signing and verification */
UA_StatusCode
UA_OpenSSL_EdDSA_Ed448_Sign(const UA_ByteString *message,
                            EVP_PKEY *privateKey,
                            UA_ByteString *outSignature);

UA_StatusCode
UA_OpenSSL_EdDSA_Ed448_Verify(const UA_ByteString *message,
                              X509 *publicKeyX509,
                              const UA_ByteString *signature);

/* X448 key generation */
UA_StatusCode
UA_OpenSSL_X448_GenerateKey(EVP_PKEY **keyPairOut,
                            UA_ByteString *keyPublicEncOut);

_UA_END_DECLS

#endif /* defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL) */

#endif /* SECURITYPOLICY_OPENSSL_COMMON_H_ */
