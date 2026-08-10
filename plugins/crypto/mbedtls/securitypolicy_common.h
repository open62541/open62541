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
#if MBEDTLS_VERSION_NUMBER < 0x04000000
#include <mbedtls/ecp.h>
#endif

#include <psa/crypto.h>

// MBEDTLS_ENTROPY_HARDWARE_ALT should be defined if your hardware does not supportd platform entropy

#define UA_SHA1_LENGTH 20
#define UA_MAXSUBJECTLENGTH 512
#define MBEDTLS_SAN_MAX_LEN    64

typedef struct {
    mbedtls_svc_key_id_t id;
    UA_Boolean owned;
} UA_mbedTLS_PsaKey;

UA_StatusCode
UA_mbedTLS_PSA_Init(void);

void
UA_mbedTLS_PsaKey_init(UA_mbedTLS_PsaKey *key);

void
UA_mbedTLS_PsaKey_clear(UA_mbedTLS_PsaKey *key);

UA_StatusCode
UA_mbedTLS_PsaKey_import(UA_mbedTLS_PsaKey *target,
                         psa_key_type_t type, psa_key_usage_t usage,
                         psa_algorithm_t algorithm,
                         const UA_ByteString *material);

UA_StatusCode
UA_mbedTLS_PsaKey_importPk(UA_mbedTLS_PsaKey *target,
                           const mbedtls_pk_context *source,
                           psa_key_usage_t usage,
                           psa_algorithm_t algorithm);

UA_StatusCode
UA_mbedTLS_PsaHashCompute(psa_algorithm_t algorithm,
                          const UA_ByteString *input,
                          UA_ByteString *output);

UA_StatusCode
UA_mbedTLS_PsaMacCompute(mbedtls_svc_key_id_t key,
                         psa_algorithm_t algorithm,
                         const UA_ByteString *input,
                         UA_ByteString *output);

UA_StatusCode
UA_mbedTLS_PsaMacVerify(mbedtls_svc_key_id_t key,
                        psa_algorithm_t algorithm,
                        const UA_ByteString *input,
                        const UA_ByteString *mac);

UA_StatusCode
UA_mbedTLS_PsaRandom(UA_ByteString *output);

UA_StatusCode
UA_mbedTLS_PsaPHash(psa_algorithm_t hashAlgorithm,
                    const UA_ByteString *secret,
                    const UA_ByteString *seed,
                    UA_ByteString *output);

UA_StatusCode
UA_mbedTLS_PsaCipher(mbedtls_svc_key_id_t key,
                     psa_algorithm_t algorithm, UA_Boolean encrypt,
                     const UA_ByteString *iv, UA_ByteString *data);

UA_StatusCode
UA_mbedTLS_PsaAsymmetricSign(const mbedtls_pk_context *key,
                             psa_algorithm_t signatureAlgorithm,
                             psa_algorithm_t hashAlgorithm,
                             const UA_ByteString *message,
                             UA_ByteString *signature);

UA_StatusCode
UA_mbedTLS_PsaAsymmetricVerify(const mbedtls_pk_context *key,
                               psa_algorithm_t signatureAlgorithm,
                               psa_algorithm_t hashAlgorithm,
                               const UA_ByteString *message,
                               const UA_ByteString *signature);

UA_StatusCode
UA_mbedTLS_PsaAsymmetricEncrypt(const mbedtls_pk_context *key,
                                psa_algorithm_t algorithm,
                                size_t plainTextBlockSize,
                                UA_ByteString *data);

UA_StatusCode
UA_mbedTLS_PsaAsymmetricDecrypt(const mbedtls_pk_context *key,
                                psa_algorithm_t algorithm,
                                UA_ByteString *data);

UA_StatusCode
UA_mbedTLS_PsaEccGenerate(psa_ecc_family_t family, size_t bits,
                          UA_mbedTLS_PsaKey *keyPair,
                          UA_ByteString *publicKey);

UA_StatusCode
UA_mbedTLS_PsaEccDerive(psa_algorithm_t hashAlgorithm,
                        const UA_ApplicationType applicationType,
                        const UA_mbedTLS_PsaKey *localEphemeralKeyPair,
                        const UA_ByteString *key1,
                        const UA_ByteString *key2,
                        UA_ByteString *output);

#if MBEDTLS_VERSION_NUMBER >= 0x04000000
UA_StatusCode
UA_mbedTLS_createSigningRequestV4(mbedtls_pk_context *localPrivateKey,
                                  mbedtls_pk_context *csrLocalPrivateKey,
                                  UA_SecurityPolicy *securityPolicy,
                                  const UA_String *subjectName,
                                  const UA_ByteString *nonce,
                                  UA_ByteString *csr,
                                  UA_ByteString *newPrivateKey);
#endif

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

typedef struct {
    UA_ByteString localSymIv;
    UA_ByteString remoteSymIv;

    mbedtls_x509_crt remoteCertificate;
    UA_mbedTLS_PsaKey localSymSigningKeyPsa;
    UA_mbedTLS_PsaKey localSymEncryptingKeyPsa;
    UA_mbedTLS_PsaKey remoteSymSigningKeyPsa;
    UA_mbedTLS_PsaKey remoteSymEncryptingKeyPsa;
} mbedtls_ChannelContext;

typedef struct {
    UA_ByteString localCertThumbprint;
    mbedtls_pk_context localPrivateKey;
    mbedtls_pk_context csrLocalPrivateKey;
} mbedtls_PolicyContext;

void
UA_mbedTLS_ChannelContext_initPsa(mbedtls_ChannelContext *context);

void
UA_mbedTLS_ChannelContext_clearPsa(mbedtls_ChannelContext *context);

void
swapBuffers(UA_ByteString *const bufA, UA_ByteString *const bufB);

#if MBEDTLS_VERSION_NUMBER < 0x04000000
UA_StatusCode
mbedtls_createSigningRequest(mbedtls_pk_context *localPrivateKey,
                             mbedtls_pk_context *csrLocalPrivateKey,
                             UA_SecurityPolicy *securityPolicy,
                             const UA_String *subjectName,
                             const UA_ByteString *nonce,
                             UA_ByteString *csr,
                             UA_ByteString *newPrivateKey);
#endif

UA_StatusCode
UA_mbedTLS_thumbprintSha1(const UA_ByteString *certificate,
                        UA_ByteString *thumbprint);

int UA_mbedTLS_LoadPrivateKey(const UA_ByteString *key,
                              mbedtls_pk_context *target);

UA_StatusCode
UA_mbedTLS_LoadCertificate(const UA_ByteString *certificate, mbedtls_x509_crt *target);

UA_Boolean
UA_mbedTLS_IsEccKeyPair(const mbedtls_pk_context *key);

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
