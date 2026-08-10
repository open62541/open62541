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

#include <mbedtls/error.h>
#include <mbedtls/md.h>
#include <mbedtls/version.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/asn1write.h>
#include <mbedtls/oid.h>
#include <mbedtls/platform.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/psa_util.h>

#define CSR_BUFFER_SIZE 4096

static void
clearSensitiveByteString(UA_ByteString *value) {
    if(value && value->data)
        mbedtls_platform_zeroize(value->data, value->length);
    UA_ByteString_clear(value);
}

static UA_StatusCode
psaStatusToStatusCode(psa_status_t status) {
    switch(status) {
    case PSA_SUCCESS:
        return UA_STATUSCODE_GOOD;
    case PSA_ERROR_INSUFFICIENT_MEMORY:
    case PSA_ERROR_INSUFFICIENT_STORAGE:
        return UA_STATUSCODE_BADOUTOFMEMORY;
    case PSA_ERROR_NOT_SUPPORTED:
        return UA_STATUSCODE_BADNOTSUPPORTED;
    case PSA_ERROR_INVALID_ARGUMENT:
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    case PSA_ERROR_BAD_STATE:
    case PSA_ERROR_CORRUPTION_DETECTED:
    case PSA_ERROR_GENERIC_ERROR:
        return UA_STATUSCODE_BADINTERNALERROR;
    default:
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }
}

UA_StatusCode
UA_mbedTLS_PSA_Init(void) {
    return psaStatusToStatusCode(psa_crypto_init());
}

void
UA_mbedTLS_PsaKey_init(UA_mbedTLS_PsaKey *key) {
    if(!key)
        return;
    key->id = MBEDTLS_SVC_KEY_ID_INIT;
    key->owned = false;
}

void
UA_mbedTLS_PsaKey_clear(UA_mbedTLS_PsaKey *key) {
    if(!key)
        return;
    if(key->owned && !mbedtls_svc_key_id_is_null(key->id))
        (void)psa_destroy_key(key->id);
    UA_mbedTLS_PsaKey_init(key);
}

