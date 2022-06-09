#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/certificate_manager.h>
#include <open62541/types.h>

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS) || defined(UA_ENABLE_PUBSUB_ENCRYPTION)

#include "securitypolicy_mbedtls_common.h"

#include <mbedtls/aes.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/md.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/version.h>
#include <mbedtls/x509_crt.h>

void
swapBuffers(UA_ByteString *const bufA, UA_ByteString *const bufB) {
    UA_ByteString tmp = *bufA;
    *bufA = *bufB;
    *bufB = tmp;
}

void
mbedtls_hmac(mbedtls_md_context_t *context, const UA_ByteString *key,
             const UA_ByteString *in, unsigned char *out) {
    mbedtls_md_hmac_starts(context, key->data, key->length);
    mbedtls_md_hmac_update(context, in->data, in->length);
    mbedtls_md_hmac_finish(context, out);
}

UA_StatusCode
mbedtls_generateKey(mbedtls_md_context_t *context,
                    const UA_ByteString *secret, const UA_ByteString *seed,
                    UA_ByteString *out) {
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    size_t hashLen = (size_t)mbedtls_md_get_size(context->md_info);
#else
    size_t hashLen = (size_t)mbedtls_md_get_size(context->private_md_info);
#endif

    UA_ByteString A_and_seed;
    UA_ByteString_allocBuffer(&A_and_seed, hashLen + seed->length);
    memcpy(A_and_seed.data + hashLen, seed->data, seed->length);

    UA_ByteString ANext_and_seed;
    UA_ByteString_allocBuffer(&ANext_and_seed, hashLen + seed->length);
    memcpy(ANext_and_seed.data + hashLen, seed->data, seed->length);

    UA_ByteString A = {
        hashLen,
        A_and_seed.data
    };

    UA_ByteString ANext = {
        hashLen,
        ANext_and_seed.data
    };

    mbedtls_hmac(context, secret, seed, A.data);

    UA_StatusCode retval = 0;
    for(size_t offset = 0; offset < out->length; offset += hashLen) {
        UA_ByteString outSegment = {
            hashLen,
            out->data + offset
        };
        UA_Boolean bufferAllocated = UA_FALSE;
        // Not enough room in out buffer to write the hash.
        if(offset + hashLen > out->length) {
            outSegment.data = NULL;
            outSegment.length = 0;
            retval = UA_ByteString_allocBuffer(&outSegment, hashLen);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_ByteString_clear(&A_and_seed);
                UA_ByteString_clear(&ANext_and_seed);
                return retval;
            }
            bufferAllocated = UA_TRUE;
        }

        mbedtls_hmac(context, secret, &A_and_seed, outSegment.data);
        mbedtls_hmac(context, secret, &A, ANext.data);

        if(retval != UA_STATUSCODE_GOOD) {
            if(bufferAllocated)
                UA_ByteString_clear(&outSegment);
            UA_ByteString_clear(&A_and_seed);
            UA_ByteString_clear(&ANext_and_seed);
            return retval;
        }

        if(bufferAllocated) {
            memcpy(out->data + offset, outSegment.data, out->length - offset);
            UA_ByteString_clear(&outSegment);
        }

        swapBuffers(&ANext_and_seed, &A_and_seed);
        swapBuffers(&ANext, &A);
    }

    UA_ByteString_clear(&A_and_seed);
    UA_ByteString_clear(&ANext_and_seed);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
mbedtls_verifySig_sha1(mbedtls_x509_crt *certificate, const UA_ByteString *message,
                       const UA_ByteString *signature) {
    /* Compute the sha1 hash */
    unsigned char hash[UA_SHA1_LENGTH];
#if MBEDTLS_VERSION_NUMBER >= 0x02070000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    mbedtls_sha1_ret(message->data, message->length, hash);
#else
    mbedtls_sha1(message->data, message->length, hash);
#endif

    /* Set the RSA settings */
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(certificate->pk);
    if(!rsaContext)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);

    /* Verify */
    int mbedErr = mbedtls_pk_verify(&certificate->pk,
                                    MBEDTLS_MD_SHA1, hash, UA_SHA1_LENGTH,
                                    signature->data, signature->length);
    if(mbedErr)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
