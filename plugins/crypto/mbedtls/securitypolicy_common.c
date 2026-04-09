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

    if(UA_ByteString_allocBuffer(outPrivateKey, (size_t)len) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    outPrivateKey->length = (size_t)len;
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
    int ret = 0;
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
    size_t byteCount = (size_t)ret;
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
    if(!certificateData.data)
        return UA_STATUSCODE_BADOUTOFMEMORY;
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
    if(!crlData.data)
        return UA_STATUSCODE_BADINTERNALERROR;
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
    if(!data.data) {
        UA_ByteString_init(target);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
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
        UA_StatusCode res = UA_ByteString_allocBuffer(&result, data->length + 1);
        if(res != UA_STATUSCODE_GOOD)
            return result;
        memcpy(result.data, data->data, data->length);
        result.data[data->length] = '\0';
    } else {
        UA_ByteString_copy(data, &result);
    }

    return result;
}

#if MBEDTLS_VERSION_NUMBER >= 0x03000000

#include <mbedtls/ecdh.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/ecp.h>
#include <mbedtls/hkdf.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>

/* ECDH shared secret computation for Weierstrass curves (NIST/Brainpool) */
static UA_StatusCode
UA_mbedTLS_ECDH(mbedtls_ecp_group_id grpId,
                mbedtls_pk_context *keyPairLocal,
                mbedtls_ctr_drbg_context *drbgContext,
                const UA_ByteString *keyPublicRemote,
                UA_ByteString *sharedSecretOut) {
    mbedtls_ecp_group grp;
    mbedtls_ecp_point Qpeer;
    mbedtls_mpi z;

    mbedtls_ecp_group_init(&grp);
    mbedtls_ecp_point_init(&Qpeer);
    mbedtls_mpi_init(&z);

    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;
    int mbedErr;

    mbedErr = mbedtls_ecp_group_load(&grp, grpId);
    if(mbedErr != 0)
        goto errout;

    /* Import the remote public key (uncompressed point: x || y) */
    mbedErr = mbedtls_mpi_read_binary(&Qpeer.MBEDTLS_PRIVATE(X),
                                      keyPublicRemote->data,
                                      keyPublicRemote->length / 2);
    if(mbedErr != 0)
        goto errout;

    mbedErr = mbedtls_mpi_read_binary(&Qpeer.MBEDTLS_PRIVATE(Y),
                                      keyPublicRemote->data + keyPublicRemote->length / 2,
                                      keyPublicRemote->length / 2);
    if(mbedErr != 0)
        goto errout;

    mbedErr = mbedtls_mpi_lset(&Qpeer.MBEDTLS_PRIVATE(Z), 1);
    if(mbedErr != 0)
        goto errout;

    /* Get the local private key scalar d from the pk_context */
    mbedtls_ecp_keypair *kp = mbedtls_pk_ec(*keyPairLocal);
    if(kp == NULL)
        goto errout;

    /* Compute ECDH shared secret: z = d * Qpeer */
    mbedErr = mbedtls_ecdh_compute_shared(&grp, &z, &Qpeer,
                                          &kp->MBEDTLS_PRIVATE(d),
                                          mbedtls_ctr_drbg_random,
                                          drbgContext);
    if(mbedErr != 0)
        goto errout;

    /* Export shared secret to byte string */
    size_t olen = mbedtls_mpi_size(&z);
    ret = UA_ByteString_allocBuffer(sharedSecretOut, olen);
    if(ret != UA_STATUSCODE_GOOD)
        goto errout;

    mbedErr = mbedtls_mpi_write_binary(&z, sharedSecretOut->data,
                                       sharedSecretOut->length);
    if(mbedErr != 0) {
        UA_ByteString_clear(sharedSecretOut);
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    ret = UA_STATUSCODE_GOOD;

errout:
    mbedtls_ecp_group_free(&grp);
    mbedtls_ecp_point_free(&Qpeer);
    mbedtls_mpi_free(&z);
    return ret;
}

/* Salt generation for ECC key derivation (same algorithm as OpenSSL version) */
static UA_StatusCode
UA_mbedTLS_ECC_GenerateSalt(const size_t L,
                            const UA_ByteString *label,
                            const UA_ByteString *key1,
                            const UA_ByteString *key2,
                            UA_ByteString *salt) {
    if(salt == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t saltLen = sizeof(uint16_t) + label->length + key1->length + key2->length;
    if(UA_STATUSCODE_GOOD != UA_ByteString_allocBuffer(salt, saltLen))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    salt->data[0] = (UA_Byte)((L) & 0xFF);
    salt->data[1] = (UA_Byte)((L >> 8) & 0xFF);

    UA_Byte *saltPtr = &salt->data[2];
    memcpy(saltPtr, label->data, label->length);
    saltPtr += label->length;
    memcpy(saltPtr, key1->data, key1->length);
    saltPtr += key1->length;
    memcpy(saltPtr, key2->data, key2->length);

    return UA_STATUSCODE_GOOD;
}

/* HKDF key derivation */
static UA_StatusCode
UA_mbedTLS_HKDF(mbedtls_md_type_t mdType,
                const UA_ByteString *ikm,
                const UA_ByteString *salt,
                const UA_ByteString *info,
                UA_ByteString *out) {
    const mbedtls_md_info_t *mdInfo = mbedtls_md_info_from_type(mdType);
    if(mdInfo == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    int mbedErr = mbedtls_hkdf(mdInfo,
                               salt->data, salt->length,
                               ikm->data, ikm->length,
                               info->data, info->length,
                               out->data, out->length);
    if(mbedErr != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_STATUSCODE_GOOD;
}

/* ECC key generation for Weierstrass curves */
static UA_StatusCode
UA_mbedTLS_ECC_GenerateKey(mbedtls_ecp_group_id grpId,
                           mbedtls_pk_context *keyPairOut,
                           mbedtls_ctr_drbg_context *drbgContext,
                           UA_ByteString *keyPublicEncOut) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_pk_init(keyPairOut);

    int mbedErr = mbedtls_pk_setup(keyPairOut,
                                   mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    if(mbedErr != 0)
        goto errout;

    mbedtls_ecp_keypair *kp = mbedtls_pk_ec(*keyPairOut);
    if(kp == NULL)
        goto errout;

    mbedErr = mbedtls_ecp_gen_key(grpId, kp,
                                  mbedtls_ctr_drbg_random, drbgContext);
    if(mbedErr != 0)
        goto errout;

    /* Export public key as uncompressed point (x || y) */
    {
        size_t coordLen = keyPublicEncOut->length / 2;
        mbedErr = mbedtls_mpi_write_binary(&kp->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(X),
                                           keyPublicEncOut->data, coordLen);
        if(mbedErr != 0)
            goto errout;

        mbedErr = mbedtls_mpi_write_binary(&kp->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(Y),
                                           keyPublicEncOut->data + coordLen, coordLen);
        if(mbedErr != 0)
            goto errout;
    }

    return UA_STATUSCODE_GOOD;

errout:
    mbedtls_pk_free(keyPairOut);
    return ret;
}

/* Full ECC key derivation pipeline: ECDH → salt → HKDF */
UA_StatusCode
UA_mbedTLS_ECC_DeriveKeys(mbedtls_ecp_group_id curveID,
                          mbedtls_md_type_t hashType,
                          const UA_ApplicationType applicationType,
                          mbedtls_pk_context *localEphemeralKeyPair,
                          mbedtls_ctr_drbg_context *drbgContext,
                          const UA_ByteString *key1,
                          const UA_ByteString *key2,
                          UA_ByteString *out) {
    UA_ByteString sharedSecret = UA_BYTESTRING_NULL;
    UA_ByteString salt = UA_BYTESTRING_NULL;

    /* Get the local ephemeral public key for comparison */
    mbedtls_ecp_keypair *kp = mbedtls_pk_ec(*localEphemeralKeyPair);
    if(kp == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Export local public key as x || y for comparison */
    size_t coordLen = key1->length / 2;
    UA_ByteString localPubKey;
    UA_StatusCode ret = UA_ByteString_allocBuffer(&localPubKey, key1->length);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    int mbedErr = mbedtls_mpi_write_binary(&kp->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(X),
                                           localPubKey.data, coordLen);
    if(mbedErr != 0) {
        UA_ByteString_clear(&localPubKey);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    mbedErr = mbedtls_mpi_write_binary(&kp->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(Y),
                                       localPubKey.data + coordLen, coordLen);
    if(mbedErr != 0) {
        UA_ByteString_clear(&localPubKey);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Determine the label for salt generation, remote ephemeral public key for
     * ECDH, and info for HKDF */
    UA_ByteString *label = NULL;
    const UA_ByteString *remoteEphPubKey = NULL;
    static UA_String serverLabel = UA_STRING_STATIC("opcua-server");
    static UA_String clientLabel = UA_STRING_STATIC("opcua-client");
    static UA_String sessionLabel = UA_STRING_STATIC("opcua-secret");

    /* (Temporary) measure to signal salt generation for sessions */
    if(out->data[0] == 0x03 && out->data[1] == 0x03 && out->data[2] == 0x04) {
        label = &sessionLabel;
        if(applicationType == UA_APPLICATIONTYPE_SERVER) {
            remoteEphPubKey = key2;
        } else {
            remoteEphPubKey = key1;
        }
    }
    else if(memcmp(localPubKey.data, key1->data, key1->length) == 0) {
        /* Key 1 is local ephemeral public key => generating remote keys */
        remoteEphPubKey = key2;
        if(applicationType == UA_APPLICATIONTYPE_SERVER) {
            label = &clientLabel;
        } else {
            label = &serverLabel;
        }
    }
    else if(memcmp(localPubKey.data, key2->data, key2->length) == 0) {
        /* Key 2 is local ephemeral public key => generating local keys */
        remoteEphPubKey = key1;
        if(applicationType == UA_APPLICATIONTYPE_SERVER) {
            label = &serverLabel;
        } else {
            label = &clientLabel;
        }
    } else {
        UA_ByteString_clear(&localPubKey);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_ByteString_clear(&localPubKey);

    /* Use ECDH to calculate shared secret */
    ret = UA_mbedTLS_ECDH(curveID, localEphemeralKeyPair, drbgContext,
                          remoteEphPubKey, &sharedSecret);
    if(ret != UA_STATUSCODE_GOOD)
        goto errout;

    /* Calculate salt. The order of the (ephemeral public) keys (key1, key2) is
     * reversed because the caller sends [remote, local] for local key
     * computation and [local, remote] for remote key computation. According to
     * 6.8.1., the local salt computation appends the keys in order [local |
     * remote] and the remote salt computation [remote | local]. Therefore, no
     * additional logic is required, reversing the order is sufficient. */
    ret = UA_mbedTLS_ECC_GenerateSalt(out->length, label, key2, key1, &salt);
    if(ret != UA_STATUSCODE_GOOD)
        goto errout;

    /* Call HKDF to derive keys */
    /* Salt is given as the info argument (check 6.8.1., tables 66 and 67) */
    ret = UA_mbedTLS_HKDF(hashType, &sharedSecret, &salt, &salt, out);
    if(ret != UA_STATUSCODE_GOOD)
        goto errout;

errout:
    UA_ByteString_clear(&sharedSecret);
    UA_ByteString_clear(&salt);
    return ret;
}

/* ECDSA sign (internal, parameterized by hash algorithm) */
static UA_StatusCode
UA_mbedTLS_ECDSA_Sign(const UA_ByteString *message,
                      mbedtls_pk_context *privateKey,
                      mbedtls_ctr_drbg_context *drbgContext,
                      mbedtls_md_type_t mdType,
                      size_t hashLen,
                      UA_ByteString *outSignature) {
    unsigned char hash[64]; /* big enough for SHA-384 (48) and SHA-256 (32) */
    unsigned char derSig[MBEDTLS_ECDSA_MAX_LEN];
    size_t derSigLen = 0;

    if(mdType == MBEDTLS_MD_SHA256) {
        mbedtls_sha256(message->data, message->length, hash, 0);
    } else if(mdType == MBEDTLS_MD_SHA384) {
        mbedtls_sha512(message->data, message->length, hash, 1);
    } else {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    int mbedErr = mbedtls_pk_sign(privateKey, mdType, hash, hashLen,
                                  derSig, sizeof(derSig), &derSigLen,
                                  mbedtls_ctr_drbg_random, drbgContext);
    if(mbedErr != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Convert DER-encoded ECDSA signature to raw (r || s) format.
     * DER format: 0x30 <len> 0x02 <r_len> <r> 0x02 <s_len> <s> */
    const unsigned char *p = derSig;
    const unsigned char *end = derSig + derSigLen;
    size_t sizeEncCoordinate = outSignature->length / 2;

    /* Skip SEQUENCE tag and length */
    if(*p != 0x30)
        return UA_STATUSCODE_BADINTERNALERROR;
    p++;
    if(*p & 0x80) p += (*p & 0x7F) + 1; /* long form length */
    else p++;

    /* Read r */
    if(p >= end || *p != 0x02)
        return UA_STATUSCODE_BADINTERNALERROR;
    p++;
    size_t rLen = *p++; 
    /* Skip leading zero padding */
    while(rLen > sizeEncCoordinate && *p == 0x00) { p++; rLen--; }
    if(rLen > sizeEncCoordinate)
        return UA_STATUSCODE_BADINTERNALERROR;
    /* Right-align r in the output */
    memset(outSignature->data, 0, sizeEncCoordinate);
    memcpy(outSignature->data + (sizeEncCoordinate - rLen), p, rLen);
    p += rLen;

    /* Read s */
    if(p >= end || *p != 0x02)
        return UA_STATUSCODE_BADINTERNALERROR;
    p++;
    size_t sLen = *p++;
    /* Skip leading zero padding */
    while(sLen > sizeEncCoordinate && *p == 0x00) { p++; sLen--; }
    if(sLen > sizeEncCoordinate)
        return UA_STATUSCODE_BADINTERNALERROR;
    /* Right-align s in the output */
    memset(outSignature->data + sizeEncCoordinate, 0, sizeEncCoordinate);
    memcpy(outSignature->data + sizeEncCoordinate + (sizeEncCoordinate - sLen), p, sLen);

    return UA_STATUSCODE_GOOD;
}

/* ECDSA verify (internal, parameterized by hash algorithm) */
static UA_StatusCode
UA_mbedTLS_ECDSA_Verify(const UA_ByteString *message,
                        mbedtls_x509_crt *publicKeyCert,
                        mbedtls_md_type_t mdType,
                        size_t hashLen,
                        const UA_ByteString *signature) {
    unsigned char hash[64];
    size_t sizeEncCoordinate = signature->length / 2;

    if(mdType == MBEDTLS_MD_SHA256) {
        mbedtls_sha256(message->data, message->length, hash, 0);
    } else if(mdType == MBEDTLS_MD_SHA384) {
        mbedtls_sha512(message->data, message->length, hash, 1);
    } else {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Convert raw (r || s) signature to DER format for mbedtls_pk_verify.
     * DER format: 0x30 <len> 0x02 <r_len> <r> 0x02 <s_len> <s>
     * Each r/s component may need a leading 0x00 if the high bit is set. */
    const unsigned char *rRaw = signature->data;
    const unsigned char *sRaw = signature->data + sizeEncCoordinate;

    /* Skip leading zeros but keep at least 1 byte */
    size_t rOff = 0;
    while(rOff < sizeEncCoordinate - 1 && rRaw[rOff] == 0x00) rOff++;
    size_t sOff = 0;
    while(sOff < sizeEncCoordinate - 1 && sRaw[sOff] == 0x00) sOff++;

    size_t rLen = sizeEncCoordinate - rOff;
    size_t sLen = sizeEncCoordinate - sOff;

    /* Add leading zero byte if high bit is set */
    int rPad = (rRaw[rOff] & 0x80) ? 1 : 0;
    int sPad = (sRaw[sOff] & 0x80) ? 1 : 0;

    size_t rDerLen = rLen + (size_t)rPad;
    size_t sDerLen = sLen + (size_t)sPad;

    /* Total DER: SEQUENCE(2 + rDerLen + 2 + sDerLen) */
    size_t innerLen = 2 + rDerLen + 2 + sDerLen;
    unsigned char derSig[MBEDTLS_ECDSA_MAX_LEN];
    unsigned char *p = derSig;

    *p++ = 0x30;
    if(innerLen >= 128) {
        *p++ = 0x81;
        *p++ = (unsigned char)innerLen;
    } else {
        *p++ = (unsigned char)innerLen;
    }

    *p++ = 0x02;
    *p++ = (unsigned char)rDerLen;
    if(rPad) *p++ = 0x00;
    memcpy(p, rRaw + rOff, rLen); p += rLen;

    *p++ = 0x02;
    *p++ = (unsigned char)sDerLen;
    if(sPad) *p++ = 0x00;
    memcpy(p, sRaw + sOff, sLen); p += sLen;

    size_t derSigLen = (size_t)(p - derSig);

    int mbedErr = mbedtls_pk_verify(&publicKeyCert->pk, mdType,
                                    hash, hashLen, derSig, derSigLen);
    if(mbedErr != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_STATUSCODE_GOOD;
}

/* Per-curve key generation wrappers */
UA_StatusCode
UA_mbedTLS_ECC_NISTP256_GenerateKey(mbedtls_pk_context *keyPairOut,
                                    mbedtls_ctr_drbg_context *drbgContext,
                                    UA_ByteString *keyPublicEncOut) {
    return UA_mbedTLS_ECC_GenerateKey(MBEDTLS_ECP_DP_SECP256R1, keyPairOut,
                                     drbgContext, keyPublicEncOut);
}

UA_StatusCode
UA_mbedTLS_ECC_NISTP384_GenerateKey(mbedtls_pk_context *keyPairOut,
                                    mbedtls_ctr_drbg_context *drbgContext,
                                    UA_ByteString *keyPublicEncOut) {
    return UA_mbedTLS_ECC_GenerateKey(MBEDTLS_ECP_DP_SECP384R1, keyPairOut,
                                     drbgContext, keyPublicEncOut);
}

UA_StatusCode
UA_mbedTLS_ECC_BRAINPOOLP256R1_GenerateKey(mbedtls_pk_context *keyPairOut,
                                           mbedtls_ctr_drbg_context *drbgContext,
                                           UA_ByteString *keyPublicEncOut) {
    return UA_mbedTLS_ECC_GenerateKey(MBEDTLS_ECP_DP_BP256R1, keyPairOut,
                                     drbgContext, keyPublicEncOut);
}

UA_StatusCode
UA_mbedTLS_ECC_BRAINPOOLP384R1_GenerateKey(mbedtls_pk_context *keyPairOut,
                                           mbedtls_ctr_drbg_context *drbgContext,
                                           UA_ByteString *keyPublicEncOut) {
    return UA_mbedTLS_ECC_GenerateKey(MBEDTLS_ECP_DP_BP384R1, keyPairOut,
                                     drbgContext, keyPublicEncOut);
}

/* ECDSA SHA-256 wrappers */
UA_StatusCode
UA_mbedTLS_ECDSA_SHA256_Sign(const UA_ByteString *message,
                             mbedtls_pk_context *privateKey,
                             mbedtls_ctr_drbg_context *drbgContext,
                             UA_ByteString *outSignature) {
    return UA_mbedTLS_ECDSA_Sign(message, privateKey, drbgContext,
                                MBEDTLS_MD_SHA256, 32, outSignature);
}

UA_StatusCode
UA_mbedTLS_ECDSA_SHA256_Verify(const UA_ByteString *message,
                               mbedtls_x509_crt *publicKeyCert,
                               const UA_ByteString *signature) {
    return UA_mbedTLS_ECDSA_Verify(message, publicKeyCert,
                                  MBEDTLS_MD_SHA256, 32, signature);
}

/* ECDSA SHA-384 wrappers */
UA_StatusCode
UA_mbedTLS_ECDSA_SHA384_Sign(const UA_ByteString *message,
                             mbedtls_pk_context *privateKey,
                             mbedtls_ctr_drbg_context *drbgContext,
                             UA_ByteString *outSignature) {
    return UA_mbedTLS_ECDSA_Sign(message, privateKey, drbgContext,
                                MBEDTLS_MD_SHA384, 48, outSignature);
}

UA_StatusCode
UA_mbedTLS_ECDSA_SHA384_Verify(const UA_ByteString *message,
                               mbedtls_x509_crt *publicKeyCert,
                               const UA_ByteString *signature) {
    return UA_mbedTLS_ECDSA_Verify(message, publicKeyCert,
                                  MBEDTLS_MD_SHA384, 48, signature);
}

/* HMAC-SHA256 sign/verify */
UA_StatusCode
UA_mbedTLS_HMAC_SHA256_Verify(const UA_ByteString *message,
                              const UA_ByteString *key,
                              const UA_ByteString *signature) {
    const mbedtls_md_info_t *mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    unsigned char mac[32];

    int mbedErr = mbedtls_md_hmac(mdInfo, key->data, key->length,
                                  message->data, message->length, mac);
    if(mbedErr != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString macBs = {32, mac};
    if(!UA_ByteString_equal(signature, &macBs))
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_HMAC_SHA256_Sign(const UA_ByteString *message,
                            const UA_ByteString *key,
                            UA_ByteString *signature) {
    const mbedtls_md_info_t *mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    int mbedErr = mbedtls_md_hmac(mdInfo, key->data, key->length,
                                  message->data, message->length,
                                  signature->data);
    if(mbedErr != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    signature->length = 32;
    return UA_STATUSCODE_GOOD;
}

/* HMAC-SHA384 sign/verify */
UA_StatusCode
UA_mbedTLS_HMAC_SHA384_Verify(const UA_ByteString *message,
                              const UA_ByteString *key,
                              const UA_ByteString *signature) {
    const mbedtls_md_info_t *mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA384);
    unsigned char mac[48];

    int mbedErr = mbedtls_md_hmac(mdInfo, key->data, key->length,
                                  message->data, message->length, mac);
    if(mbedErr != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString macBs = {48, mac};
    if(!UA_ByteString_equal(signature, &macBs))
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_HMAC_SHA384_Sign(const UA_ByteString *message,
                            const UA_ByteString *key,
                            UA_ByteString *signature) {
    const mbedtls_md_info_t *mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA384);

    int mbedErr = mbedtls_md_hmac(mdInfo, key->data, key->length,
                                  message->data, message->length,
                                  signature->data);
    if(mbedErr != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    signature->length = 48;
    return UA_STATUSCODE_GOOD;
}

#endif /* MBEDTLS_VERSION_NUMBER >= 0x03000000 */

#endif