UA_StatusCode
UA_mbedTLS_PsaKey_import(UA_mbedTLS_PsaKey *target,
                         psa_key_type_t type, psa_key_usage_t usage,
                         psa_algorithm_t algorithm,
                         const UA_ByteString *material) {
    if(!target || !material)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_StatusCode res = UA_mbedTLS_PSA_Init();
    if(res != UA_STATUSCODE_GOOD)
        return res;

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attributes, type);
    psa_set_key_bits(&attributes, material->length * 8);
    psa_set_key_usage_flags(&attributes, usage);
    psa_set_key_algorithm(&attributes, algorithm);

    mbedtls_svc_key_id_t id = MBEDTLS_SVC_KEY_ID_INIT;
    psa_status_t status = psa_import_key(&attributes, material->data,
                                         material->length, &id);
    psa_reset_key_attributes(&attributes);
    if(status != PSA_SUCCESS)
        return psaStatusToStatusCode(status);

    UA_mbedTLS_PsaKey_clear(target);
    target->id = id;
    target->owned = true;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_PsaKey_importPk(UA_mbedTLS_PsaKey *target,
                           const mbedtls_pk_context *source,
                           psa_key_usage_t usage,
                           psa_algorithm_t algorithm) {
    if(!target || !source)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_StatusCode res = UA_mbedTLS_PSA_Init();
    if(res != UA_STATUSCODE_GOOD)
        return res;

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    if(mbedtls_pk_get_psa_attributes(source, usage, &attributes) != 0)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    psa_set_key_algorithm(&attributes, algorithm);

    mbedtls_svc_key_id_t id = MBEDTLS_SVC_KEY_ID_INIT;
    int err = mbedtls_pk_import_into_psa(source, &attributes, &id);
    psa_reset_key_attributes(&attributes);
    if(err != 0)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    UA_mbedTLS_PsaKey_clear(target);
    target->id = id;
    target->owned = true;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_PsaHashCompute(psa_algorithm_t algorithm,
                          const UA_ByteString *input,
                          UA_ByteString *output) {
    if(!input || !output)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    size_t outputLength = 0;
    psa_status_t status = psa_hash_compute(algorithm, input->data, input->length,
                                           output->data, output->length,
                                           &outputLength);
    if(status != PSA_SUCCESS)
        return psaStatusToStatusCode(status);
    if(outputLength != output->length)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_PsaMacCompute(mbedtls_svc_key_id_t key,
                         psa_algorithm_t algorithm,
                         const UA_ByteString *input,
                         UA_ByteString *output) {
    if(!input || !output)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    size_t outputLength = 0;
    psa_status_t status = psa_mac_compute(key, algorithm, input->data,
                                          input->length, output->data,
                                          output->length, &outputLength);
    if(status != PSA_SUCCESS)
        return psaStatusToStatusCode(status);
    if(outputLength != output->length)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_PsaMacVerify(mbedtls_svc_key_id_t key,
                        psa_algorithm_t algorithm,
                        const UA_ByteString *input,
                        const UA_ByteString *mac) {
    if(!input || !mac)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return psaStatusToStatusCode(psa_mac_verify(
        key, algorithm, input->data, input->length, mac->data, mac->length));
}

UA_StatusCode
UA_mbedTLS_PsaRandom(UA_ByteString *output) {
    if(!output)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_StatusCode res = UA_mbedTLS_PSA_Init();
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return psaStatusToStatusCode(
        psa_generate_random(output->data, output->length));
}

UA_StatusCode
UA_mbedTLS_PsaPHash(psa_algorithm_t hashAlgorithm,
                    const UA_ByteString *secret,
                    const UA_ByteString *seed,
                    UA_ByteString *output) {
    if(!secret || !seed || !output || !PSA_ALG_IS_HASH(hashAlgorithm))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    size_t hashLength = PSA_HASH_LENGTH(hashAlgorithm);
    if(hashLength == 0)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    UA_ByteString a = UA_BYTESTRING_NULL;
    UA_ByteString nextA = UA_BYTESTRING_NULL;
    UA_ByteString macInput = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_ByteString_allocBuffer(&a, hashLength);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = UA_ByteString_allocBuffer(&nextA, hashLength);
    if(res != UA_STATUSCODE_GOOD) {
        clearSensitiveByteString(&a);
        return res;
    }
    res = UA_ByteString_allocBuffer(&macInput, hashLength + seed->length);
    if(res != UA_STATUSCODE_GOOD) {
        clearSensitiveByteString(&nextA);
        clearSensitiveByteString(&a);
        return res;
    }

    psa_algorithm_t macAlgorithm = PSA_ALG_HMAC(hashAlgorithm);
    UA_mbedTLS_PsaKey hmacKey;
    UA_mbedTLS_PsaKey_init(&hmacKey);
    res = UA_mbedTLS_PsaKey_import(&hmacKey, PSA_KEY_TYPE_HMAC,
                                   PSA_KEY_USAGE_SIGN_MESSAGE,
                                   macAlgorithm, secret);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_mbedTLS_PsaMacCompute(hmacKey.id, macAlgorithm, seed, &a);
    for(size_t offset = 0; offset < output->length; offset += hashLength) {
        if(res != UA_STATUSCODE_GOOD)
            break;
        memcpy(macInput.data, a.data, hashLength);
        memcpy(macInput.data + hashLength, seed->data, seed->length);
        UA_ByteString block = {hashLength, output->data + offset};
        UA_ByteString temporary = UA_BYTESTRING_NULL;
        if(offset + hashLength > output->length) {
            res = UA_ByteString_allocBuffer(&temporary, hashLength);
            if(res != UA_STATUSCODE_GOOD)
                break;
            block = temporary;
        }
        res = UA_mbedTLS_PsaMacCompute(hmacKey.id, macAlgorithm, &macInput, &block);
        if(temporary.data) {
            if(res == UA_STATUSCODE_GOOD)
                memcpy(output->data + offset, temporary.data, output->length - offset);
            clearSensitiveByteString(&temporary);
        }
        if(res != UA_STATUSCODE_GOOD)
            break;
        res = UA_mbedTLS_PsaMacCompute(hmacKey.id, macAlgorithm, &a, &nextA);
        swapBuffers(&a, &nextA);
    }

    UA_mbedTLS_PsaKey_clear(&hmacKey);
    clearSensitiveByteString(&macInput);
    clearSensitiveByteString(&nextA);
    clearSensitiveByteString(&a);
    return res;
}

UA_StatusCode
UA_mbedTLS_PsaCipher(mbedtls_svc_key_id_t key,
                     psa_algorithm_t algorithm, UA_Boolean encrypt,
                     const UA_ByteString *iv, UA_ByteString *data) {
    if(!iv || !data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_ByteString output = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_ByteString_allocBuffer(&output, data->length);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
    psa_status_t status = encrypt ?
        psa_cipher_encrypt_setup(&operation, key, algorithm) :
        psa_cipher_decrypt_setup(&operation, key, algorithm);
    if(status == PSA_SUCCESS)
        status = psa_cipher_set_iv(&operation, iv->data, iv->length);

    size_t updateLength = 0;
    if(status == PSA_SUCCESS)
        status = psa_cipher_update(&operation, data->data, data->length,
                                   output.data, output.length, &updateLength);
    size_t finishLength = 0;
    if(status == PSA_SUCCESS)
        status = psa_cipher_finish(&operation, output.data + updateLength,
                                   output.length - updateLength, &finishLength);
    if(status != PSA_SUCCESS || updateLength + finishLength != data->length) {
        (void)psa_cipher_abort(&operation);
        UA_ByteString_clear(&output);
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    memcpy(data->data, output.data, data->length);
    clearSensitiveByteString(&output);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_PsaAsymmetricSign(const mbedtls_pk_context *key,
                             psa_algorithm_t signatureAlgorithm,
                             psa_algorithm_t hashAlgorithm,
                             const UA_ByteString *message,
                             UA_ByteString *signature) {
    UA_Byte hashBuffer[PSA_HASH_MAX_SIZE];
    UA_ByteString hash = {PSA_HASH_LENGTH(hashAlgorithm), hashBuffer};
    UA_StatusCode res = UA_mbedTLS_PsaHashCompute(hashAlgorithm, message, &hash);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_mbedTLS_PsaKey imported;
    UA_mbedTLS_PsaKey_init(&imported);
    res = UA_mbedTLS_PsaKey_importPk(&imported, key, PSA_KEY_USAGE_SIGN_HASH,
                                     signatureAlgorithm);
    size_t signatureLength = 0;
    if(res == UA_STATUSCODE_GOOD &&
       psa_sign_hash(imported.id, signatureAlgorithm, hash.data, hash.length,
                     signature->data, signature->length,
                     &signatureLength) != PSA_SUCCESS)
        res = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    UA_mbedTLS_PsaKey_clear(&imported);
    if(res == UA_STATUSCODE_GOOD)
        signature->length = signatureLength;
    return res;
}

UA_StatusCode
UA_mbedTLS_PsaAsymmetricVerify(const mbedtls_pk_context *key,
                               psa_algorithm_t signatureAlgorithm,
                               psa_algorithm_t hashAlgorithm,
                               const UA_ByteString *message,
                               const UA_ByteString *signature) {
    UA_Byte hashBuffer[PSA_HASH_MAX_SIZE];
    UA_ByteString hash = {PSA_HASH_LENGTH(hashAlgorithm), hashBuffer};
    UA_StatusCode res = UA_mbedTLS_PsaHashCompute(hashAlgorithm, message, &hash);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_mbedTLS_PsaKey imported;
    UA_mbedTLS_PsaKey_init(&imported);
    res = UA_mbedTLS_PsaKey_importPk(&imported, key, PSA_KEY_USAGE_VERIFY_HASH,
                                     signatureAlgorithm);
    if(res == UA_STATUSCODE_GOOD &&
       psa_verify_hash(imported.id, signatureAlgorithm, hash.data, hash.length,
                       signature->data, signature->length) != PSA_SUCCESS)
        res = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    UA_mbedTLS_PsaKey_clear(&imported);
    return res;
}

UA_StatusCode
UA_mbedTLS_PsaAsymmetricEncrypt(const mbedtls_pk_context *key,
                                psa_algorithm_t algorithm,
                                size_t plainTextBlockSize,
                                UA_ByteString *data) {
    if(!key || !data || plainTextBlockSize == 0 ||
       data->length % plainTextBlockSize != 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    size_t keyLength = (mbedtls_pk_get_bitlen(key) + 7) / 8;
    size_t blockCount = data->length / plainTextBlockSize;
    UA_ByteString output = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_ByteString_allocBuffer(&output, blockCount * keyLength);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_mbedTLS_PsaKey imported;
    UA_mbedTLS_PsaKey_init(&imported);
    res = UA_mbedTLS_PsaKey_importPk(&imported, key, PSA_KEY_USAGE_ENCRYPT, algorithm);
    size_t inputOffset = 0;
    size_t outputOffset = 0;
    while(res == UA_STATUSCODE_GOOD && inputOffset < data->length) {
        size_t outputLength = 0;
        if(psa_asymmetric_encrypt(imported.id, algorithm,
                                  data->data + inputOffset, plainTextBlockSize,
                                  NULL, 0, output.data + outputOffset,
                                  output.length - outputOffset,
                                  &outputLength) != PSA_SUCCESS ||
           outputLength != keyLength) {
            res = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
            break;
        }
        inputOffset += plainTextBlockSize;
        outputOffset += outputLength;
    }
    UA_mbedTLS_PsaKey_clear(&imported);
    if(res == UA_STATUSCODE_GOOD) {
        memcpy(data->data, output.data, outputOffset);
        data->length = outputOffset;
    }
    UA_ByteString_clear(&output);
    return res;
}

UA_StatusCode
UA_mbedTLS_PsaAsymmetricDecrypt(const mbedtls_pk_context *key,
                                psa_algorithm_t algorithm,
                                UA_ByteString *data) {
    if(!key || !data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    size_t keyLength = (mbedtls_pk_get_bitlen(key) + 7) / 8;
    if(keyLength == 0 || data->length % keyLength != 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_ByteString output = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_ByteString_allocBuffer(&output, data->length);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_mbedTLS_PsaKey imported;
    UA_mbedTLS_PsaKey_init(&imported);
    res = UA_mbedTLS_PsaKey_importPk(&imported, key, PSA_KEY_USAGE_DECRYPT, algorithm);
    size_t inputOffset = 0;
    size_t outputOffset = 0;
    while(res == UA_STATUSCODE_GOOD && inputOffset < data->length) {
        size_t outputLength = 0;
        if(psa_asymmetric_decrypt(imported.id, algorithm,
                                  data->data + inputOffset, keyLength,
                                  NULL, 0, output.data + outputOffset,
                                  output.length - outputOffset,
                                  &outputLength) != PSA_SUCCESS) {
            res = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
            break;
        }
        inputOffset += keyLength;
        outputOffset += outputLength;
    }
    UA_mbedTLS_PsaKey_clear(&imported);
    if(res == UA_STATUSCODE_GOOD) {
        memcpy(data->data, output.data, outputOffset);
        data->length = outputOffset;
    }
    clearSensitiveByteString(&output);
    return res;
}

UA_StatusCode
UA_mbedTLS_PsaEccGenerate(psa_ecc_family_t family, size_t bits,
                          UA_mbedTLS_PsaKey *keyPair,
                          UA_ByteString *publicKey) {
    if(!keyPair || !publicKey || publicKey->length != 2 * ((bits + 7) / 8))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_StatusCode res = UA_mbedTLS_PSA_Init();
    if(res != UA_STATUSCODE_GOOD)
        return res;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(family));
    psa_set_key_bits(&attributes, bits);
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECDH);
    mbedtls_svc_key_id_t id = MBEDTLS_SVC_KEY_ID_INIT;
    psa_status_t status = psa_generate_key(&attributes, &id);
    psa_reset_key_attributes(&attributes);
    if(status != PSA_SUCCESS)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    UA_Byte encoded[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
    size_t encodedLength = 0;
    status = psa_export_public_key(id, encoded, sizeof(encoded), &encodedLength);
    if(status != PSA_SUCCESS || encodedLength != publicKey->length + 1 ||
       encoded[0] != 0x04) {
        (void)psa_destroy_key(id);
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }
    memcpy(publicKey->data, encoded + 1, publicKey->length);
    UA_mbedTLS_PsaKey_clear(keyPair);
    keyPair->id = id;
    keyPair->owned = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
psaEccGenerateSalt(size_t length, const UA_ByteString *label,
                   const UA_ByteString *key1, const UA_ByteString *key2,
                   UA_ByteString *salt) {
    UA_StatusCode res = UA_ByteString_allocBuffer(
        salt, 2 + label->length + key1->length + key2->length);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    salt->data[0] = (UA_Byte)(length & 0xff);
    salt->data[1] = (UA_Byte)((length >> 8) & 0xff);
    UA_Byte *pos = salt->data + 2;
    memcpy(pos, label->data, label->length);
    pos += label->length;
    memcpy(pos, key1->data, key1->length);
    pos += key1->length;
    memcpy(pos, key2->data, key2->length);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_PsaEccDerive(psa_algorithm_t hashAlgorithm,
                        const UA_ApplicationType applicationType,
                        const UA_mbedTLS_PsaKey *localEphemeralKeyPair,
                        const UA_ByteString *key1,
                        const UA_ByteString *key2,
                        UA_ByteString *output) {
    if(!localEphemeralKeyPair || !key1 || !key2 || !output)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_Byte encoded[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
    size_t encodedLength = 0;
    if(psa_export_public_key(localEphemeralKeyPair->id, encoded, sizeof(encoded),
                             &encodedLength) != PSA_SUCCESS ||
       encodedLength != key1->length + 1 || encoded[0] != 0x04)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    UA_ByteString localPublic = {encodedLength - 1, encoded + 1};

    const UA_ByteString *remotePublic = NULL;
    const UA_ByteString *label = NULL;
    static const UA_ByteString serverLabel = UA_STRING_STATIC("opcua-server");
    static const UA_ByteString clientLabel = UA_STRING_STATIC("opcua-client");
    static const UA_ByteString sessionLabel = UA_STRING_STATIC("opcua-secret");
    if(output->length >= 3 && output->data[0] == 0x03 &&
       output->data[1] == 0x03 && output->data[2] == 0x04) {
        label = &sessionLabel;
        remotePublic = (applicationType == UA_APPLICATIONTYPE_SERVER) ? key2 : key1;
    } else if(UA_ByteString_equal(&localPublic, key1)) {
        remotePublic = key2;
        label = (applicationType == UA_APPLICATIONTYPE_SERVER) ?
            &clientLabel : &serverLabel;
    } else if(UA_ByteString_equal(&localPublic, key2)) {
        remotePublic = key1;
        label = (applicationType == UA_APPLICATIONTYPE_SERVER) ?
            &serverLabel : &clientLabel;
    } else {
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    UA_Byte remoteEncoded[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
    if(remotePublic->length + 1 > sizeof(remoteEncoded))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    remoteEncoded[0] = 0x04;
    memcpy(remoteEncoded + 1, remotePublic->data, remotePublic->length);
    UA_ByteString shared = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_ByteString_allocBuffer(&shared, remotePublic->length / 2);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    size_t sharedLength = 0;
    if(psa_raw_key_agreement(PSA_ALG_ECDH, localEphemeralKeyPair->id,
                             remoteEncoded, remotePublic->length + 1,
                             shared.data, shared.length,
                             &sharedLength) != PSA_SUCCESS) {
        clearSensitiveByteString(&shared);
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }
    shared.length = sharedLength;

    UA_ByteString salt = UA_BYTESTRING_NULL;
    res = psaEccGenerateSalt(output->length, label, key2, key1, &salt);
    if(res != UA_STATUSCODE_GOOD) {
        clearSensitiveByteString(&shared);
        return res;
    }
    UA_mbedTLS_PsaKey secret;
    UA_mbedTLS_PsaKey_init(&secret);
    res = UA_mbedTLS_PsaKey_import(&secret, PSA_KEY_TYPE_DERIVE,
                                   PSA_KEY_USAGE_DERIVE,
                                   PSA_ALG_HKDF(hashAlgorithm), &shared);
    if(res == UA_STATUSCODE_GOOD) {
        psa_key_derivation_operation_t operation = PSA_KEY_DERIVATION_OPERATION_INIT;
        psa_status_t status = psa_key_derivation_setup(&operation,
                                                       PSA_ALG_HKDF(hashAlgorithm));
        if(status == PSA_SUCCESS)
            status = psa_key_derivation_input_bytes(&operation,
                PSA_KEY_DERIVATION_INPUT_SALT, salt.data, salt.length);
        if(status == PSA_SUCCESS)
            status = psa_key_derivation_input_key(&operation,
                PSA_KEY_DERIVATION_INPUT_SECRET, secret.id);
        if(status == PSA_SUCCESS)
            status = psa_key_derivation_input_bytes(&operation,
                PSA_KEY_DERIVATION_INPUT_INFO, salt.data, salt.length);
        if(status == PSA_SUCCESS)
            status = psa_key_derivation_output_bytes(&operation,
                                                      output->data, output->length);
        (void)psa_key_derivation_abort(&operation);
        if(status != PSA_SUCCESS)
            res = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }
    UA_mbedTLS_PsaKey_clear(&secret);
    clearSensitiveByteString(&salt);
    clearSensitiveByteString(&shared);
    return res;
}

void
UA_mbedTLS_ChannelContext_initPsa(mbedtls_ChannelContext *context) {
    UA_mbedTLS_PsaKey_init(&context->localSymSigningKeyPsa);
    UA_mbedTLS_PsaKey_init(&context->localSymEncryptingKeyPsa);
    UA_mbedTLS_PsaKey_init(&context->remoteSymSigningKeyPsa);
    UA_mbedTLS_PsaKey_init(&context->remoteSymEncryptingKeyPsa);
}

void
UA_mbedTLS_ChannelContext_clearPsa(mbedtls_ChannelContext *context) {
    UA_mbedTLS_PsaKey_clear(&context->localSymSigningKeyPsa);
    UA_mbedTLS_PsaKey_clear(&context->localSymEncryptingKeyPsa);
    UA_mbedTLS_PsaKey_clear(&context->remoteSymSigningKeyPsa);
    UA_mbedTLS_PsaKey_clear(&context->remoteSymEncryptingKeyPsa);
}

void
swapBuffers(UA_ByteString *const bufA, UA_ByteString *const bufB) {
    UA_ByteString tmp = *bufA;
    *bufA = *bufB;
    *bufB = tmp;
}

/* Substring search on a (not necessarily null-terminated) UA_String. */
static UA_Boolean
policyUriContains(const UA_String *uri, const char *token) {
    size_t n = strlen(token);
    if(uri->length < n)
        return false;
    for(size_t i = 0; i + n <= uri->length; i++) {
        if(memcmp(uri->data + i, token, n) == 0)
            return true;
    }
    return false;
}

/* OPC UA Part 6 v1.05.07 (SecureChannelEnhancements): hash a certificate (the
 * leaf, DER) with the hash of the policy's elliptic curve - SHA-256 for the
 * nistP256 curve, SHA-384 for nistP384. Used to build the channel-bound
 * CreateSession / ActivateSession SignatureData.
 *
 * This is NOT the OPN-header Certificate thumbprint: that thumbprint uses the
 * policy's CertificateThumbprintAlgorithm (SHA-1 by default) and is produced by
 * makeCertThumbprint. The digest here is selected from the policy URI's curve. */
UA_StatusCode
UA_SecurityPolicy_hashCertificate(const UA_SecurityPolicy *policy,
                                  const UA_ByteString *certificate,
                                  UA_ByteString *hash) {
    if(policy == NULL || certificate == NULL || hash == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Select the digest from the policy's elliptic curve, not a fixed
     * algorithm. */
    mbedtls_md_type_t mdType;
    size_t hashLen;
    if(policyUriContains(&policy->policyUri, "P384")) {
        mdType = MBEDTLS_MD_SHA384;
        hashLen = 48;
    } else if(policyUriContains(&policy->policyUri, "P256")) {
        mdType = MBEDTLS_MD_SHA256;
        hashLen = 32;
    } else {
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
    }

    const mbedtls_md_info_t *mdInfo = mbedtls_md_info_from_type(mdType);
    if(mdInfo == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_StatusCode ret = UA_ByteString_allocBuffer(hash, hashLen);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;
    UA_ByteString cert = {certificate->length, certificate->data};
    psa_algorithm_t psaAlgorithm = (mdType == MBEDTLS_MD_SHA384) ?
        PSA_ALG_SHA_384 : PSA_ALG_SHA_256;
    if(UA_mbedTLS_PsaHashCompute(psaAlgorithm, &cert, hash) != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(hash);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

/* Backend-agnostic SecurityPolicy properties derived from the policy URI. Kept
 * out of the UA_SecurityPolicy struct (so its layout stays stable across the
 * 1.5 release family); see the #None fallback in ua_securitypolicy_none.c used
 * when no crypto backend is built. */

/* True if the policy requires the OPC UA Part 6 v1.05.07
 * SecureChannelEnhancements behavior (the ECC_nistP256_* policies). */
UA_Boolean
UA_SecurityPolicy_isEnhancedSecurity(const UA_SecurityPolicy *policy) {
    if(!policy)
        return false;
    static const UA_String eccNistP256AesGcm =
        UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256_AesGcm");
    static const UA_String eccNistP256ChaChaPoly =
        UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256_ChaChaPoly");
    return UA_String_equal(&policy->policyUri, &eccNistP256AesGcm) ||
           UA_String_equal(&policy->policyUri, &eccNistP256ChaChaPoly);
}

/* In the OPC UA reference stack the SequenceNumber handling depends on a
 * SecurityPolicy property `LegacySequenceNumbers`: false for all ECC policies
 * (and RSA-DH, which open62541 does not implement), true for None / RSA
 * (Basic*, Aes*_RsaOaep/RsaPss). Non-legacy starts the channel SequenceNumber
 * at 0 and wraps UA_UINT32_MAX -> 0; legacy starts at 1 with the "< 1024"
 * rollover. Detected from the policy URI (all ECC URIs carry the "ECC_"
 * fragment). A NULL policy is treated as legacy. */
UA_Boolean
UA_SecurityPolicy_useLegacySequenceNumbers(const UA_SecurityPolicy *policy) {
    if(!policy)
        return true;
    return !policyUriContains(&policy->policyUri, "ECC_");
}

UA_StatusCode
UA_mbedTLS_thumbprintSha1(const UA_ByteString *certificate,
                        UA_ByteString *thumbprint) {
    if(UA_ByteString_equal(certificate, &UA_BYTESTRING_NULL))
        return UA_STATUSCODE_BADINTERNALERROR;

    if(thumbprint->length != UA_SHA1_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* The certificate thumbprint is always a 20 bit sha1 hash, see Part 4 of the Specification. */
    return UA_mbedTLS_PsaHashCompute(PSA_ALG_SHA_1, certificate, thumbprint);
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

    ret = mbedtls_x509write_csr_set_extension(ctx, MBEDTLS_OID_SUBJECT_ALT_NAME,
                                              MBEDTLS_OID_SIZE(MBEDTLS_OID_SUBJECT_ALT_NAME), 0, (const unsigned char*)(buf + buflen - len), len);

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
#if MBEDTLS_VERSION_NUMBER < 0x04000000
UA_StatusCode
mbedtls_createSigningRequest(mbedtls_pk_context *localPrivateKey,
                             mbedtls_pk_context *csrLocalPrivateKey,
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
    int (*rng)(void *, unsigned char *, size_t) = mbedtls_psa_get_random;
    void *rngContext = MBEDTLS_PSA_RANDOM_STATE;
    (void)nonce; /* PSA manages the entropy pool. */

    mbedtls_pk_free(csrLocalPrivateKey);

    /* CSR has already been generated and private key only needs to be set
     * if a new one has been generated. */
    if(newPrivateKey && newPrivateKey->length > 0) {
        mbedtls_pk_init(csrLocalPrivateKey);

        /* Set the private key */
        if(UA_mbedTLS_LoadPrivateKey(newPrivateKey, csrLocalPrivateKey))
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
        if(mbedtls_pk_can_do(localPrivateKey, MBEDTLS_PK_ECKEY)) {
            mbedtls_pk_setup(csrLocalPrivateKey,
                             mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
            mbedtls_ecp_keypair *existingKp = mbedtls_pk_ec(*localPrivateKey);
            mbedtls_ecp_keypair *newKp = mbedtls_pk_ec(*csrLocalPrivateKey);
            if(mbedtls_ecp_gen_key(
                   existingKp->MBEDTLS_PRIVATE(grp).id,
                   newKp, rng, rngContext) != 0) {
                retval = UA_STATUSCODE_BADINTERNALERROR;
                goto cleanup;
            }
        } else
        {
            mbedtls_pk_setup(csrLocalPrivateKey,
                             mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
            size_t keySize = 0;
            UA_CertificateUtils_getKeySize(&securityPolicy->localCertificate, &keySize);
            mbedtls_rsa_gen_key(mbedtls_pk_rsa(*csrLocalPrivateKey), rng,
                                rngContext, (unsigned int)keySize, 65537);
        }
        mbedtls_x509write_csr_set_key(&request, csrLocalPrivateKey);
        mbedtls_writePrivateKeyDer(csrLocalPrivateKey, newPrivateKey);
    } else {
        mbedtls_x509write_csr_set_key(&request, localPrivateKey);
    }

    /* The private key associated with the request will be used for signing the
     * created CSR. For RSA keys, enforce using RSASSA-PKCS1-v1_5 scheme. */
    mbedtls_pk_context *reqKey = request.private_key;
    if(mbedtls_pk_can_do(reqKey, MBEDTLS_PK_RSA)) {
        mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(*reqKey);
        mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);
    }

    unsigned char requestBuf[CSR_BUFFER_SIZE];
    memset(requestBuf, 0, sizeof(requestBuf));
    ret = mbedtls_x509write_csr_der(&request, requestBuf, sizeof(requestBuf),
                                    rng, rngContext);
    if(ret <= 0) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* number of CSR data bytes located at the end of the request buffer */
    size_t byteCount = (size_t)ret;
    size_t offset = sizeof(requestBuf) - byteCount;

    /* copy return parameter into a ByteString */
    UA_ByteString_init(csr);
    retval = UA_ByteString_allocBuffer(csr, byteCount);
    if(retval == UA_STATUSCODE_GOOD)
        memcpy(csr->data, requestBuf + offset, byteCount);

cleanup:
    mbedtls_x509_crt_free(&x509Cert);
    mbedtls_x509write_csr_free(&request);
    if(subj)
        UA_free(subj);

    return retval;
}
#endif

#if MBEDTLS_VERSION_NUMBER >= 0x04000000
UA_StatusCode
UA_mbedTLS_createSigningRequestV4(mbedtls_pk_context *localPrivateKey,
                                  mbedtls_pk_context *csrLocalPrivateKey,
                                  UA_SecurityPolicy *securityPolicy,
                                  const UA_String *subjectName,
                                  const UA_ByteString *nonce,
                                  UA_ByteString *csr,
                                  UA_ByteString *newPrivateKey) {
    if(!securityPolicy || !csr || !localPrivateKey || !csrLocalPrivateKey)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    (void)nonce; /* PSA manages its own entropy pool. */
    mbedtls_pk_free(csrLocalPrivateKey);
    if(newPrivateKey && newPrivateKey->length > 0) {
        mbedtls_pk_init(csrLocalPrivateKey);
        return UA_mbedTLS_LoadPrivateKey(newPrivateKey, csrLocalPrivateKey) == 0 ?
            UA_STATUSCODE_GOOD : UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }
    if(csr->length > 0)
        return UA_STATUSCODE_GOOD;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    char *subj = NULL;
    mbedtls_x509_crt x509Cert;
    mbedtls_x509_crt_init(&x509Cert);
    mbedtls_x509write_csr request;
    mbedtls_x509write_csr_init(&request);

    UA_ByteString certificateStr =
        UA_mbedTLS_CopyDataFormatAware(&securityPolicy->localCertificate);
    int ret = mbedtls_x509_crt_parse(&x509Cert, certificateStr.data,
                                     certificateStr.length);
    UA_ByteString_clear(&certificateStr);
    if(ret != 0) {
        retval = UA_STATUSCODE_BADCERTIFICATEINVALID;
        goto cleanup;
    }

    mbedtls_x509write_csr_set_md_alg(&request, MBEDTLS_MD_SHA256);
    ret = mbedtls_x509write_csr_set_key_usage(
        &request, MBEDTLS_X509_KU_DIGITAL_SIGNATURE |
                  MBEDTLS_X509_KU_DATA_ENCIPHERMENT |
                  MBEDTLS_X509_KU_NON_REPUDIATION |
                  MBEDTLS_X509_KU_KEY_ENCIPHERMENT);
    if(ret != 0) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    if(subjectName && subjectName->length > 0) {
        subj = (char*)UA_malloc(subjectName->length + 1);
        if(!subj) {
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        memcpy(subj, subjectName->data, subjectName->length);
        subj[subjectName->length] = 0;
        for(size_t i = 0; i < subjectName->length; i++) {
            if(subj[i] == '/')
                subj[i] = ',';
        }
    } else {
        subj = (char*)UA_malloc(UA_MAXSUBJECTLENGTH);
        if(!subj) {
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        if(mbedtls_x509_dn_gets(subj, UA_MAXSUBJECTLENGTH,
                               &x509Cert.subject) <= 0) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
    }
    if(mbedtls_x509write_csr_set_subject_name(&request, subj) != 0 ||
       mbedtls_x509write_csrSetSubjectAltName(
           &request, &x509Cert.subject_alt_names) != UA_STATUSCODE_GOOD) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    if(newPrivateKey) {
        psa_key_type_t keyType = mbedtls_pk_get_key_type(localPrivateKey);
        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
        psa_set_key_type(&attributes, keyType);
        psa_set_key_bits(&attributes, mbedtls_pk_get_bitlen(localPrivateKey));
        psa_set_key_usage_flags(&attributes,
                                PSA_KEY_USAGE_EXPORT | PSA_KEY_USAGE_SIGN_HASH);
        if(keyType == PSA_KEY_TYPE_RSA_KEY_PAIR)
            psa_set_key_algorithm(&attributes,
                PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256));
        else if(PSA_KEY_TYPE_IS_ECC_KEY_PAIR(keyType))
            psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
        else {
            psa_reset_key_attributes(&attributes);
            retval = UA_STATUSCODE_BADNOTSUPPORTED;
            goto cleanup;
        }
        UA_mbedTLS_PsaKey generated;
        UA_mbedTLS_PsaKey_init(&generated);
        psa_status_t status = psa_generate_key(&attributes, &generated.id);
        psa_reset_key_attributes(&attributes);
        if(status == PSA_SUCCESS)
            generated.owned = true;
        mbedtls_pk_init(csrLocalPrivateKey);
        if(status != PSA_SUCCESS ||
           mbedtls_pk_copy_from_psa(generated.id, csrLocalPrivateKey) != 0) {
            UA_mbedTLS_PsaKey_clear(&generated);
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
        UA_mbedTLS_PsaKey_clear(&generated);
        mbedtls_x509write_csr_set_key(&request, csrLocalPrivateKey);
        retval = mbedtls_writePrivateKeyDer(csrLocalPrivateKey, newPrivateKey);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
    } else {
        mbedtls_x509write_csr_set_key(&request, localPrivateKey);
    }

    unsigned char requestBuf[CSR_BUFFER_SIZE];
    memset(requestBuf, 0, sizeof(requestBuf));
    ret = mbedtls_x509write_csr_der(&request, requestBuf, sizeof(requestBuf));
    if(ret <= 0) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }
    size_t byteCount = (size_t)ret;
    retval = UA_ByteString_allocBuffer(csr, byteCount);
    if(retval == UA_STATUSCODE_GOOD)
        memcpy(csr->data, requestBuf + sizeof(requestBuf) - byteCount, byteCount);

cleanup:
    mbedtls_x509_crt_free(&x509Cert);
    mbedtls_x509write_csr_free(&request);
    UA_free(subj);
    return retval;
}
#endif

int
UA_mbedTLS_LoadPrivateKey(const UA_ByteString *key, mbedtls_pk_context *target) {
    if(UA_mbedTLS_PSA_Init() != UA_STATUSCODE_GOOD)
        return -1;
    UA_ByteString data = UA_mbedTLS_CopyDataFormatAware(key);
#if MBEDTLS_VERSION_NUMBER >= 0x04000000
    int mbedErr = mbedtls_pk_parse_key(target, data.data, data.length, NULL, 0);
#else
    int mbedErr = mbedtls_pk_parse_key(target, data.data, data.length, NULL, 0,
                                       mbedtls_psa_get_random,
                                       MBEDTLS_PSA_RANDOM_STATE);
#endif
    UA_ByteString_clear(&data);
    return mbedErr;
}

UA_Boolean
UA_mbedTLS_IsEccKeyPair(const mbedtls_pk_context *key) {
#if MBEDTLS_VERSION_NUMBER >= 0x04000000
    return key && PSA_KEY_TYPE_IS_ECC_KEY_PAIR(mbedtls_pk_get_key_type(key));
#else
    return key && mbedtls_pk_can_do(key, MBEDTLS_PK_ECKEY);
#endif
}

UA_StatusCode
UA_mbedTLS_LoadCertificate(const UA_ByteString *certificate, mbedtls_x509_crt *target) {
    UA_StatusCode res = UA_mbedTLS_PSA_Init();
    if(res != UA_STATUSCODE_GOOD)
        return res;
    const unsigned char *pData = certificate->data;

    // Magic number for DER encoded files
    if(certificate->length > 1 && pData[0] == 0x30 && pData[1] == 0x82)
        return UA_mbedTLS_LoadDerCertificate(certificate, target);
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
    int mbedErr = mbedtls_x509_crt_parse(target, certificateData.data,
                                         certificateData.length);
    UA_ByteString_clear(&certificateData);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_LoadCrl(const UA_ByteString *crl, mbedtls_x509_crl *target) {
    UA_StatusCode res = UA_mbedTLS_PSA_Init();
    if(res != UA_STATUSCODE_GOOD)
        return res;
    const unsigned char *pData = crl->data;

    // Magic number for DER encoded files
    if(crl->length > 1 && pData[0] == 0x30 && pData[1] == 0x82)
        return UA_mbedTLS_LoadDerCrl(crl, target);
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
    if(UA_mbedTLS_PSA_Init() != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADINTERNALERROR;
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

size_t
UA_mbedTLS_asym_getRemoteSignatureSize_generic(const UA_SecurityPolicy *policy,
                                               const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    const mbedtls_ChannelContext *cc =
        (const mbedtls_ChannelContext*)channelContext;
    return (mbedtls_pk_get_bitlen(&cc->remoteCertificate.pk) + 7) / 8;
}

size_t
UA_mbedTLS_asym_getRemoteBlockSize_generic(const UA_SecurityPolicy *policy,
                                           const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    const mbedtls_ChannelContext *cc =
        (const mbedtls_ChannelContext*)channelContext;
    return (mbedtls_pk_get_bitlen(&cc->remoteCertificate.pk) + 7) / 8;
}

static psa_algorithm_t
policyMacAlgorithm(const UA_SecurityPolicy *policy) {
    psa_algorithm_t hashAlgorithm = PSA_ALG_SHA_256;
    if(policyUriContains(&policy->policyUri, "Basic128Rsa15") ||
       (policyUriContains(&policy->policyUri, "#Basic256") &&
        !policyUriContains(&policy->policyUri, "Basic256Sha256")))
        hashAlgorithm = PSA_ALG_SHA_1;
    else if(policyUriContains(&policy->policyUri, "P384") ||
            policyUriContains(&policy->policyUri, "brainpoolP384"))
        hashAlgorithm = PSA_ALG_SHA_384;
    return PSA_ALG_HMAC(hashAlgorithm);
}

UA_StatusCode
UA_mbedTLS_setLocalSymEncryptingKey_generic(const UA_SecurityPolicy *policy,
                                            void *channelContext,
                                            const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_ChannelContext *cc =
        (mbedtls_ChannelContext*)channelContext;
    UA_mbedTLS_PsaKey replacementPsa;
    UA_mbedTLS_PsaKey_init(&replacementPsa);
    UA_StatusCode res = UA_mbedTLS_PsaKey_import(
        &replacementPsa, PSA_KEY_TYPE_AES, PSA_KEY_USAGE_ENCRYPT,
        PSA_ALG_CBC_NO_PADDING, key);
    if(res == UA_STATUSCODE_GOOD) {
        UA_mbedTLS_PsaKey_clear(&cc->localSymEncryptingKeyPsa);
        cc->localSymEncryptingKeyPsa = replacementPsa;
        UA_mbedTLS_PsaKey_init(&replacementPsa);
    }
    UA_mbedTLS_PsaKey_clear(&replacementPsa);
    return res;
}

UA_StatusCode
UA_mbedTLS_setLocalSymSigningKey_generic(const UA_SecurityPolicy *policy,
                                         void *channelContext,
                                         const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_ChannelContext *cc = (mbedtls_ChannelContext *)channelContext;
    UA_mbedTLS_PsaKey replacementPsa;
    UA_mbedTLS_PsaKey_init(&replacementPsa);
    UA_StatusCode res = UA_mbedTLS_PsaKey_import(
        &replacementPsa, PSA_KEY_TYPE_HMAC, PSA_KEY_USAGE_SIGN_MESSAGE,
        policyMacAlgorithm(policy), key);
    if(res == UA_STATUSCODE_GOOD) {
        UA_mbedTLS_PsaKey_clear(&cc->localSymSigningKeyPsa);
        cc->localSymSigningKeyPsa = replacementPsa;
        UA_mbedTLS_PsaKey_init(&replacementPsa);
    }
    UA_mbedTLS_PsaKey_clear(&replacementPsa);
    return res;
}

UA_StatusCode
UA_mbedTLS_setLocalSymIv_generic(const UA_SecurityPolicy *policy,
                                 void *channelContext,
                                 const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_ChannelContext *cc = (mbedtls_ChannelContext *)channelContext;
    UA_ByteString replacement = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_ByteString_copy(iv, &replacement);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_ByteString_clear(&cc->localSymIv);
    cc->localSymIv = replacement;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_setRemoteSymEncryptingKey_generic(const UA_SecurityPolicy *policy,
                                             void *channelContext,
                                             const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_ChannelContext *cc =
        (mbedtls_ChannelContext*)channelContext;
    UA_mbedTLS_PsaKey replacementPsa;
    UA_mbedTLS_PsaKey_init(&replacementPsa);
    UA_StatusCode res = UA_mbedTLS_PsaKey_import(
        &replacementPsa, PSA_KEY_TYPE_AES, PSA_KEY_USAGE_DECRYPT,
        PSA_ALG_CBC_NO_PADDING, key);
    if(res == UA_STATUSCODE_GOOD) {
        UA_mbedTLS_PsaKey_clear(&cc->remoteSymEncryptingKeyPsa);
        cc->remoteSymEncryptingKeyPsa = replacementPsa;
        UA_mbedTLS_PsaKey_init(&replacementPsa);
    }
    UA_mbedTLS_PsaKey_clear(&replacementPsa);
    return res;
}

UA_StatusCode
UA_mbedTLS_setRemoteSymSigningKey_generic(const UA_SecurityPolicy *policy,
                                          void *channelContext,
                                          const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_ChannelContext *cc =
        (mbedtls_ChannelContext*)channelContext;
    UA_mbedTLS_PsaKey replacementPsa;
    UA_mbedTLS_PsaKey_init(&replacementPsa);
    UA_StatusCode res = UA_mbedTLS_PsaKey_import(
        &replacementPsa, PSA_KEY_TYPE_HMAC, PSA_KEY_USAGE_VERIFY_MESSAGE,
        policyMacAlgorithm(policy), key);
    if(res == UA_STATUSCODE_GOOD) {
        UA_mbedTLS_PsaKey_clear(&cc->remoteSymSigningKeyPsa);
        cc->remoteSymSigningKeyPsa = replacementPsa;
        UA_mbedTLS_PsaKey_init(&replacementPsa);
    }
    UA_mbedTLS_PsaKey_clear(&replacementPsa);
    return res;
}

UA_StatusCode
UA_mbedTLS_setRemoteSymIv_generic(const UA_SecurityPolicy *policy,
                                  void *channelContext,
                                  const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_ChannelContext *cc =
        (mbedtls_ChannelContext*)channelContext;
    UA_ByteString replacement = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_ByteString_copy(iv, &replacement);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_ByteString_clear(&cc->remoteSymIv);
    cc->remoteSymIv = replacement;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_compareCertificate_generic(const UA_SecurityPolicy *policy,
                                      const void *channelContext,
                                      const UA_ByteString *certificate) {
    if(channelContext == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    int mbedErr = mbedtls_x509_crt_parse(&cert, certificate->data, certificate->length);
    if(mbedErr) {
        mbedtls_x509_crt_free(&cert);
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    const mbedtls_ChannelContext *cc =
        (const mbedtls_ChannelContext*)channelContext;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(cert.raw.len != cc->remoteCertificate.raw.len ||
       memcmp(cert.raw.p, cc->remoteCertificate.raw.p, cert.raw.len) != 0)
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    mbedtls_x509_crt_free(&cert);
    return retval;
}

size_t
UA_mbedTLS_getRemoteCertificatePrivateKeyLength(const UA_SecurityPolicy *policy,
                                                const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    const mbedtls_ChannelContext *cc =
        (const mbedtls_ChannelContext*)channelContext;
    return (mbedtls_pk_get_bitlen(&cc->remoteCertificate.pk) + 7) / 8;
}

size_t
UA_mbedTLS_getLocalPrivateKeyLength(const UA_SecurityPolicy *policy,
                                    const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    mbedtls_PolicyContext *pc =
        (mbedtls_PolicyContext*)policy->policyContext;
    return (mbedtls_pk_get_bitlen(&pc->localPrivateKey) + 7) / 8;
}

UA_StatusCode
UA_mbedTLS_compareCertificateThumbprint_generic(const UA_SecurityPolicy *securityPolicy,
                                                const UA_ByteString *certificateThumbprint) {
    if(securityPolicy == NULL || certificateThumbprint == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const mbedtls_PolicyContext *pc = (const mbedtls_PolicyContext *)
        securityPolicy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint))
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_mbedTLS_sym_generateKey_generic(const UA_SecurityPolicy *policy,
                                   void *channelContext, const UA_ByteString *secret,
                                   const UA_ByteString *seed, UA_ByteString *out) {
    if(secret == NULL || seed == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    psa_algorithm_t hashAlgorithm = PSA_ALG_SHA_256;
    if(policyUriContains(&policy->policyUri, "Basic128Rsa15") ||
       (policyUriContains(&policy->policyUri, "#Basic256") &&
        !policyUriContains(&policy->policyUri, "Basic256Sha256")))
        hashAlgorithm = PSA_ALG_SHA_1;
    else if(policyUriContains(&policy->policyUri, "P384") ||
            policyUriContains(&policy->policyUri, "brainpoolP384"))
        hashAlgorithm = PSA_ALG_SHA_384;
    return UA_mbedTLS_PsaPHash(hashAlgorithm, secret, seed, out);
}

UA_StatusCode
UA_mbedTLS_sym_generateNonce_generic(const UA_SecurityPolicy *policy,
                                     void *channelContext, UA_ByteString *out) {
    if(out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_mbedTLS_PsaRandom(out);
}

UA_StatusCode
UA_mbedTLS_createSigningRequest_generic(UA_SecurityPolicy *securityPolicy,
                                        const UA_String *subjectName,
                                        const UA_ByteString *nonce,
                                        const UA_KeyValueMap *params,
                                        UA_ByteString *csr,
                                        UA_ByteString *newPrivateKey) {
    if(securityPolicy == NULL || csr == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(securityPolicy->policyContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mbedtls_PolicyContext *pc = (mbedtls_PolicyContext *)securityPolicy->policyContext;
#if MBEDTLS_VERSION_NUMBER >= 0x04000000
    (void)params;
    return UA_mbedTLS_createSigningRequestV4(
        &pc->localPrivateKey, &pc->csrLocalPrivateKey, securityPolicy,
        subjectName, nonce, csr, newPrivateKey);
#else
    return mbedtls_createSigningRequest(&pc->localPrivateKey, &pc->csrLocalPrivateKey,
                                        securityPolicy, subjectName, nonce,
                                        csr, newPrivateKey);
#endif
}



#endif