mbedtls_sign_sha256(ChannelContext_mbedtls *cc,
                    mbedtls_pk_context *privateKey,
                    const UA_ByteString *message,
                    UA_ByteString *signature) {
    unsigned char hash[UA_SHA256_LENGTH];
#if MBEDTLS_VERSION_NUMBER >= 0x02070000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    // TODO check return status
    mbedtls_sha256_ret(message->data, message->length, hash, 0);
#else
    mbedtls_sha256(message->data, message->length, hash, 0);
#endif

    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(*privateKey);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_SHA256);

    size_t sigLen = 0;

    /* For RSA keys, the default padding type is PKCS#1 v1.5 in mbedtls_pk_sign */
    /* Alternatively use more specific function mbedtls_rsa_rsassa_pkcs1_v15_sign() */
    int mbedErr = mbedtls_pk_sign(privateKey,
                                  MBEDTLS_MD_SHA256, hash,
                                  UA_SHA256_LENGTH, signature->data,
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
        signature->length,
#endif
                                  &sigLen, mbedtls_ctr_drbg_random,
                                  &cc->policyContext->drbgContext);

    if(mbedErr) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
mbedtls_sign_sha1(ChannelContext_mbedtls *cc,
                  mbedtls_pk_context *privateKey,
                  const UA_ByteString *message,
                  UA_ByteString *signature) {
    unsigned char hash[UA_SHA1_LENGTH];
#if MBEDTLS_VERSION_NUMBER >= 0x02070000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    mbedtls_sha1_ret(message->data, message->length, hash);
#else
    mbedtls_sha1(message->data, message->length, hash);
#endif

    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(*privateKey);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);

    size_t sigLen = 0;
    int mbedErr = mbedtls_pk_sign(privateKey, MBEDTLS_MD_SHA1, hash,
                                  UA_SHA1_LENGTH, signature->data,
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
        signature->length,
#endif
                                  &sigLen,
                                  mbedtls_ctr_drbg_random, &cc->policyContext->drbgContext);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
