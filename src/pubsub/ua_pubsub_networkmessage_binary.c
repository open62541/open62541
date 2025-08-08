/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Tino Bischoff)
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <open62541/types.h>

#include "ua_pubsub_networkmessage.h"
#include "../util/ua_util_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#define _DECODE_BINARY(VAR, TYPE)                                       \
    decodeBinaryJumpTable[UA_DATATYPEKIND_##TYPE](&ctx->ctx, VAR, NULL);
#define _ENCODE_BINARY(VAR, TYPE)                                       \
    encodeBinaryJumpTable[UA_DATATYPEKIND_##TYPE](&ctx->ctx, VAR, NULL);

#define NM_VERSION_MASK 15
#define NM_PUBLISHER_ID_ENABLED_MASK 16
#define NM_GROUP_HEADER_ENABLED_MASK 32
#define NM_PAYLOAD_HEADER_ENABLED_MASK 64
#define NM_EXTENDEDFLAGS1_ENABLED_MASK 128
#define NM_PUBLISHER_ID_MASK 7
#define NM_DATASET_CLASSID_ENABLED_MASK 8
#define NM_SECURITY_ENABLED_MASK 16
#define NM_TIMESTAMP_ENABLED_MASK 32
#define NM_PICOSECONDS_ENABLED_MASK 64
#define NM_EXTENDEDFLAGS2_ENABLED_MASK 128
#define NM_NETWORK_MSG_TYPE_MASK 28
#define NM_CHUNK_MESSAGE_MASK 1
#define NM_PROMOTEDFIELDS_ENABLED_MASK 2
#define GROUP_HEADER_WRITER_GROUPID_ENABLED 1
#define GROUP_HEADER_GROUP_VERSION_ENABLED 2
#define GROUP_HEADER_NM_NUMBER_ENABLED 4
#define GROUP_HEADER_SEQUENCE_NUMBER_ENABLED 8
#define SECURITY_HEADER_NM_SIGNED 1
#define SECURITY_HEADER_NM_ENCRYPTED 2
#define SECURITY_HEADER_SEC_FOOTER_ENABLED 4
#define SECURITY_HEADER_FORCE_KEY_RESET 8
#define DS_MESSAGEHEADER_DS_MSG_VALID 1
#define DS_MESSAGEHEADER_FIELD_ENCODING_MASK 6
#define DS_MESSAGEHEADER_SEQ_NR_ENABLED_MASK 8
#define DS_MESSAGEHEADER_STATUS_ENABLED_MASK 16
#define DS_MESSAGEHEADER_CONFIGMAJORVERSION_ENABLED_MASK 32
#define DS_MESSAGEHEADER_CONFIGMINORVERSION_ENABLED_MASK 64
#define DS_MESSAGEHEADER_FLAGS2_ENABLED_MASK 128
#define DS_MESSAGEHEADER_DS_MESSAGE_TYPE_MASK 15
#define DS_MESSAGEHEADER_TIMESTAMP_ENABLED_MASK 16
#define DS_MESSAGEHEADER_PICOSECONDS_INCLUDED_MASK 32
#define NM_SHIFT_LEN 2
#define DS_MH_SHIFT_LEN 1

static UA_Boolean UA_NetworkMessage_ExtendedFlags1Enabled(const UA_NetworkMessage* src);
static UA_Boolean UA_NetworkMessage_ExtendedFlags2Enabled(const UA_NetworkMessage* src);
static UA_Boolean UA_DataSetMessageHeader_DataSetFlags2Enabled(const UA_DataSetMessageHeader* src);

static UA_StatusCode
UA_NetworkMessageHeader_encodeBinary(PubSubEncodeCtx *ctx,
                                     const UA_NetworkMessage *src) {
    /* UADPVersion + UADP Flags */
    UA_Byte v = src->version;
    if(src->publisherIdEnabled)
        v |= NM_PUBLISHER_ID_ENABLED_MASK;

    if(src->groupHeaderEnabled)
        v |= NM_GROUP_HEADER_ENABLED_MASK;

    if(src->payloadHeaderEnabled)
        v |= NM_PAYLOAD_HEADER_ENABLED_MASK;

    if(UA_NetworkMessage_ExtendedFlags1Enabled(src))
        v |= NM_EXTENDEDFLAGS1_ENABLED_MASK;

    UA_StatusCode rv = _ENCODE_BINARY(&v, BYTE);
    UA_CHECK_STATUS(rv, return rv);
    /* ExtendedFlags1 */
    if(UA_NetworkMessage_ExtendedFlags1Enabled(src)) {
        v = (UA_Byte)src->publisherId.idType;

        if(src->dataSetClassIdEnabled)
            v |= NM_DATASET_CLASSID_ENABLED_MASK;

        if(src->securityEnabled)
            v |= NM_SECURITY_ENABLED_MASK;

        if(src->timestampEnabled)
            v |= NM_TIMESTAMP_ENABLED_MASK;

        if(src->picosecondsEnabled)
            v |= NM_PICOSECONDS_ENABLED_MASK;

        if(UA_NetworkMessage_ExtendedFlags2Enabled(src))
            v |= NM_EXTENDEDFLAGS2_ENABLED_MASK;

        rv = _ENCODE_BINARY(&v, BYTE);
        UA_CHECK_STATUS(rv, return rv);

        /* ExtendedFlags2 */
        if(UA_NetworkMessage_ExtendedFlags2Enabled(src)) {
            v = (UA_Byte)src->networkMessageType;
            /* Shift left 2 bit */
            v = (UA_Byte) (v << NM_SHIFT_LEN);

            if(src->chunkMessage)
                v |= NM_CHUNK_MESSAGE_MASK;

            if(src->promotedFieldsEnabled)
                v |= NM_PROMOTEDFIELDS_ENABLED_MASK;

            rv = _ENCODE_BINARY(&v, BYTE);
            UA_CHECK_STATUS(rv, return rv);
        }
    }

    /* PublisherId */
    if(src->publisherIdEnabled) {
        switch (src->publisherId.idType) {
        case UA_PUBLISHERIDTYPE_BYTE:
            rv = _ENCODE_BINARY(&src->publisherId.id.byte, BYTE); break;
        case UA_PUBLISHERIDTYPE_UINT16:
            rv = _ENCODE_BINARY(&src->publisherId.id.uint16, UINT16); break;
        case UA_PUBLISHERIDTYPE_UINT32:
            rv = _ENCODE_BINARY(&src->publisherId.id.uint32, UINT32); break;
        case UA_PUBLISHERIDTYPE_UINT64:
            rv = _ENCODE_BINARY(&src->publisherId.id.uint64, UINT64); break;
        case UA_PUBLISHERIDTYPE_STRING:
            rv = _ENCODE_BINARY(&src->publisherId.id.string, STRING); break;
        default:
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        UA_CHECK_STATUS(rv, return rv);
    }

    /* DataSetClassId */
    if(src->dataSetClassIdEnabled) {
        rv = _ENCODE_BINARY(&src->dataSetClassId, GUID);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_GroupHeader_encodeBinary(PubSubEncodeCtx *ctx, const UA_NetworkMessage* src) {
    UA_Byte v = 0;
    if(src->groupHeader.writerGroupIdEnabled)
        v |= GROUP_HEADER_WRITER_GROUPID_ENABLED;

    if(src->groupHeader.groupVersionEnabled)
        v |= GROUP_HEADER_GROUP_VERSION_ENABLED;

    if(src->groupHeader.networkMessageNumberEnabled)
        v |= GROUP_HEADER_NM_NUMBER_ENABLED;

    if(src->groupHeader.sequenceNumberEnabled)
        v |= GROUP_HEADER_SEQUENCE_NUMBER_ENABLED;

    UA_StatusCode rv = _ENCODE_BINARY(&v, BYTE);
    if(src->groupHeader.writerGroupIdEnabled)
        rv |= _ENCODE_BINARY(&src->groupHeader.writerGroupId, UINT16);

    if(src->groupHeader.groupVersionEnabled)
        rv |= _ENCODE_BINARY(&src->groupHeader.groupVersion, UINT32);

    if(src->groupHeader.networkMessageNumberEnabled)
        rv |= _ENCODE_BINARY(&src->groupHeader.networkMessageNumber, UINT16);

    if(src->groupHeader.sequenceNumberEnabled)
        rv |= _ENCODE_BINARY(&src->groupHeader.sequenceNumber, UINT16);

    return rv;
}

static UA_StatusCode
UA_PayloadHeader_encodeBinary(PubSubEncodeCtx *ctx, const UA_NetworkMessage* src) {
    if(src->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    UA_Byte count = src->messageCount;
    UA_StatusCode rv = _ENCODE_BINARY(&count, BYTE);
    for(UA_Byte i = 0; i < src->messageCount; i++) {
        rv |= _ENCODE_BINARY(&src->dataSetWriterIds[i], UINT16);
    }
    return rv;
}

static UA_StatusCode
UA_ExtendedNetworkMessageHeader_encodeBinary(PubSubEncodeCtx *ctx,
                                             const UA_NetworkMessage* src) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    if(src->timestampEnabled)
        rv |= _ENCODE_BINARY(&src->timestamp, DATETIME);

    if(src->picosecondsEnabled)
        rv |= _ENCODE_BINARY(&src->picoseconds, UINT16);

    if(src->promotedFieldsEnabled) {
        /* Size (calculate & encode) */
        UA_UInt16 pfSize = 0;
        for(UA_UInt16 i = 0; i < src->promotedFieldsSize; i++)
            pfSize = (UA_UInt16)(pfSize + UA_Variant_calcSizeBinary(&src->promotedFields[i]));
        rv |= _ENCODE_BINARY(&pfSize, UINT16);

        for(UA_UInt16 i = 0; i < src->promotedFieldsSize; i++)
            rv |= _ENCODE_BINARY(&src->promotedFields[i], VARIANT);
    }

    return rv;
}

static UA_StatusCode
UA_SecurityHeader_encodeBinary(PubSubEncodeCtx *ctx,
                               const UA_NetworkMessage* src) {
    /* SecurityFlags */
    UA_Byte v = 0;
    if(src->securityHeader.networkMessageSigned)
        v |= SECURITY_HEADER_NM_SIGNED;

    if(src->securityHeader.networkMessageEncrypted)
        v |= SECURITY_HEADER_NM_ENCRYPTED;

    if(src->securityHeader.securityFooterEnabled)
        v |= SECURITY_HEADER_SEC_FOOTER_ENABLED;

    if(src->securityHeader.forceKeyReset)
        v |= SECURITY_HEADER_FORCE_KEY_RESET;

    UA_StatusCode rv = _ENCODE_BINARY(&v, BYTE);

    /* SecurityTokenId */
    rv |= _ENCODE_BINARY(&src->securityHeader.securityTokenId, UINT32);

    /* NonceLength */
    UA_Byte nonceLength = (UA_Byte)src->securityHeader.messageNonceSize;
    rv |= _ENCODE_BINARY(&nonceLength, BYTE);

    /* MessageNonce */
    for(size_t i = 0; i < src->securityHeader.messageNonceSize; i++) {
        rv |= _ENCODE_BINARY(&src->securityHeader.messageNonce[i], BYTE);
    }

    /* SecurityFooterSize */
    if(src->securityHeader.securityFooterEnabled) {
        rv |= _ENCODE_BINARY(&src->securityHeader.securityFooterSize, UINT16);
    }

    return rv;
}

UA_StatusCode
UA_NetworkMessage_encodeHeaders(PubSubEncodeCtx *ctx,
                                const UA_NetworkMessage* src) {
    /* Message Header */
    UA_StatusCode rv = UA_NetworkMessageHeader_encodeBinary(ctx, src);

    /* Group Header */
    if(src->groupHeaderEnabled)
        rv |= UA_GroupHeader_encodeBinary(ctx, src);

    /* Payload Header */
    if(src->payloadHeaderEnabled)
        rv |= UA_PayloadHeader_encodeBinary(ctx, src);

    /* Extended Network Message Header */
    rv |= UA_ExtendedNetworkMessageHeader_encodeBinary(ctx, src);

    /* SecurityHeader */
    if(src->securityEnabled)
        rv |= UA_SecurityHeader_encodeBinary(ctx, src);

    return rv;
}

const UA_DataSetMessage_EncodingMetaData *
findEncodingMetaData(const UA_NetworkMessage_EncodingOptions *eo,
                     UA_UInt16 dsWriterId) {
    if(!eo)
        return NULL;
    for(size_t i = 0; i < eo->metaDataSize; i++) {
        if(eo->metaData[i].dataSetWriterId == dsWriterId)
            return &eo->metaData[i];
    }
    return NULL;
}

const UA_FieldMetaData *
getFieldMetaData(const UA_DataSetMessage_EncodingMetaData *emd,
                 size_t index) {
    if(!emd)
        return NULL;
    if(index > emd->fieldsSize)
        return NULL;
    return &emd->fields[index];
}

UA_StatusCode
UA_NetworkMessage_encodePayload(PubSubEncodeCtx *ctx,
                                const UA_NetworkMessage* src) {
    /* Only DataSet support so far */
    if(src->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    /* Encode the length of each DataSet */
    UA_Byte count = src->messageCount;
    UA_StatusCode rv;
    if(src->payloadHeaderEnabled && count > 1) {
        for(UA_Byte i = 0; i < count; i++) {
            UA_DataSetMessage *dsm = &src->payload.dataSetMessages[i];
            const UA_DataSetMessage_EncodingMetaData *emd =
                findEncodingMetaData(&ctx->eo, src->dataSetWriterIds[i]);
            UA_UInt16 sz = (UA_UInt16)UA_DataSetMessage_calcSizeBinary(ctx, emd, dsm, 0);
            rv = _ENCODE_BINARY(&sz, UINT16);
            UA_CHECK_STATUS(rv, return rv);
        }
    }

    /* Encode the DataSets  */
    for(UA_Byte i = 0; i < count; i++) {
        UA_DataSetMessage *dsm = &src->payload.dataSetMessages[i];
        const UA_DataSetMessage_EncodingMetaData *emd =
            findEncodingMetaData(&ctx->eo, src->dataSetWriterIds[i]);
        rv = UA_DataSetMessage_encodeBinary(ctx, emd, dsm);
        UA_CHECK_STATUS(rv, return rv);
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NetworkMessage_encodeFooters(PubSubEncodeCtx *ctx,
                                const UA_NetworkMessage* src) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    if(src->securityEnabled &&
       src->securityHeader.securityFooterEnabled) {
        for(size_t i = 0; i < src->securityHeader.securityFooterSize; i++) {
            rv |= _ENCODE_BINARY(&src->securityFooter.data[i], BYTE);
        }
    }
    return rv;
}

UA_StatusCode
UA_NetworkMessage_encodeBinary(const UA_NetworkMessage* src,
                               UA_ByteString *outBuf,
                               const UA_NetworkMessage_EncodingOptions *eo) {
    /* Allocate the buffer */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_Boolean alloced = (outBuf->length == 0);
    if(alloced) {
        size_t length = UA_NetworkMessage_calcSizeBinary(src, eo);
        if(length == 0)
            return UA_STATUSCODE_BADENCODINGERROR;
        res = UA_ByteString_allocBuffer(outBuf, length);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    /* Encode the NetworkMessage */
    PubSubEncodeCtx ctx;
    memset(&ctx, 0, sizeof(PubSubEncodeCtx));
    ctx.ctx.pos = outBuf->data;
    ctx.ctx.end = outBuf->data + outBuf->length;
    if(eo)
        ctx.eo = *eo;
    res = UA_NetworkMessage_encodeBinaryWithEncryptStart(&ctx, src, NULL);

    /* In case the buffer was supplied externally and is longer than the encoded
     * string */
    if(UA_LIKELY(res == UA_STATUSCODE_GOOD))
        outBuf->length = (size_t)((uintptr_t)ctx.ctx.pos - (uintptr_t)outBuf->data);

    if(alloced && res != UA_STATUSCODE_GOOD)
        UA_ByteString_clear(outBuf);
    return res;
}

UA_StatusCode
UA_NetworkMessage_encodeBinaryWithEncryptStart(PubSubEncodeCtx *ctx,
                                               const UA_NetworkMessage* src,
                                               UA_Byte **dataToEncryptStart) {
    UA_StatusCode rv = UA_NetworkMessage_encodeHeaders(ctx, src);
    if(dataToEncryptStart)
        *dataToEncryptStart = ctx->ctx.pos;
    rv |= UA_NetworkMessage_encodePayload(ctx, src);
    rv |= UA_NetworkMessage_encodeFooters(ctx, src);
    return rv;
}

static UA_StatusCode
UA_NetworkMessageHeader_decodeBinary(PubSubDecodeCtx *ctx,
                                     UA_NetworkMessage *nm) {
    UA_Byte decoded;
    UA_StatusCode rv = _DECODE_BINARY(&decoded, BYTE);
    UA_CHECK_STATUS(rv, return rv);

    nm->version = decoded & NM_VERSION_MASK;

    if((decoded & NM_PUBLISHER_ID_ENABLED_MASK) != 0)
        nm->publisherIdEnabled = true;

    if((decoded & NM_GROUP_HEADER_ENABLED_MASK) != 0)
        nm->groupHeaderEnabled = true;

    if((decoded & NM_PAYLOAD_HEADER_ENABLED_MASK) != 0)
        nm->payloadHeaderEnabled = true;

    if((decoded & NM_EXTENDEDFLAGS1_ENABLED_MASK) != 0) {
        rv = _DECODE_BINARY(&decoded, BYTE);
        UA_CHECK_STATUS(rv, return rv);

        nm->publisherId.idType = (UA_PublisherIdType)(decoded & NM_PUBLISHER_ID_MASK);
        if((decoded & NM_DATASET_CLASSID_ENABLED_MASK) != 0)
            nm->dataSetClassIdEnabled = true;

        if((decoded & NM_SECURITY_ENABLED_MASK) != 0)
            nm->securityEnabled = true;

        if((decoded & NM_TIMESTAMP_ENABLED_MASK) != 0)
            nm->timestampEnabled = true;

        if((decoded & NM_PICOSECONDS_ENABLED_MASK) != 0)
            nm->picosecondsEnabled = true;

        if((decoded & NM_EXTENDEDFLAGS2_ENABLED_MASK) != 0) {
            rv = _DECODE_BINARY(&decoded, BYTE);
            UA_CHECK_STATUS(rv, return rv);

            if((decoded & NM_CHUNK_MESSAGE_MASK) != 0)
                nm->chunkMessage = true;

            if((decoded & NM_PROMOTEDFIELDS_ENABLED_MASK) != 0)
                nm->promotedFieldsEnabled = true;

            decoded = decoded & NM_NETWORK_MSG_TYPE_MASK;
            decoded = (UA_Byte) (decoded >> NM_SHIFT_LEN);
            nm->networkMessageType = (UA_NetworkMessageType)decoded;
        }
    }

    if(nm->publisherIdEnabled) {
        switch(nm->publisherId.idType) {
            case UA_PUBLISHERIDTYPE_BYTE:
                rv = _DECODE_BINARY(&nm->publisherId.id.byte, BYTE);
                break;
            case UA_PUBLISHERIDTYPE_UINT16:
                rv = _DECODE_BINARY(&nm->publisherId.id.uint16, UINT16);
                break;
            case UA_PUBLISHERIDTYPE_UINT32:
                rv = _DECODE_BINARY(&nm->publisherId.id.uint32, UINT32);
                break;
            case UA_PUBLISHERIDTYPE_UINT64:
                rv = _DECODE_BINARY(&nm->publisherId.id.uint64, UINT64);
                break;
            case UA_PUBLISHERIDTYPE_STRING:
                rv = _DECODE_BINARY(&nm->publisherId.id.string, STRING);
                break;
            default:
                rv = UA_STATUSCODE_BADINTERNALERROR;
                break;
        }
        UA_CHECK_STATUS(rv, return rv);
    }

    if(nm->dataSetClassIdEnabled) {
        rv = _DECODE_BINARY(&nm->dataSetClassId, GUID);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_GroupHeader_decodeBinary(PubSubDecodeCtx *ctx,
                            UA_NetworkMessage* nm) {
    UA_Byte decoded;
    UA_StatusCode rv = _DECODE_BINARY(&decoded, BYTE);
    UA_CHECK_STATUS(rv, return rv);

    if((decoded & GROUP_HEADER_WRITER_GROUPID_ENABLED) != 0) {
        nm->groupHeader.writerGroupIdEnabled = true;
        rv |= _DECODE_BINARY(&nm->groupHeader.writerGroupId, UINT16);
    }

    if((decoded & GROUP_HEADER_GROUP_VERSION_ENABLED) != 0) {
        nm->groupHeader.groupVersionEnabled = true;
        rv |= _DECODE_BINARY(&nm->groupHeader.groupVersion, UINT32);
    }

    if((decoded & GROUP_HEADER_NM_NUMBER_ENABLED) != 0) {
        nm->groupHeader.networkMessageNumberEnabled = true;
        rv |= _DECODE_BINARY(&nm->groupHeader.networkMessageNumber, UINT16);
    }

    if((decoded & GROUP_HEADER_SEQUENCE_NUMBER_ENABLED) != 0) {
        nm->groupHeader.sequenceNumberEnabled = true;
        rv |= _DECODE_BINARY(&nm->groupHeader.sequenceNumber, UINT16);
    }

    return rv;
}

static UA_StatusCode
UA_PayloadHeader_decodeBinary(PubSubDecodeCtx *ctx,
                              UA_NetworkMessage *nm) {
    if(nm->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    /* Decode the MessageCount */
    UA_Byte count;
    UA_StatusCode rv = _DECODE_BINARY(&count, BYTE);
    UA_CHECK_STATUS(rv, return rv);

    /* The NetworkMessage shall contain at least one DataSetMessage if the
     * NetworkMessage type is DataSetMessage payload. */
    if(count == 0)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Limit for the inline-defined DataSetWriterIds */
    if(count > UA_NETWORKMESSAGE_MAXMESSAGECOUNT)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Decode the DataSetWriterIds */
    for(UA_Byte i = 0; i < count; i++) {
        rv |= _DECODE_BINARY(&nm->dataSetWriterIds[i], UINT16);
    }
    UA_CHECK_STATUS(rv, return rv);

    /* Set the MessageCount */
    nm->messageCount = count;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_ExtendedNetworkMessageHeader_decodeBinary(PubSubDecodeCtx *ctx,
                                             UA_NetworkMessage* nm) {
    UA_StatusCode rv;

    /* Timestamp*/
    if(nm->timestampEnabled) {
        rv = _DECODE_BINARY(&nm->timestamp, DATETIME);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* Picoseconds */
    if(nm->picosecondsEnabled) {
        rv = _DECODE_BINARY(&nm->picoseconds, UINT16);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* PromotedFields */
    if(UA_LIKELY(!nm->promotedFieldsEnabled))
        return UA_STATUSCODE_GOOD;

    UA_UInt16 promotedFieldsLength; /* Size in bytes, not in number of fields */
    rv = _DECODE_BINARY(&promotedFieldsLength, UINT16);
    UA_CHECK_STATUS(rv, return rv);
    if(promotedFieldsLength == 0)
        return UA_STATUSCODE_GOOD;

    UA_Byte *endPos = ctx->ctx.pos + promotedFieldsLength;
    if(endPos > ctx->ctx.end)
        return UA_STATUSCODE_BADDECODINGERROR;
    
    size_t counter = 0;
    size_t space = 4;
    UA_Variant *pf = (UA_Variant*)
        ctxCalloc(&ctx->ctx, space, UA_TYPES[UA_TYPES_VARIANT].memSize);
    if(!pf)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    do {
        /* Increase the available space */
        if(counter == space) {
            UA_Variant *tmp = (UA_Variant*)
                ctxCalloc(&ctx->ctx, space << 1, UA_TYPES[UA_TYPES_VARIANT].memSize);
            if(!tmp) {
                if(!ctx->ctx.opts.calloc)
                    UA_Array_delete(pf, counter, &UA_TYPES[UA_TYPES_VARIANT]);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            memcpy(tmp, pf, space * UA_TYPES[UA_TYPES_VARIANT].memSize);
            ctxFree(&ctx->ctx, pf);
            pf = tmp;
            space = space << 1;
        }

        /* Decode the PromotedField */
        rv = _DECODE_BINARY(&pf[counter], VARIANT);
        if(rv != UA_STATUSCODE_GOOD) {
            if(!ctx->ctx.opts.calloc)
                UA_Array_delete(pf, counter, &UA_TYPES[UA_TYPES_VARIANT]);
            return rv;
        }

        counter++;
    } while(ctx->ctx.pos < endPos);

    nm->promotedFields = pf;
    nm->promotedFieldsSize = (UA_UInt16)counter;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_SecurityHeader_decodeBinary(PubSubDecodeCtx *ctx,
                               UA_NetworkMessage* nm) {
    /* SecurityFlags */
    UA_Byte decoded;
    UA_StatusCode rv = _DECODE_BINARY(&decoded, BYTE);
    UA_CHECK_STATUS(rv, return rv);

    if((decoded & SECURITY_HEADER_NM_SIGNED) != 0)
        nm->securityHeader.networkMessageSigned = true;

    if((decoded & SECURITY_HEADER_NM_ENCRYPTED) != 0)
        nm->securityHeader.networkMessageEncrypted = true;

    if((decoded & SECURITY_HEADER_SEC_FOOTER_ENABLED) != 0)
        nm->securityHeader.securityFooterEnabled = true;

    if((decoded & SECURITY_HEADER_FORCE_KEY_RESET) != 0)
        nm->securityHeader.forceKeyReset = true;

    /* SecurityTokenId */
    rv = _DECODE_BINARY(&nm->securityHeader.securityTokenId, UINT32);
    UA_CHECK_STATUS(rv, return rv);

    /* MessageNonce */
    UA_Byte nonceLength;
    rv = _DECODE_BINARY(&nonceLength, BYTE);
    UA_CHECK_STATUS(rv, return rv);

    if(nonceLength > UA_NETWORKMESSAGE_MAX_NONCE_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    if(nonceLength > 0) {
        nm->securityHeader.messageNonceSize = nonceLength;
        for(UA_Byte i = 0; i < nonceLength; i++) {
            rv = _DECODE_BINARY(&nm->securityHeader.messageNonce[i], BYTE);
            UA_CHECK_STATUS(rv, return rv);
        }
    }

    /* SecurityFooterSize */
    if(nm->securityHeader.securityFooterEnabled)
        rv = _DECODE_BINARY(&nm->securityHeader.securityFooterSize, UINT16);
    return rv;
}

UA_StatusCode
UA_NetworkMessage_decodeHeaders(PubSubDecodeCtx *ctx,
                                UA_NetworkMessage *nm) {
    UA_StatusCode rv = UA_NetworkMessageHeader_decodeBinary(ctx, nm);
    UA_CHECK_STATUS(rv, return rv);

    if(nm->groupHeaderEnabled) {
        rv = UA_GroupHeader_decodeBinary(ctx, nm);
        UA_CHECK_STATUS(rv, return rv);
    }

    if(nm->payloadHeaderEnabled) {
        rv = UA_PayloadHeader_decodeBinary(ctx, nm);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        /* If no PayloadHeader is defined, then assume the EncodingOptions
         * reflect the DataSetMessages */
        nm->messageCount = (UA_Byte)ctx->eo.metaDataSize;
        for(size_t i = 0; i < nm->messageCount; i++)
            nm->dataSetWriterIds[i] = ctx->eo.metaData[i].dataSetWriterId;

        /* No Metadata configured. Assume one DataSetMessage. The NetworkMessage
         * shall contain at least one DataSetMessage if the NetworkMessage type
         * is DataSetMessage payload. */
        if(nm->messageCount == 0)
            nm->messageCount = 1;
    }

    rv = UA_ExtendedNetworkMessageHeader_decodeBinary(ctx, nm);
    UA_CHECK_STATUS(rv, return rv);

    if(nm->securityEnabled)
        rv = UA_SecurityHeader_decodeBinary(ctx, nm);

    return rv;
}

UA_StatusCode
UA_NetworkMessage_decodePayload(PubSubDecodeCtx *ctx,
                                UA_NetworkMessage *nm) {
    /* Payload */
    if(nm->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    UA_assert(nm->messageCount > 0 &&
              nm->messageCount <= UA_NETWORKMESSAGE_MAXMESSAGECOUNT);

    /* Allocate the DataSetMessages */
    nm->payload.dataSetMessages = (UA_DataSetMessage*)
        ctxCalloc(&ctx->ctx, nm->messageCount, sizeof(UA_DataSetMessage));
    UA_CHECK_MEM(nm->payload.dataSetMessages,
                 return UA_STATUSCODE_BADOUTOFMEMORY);

    /* Get the payload sizes */
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    UA_UInt16 dataSetMessageSizes[UA_NETWORKMESSAGE_MAXMESSAGECOUNT];
    if(nm->messageCount == 1) {
        /* Not contained in the message, but can be inferred from the
         * remaining message length */
        UA_UInt16 size = (UA_UInt16)(ctx->ctx.end - ctx->ctx.pos);
        dataSetMessageSizes[0] = size;
    } else {
        if(nm->payloadHeaderEnabled) {
            /* Decode from the message */
            for(size_t i = 0; i < nm->messageCount; i++) {
                rv |= _DECODE_BINARY(&dataSetMessageSizes[i], UINT16);
                if(dataSetMessageSizes[i] == 0)
                    return UA_STATUSCODE_BADDECODINGERROR;
            }
            UA_CHECK_STATUS(rv, return rv);
        } else {
            /* If no PayloadHeader is defined, then assume the EncodingOptions
             * reflect the DataSetMessages */
            for(size_t i = 0; i < nm->messageCount; i++)
                dataSetMessageSizes[i] = ctx->eo.metaData[i].configuredSize;
        }
    }

    /* Decode the DataSetMessages */
    for(size_t i = 0; i < nm->messageCount; i++) {
        const UA_DataSetMessage_EncodingMetaData *emd =
            findEncodingMetaData(&ctx->eo, nm->dataSetWriterIds[i]);
        rv |= UA_DataSetMessage_decodeBinary(ctx, emd,
                                             &nm->payload.dataSetMessages[i],
                                             dataSetMessageSizes[i]);
    }

    return rv;
}

UA_StatusCode
UA_NetworkMessage_decodeFooters(PubSubDecodeCtx *ctx,
                                UA_NetworkMessage *nm) {
    if(!nm->securityEnabled)
        return UA_STATUSCODE_GOOD;

    if(!nm->securityHeader.securityFooterEnabled ||
       nm->securityHeader.securityFooterSize == 0)
        return UA_STATUSCODE_GOOD;
    
    UA_StatusCode rv = UA_ByteString_allocBuffer(&nm->securityFooter,
                                                 nm->securityHeader.securityFooterSize);
    UA_CHECK_STATUS(rv, return rv);
    
    for(UA_UInt16 i = 0; i < nm->securityHeader.securityFooterSize; i++) {
        rv |= _DECODE_BINARY(&nm->securityFooter.data[i], BYTE);
    }
    return rv;
}

UA_StatusCode
UA_NetworkMessage_decodeBinary(const UA_ByteString *src,
                               UA_NetworkMessage* nm,
                               const UA_NetworkMessage_EncodingOptions *eo,
                               const UA_DecodeBinaryOptions *bo) {
    /* Initialize the decoding context */
    PubSubDecodeCtx ctx;
    memset(&ctx, 0, sizeof(PubSubDecodeCtx));
    ctx.ctx.pos = src->data;
    ctx.ctx.end = &src->data[src->length];
    ctx.ctx.depth = 0;
    if(eo)
        ctx.eo = *eo;
    if(bo)
        ctx.ctx.opts = *bo;

    /* headers only need to be decoded when not in encryption mode
     * because headers are already decoded when encryption mode is enabled
     * to check for security parameters and decrypt/verify
     *
     * TODO: check if there is a workaround to use this function
     *       also when encryption is enabled
     */
    // #ifndef UA_ENABLE_PUBSUB_ENCRYPTION
    // if(*offset == 0) {
    //    rv = UA_NetworkMessage_decodeHeaders(src, offset, nm);
    //    UA_CHECK_STATUS(rv, return rv);
    // }
    // #endif

    /* Initialize the NetworkMessage */
    memset(nm, 0, sizeof(UA_NetworkMessage));

    UA_StatusCode rv = UA_NetworkMessage_decodeHeaders(&ctx, nm);
    UA_CHECK_STATUS(rv, goto cleanup);

    rv = UA_NetworkMessage_decodePayload(&ctx, nm);
    UA_CHECK_STATUS(rv, goto cleanup);

    rv = UA_NetworkMessage_decodeFooters(&ctx, nm);
    UA_CHECK_STATUS(rv, goto cleanup);

cleanup:
    if(rv != UA_STATUSCODE_GOOD && !ctx.ctx.opts.calloc)
        UA_NetworkMessage_clear(nm);

    return rv;
}

UA_StatusCode
UA_NetworkMessage_decodeBinaryHeaders(const UA_ByteString *src,
                                      UA_NetworkMessage *dst,
                                      const UA_NetworkMessage_EncodingOptions *eo,
                                      const UA_DecodeBinaryOptions *bo,
                                      size_t *payloadOffset) {
    /* Initialize the decoding context */
    PubSubDecodeCtx ctx;
    memset(&ctx, 0, sizeof(PubSubDecodeCtx));
    ctx.ctx.pos = src->data;
    ctx.ctx.end = &src->data[src->length];
    ctx.ctx.depth = 0;
    if(eo)
        ctx.eo = *eo;
    if(bo)
        ctx.ctx.opts = *bo;

    /* Initialize the NetworkMessage */
    memset(dst, 0, sizeof(UA_NetworkMessage));

    /* Decode the headers */
    UA_StatusCode rv = UA_NetworkMessage_decodeHeaders(&ctx, dst);
    if(rv != UA_STATUSCODE_GOOD) {
        if(!ctx.ctx.opts.calloc)
            UA_NetworkMessage_clear(dst);
        return rv;
    }

    /* Set the offset */
    if(payloadOffset)
        *payloadOffset = (size_t)(ctx.ctx.pos - src->data);

    return rv;
}

static UA_Boolean
incrOffsetTable(UA_PubSubOffsetTable *ot) {
    UA_PubSubOffset *tmpOffsets = (UA_PubSubOffset *)
        UA_realloc(ot->offsets, sizeof(UA_PubSubOffset) * (ot->offsetsSize + 1));
    UA_CHECK_MEM(tmpOffsets, return false);
    memset(&tmpOffsets[ot->offsetsSize], 0, sizeof(UA_PubSubOffset));
    ot->offsets = tmpOffsets;
    ot->offsetsSize++;
    return true;
}

size_t
UA_NetworkMessage_calcSizeBinary(const UA_NetworkMessage *p,
                                 const UA_NetworkMessage_EncodingOptions *eo) {
    PubSubEncodeCtx ctx;
    memset(&ctx, 0, sizeof(PubSubEncodeCtx));
    if(eo)
        ctx.eo = *eo;
    return UA_NetworkMessage_calcSizeBinaryInternal(&ctx, p);
}

size_t
UA_NetworkMessage_calcSizeBinaryInternal(PubSubEncodeCtx *ctx,
                                         const UA_NetworkMessage *p) {
    UA_PubSubOffsetTable *ot = ctx->ot;

    size_t size = 1; /* byte */
    if(UA_NetworkMessage_ExtendedFlags1Enabled(p)) {
        size += 1; /* byte */
        if(UA_NetworkMessage_ExtendedFlags2Enabled(p))
            size += 1; /* byte */
    }

    if(p->publisherIdEnabled) {
        switch(p->publisherId.idType) {
            case UA_PUBLISHERIDTYPE_BYTE:
                size += 1; /* byte */
                break;
            case UA_PUBLISHERIDTYPE_UINT16:
                size += 2; /* uint16 */
                break;
            case UA_PUBLISHERIDTYPE_UINT32:
                size += 4; /* uint32 */
                break;
            case UA_PUBLISHERIDTYPE_UINT64:
                size += 8; /* uint64 */
                break;
            case UA_PUBLISHERIDTYPE_STRING:
                size += UA_String_calcSizeBinary(&p->publisherId.id.string);
                break;
        }
    }

    if(p->dataSetClassIdEnabled)
        size += 16; /* guid */

    /* Group Header */
    if(p->groupHeaderEnabled) {
        size += 1; /* byte */

        if(p->groupHeader.writerGroupIdEnabled)
            size += 2; /* UA_UInt16_calcSizeBinary(&p->groupHeader.writerGroupId) */

        if(p->groupHeader.groupVersionEnabled) {
            if(ot) {
                size_t pos = ot->offsetsSize;
                if(!incrOffsetTable(ot))
                    return 0;
                ot->offsets[pos].offset = size;
                ot->offsets[pos].offsetType =
                    UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_GROUPVERSION;
            }
            size += 4; /* UA_UInt32_calcSizeBinary(&p->groupHeader.groupVersion) */
        }

        if(p->groupHeader.networkMessageNumberEnabled) {
            size += 2; /* UA_UInt16_calcSizeBinary(&p->groupHeader.networkMessageNumber) */
        }

        if(p->groupHeader.sequenceNumberEnabled){
            if(ot){
                size_t pos = ot->offsetsSize;
                if(!incrOffsetTable(ot))
                    return 0;
                ot->offsets[pos].offset = size;
                ot->offsets[pos].offsetType =
                    UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_SEQUENCENUMBER;
            }
            size += 2; /* UA_UInt16_calcSizeBinary(&p->groupHeader.sequenceNumber) */
        }
    }

    /* Payload Header */
    if(p->payloadHeaderEnabled) {
        if(p->networkMessageType != UA_NETWORKMESSAGE_DATASET)
            return 0; /* not implemented */
        size += 1; /* p->messageCount */
        size += (size_t)(2LU * p->messageCount); /* uint16 */
    }

    if(p->timestampEnabled) {
        if(ot) {
            size_t pos = ot->offsetsSize;
            if(!incrOffsetTable(ot))
                return 0;
            ot->offsets[pos].offset = size;
            ot->offsets[pos].offsetType = UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_TIMESTAMP;
        }
        size += 8; /* UA_DateTime_calcSizeBinary(&p->timestamp) */
    }

    if(p->picosecondsEnabled){
        if(ot) {
            size_t pos = ot->offsetsSize;
            if(!incrOffsetTable(ot))
                return 0;
            ot->offsets[pos].offset = size;
            ot->offsets[pos].offsetType = UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_PICOSECONDS;
        }
        size += 2; /* UA_UInt16_calcSizeBinary(&p->picoseconds) */
    }

    if(p->promotedFieldsEnabled) {
        size += 2; /* UA_UInt16_calcSizeBinary(&p->promotedFieldsSize) */
        for(UA_UInt16 i = 0; i < p->promotedFieldsSize; i++)
            size += UA_Variant_calcSizeBinary(&p->promotedFields[i]);
    }

    if(p->securityEnabled) {
        size += 1; /* UA_Byte_calcSizeBinary(&byte) */
        size += 4; /* UA_UInt32_calcSizeBinary(&p->securityHeader.securityTokenId) */
        size += 1; /* UA_Byte_calcSizeBinary(&p->securityHeader.nonceLength) */
        size += p->securityHeader.messageNonceSize;
        if(p->securityHeader.securityFooterEnabled)
            size += 2; /* UA_UInt16_calcSizeBinary(&p->securityHeader.securityFooterSize) */
    }

    /* Encode the payload */
    if(p->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return 0; /* not implemented */

    UA_Byte count = p->messageCount;
    if(p->payloadHeaderEnabled && count > 1)
        size += (size_t)(2LU * count); /* DataSetMessagesSize (uint16) */
    for(size_t i = 0; i < count; i++) {
        /* Add the offset here and not inside UA_DataSetMessage_calcSizeBinary.
         * We don't want the offset marking the beginning of the DataSetMessage
         * if we compute the offsets of a single DataSetMessage (without looking
         * at the entire NetworkMessage). */
        if(ot) {
            size_t pos = ot->offsetsSize;
            if(!incrOffsetTable(ot))
                return 0;
            ot->offsets[pos].offset = size;
            ot->offsets[pos].offsetType = UA_PUBSUBOFFSETTYPE_DATASETMESSAGE;
        }

        /* size = ... as the original size is used as the starting point in
         * UA_DataSetMessage_calcSizeBinary */
        UA_DataSetMessage *dsm = &p->payload.dataSetMessages[i];
        const UA_DataSetMessage_EncodingMetaData *emd =
            findEncodingMetaData(&ctx->eo, p->dataSetWriterIds[i]);
        size = UA_DataSetMessage_calcSizeBinary(ctx, emd, dsm, size);
        if(size == 0)
            return 0;
    }

    if(p->securityEnabled && p->securityHeader.securityFooterEnabled)
        size += p->securityHeader.securityFooterSize;

    return size;
}

void
UA_NetworkMessage_clear(UA_NetworkMessage* p) {
    if(p->promotedFieldsEnabled) {
        UA_Array_delete(p->promotedFields, p->promotedFieldsSize,
                        &UA_TYPES[UA_TYPES_VARIANT]);
    }

    if(p->networkMessageType == UA_NETWORKMESSAGE_DATASET) {
        if(p->payload.dataSetMessages) {
            for(size_t i = 0; i < p->messageCount; i++)
                UA_DataSetMessage_clear(&p->payload.dataSetMessages[i]);
            UA_free(p->payload.dataSetMessages);
        }
    }

    UA_ByteString_clear(&p->securityFooter);
    UA_String_clear(&p->messageId);

    if(p->publisherIdEnabled &&
       p->publisherId.idType == UA_PUBLISHERIDTYPE_STRING)
       UA_String_clear(&p->publisherId.id.string);

    memset(p, 0, sizeof(UA_NetworkMessage));
}

UA_Boolean
UA_NetworkMessage_ExtendedFlags1Enabled(const UA_NetworkMessage* src) {
    if(src->publisherId.idType != UA_PUBLISHERIDTYPE_BYTE ||
       src->dataSetClassIdEnabled || src->securityEnabled ||
       src->timestampEnabled || src->picosecondsEnabled ||
       UA_NetworkMessage_ExtendedFlags2Enabled(src))
        return true;
    return false;
}

UA_Boolean
UA_NetworkMessage_ExtendedFlags2Enabled(const UA_NetworkMessage* src) {
    if(src->chunkMessage || src->promotedFieldsEnabled ||
       src->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return true;
    return false;
}

UA_Boolean
UA_DataSetMessageHeader_DataSetFlags2Enabled(const UA_DataSetMessageHeader* src) {
    if(src->dataSetMessageType != UA_DATASETMESSAGE_DATAKEYFRAME ||
       src->timestampEnabled || src->picoSecondsIncluded)
        return true;
    return false;
}

UA_StatusCode
UA_DataSetMessageHeader_encodeBinary(PubSubEncodeCtx *ctx,
                                     const UA_DataSetMessageHeader *src) {
    UA_Byte v;
    /* DataSetFlags1 */
    v = (UA_Byte)src->fieldEncoding;
    /* shift left 1 bit */
    v = (UA_Byte)(v << DS_MH_SHIFT_LEN);

    if(src->dataSetMessageValid)
        v |= DS_MESSAGEHEADER_DS_MSG_VALID;

    if(src->dataSetMessageSequenceNrEnabled)
        v |= DS_MESSAGEHEADER_SEQ_NR_ENABLED_MASK;

    if(src->statusEnabled)
        v |= DS_MESSAGEHEADER_STATUS_ENABLED_MASK;

    if(src->configVersionMajorVersionEnabled)
        v |= DS_MESSAGEHEADER_CONFIGMAJORVERSION_ENABLED_MASK;

    if(src->configVersionMinorVersionEnabled)
        v |= DS_MESSAGEHEADER_CONFIGMINORVERSION_ENABLED_MASK;

    if(UA_DataSetMessageHeader_DataSetFlags2Enabled(src))
        v |= DS_MESSAGEHEADER_FLAGS2_ENABLED_MASK;

    UA_StatusCode rv = _ENCODE_BINARY(&v, BYTE);
    UA_CHECK_STATUS(rv, return rv);

    /* DataSetFlags2 */
    if(UA_DataSetMessageHeader_DataSetFlags2Enabled(src)) {
        v = (UA_Byte)src->dataSetMessageType;

        if(src->timestampEnabled)
            v |= DS_MESSAGEHEADER_TIMESTAMP_ENABLED_MASK;

        if(src->picoSecondsIncluded)
            v |= DS_MESSAGEHEADER_PICOSECONDS_INCLUDED_MASK;

        rv = _ENCODE_BINARY(&v, BYTE);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* DataSetMessageSequenceNr */
    if(src->dataSetMessageSequenceNrEnabled) {
        rv = _ENCODE_BINARY(&src->dataSetMessageSequenceNr, UINT16);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* Timestamp */
    if(src->timestampEnabled) {
        rv = _ENCODE_BINARY(&src->timestamp, DATETIME);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* PicoSeconds */
    if(src->picoSecondsIncluded) {
        rv = _ENCODE_BINARY(&src->picoSeconds, UINT16);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* Status */
    if(src->statusEnabled) {
        rv = _ENCODE_BINARY(&src->status, UINT16);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* ConfigVersionMajorVersion */
    if(src->configVersionMajorVersionEnabled) {
        rv = _ENCODE_BINARY(&src->configVersionMajorVersion, UINT32);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* ConfigVersionMinorVersion */
    if(src->configVersionMinorVersionEnabled) {
        rv = _ENCODE_BINARY(&src->configVersionMinorVersion, UINT32);
        UA_CHECK_STATUS(rv, return rv);
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NetworkMessage_signEncrypt(UA_NetworkMessage *nm, UA_MessageSecurityMode securityMode,
                              UA_PubSubSecurityPolicy *policy, void *policyContext,
                              UA_Byte *messageStart, UA_Byte *encryptStart,
                              UA_Byte *sigStart) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    /* Encrypt the payload */
    if(securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        /* Set the temporary MessageNonce in the SecurityPolicy */
        const UA_ByteString nonce = {
            (size_t)nm->securityHeader.messageNonceSize,
            nm->securityHeader.messageNonce
        };
        res = policy->setMessageNonce(policyContext, &nonce);
        UA_CHECK_STATUS(res, return res);

        /* The encryption is done in-place, no need to encode again */
        UA_ByteString encryptBuf;
        encryptBuf.data = encryptStart;
        encryptBuf.length = (uintptr_t)sigStart - (uintptr_t)encryptStart;
        res = policy->symmetricModule.cryptoModule.encryptionAlgorithm.
            encrypt(policyContext, &encryptBuf);
        UA_CHECK_STATUS(res, return res);
    }

    /* Sign the entire message */
    if(securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        UA_ByteString sigBuf;
        sigBuf.length = (uintptr_t)sigStart - (uintptr_t)messageStart;
        sigBuf.data = messageStart;
        size_t sigSize = policy->symmetricModule.cryptoModule.
            signatureAlgorithm.getLocalSignatureSize(policyContext);
        UA_ByteString sig = {sigSize, sigStart};
        res = policy->symmetricModule.cryptoModule.
            signatureAlgorithm.sign(policyContext, &sigBuf, &sig);
    }

    return res;
}

UA_StatusCode
UA_DataSetMessageHeader_decodeBinary(PubSubDecodeCtx *ctx,
                                     UA_DataSetMessageHeader* dsmh) {
    UA_Byte v;
    UA_StatusCode rv = _DECODE_BINARY(&v, BYTE);
    UA_CHECK_STATUS(rv, return rv);

    UA_Byte v2 = v & DS_MESSAGEHEADER_FIELD_ENCODING_MASK;
    v2 = (UA_Byte)(v2 >> DS_MH_SHIFT_LEN);
    dsmh->fieldEncoding = (UA_FieldEncoding)v2;

    if((v & DS_MESSAGEHEADER_DS_MSG_VALID) != 0)
        dsmh->dataSetMessageValid = true;

    if((v & DS_MESSAGEHEADER_SEQ_NR_ENABLED_MASK) != 0)
        dsmh->dataSetMessageSequenceNrEnabled = true;

    if((v & DS_MESSAGEHEADER_STATUS_ENABLED_MASK) != 0)
        dsmh->statusEnabled = true;

    if((v & DS_MESSAGEHEADER_CONFIGMAJORVERSION_ENABLED_MASK) != 0)
        dsmh->configVersionMajorVersionEnabled = true;

    if((v & DS_MESSAGEHEADER_CONFIGMINORVERSION_ENABLED_MASK) != 0)
        dsmh->configVersionMinorVersionEnabled = true;

    if((v & DS_MESSAGEHEADER_FLAGS2_ENABLED_MASK) != 0) {
        rv = _DECODE_BINARY(&v, BYTE);
        UA_CHECK_STATUS(rv, return rv);

        dsmh->dataSetMessageType = (UA_DataSetMessageType)(v & DS_MESSAGEHEADER_DS_MESSAGE_TYPE_MASK);

        if((v & DS_MESSAGEHEADER_TIMESTAMP_ENABLED_MASK) != 0)
            dsmh->timestampEnabled = true;

        if((v & DS_MESSAGEHEADER_PICOSECONDS_INCLUDED_MASK) != 0)
            dsmh->picoSecondsIncluded = true;
    }
    /* The else-case is implied as dsmh is zeroed-out initially:
     * else {
     * dsmh->dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
     *   dsmh->picoSecondsIncluded = false;
     * } */

    if(dsmh->dataSetMessageSequenceNrEnabled) {
        rv = _DECODE_BINARY(&dsmh->dataSetMessageSequenceNr, UINT16);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dsmh->dataSetMessageSequenceNr = 0;
    }

    if(dsmh->timestampEnabled) {
        rv = _DECODE_BINARY(&dsmh->timestamp, DATETIME);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dsmh->timestamp = 0;
    }

    if(dsmh->picoSecondsIncluded) {
        rv = _DECODE_BINARY(&dsmh->picoSeconds, UINT16);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dsmh->picoSeconds = 0;
    }

    if(dsmh->statusEnabled) {
        rv = _DECODE_BINARY(&dsmh->status, UINT16);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dsmh->status = 0;
    }

    if(dsmh->configVersionMajorVersionEnabled) {
        rv = _DECODE_BINARY(&dsmh->configVersionMajorVersion, UINT32);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dsmh->configVersionMajorVersion = 0;
    }

    if(dsmh->configVersionMinorVersionEnabled) {
        rv = _DECODE_BINARY(&dsmh->configVersionMinorVersion, UINT32);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dsmh->configVersionMinorVersion = 0;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_DataSetMessage_keyFrame_rawScalar_encodeBinary(PubSubEncodeCtx *ctx,
                                                  const UA_FieldMetaData *fmd,
                                                  void *p, const UA_DataType *type) {
    /* TODO: Padding not yet supported for strings inside structures */
    UA_StatusCode rv = encodeBinaryJumpTable[type->typeKind](&ctx->ctx, p, type);
    UA_CHECK_STATUS(rv, return rv);
    if(fmd && fmd->maxStringLength != 0 &&
       (type->typeKind == UA_DATATYPEKIND_STRING ||
        type->typeKind == UA_DATATYPEKIND_BYTESTRING)) {
        UA_String *str = (UA_String *)p;
        if(str->length > fmd->maxStringLength)
            return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
        size_t padding = fmd->maxStringLength - str->length;
        memset(ctx->ctx.pos, 0, padding);
        ctx->ctx.pos += padding;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_DataSetMessage_keyFrame_raw_encodeBinary(PubSubEncodeCtx *ctx,
                                            const UA_FieldMetaData *fmd,
                                            const UA_Variant *v) {
    if(!v->type)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Scalar encoding */
    if(UA_Variant_isScalar(v))
        return UA_DataSetMessage_keyFrame_rawScalar_encodeBinary(ctx, fmd, v->data, v->type);

    /* Set up the Arraydimensions of the value.
     * No defined ArrayDimensions -> use one-dimensional */
    UA_UInt32 tmpDim;
    UA_UInt32 *arrayDims = v->arrayDimensions;
    size_t arrayDimsSize = v->arrayDimensionsSize;
    if(!arrayDims) {
        tmpDim = (UA_UInt32)v->arrayLength;
        arrayDims = &tmpDim;
        arrayDimsSize = 1;
    }

    /* Verify the value-arraydimensions match the fmd->arraydimensions.
     * TODO: Fill up with padding when the content is lacking. */
    if(fmd && fmd->arrayDimensionsSize > 0) {
        if(fmd->arrayDimensionsSize != arrayDimsSize)
            return UA_STATUSCODE_BADENCODINGERROR;
        for(size_t i = 0; i < arrayDimsSize; i++) {
            if(arrayDims[i] != fmd->arrayDimensions[i])
                return UA_STATUSCODE_BADENCODINGERROR;
        }
    }

    /* For arrays encode the dimension sizes before the actual data */
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < arrayDimsSize; i++) {
        rv |= _ENCODE_BINARY(&arrayDims[i], UINT32);
    }
    UA_CHECK_STATUS(rv, return rv);

    /* Encode the array of values */
    uintptr_t valuePtr = (uintptr_t)v->data;
    for(size_t i = 0; i < v->arrayLength; i++) {
        rv = UA_DataSetMessage_keyFrame_rawScalar_encodeBinary(ctx, fmd,
                                                               (void*)valuePtr, v->type);
        UA_CHECK_STATUS(rv, return rv);
        valuePtr += v->type->memSize;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_DataSetMessage_keyFrame_encodeBinary(PubSubEncodeCtx *ctx,
                                        const UA_DataSetMessage_EncodingMetaData *emd,
                                        const UA_DataSetMessage *src) {
    /* Heartbeat: "DataSetMessage is a key frame that only contains header
     * information" */
    if(src->fieldCount == 0)
        return UA_STATUSCODE_GOOD;

    /* Part 14: The FieldCount shall be omitted if RawData field encoding is set */
    UA_StatusCode rv;
    if(src->header.fieldEncoding != UA_FIELDENCODING_RAWDATA) {
        rv = _ENCODE_BINARY(&src->fieldCount, UINT16);
        UA_CHECK_STATUS(rv, return rv);
    }
    
    for(UA_UInt16 i = 0; i < src->fieldCount; i++) {
        const UA_DataValue *v = &src->data.keyFrameFields[i];
        if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
            rv = _ENCODE_BINARY(&v->value, VARIANT);
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
            rv = _ENCODE_BINARY(v, DATAVALUE);
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
            const UA_FieldMetaData *fmd = getFieldMetaData(emd, i);
            rv = UA_DataSetMessage_keyFrame_raw_encodeBinary(ctx, fmd, &v->value);
        } else {
            rv = UA_STATUSCODE_BADENCODINGERROR;
        }
        UA_CHECK_STATUS(rv, return rv);
    }
    return rv;
}

static UA_StatusCode
UA_DataSetMessage_deltaFrame_encodeBinary(PubSubEncodeCtx *ctx,
                                          const UA_DataSetMessage* src) {
    if(src->header.fieldEncoding == UA_FIELDENCODING_RAWDATA)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    /* Here the FieldCount is always present */
    UA_StatusCode rv = _ENCODE_BINARY(&src->fieldCount, UINT16);
    if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
        for(UA_UInt16 i = 0; i < src->fieldCount; i++) {
            rv |= _ENCODE_BINARY(&src->data.deltaFrameFields[i].index, UINT16);
            rv |= _ENCODE_BINARY(&src->data.deltaFrameFields[i].value.value, VARIANT);
        }
    } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
        for(UA_UInt16 i = 0; i < src->fieldCount; i++) {
            rv |= _ENCODE_BINARY(&src->data.deltaFrameFields[i].index, UINT16);
            rv |= _ENCODE_BINARY(&src->data.deltaFrameFields[i].value, DATAVALUE);
        }
    }
    return rv;
}

UA_StatusCode
UA_DataSetMessage_encodeBinary(PubSubEncodeCtx *ctx,
                               const UA_DataSetMessage_EncodingMetaData *emd,
                               const UA_DataSetMessage* src) {
    /* Store the beginning */
    UA_Byte *begin = ctx->ctx.pos;
    
    /* Encode Header */
    UA_StatusCode rv = UA_DataSetMessageHeader_encodeBinary(ctx, &src->header);
    UA_CHECK_STATUS(rv, return rv);

    /* Encode Payload */
    switch(src->header.dataSetMessageType) {
    case UA_DATASETMESSAGE_DATAKEYFRAME:
        rv = UA_DataSetMessage_keyFrame_encodeBinary(ctx, emd, src);
        break;
    case UA_DATASETMESSAGE_DATADELTAFRAME:
        rv = UA_DataSetMessage_deltaFrame_encodeBinary(ctx, src);
        break;
    case UA_DATASETMESSAGE_KEEPALIVE:
        break; /* Keep-Alive Message contains no Payload Data */
    default:
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    /* Zero-Padding according to a ConfiguredSize */
    if(emd && emd->configuredSize > 0 && src->header.dataSetMessageValid) {
        UA_Byte *configuredEnd = begin + emd->configuredSize;
        if(configuredEnd > ctx->ctx.end)
            return UA_STATUSCODE_BADENCODINGERROR;
        if(configuredEnd > ctx->ctx.pos) {
            size_t padding = (size_t)(configuredEnd - ctx->ctx.pos);
            memset(ctx->ctx.pos, 0, padding);
            ctx->ctx.pos += padding;
        }
    }
    UA_CHECK_STATUS(rv, return rv);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
decodeRawField(PubSubDecodeCtx *ctx,
               const UA_FieldMetaData *fmd,
               UA_DataValue *value) {
    if(!fmd)
        return UA_STATUSCODE_BADDECODINGERROR;
    const UA_DataType *type =
        UA_findDataTypeWithCustom(&fmd->dataType, ctx->ctx.opts.customTypes);
    if(!type) {
        if(fmd->builtInType == 0 ||
           fmd->builtInType > UA_DATATYPEKIND_DIAGNOSTICINFO + 1)
            return UA_STATUSCODE_BADDECODINGERROR;
        type = &UA_TYPES[fmd->builtInType - 1];
    }

    /* The ValueRank must be scalar or a defined dimensionality */
    if(fmd->valueRank < -1 || fmd->valueRank == 0)
        return UA_STATUSCODE_BADDECODINGERROR;

    value->hasValue = true;
    value->value.type = type;

    /* Scalar */
    if(fmd->valueRank == -1) {
        value->value.data = ctxCalloc(&ctx->ctx, 1, type->memSize);
        if(!value->value.data)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        return decodeBinaryJumpTable[type->typeKind](&ctx->ctx, value->value.data, type);
    }

    /* Decode the ArrayDimensions */
    value->value.arrayDimensions = (UA_UInt32*)
        ctxCalloc(&ctx->ctx, fmd->arrayDimensionsSize, sizeof(UA_UInt32));
    if(!value->value.arrayDimensions)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    value->value.arrayDimensionsSize = fmd->arrayDimensionsSize;

    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < fmd->arrayDimensionsSize; i++) {
        rv |= _DECODE_BINARY(&value->value.arrayDimensions[i], UINT32);
    }
    UA_CHECK_STATUS(rv, return rv);

    /* Validate the ArrayDimensions and prepare the total count */
    size_t count = 1;
    for(size_t i = 0; i < fmd->arrayDimensionsSize; i++) {
        if(value->value.arrayDimensions[i] == 0 ||
           value->value.arrayDimensions[i] > fmd->arrayDimensions[i])
            return UA_STATUSCODE_BADDECODINGERROR;
        count *= value->value.arrayDimensions[i];
    }

    /* Allocate the array memory */
    value->value.data = ctxCalloc(&ctx->ctx, count, type->memSize);
    if(!value->value.data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Decode the content */
    uintptr_t val = (uintptr_t)value->value.data;
    for(size_t i = 0; i < count; i++) {
        rv |= decodeBinaryJumpTable[type->typeKind](&ctx->ctx, (void*)val, type);
        val += type->memSize;
    }

    return rv;
}

static UA_StatusCode
UA_DataSetMessage_keyFrame_decodeBinary(PubSubDecodeCtx *ctx,
                                        const UA_DataSetMessage_EncodingMetaData *emd,
                                        UA_DataSetMessage *dsm) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;

    /* Part 14: The FieldCount shall be omitted if RawData field encoding is set */
    if(dsm->header.fieldEncoding != UA_FIELDENCODING_RAWDATA) {
        rv = _DECODE_BINARY(&dsm->fieldCount, UINT16);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        if(!emd)
            return UA_STATUSCODE_BADDECODINGERROR;
        dsm->fieldCount = (UA_UInt16)emd->fieldsSize;
    }

    if(dsm->fieldCount == 0)
        return UA_STATUSCODE_GOOD; /* Heartbeat */

    dsm->data.keyFrameFields = (UA_DataValue *)
        ctxCalloc(&ctx->ctx, dsm->fieldCount, sizeof(UA_DataValue));
    if(!dsm->data.keyFrameFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    switch(dsm->header.fieldEncoding) {
    case UA_FIELDENCODING_VARIANT:
        for(UA_UInt16 i = 0; i < dsm->fieldCount; i++) {
            rv = _DECODE_BINARY(&dsm->data.keyFrameFields[i].value, VARIANT);
            UA_CHECK_STATUS(rv, return rv);
            dsm->data.keyFrameFields[i].hasValue = true;
            if(dsm->header.timestampEnabled) {
                /*Since the variant field has no timestamp of its own, use dsm (DataSetMessage) timestamp */
                dsm->data.keyFrameFields[i].hasSourceTimestamp = true;
                dsm->data.keyFrameFields[i].sourceTimestamp = dsm->header.timestamp;
            }
        }
        break;

    case UA_FIELDENCODING_DATAVALUE:
        for(UA_UInt16 i = 0; i < dsm->fieldCount; i++) {
            rv = _DECODE_BINARY(&dsm->data.keyFrameFields[i], DATAVALUE);
            UA_CHECK_STATUS(rv, return rv);
        }
        break;

    case UA_FIELDENCODING_RAWDATA: {
        for(UA_UInt16 i = 0; i < dsm->fieldCount; i++) {
            const UA_FieldMetaData *fmd = getFieldMetaData(emd, i);
            rv = decodeRawField(ctx, fmd, &dsm->data.keyFrameFields[i]);
            UA_CHECK_STATUS(rv, return rv);
            if(dsm->header.timestampEnabled) {
                /*Since the raw data field has no timestamp of its own, use dsm (DataSetMessage) timestamp */
                dsm->data.keyFrameFields[i].hasSourceTimestamp = true;
                dsm->data.keyFrameFields[i].sourceTimestamp = dsm->header.timestamp;
            }
        }
        break;
    }

    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return rv;
}

static UA_StatusCode
UA_DataSetMessage_deltaFrame_decodeBinary(PubSubDecodeCtx *ctx,
                                          UA_DataSetMessage *dsm) {
    if(dsm->header.fieldEncoding == UA_FIELDENCODING_RAWDATA)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    if(dsm->header.fieldEncoding != UA_FIELDENCODING_VARIANT &&
       dsm->header.fieldEncoding != UA_FIELDENCODING_DATAVALUE)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode rv = _DECODE_BINARY(&dsm->fieldCount, UINT16);
    UA_CHECK_STATUS(rv, return rv);

    dsm->data.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField *)
        ctxCalloc(&ctx->ctx, dsm->fieldCount, sizeof(UA_DataSetMessage_DeltaFrameField));
    if(!dsm->data.deltaFrameFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    for(UA_UInt16 i = 0; i < dsm->fieldCount; i++) {
        rv = _DECODE_BINARY(&dsm->data.deltaFrameFields[i].index, UINT16);
        UA_CHECK_STATUS(rv, return rv);
        if(dsm->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
            rv = _DECODE_BINARY(&dsm->data.deltaFrameFields[i].value.value, VARIANT);
            UA_CHECK_STATUS(rv, return rv);
            dsm->data.deltaFrameFields[i].value.hasValue = true;
        } else {
            rv = _DECODE_BINARY(&dsm->data.deltaFrameFields[i].value, DATAVALUE);
            UA_CHECK_STATUS(rv, return rv);
        }
    }

    return rv;
}

UA_StatusCode
UA_DataSetMessage_decodeBinary(PubSubDecodeCtx *ctx,
                               const UA_DataSetMessage_EncodingMetaData *em,
                               UA_DataSetMessage *dsm,
                               size_t dsmSize) {
    UA_Byte *begin = ctx->ctx.pos;
    UA_StatusCode rv = UA_DataSetMessageHeader_decodeBinary(ctx, &dsm->header);
    UA_CHECK_STATUS(rv, return rv);
    switch(dsm->header.dataSetMessageType) {
    case UA_DATASETMESSAGE_DATAKEYFRAME:
        rv = UA_DataSetMessage_keyFrame_decodeBinary(ctx, em, dsm);
        break;
    case UA_DATASETMESSAGE_DATADELTAFRAME:
        rv = UA_DataSetMessage_deltaFrame_decodeBinary(ctx, dsm);
        break;
    case UA_DATASETMESSAGE_KEEPALIVE:
        return UA_STATUSCODE_GOOD; /* Keep-Alive Message contains no Payload Data */
    default:
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    /* If the message could not be decoded (e.g. no metadata for the raw
     * encoding), but technically the message is not fauly, jump to the next
     * dsm. */
    if(!dsm->header.dataSetMessageValid) {
        if(dsmSize == 0) /* Only possible if the size is known */
            return UA_STATUSCODE_BADDECODINGERROR;
        ctx->ctx.pos = begin + dsmSize;
    }
    return rv;
}

static size_t
UA_DataSetMessage_rawScalar_calcSizeBinary(void *p, const UA_DataType *type,
                                           const UA_FieldMetaData *fmd,
                                           size_t size) {
    /* TODO: Padding not yet supported for strings inside structures */
    size += UA_calcSizeBinary(p, type, NULL);
    if(fmd && fmd->maxStringLength != 0 &&
       (type->typeKind == UA_DATATYPEKIND_STRING ||
        type->typeKind == UA_DATATYPEKIND_BYTESTRING)) {
        UA_String *str = (UA_String *)p;
        if(str->length > fmd->maxStringLength)
            return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
        size_t padding = fmd->maxStringLength - str->length;
        size += padding;
    }
    return size;
}

static size_t
UA_DataSetMessage_raw_calcSizeBinary(const UA_Variant *v, const UA_FieldMetaData *fmd,
                                     UA_PubSubOffsetTable *ot, size_t size) {
    if(!v->type || !fmd)
        return 0;

    /* Scalar encoding */
    if(UA_Variant_isScalar(v))
        return UA_DataSetMessage_rawScalar_calcSizeBinary(v->data, v->type, fmd, size);

    /* Set up the Arraydimensions of the value.
     * No defined ArrayDimensions -> use one-dimensional */
    UA_UInt32 tmpDim;
    UA_UInt32 *arrayDims = v->arrayDimensions;
    size_t arrayDimsSize = v->arrayDimensionsSize;
    if(!arrayDims) {
        tmpDim = (UA_UInt32)v->arrayLength;
        arrayDims = &tmpDim;
        arrayDimsSize = 1;
    }

    /* Verify the value-arraydimensions match the fmd->arraydimensions.
     * TODO: Fill up with padding when the content is lacking. */
    if(fmd->arrayDimensionsSize > 0) {
        if(fmd->arrayDimensionsSize != arrayDimsSize)
            return 0;
        for(size_t i = 0; i < arrayDimsSize; i++) {
            if(arrayDims[i] != fmd->arrayDimensions[i])
                return 0;
        }
    }

    /* For arrays add the dimension sizes before the actual data */
    size += arrayDimsSize * sizeof(UA_UInt32);
    if(ot) {
        /* Start the offset at beginning of the payload, after the dimensions */
        UA_PubSubOffset *offset = &ot->offsets[ot->offsetsSize-1];
        offset->offset += arrayDimsSize * sizeof(UA_UInt32);
    }

    /* Add the array of values */
    uintptr_t valuePtr = (uintptr_t)v->data;
    for(size_t i = 0; i < v->arrayLength; i++) {
        size = UA_DataSetMessage_rawScalar_calcSizeBinary((void*)valuePtr, v->type, fmd, size);
        if(size == 0)
            return 0;
        valuePtr += v->type->memSize;
    }
    return size;
}

size_t
UA_DataSetMessage_calcSizeBinary(PubSubEncodeCtx *ctx,
                                 const UA_DataSetMessage_EncodingMetaData *emd,
                                 UA_DataSetMessage *p,
                                 size_t size) {
    UA_PubSubOffsetTable *ot = ctx->ot;

    size += 1; /* byte: DataSetMessage Type + Flags */
    if(UA_DataSetMessageHeader_DataSetFlags2Enabled(&p->header))
        size += 1; /* byte */

    if(p->header.dataSetMessageSequenceNrEnabled) {
        if(ot) {
            size_t pos = ot->offsetsSize;
            if(!incrOffsetTable(ot))
                return 0;
            ot->offsets[pos].offset = size;
            ot->offsets[pos].offsetType = UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_SEQUENCENUMBER;
        }
        size += 2; /* UA_UInt16_calcSizeBinary(&p->header.dataSetMessageSequenceNr) */
    }

    if(p->header.timestampEnabled) {
        if(ot) {
            size_t pos = ot->offsetsSize;
            if(!incrOffsetTable(ot))
                return 0;
            ot->offsets[pos].offset = size;
            ot->offsets[pos].offsetType = UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_TIMESTAMP;
        }
        size += 8; /* UA_DateTime_calcSizeBinary(&p->header.timestamp) */
    }

    if(p->header.picoSecondsIncluded) {
        if(ot) {
            size_t pos = ot->offsetsSize;
            if(!incrOffsetTable(ot))
                return 0;
            ot->offsets[pos].offset = size;
            ot->offsets[pos].offsetType = UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_PICOSECONDS;
        }
        size += 2; /* UA_UInt16_calcSizeBinary(&p->header.picoSeconds) */
    }

    if(p->header.statusEnabled) {
        if(ot) {
            size_t pos = ot->offsetsSize;
            if(!incrOffsetTable(ot))
                return 0;
            ot->offsets[pos].offset = size;
            ot->offsets[pos].offsetType = UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_STATUS;
        }
        size += 2; /* UA_UInt16_calcSizeBinary(&p->header.status) */
    }

    if(p->header.configVersionMajorVersionEnabled)
        size += 4; /* UA_UInt32_calcSizeBinary(&p->header.configVersionMajorVersion) */

    if(p->header.configVersionMinorVersionEnabled)
        size += 4; /* UA_UInt32_calcSizeBinary(&p->header.configVersionMinorVersion) */

    /* Keyframe with no fields is a heartbeat */
    if(p->header.dataSetMessageType == UA_DATASETMESSAGE_KEEPALIVE ||
       (p->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME && p->fieldCount == 0))
        return size;

    if(p->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
        if(p->header.fieldEncoding != UA_FIELDENCODING_RAWDATA)
            size += 2; /* p->data.keyFrameData.fieldCount */

        for(UA_UInt16 i = 0; i < p->fieldCount; i++){
            if(!p->data.keyFrameFields)
                return 0;
            UA_PubSubOffset *offset = NULL;
            const UA_DataValue *v = &p->data.keyFrameFields[i];
            if(ot) {
                size_t pos = ot->offsetsSize;
                if(!incrOffsetTable(ot))
                    return 0;
                offset = &ot->offsets[pos];
                offset->offset = size;
                if(p->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
                    offset->offsetType = UA_PUBSUBOFFSETTYPE_DATASETFIELD_VARIANT;
                } else if(p->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
                    if(!v->value.type || !v->value.type->pointerFree)
                        return 0; /* only integer types for now */
                    offset->offsetType = UA_PUBSUBOFFSETTYPE_DATASETFIELD_RAW;
                } else if(p->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
                    offset->offsetType = UA_PUBSUBOFFSETTYPE_DATASETFIELD_DATAVALUE;
                }
            }

            if(p->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
                size += UA_calcSizeBinary(&v->value, &UA_TYPES[UA_TYPES_VARIANT], NULL);
            } else if(p->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
                const UA_FieldMetaData *fmd = getFieldMetaData(emd, i);
                size = UA_DataSetMessage_raw_calcSizeBinary(&v->value, fmd, ot, size);
            } else if(p->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
                size += UA_calcSizeBinary(v, &UA_TYPES[UA_TYPES_DATAVALUE], NULL);
            } else {
                return 0;
            }
        }
    } else if(p->header.dataSetMessageType == UA_DATASETMESSAGE_DATADELTAFRAME) {
        if(ot)
            return 0; /* Not supported for RT */

        if(p->header.fieldEncoding == UA_FIELDENCODING_RAWDATA)
            return 0; /* not supported */

        size += 2; /* p->data.deltaFrameData.fieldCount */
        size += (size_t)(2LU * p->fieldCount); /* index per field */

        for(UA_UInt16 i = 0; i < p->fieldCount; i++) {
            const UA_DataValue *v = &p->data.deltaFrameFields[i].value;
            if(p->header.fieldEncoding == UA_FIELDENCODING_VARIANT)
                size += UA_calcSizeBinary(&v->value, &UA_TYPES[UA_TYPES_VARIANT], NULL);
            else if(p->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE)
                size += UA_calcSizeBinary(v, &UA_TYPES[UA_TYPES_DATAVALUE], NULL);
        }
    } else {
        /* Unknown message type */
        return 0;
    }

    /* Minimum message size configured.
     * If the message is larger than the configuredSize, it shall be set to not valid. */
    if(emd && emd->configuredSize > 0) {
        if(emd->configuredSize < size) 
            p->header.dataSetMessageValid = false;
        size = emd->configuredSize;
    }
    
    return size;
}

void
UA_DataSetMessage_clear(UA_DataSetMessage* p) {
    if(p->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
        if(p->data.keyFrameFields)
            UA_Array_delete(p->data.keyFrameFields, p->fieldCount,
                            &UA_TYPES[UA_TYPES_DATAVALUE]);
    } else if(p->header.dataSetMessageType == UA_DATASETMESSAGE_DATADELTAFRAME) {
        if(p->data.deltaFrameFields) {
            for(UA_UInt16 i = 0; i < p->fieldCount; i++)
                UA_DataValue_clear(&p->data.deltaFrameFields[i].value);
            UA_free(p->data.deltaFrameFields);
        }
    }

    memset(p, 0, sizeof(UA_DataSetMessage));
}

#endif /* UA_ENABLE_PUBSUB */
