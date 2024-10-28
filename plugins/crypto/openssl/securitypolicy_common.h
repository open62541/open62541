/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Wind River Systems, Inc.
 *    Copyright 2020 (c) basysKom GmbH
 *
 */

#ifndef SECURITYPOLICY_OPENSSL_COMMON_H_
#define SECURITYPOLICY_OPENSSL_COMMON_H_

#include <open62541/util.h>
#include <open62541/plugin/securitypolicy.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)

#include <openssl/x509.h>
#include <openssl/evp.h>

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

X509 *
UA_OpenSSL_LoadCertificate(const UA_ByteString *certificate);

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
UA_OpenSSL_LoadLocalCertificate(const UA_ByteString *certificate, UA_ByteString *target);

_UA_END_DECLS

#endif /* defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL) */

#endif /* SECURITYPOLICY_OPENSSL_COMMON_H_ */
