/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_SECURITYPOLICY_MBEDTLS_COMMON_H_
#define UA_SECURITYPOLICY_MBEDTLS_COMMON_H_

#include <open62541/plugin/securitypolicy.h>

#ifdef UA_ENABLE_ENCRYPTION

#include <mbedtls/md.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/ctr_drbg.h>

#define UA_SHA1_LENGTH 20

_UA_BEGIN_DECLS

void
swapBuffers(UA_ByteString *const bufA, UA_ByteString *const bufB);

void
mbedtls_hmac(mbedtls_md_context_t *context, const UA_ByteString *key,
             const UA_ByteString *in, unsigned char *out);

UA_StatusCode
mbedtls_generateKey(mbedtls_md_context_t *context,
                    const UA_ByteString *secret, const UA_ByteString *seed,
                    UA_ByteString *out);

UA_StatusCode
mbedtls_verifySig_sha1(mbedtls_x509_crt *certificate, const UA_ByteString *message,
                       const UA_ByteString *signature);

UA_StatusCode
mbedtls_sign_sha1(mbedtls_pk_context *localPrivateKey,
                  mbedtls_ctr_drbg_context *drbgContext,
                  const UA_ByteString *message,
                  UA_ByteString *signature);

UA_StatusCode
mbedtls_thumbprint_sha1(const UA_ByteString *certificate,
                        UA_ByteString *thumbprint);

/* Set the hashing scheme before calling
 * E.g. mbedtls_rsa_set_padding(context, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA1); */
UA_StatusCode
mbedtls_encrypt_rsaOaep(mbedtls_rsa_context *context,
                        mbedtls_ctr_drbg_context *drbgContext,
                        UA_ByteString *data, const size_t plainTextBlockSize);

UA_StatusCode
mbedtls_decrypt_rsaOaep(mbedtls_pk_context *localPrivateKey,
                        mbedtls_ctr_drbg_context *drbgContext,
                        UA_ByteString *data);

_UA_END_DECLS

#endif

#endif /* UA_SECURITYPOLICY_MBEDTLS_COMMON_H_ */
