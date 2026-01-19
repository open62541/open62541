/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Siemens AG (Author: Tin Raic)
 *    Copyright 2025-2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "ua_util_internal.h"

/************************/
/* ECC Encrypted Secret */
/************************/

typedef struct {
    /* Common Header */
    UA_NodeId typeId;
    UA_Byte encodingMask;
    UA_UInt32 length;
    UA_String securityPolicyUri;
    UA_ByteString certificate;
    UA_DateTime signingTime;
    UA_UInt16 keyDataLen;

    /* Policy Header*/
    UA_ByteString senderPublicKey;
    UA_ByteString receiverPublicKey;

    /* Payload */
    UA_ByteString nonce;
    UA_ByteString secret;
    /* UA_ByteString padding; // Computed when needed */

    /* Signature */
    UA_Byte* signature;
} UA_EccEncryptedSecretStruct;

static void
UA_EccEncryptedSecretStruct_init(UA_EccEncryptedSecretStruct* es) {
    memset(es, 0, sizeof(UA_EccEncryptedSecretStruct));
}

static void
UA_EccEncryptedSecretStruct_clear(UA_EccEncryptedSecretStruct* es) {
    UA_String_clear(&es->securityPolicyUri);
    UA_ByteString_clear(&es->certificate);
    UA_ByteString_clear(&es->senderPublicKey);
    UA_ByteString_clear(&es->receiverPublicKey);
    UA_ByteString_clear(&es->nonce);
    UA_ByteString_clear(&es->secret);
    UA_EccEncryptedSecretStruct_init(es);
}

static size_t
UA_EccEncryptedSecret_getCommonHeaderSize(const UA_EccEncryptedSecretStruct* src) {
    size_t len = 0;
    len += UA_calcSizeBinary(&src->typeId, &UA_TYPES[UA_TYPES_NODEID], NULL);
    len += UA_calcSizeBinary(&src->encodingMask, &UA_TYPES[UA_TYPES_BYTE], NULL);
    len += UA_calcSizeBinary(&src->length, &UA_TYPES[UA_TYPES_UINT32], NULL);
    len += UA_calcSizeBinary(&src->securityPolicyUri, &UA_TYPES[UA_TYPES_STRING], NULL);
    len += UA_calcSizeBinary(&src->certificate, &UA_TYPES[UA_TYPES_BYTESTRING], NULL);
    len += UA_calcSizeBinary(&src->signingTime, &UA_TYPES[UA_TYPES_DATETIME], NULL);
    len += UA_calcSizeBinary(&src->keyDataLen, &UA_TYPES[UA_TYPES_UINT16], NULL);
    return len;
}

static size_t
UA_EccEncryptedSecret_getPolicyHeaderSize(const UA_EccEncryptedSecretStruct* src) {
    size_t len = 0;
    len += UA_calcSizeBinary(&src->senderPublicKey, &UA_TYPES[UA_TYPES_BYTESTRING], NULL);
    len += UA_calcSizeBinary(&src->receiverPublicKey, &UA_TYPES[UA_TYPES_BYTESTRING], NULL);
    return len;
}

static UA_StatusCode
UA_EccEncryptedSecret_serializeCommonHeader(const UA_EccEncryptedSecretStruct *src,
                                            UA_Byte** bufPos, const UA_Byte* bufEnd) {
    UA_UInt32 length32 = (UA_UInt32)src->length;
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    ret |= UA_NodeId_encodeBinary(&src->typeId, bufPos, bufEnd);
    ret |= UA_Byte_encodeBinary(&src->encodingMask, bufPos, bufEnd);
    ret |= UA_UInt32_encodeBinary(&length32, bufPos, bufEnd);
    ret |= UA_String_encodeBinary(&src->securityPolicyUri, bufPos, bufEnd);
    ret |= UA_ByteString_encodeBinary(&src->certificate, bufPos, bufEnd);
    ret |= UA_DateTime_encodeBinary(&src->signingTime, bufPos, bufEnd);
    ret |= UA_UInt16_encodeBinary(&src->keyDataLen, bufPos, bufEnd);
    return ret;
}

static UA_StatusCode
UA_EccEncryptedSecret_serializePolicyHeader(const UA_EccEncryptedSecretStruct *src,
                                            UA_Byte** bufPos, const UA_Byte* bufEnd) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    ret |= UA_ByteString_encodeBinary(&src->senderPublicKey, bufPos, bufEnd);
    ret |= UA_ByteString_encodeBinary(&src->receiverPublicKey, bufPos, bufEnd);
    return ret;
}

