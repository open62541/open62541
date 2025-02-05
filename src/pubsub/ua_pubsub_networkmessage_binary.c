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

typedef struct {
    u8 *pos;
    const u8 *end;
} EncodeCtx;

static UA_Boolean UA_NetworkMessage_ExtendedFlags1Enabled(const UA_NetworkMessage* src);
static UA_Boolean UA_NetworkMessage_ExtendedFlags2Enabled(const UA_NetworkMessage* src);
static UA_Boolean UA_DataSetMessageHeader_DataSetFlags2Enabled(const UA_DataSetMessageHeader* src);

static UA_StatusCode
UA_NetworkMessageHeader_encodeBinary(EncodeCtx *ctx,
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

    UA_StatusCode rv = UA_Byte_encodeBinary(&v, &ctx->pos, ctx->end);
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

        rv = UA_Byte_encodeBinary(&v, &ctx->pos, ctx->end);
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

            rv = UA_Byte_encodeBinary(&v, &ctx->pos, ctx->end);
            UA_CHECK_STATUS(rv, return rv);
        }
    }

    /* PublisherId */
    if(src->publisherIdEnabled) {
        switch (src->publisherId.idType) {
        case UA_PUBLISHERIDTYPE_BYTE:
            rv = UA_Byte_encodeBinary(&src->publisherId.id.byte, &ctx->pos, ctx->end);
            break;

        case UA_PUBLISHERIDTYPE_UINT16:
            rv = UA_UInt16_encodeBinary(&src->publisherId.id.uint16, &ctx->pos, ctx->end);
            break;

        case UA_PUBLISHERIDTYPE_UINT32:
            rv = UA_UInt32_encodeBinary(&src->publisherId.id.uint32, &ctx->pos, ctx->end);
            break;

        case UA_PUBLISHERIDTYPE_UINT64:
            rv = UA_UInt64_encodeBinary(&src->publisherId.id.uint64, &ctx->pos, ctx->end);
            break;

        case UA_PUBLISHERIDTYPE_STRING:
            rv = UA_String_encodeBinary(&src->publisherId.id.string, &ctx->pos, ctx->end);
            break;

        default:
            rv = UA_STATUSCODE_BADINTERNALERROR;
            break;
        }
        UA_CHECK_STATUS(rv, return rv);
    }

    /* DataSetClassId */
    if(src->dataSetClassIdEnabled) {
        rv = UA_Guid_encodeBinary(&src->dataSetClassId, &ctx->pos, ctx->end);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_GroupHeader_encodeBinary(EncodeCtx *ctx, const UA_NetworkMessage* src) {
    UA_Byte v = 0;
    if(src->groupHeader.writerGroupIdEnabled)
        v |= GROUP_HEADER_WRITER_GROUPID_ENABLED;

    if(src->groupHeader.groupVersionEnabled)
        v |= GROUP_HEADER_GROUP_VERSION_ENABLED;

    if(src->groupHeader.networkMessageNumberEnabled)
        v |= GROUP_HEADER_NM_NUMBER_ENABLED;

    if(src->groupHeader.sequenceNumberEnabled)
        v |= GROUP_HEADER_SEQUENCE_NUMBER_ENABLED;

    UA_StatusCode rv = UA_Byte_encodeBinary(&v, &ctx->pos, ctx->end);
    if(src->groupHeader.writerGroupIdEnabled)
        rv |= UA_UInt16_encodeBinary(&src->groupHeader.writerGroupId,
                                     &ctx->pos, ctx->end);

    if(src->groupHeader.groupVersionEnabled)
        rv |= UA_UInt32_encodeBinary(&src->groupHeader.groupVersion,
                                     &ctx->pos, ctx->end);

    if(src->groupHeader.networkMessageNumberEnabled)
        rv |= UA_UInt16_encodeBinary(&src->groupHeader.networkMessageNumber,
                                     &ctx->pos, ctx->end);

    if(src->groupHeader.sequenceNumberEnabled)
        rv |= UA_UInt16_encodeBinary(&src->groupHeader.sequenceNumber,
                                     &ctx->pos, ctx->end);

    return rv;
}

static UA_StatusCode
UA_PayloadHeader_encodeBinary(EncodeCtx *ctx, const UA_NetworkMessage* src) {
    if(src->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    UA_Byte count = (UA_Byte)src->payload.dataSetPayload.dataSetMessagesSize;
    UA_StatusCode rv = UA_Byte_encodeBinary(&count, &ctx->pos, ctx->end);

    for(UA_Byte i = 0; i < count; i++) {
        UA_UInt16 dswId = src->payload.dataSetPayload.dataSetMessages[i].dataSetWriterId;
        rv |= UA_UInt16_encodeBinary(&dswId, &ctx->pos, ctx->end);
    }

    return rv;
}

static UA_StatusCode
UA_ExtendedNetworkMessageHeader_encodeBinary(EncodeCtx *ctx, const UA_NetworkMessage* src) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    if(src->timestampEnabled)
        rv |= UA_DateTime_encodeBinary(&src->timestamp, &ctx->pos, ctx->end);

    if(src->picosecondsEnabled)
        rv |= UA_UInt16_encodeBinary(&src->picoseconds, &ctx->pos, ctx->end);

    if(src->promotedFieldsEnabled) {
        /* Size (calculate & encode) */
        UA_UInt16 pfSize = 0;
        for(UA_UInt16 i = 0; i < src->promotedFieldsSize; i++)
            pfSize = (UA_UInt16)(pfSize + UA_Variant_calcSizeBinary(&src->promotedFields[i]));
        rv |= UA_UInt16_encodeBinary(&pfSize, &ctx->pos, ctx->end);

        for(UA_UInt16 i = 0; i < src->promotedFieldsSize; i++)
            rv |= UA_Variant_encodeBinary(&src->promotedFields[i], &ctx->pos, ctx->end);
    }

    return rv;
}

static UA_StatusCode
UA_SecurityHeader_encodeBinary(EncodeCtx *ctx, const UA_NetworkMessage* src) {
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

    UA_StatusCode rv = UA_Byte_encodeBinary(&v, &ctx->pos, ctx->end);

    /* SecurityTokenId */
    rv |= UA_UInt32_encodeBinary(&src->securityHeader.securityTokenId,
                                 &ctx->pos, ctx->end);

    /* NonceLength */
    UA_Byte nonceLength = (UA_Byte)src->securityHeader.messageNonceSize;
    rv |= UA_Byte_encodeBinary(&nonceLength, &ctx->pos, ctx->end);

    /* MessageNonce */
    for(size_t i = 0; i < src->securityHeader.messageNonceSize; i++) {
        rv |= UA_Byte_encodeBinary(&src->securityHeader.messageNonce[i],
                                   &ctx->pos, ctx->end);
    }

    /* SecurityFooterSize */
    if(src->securityHeader.securityFooterEnabled) {
        rv |= UA_UInt16_encodeBinary(&src->securityHeader.securityFooterSize,
                                     &ctx->pos, ctx->end);
    }

    return rv;
}

UA_StatusCode
UA_NetworkMessage_encodeHeaders(const UA_NetworkMessage* src, UA_Byte **bufPos,
                                const UA_Byte *bufEnd) {
    EncodeCtx ctx;
    ctx.pos = *bufPos;
    ctx.end = bufEnd;
    
    /* Message Header */
    UA_StatusCode rv = UA_NetworkMessageHeader_encodeBinary(&ctx, src);

    /* Group Header */
    if(src->groupHeaderEnabled)
        rv |= UA_GroupHeader_encodeBinary(&ctx, src);

    /* Payload Header */
    if(src->payloadHeaderEnabled)
        rv |= UA_PayloadHeader_encodeBinary(&ctx, src);

    /* Extended Network Message Header */
    rv |= UA_ExtendedNetworkMessageHeader_encodeBinary(&ctx, src);

    /* SecurityHeader */
    if(src->securityEnabled)
        rv |= UA_SecurityHeader_encodeBinary(&ctx, src);

    *bufPos = ctx.pos;
    return rv;
}


UA_StatusCode
UA_NetworkMessage_encodePayload(const UA_NetworkMessage* src, UA_Byte **bufPos,
                                const UA_Byte *bufEnd) {
    /* Payload */
    if(src->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    EncodeCtx ctx;
    ctx.pos = *bufPos;
    ctx.end = bufEnd;

    UA_Byte count = (UA_Byte)src->payload.dataSetPayload.dataSetMessagesSize;
    UA_StatusCode rv;
    if(src->payloadHeaderEnabled && count > 1) {
        for(UA_Byte i = 0; i < count; i++) {
            /* Calculate the size, if not specified */
            UA_DataSetMessage *dsm = &src->payload.dataSetPayload.dataSetMessages[i];
            UA_UInt16 sz = (UA_UInt16)UA_DataSetMessage_calcSizeBinary(dsm, NULL, 0);
            rv = UA_UInt16_encodeBinary(&sz, &ctx.pos, ctx.end);
            UA_CHECK_STATUS(rv, return rv);
        }
    }

    for(UA_Byte i = 0; i < count; i++) {
        UA_DataSetMessage *dsm = &src->payload.dataSetPayload.dataSetMessages[i];
        rv = UA_DataSetMessage_encodeBinary(dsm, &ctx.pos, ctx.end);
        UA_CHECK_STATUS(rv, return rv);
    }

    *bufPos = ctx.pos;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NetworkMessage_encodeFooters(const UA_NetworkMessage* src, UA_Byte **bufPos,
                                const UA_Byte *bufEnd) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    if(src->securityEnabled &&
       src->securityHeader.securityFooterEnabled) {
        for(size_t i = 0; i < src->securityHeader.securityFooterSize; i++) {
            rv |= UA_Byte_encodeBinary(&src->securityFooter.data[i], bufPos, bufEnd);
        }
    }
    return rv;
}

UA_StatusCode
UA_NetworkMessage_encodeBinary(const UA_NetworkMessage* src,
                               UA_ByteString *outBuf) {
    /* Allocate memory */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_Boolean alloced = (outBuf->length == 0);
    if(alloced) {
        size_t length = UA_NetworkMessage_calcSizeBinary(src);
        if(length == 0)
            return UA_STATUSCODE_BADENCODINGERROR;
        res = UA_ByteString_allocBuffer(outBuf, length);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    UA_Byte *pos = outBuf->data;
    UA_Byte *end = pos + outBuf->length;
    res = UA_NetworkMessage_encodeBinaryWithEncryptStart(src, &pos, end, NULL);

    /* In case the buffer was supplied externally and is longer than the encoded
     * string */
    if(UA_LIKELY(res == UA_STATUSCODE_GOOD))
        outBuf->length = (size_t)((uintptr_t)pos - (uintptr_t)outBuf->data);

    if(alloced && res != UA_STATUSCODE_GOOD)
        UA_ByteString_clear(outBuf);
    return res;
}

UA_StatusCode
UA_NetworkMessage_encodeBinaryWithEncryptStart(const UA_NetworkMessage* src,
                                               UA_Byte **bufPos,
                                               const UA_Byte *bufEnd,
                                               UA_Byte **dataToEncryptStart) {
    UA_StatusCode rv = UA_NetworkMessage_encodeHeaders(src, bufPos, bufEnd);

    if(dataToEncryptStart)
        *dataToEncryptStart = *bufPos;

    rv |= UA_NetworkMessage_encodePayload(src, bufPos, bufEnd);
    rv |= UA_NetworkMessage_encodeFooters(src, bufPos, bufEnd);
    return rv;
}

static UA_StatusCode
UA_NetworkMessageHeader_decodeBinary(Ctx *ctx, UA_NetworkMessage *dst) {
    UA_Byte decoded;
    UA_StatusCode rv = DECODE_BINARY(&decoded, BYTE);
    UA_CHECK_STATUS(rv, return rv);

    dst->version = decoded & NM_VERSION_MASK;

    if((decoded & NM_PUBLISHER_ID_ENABLED_MASK) != 0)
        dst->publisherIdEnabled = true;

    if((decoded & NM_GROUP_HEADER_ENABLED_MASK) != 0)
        dst->groupHeaderEnabled = true;

    if((decoded & NM_PAYLOAD_HEADER_ENABLED_MASK) != 0)
        dst->payloadHeaderEnabled = true;

    if((decoded & NM_EXTENDEDFLAGS1_ENABLED_MASK) != 0) {
        rv = DECODE_BINARY(&decoded, BYTE);
        UA_CHECK_STATUS(rv, return rv);

        dst->publisherId.idType = (UA_PublisherIdType)(decoded & NM_PUBLISHER_ID_MASK);
        if((decoded & NM_DATASET_CLASSID_ENABLED_MASK) != 0)
            dst->dataSetClassIdEnabled = true;

        if((decoded & NM_SECURITY_ENABLED_MASK) != 0)
            dst->securityEnabled = true;

        if((decoded & NM_TIMESTAMP_ENABLED_MASK) != 0)
            dst->timestampEnabled = true;

        if((decoded & NM_PICOSECONDS_ENABLED_MASK) != 0)
            dst->picosecondsEnabled = true;

        if((decoded & NM_EXTENDEDFLAGS2_ENABLED_MASK) != 0) {
            rv = DECODE_BINARY(&decoded, BYTE);
            UA_CHECK_STATUS(rv, return rv);

            if((decoded & NM_CHUNK_MESSAGE_MASK) != 0)
                dst->chunkMessage = true;

            if((decoded & NM_PROMOTEDFIELDS_ENABLED_MASK) != 0)
                dst->promotedFieldsEnabled = true;

            decoded = decoded & NM_NETWORK_MSG_TYPE_MASK;
            decoded = (UA_Byte) (decoded >> NM_SHIFT_LEN);
            dst->networkMessageType = (UA_NetworkMessageType)decoded;
        }
    }

    if(dst->publisherIdEnabled) {
        switch(dst->publisherId.idType) {
            case UA_PUBLISHERIDTYPE_BYTE:
                rv = DECODE_BINARY(&dst->publisherId.id.byte, BYTE);
                break;
            case UA_PUBLISHERIDTYPE_UINT16:
                rv = DECODE_BINARY(&dst->publisherId.id.uint16, UINT16);
                break;
            case UA_PUBLISHERIDTYPE_UINT32:
                rv = DECODE_BINARY(&dst->publisherId.id.uint32, UINT32);
                break;
            case UA_PUBLISHERIDTYPE_UINT64:
                rv = DECODE_BINARY(&dst->publisherId.id.uint64, UINT64);
                break;
            case UA_PUBLISHERIDTYPE_STRING:
                rv = DECODE_BINARY(&dst->publisherId.id.string, STRING);
                break;
            default:
                rv = UA_STATUSCODE_BADINTERNALERROR;
                break;
        }
        UA_CHECK_STATUS(rv, return rv);
    }

    if(dst->dataSetClassIdEnabled) {
        rv = DECODE_BINARY(&dst->dataSetClassId, GUID);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_GroupHeader_decodeBinary(Ctx *ctx, UA_NetworkMessage* dst) {
    UA_Byte decoded;
    UA_StatusCode rv = DECODE_BINARY(&decoded, BYTE);
    UA_CHECK_STATUS(rv, return rv);

    if((decoded & GROUP_HEADER_WRITER_GROUPID_ENABLED) != 0) {
        dst->groupHeader.writerGroupIdEnabled = true;
        rv |= DECODE_BINARY(&dst->groupHeader.writerGroupId, UINT16);
    }

    if((decoded & GROUP_HEADER_GROUP_VERSION_ENABLED) != 0) {
        dst->groupHeader.groupVersionEnabled = true;
        rv |= DECODE_BINARY(&dst->groupHeader.groupVersion, UINT32);
    }

    if((decoded & GROUP_HEADER_NM_NUMBER_ENABLED) != 0) {
        dst->groupHeader.networkMessageNumberEnabled = true;
        rv |= DECODE_BINARY(&dst->groupHeader.networkMessageNumber, UINT16);
    }

    if((decoded & GROUP_HEADER_SEQUENCE_NUMBER_ENABLED) != 0) {
        dst->groupHeader.sequenceNumberEnabled = true;
        rv |= DECODE_BINARY(&dst->groupHeader.sequenceNumber, UINT16);
    }

    return rv;
}

static UA_StatusCode
UA_PayloadHeader_decodeBinary(Ctx *ctx, UA_NetworkMessage* dst) {
    if(dst->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    UA_Byte count;
    UA_StatusCode rv = DECODE_BINARY(&count, BYTE);
    UA_CHECK_STATUS(rv, return rv);

    if(count == 0)
        return UA_STATUSCODE_GOOD;

    dst->payload.dataSetPayload.dataSetMessages = (UA_DataSetMessage*)
        ctxCalloc(ctx, count, sizeof(UA_DataSetMessage));
    if(!dst->payload.dataSetPayload.dataSetMessages)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    dst->payload.dataSetPayload.dataSetMessagesSize = count;

    for(UA_Byte i = 0; i < count; i++) {
        rv |= DECODE_BINARY(&dst->payload.dataSetPayload.dataSetMessages[i].
                            dataSetWriterId, UINT16);
    }
    return rv;
}

static UA_StatusCode
UA_ExtendedNetworkMessageHeader_decodeBinary(Ctx *ctx, UA_NetworkMessage* dst) {
    UA_StatusCode rv;

    /* Timestamp*/
    if(dst->timestampEnabled) {
        rv = DECODE_BINARY(&dst->timestamp, DATETIME);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* Picoseconds */
    if(dst->picosecondsEnabled) {
        rv = DECODE_BINARY(&dst->picoseconds, UINT16);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* PromotedFields */
    if(UA_LIKELY(!dst->promotedFieldsEnabled))
        return UA_STATUSCODE_GOOD;

    UA_UInt16 promotedFieldsLength; /* Size in bytes, not in number of fields */
    rv = DECODE_BINARY(&promotedFieldsLength, UINT16);
    UA_CHECK_STATUS(rv, return rv);
    if(promotedFieldsLength == 0)
        return UA_STATUSCODE_GOOD;

    UA_Byte *endPos = ctx->pos + promotedFieldsLength;
    if(endPos > ctx->end)
        return UA_STATUSCODE_BADDECODINGERROR;
    
    size_t counter = 0;
    size_t space = 4;
    UA_Variant *pf = (UA_Variant*)
        ctxCalloc(ctx, space, UA_TYPES[UA_TYPES_VARIANT].memSize);
    if(!pf)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    do {
        /* Increase the available space */
        if(counter == space) {
            UA_Variant *tmp = (UA_Variant*)
                ctxCalloc(ctx, space << 1, UA_TYPES[UA_TYPES_VARIANT].memSize);
            if(!tmp) {
                if(!ctx->opts.calloc)
                    UA_Array_delete(pf, counter, &UA_TYPES[UA_TYPES_VARIANT]);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            memcpy(tmp, pf, space * UA_TYPES[UA_TYPES_VARIANT].memSize);
            ctxFree(ctx, pf);
            pf = tmp;
            space = space << 1;
        }

        /* Decode the PromotedField */
        rv = DECODE_BINARY(&pf[counter], VARIANT);
        if(rv != UA_STATUSCODE_GOOD) {
            if(!ctx->opts.calloc)
                UA_Array_delete(pf, counter, &UA_TYPES[UA_TYPES_VARIANT]);
            return rv;
        }

        counter++;
    } while(ctx->pos < endPos);

    dst->promotedFields = pf;
    dst->promotedFieldsSize = (UA_UInt16)counter;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_SecurityHeader_decodeBinary(Ctx *ctx, UA_NetworkMessage* dst) {
    /* SecurityFlags */
    UA_Byte decoded;
    UA_StatusCode rv = DECODE_BINARY(&decoded, BYTE);
    UA_CHECK_STATUS(rv, return rv);

    if((decoded & SECURITY_HEADER_NM_SIGNED) != 0)
        dst->securityHeader.networkMessageSigned = true;

    if((decoded & SECURITY_HEADER_NM_ENCRYPTED) != 0)
        dst->securityHeader.networkMessageEncrypted = true;

    if((decoded & SECURITY_HEADER_SEC_FOOTER_ENABLED) != 0)
        dst->securityHeader.securityFooterEnabled = true;

    if((decoded & SECURITY_HEADER_FORCE_KEY_RESET) != 0)
        dst->securityHeader.forceKeyReset = true;

    /* SecurityTokenId */
    rv = DECODE_BINARY(&dst->securityHeader.securityTokenId, UINT32);
    UA_CHECK_STATUS(rv, return rv);

    /* MessageNonce */
    UA_Byte nonceLength;
    rv = DECODE_BINARY(&nonceLength, BYTE);
    UA_CHECK_STATUS(rv, return rv);

    if(nonceLength > UA_NETWORKMESSAGE_MAX_NONCE_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    if(nonceLength > 0) {
        dst->securityHeader.messageNonceSize = nonceLength;
        for(UA_Byte i = 0; i < nonceLength; i++) {
            rv = DECODE_BINARY(&dst->securityHeader.messageNonce[i], BYTE);
            UA_CHECK_STATUS(rv, return rv);
        }
    }

    /* SecurityFooterSize */
    if(dst->securityHeader.securityFooterEnabled)
        rv = DECODE_BINARY(&dst->securityHeader.securityFooterSize, UINT16);
    return rv;
}

UA_StatusCode
UA_NetworkMessage_decodeHeaders(Ctx *ctx, UA_NetworkMessage *dst) {
    UA_StatusCode rv = UA_NetworkMessageHeader_decodeBinary(ctx, dst);
    UA_CHECK_STATUS(rv, return rv);

    if(dst->groupHeaderEnabled) {
        rv = UA_GroupHeader_decodeBinary(ctx, dst);
        UA_CHECK_STATUS(rv, return rv);
    }

    if(dst->payloadHeaderEnabled) {
        rv = UA_PayloadHeader_decodeBinary(ctx, dst);
        UA_CHECK_STATUS(rv, return rv);
    }

    rv = UA_ExtendedNetworkMessageHeader_decodeBinary(ctx, dst);
    UA_CHECK_STATUS(rv, return rv);

    if(dst->securityEnabled)
        rv = UA_SecurityHeader_decodeBinary(ctx, dst);

    return rv;
}

UA_StatusCode
UA_NetworkMessage_decodePayload(Ctx *ctx, UA_NetworkMessage *dst) {
    /* Payload */
    if(dst->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    /* The dataset was already allocated if the header is enabled.
     * To decode the DataSetReaderIds. */
    size_t count = dst->payload.dataSetPayload.dataSetMessagesSize;
    if(!dst->payloadHeaderEnabled) {
        count = 1;
        dst->payload.dataSetPayload.dataSetMessages =
            (UA_DataSetMessage *)ctxCalloc(ctx, 1, sizeof(UA_DataSetMessage));
        UA_CHECK_MEM(dst->payload.dataSetPayload.dataSetMessages,
                     return UA_STATUSCODE_BADOUTOFMEMORY);
        dst->payload.dataSetPayload.dataSetMessagesSize = 1;
    }

    /* Decode the payload sizes (for the raw encoding) */
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    UA_STACKARRAY(UA_UInt16, payloadSizes, count);
    memset(payloadSizes, 0, sizeof(UA_UInt16) * count);
    if(count > 1) {
        for(size_t i = 0; i < count; i++) {
            rv |= DECODE_BINARY(&payloadSizes[i], UINT16);
            if(payloadSizes[i] == 0)
                return UA_STATUSCODE_BADDECODINGERROR;
        }
    }
    UA_CHECK_STATUS(rv, return rv);

    /* Decode the DataSetMessages */
    for(size_t i = 0; i < count; i++) {
        rv |= UA_DataSetMessage_decodeBinary(ctx,
                                             &dst->payload.dataSetPayload.dataSetMessages[i],
                                             payloadSizes[i]);
    }

    return rv;
}

UA_StatusCode
UA_NetworkMessage_decodeFooters(Ctx *ctx, UA_NetworkMessage *dst) {
    if(!dst->securityEnabled)
        return UA_STATUSCODE_GOOD;

    if(!dst->securityHeader.securityFooterEnabled ||
       dst->securityHeader.securityFooterSize == 0)
        return UA_STATUSCODE_GOOD;
    
    UA_StatusCode rv = UA_ByteString_allocBuffer(&dst->securityFooter,
                                                 dst->securityHeader.securityFooterSize);
    UA_CHECK_STATUS(rv, return rv);
    
    for(UA_UInt16 i = 0; i < dst->securityHeader.securityFooterSize; i++) {
        rv |= DECODE_BINARY(&dst->securityFooter.data[i], BYTE);
    }
    return rv;
}

UA_StatusCode
UA_NetworkMessage_decodeBinary(const UA_ByteString *src, UA_NetworkMessage *dst,
                               const UA_DecodeBinaryOptions *options) {
    /* Initialize the decoding context */
    Ctx ctx;
    ctx.pos = src->data;
    ctx.end = &src->data[src->length];
    ctx.depth = 0;
    if(options)
        ctx.opts = *options;
    else
        memset(&ctx.opts, 0, sizeof(UA_DecodeBinaryOptions));

    /* headers only need to be decoded when not in encryption mode
     * because headers are already decoded when encryption mode is enabled
     * to check for security parameters and decrypt/verify
     *
     * TODO: check if there is a workaround to use this function
     *       also when encryption is enabled
     */
    // #ifndef UA_ENABLE_PUBSUB_ENCRYPTION
    // if(*offset == 0) {
    //    rv = UA_NetworkMessage_decodeHeaders(src, offset, dst);
    //    UA_CHECK_STATUS(rv, return rv);
    // }
    // #endif

    /* Initialize the NetworkMessage */
    memset(dst, 0, sizeof(UA_NetworkMessage));

    UA_StatusCode rv = UA_NetworkMessage_decodeHeaders(&ctx, dst);
    UA_CHECK_STATUS(rv, goto cleanup);

    rv = UA_NetworkMessage_decodePayload(&ctx, dst);
    UA_CHECK_STATUS(rv, goto cleanup);

    rv = UA_NetworkMessage_decodeFooters(&ctx, dst);
    UA_CHECK_STATUS(rv, goto cleanup);

cleanup:
    if(rv != UA_STATUSCODE_GOOD && !ctx.opts.calloc)
        UA_NetworkMessage_clear(dst);

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
UA_NetworkMessage_calcSizeBinary(const UA_NetworkMessage *p) {
    return UA_NetworkMessage_calcSizeBinaryWithOffsetTable(p, NULL);
}

size_t
UA_NetworkMessage_calcSizeBinaryWithOffsetTable(const UA_NetworkMessage *p,
                                                UA_PubSubOffsetTable *ot) {
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
        size += 1; /* p->payloadHeader.dataSetPayloadHeader.count */
        size += (size_t)(2LU * p->payload.dataSetPayload.dataSetMessagesSize); /* uint16 */
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

    UA_Byte count = (UA_Byte)p->payload.dataSetPayload.dataSetMessagesSize;
    if(p->payloadHeaderEnabled && count > 1)
        size += (size_t)(2LU * count); /* DataSetMessagesSize (uint16) */
    for(size_t i = 0; i < count; i++) {
        /* size = ... as the original size is used as the starting point in
         * UA_DataSetMessage_calcSizeBinary */
        UA_DataSetMessage *dsm = &p->payload.dataSetPayload.dataSetMessages[i];
        size = UA_DataSetMessage_calcSizeBinary(dsm, ot, size);
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
        if(p->payload.dataSetPayload.dataSetMessages) {
            UA_Byte count = (UA_Byte)p->payload.dataSetPayload.dataSetMessagesSize;
            for(size_t i = 0; i < count; i++)
                UA_DataSetMessage_clear(&p->payload.dataSetPayload.dataSetMessages[i]);
            UA_free(p->payload.dataSetPayload.dataSetMessages);
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
UA_DataSetMessageHeader_encodeBinary(const UA_DataSetMessageHeader* src, UA_Byte **bufPos,
                                     const UA_Byte *bufEnd) {
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

    UA_StatusCode rv = UA_Byte_encodeBinary(&v, bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

    /* DataSetFlags2 */
    if(UA_DataSetMessageHeader_DataSetFlags2Enabled(src)) {
        v = (UA_Byte)src->dataSetMessageType;

        if(src->timestampEnabled)
            v |= DS_MESSAGEHEADER_TIMESTAMP_ENABLED_MASK;

        if(src->picoSecondsIncluded)
            v |= DS_MESSAGEHEADER_PICOSECONDS_INCLUDED_MASK;

        rv = UA_Byte_encodeBinary(&v, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* DataSetMessageSequenceNr */
    if(src->dataSetMessageSequenceNrEnabled) {
        rv = UA_UInt16_encodeBinary(&src->dataSetMessageSequenceNr, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* Timestamp */
    if(src->timestampEnabled) {
        rv = UA_DateTime_encodeBinary(&src->timestamp, bufPos, bufEnd); /* UtcTime */
        UA_CHECK_STATUS(rv, return rv);
    }

    /* PicoSeconds */
    if(src->picoSecondsIncluded) {
        rv = UA_UInt16_encodeBinary(&src->picoSeconds, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* Status */
    if(src->statusEnabled) {
        rv = UA_UInt16_encodeBinary(&src->status, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* ConfigVersionMajorVersion */
    if(src->configVersionMajorVersionEnabled) {
        rv = UA_UInt32_encodeBinary(&src->configVersionMajorVersion, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* ConfigVersionMinorVersion */
    if(src->configVersionMinorVersionEnabled) {
        rv = UA_UInt32_encodeBinary(&src->configVersionMinorVersion, bufPos, bufEnd);
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
UA_DataSetMessageHeader_decodeBinary(Ctx *ctx, UA_DataSetMessageHeader* dst) {
    UA_Byte v;
    UA_StatusCode rv = DECODE_BINARY(&v, BYTE);
    UA_CHECK_STATUS(rv, return rv);

    UA_Byte v2 = v & DS_MESSAGEHEADER_FIELD_ENCODING_MASK;
    v2 = (UA_Byte)(v2 >> DS_MH_SHIFT_LEN);
    dst->fieldEncoding = (UA_FieldEncoding)v2;

    if((v & DS_MESSAGEHEADER_DS_MSG_VALID) != 0)
        dst->dataSetMessageValid = true;

    if((v & DS_MESSAGEHEADER_SEQ_NR_ENABLED_MASK) != 0)
        dst->dataSetMessageSequenceNrEnabled = true;

    if((v & DS_MESSAGEHEADER_STATUS_ENABLED_MASK) != 0)
        dst->statusEnabled = true;

    if((v & DS_MESSAGEHEADER_CONFIGMAJORVERSION_ENABLED_MASK) != 0)
        dst->configVersionMajorVersionEnabled = true;

    if((v & DS_MESSAGEHEADER_CONFIGMINORVERSION_ENABLED_MASK) != 0)
        dst->configVersionMinorVersionEnabled = true;

    if((v & DS_MESSAGEHEADER_FLAGS2_ENABLED_MASK) != 0) {
        rv = DECODE_BINARY(&v, BYTE);
        UA_CHECK_STATUS(rv, return rv);

        dst->dataSetMessageType = (UA_DataSetMessageType)(v & DS_MESSAGEHEADER_DS_MESSAGE_TYPE_MASK);

        if((v & DS_MESSAGEHEADER_TIMESTAMP_ENABLED_MASK) != 0)
            dst->timestampEnabled = true;

        if((v & DS_MESSAGEHEADER_PICOSECONDS_INCLUDED_MASK) != 0)
            dst->picoSecondsIncluded = true;
    }
    /* The else-case is implied as dst is zeroed-out initially:
     * else {
     * dst->dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
     *   dst->picoSecondsIncluded = false;
     * } */

    if(dst->dataSetMessageSequenceNrEnabled) {
        rv = DECODE_BINARY(&dst->dataSetMessageSequenceNr, UINT16);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dst->dataSetMessageSequenceNr = 0;
    }

    if(dst->timestampEnabled) {
        rv = DECODE_BINARY(&dst->timestamp, DATETIME);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dst->timestamp = 0;
    }

    if(dst->picoSecondsIncluded) {
        rv = DECODE_BINARY(&dst->picoSeconds, UINT16);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dst->picoSeconds = 0;
    }

    if(dst->statusEnabled) {
        rv = DECODE_BINARY(&dst->status, UINT16);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dst->status = 0;
    }

    if(dst->configVersionMajorVersionEnabled) {
        rv = DECODE_BINARY(&dst->configVersionMajorVersion, UINT32);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dst->configVersionMajorVersion = 0;
    }

    if(dst->configVersionMinorVersionEnabled) {
        rv = DECODE_BINARY(&dst->configVersionMinorVersion, UINT32);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dst->configVersionMinorVersion = 0;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_DataSetMessage_keyFrame_encodeBinary(const UA_DataSetMessage* src, UA_Byte **bufPos,
                                        const UA_Byte *bufEnd) {
    /* Heartbeat: "DataSetMessage is a key frame that only contains header
     * information" */
    if(src->data.keyFrameData.fieldCount == 0)
        return UA_STATUSCODE_GOOD;

    UA_StatusCode rv;
    if(src->header.fieldEncoding != UA_FIELDENCODING_RAWDATA) {
        rv = UA_UInt16_encodeBinary(&src->data.keyFrameData.fieldCount,
                                    bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }
    
    for(UA_UInt16 i = 0; i < src->data.keyFrameData.fieldCount; i++) {
        const UA_DataValue *v = &src->data.keyFrameData.dataSetFields[i];
        
        if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
            rv = UA_Variant_encodeBinary(&v->value, bufPos, bufEnd);
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
            rv = UA_DataValue_encodeBinary(v, bufPos, bufEnd);
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
            if(!v->value.type)
                return UA_STATUSCODE_BADINTERNALERROR;

            UA_FieldMetaData *fmd = &src->data.keyFrameData.dataSetMetaDataType->fields[i];

            /* For arrays we need to encode the dimension sizes before the actual data */
            size_t elementCount = 1;
            for(size_t cnt = 0; cnt < fmd->arrayDimensionsSize; cnt++) {
                elementCount *= fmd->arrayDimensions[cnt];
                rv = UA_UInt32_encodeBinary(&fmd->arrayDimensions[cnt], bufPos, bufEnd);
                UA_CHECK_STATUS(rv, return rv);
            }

            /* Check if Array size matches the one specified in metadata */
            if(fmd->valueRank > 0 && elementCount != v->value.arrayLength)
                return UA_STATUSCODE_BADENCODINGERROR;

            UA_Byte *valuePtr = (UA_Byte *)v->value.data;
            for(size_t cnt = 0; cnt < elementCount; cnt++) {
                if(fmd->maxStringLength != 0 &&
                   (v->value.type->typeKind == UA_DATATYPEKIND_STRING ||
                    v->value.type->typeKind == UA_DATATYPEKIND_BYTESTRING)) {
                    if(((UA_String *)v->value.data)->length > fmd->maxStringLength){
                        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
                    }
                    rv = UA_encodeBinaryInternal(valuePtr, v->value.type, bufPos, &bufEnd,
                                                 NULL, NULL, NULL);
                    size_t lengthDifference = fmd->maxStringLength - ((UA_String *)valuePtr)->length;
                    memset(*bufPos, 0, lengthDifference);
                    *bufPos += lengthDifference;
                } else {
                    /* Padding not yet supported for strings as part of structures */
                    rv = UA_encodeBinaryInternal(valuePtr, v->value.type, bufPos, &bufEnd,
                                                 NULL, NULL, NULL);
                }
                valuePtr += v->value.type->memSize;
            }
        }
        
        UA_CHECK_STATUS(rv, return rv);
    }
    return rv;
}

static UA_StatusCode
UA_DataSetMessage_deltaFrame_encodeBinary(const UA_DataSetMessage* src, UA_Byte **bufPos,
                                          const UA_Byte *bufEnd) {
    if(src->header.fieldEncoding == UA_FIELDENCODING_RAWDATA)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    /* Here the FieldCount is always present */
    const UA_DataSetMessage_DataDeltaFrameData *dfd = &src->data.deltaFrameData;
    UA_StatusCode rv = UA_UInt16_encodeBinary(&dfd->fieldCount, bufPos, bufEnd);
    if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
        for(UA_UInt16 i = 0; i < dfd->fieldCount; i++) {
            rv |= UA_UInt16_encodeBinary(&dfd->deltaFrameFields[i].fieldIndex,
                                         bufPos, bufEnd);
            rv |= UA_Variant_encodeBinary(&dfd->deltaFrameFields[i].fieldValue.value,
                                         bufPos, bufEnd);
        }
    } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
        for(UA_UInt16 i = 0; i < dfd->fieldCount; i++) {
            rv |= UA_UInt16_encodeBinary(&dfd->deltaFrameFields[i].fieldIndex,
                                         bufPos, bufEnd);
            rv |= UA_DataValue_encodeBinary(&dfd->deltaFrameFields[i].fieldValue,
                                            bufPos, bufEnd);
        }
    }
    return rv;
}

UA_StatusCode
UA_DataSetMessage_encodeBinary(const UA_DataSetMessage* src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd) {
    /* Encode Header */
    UA_StatusCode rv = UA_DataSetMessageHeader_encodeBinary(&src->header,
                                                            bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

    /* Encode Payload */
    switch(src->header.dataSetMessageType) {
    case UA_DATASETMESSAGE_DATAKEYFRAME:
        rv = UA_DataSetMessage_keyFrame_encodeBinary(src, bufPos, bufEnd);
        break;
    case UA_DATASETMESSAGE_DATADELTAFRAME:
        rv = UA_DataSetMessage_deltaFrame_encodeBinary(src, bufPos, bufEnd);
        break;
    case UA_DATASETMESSAGE_KEEPALIVE:
        break; /* Keep-Alive Message contains no Payload Data */
    default:
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    /* Padding */
    if(src->configuredSize > 0 && src->header.dataSetMessageValid) {
        size_t padding = (size_t)(bufEnd - *bufPos);
        memset(*bufPos, 0, padding); /* Set the bytes to 0*/
        *bufPos += padding; /* move the bufpos accordingly*/
    }
    UA_CHECK_STATUS(rv, return rv);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_DataSetMessage_keyFrame_decodeBinary(Ctx *ctx, UA_Byte *initialPos, UA_DataSetMessage* dst,
                                        size_t dsmSize) {
    if(ctx->pos == ctx->end)
        return UA_STATUSCODE_GOOD; /* Messages ends after the header --> Heartbeat */

    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    UA_DataSetMessage_DataKeyFrameData *kfd = &dst->data.keyFrameData;
    switch(dst->header.fieldEncoding) {
    case UA_FIELDENCODING_VARIANT:
        rv = DECODE_BINARY(&kfd->fieldCount, UINT16);
        UA_CHECK_STATUS(rv, return rv);

        kfd->dataSetFields = (UA_DataValue *)
            ctxCalloc(ctx, dst->data.keyFrameData.fieldCount, sizeof(UA_DataValue));
        if(!kfd->dataSetFields) {
            kfd->fieldCount = 0;
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }

        for(UA_UInt16 i = 0; i < kfd->fieldCount; i++) {
            UA_DataValue_init(&kfd->dataSetFields[i]);
            rv = DECODE_BINARY(&kfd->dataSetFields[i].value, VARIANT);
            UA_CHECK_STATUS(rv, return rv);
            kfd->dataSetFields[i].hasValue = true;
        }
        break;

    case UA_FIELDENCODING_DATAVALUE:
        rv = DECODE_BINARY(&kfd->fieldCount, UINT16);
        UA_CHECK_STATUS(rv, return rv);

        kfd->dataSetFields = (UA_DataValue *)
            ctxCalloc(ctx, kfd->fieldCount, sizeof(UA_DataValue));
        if(!kfd->dataSetFields) {
            kfd->fieldCount = 0;
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }

        for(UA_UInt16 i = 0; i < kfd->fieldCount; i++) {
            rv = DECODE_BINARY(&kfd->dataSetFields[i], DATAVALUE);
            UA_CHECK_STATUS(rv, return rv);
        }
        break;

    case UA_FIELDENCODING_RAWDATA: {
        /* If no size is known from the payload header, then the payload fills
         * the entire message. In the future we should subtract security footer
         * and signature as well (or do that before). */
        const UA_Byte *payloadEnd = initialPos + dsmSize;
        if(payloadEnd > ctx->end)
            return UA_STATUSCODE_BADDECODINGERROR;
        if(dsmSize == 0)
            payloadEnd = ctx->end;
        kfd->rawFields.data = ctx->pos;
        kfd->rawFields.length = (size_t)(uintptr_t)(payloadEnd - ctx->pos);
        ctx->pos = (UA_Byte*)(uintptr_t)payloadEnd;
        break;
    }

    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return rv;
}

static UA_StatusCode
UA_DataSetMessage_deltaFrame_decodeBinary(Ctx *ctx, UA_DataSetMessage* dst) {
    if(dst->header.fieldEncoding == UA_FIELDENCODING_RAWDATA)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    if(dst->header.fieldEncoding != UA_FIELDENCODING_VARIANT &&
       dst->header.fieldEncoding != UA_FIELDENCODING_DATAVALUE)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_DataSetMessage_DataDeltaFrameData *dfd = &dst->data.deltaFrameData;
    UA_StatusCode rv = DECODE_BINARY(&dfd->fieldCount, UINT16);
    UA_CHECK_STATUS(rv, return rv);

    dfd->deltaFrameFields = (UA_DataSetMessage_DeltaFrameField *)
        ctxCalloc(ctx, dfd->fieldCount, sizeof(UA_DataSetMessage_DeltaFrameField));
    if(!dst->data.deltaFrameData.deltaFrameFields) {
        dfd->fieldCount = 0;
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    for(UA_UInt16 i = 0; i < dfd->fieldCount; i++) {
        rv = DECODE_BINARY(&dfd->deltaFrameFields[i].fieldIndex, UINT16);
        UA_CHECK_STATUS(rv, return rv);

        if(dst->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
            rv = DECODE_BINARY(&dfd->deltaFrameFields[i].fieldValue.value, VARIANT);
            UA_CHECK_STATUS(rv, return rv);
            dfd->deltaFrameFields[i].fieldValue.hasValue = true;
        } else {
            rv = DECODE_BINARY(&dfd->deltaFrameFields[i].fieldValue, DATAVALUE);
            UA_CHECK_STATUS(rv, return rv);
        }
    }

    return rv;
}

UA_StatusCode
UA_DataSetMessage_decodeBinary(Ctx *ctx, UA_DataSetMessage *dst, UA_UInt16 dsmSize) {
    UA_Byte *initialPos = ctx->pos;
    UA_StatusCode rv = UA_DataSetMessageHeader_decodeBinary(ctx, &dst->header);
    UA_CHECK_STATUS(rv, return rv);

    switch(dst->header.dataSetMessageType) {
    case UA_DATASETMESSAGE_DATAKEYFRAME:
        rv = UA_DataSetMessage_keyFrame_decodeBinary(ctx, initialPos, dst, dsmSize);
        break;
    case UA_DATASETMESSAGE_DATADELTAFRAME:
        rv = UA_DataSetMessage_deltaFrame_decodeBinary(ctx, dst);
        break;
    case UA_DATASETMESSAGE_KEEPALIVE:
        break; /* Keep-Alive Message contains no Payload Data */
    default:
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    return rv;
}

size_t
UA_DataSetMessage_calcSizeBinary(UA_DataSetMessage *p, UA_PubSubOffsetTable *ot,
                                 size_t size) {
    if(ot) {
        size_t pos = ot->offsetsSize;
        if(!incrOffsetTable(ot))
            return 0;
        ot->offsets[pos].offset = size;
        ot->offsets[pos].offsetType = UA_PUBSUBOFFSETTYPE_DATASETMESSAGE;
    }

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
       (p->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME &&
        p->data.keyFrameData.fieldCount == 0))
        return size;

    if(p->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
        if(p->header.fieldEncoding != UA_FIELDENCODING_RAWDATA)
            size += 2; /* p->data.keyFrameData.fieldCount */

        for(UA_UInt16 i = 0; i < p->data.keyFrameData.fieldCount; i++){
            UA_PubSubOffset *offset = NULL;
            const UA_DataValue *v = &p->data.keyFrameData.dataSetFields[i];
            if(ot) {
                size_t pos = ot->offsetsSize;
                if(!incrOffsetTable(ot))
                    return 0;
                offset = &ot->offsets[pos];
                offset->offset = size;
            }

            if(p->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
                if(ot)
                    offset->offsetType = UA_PUBSUBOFFSETTYPE_DATASETFIELD_VARIANT;
                size += UA_calcSizeBinary(&v->value, &UA_TYPES[UA_TYPES_VARIANT], NULL);
            } else if(p->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
                if(p->data.keyFrameData.dataSetFields != NULL) {
                    if(ot) {
                        if(!v->value.type || !v->value.type->pointerFree)
                            return 0; /* only integer types for now */
                        /* Count the memory size of the specific field */
                        offset->offsetType = UA_PUBSUBOFFSETTYPE_DATASETFIELD_RAW;
                    }
                    UA_FieldMetaData *fmd =
                        &p->data.keyFrameData.dataSetMetaDataType->fields[i];

                    /* For arrays add encoded array length (4 bytes for each dimension) */
                    size += fmd->arrayDimensionsSize * sizeof(UA_UInt32);
                    if(ot)
                        offset->offset += fmd->arrayDimensionsSize * sizeof(UA_UInt32);

                    /* We need to know how many elements there are */
                    size_t elemCnt = 1;
                    for(size_t cnt = 0; cnt < fmd->arrayDimensionsSize; cnt++) {
                        elemCnt *= fmd->arrayDimensions[cnt];
                    }
                    size += (elemCnt * UA_calcSizeBinary(v->value.data, v->value.type, NULL));

                    /* Handle zero-padding for strings with max-string-length.
                     * Currently not supported for strings that are a part of larger
                     * structures. */
                    if(fmd->maxStringLength != 0 &&
                       (v->value.type->typeKind == UA_DATATYPEKIND_STRING ||
                        v->value.type->typeKind == UA_DATATYPEKIND_BYTESTRING)) {
                        /* Check if length < maxStringLength, The types ByteString
                         * and String are equal in their base definition */
                        size_t lengthDifference = fmd->maxStringLength -
                            ((UA_String *)v->value.data)->length;
                        size += lengthDifference;
                    }
                } else {
                    /* get length calculated in UA_DataSetMessage_decodeBinary */
                    if(ot)
                        offset->offsetType = UA_PUBSUBOFFSETTYPE_DATASETFIELD_RAW;
                    size += p->data.keyFrameData.rawFields.length;
                    /* no iteration needed */
                    break;
                }
            } else if(p->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
                if(ot)
                    offset->offsetType = UA_PUBSUBOFFSETTYPE_DATASETFIELD_DATAVALUE;
                size += UA_calcSizeBinary(v, &UA_TYPES[UA_TYPES_DATAVALUE], NULL);
            }
        }
    } else if(p->header.dataSetMessageType == UA_DATASETMESSAGE_DATADELTAFRAME) {
        if(ot)
            return 0; /* Not supported for RT */

        if(p->header.fieldEncoding == UA_FIELDENCODING_RAWDATA)
            return 0; /* not supported */

        size += 2; /* p->data.deltaFrameData.fieldCount */
        size += (size_t)(2LU * p->data.deltaFrameData.fieldCount); /* fieldIndex per field */

        for(UA_UInt16 i = 0; i < p->data.deltaFrameData.fieldCount; i++) {
            const UA_DataValue *v = &p->data.deltaFrameData.deltaFrameFields[i].fieldValue;
            if(p->header.fieldEncoding == UA_FIELDENCODING_VARIANT)
                size += UA_calcSizeBinary(&v->value, &UA_TYPES[UA_TYPES_VARIANT], NULL);
            else if(p->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE)
                size += UA_calcSizeBinary(v, &UA_TYPES[UA_TYPES_DATAVALUE], NULL);
        }
    } else {
        return 0;
    }

    if(p->configuredSize > 0) {
        /* If the message is larger than the configuredSize, it shall be set to not valid */
        if(p->configuredSize < size) 
            p->header.dataSetMessageValid = UA_FALSE;
        size = p->configuredSize;
    }
    
    /* KeepAlive-Message contains no Payload Data */
    return size;
}

void
UA_DataSetMessage_clear(UA_DataSetMessage* p) {
    if(p->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
        if(p->data.keyFrameData.dataSetFields) {
            UA_Array_delete(p->data.keyFrameData.dataSetFields,
                            p->data.keyFrameData.fieldCount,
                            &UA_TYPES[UA_TYPES_DATAVALUE]);
        }

        /* Json keys */
        if(p->data.keyFrameData.fieldNames){
            UA_Array_delete(p->data.keyFrameData.fieldNames,
                            p->data.keyFrameData.fieldCount,
                            &UA_TYPES[UA_TYPES_STRING]);
        }
    } else if(p->header.dataSetMessageType == UA_DATASETMESSAGE_DATADELTAFRAME) {
        if(p->data.deltaFrameData.deltaFrameFields) {
            for(UA_UInt16 i = 0; i < p->data.deltaFrameData.fieldCount; i++) {
                UA_DataSetMessage_DeltaFrameField *f =
                    &p->data.deltaFrameData.deltaFrameFields[i];
                UA_DataValue_clear(&f->fieldValue);
            }
            UA_free(p->data.deltaFrameData.deltaFrameFields);
        }
    }

    memset(p, 0, sizeof(UA_DataSetMessage));
}

#endif /* UA_ENABLE_PUBSUB */
