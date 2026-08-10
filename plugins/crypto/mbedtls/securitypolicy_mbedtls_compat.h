/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifndef UA_SECURITYPOLICY_MBEDTLS_COMPAT_H_
#define UA_SECURITYPOLICY_MBEDTLS_COMPAT_H_

#include <open62541/types.h>

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)

#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>

int
UA_mbedTLS_compat_parsePrivateKey(mbedtls_pk_context *target,
                                  const unsigned char *key, size_t keyLength,
                                  const unsigned char *password,
                                  size_t passwordLength);

UA_Boolean
UA_mbedTLS_compat_isEccKeyPair(const mbedtls_pk_context *key);

UA_Boolean
UA_mbedTLS_compat_isRsaKeyPair(const mbedtls_pk_context *key);

UA_Boolean
UA_mbedTLS_compat_verifyCertificateSignature(const mbedtls_x509_crt *certificate,
                                              mbedtls_x509_crt *issuer);

int
UA_mbedTLS_compat_writeCertificateDer(mbedtls_x509write_cert *certificate,
                                      unsigned char *buffer,
                                      size_t bufferSize);

int
UA_mbedTLS_compat_writeCertificatePem(mbedtls_x509write_cert *certificate,
                                      unsigned char *buffer,
                                      size_t bufferSize);

UA_StatusCode
UA_mbedTLS_compat_generateCsrKey(const mbedtls_pk_context *templateKey,
                                 mbedtls_pk_context *generatedKey);

UA_StatusCode
UA_mbedTLS_compat_configureCsrSigningKey(mbedtls_pk_context *key);

int
UA_mbedTLS_compat_writeCsrDer(mbedtls_x509write_csr *request,
                              unsigned char *buffer, size_t bufferSize);

#endif /* UA_ENABLE_ENCRYPTION_MBEDTLS */
#endif /* UA_SECURITYPOLICY_MBEDTLS_COMPAT_H_ */