static UA_StatusCode
UA_EccEncryptedSecret_deserializeCommonHeader(UA_EccEncryptedSecret *src,
                                              UA_EccEncryptedSecretStruct *dest,
                                              size_t* offset) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    ret |= UA_NodeId_decodeBinary(src, offset, &dest->typeId);
    ret |= UA_Byte_decodeBinary(src, offset, &dest->encodingMask);
    ret |= UA_UInt32_decodeBinary(src, offset, &dest->length);
    ret |= UA_String_decodeBinary(src, offset, &dest->securityPolicyUri);
    ret |= UA_ByteString_decodeBinary(src, offset, &dest->certificate);
    ret |= UA_DateTime_decodeBinary(src, offset, &dest->signingTime);
    ret |= UA_UInt16_decodeBinary(src, offset, &dest->keyDataLen);
    return ret;
}

static UA_StatusCode
UA_EccEncryptedSecret_deserializePolicyHeader(UA_EccEncryptedSecret *src,
                                              UA_EccEncryptedSecretStruct *dest,
                                              size_t* offset) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    ret |= UA_ByteString_decodeBinary(src, offset, &dest->senderPublicKey);
    ret |= UA_ByteString_decodeBinary(src, offset, &dest->receiverPublicKey);
    return ret;
}

UA_StatusCode
encryptUserIdentityTokenEcc(UA_Logger *logger, UA_SecureChannel *channel,
                            const UA_SecurityPolicy *sp, void *spContext,
                            UA_ByteString *tokenData,
                            const UA_ByteString serverSessionNonce,
                            const UA_ByteString serverEphemeralPubKey) {
    /* Extract some basic information from the SecurityPolicy */
    size_t symKeyLen = sp->symEncryptionAlgorithm.getLocalKeyLength(sp, spContext);
    size_t ivLen = sp->symEncryptionAlgorithm.getRemoteBlockSize(sp, spContext);
    size_t sigLen = sp->asymSignatureAlgorithm.getRemoteSignatureSize(sp, spContext);
    UA_assert(symKeyLen > 0 && ivLen > 0);

    /* Filling out the EccEncryptedSecretStruct fields. The length field is
     * computed after. */
    UA_EccEncryptedSecretStruct secret;
    UA_EccEncryptedSecretStruct_init(&secret);
    secret.typeId = UA_NS0ID(ECCENCRYPTEDSECRET);
    secret.encodingMask = 0x01;
    secret.signingTime = UA_DateTime_now();

    /* Copy-only methods */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_String_copy(&sp->policyUri, &secret.securityPolicyUri);
    retval |= UA_ByteString_copy(&sp->localCertificate, &secret.certificate);
    retval |= UA_ByteString_copy(&serverEphemeralPubKey, &secret.receiverPublicKey);
    retval |= UA_ByteString_copy(&serverSessionNonce, &secret.nonce);
    retval |= UA_ByteString_copy(tokenData, &secret.secret);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_EccEncryptedSecretStruct_clear(&secret);
        return retval;
    }

    /* Generate a new local ephemeral key. The private part remains persisted
     * inside the SecurityPolicy. */
    size_t ephKeyLen = sp->nonceLength; /* Also length of the ephemeral public key */
    retval = UA_ByteString_allocBuffer(&secret.senderPublicKey, ephKeyLen);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_EccEncryptedSecretStruct_clear(&secret);
        return retval;
    }
    retval = sp->generateNonce(sp, spContext, &secret.senderPublicKey);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(logger, channel,
                     "EccEncryptedSecret: Failed to generate local ephemeral key");
        UA_EccEncryptedSecretStruct_clear(&secret);
        return retval;
    }

    /* Length of the encrypted content. If the InitializationVectorLength is
     * less than 16 bytes, then 16 bytes are used instead. */
    size_t encryptedLength =
        UA_ByteString_calcSizeBinary(&secret.nonce) +
        UA_ByteString_calcSizeBinary(&secret.secret) + 2; /* Incl padding length */
    size_t paddingIvLen = ivLen;
    if(paddingIvLen < 16)
        paddingIvLen = 16;
    size_t paddingLen = paddingIvLen - (encryptedLength % paddingIvLen);
    encryptedLength += paddingLen;

    /* Compute the total length including the headers and the signature */
    size_t signatureLen = sp->asymSignatureAlgorithm.
        getLocalSignatureSize(sp, spContext);
    secret.keyDataLen = (UA_UInt16)
        UA_EccEncryptedSecret_getPolicyHeaderSize(&secret);
    size_t totalLength = encryptedLength + secret.keyDataLen +
        UA_EccEncryptedSecret_getCommonHeaderSize(&secret) + signatureLen;
    secret.length = (UA_UInt32)totalLength;

    /* Compute the symmetric key to encrypt */
    UA_ByteString symEncKeyMaterial;
    retval = UA_ByteString_allocBuffer(&symEncKeyMaterial, symKeyLen+ivLen);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_EccEncryptedSecretStruct_clear(&secret);
        return retval;
    }

    /* This is a (temporary) measure so that the salt generation function (for
     * the symmetric key derivation ) knows that the salt is generated for
     * session authentication (to choose the correct label) */
    /* TODO: find a better way to signal symmetric key generation for session
     * authentication */
    symEncKeyMaterial.data[0] = 0x03;
    symEncKeyMaterial.data[1] = 0x03;
    symEncKeyMaterial.data[2] = 0x04;
    retval = sp->generateKey(sp, spContext, &secret.receiverPublicKey,
                             &secret.senderPublicKey, &symEncKeyMaterial);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(logger, channel,
                             "EccEncryptedSecret: Failed to derive key material");
        UA_ByteString_clear(&symEncKeyMaterial);
        UA_EccEncryptedSecretStruct_clear(&secret);
        return retval;
    }

    /* Extracting the key and the initialization vector from the key material */
    UA_ByteString encKey = {symKeyLen, symEncKeyMaterial.data};
    UA_ByteString iv = {ivLen, &symEncKeyMaterial.data[symKeyLen]};
    retval |= sp->setLocalSymEncryptingKey(sp, spContext, &encKey);
    retval |= sp->setLocalSymIv(sp, spContext, &iv);
    UA_ByteString_clear(&symEncKeyMaterial);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(logger, channel,
                             "EccEncryptedSecret: Failed to set EncryptingKey/"
                             "IV in the SecurityPolicy");
        UA_EccEncryptedSecretStruct_clear(&secret);
        return retval;
    }

    /* Allocate the output buffer */
    UA_ByteString output;
    retval = UA_ByteString_allocBuffer(&output, totalLength);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_EccEncryptedSecretStruct_clear(&secret);
        return retval;
    }

    /* Encode all the content into the output buffer */
    UA_Byte* bufPos = output.data;
    UA_Byte* bufEnd = output.data + output.length;
    retval |= UA_EccEncryptedSecret_serializeCommonHeader(&secret, &bufPos, bufEnd);
    retval |= UA_EccEncryptedSecret_serializePolicyHeader(&secret, &bufPos, bufEnd);
    UA_Byte* payloadPos = bufPos;
    retval |= UA_ByteString_encodeBinary(&secret.nonce, &bufPos, bufEnd);
    retval |= UA_ByteString_encodeBinary(&secret.secret, &bufPos, bufEnd);
    UA_Byte pad = paddingLen & 0xFF;
    for(size_t i = 0; i < paddingLen; i++) {
        *bufPos = pad;
        bufPos++;
    }
    UA_UInt16 paddingLen16 = (UA_UInt16)paddingLen;
    retval |= UA_UInt16_encodeBinary(&paddingLen16, &bufPos, bufEnd);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_EccEncryptedSecretStruct_clear(&secret);
        UA_ByteString_clear(&output);
        return retval;
    }

    /* Encrypt the payload part in-situ */
    UA_ByteString payload = {(size_t)(bufPos - payloadPos), payloadPos};
    retval |= sp->symEncryptionAlgorithm.encrypt(sp, spContext, &payload);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(logger, channel,
                     "EccEncryptedSecret: Failed to encrypt the payload");
        UA_EccEncryptedSecretStruct_clear(&secret);
        UA_ByteString_clear(&output);
        return retval;
    }

    UA_assert(bufPos + sigLen == bufEnd);

    /* Compute the overall signature */
    UA_ByteString sigContent = {(size_t)(bufPos - output.data), output.data};
    UA_ByteString signature = {sigLen, bufPos};
    retval = sp->asymSignatureAlgorithm.sign(sp, spContext, &sigContent, &signature);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(logger, channel,
                     "EccEncryptedSecret: Failed to sign the EccEncryptedSecret");
        UA_EccEncryptedSecretStruct_clear(&secret);
        UA_ByteString_clear(&output);
        return retval;
    }

    /* Replace tokenData with the output */
    UA_ByteString_clear(tokenData);
    *tokenData = output;

    UA_EccEncryptedSecretStruct_clear(&secret);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
