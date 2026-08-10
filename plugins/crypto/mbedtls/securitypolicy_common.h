/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_SECURITYPOLICY_MBEDTLS_COMMON_H_
#define UA_SECURITYPOLICY_MBEDTLS_COMMON_H_

#include <open62541/plugin/securitypolicy.h>

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)

#include <mbedtls/x509_crt.h>

#include <psa/crypto.h>

/* Define MBEDTLS_ENTROPY_HARDWARE_ALT when the platform has no default
 * entropy source. */

#define UA_SHA1_LENGTH 20
#define UA_MAXSUBJECTLENGTH 512
#define MBEDTLS_SAN_MAX_LEN    64

typedef struct {
    mbedtls_svc_key_id_t id;
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
UA_mbedTLS_PsaEccDerive(psa_algorithm_t hashAlgorithm,
                        const UA_ApplicationType applicationType,
                        const UA_mbedTLS_PsaKey *localEphemeralKeyPair,
                        const UA_ByteString *key1,
                        const UA_ByteString *key2,
                        UA_ByteString *output);

UA_StatusCode
UA_mbedTLS_EccGenerateNonce(const UA_SecurityPolicy *policy,
                            UA_mbedTLS_PsaKey *ephemeralKey,
                            psa_ecc_family_t family, size_t bits,
                            UA_ByteString *output);

_UA_BEGIN_DECLS

typedef struct {
    UA_ByteString localSymIv;
    UA_ByteString remoteSymIv;

    mbedtls_x509_crt remoteCertificate;
    UA_mbedTLS_PsaKey localSymSigningKey;
    UA_mbedTLS_PsaKey localSymEncryptingKey;
    UA_mbedTLS_PsaKey remoteSymSigningKey;
    UA_mbedTLS_PsaKey remoteSymEncryptingKey;
    psa_algorithm_t symmetricMacAlgorithm;
} mbedtls_ChannelContext;

typedef struct {
    UA_ByteString localCertThumbprint;
    mbedtls_pk_context localPrivateKey;
    mbedtls_pk_context csrLocalPrivateKey;
} mbedtls_PolicyContext;

void
UA_mbedTLS_ChannelContext_init(mbedtls_ChannelContext *context);

void
UA_mbedTLS_ChannelContext_clear(mbedtls_ChannelContext *context);

void
UA_mbedTLS_PolicyContext_init(mbedtls_PolicyContext *context);

void
UA_mbedTLS_PolicyContext_clear(mbedtls_PolicyContext *context);

UA_StatusCode
UA_mbedTLS_thumbprintSha1(const UA_ByteString *certificate,
                        UA_ByteString *thumbprint);

UA_StatusCode
UA_mbedTLS_makeCertificateThumbprint_generic(const UA_SecurityPolicy *policy,
                                              const UA_ByteString *certificate,
                                              UA_ByteString *thumbprint);

UA_StatusCode
UA_mbedTLS_LoadPrivateKey(const UA_ByteString *key,
                          mbedtls_pk_context *target);

UA_StatusCode
UA_mbedTLS_LoadCertificate(const UA_ByteString *certificate, mbedtls_x509_crt *target);

UA_StatusCode
UA_mbedTLS_LoadCrl(const UA_ByteString *crl, mbedtls_x509_crl *target);

UA_StatusCode UA_mbedTLS_LoadLocalCertificate(const UA_ByteString *certData, UA_ByteString *target);

UA_StatusCode
UA_mbedTLS_UpdateCertificateAndPrivateKey(UA_SecurityPolicy *securityPolicy,
                                          const UA_ByteString newCertificate,
                                          const UA_ByteString newPrivateKey);

UA_StatusCode
UA_mbedTLS_CopyDataFormatAware(const UA_ByteString *data,
                               UA_ByteString *result);

void
UA_mbedTLS_clearSensitiveByteString(UA_ByteString *value);

size_t
UA_mbedTLS_getRemoteCertificateKeyLength(const UA_SecurityPolicy *policy,
                                         const void *channelContext);

size_t
UA_mbedTLS_getRemoteCertificateKeyBitLength(const UA_SecurityPolicy *policy,
                                            const void *channelContext);

size_t
UA_mbedTLS_symmetricEncryptionBlockSize(const UA_SecurityPolicy *policy,
                                        const void *channelContext);

UA_StatusCode
UA_mbedTLS_symmetricSign(mbedtls_ChannelContext *context, size_t signatureLength,
                         const UA_ByteString *message, UA_ByteString *signature);

UA_StatusCode
UA_mbedTLS_symmetricVerify(mbedtls_ChannelContext *context, size_t signatureLength,
                           const UA_ByteString *message,
                           const UA_ByteString *signature);

UA_StatusCode
UA_mbedTLS_symmetricEncrypt(const UA_SecurityPolicy *policy,
                            void *channelContext, UA_ByteString *data);

UA_StatusCode
UA_mbedTLS_symmetricDecrypt(const UA_SecurityPolicy *policy,
                            void *channelContext, UA_ByteString *data);

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
UA_mbedTLS_getLocalPrivateKeyLength(const UA_SecurityPolicy *policy,
                                    const void *channelContext);

size_t
UA_mbedTLS_getLocalPrivateKeyBitLength(const UA_SecurityPolicy *policy,
                                       const void *channelContext);

UA_StatusCode
UA_mbedTLS_compareCertificateThumbprint_generic(const UA_SecurityPolicy *securityPolicy,
                                                const UA_ByteString *certificateThumbprint);

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
