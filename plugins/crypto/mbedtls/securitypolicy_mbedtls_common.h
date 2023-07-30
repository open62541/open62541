/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Kai Huebl)
 */

#ifndef UA_SECURITYPOLICY_MBEDTLS_COMMON_H_
#define UA_SECURITYPOLICY_MBEDTLS_COMMON_H_

#include <open62541/plugin/securitypolicy.h>

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS) || defined(UA_ENABLE_PUBSUB_ENCRYPTION)

#include <mbedtls/md.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/version.h>

// MBEDTLS_ENTROPY_HARDWARE_ALT should be defined if your hardware does not supportd platform entropy

#define UA_SHA1_LENGTH 20
#define UA_SHA256_LENGTH 32

_UA_BEGIN_DECLS

typedef struct {
    mbedtls_ctr_drbg_context drbgContext;
    mbedtls_entropy_context entropyContext;
    mbedtls_md_context_t mdContext;
} PolicyContext_mbedtls;

typedef struct {
    PolicyContext_mbedtls *policyContext;
    UA_PKIStore *pkiStore;
    UA_NodeId certificateTypeId;

    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;

    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    mbedtls_x509_crt remoteCertificate;
} ChannelContext_mbedtls;

UA_StatusCode
channelContext_mbedtls_loadKeyThenSign(ChannelContext_mbedtls *cc,
                                       const UA_ByteString *message,
                                       UA_ByteString *signature,
                                       UA_StatusCode (*sign)(ChannelContext_mbedtls *,
                                                             mbedtls_pk_context *,
                                                             const UA_ByteString *,
                                                             UA_ByteString *));

UA_StatusCode
channelContext_mbedtls_parseKeyThenSign(ChannelContext_mbedtls *cc,
                                        const UA_ByteString *message,
                                        UA_ByteString *signature,
										UA_ByteString* publicKeyStr,
                                        UA_StatusCode (*sign)(ChannelContext_mbedtls *,
                                                              mbedtls_pk_context *,
                                                              const UA_ByteString *,
                                                              UA_ByteString *));


UA_StatusCode
channelContext_mbedtls_loadKeyThenCrypt(ChannelContext_mbedtls *cc,
                                        UA_ByteString *data,
                                        UA_StatusCode (*crypt)(ChannelContext_mbedtls *,
                                                               mbedtls_pk_context *,
                                                               UA_ByteString *));

size_t
channelContext_mbedtls_loadKeyThenGetSize(const ChannelContext_mbedtls *cc,
                                          size_t (*getSize)(const ChannelContext_mbedtls *,
                                                            const mbedtls_pk_context *));

UA_StatusCode
channelContext_mbedtls_setLocalSymEncryptingKey(ChannelContext_mbedtls *cc,
                                                const UA_ByteString *key);

UA_StatusCode
channelContext_mbedtls_setLocalSymSigningKey(ChannelContext_mbedtls *cc,
                                             const UA_ByteString *key);

UA_StatusCode
channelContext_mbedtls_setLocalSymIv(ChannelContext_mbedtls *cc,
                                     const UA_ByteString *iv);

UA_StatusCode
channelContext_mbedtls_setRemoteSymEncryptingKey(ChannelContext_mbedtls *cc,
                                                 const UA_ByteString *key);

UA_StatusCode
channelContext_mbedtls_setRemoteSymSigningKey(ChannelContext_mbedtls *cc,
                                              const UA_ByteString *key);

UA_StatusCode
channelContext_mbedtls_setRemoteSymIv(ChannelContext_mbedtls *cc,
                                      const UA_ByteString *iv);

UA_StatusCode
channelContext_mbedtls_compareCertificate(const ChannelContext_mbedtls *cc,
                                          const UA_ByteString *certificate);

/* Assumes that the certificate has been verified externally */
UA_StatusCode
mbedtls_parseRemoteCertificate(ChannelContext_mbedtls *cc,
                               const size_t minAsymKeyLen,
                               const size_t maxAsymKeyLen,
                               const UA_ByteString *remoteCertificate);

void
channelContext_mbedtls_deleteContext(ChannelContext_mbedtls *cc);

UA_StatusCode
channelContext_mbedtls_newContext(const UA_SecurityPolicy *securityPolicy,
                                  UA_PKIStore *pkiStore,
                                  const UA_ByteString *remoteCertificate,
                                  const size_t minAsymKeyLen,
                                  const size_t maxAsymKeyLen,
                                  void **pp_contextData);

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
mbedtls_sign_sha256(ChannelContext_mbedtls *cc,
                    mbedtls_pk_context *privateKey,
                    const UA_ByteString *message,
                    UA_ByteString *signature);

UA_StatusCode
mbedtls_sign_sha1(ChannelContext_mbedtls *cc,
                  mbedtls_pk_context *privateKey,
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
mbedtls_decrypt_rsaOaep(ChannelContext_mbedtls *cc,
                        mbedtls_pk_context *privateKey,
                        UA_ByteString *data);

UA_StatusCode
mbedtls_compare_thumbprints(const UA_SecurityPolicy *securityPolicy,
                            UA_PKIStore *pkiStore,
                            const UA_ByteString *certificateThumbprint,
                            UA_StatusCode (*thumbprint)(const UA_SecurityPolicy *,
                                                        const UA_ByteString *,
                                                        UA_ByteString *));

int
UA_mbedTLS_parsePrivateKey(const UA_ByteString *key, mbedtls_pk_context *target, void *p_rng);

UA_StatusCode
UA_mbedTLS_loadPrivateKey(UA_PKIStore *pkiStore, void *p_rng,
                          const UA_NodeId certificateTypeId,
                          mbedtls_pk_context *pkContext);

/* Try to load certificate. Checks validity. If target is NULL, will only check for validity. */
UA_StatusCode
UA_mbedTLS_LoadLocalCertificate(const UA_SecurityPolicy *policy, UA_PKIStore *pkiStore, UA_ByteString *target);

UA_ByteString
UA_mbedTLS_CopyDataFormatAware(const UA_ByteString *data);

_UA_END_DECLS

#endif

#endif /* UA_SECURITYPOLICY_MBEDTLS_COMMON_H_ */