mbedtls_thumbprint_sha1(const UA_ByteString *certificate,
                        UA_ByteString *thumbprint) {
    if(UA_ByteString_equal(certificate, &UA_BYTESTRING_NULL))
        return UA_STATUSCODE_BADINTERNALERROR;

    if(thumbprint->length != UA_SHA1_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* The certificate thumbprint is always a 20 bit sha1 hash, see Part 4 of the Specification. */
#if MBEDTLS_VERSION_NUMBER >= 0x02070000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    mbedtls_sha1_ret(certificate->data, certificate->length, thumbprint->data);
#else
    mbedtls_sha1(certificate->data, certificate->length, thumbprint->data);
#endif
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
mbedtls_encrypt_rsaOaep(mbedtls_rsa_context *context,
                        mbedtls_ctr_drbg_context *drbgContext,
                        UA_ByteString *data, const size_t plainTextBlockSize) {
    if(data->length % plainTextBlockSize != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t max_blocks = data->length / plainTextBlockSize;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    size_t keylen = context->len;
#else
    size_t keylen = mbedtls_rsa_get_len(context);
#endif

    UA_ByteString encrypted;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&encrypted, max_blocks * keylen);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    size_t lenDataToEncrypt = data->length;
    size_t inOffset = 0;
    size_t offset = 0;
    const unsigned char *label = NULL;
    while(lenDataToEncrypt >= plainTextBlockSize) {
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
        int mbedErr = mbedtls_rsa_rsaes_oaep_encrypt(context, mbedtls_ctr_drbg_random,
                                                     drbgContext, MBEDTLS_RSA_PUBLIC,
                                                     label, 0, plainTextBlockSize,
                                                     data->data + inOffset, encrypted.data + offset);
#else
        int mbedErr = mbedtls_rsa_rsaes_oaep_encrypt(context, mbedtls_ctr_drbg_random,
                                                     drbgContext, label, 0, plainTextBlockSize,
                                                     data->data + inOffset, encrypted.data + offset);
#endif

        if(mbedErr) {
            UA_ByteString_clear(&encrypted);
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        inOffset += plainTextBlockSize;
        offset += keylen;
        lenDataToEncrypt -= plainTextBlockSize;
    }

    memcpy(data->data, encrypted.data, offset);
    UA_ByteString_clear(&encrypted);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
mbedtls_decrypt_rsaOaep(ChannelContext_mbedtls *cc,
                        mbedtls_pk_context *privateKey,
                        UA_ByteString *data) {
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(*privateKey);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA1);
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    size_t keylen = rsaContext->len;
#else
    size_t keylen = mbedtls_rsa_get_len(rsaContext);
#endif
    if(data->length % keylen != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t inOffset = 0;
    size_t outOffset = 0;
    size_t outLength = 0;
    unsigned char buf[512];

    while(inOffset < data->length) {
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
        int mbedErr = mbedtls_rsa_rsaes_oaep_decrypt(rsaContext, mbedtls_ctr_drbg_random,
                                                     &cc->policyContext->drbgContext, MBEDTLS_RSA_PRIVATE,
                                                     NULL, 0, &outLength,
                                                     data->data + inOffset,
                                                     buf, 512);
#else
        int mbedErr = mbedtls_rsa_rsaes_oaep_decrypt(rsaContext, mbedtls_ctr_drbg_random,
                                                     drbgContext,
                                                     NULL, 0, &outLength,
                                                     data->data + inOffset,
                                                     buf, 512);
#endif

        if(mbedErr)
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

        memcpy(data->data + outOffset, buf, outLength);
        inOffset += keylen;
        outOffset += outLength;
    }

    data->length = outOffset;
    return UA_STATUSCODE_GOOD;
}

int
UA_mbedTLS_parsePrivateKey(const UA_ByteString *key, mbedtls_pk_context *target, void *p_rng) {
    UA_ByteString data = UA_mbedTLS_CopyDataFormatAware(key);
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    int mbedErr = mbedtls_pk_parse_key(target, data.data, data.length, NULL, 0);
#else
    int mbedErr = mbedtls_pk_parse_key(target, data.data, data.length, NULL, 0, mbedtls_entropy_func, p_rng);
#endif
    UA_ByteString_clear(&data);
    return mbedErr;
}

/**
 * Loads the key specified by certificateTypeId from the supplied pkiStore.
 * Initializes the supplied pkContext.
 * The pkContext is only initialized if the function returns successfully.
 */
UA_StatusCode
UA_mbedTLS_loadPrivateKey(UA_PKIStore *pkiStore, void *p_rng,
                          const UA_NodeId certificateTypeId,
                          mbedtls_pk_context *pkContext) {
    mbedtls_pk_init(pkContext);
    UA_ByteString localPrivateKey;
    UA_ByteString_init(&localPrivateKey);
    UA_StatusCode retval = pkiStore->loadPrivateKey(pkiStore, certificateTypeId, &localPrivateKey);
    if(retval != UA_STATUSCODE_GOOD) {
        mbedtls_pk_free(pkContext);
        return retval;
    }

    int mbedErr = UA_mbedTLS_parsePrivateKey(&localPrivateKey, pkContext, &p_rng);
    if(mbedErr) {
        mbedtls_pk_free(pkContext);
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto cleanup;
    }

cleanup:
    UA_ByteString_clear(&localPrivateKey);
    return retval;
}

UA_StatusCode
UA_mbedTLS_LoadLocalCertificate(const UA_SecurityPolicy *policy, UA_PKIStore *pkiStore, UA_ByteString *target) {
    UA_ByteString localCertificate;
    UA_ByteString_init(&localCertificate);
    UA_StatusCode retval = pkiStore->loadCertificate(pkiStore, policy->certificateTypeId, &localCertificate);
    if(!UA_StatusCode_isGood(retval))
        return retval;

    UA_ByteString data = UA_mbedTLS_CopyDataFormatAware(&localCertificate);
    UA_ByteString_clear(&localCertificate);

    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);

    int mbedErr = mbedtls_x509_crt_parse(&cert, data.data, data.length);

    UA_StatusCode result = UA_STATUSCODE_BADINVALIDARGUMENT;

    if(!mbedErr) {
        UA_ByteString tmp;
        tmp.data = cert.raw.p;
        tmp.length = cert.raw.len;
        if(target != NULL) {
            result = UA_ByteString_copy(&tmp, target);
        }
    } else {
        if(target != NULL) {
            UA_ByteString_init(target);
        }
        result = UA_STATUSCODE_BADNOTFOUND;
    }

    UA_ByteString_clear(&data);
    mbedtls_x509_crt_free(&cert);
    return result;
}

// mbedTLS expects PEM data to be null terminated
// The data length parameter must include the null terminator
UA_ByteString
UA_mbedTLS_CopyDataFormatAware(const UA_ByteString *data) {
    UA_ByteString result;
    UA_ByteString_init(&result);

    if (!data->length)
        return result;

    if (data->length && data->data[0] == '-') {
        UA_ByteString_allocBuffer(&result, data->length + 1);
        memcpy(result.data, data->data, data->length);
        result.data[data->length] = '\0';
    } else {
        UA_ByteString_copy(data, &result);
    }

    return result;
}

UA_StatusCode
mbedtls_parseRemoteCertificate(ChannelContext_mbedtls *cc,
                               const size_t minAsymKeyLen,
                               const size_t maxAsymKeyLen,
                               const UA_ByteString *remoteCertificate) {
    if(remoteCertificate == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Parse the certificate */
    int mbedErr = mbedtls_x509_crt_parse(&cc->remoteCertificate, remoteCertificate->data,
                                         remoteCertificate->length);
    if(mbedErr)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Check the key length */
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    if(rsaContext->len < minAsymKeyLen ||
       rsaContext->len > maxAsymKeyLen)
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
#else
    size_t keylen = mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk));
    if(keylen < minAsymKeyLen ||
       keylen > maxAsymKeyLen)
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
#endif

    return UA_STATUSCODE_GOOD;
}

void
channelContext_mbedtls_deleteContext(ChannelContext_mbedtls *cc) {
    UA_NodeId_clear(&cc->certificateTypeId);
    UA_ByteString_clear(&cc->localSymSigningKey);
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    UA_ByteString_clear(&cc->localSymIv);

    UA_ByteString_clear(&cc->remoteSymSigningKey);
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    UA_ByteString_clear(&cc->remoteSymIv);

    mbedtls_x509_crt_free(&cc->remoteCertificate);

    UA_free(cc);
}

UA_StatusCode
channelContext_mbedtls_newContext(const UA_SecurityPolicy *securityPolicy,
                                  UA_PKIStore *pkiStore,
                                  const UA_ByteString *remoteCertificate,
                                  const size_t minAsymKeyLen,
                                  const size_t maxAsymKeyLen,
                                  void **pp_contextData) {
    if(securityPolicy == NULL || remoteCertificate == NULL || pp_contextData == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the channel context */
    *pp_contextData = UA_malloc(sizeof(ChannelContext_mbedtls));
    if(*pp_contextData == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    ChannelContext_mbedtls *cc = (ChannelContext_mbedtls *)*pp_contextData;

    /* Initialize the channel context */
    cc->policyContext = (PolicyContext_mbedtls *)securityPolicy->policyContext;
    cc->pkiStore = pkiStore;
    UA_StatusCode retval = UA_NodeId_copy(&securityPolicy->certificateTypeId, &cc->certificateTypeId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(*pp_contextData);
        return retval;
    }

    UA_ByteString_init(&cc->localSymSigningKey);
    UA_ByteString_init(&cc->localSymEncryptingKey);
    UA_ByteString_init(&cc->localSymIv);

    UA_ByteString_init(&cc->remoteSymSigningKey);
    UA_ByteString_init(&cc->remoteSymEncryptingKey);
    UA_ByteString_init(&cc->remoteSymIv);

    mbedtls_x509_crt_init(&cc->remoteCertificate);

    // TODO: this can be optimized so that we dont allocate memory before parsing the certificate
    retval = mbedtls_parseRemoteCertificate(cc, minAsymKeyLen, maxAsymKeyLen, remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        channelContext_mbedtls_deleteContext(cc);
        *pp_contextData = NULL;
    }
    return retval;
}

UA_StatusCode
channelContext_mbedtls_setRemoteSymIv(ChannelContext_mbedtls *cc, const UA_ByteString *iv) {
    if(iv == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->remoteSymIv);
    return UA_ByteString_copy(iv, &cc->remoteSymIv);
}

UA_StatusCode
channelContext_mbedtls_setRemoteSymSigningKey(ChannelContext_mbedtls *cc, const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

UA_StatusCode
channelContext_mbedtls_setRemoteSymEncryptingKey(ChannelContext_mbedtls *cc, const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

UA_StatusCode
channelContext_mbedtls_setLocalSymIv(ChannelContext_mbedtls *cc, const UA_ByteString *iv) {
    if(iv == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

UA_StatusCode
channelContext_mbedtls_setLocalSymSigningKey(ChannelContext_mbedtls *cc, const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}

UA_StatusCode
channelContext_mbedtls_setLocalSymEncryptingKey(ChannelContext_mbedtls *cc, const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

UA_StatusCode
channelContext_mbedtls_compareCertificate(const ChannelContext_mbedtls *cc, const UA_ByteString *certificate) {
    if(cc == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    int mbedErr = mbedtls_x509_crt_parse(&cert, certificate->data, certificate->length);
    if(mbedErr)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(cert.raw.len != cc->remoteCertificate.raw.len ||
       memcmp(cert.raw.p, cc->remoteCertificate.raw.p, cert.raw.len) != 0)
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    mbedtls_x509_crt_free(&cert);
    return retval;
}

UA_StatusCode
channelContext_mbedtls_loadKeyThenSign(ChannelContext_mbedtls *cc,
                                       const UA_ByteString *message,
                                       UA_ByteString *signature,
                                       UA_StatusCode (*sign)(ChannelContext_mbedtls *,
                                                             mbedtls_pk_context *,
                                                             const UA_ByteString *,
                                                             UA_ByteString *)) {
    if(cc == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_pk_context privateKey;
    UA_StatusCode retval =
        UA_mbedTLS_loadPrivateKey(cc->pkiStore, &cc->policyContext->drbgContext,
                                  cc->certificateTypeId,
                                  &privateKey);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    retval = sign(cc, &privateKey, message, signature);
    mbedtls_pk_free(&privateKey);
    return retval;
}

UA_StatusCode
channelContext_mbedtls_loadKeyThenCrypt(ChannelContext_mbedtls *cc,
                                        UA_ByteString *data,
                                        UA_StatusCode (*crypt)(ChannelContext_mbedtls *,
                                                               mbedtls_pk_context *,
                                                               UA_ByteString *)) {
    if(cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_pk_context privateKey;
    UA_StatusCode retval =
        UA_mbedTLS_loadPrivateKey(cc->pkiStore, &cc->policyContext->drbgContext,
                                  cc->certificateTypeId,
                                  &privateKey);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    retval = crypt(cc, &privateKey, data);
    mbedtls_pk_free(&privateKey);
    return retval;
}

size_t
channelContext_mbedtls_loadKeyThenGetSize(const ChannelContext_mbedtls *cc,
                                          size_t (*getSize)(const ChannelContext_mbedtls *,
                                                            const mbedtls_pk_context *)) {
    if(cc == NULL)
        return 0;

    mbedtls_pk_context privateKey;
    UA_StatusCode retval =
        UA_mbedTLS_loadPrivateKey(cc->pkiStore, &cc->policyContext->drbgContext,
                                  cc->certificateTypeId,
                                  &privateKey);
    if(retval != UA_STATUSCODE_GOOD) {
        return 0;
    }

    size_t size = getSize(cc, &privateKey);
    mbedtls_pk_free(&privateKey);
    return size;
}

UA_StatusCode
mbedtls_compare_thumbprints(const UA_SecurityPolicy *securityPolicy,
                            UA_PKIStore *pkiStore,
                            const UA_ByteString *certificateThumbprint,
                            UA_StatusCode (*thumbprint)(const UA_SecurityPolicy *,
                                                        const UA_ByteString *,
                                                        UA_ByteString *)) {
    if(securityPolicy == NULL || pkiStore == NULL || certificateThumbprint == NULL || thumbprint == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString localCertificate;
    UA_ByteString_init(&localCertificate);
    UA_StatusCode retval = pkiStore->loadCertificate(pkiStore, securityPolicy->certificateTypeId, &localCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    UA_ByteString localCertThumbprint;
    UA_ByteString_init(&localCertThumbprint);
    retval = UA_ByteString_allocBuffer(&localCertThumbprint, UA_SHA1_LENGTH);
    if(retval != UA_STATUSCODE_GOOD) {
        goto error;
    }
    retval = thumbprint(securityPolicy,
                        &localCertificate,
                        &localCertThumbprint);
    if(retval != UA_STATUSCODE_GOOD) {
        goto error;
    }
    if(!UA_ByteString_equal(certificateThumbprint, &localCertThumbprint)) {
        retval = UA_STATUSCODE_BADCERTIFICATEINVALID;
        goto error;
    }

    return UA_STATUSCODE_GOOD;
error:
    UA_ByteString_clear(&localCertificate);
    return retval;
}

#endif
