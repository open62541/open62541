/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Siemens AG (Author: Tin Raic)
 *    Copyright 2025-2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "ua_util_internal.h"

typedef struct {
    /* Common Header */
    UA_NodeId typeId;
    UA_Byte encodingMask;
    size_t length;
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
    UA_UInt32 length32 = 0;
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    ret |= UA_NodeId_decodeBinary(src, offset, &dest->typeId);
    ret |= UA_Byte_decodeBinary(src, offset, &dest->encodingMask);
    ret |= UA_UInt32_decodeBinary(src, offset, &length32);
    ret |= UA_String_decodeBinary(src, offset, &dest->securityPolicyUri);
    ret |= UA_ByteString_decodeBinary(src, offset, &dest->certificate);
    ret |= UA_DateTime_decodeBinary(src, offset, &dest->signingTime);
    ret |= UA_UInt16_decodeBinary(src, offset, &dest->keyDataLen);
    dest->length = length32;
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
encryptUserIdentityTokenEcc(UA_Logger *logger, UA_ByteString *tokenData,
                            const UA_ByteString serverSessionNonce,
                            const UA_ByteString serverEphemeralPubKey,
                            UA_SecurityPolicy *sp, void *tempChannelContext) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_ByteString sig;
    UA_ByteString_init(&sig);
    UA_ByteString payload;
    UA_ByteString_init(&payload);
    UA_ByteString symEncKeyMaterial;
    UA_ByteString_init(&symEncKeyMaterial);
    UA_EccEncryptedSecretStruct secret;
    UA_EccEncryptedSecretStruct_init(&secret);
    UA_EccEncryptedSecret eccEncSecSer;
    UA_EccEncryptedSecret_init(&eccEncSecSer);

    /* Filling out Common Header fields */
    secret.typeId = UA_NS0ID(ECCENCRYPTEDSECRET);
    secret.encodingMask = 0x01;
    secret.length = 0;

    retval = UA_String_copy(&sp->policyUri, &secret.securityPolicyUri);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to copy policy URI");
        goto cleanup;
    }
    secret.length += secret.securityPolicyUri.length;

    retval = UA_ByteString_copy(&sp->localCertificate, &secret.certificate);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to copy local certificate");
        goto cleanup;
    }
    secret.length += secret.certificate.length;

    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SESSION,
                 "[EncryptedSecret] Local certificate copied:");

    secret.signingTime = UA_DateTime_now();
    secret.length += sizeof(UA_DateTime);

    /* TODO: use proper policy functions */
    secret.keyDataLen = (UA_UInt16)sp->nonceLength; /* Also ephemeral public key length */
    secret.keyDataLen += (UA_UInt16)sp->nonceLength; /* Also ephemeral public key length */
    secret.keyDataLen += 8; /* 2x4 byte length fields  */
    if(secret.keyDataLen == 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] KeyData length shouldn't be 0");
        return UA_STATUSCODE_BADUNKNOWNRESPONSE;
    }
    secret.length += sizeof(UA_UInt16);

    /* Filling out Policy Header (KeyData) */
    retval = UA_ByteString_copy(&serverEphemeralPubKey, &secret.receiverPublicKey);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to copy server ephemeral key");
        goto cleanup;
    }
    secret.length += serverEphemeralPubKey.length;

    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SESSION,
                 "[EncryptedSecret] Server ephemeral key copied: ");

    /* TODO: use proper policy functions */
    size_t ephKeyLen = sp->nonceLength; /* Also length of the ephemeral public key */
    retval = UA_ByteString_allocBuffer(&secret.senderPublicKey, ephKeyLen);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to allocate buffer");
        goto cleanup;
    }

    retval = sp->generateNonce(sp, tempChannelContext, &secret.senderPublicKey);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to generate local ephemeral key");
        goto cleanup;
    }

    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SESSION,
                 "[EncryptedSecret] Client ephemeral key created and copied: ");

    /* Sanity check of KeyData length */
    if(secret.keyDataLen != (secret.senderPublicKey.length + secret.receiverPublicKey.length) ) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Individual key lengths "
                     "(%u and %u) don't add up to the total keyData length: (%u)",
                     secret.senderPublicKey.length, secret.receiverPublicKey.length,
                     secret.keyDataLen);
        retval = UA_STATUSCODE_BAD;
        goto cleanup;
    }

    secret.length += secret.receiverPublicKey.length;

    /* Preparing payload */
    retval = UA_ByteString_copy(&serverSessionNonce, &secret.nonce);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to copy server session nonce");
        goto cleanup;
    }

    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SESSION,
                 "[EncryptedSecret] Server session nonce copied: ");

    /* Creating symmetric encryption key to encrypt the payload */
    size_t symKeyLen =
        sp->symEncryptionAlgorithm.getLocalKeyLength(sp, tempChannelContext);
    size_t ivLen =
        sp->symEncryptionAlgorithm.getRemoteBlockSize(sp, tempChannelContext);
    if(symKeyLen == 0 || ivLen == 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] IV length or symmetric encryption key "
                     "length is 0");
        retval = UA_STATUSCODE_BAD;
        goto cleanup;
    }

    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SESSION,
                 "[EncryptedSecret] Local symmetric encrypting key length: %d",
                 symKeyLen);
    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SESSION,
                 "[EncryptedSecret] Initialization vector length: %d", ivLen);

    retval = UA_ByteString_allocBuffer(&symEncKeyMaterial, symKeyLen+ivLen);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to allocate buffer for key material");
        goto cleanup;
    }

    /* This is a (temporary) measure so that the salt generation function (for
     * the symmetric key derivation ) knows that the salt is generated for
     * session authentication (to choose the correct label) */
    /* TODO: find a better way to signal symmetric key generation for session
     * authentication */
    symEncKeyMaterial.data[0] = 0x03;
    symEncKeyMaterial.data[1] = 0x03;
    symEncKeyMaterial.data[2] = 0x04;

    /* Call logic for client (for session authentication): receiver public key
     * is remote (server), sender public key is local (client)*/
    retval = sp->generateKey(sp, tempChannelContext,
                             &secret.receiverPublicKey,
                             &secret.senderPublicKey,
                             &symEncKeyMaterial);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to derive key material");
        goto cleanup;
    }

    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SESSION,
                 "[EncryptedSecret] Derived key material: ");

    /* Extracting the key and the initialization vector from the key material */
    UA_ByteString encKey = {symKeyLen, symEncKeyMaterial.data};
    UA_ByteString iv = {ivLen, &symEncKeyMaterial.data[symKeyLen]};
    retval = sp->setLocalSymEncryptingKey(sp, tempChannelContext, &encKey);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to set symmetric encryption key");
        goto cleanup;
    }

    retval = sp->setLocalSymIv(sp, tempChannelContext, &iv);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to set initialization vector");
        goto cleanup;
    }

    /* Copy the data that needs to be protected (e.g. password) */
    retval = UA_ByteString_copy(tokenData, &secret.secret);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to copy the secret");
        goto cleanup;
    }

    /* Compute the padding size and pad the payload */
    size_t paddingLen = ivLen - ((secret.nonce.length + tokenData->length + 2) % ivLen);
    if(tokenData->length + paddingLen < ivLen)
        paddingLen += ivLen;
    UA_Byte pad = paddingLen & 0xFF;
    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SESSION,
                 "[EncryptedSecret] Padding size: %zu; Pad byte: 0x%02x",
                 paddingLen, pad);

    /* Constructing payload, refer to
     * https://reference.opcfoundation.org/Core/Part4/v105/docs/7.41.2.3 */
    size_t payloadLen = secret.nonce.length + secret.secret.length + paddingLen + 2;
    retval = UA_ByteString_allocBuffer(&payload, payloadLen);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to alocate the buffer for payload");
        goto cleanup;
    }

    UA_Byte* bufPos = payload.data;
    UA_Byte* bufEnd = payload.data + payload.length;
    memcpy(bufPos, secret.nonce.data, secret.nonce.length);
    bufPos += secret.nonce.length;
    memcpy(bufPos, secret.secret.data, secret.secret.length);

    /* Encode Padding */
    bufPos += secret.secret.length;
    for(size_t i = 0; i < paddingLen; i++) {
        *bufPos = pad;
        bufPos++;
    }

    UA_UInt16 paddingLen16 = (UA_UInt16)paddingLen;
    retval |= UA_UInt16_encodeBinary(&paddingLen16, &bufPos, bufEnd);
    retval |= sp->symEncryptionAlgorithm.encrypt(sp, tempChannelContext, &payload);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to encrypt the payload");
        goto cleanup;
    }

    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SESSION,
                 "[EncryptedSecret] Encrypted payload (len: %u):", payload.length);

    secret.length += payload.length;

    /* Init the EccEncryptedSecret (serialized) and calculate the lengths*/
    size_t commonHeaderSerLen = UA_EccEncryptedSecret_getCommonHeaderSize(&secret);
    size_t signatureLen = sp->asymSignatureAlgorithm.
        getLocalSignatureSize(sp, tempChannelContext);
    size_t policyHeaderSerLen = UA_EccEncryptedSecret_getPolicyHeaderSize(&secret);
    size_t payloadSerLen = UA_ByteString_calcSizeBinary(&payload);
    size_t signedDataLen = commonHeaderSerLen + policyHeaderSerLen + payloadSerLen;
    size_t totalLen = signedDataLen + signatureLen;

    secret.length += signatureLen;

    /* Init the EccEncryptedSecret */
    retval = UA_ByteString_allocBuffer(&eccEncSecSer, totalLen);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to allocate the buffer");
        goto cleanup;
    }

    /* Serialize the Common Header and put it in EccEncryptedSecret */
    bufPos = eccEncSecSer.data;
    bufEnd = &eccEncSecSer.data[commonHeaderSerLen];
    retval = UA_EccEncryptedSecret_serializeCommonHeader(&secret, &bufPos, bufEnd);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to serialize the common header");
        goto cleanup;
    }

    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SESSION,
                 "[EncryptedSecret] After Common Header serialization:");

    /* Serialize the policy header (key data) */
    bufEnd = bufPos + policyHeaderSerLen;
    retval = UA_EccEncryptedSecret_serializePolicyHeader(&secret, &bufPos, bufEnd);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to serialize the policy header (key data)");
        goto cleanup;
    }

    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SESSION,
                 "[EncryptedSecret] After serializing the policy header (key data):");

    /* Serialize the payload */
    bufEnd = bufPos + payloadSerLen;
    retval = UA_ByteString_encodeBinary(&payload, &bufPos, bufEnd);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to serialize the payload");
        goto cleanup;
    }

    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SESSION,
                 "[EncryptedSecret] After serializing the payload:");

    /* Sign */
    retval = UA_ByteString_allocBuffer(&sig, signatureLen);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to allocate signature buffer");
        goto cleanup;
    }

    /* Temporarily change length for signing purposes */
    eccEncSecSer.length = signedDataLen;
    retval = sp->asymSignatureAlgorithm.sign(sp, tempChannelContext, &eccEncSecSer, &sig);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SESSION,
                     "[EncryptedSecret] Failed to compute the signature");
        goto cleanup;
    }
    eccEncSecSer.length = totalLen;

    /* Copy the signature */
    memcpy(bufPos, sig.data, sig.length);

    /* Set output */
    UA_ByteString_clear(tokenData);
    *tokenData = eccEncSecSer;

cleanup:
    UA_EccEncryptedSecretStruct_clear(&secret);
    UA_ByteString_clear(&symEncKeyMaterial);
    UA_ByteString_clear(&payload);
    UA_ByteString_clear(&sig);
    UA_ByteString_clear(&eccEncSecSer);
    return retval;
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
