/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Kai Huebl)
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/certificategroup.h>
#include <open62541/types.h>

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)

#include "securitypolicy_common.h"

#include <mbedtls/aes.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/md.h>
#include <mbedtls/sha1.h>
#include <mbedtls/version.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/asn1write.h>
#include <mbedtls/oid.h>
#include <mbedtls/platform.h>

#define CSR_BUFFER_SIZE 4096

void
swapBuffers(UA_ByteString *const bufA, UA_ByteString *const bufB) {
    UA_ByteString tmp = *bufA;
    *bufA = *bufB;
    *bufB = tmp;
}

UA_StatusCode
mbedtls_hmac(mbedtls_md_context_t *context, const UA_ByteString *key,
             const UA_ByteString *in, unsigned char *out) {

    if(mbedtls_md_hmac_starts(context, key->data, key->length) != 0)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    if(mbedtls_md_hmac_update(context, in->data, in->length) != 0)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    if(mbedtls_md_hmac_finish(context, out) != 0)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    return UA_STATUSCODE_GOOD;
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

    UA_StatusCode retval = mbedtls_hmac(context, secret, seed, A.data);

    if(retval != UA_STATUSCODE_GOOD){
        UA_ByteString_clear(&A_and_seed);
        UA_ByteString_clear(&ANext_and_seed);
        return retval;
    }

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

        retval = mbedtls_hmac(context, secret, &A_and_seed, outSegment.data);
        if(retval != UA_STATUSCODE_GOOD){
            UA_ByteString_clear(&A_and_seed);
            UA_ByteString_clear(&ANext_and_seed);
            return retval;
        }
        retval = mbedtls_hmac(context, secret, &A, ANext.data);
        if(retval != UA_STATUSCODE_GOOD){
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
mbedtls_sign_sha1(mbedtls_pk_context *localPrivateKey,
                  mbedtls_ctr_drbg_context *drbgContext,
                  const UA_ByteString *message,
                  UA_ByteString *signature) {
    unsigned char hash[UA_SHA1_LENGTH];
#if MBEDTLS_VERSION_NUMBER >= 0x02070000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    mbedtls_sha1_ret(message->data, message->length, hash);
#else
    mbedtls_sha1(message->data, message->length, hash);
#endif

    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(*localPrivateKey);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);

    size_t sigLen = 0;
    int mbedErr = mbedtls_pk_sign(localPrivateKey, MBEDTLS_MD_SHA1, hash,
                                  UA_SHA1_LENGTH, signature->data,
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
                                  signature->length,
#endif
                                  &sigLen,
                                  mbedtls_ctr_drbg_random, drbgContext);
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
mbedtls_decrypt_rsaOaep(mbedtls_pk_context *localPrivateKey,
                        mbedtls_ctr_drbg_context *drbgContext,
                        UA_ByteString *data, int hash_id) {
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(*localPrivateKey);
    if(!rsaContext)
        return UA_STATUSCODE_BADINTERNALERROR;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V21, hash_id);
    size_t keylen = rsaContext->len;
#else
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V21, (mbedtls_md_type_t)hash_id);
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
                                                     drbgContext, MBEDTLS_RSA_PRIVATE,
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

static size_t
mbedtls_getSequenceListDeep(const mbedtls_x509_sequence *sanlist) {
    size_t ret = 0;
    const mbedtls_x509_sequence *cur = sanlist;
    while(cur) {
        ret++;
        cur = cur->next;
    }

    return ret;
}

static UA_StatusCode
mbedtls_x509write_csrSetSubjectAltName(mbedtls_x509write_csr *ctx, const mbedtls_x509_sequence* sanlist) {
    int ret = 0;
    const mbedtls_x509_sequence* cur = sanlist;
    unsigned char *buf;
    unsigned char *pc;
    size_t len = 0;

    /* How many alt names to be written */
    size_t sandeep = mbedtls_getSequenceListDeep(sanlist);
    if(sandeep == 0)
        return UA_STATUSCODE_GOOD;

    size_t buflen = MBEDTLS_SAN_MAX_LEN * sandeep + sandeep;
    buf = (unsigned char *)mbedtls_calloc(1, buflen);
    if(!buf)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    memset(buf, 0, buflen);
    pc = buf + buflen;

    while(cur) {
        switch (cur->buf.tag & 0x0F) {
            case MBEDTLS_X509_SAN_DNS_NAME:
            case MBEDTLS_X509_SAN_RFC822_NAME:
            case MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER:
            case MBEDTLS_X509_SAN_IP_ADDRESS: {
                const int writtenBytes = mbedtls_asn1_write_raw_buffer(
                    &pc, buf, (const unsigned char *)cur->buf.p, cur->buf.len);
                MBEDTLS_ASN1_CHK_CLEANUP_ADD(len, writtenBytes);
                MBEDTLS_ASN1_CHK_CLEANUP_ADD(len, mbedtls_asn1_write_len(&pc, buf, cur->buf.len));
                MBEDTLS_ASN1_CHK_CLEANUP_ADD(len, mbedtls_asn1_write_tag(&pc, buf,
                                                                         MBEDTLS_ASN1_CONTEXT_SPECIFIC | cur->buf.tag));
                break;
            }
            default:
                /* Error out on an unsupported SAN */
                ret = MBEDTLS_ERR_X509_FEATURE_UNAVAILABLE;
                goto cleanup;
        }
        cur = cur->next;
    }

    MBEDTLS_ASN1_CHK_CLEANUP_ADD(len, mbedtls_asn1_write_len(&pc, buf, len));
    MBEDTLS_ASN1_CHK_CLEANUP_ADD(len, mbedtls_asn1_write_tag(&pc, buf, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

#if MBEDTLS_VERSION_NUMBER < 0x03000000
    ret = mbedtls_x509write_csr_set_extension(ctx, MBEDTLS_OID_SUBJECT_ALT_NAME,
                                              MBEDTLS_OID_SIZE(MBEDTLS_OID_SUBJECT_ALT_NAME), (const unsigned char*)(buf + buflen - len), len);
#else
    ret = mbedtls_x509write_csr_set_extension(ctx, MBEDTLS_OID_SUBJECT_ALT_NAME,
                                              MBEDTLS_OID_SIZE(MBEDTLS_OID_SUBJECT_ALT_NAME), 0, (const unsigned char*)(buf + buflen - len), len);
#endif

cleanup:
    mbedtls_free(buf);
    return (ret == 0) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
}

static UA_StatusCode
mbedtls_writePrivateKeyDer(mbedtls_pk_context *key, UA_ByteString *outPrivateKey) {
    unsigned char output_buf[16000];
    unsigned char *c = NULL;

    memset(output_buf, 0, 16000);
    const int len = mbedtls_pk_write_key_der(key, output_buf, 16000);
    if(len < 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    c = output_buf + sizeof(output_buf) - len;

    if(UA_ByteString_allocBuffer(outPrivateKey, len) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    outPrivateKey->length = len;
    memcpy(outPrivateKey->data, c, outPrivateKey->length);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
mbedtls_createSigningRequest(mbedtls_pk_context *localPrivateKey,
                             mbedtls_pk_context *csrLocalPrivateKey,
                             mbedtls_entropy_context *entropyContext,
                             mbedtls_ctr_drbg_context *drbgContext,
                             UA_SecurityPolicy *securityPolicy,
                             const UA_String *subjectName,
                             const UA_ByteString *nonce,
                             UA_ByteString *csr,
                             UA_ByteString *newPrivateKey) {
    /* Check parameter */
    if(!securityPolicy || !csr || !localPrivateKey || !csrLocalPrivateKey) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    size_t ret = 0;
    char *subj = NULL;
    const mbedtls_x509_sequence *san_list = NULL;

    mbedtls_pk_free(csrLocalPrivateKey);

    /* CSR has already been generated and private key only needs to be set
     * if a new one has been generated. */
    if(newPrivateKey && newPrivateKey->length > 0) {
        mbedtls_pk_init(csrLocalPrivateKey);
        mbedtls_pk_setup(csrLocalPrivateKey, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));

        /* Set the private key */
        if(UA_mbedTLS_LoadPrivateKey(newPrivateKey, csrLocalPrivateKey, entropyContext))
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

        return UA_STATUSCODE_GOOD;
    }

    /* CSR is already created. Nothing to do */
    if(csr && csr->length > 0)
        return UA_STATUSCODE_GOOD;

    /* Get X509 certificate */
    mbedtls_x509_crt x509Cert;
    mbedtls_x509_crt_init(&x509Cert);
    UA_ByteString certificateStr = UA_mbedTLS_CopyDataFormatAware(&securityPolicy->localCertificate);
    ret = mbedtls_x509_crt_parse(&x509Cert, certificateStr.data, certificateStr.length);
    UA_ByteString_clear(&certificateStr);
    if(ret)
        return UA_STATUSCODE_BADCERTIFICATEINVALID;

    mbedtls_x509write_csr request;
    mbedtls_x509write_csr_init(&request);
    /* Set message digest algorithms in CSR context */
    mbedtls_x509write_csr_set_md_alg(&request, MBEDTLS_MD_SHA256);

    /* Set key usage in CSR context */
    if(mbedtls_x509write_csr_set_key_usage(&request, MBEDTLS_X509_KU_DIGITAL_SIGNATURE |
                                                     MBEDTLS_X509_KU_DATA_ENCIPHERMENT |
                                                     MBEDTLS_X509_KU_NON_REPUDIATION |
                                                     MBEDTLS_X509_KU_KEY_ENCIPHERMENT) != 0) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* Add entropy */
    UA_Boolean hasEntropy = entropyContext && nonce && nonce->length > 0;
    if(hasEntropy) {
        if(mbedtls_entropy_update_manual(entropyContext,
                                         (const unsigned char*)(nonce->data),
                                         nonce->length) != 0) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
    }

    /* Get subject from argument or read it from certificate */
    if(subjectName && subjectName->length > 0) {
        /* subject from argument */
        subj = (char *)UA_malloc(subjectName->length + 1);
        if(!subj) {
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        memset(subj, 0x00, subjectName->length + 1);
        strncpy(subj, (char *)subjectName->data, subjectName->length);
        /* search for / in subject and replace it by comma */
        char *p = subj;
        for(size_t i = 0; i < subjectName->length; i++) {
            if(*p == '/' ) {
                *p = ',';
            }
            ++p;
        }
    } else {
        /* read subject from certificate */
        mbedtls_x509_name s = x509Cert.subject;
        subj = (char *)UA_malloc(UA_MAXSUBJECTLENGTH);
        if(!subj) {
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        if(mbedtls_x509_dn_gets(subj, UA_MAXSUBJECTLENGTH, &s) <= 0) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
    }

    /* Set the subject in CSR context */
    if(mbedtls_x509write_csr_set_subject_name(&request, subj) != 0) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* Get the subject alternate names from certificate and set them in CSR context*/
    san_list = &x509Cert.subject_alt_names;
    mbedtls_x509write_csrSetSubjectAltName(&request, san_list);

    /* Set private key in CSR context */
    if(newPrivateKey) {
        mbedtls_pk_init(csrLocalPrivateKey);
        mbedtls_pk_setup(csrLocalPrivateKey, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));

        size_t keySize = 0;
        UA_CertificateUtils_getKeySize(&securityPolicy->localCertificate, &keySize);
        mbedtls_rsa_gen_key(mbedtls_pk_rsa(*csrLocalPrivateKey), mbedtls_ctr_drbg_random,
                            drbgContext, (unsigned int)keySize, 65537);
        mbedtls_x509write_csr_set_key(&request, csrLocalPrivateKey);
        mbedtls_writePrivateKeyDer(csrLocalPrivateKey, newPrivateKey);
    } else {
        mbedtls_x509write_csr_set_key(&request, localPrivateKey);
    }

    /* The private key associated with the request will be used for signing the
     * created CSR. Enforce using RSASSA-PKCS1-v1_5 scheme. The hash_id
     * argument is ignored when padding is set to MBEDTLS_RSA_PKCS_V15, so just
     * set it to MBEDTLS_MD_NONE. */
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(
#if MBEDTLS_VERSION_NUMBER < 0x03000000
        *request.key);
#else
        *request.private_key);
#endif
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);

    unsigned char requestBuf[CSR_BUFFER_SIZE];
    memset(requestBuf, 0, sizeof(requestBuf));
    ret = mbedtls_x509write_csr_der(&request, requestBuf, sizeof(requestBuf),
                                    mbedtls_ctr_drbg_random, drbgContext);
    if(ret <= 0) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* number of CSR data bytes located at the end of the request buffer */
    size_t byteCount = ret;
    size_t offset = sizeof(requestBuf) - byteCount;

    /* copy return parameter into a ByteString */
    UA_ByteString_init(csr);
    UA_ByteString_allocBuffer(csr, byteCount);
    memcpy(csr->data, requestBuf + offset, byteCount);

cleanup:
    mbedtls_x509_crt_free(&x509Cert);
    mbedtls_x509write_csr_free(&request);
    if(subj)
        UA_free(subj);

    return retval;
}

int
UA_mbedTLS_LoadPrivateKey(const UA_ByteString *key, mbedtls_pk_context *target, void *p_rng) {
    UA_ByteString data = UA_mbedTLS_CopyDataFormatAware(key);
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    int mbedErr = mbedtls_pk_parse_key(target, data.data, data.length, NULL, 0);
#else
    int mbedErr = mbedtls_pk_parse_key(target, data.data, data.length, NULL, 0, mbedtls_entropy_func, p_rng);
#endif
    UA_ByteString_clear(&data);
    return mbedErr;
}

UA_StatusCode
UA_mbedTLS_LoadCertificate(const UA_ByteString *certificate, mbedtls_x509_crt *target) {
    const unsigned char *pData = certificate->data;

    if(certificate->length > 1 && pData[0] == 0x30 && pData[1] == 0x82) { // Magic number for DER encoded files
        return UA_mbedTLS_LoadDerCertificate(certificate, target);
    }
    return UA_mbedTLS_LoadPemCertificate(certificate, target);
}

UA_StatusCode
UA_mbedTLS_LoadDerCertificate(const UA_ByteString *certificate, mbedtls_x509_crt *target) {
    int mbedErr = mbedtls_x509_crt_parse(target, certificate->data, certificate->length);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_LoadPemCertificate(const UA_ByteString *certificate, mbedtls_x509_crt *target) {
    UA_ByteString certificateData = UA_mbedTLS_CopyDataFormatAware(certificate);
    int mbedErr = mbedtls_x509_crt_parse(target, certificateData.data, certificateData.length);
    UA_ByteString_clear(&certificateData);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_LoadCrl(const UA_ByteString *crl, mbedtls_x509_crl *target) {
    const unsigned char *pData = crl->data;

    if(crl->length > 1 && pData[0] == 0x30 && pData[1] == 0x82) { // Magic number for DER encoded files
        return UA_mbedTLS_LoadDerCrl(crl, target);
    }
    return UA_mbedTLS_LoadPemCrl(crl,target);

}

UA_StatusCode
UA_mbedTLS_LoadDerCrl(const UA_ByteString *crl, mbedtls_x509_crl *target) {
    int mbedErr = mbedtls_x509_crl_parse(target, crl->data, crl->length);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_LoadPemCrl(const UA_ByteString *crl, mbedtls_x509_crl *target) {
    UA_ByteString crlData = UA_mbedTLS_CopyDataFormatAware(crl);
    int mbedErr = mbedtls_x509_crl_parse(target, crlData.data, crlData.length);
    UA_ByteString_clear(&crlData);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_LoadLocalCertificate(const UA_ByteString *certData,
                                UA_ByteString *target) {
    UA_ByteString data = UA_mbedTLS_CopyDataFormatAware(certData);

    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);

    int mbedErr = mbedtls_x509_crt_parse(&cert, data.data, data.length);

    UA_StatusCode result = UA_STATUSCODE_BADINVALIDARGUMENT;

    if (!mbedErr) {
        UA_ByteString tmp;
        tmp.data = cert.raw.p;
        tmp.length = cert.raw.len;

        result = UA_ByteString_copy(&tmp, target);
    } else {
        UA_ByteString_init(target);
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

#endif