decryptUserTokenEcc(UA_Logger *logger, UA_SecureChannel *channel,
                    const UA_SecurityPolicy *sp, void *spContext,
                    UA_ByteString sessionServerNonce,
                    const UA_String encryptionAlgorithm,
                    UA_EccEncryptedSecret *es) {
    /* ECC usage verified before calling into this function */
    UA_assert(sp->policyType == UA_SECURITYPOLICYTYPE_ECC);

    /* Test if the correct encryption algorithm is used */
    if(encryptionAlgorithm.length > 0 &&
       !UA_String_equal(&encryptionAlgorithm, &sp->asymEncryptionAlgorithm.uri)) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EccEncryptedSecret] Wrong encryption algorithm");
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    }

    /* Define and initialize in case of clean-up */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_EccEncryptedSecretStruct esd;
    UA_EccEncryptedSecretStruct_init(&esd);
    UA_ByteString symEncKeyMaterial = UA_BYTESTRING_NULL;
    UA_ByteString pass = UA_BYTESTRING_NULL;

    size_t offset = 0;
    res = UA_EccEncryptedSecret_deserializeCommonHeader(es, &esd, &offset);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(logger, channel, "EccEncryptedSecret: "
                             "Failed to decode the common header");
        goto cleanecc;
    }

    /* Check TypeId */
    UA_NodeId eccTypeId = UA_NS0ID(ECCENCRYPTEDSECRET);
    if(!UA_NodeId_equal(&eccTypeId, &esd.typeId) ||
       esd.encodingMask != 0x01 ||
       !UA_String_equal(&esd.securityPolicyUri, &sp->policyUri)) {
        UA_LOG_ERROR_CHANNEL(logger, channel, "EccEncryptedSecret: "
                             "Inconsistent common header");
        res = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto cleanecc;
    }

    /* Verify signature */
    size_t sigLen = sp->asymSignatureAlgorithm.getRemoteSignatureSize(sp, spContext);
    size_t signedDataLen = es->length - sigLen;
    UA_ByteString signedData = {signedDataLen, es->data};
    UA_ByteString signature = {sigLen, &es->data[signedDataLen]};
    res = sp->asymSignatureAlgorithm.verify(sp, spContext, &signedData, &signature);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(logger, channel, "EccEncryptedSecret: "
                             "Signature verification failed");
        goto cleanecc;
    }

    size_t oldoffset = offset;
    res = UA_EccEncryptedSecret_deserializePolicyHeader(es, &esd, &offset);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(logger, channel, "EccEncryptedSecret: "
                             "Failed to decode the policy header");
        goto cleanecc;
    }

    /* Sanity check of the key data length.
     * This needs to include the 2x4 byte length fields. */
    if(esd.keyDataLen != (UA_UInt16)(offset - oldoffset)) {
        UA_LOG_ERROR_CHANNEL(logger, channel, "EccEncryptedSecret: "
                             "Inconstent KeyDataLength");
        res = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        goto cleanecc;
    }

    /* Check the payload length */
    if(es->length <= offset + sigLen) {
        UA_LOG_ERROR_CHANNEL(logger, channel, "EccEncryptedSecret: "
                             "Inconstent payload / signature length");
        res = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        goto cleanecc;
    }
    UA_ByteString payload = {es->length - offset - sigLen, es->data + offset};

    /* Deriving (remote) symmetric encryption key to decrypt the payload */
    size_t symKeyLen = sp->symEncryptionAlgorithm.getRemoteKeyLength(sp, spContext);
    size_t ivLen = sp->symEncryptionAlgorithm.getRemoteBlockSize(sp, spContext);
    res = UA_ByteString_allocBuffer(&symEncKeyMaterial, symKeyLen+ivLen);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanecc;

    /* This is a (temporary) measure so that the salt generation function (for
     * the symmetric key derivation ) knows that the salt is generated for
     * session authentication (to choose the correct label) */
    /* TODO: find a better way to signal symmetric key generation for session
     * authentication */
    symEncKeyMaterial.data[0] = 0x03;
    symEncKeyMaterial.data[1] = 0x03;
    symEncKeyMaterial.data[2] = 0x04;

    /* Call logic for server for session authentication: receiver public key is
     * local (server), sender public key is remote (client) */
    res = sp->generateKey(sp, spContext, &esd.receiverPublicKey,
                          &esd.senderPublicKey, &symEncKeyMaterial);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(logger, channel, "EccEncryptedSecret: "
                             "Failed to derive key material");
        goto cleanecc;
    }

    /* Extract the encryption key and the initialization vector */
    UA_ByteString encKey = {symKeyLen, symEncKeyMaterial.data};
    UA_ByteString iv = {ivLen, &symEncKeyMaterial.data[symKeyLen]};

    /* Set IV and RemoteEncryptingKey. That is all we need to decode the
     * secret. */
    res = sp->setRemoteSymEncryptingKey(sp, spContext, &encKey);
    res = sp->setRemoteSymIv(sp, spContext, &iv);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(logger, channel, "EccEncryptedSecret: "
                             "Failed to set IV/RemoteSymEncryptingKey");
        goto cleanecc;
    }

    /* Decrypt payload (password) */
    res = sp->symEncryptionAlgorithm.decrypt(sp, spContext, &payload);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(logger, channel, "EccEncryptedSecret: "
                             "Failed to decrypt the payload");
        goto cleanecc;
    }

    /* Decode nonce and secret */
    offset = 0; /* Within the payload */
    UA_ByteString nonce;
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    ret |= UA_ByteString_decodeBinary(&payload, &offset, &nonce);
    ret |= UA_ByteString_decodeBinary(&payload, &offset, &pass);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&nonce);
        UA_LOG_ERROR_CHANNEL(logger, channel, "EccEncryptedSecret: "
                             "Failed to decode the payload");
        goto cleanecc;
    }

    /* Compare the nonce */
    UA_Boolean nonceMatch = UA_ByteString_equal(&sessionServerNonce, &nonce);
    UA_ByteString_clear(&nonce);
    if(!nonceMatch) {
        UA_LOG_ERROR_CHANNEL(logger, channel, "EccEncryptedSecret: "
                             "Nonce does not match");
        res = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto cleanecc;
    }

    /* Decode the padding length and validate */
    UA_UInt16 paddingSize = 0;
    size_t paddingOffset = payload.length - 2;
    UA_UInt16_decodeBinary(&payload, &paddingOffset, &paddingSize);
    UA_Byte padd = (UA_Byte)paddingSize;
    for(; offset < payload.length-2; offset++) {
        if(payload.data[offset] != padd) {
            UA_LOG_ERROR_CHANNEL(logger, channel,
                                 "EccEncryptedSecret: Inconsistent padding");
            res = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
            goto cleanecc;
        }
    }

    /* Copy the password to the output */
    memcpy(es->data, pass.data, pass.length);
    es->length = pass.length;

