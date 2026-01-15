/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_SECURITYPOLICY_MBEDTLS_COMMON_H_
#define UA_SECURITYPOLICY_MBEDTLS_COMMON_H_

#include <open62541/plugin/securitypolicy.h>

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)

#include <mbedtls/md.h>
#include <mbedtls/version.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

// MBEDTLS_ENTROPY_HARDWARE_ALT should be defined if your hardware does not supportd platform entropy

#define UA_SHA1_LENGTH 20
#define UA_MAXSUBJECTLENGTH 512
#define MBEDTLS_SAN_MAX_LEN    64

/* 
 * Define fallback for MBEDTLS_ASN1_CHK_CLEANUP_ADD if not already defined.
 * Some versions of mbedTLS (≥3.x) provide this macro via <mbedtls/asn1write.h>,
 * but it may be missing in others, or unavailable in amalgamation builds.
 *
 * This guard ensures compatibility across mbedTLS versions without redefining
 * an existing macro, avoiding compiler warnings in UA_ENABLE_AMALGAMATION mode.
 */
#ifndef MBEDTLS_ASN1_CHK_CLEANUP_ADD
#define MBEDTLS_ASN1_CHK_CLEANUP_ADD(g, f)                    \
    do {                                                      \
        if ((ret = (f)) < 0)                                  \
            goto cleanup;                                     \
        else                                                  \
            (g) += ret;                                       \
    } while (0)
#endif

_UA_BEGIN_DECLS

#if MBEDTLS_VERSION_NUMBER < 0x02170000
#define MBEDTLS_X509_SAN_OTHER_NAME                      0
#define MBEDTLS_X509_SAN_RFC822_NAME                     1
#define MBEDTLS_X509_SAN_DNS_NAME                        2
#define MBEDTLS_X509_SAN_X400_ADDRESS_NAME               3
#define MBEDTLS_X509_SAN_DIRECTORY_NAME                  4
#define MBEDTLS_X509_SAN_EDI_PARTY_NAME                  5
#define MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER     6
#define MBEDTLS_X509_SAN_IP_ADDRESS                      7
#define MBEDTLS_X509_SAN_REGISTERED_ID                   8
#endif

typedef struct {
    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;

    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    mbedtls_x509_crt remoteCertificate;
} mbedtls_ChannelContext;

typedef struct {
    UA_ByteString localCertThumbprint;

    mbedtls_ctr_drbg_context drbgContext;
    mbedtls_entropy_context entropyContext;
    mbedtls_md_context_t mdContext;
    mbedtls_pk_context localPrivateKey;
    mbedtls_pk_context csrLocalPrivateKey;
} mbedtls_PolicyContext;

void
swapBuffers(UA_ByteString *const bufA, UA_ByteString *const bufB);

UA_StatusCode
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
                        UA_ByteString *data, int hash_id);

UA_StatusCode
mbedtls_createSigningRequest(mbedtls_pk_context *localPrivateKey,
                             mbedtls_pk_context *csrLocalPrivateKey,
                             mbedtls_entropy_context *entropyContext,
                             mbedtls_ctr_drbg_context *drbgContext,
                             UA_SecurityPolicy *securityPolicy,
                             const UA_String *subjectName,
                             const UA_ByteString *nonce,
                             UA_ByteString *csr,
                             UA_ByteString *newPrivateKey);

int UA_mbedTLS_LoadPrivateKey(const UA_ByteString *key, mbedtls_pk_context *target, void *p_rng);

UA_StatusCode
UA_mbedTLS_LoadCertificate(const UA_ByteString *certificate, mbedtls_x509_crt *target);

UA_StatusCode
UA_mbedTLS_LoadDerCertificate(const UA_ByteString *certificate, mbedtls_x509_crt *target);

UA_StatusCode
UA_mbedTLS_LoadPemCertificate(const UA_ByteString *certificate, mbedtls_x509_crt *target);

UA_StatusCode
UA_mbedTLS_LoadCrl(const UA_ByteString *crl, mbedtls_x509_crl *target);

UA_StatusCode
UA_mbedTLS_LoadDerCrl(const UA_ByteString *crl, mbedtls_x509_crl *target);

UA_StatusCode
UA_mbedTLS_LoadPemCrl(const UA_ByteString *crl, mbedtls_x509_crl *target);

UA_StatusCode UA_mbedTLS_LoadLocalCertificate(const UA_ByteString *certData, UA_ByteString *target);

UA_ByteString UA_mbedTLS_CopyDataFormatAware(const UA_ByteString *data);

size_t
UA_mbedTLS_asym_getRemoteSignatureSize_generic(const UA_SecurityPolicy *policy, const void *channelContext);

size_t
UA_mbedTLS_asym_getRemoteBlockSize_generic(const UA_SecurityPolicy *policy,
                                           const void *channelContext);

size_t
UA_mbedTLS_asym_cert_getLocalSignatureSize_generic(const UA_SecurityPolicy *policy,
                                                    const void *channelContext);

UA_StatusCode
UA_mbedTLS_setLocalSymEncryptingKey_generic(const UA_SecurityPolicy *policy,
                                            void *channelContext,
                                            const UA_ByteString *key);

UA_StatusCode
UA_mbedTLS_setLocalSymSigningKey_generic(const UA_SecurityPolicy *policy,
                                         void *channelContext,
                                         const UA_ByteString *key);

UA_StatusCode
UA_mbedTLS_setLocalSymIv_generic(const UA_SecurityPolicy *policy,
                                 void *channelContext,
                                 const UA_ByteString *iv);

UA_StatusCode
UA_mbedTLS_setRemoteSymEncryptingKey_generic(const UA_SecurityPolicy *policy,
                                             void *channelContext,
                                             const UA_ByteString *key);

UA_StatusCode
UA_mbedTLS_setRemoteSymSigningKey_generic(const UA_SecurityPolicy *policy,
                                          void *channelContext,
                                          const UA_ByteString *key);

UA_StatusCode
UA_mbedTLS_setRemoteSymIv_generic(const UA_SecurityPolicy *policy,
                                  void *channelContext,
                                  const UA_ByteString *iv);

UA_StatusCode
UA_mbedTLS_compareCertificate_generic(const UA_SecurityPolicy *policy,
                                      const void *channelContext,
                                      const UA_ByteString *certificate);

size_t
UA_mbedTLS_getRemoteCertificatePrivateKeyLength(const UA_SecurityPolicy *policy,
                                                const void *channelContext);

size_t
UA_mbedTLS_getLocalPrivateKeyLength(const UA_SecurityPolicy *policy,
                                    const void *channelContext);

UA_StatusCode
UA_mbedTLS_compareCertificateThumbprint_generic(const UA_SecurityPolicy *securityPolicy,
                                                const UA_ByteString *certificateThumbprint);

UA_StatusCode
UA_mbedTLS_sym_generateKey_generic(const UA_SecurityPolicy *policy,
                                   void *channelContext, const UA_ByteString *secret,
                                   const UA_ByteString *seed, UA_ByteString *out);

UA_StatusCode
UA_mbedTLS_sym_generateNonce_generic(const UA_SecurityPolicy *policy,
                                     void *channelContext, UA_ByteString *out);

UA_StatusCode
UA_mbedTLS_createSigningRequest_generic(UA_SecurityPolicy *securityPolicy,
                                        const UA_String *subjectName,
                                        const UA_ByteString *nonce,
                                        const UA_KeyValueMap *params,
                                        UA_ByteString *csr,
                                        UA_ByteString *newPrivateKey);

_UA_END_DECLS

#endif

#endif /* UA_SECURITYPOLICY_MBEDTLS_COMMON_H_ */
