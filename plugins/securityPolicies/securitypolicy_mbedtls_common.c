#include <open62541/plugin/securitypolicy.h>

#ifdef UA_ENABLE_ENCRYPTION

#include <open62541/plugin/pki.h>
#include <open62541/plugin/securitypolicy_mbedtls_common.h>
#include <open62541/types.h>

#include <mbedtls/aes.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/entropy_poll.h>
#include <mbedtls/error.h>
#include <mbedtls/md.h>
#include <mbedtls/sha1.h>
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
    size_t hashLen = (size_t)mbedtls_md_get_size(context->md_info);

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
                UA_ByteString_deleteMembers(&A_and_seed);
                UA_ByteString_deleteMembers(&ANext_and_seed);
                return retval;
            }
            bufferAllocated = UA_TRUE;
        }

        mbedtls_hmac(context, secret, &A_and_seed, outSegment.data);
        mbedtls_hmac(context, secret, &A, ANext.data);

        if(retval != UA_STATUSCODE_GOOD) {
            if(bufferAllocated)
                UA_ByteString_deleteMembers(&outSegment);
            UA_ByteString_deleteMembers(&A_and_seed);
            UA_ByteString_deleteMembers(&ANext_and_seed);
            return retval;
        }

        if(bufferAllocated) {
            memcpy(out->data + offset, outSegment.data, out->length - offset);
            UA_ByteString_deleteMembers(&outSegment);
        }

        swapBuffers(&ANext_and_seed, &A_and_seed);
        swapBuffers(&ANext, &A);
    }

    UA_ByteString_deleteMembers(&A_and_seed);
    UA_ByteString_deleteMembers(&ANext_and_seed);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
mbedtls_verifySig_sha1(mbedtls_x509_crt *certificate, const UA_ByteString *message,
                       const UA_ByteString *signature) {
    /* Compute the sha1 hash */
    unsigned char hash[UA_SHA1_LENGTH];
#if MBEDTLS_VERSION_NUMBER >= 0x02070000
    mbedtls_sha1_ret(message->data, message->length, hash);
#else
    mbedtls_sha1(message->data, message->length, hash);
#endif

    /* Set the RSA settings */
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(certificate->pk);
    if(!rsaContext)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V15, 0);

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
#if MBEDTLS_VERSION_NUMBER >= 0x02070000
    mbedtls_sha1_ret(message->data, message->length, hash);
#else
    mbedtls_sha1(message->data, message->length, hash);
#endif

    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(*localPrivateKey);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V15, 0);

    size_t sigLen = 0;
    int mbedErr = mbedtls_pk_sign(localPrivateKey, MBEDTLS_MD_SHA1, hash,
                                  UA_SHA1_LENGTH, signature->data, &sigLen,
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
#if MBEDTLS_VERSION_NUMBER >= 0x02070000
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

    UA_ByteString encrypted;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&encrypted, max_blocks * context->len);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    size_t lenDataToEncrypt = data->length;
    size_t inOffset = 0;
    size_t offset = 0;
    const unsigned char *label = NULL;
    while(lenDataToEncrypt >= plainTextBlockSize) {
        int mbedErr = mbedtls_rsa_rsaes_oaep_encrypt(context, mbedtls_ctr_drbg_random,
                                                     drbgContext, MBEDTLS_RSA_PUBLIC,
                                                     label, 0, plainTextBlockSize,
                                                     data->data + inOffset, encrypted.data + offset);
        if(mbedErr) {
            UA_ByteString_deleteMembers(&encrypted);
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        inOffset += plainTextBlockSize;
        offset += context->len;
        lenDataToEncrypt -= plainTextBlockSize;
    }

    memcpy(data->data, encrypted.data, offset);
    UA_ByteString_deleteMembers(&encrypted);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
mbedtls_decrypt_rsaOaep(mbedtls_pk_context *localPrivateKey,
                        mbedtls_ctr_drbg_context *drbgContext,
                        UA_ByteString *data) {
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(*localPrivateKey);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA1);

    if(data->length % rsaContext->len != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t inOffset = 0;
    size_t outOffset = 0;
    size_t outLength = 0;
    unsigned char buf[512];

    while(inOffset < data->length) {
        int mbedErr = mbedtls_rsa_rsaes_oaep_decrypt(rsaContext, mbedtls_ctr_drbg_random,
                                                     drbgContext, MBEDTLS_RSA_PRIVATE,
                                                     NULL, 0, &outLength,
                                                     data->data + inOffset,
                                                     buf, 512);
        if(mbedErr)
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

        memcpy(data->data + outOffset, buf, outLength);
        inOffset += rsaContext->len;
        outOffset += outLength;
    }

    data->length = outOffset;
    return UA_STATUSCODE_GOOD;
}

#endif