cleanecc:
    UA_EccEncryptedSecretStruct_clear(&esd);
    UA_ByteString_clear(&symEncKeyMaterial);
    UA_ByteString_clear(&pass);
    return res;
}

/***************************/
/* Legacy Encrypted Secret */
/***************************/

UA_StatusCode
encryptSecretLegacy(const UA_SecurityPolicy *sp, void *spContext,
                    const UA_ByteString serverSessionNonce,
                    UA_ByteString *tokenData) {
    /* Compute the encrypted length (at least one byte padding) */
    size_t plainTextBlockSize = sp->asymEncryptionAlgorithm.
        getRemotePlainTextBlockSize(sp, spContext);
    size_t encryptedBlockSize = sp->asymEncryptionAlgorithm.
        getRemoteBlockSize(sp, spContext);
    UA_UInt32 length =
        (UA_UInt32)(tokenData->length + serverSessionNonce.length);
    UA_UInt32 totalLength = length + 4; /* Including the length field */
    size_t blocks = totalLength / plainTextBlockSize;
    if(totalLength % plainTextBlockSize != 0)
        blocks++;
    size_t encryptedLength = blocks * encryptedBlockSize;

    /* Allocate memory for the encrypted secret */
    UA_ByteString encrypted;
    UA_StatusCode res = UA_ByteString_allocBuffer(&encrypted, encryptedLength);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Copy the secret and the nonce into the output */
    UA_Byte *pos = encrypted.data;
    const UA_Byte *end = &encrypted.data[encrypted.length];
    res = UA_UInt32_encodeBinary(&length, &pos, end);
    memcpy(pos, tokenData->data, tokenData->length);
    memcpy(&pos[tokenData->length], serverSessionNonce.data,
           serverSessionNonce.length);
    UA_assert(res == UA_STATUSCODE_GOOD);

    /* Add padding
     *
     * 7.36.2.2 Legacy Encrypted Token Secret Format: A Client should not add
     * any padding after the secret. If a Client adds padding then all bytes
     * shall be zero. A Server shall check for padding added by Clients and
     * ensure that all padding bytes are zeros. */
    size_t paddedLength = plainTextBlockSize * blocks;
    for(size_t i = totalLength; i < paddedLength; i++)
        encrypted.data[i] = 0;
    encrypted.length = paddedLength;

    /* Encrypt */
    res = sp->asymEncryptionAlgorithm.encrypt(sp, spContext, &encrypted);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&encrypted);
        return res;
    }

    /* Replace the tokenData with the output */
    encrypted.length = encryptedLength;
    UA_ByteString_clear(tokenData);
    *tokenData = encrypted;
    return UA_STATUSCODE_GOOD;
}
