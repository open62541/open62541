/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Tino Bischoff)
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <open62541/types_generated_handling.h>

#include "ua_util_internal.h"
#include "ua_types_encoding_binary.h"
#include "ua_pubsub_networkmessage.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

const UA_Byte NM_VERSION_MASK = 15;
const UA_Byte NM_PUBLISHER_ID_ENABLED_MASK = 16;
const UA_Byte NM_GROUP_HEADER_ENABLED_MASK = 32;
const UA_Byte NM_PAYLOAD_HEADER_ENABLED_MASK = 64;
const UA_Byte NM_EXTENDEDFLAGS1_ENABLED_MASK = 128;
const UA_Byte NM_PUBLISHER_ID_MASK = 7;
const UA_Byte NM_DATASET_CLASSID_ENABLED_MASK = 8;
const UA_Byte NM_SECURITY_ENABLED_MASK = 16;
const UA_Byte NM_TIMESTAMP_ENABLED_MASK = 32;
const UA_Byte NM_PICOSECONDS_ENABLED_MASK = 64;
const UA_Byte NM_EXTENDEDFLAGS2_ENABLED_MASK = 128;
const UA_Byte NM_NETWORK_MSG_TYPE_MASK = 28;
const UA_Byte NM_CHUNK_MESSAGE_MASK = 1;
const UA_Byte NM_PROMOTEDFIELDS_ENABLED_MASK = 2;
const UA_Byte GROUP_HEADER_WRITER_GROUPID_ENABLED = 1;
const UA_Byte GROUP_HEADER_GROUP_VERSION_ENABLED = 2;
const UA_Byte GROUP_HEADER_NM_NUMBER_ENABLED = 4;
const UA_Byte GROUP_HEADER_SEQUENCE_NUMBER_ENABLED = 8;
const UA_Byte SECURITY_HEADER_NM_SIGNED = 1;
const UA_Byte SECURITY_HEADER_NM_ENCRYPTED = 2;
const UA_Byte SECURITY_HEADER_SEC_FOOTER_ENABLED = 4;
const UA_Byte SECURITY_HEADER_FORCE_KEY_RESET = 8;
const UA_Byte DS_MESSAGEHEADER_DS_MSG_VALID = 1;
const UA_Byte DS_MESSAGEHEADER_FIELD_ENCODING_MASK = 6;
const UA_Byte DS_MESSAGEHEADER_SEQ_NR_ENABLED_MASK = 8;
const UA_Byte DS_MESSAGEHEADER_STATUS_ENABLED_MASK = 16;
const UA_Byte DS_MESSAGEHEADER_CONFIGMAJORVERSION_ENABLED_MASK = 32;
const UA_Byte DS_MESSAGEHEADER_CONFIGMINORVERSION_ENABLED_MASK = 64;
const UA_Byte DS_MESSAGEHEADER_FLAGS2_ENABLED_MASK = 128;
const UA_Byte DS_MESSAGEHEADER_DS_MESSAGE_TYPE_MASK = 15;
const UA_Byte DS_MESSAGEHEADER_TIMESTAMP_ENABLED_MASK = 16;
const UA_Byte DS_MESSAGEHEADER_PICOSECONDS_INCLUDED_MASK = 32;
const UA_Byte NM_SHIFT_LEN = 2;
const UA_Byte DS_MH_SHIFT_LEN = 1;

static UA_Boolean UA_NetworkMessage_ExtendedFlags1Enabled(const UA_NetworkMessage* src);
static UA_Boolean UA_NetworkMessage_ExtendedFlags2Enabled(const UA_NetworkMessage* src);
static UA_Boolean UA_DataSetMessageHeader_DataSetFlags2Enabled(const UA_DataSetMessageHeader* src);

UA_StatusCode
UA_NetworkMessage_updateBufferedMessage(UA_NetworkMessageOffsetBuffer *buffer) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    const UA_Byte *bufEnd = &buffer->buffer.data[buffer->buffer.length];
    for(size_t i = 0; i < buffer->offsetsSize; ++i) {
        UA_NetworkMessageOffset *nmo = &buffer->offsets[i];
        UA_Byte *bufPos = &buffer->buffer.data[nmo->offset];
        switch(nmo->contentType) {
            case UA_PUBSUB_OFFSETTYPE_DATASETMESSAGE_SEQUENCENUMBER:
            case UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_SEQUENCENUMBER:
                rv = UA_UInt16_encodeBinary(&nmo->content.sequenceNumber, &bufPos, bufEnd);
                nmo->content.sequenceNumber++;
                break;
            case UA_PUBSUB_OFFSETTYPE_PAYLOAD_DATAVALUE:
                rv = UA_DataValue_encodeBinary(&nmo->content.value, &bufPos, bufEnd);
                break;
            case UA_PUBSUB_OFFSETTYPE_PAYLOAD_DATAVALUE_EXTERNAL:
                rv = UA_DataValue_encodeBinary(*nmo->content.externalValue, &bufPos, bufEnd);
                break;
            case UA_PUBSUB_OFFSETTYPE_PAYLOAD_VARIANT:
                rv = UA_Variant_encodeBinary(&nmo->content.value.value, &bufPos, bufEnd);
                break;
            case UA_PUBSUB_OFFSETTYPE_PAYLOAD_VARIANT_EXTERNAL:
                rv = UA_Variant_encodeBinary(&(*nmo->content.externalValue)->value, &bufPos, bufEnd);
                break;
            case UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW:
                rv = UA_encodeBinaryInternal(nmo->content.value.value.data,
                                             nmo->content.value.value.type,
                                             &bufPos, &bufEnd, NULL, NULL);
                break;
            case UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW_EXTERNAL:
                rv = UA_encodeBinaryInternal((*nmo->content.externalValue)->value.data,
                                             (*nmo->content.externalValue)->value.type,
                                             &bufPos, &bufEnd, NULL, NULL);
                break;
            default:
                break; /* The other fields are assumed to not change between messages.
                        * Only used for RT decoding (not encoding). */
        }
    }
    return rv;
}

UA_StatusCode
UA_NetworkMessage_updateBufferedNwMessage(UA_NetworkMessageOffsetBuffer *buffer,
                                          const UA_ByteString *src, size_t *bufferPosition) {
    /* The offset buffer was not prepared */
    UA_NetworkMessage *nm = buffer->nm;
    if(!nm)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* The source string is too short */
    if(src->length < buffer->buffer.length + *bufferPosition)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Considering one DSM in RT TODO: Clarify multiple DSM */
    UA_DataSetMessage* dsm = nm->payload.dataSetPayload.dataSetMessages;

    size_t pos = 0;
    size_t payloadCounter = 0;
    UA_DataSetMessageHeader header;
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < buffer->offsetsSize; ++i) {
        pos = buffer->offsets[i].offset + *bufferPosition;
        switch(buffer->offsets[i].contentType) {
        case UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_FIELDENCDODING:
            rv = UA_DataSetMessageHeader_decodeBinary(src, &pos, &header);
            break;
        case UA_PUBSUB_OFFSETTYPE_PUBLISHERID:
            switch(nm->publisherIdType) {
            case UA_PUBLISHERIDTYPE_BYTE:
                rv = UA_Byte_decodeBinary(src, &pos, &nm->publisherId.byte);
                break;
            case UA_PUBLISHERIDTYPE_UINT16:
                rv = UA_UInt16_decodeBinary(src, &pos, &nm->publisherId.uint16);
                break;
            case UA_PUBLISHERIDTYPE_UINT32:
                rv = UA_UInt32_decodeBinary(src, &pos, &nm->publisherId.uint32);
                break;
            case UA_PUBLISHERIDTYPE_UINT64:
                rv = UA_UInt64_decodeBinary(src, &pos, &nm->publisherId.uint64);
                break;
            default:
                /* UA_PUBLISHERIDTYPE_STRING is not supported because of
                 * UA_PUBSUB_RT_FIXED_SIZE */
                return UA_STATUSCODE_BADNOTSUPPORTED;
            }
            break;
        case UA_PUBSUB_OFFSETTYPE_WRITERGROUPID:
            rv = UA_UInt16_decodeBinary(src, &pos, &nm->groupHeader.writerGroupId);
            break;
        case UA_PUBSUB_OFFSETTYPE_DATASETWRITERID:
            rv = UA_UInt16_decodeBinary(src, &pos,
                                        &nm->payloadHeader.dataSetPayloadHeader.dataSetWriterIds[0]); /* TODO */
            break;
        case UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_SEQUENCENUMBER:
            rv = UA_UInt16_decodeBinary(src, &pos, &nm->groupHeader.sequenceNumber);
            break;
        case UA_PUBSUB_OFFSETTYPE_DATASETMESSAGE_SEQUENCENUMBER:
            rv = UA_UInt16_decodeBinary(src, &pos, &dsm->header.dataSetMessageSequenceNr);
            break;
        case UA_PUBSUB_OFFSETTYPE_PAYLOAD_DATAVALUE:
            UA_DataValue_clear(&dsm->data.keyFrameData.dataSetFields[payloadCounter]);
            rv = UA_DataValue_decodeBinary(src, &pos,
                                           &dsm->data.keyFrameData.dataSetFields[payloadCounter]);
            payloadCounter++;
            break;
        case UA_PUBSUB_OFFSETTYPE_PAYLOAD_VARIANT:
            UA_Variant_clear(&dsm->data.keyFrameData.dataSetFields[payloadCounter].value);
            rv = UA_Variant_decodeBinary(src, &pos,
                                         &dsm->data.keyFrameData.dataSetFields[payloadCounter].value);
            dsm->data.keyFrameData.dataSetFields[payloadCounter].hasValue =
                (rv == UA_STATUSCODE_GOOD);
            payloadCounter++;
            break;
        case UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW:
            if(src->length < pos + buffer->offsets[i].content.rawMessageLength)
                return UA_STATUSCODE_BADDECODINGERROR;
            dsm->data.keyFrameData.rawFields.data = &src->data[pos];
            dsm->data.keyFrameData.rawFields.length = buffer->offsets[i].content.rawMessageLength;
            pos += buffer->offsets[i].content.rawMessageLength;
            payloadCounter++;
            break;
        default:
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }
        UA_CHECK_STATUS(rv, return rv);
    }

    *bufferPosition = pos;

    return rv;
}

static UA_StatusCode
UA_NetworkMessageHeader_encodeBinary(const UA_NetworkMessage *src, UA_Byte **bufPos,
                                     const UA_Byte *bufEnd) {
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

    UA_StatusCode rv = UA_Byte_encodeBinary(&v, bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);
    // ExtendedFlags1
    if(UA_NetworkMessage_ExtendedFlags1Enabled(src)) {
        v = (UA_Byte)src->publisherIdType;

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

        rv = UA_Byte_encodeBinary(&v, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);

        // ExtendedFlags2
        if(UA_NetworkMessage_ExtendedFlags2Enabled(src)) {
            v = (UA_Byte)src->networkMessageType;
            // shift left 2 bit
            v = (UA_Byte) (v << NM_SHIFT_LEN);

            if(src->chunkMessage)
                v |= NM_CHUNK_MESSAGE_MASK;

            if(src->promotedFieldsEnabled)
                v |= NM_PROMOTEDFIELDS_ENABLED_MASK;

            rv = UA_Byte_encodeBinary(&v, bufPos, bufEnd);
            UA_CHECK_STATUS(rv, return rv);
        }
    }

    // PublisherId
    if(src->publisherIdEnabled) {
        switch (src->publisherIdType) {
        case UA_PUBLISHERIDTYPE_BYTE:
            rv = UA_Byte_encodeBinary(&src->publisherId.byte, bufPos, bufEnd);
            break;

        case UA_PUBLISHERIDTYPE_UINT16:
            rv = UA_UInt16_encodeBinary(&src->publisherId.uint16, bufPos, bufEnd);
            break;

        case UA_PUBLISHERIDTYPE_UINT32:
            rv = UA_UInt32_encodeBinary(&src->publisherId.uint32, bufPos, bufEnd);
            break;

        case UA_PUBLISHERIDTYPE_UINT64:
            rv = UA_UInt64_encodeBinary(&src->publisherId.uint64, bufPos, bufEnd);
            break;

        case UA_PUBLISHERIDTYPE_STRING:
            rv = UA_String_encodeBinary(&src->publisherId.string, bufPos, bufEnd);
            break;

        default:
            rv = UA_STATUSCODE_BADINTERNALERROR;
            break;
        }
        UA_CHECK_STATUS(rv, return rv);
    }

    // DataSetClassId
    if(src->dataSetClassIdEnabled) {
        rv = UA_Guid_encodeBinary(&src->dataSetClassId, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_GroupHeader_encodeBinary(const UA_NetworkMessage* src, UA_Byte **bufPos,
                            const UA_Byte *bufEnd) {
    UA_Byte v = 0;
    if(src->groupHeader.writerGroupIdEnabled)
        v |= GROUP_HEADER_WRITER_GROUPID_ENABLED;

    if(src->groupHeader.groupVersionEnabled)
        v |= GROUP_HEADER_GROUP_VERSION_ENABLED;

    if(src->groupHeader.networkMessageNumberEnabled)
        v |= GROUP_HEADER_NM_NUMBER_ENABLED;

    if(src->groupHeader.sequenceNumberEnabled)
        v |= GROUP_HEADER_SEQUENCE_NUMBER_ENABLED;

    UA_StatusCode rv = UA_Byte_encodeBinary(&v, bufPos, bufEnd);

    if(src->groupHeader.writerGroupIdEnabled)
        rv |= UA_UInt16_encodeBinary(&src->groupHeader.writerGroupId, bufPos, bufEnd);

    if(src->groupHeader.groupVersionEnabled)
        rv |= UA_UInt32_encodeBinary(&src->groupHeader.groupVersion, bufPos, bufEnd);

    if(src->groupHeader.networkMessageNumberEnabled)
        rv |= UA_UInt16_encodeBinary(&src->groupHeader.networkMessageNumber, bufPos, bufEnd);

    if(src->groupHeader.sequenceNumberEnabled)
        rv |= UA_UInt16_encodeBinary(&src->groupHeader.sequenceNumber, bufPos, bufEnd);

    return rv;
}

static UA_StatusCode
UA_PayloadHeader_encodeBinary(const UA_NetworkMessage* src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd) {
    if(src->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    if(src->payloadHeader.dataSetPayloadHeader.dataSetWriterIds == NULL)
        return UA_STATUSCODE_BADENCODINGERROR;

    UA_Byte count = src->payloadHeader.dataSetPayloadHeader.count;

    UA_StatusCode rv = UA_Byte_encodeBinary(&count, bufPos, bufEnd);

    for(UA_Byte i = 0; i < count; i++) {
        UA_UInt16 dswId = src->payloadHeader.dataSetPayloadHeader.dataSetWriterIds[i];
        rv |= UA_UInt16_encodeBinary(&dswId, bufPos, bufEnd);
    }

    return rv;
}

static UA_StatusCode
UA_ExtendedNetworkMessageHeader_encodeBinary(const UA_NetworkMessage* src, UA_Byte **bufPos,
                                             const UA_Byte *bufEnd) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    if(src->timestampEnabled)
        rv |= UA_DateTime_encodeBinary(&src->timestamp, bufPos, bufEnd);

    if(src->picosecondsEnabled)
        rv |= UA_UInt16_encodeBinary(&src->picoseconds, bufPos, bufEnd);

    if(src->promotedFieldsEnabled) {
        /* Size (calculate & encode) */
        UA_UInt16 pfSize = 0;
        for(UA_UInt16 i = 0; i < src->promotedFieldsSize; i++)
            pfSize = (UA_UInt16)(pfSize + UA_Variant_calcSizeBinary(&src->promotedFields[i]));
        rv |= UA_UInt16_encodeBinary(&pfSize, bufPos, bufEnd);

        for(UA_UInt16 i = 0; i < src->promotedFieldsSize; i++)
            rv |= UA_Variant_encodeBinary(&src->promotedFields[i], bufPos, bufEnd);
    }

    return rv;
}

static UA_StatusCode
UA_SecurityHeader_encodeBinary(const UA_NetworkMessage* src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd) {
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

    UA_StatusCode rv = UA_Byte_encodeBinary(&v, bufPos, bufEnd);

    /* SecurityTokenId */
    rv |= UA_UInt32_encodeBinary(&src->securityHeader.securityTokenId, bufPos, bufEnd);

    /* NonceLength */
    UA_Byte nonceLength = (UA_Byte)src->securityHeader.messageNonceSize;
    rv |= UA_Byte_encodeBinary(&nonceLength, bufPos, bufEnd);

    /* MessageNonce */
    for(size_t i = 0; i < src->securityHeader.messageNonceSize; i++) {
        rv |= UA_Byte_encodeBinary(&src->securityHeader.messageNonce[i],
                                   bufPos, bufEnd);
    }

    /* SecurityFooterSize */
    if(src->securityHeader.securityFooterEnabled) {
        rv |= UA_UInt16_encodeBinary(&src->securityHeader.securityFooterSize,
                                     bufPos, bufEnd);
    }

    return rv;
}

UA_StatusCode
UA_NetworkMessage_encodeHeaders(const UA_NetworkMessage* src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd) {
    /* Message Header */
    UA_StatusCode rv = UA_NetworkMessageHeader_encodeBinary(src, bufPos, bufEnd);

    /* Group Header */
    if(src->groupHeaderEnabled)
        rv |= UA_GroupHeader_encodeBinary(src, bufPos, bufEnd);

    /* Payload Header */
    if(src->payloadHeaderEnabled)
        rv |= UA_PayloadHeader_encodeBinary(src, bufPos, bufEnd);

    /* Extended Network Message Header */
    rv |= UA_ExtendedNetworkMessageHeader_encodeBinary(src, bufPos, bufEnd);

    /* SecurityHeader */
    if(src->securityEnabled)
        rv |= UA_SecurityHeader_encodeBinary(src, bufPos, bufEnd);

    return rv;
}


UA_StatusCode
UA_NetworkMessage_encodePayload(const UA_NetworkMessage* src, UA_Byte **bufPos,
                                const UA_Byte *bufEnd) {
    UA_StatusCode rv;

    // Payload
    if(src->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    UA_Byte count = 1;

    if(src->payloadHeaderEnabled) {
        count = src->payloadHeader.dataSetPayloadHeader.count;
        if(count > 1) {
            for(UA_Byte i = 0; i < count; i++) {
                /* Calculate the size, if not specified */
                UA_UInt16 sz = 0;
                if((src->payload.dataSetPayload.sizes != NULL) &&
                   (src->payload.dataSetPayload.sizes[i] != 0)) {
                    sz = src->payload.dataSetPayload.sizes[i];
                } else {
                    UA_DataSetMessage *dsm = &src->payload.dataSetPayload.dataSetMessages[i];
                    sz = (UA_UInt16)UA_DataSetMessage_calcSizeBinary(dsm, NULL, 0);
                }

                rv = UA_UInt16_encodeBinary(&sz, bufPos, bufEnd);
                UA_CHECK_STATUS(rv, return rv);
            }
        }
    }

    for(UA_Byte i = 0; i < count; i++) {
        UA_DataSetMessage *dsm = &src->payload.dataSetPayload.dataSetMessages[i];
        rv = UA_DataSetMessage_encodeBinary(dsm, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

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
UA_NetworkMessage_encodeBinary(const UA_NetworkMessage* src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd, UA_Byte **dataToEncryptStart) {
    UA_StatusCode rv = UA_NetworkMessage_encodeHeaders(src, bufPos, bufEnd);

    if(dataToEncryptStart)
        *dataToEncryptStart = *bufPos;

    rv |= UA_NetworkMessage_encodePayload(src, bufPos, bufEnd);
    rv |= UA_NetworkMessage_encodeFooters(src, bufPos, bufEnd);
    return rv;
}

UA_StatusCode
UA_NetworkMessageHeader_decodeBinary(const UA_ByteString *src, size_t *offset, UA_NetworkMessage *dst) {
    UA_Byte decoded = 0;
    UA_StatusCode rv = UA_Byte_decodeBinary(src, offset, &decoded);
    UA_CHECK_STATUS(rv, return rv);

    dst->version = decoded & NM_VERSION_MASK;

    if((decoded & NM_PUBLISHER_ID_ENABLED_MASK) != 0)
        dst->publisherIdEnabled = true;

    if((decoded & NM_GROUP_HEADER_ENABLED_MASK) != 0)
        dst->groupHeaderEnabled = true;

    if((decoded & NM_PAYLOAD_HEADER_ENABLED_MASK) != 0)
        dst->payloadHeaderEnabled = true;

    if((decoded & NM_EXTENDEDFLAGS1_ENABLED_MASK) != 0) {
        decoded = 0;
        rv = UA_Byte_decodeBinary(src, offset, &decoded);
        UA_CHECK_STATUS(rv, return rv);

        dst->publisherIdType = (UA_PublisherIdType)(decoded & NM_PUBLISHER_ID_MASK);
        if((decoded & NM_DATASET_CLASSID_ENABLED_MASK) != 0)
            dst->dataSetClassIdEnabled = true;

        if((decoded & NM_SECURITY_ENABLED_MASK) != 0)
            dst->securityEnabled = true;

        if((decoded & NM_TIMESTAMP_ENABLED_MASK) != 0)
            dst->timestampEnabled = true;

        if((decoded & NM_PICOSECONDS_ENABLED_MASK) != 0)
            dst->picosecondsEnabled = true;

        if((decoded & NM_EXTENDEDFLAGS2_ENABLED_MASK) != 0) {
            decoded = 0;
            rv = UA_Byte_decodeBinary(src, offset, &decoded);
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
        switch (dst->publisherIdType) {
            case UA_PUBLISHERIDTYPE_BYTE:
                rv = UA_Byte_decodeBinary(src, offset, &dst->publisherId.byte);
                break;

            case UA_PUBLISHERIDTYPE_UINT16:
                rv = UA_UInt16_decodeBinary(src, offset, &dst->publisherId.uint16);
                break;

            case UA_PUBLISHERIDTYPE_UINT32:
                rv = UA_UInt32_decodeBinary(src, offset, &dst->publisherId.uint32);
                break;

            case UA_PUBLISHERIDTYPE_UINT64:
                rv = UA_UInt64_decodeBinary(src, offset, &dst->publisherId.uint64);
                break;

            case UA_PUBLISHERIDTYPE_STRING:
                rv = UA_String_decodeBinary(src, offset, &dst->publisherId.string);
                break;

            default:
                rv = UA_STATUSCODE_BADINTERNALERROR;
                break;
        }
        UA_CHECK_STATUS(rv, return rv);
    }

    if(dst->dataSetClassIdEnabled) {
        rv = UA_Guid_decodeBinary(src, offset, &dst->dataSetClassId);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_GroupHeader_decodeBinary(const UA_ByteString *src, size_t *offset,
                         UA_NetworkMessage* dst) {
    UA_Byte decoded = 0;
    UA_StatusCode rv = UA_Byte_decodeBinary(src, offset, &decoded);

    if((decoded & GROUP_HEADER_WRITER_GROUPID_ENABLED) != 0) {
        dst->groupHeader.writerGroupIdEnabled = true;
        rv |= UA_UInt16_decodeBinary(src, offset, &dst->groupHeader.writerGroupId);
    }

    if((decoded & GROUP_HEADER_GROUP_VERSION_ENABLED) != 0) {
        dst->groupHeader.groupVersionEnabled = true;
        rv |= UA_UInt32_decodeBinary(src, offset, &dst->groupHeader.groupVersion);
    }

    if((decoded & GROUP_HEADER_NM_NUMBER_ENABLED) != 0) {
        dst->groupHeader.networkMessageNumberEnabled = true;
        rv |= UA_UInt16_decodeBinary(src, offset, &dst->groupHeader.networkMessageNumber);
    }

    if((decoded & GROUP_HEADER_SEQUENCE_NUMBER_ENABLED) != 0) {
        dst->groupHeader.sequenceNumberEnabled = true;
        rv |= UA_UInt16_decodeBinary(src, offset, &dst->groupHeader.sequenceNumber);
    }

    return rv;
}

static UA_StatusCode
UA_PayloadHeader_decodeBinary(const UA_ByteString *src, size_t *offset,
                              UA_NetworkMessage* dst) {

    if(dst->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    UA_StatusCode rv = UA_Byte_decodeBinary(src, offset, &dst->payloadHeader.dataSetPayloadHeader.count);
    UA_CHECK_STATUS(rv, return rv);

    dst->payloadHeader.dataSetPayloadHeader.dataSetWriterIds =
        (UA_UInt16 *)UA_Array_new(dst->payloadHeader.dataSetPayloadHeader.count,
                                  &UA_TYPES[UA_TYPES_UINT16]);
    for(UA_Byte i = 0; i < dst->payloadHeader.dataSetPayloadHeader.count; i++) {
        rv = UA_UInt16_decodeBinary(src, offset,
                                    &dst->payloadHeader.dataSetPayloadHeader.dataSetWriterIds[i]);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_ExtendedNetworkMessageHeader_decodeBinary(const UA_ByteString *src, size_t *offset,
                            UA_NetworkMessage* dst) {
    UA_StatusCode rv;

    // Timestamp
    if(dst->timestampEnabled) {
        rv = UA_DateTime_decodeBinary(src, offset, &dst->timestamp);
        UA_CHECK_STATUS(rv, goto error);
    }

    // Picoseconds
    if(dst->picosecondsEnabled) {
        rv = UA_UInt16_decodeBinary(src, offset, &dst->picoseconds);
        UA_CHECK_STATUS(rv, goto error);
    }

    // PromotedFields
    if(dst->promotedFieldsEnabled) {
        // Size
        UA_UInt16 promotedFieldsSize = 0;
        rv = UA_UInt16_decodeBinary(src, offset, &promotedFieldsSize);
        UA_CHECK_STATUS(rv, goto error);

        // promotedFieldsSize: here size in Byte, not the number of objects!
        if(promotedFieldsSize > 0) {
            // store offset, later compared with promotedFieldsSize
            size_t offsetEnd = (*offset) + promotedFieldsSize;

            unsigned int counter = 0;
            do {
                if(counter == 0) {
                    dst->promotedFields = (UA_Variant*)UA_malloc(UA_TYPES[UA_TYPES_VARIANT].memSize);
                    UA_CHECK_MEM(dst->promotedFields,
                                 return UA_STATUSCODE_BADOUTOFMEMORY);
                    // set promotedFieldsSize to the number of objects
                    dst->promotedFieldsSize = (UA_UInt16) (counter + 1);
                } else {
                    dst->promotedFields = (UA_Variant*)
                        UA_realloc(dst->promotedFields,
                                   (size_t) UA_TYPES[UA_TYPES_VARIANT].memSize * (counter + 1));
                    UA_CHECK_MEM(dst->promotedFields,
                                 return UA_STATUSCODE_BADOUTOFMEMORY);
                    // set promotedFieldsSize to the number of objects
                    dst->promotedFieldsSize = (UA_UInt16) (counter + 1);
                }

                UA_Variant_init(&dst->promotedFields[counter]);
                rv = UA_Variant_decodeBinary(src, offset, &dst->promotedFields[counter]);
                UA_CHECK_STATUS(rv, goto error);

                counter++;
            } while ((*offset) < offsetEnd);
        }
    }
    return UA_STATUSCODE_GOOD;

error:
    if(dst->promotedFields) {
        UA_free(dst->promotedFields);
        dst->promotedFields = NULL;
    }
    return rv;
}

static UA_StatusCode
UA_SecurityHeader_decodeBinary(const UA_ByteString *src, size_t *offset,
                              UA_NetworkMessage* dst) {
    UA_Byte decoded = 0;
    // SecurityFlags
    decoded = 0;
    UA_StatusCode rv = UA_Byte_decodeBinary(src, offset, &decoded);
    UA_CHECK_STATUS(rv, return rv);

    if((decoded & SECURITY_HEADER_NM_SIGNED) != 0)
        dst->securityHeader.networkMessageSigned = true;

    if((decoded & SECURITY_HEADER_NM_ENCRYPTED) != 0)
        dst->securityHeader.networkMessageEncrypted = true;

    if((decoded & SECURITY_HEADER_SEC_FOOTER_ENABLED) != 0)
        dst->securityHeader.securityFooterEnabled = true;

    if((decoded & SECURITY_HEADER_FORCE_KEY_RESET) != 0)
        dst->securityHeader.forceKeyReset = true;

    // SecurityTokenId
    rv = UA_UInt32_decodeBinary(src, offset, &dst->securityHeader.securityTokenId);
    UA_CHECK_STATUS(rv, return rv);

    // MessageNonce
    UA_Byte nonceLength;
    rv = UA_Byte_decodeBinary(src, offset, &nonceLength);
    UA_CHECK_STATUS(rv, return rv);
    if(nonceLength > UA_NETWORKMESSAGE_MAX_NONCE_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    if(nonceLength > 0) {
        dst->securityHeader.messageNonceSize = nonceLength;
        for(UA_Byte i = 0; i < nonceLength; i++) {
            rv = UA_Byte_decodeBinary(src, offset,
                                      &dst->securityHeader.messageNonce[i]);
            UA_CHECK_STATUS(rv, return rv);
        }
    }

    // SecurityFooterSize
    if(dst->securityHeader.securityFooterEnabled) {
        rv = UA_UInt16_decodeBinary(src, offset, &dst->securityHeader.securityFooterSize);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NetworkMessage_decodeHeaders(const UA_ByteString *src, size_t *offset, UA_NetworkMessage *dst) {

    UA_StatusCode rv = UA_NetworkMessageHeader_decodeBinary(src, offset, dst);
    UA_CHECK_STATUS(rv, return rv);

    if(dst->groupHeaderEnabled) {
        rv = UA_GroupHeader_decodeBinary(src, offset, dst);
        UA_CHECK_STATUS(rv, return rv);
    }

    if(dst->payloadHeaderEnabled) {
        rv = UA_PayloadHeader_decodeBinary(src, offset, dst);
        UA_CHECK_STATUS(rv, return rv);
    }

    rv = UA_ExtendedNetworkMessageHeader_decodeBinary(src, offset, dst);
    UA_CHECK_STATUS(rv, return rv);

    if(dst->securityEnabled) {
        rv = UA_SecurityHeader_decodeBinary(src, offset, dst);
        UA_CHECK_STATUS(rv, return rv);
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NetworkMessage_decodePayload(const UA_ByteString *src, size_t *offset, UA_NetworkMessage *dst, const UA_DataTypeArray *customTypes, UA_DataSetMetaDataType *dsm) {

    // Payload
    if(dst->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    UA_StatusCode rv;

    UA_Byte count = 1;
    if(dst->payloadHeaderEnabled) {
        count = dst->payloadHeader.dataSetPayloadHeader.count;
        if(count > 1) {
            dst->payload.dataSetPayload.sizes = (UA_UInt16 *)UA_Array_new(count, &UA_TYPES[UA_TYPES_UINT16]);
            for(UA_Byte i = 0; i < count; i++) {
                rv = UA_UInt16_decodeBinary(src, offset, &dst->payload.dataSetPayload.sizes[i]);
                UA_CHECK_STATUS(rv, return rv);
            }
        }
    }

    dst->payload.dataSetPayload.dataSetMessages = (UA_DataSetMessage*)
        UA_calloc(count, sizeof(UA_DataSetMessage));
    UA_CHECK_MEM(dst->payload.dataSetPayload.dataSetMessages,
                 return UA_STATUSCODE_BADOUTOFMEMORY);

    if(count == 1)
        rv = UA_DataSetMessage_decodeBinary(src, offset,
                                            &dst->payload.dataSetPayload.dataSetMessages[0],
                                            0, customTypes, dsm);
    else {
        for(UA_Byte i = 0; i < count; i++) {
            rv = UA_DataSetMessage_decodeBinary(src, offset,
                                                &dst->payload.dataSetPayload.dataSetMessages[i],
                                                dst->payload.dataSetPayload.sizes[i], customTypes,
                                                dsm);
        }
    }
    UA_CHECK_STATUS(rv, return rv);

    return UA_STATUSCODE_GOOD;

    /**
     * TODO: check if making the cleanup to free its own allocated memory is better,
     *       currently the free happens in a parent context
     */
}

UA_StatusCode
UA_NetworkMessage_decodeFooters(const UA_ByteString *src, size_t *offset,
                                UA_NetworkMessage *dst) {
    if(!dst->securityEnabled)
        return UA_STATUSCODE_GOOD;

    // SecurityFooter
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    if(dst->securityHeader.securityFooterEnabled &&
       dst->securityHeader.securityFooterSize > 0) {
        rv = UA_ByteString_allocBuffer(&dst->securityFooter,
                                       dst->securityHeader.securityFooterSize);
        UA_CHECK_STATUS(rv, return rv);

        for(UA_UInt16 i = 0; i < dst->securityHeader.securityFooterSize; i++) {
            rv |= UA_Byte_decodeBinary(src, offset, &dst->securityFooter.data[i]);
        }
    }
    return rv;
}

UA_StatusCode
UA_NetworkMessage_decodeBinary(const UA_ByteString *src, size_t *offset,
                               UA_NetworkMessage* dst, const UA_DataTypeArray *customTypes) {
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

    UA_StatusCode rv = UA_NetworkMessage_decodeHeaders(src, offset, dst);
    UA_CHECK_STATUS(rv, return rv);

    rv = UA_NetworkMessage_decodePayload(src, offset, dst, customTypes, NULL);
    UA_CHECK_STATUS(rv, return rv);

    rv = UA_NetworkMessage_decodeFooters(src, offset, dst);
    UA_CHECK_STATUS(rv, return rv);

    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
increaseOffsetArray(UA_NetworkMessageOffsetBuffer *offsetBuffer) {
    UA_NetworkMessageOffset *tmpOffsets = (UA_NetworkMessageOffset *)
        UA_realloc(offsetBuffer->offsets, sizeof(UA_NetworkMessageOffset) * (offsetBuffer->offsetsSize + (size_t)1));
    UA_CHECK_MEM(tmpOffsets, return false);
    memset(&tmpOffsets[offsetBuffer->offsetsSize], 0, sizeof(UA_NetworkMessageOffset));
    offsetBuffer->offsets = tmpOffsets;
    offsetBuffer->offsetsSize++;
    return true;
}

size_t
UA_NetworkMessage_calcSizeBinary(UA_NetworkMessage *p,
                                 UA_NetworkMessageOffsetBuffer *offsetBuffer) {
    size_t size = 1; /* byte */
    if(UA_NetworkMessage_ExtendedFlags1Enabled(p)) {
        size += 1; /* byte */
        if(UA_NetworkMessage_ExtendedFlags2Enabled(p))
            size += 1; /* byte */
    }

    if(p->publisherIdEnabled) {
        if(offsetBuffer) {
            size_t pos = offsetBuffer->offsetsSize;
            if(!increaseOffsetArray(offsetBuffer))
                return 0;

            offsetBuffer->offsets[pos].offset = size;
            offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_PUBLISHERID;
        }

        switch(p->publisherIdType) {
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
                size += UA_String_calcSizeBinary(&p->publisherId.string);
                break;
        }
    }

    if(p->dataSetClassIdEnabled)
        size += 16; /* guid */

    // Group Header
    if(p->groupHeaderEnabled) {
        size += 1; /* byte */

        if(p->groupHeader.writerGroupIdEnabled) {
            if(offsetBuffer) {
                size_t pos = offsetBuffer->offsetsSize;
                if(!increaseOffsetArray(offsetBuffer))
                    return 0;

                offsetBuffer->offsets[pos].offset = size;
                offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_WRITERGROUPID;
            }
            size += 2; /* UA_UInt16_calcSizeBinary(&p->groupHeader.writerGroupId) */
        }

        if(p->groupHeader.groupVersionEnabled)
            size += 4; /* UA_UInt32_calcSizeBinary(&p->groupHeader.groupVersion) */

        if(p->groupHeader.networkMessageNumberEnabled) {
            size += 2; /* UA_UInt16_calcSizeBinary(&p->groupHeader.networkMessageNumber) */
        }

        if(p->groupHeader.sequenceNumberEnabled){
            if(offsetBuffer){
                size_t pos = offsetBuffer->offsetsSize;
                if(!increaseOffsetArray(offsetBuffer))
                    return 0;
                offsetBuffer->offsets[pos].offset = size;
                offsetBuffer->offsets[pos].content.sequenceNumber =
                    p->groupHeader.sequenceNumber;
                offsetBuffer->offsets[pos].contentType =
                    UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_SEQUENCENUMBER;
            }
            size += 2; /* UA_UInt16_calcSizeBinary(&p->groupHeader.sequenceNumber) */
        }
    }

    // Payload Header
    if(p->payloadHeaderEnabled) {
        if(p->networkMessageType != UA_NETWORKMESSAGE_DATASET)
            return 0; /* not implemented */
        if(!p->payloadHeader.dataSetPayloadHeader.dataSetWriterIds)
            return 0; /* no dataSetWriterIds given! */
        size += 1; /* p->payloadHeader.dataSetPayloadHeader.count */
        if(offsetBuffer) {
            size_t pos = offsetBuffer->offsetsSize;
            if(!increaseOffsetArray(offsetBuffer))
                return 0;
            offsetBuffer->offsets[pos].offset = size;
            offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_DATASETWRITERID;
        }
        size += (size_t)(2LU * p->payloadHeader.dataSetPayloadHeader.count); /* uint16 */
    }

    if(p->timestampEnabled) {
        if(offsetBuffer){
            size_t pos = offsetBuffer->offsetsSize;
            if(!increaseOffsetArray(offsetBuffer))
                return 0;
            offsetBuffer->offsets[pos].offset = size;
            offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_TIMESTAMP;
        }
        size += 8; /* UA_DateTime_calcSizeBinary(&p->timestamp) */
    }

    if(p->picosecondsEnabled){
        if(offsetBuffer) {
            size_t pos = offsetBuffer->offsetsSize;
            if(!increaseOffsetArray(offsetBuffer))
                return 0;
            offsetBuffer->offsets[pos].offset = size;
            offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_TIMESTAMP_PICOSECONDS;
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
    UA_Byte count = 1;
    if(p->payloadHeaderEnabled) {
        count = p->payloadHeader.dataSetPayloadHeader.count;
        if(count > 1)
            size += (size_t)(2LU * count); /* uint16 */
    }
    for(size_t i = 0; i < count; i++) {
        UA_DataSetMessage *dsm = &p->payload.dataSetPayload.dataSetMessages[i];
        size = UA_DataSetMessage_calcSizeBinary(dsm, offsetBuffer, size);
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
        if(p->payloadHeader.dataSetPayloadHeader.dataSetWriterIds &&
           p->payloadHeader.dataSetPayloadHeader.dataSetWriterIds != UA_EMPTY_ARRAY_SENTINEL)
            UA_free(p->payloadHeader.dataSetPayloadHeader.dataSetWriterIds);

        if(p->payload.dataSetPayload.sizes)
            UA_free(p->payload.dataSetPayload.sizes);

        if(p->payload.dataSetPayload.dataSetMessages) {
            UA_Byte count = 1;
            if(p->payloadHeaderEnabled)
                count = p->payloadHeader.dataSetPayloadHeader.count;
            for(size_t i = 0; i < count; i++)
                UA_DataSetMessage_clear(&p->payload.dataSetPayload.dataSetMessages[i]);
            UA_free(p->payload.dataSetPayload.dataSetMessages);
        }
    }

    UA_ByteString_clear(&p->securityFooter);
    UA_String_clear(&p->messageId);

    if(p->publisherIdEnabled &&
       p->publisherIdType == UA_PUBLISHERIDTYPE_STRING)
       UA_String_clear(&p->publisherId.string);

    memset(p, 0, sizeof(UA_NetworkMessage));
}

UA_Boolean
UA_NetworkMessage_ExtendedFlags1Enabled(const UA_NetworkMessage* src) {
    if(src->publisherIdType != UA_PUBLISHERIDTYPE_BYTE ||
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
    // DataSetFlags1
    v = (UA_Byte)src->fieldEncoding;
    // shift left 1 bit
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

    // DataSetFlags2
    if(UA_DataSetMessageHeader_DataSetFlags2Enabled(src)) {
        v = (UA_Byte)src->dataSetMessageType;

        if(src->timestampEnabled)
            v |= DS_MESSAGEHEADER_TIMESTAMP_ENABLED_MASK;

        if(src->picoSecondsIncluded)
            v |= DS_MESSAGEHEADER_PICOSECONDS_INCLUDED_MASK;

        rv = UA_Byte_encodeBinary(&v, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    // DataSetMessageSequenceNr
    if(src->dataSetMessageSequenceNrEnabled) {
        rv = UA_UInt16_encodeBinary(&src->dataSetMessageSequenceNr, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    // Timestamp
    if(src->timestampEnabled) {
        rv = UA_DateTime_encodeBinary(&src->timestamp, bufPos, bufEnd); /* UtcTime */
        UA_CHECK_STATUS(rv, return rv);
    }

    // PicoSeconds
    if(src->picoSecondsIncluded) {
        rv = UA_UInt16_encodeBinary(&src->picoSeconds, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    // Status
    if(src->statusEnabled) {
        rv = UA_UInt16_encodeBinary(&src->status, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    // ConfigVersionMajorVersion
    if(src->configVersionMajorVersionEnabled) {
        rv = UA_UInt32_encodeBinary(&src->configVersionMajorVersion, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    // ConfigVersionMinorVersion
    if(src->configVersionMinorVersionEnabled) {
        rv = UA_UInt32_encodeBinary(&src->configVersionMinorVersion, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION

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
#endif

UA_StatusCode
UA_DataSetMessageHeader_decodeBinary(const UA_ByteString *src, size_t *offset,
                                     UA_DataSetMessageHeader* dst) {
    memset(dst, 0, sizeof(UA_DataSetMessageHeader));
    UA_Byte v = 0;
    UA_StatusCode rv = UA_Byte_decodeBinary(src, offset, &v);
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
        v = 0;
        rv = UA_Byte_decodeBinary(src, offset, &v);
        UA_CHECK_STATUS(rv, return rv);

        dst->dataSetMessageType = (UA_DataSetMessageType)(v & DS_MESSAGEHEADER_DS_MESSAGE_TYPE_MASK);

        if((v & DS_MESSAGEHEADER_TIMESTAMP_ENABLED_MASK) != 0)
            dst->timestampEnabled = true;

        if((v & DS_MESSAGEHEADER_PICOSECONDS_INCLUDED_MASK) != 0)
            dst->picoSecondsIncluded = true;
    } else {
        dst->dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
        dst->picoSecondsIncluded = false;
    }

    if(dst->dataSetMessageSequenceNrEnabled) {
        rv = UA_UInt16_decodeBinary(src, offset, &dst->dataSetMessageSequenceNr);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dst->dataSetMessageSequenceNr = 0;
    }

    if(dst->timestampEnabled) {
        rv = UA_DateTime_decodeBinary(src, offset, &dst->timestamp); /* UtcTime */
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dst->timestamp = 0;
    }

    if(dst->picoSecondsIncluded) {
        rv = UA_UInt16_decodeBinary(src, offset, &dst->picoSeconds);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dst->picoSeconds = 0;
    }

    if(dst->statusEnabled) {
        rv = UA_UInt16_decodeBinary(src, offset, &dst->status);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dst->status = 0;
    }

    if(dst->configVersionMajorVersionEnabled) {
        rv = UA_UInt32_decodeBinary(src, offset, &dst->configVersionMajorVersion);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dst->configVersionMajorVersion = 0;
    }

    if(dst->configVersionMinorVersionEnabled) {
        rv = UA_UInt32_decodeBinary(src, offset, &dst->configVersionMinorVersion);
        UA_CHECK_STATUS(rv, return rv);
    } else {
        dst->configVersionMinorVersion = 0;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_DataSetMessage_encodeBinary(const UA_DataSetMessage* src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd) {
    UA_StatusCode rv = UA_DataSetMessageHeader_encodeBinary(&src->header, bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

    if(src->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
        /* Heartbeat: "DataSetMessage is a key frame that only contains header
         * information" */
        if(src->data.keyFrameData.fieldCount == 0)
            return rv;

        if(src->header.fieldEncoding != UA_FIELDENCODING_RAWDATA) {
            rv = UA_UInt16_encodeBinary(&src->data.keyFrameData.fieldCount,
                                        bufPos, bufEnd);
            UA_CHECK_STATUS(rv, return rv);
        }

        for(UA_UInt16 i = 0; i < src->data.keyFrameData.fieldCount; i++) {
            const UA_DataValue *v = &src->data.keyFrameData.dataSetFields[i];

            if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
                rv = UA_Variant_encodeBinary(&v->value, bufPos, bufEnd);
            } else if(src->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
                UA_FieldMetaData *fmd =
                    &src->data.keyFrameData.dataSetMetaDataType->fields[i];
                if(fmd->maxStringLength != 0 &&
                   (v->value.type->typeKind == UA_DATATYPEKIND_STRING ||
                    v->value.type->typeKind == UA_DATATYPEKIND_BYTESTRING)){
                    rv = UA_encodeBinaryInternal(v->value.data, v->value.type,
                                                 bufPos, &bufEnd, NULL, NULL);
                    size_t lengthDifference = fmd->maxStringLength -
                        ((UA_String *)v->value.data)->length;
                    memset(*bufPos, 0, lengthDifference);
                    *bufPos += lengthDifference;
                } else {
                    /* padding not yet supported for strings as part of structures */
                    rv = UA_encodeBinaryInternal(v->value.data, v->value.type,
                                                 bufPos, &bufEnd, NULL, NULL);
                }
            } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
                rv = UA_DataValue_encodeBinary(v, bufPos, bufEnd);
            }

            UA_CHECK_STATUS(rv, return rv);
        }
    } else if(src->header.dataSetMessageType == UA_DATASETMESSAGE_DATADELTAFRAME) {
        // Encode Delta Frame
        // Here the FieldCount is always present
        rv = UA_UInt16_encodeBinary(&src->data.deltaFrameData.fieldCount, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);

        if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
            for(UA_UInt16 i = 0; i < src->data.deltaFrameData.fieldCount; i++) {
                rv = UA_UInt16_encodeBinary(&src->data.deltaFrameData.deltaFrameFields[i].fieldIndex, bufPos, bufEnd);
                UA_CHECK_STATUS(rv, return rv);

                rv = UA_Variant_encodeBinary(&src->data.deltaFrameData.deltaFrameFields[i].fieldValue.value, bufPos, bufEnd);
                UA_CHECK_STATUS(rv, return rv);
            }
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
            return UA_STATUSCODE_BADNOTIMPLEMENTED;
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
            for(UA_UInt16 i = 0; i < src->data.deltaFrameData.fieldCount; i++) {
                rv = UA_UInt16_encodeBinary(&src->data.deltaFrameData.deltaFrameFields[i].fieldIndex, bufPos, bufEnd);
                UA_CHECK_STATUS(rv, return rv);

                rv = UA_DataValue_encodeBinary(&src->data.deltaFrameData.deltaFrameFields[i].fieldValue, bufPos, bufEnd);
                UA_CHECK_STATUS(rv, return rv);
            }
        }
    } else if(src->header.dataSetMessageType != UA_DATASETMESSAGE_KEEPALIVE) {
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    /* if the configuredSize is set -> padd the message with 0. */
    if(src->configuredSize > 0 && src->header.dataSetMessageValid) {
        size_t padding = (size_t)(bufEnd - *bufPos);
        /* Set the bytes to 0*/
        memset(*bufPos, 0, padding);
        /* move the bufpos accordingly*/
        *bufPos += padding;
    }
    /* Keep-Alive Message contains no Payload Data */
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_DataSetMessage_decodeBinary(const UA_ByteString *src, size_t *offset, UA_DataSetMessage* dst, UA_UInt16 dsmSize, const UA_DataTypeArray *customTypes, UA_DataSetMetaDataType *dsm) {
    size_t initialOffset = *offset;
    memset(dst, 0, sizeof(UA_DataSetMessage));
    UA_StatusCode rv = UA_DataSetMessageHeader_decodeBinary(src, offset, &dst->header);
    UA_CHECK_STATUS(rv, return rv);

    if(dst->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
        if(*offset == src->length) {
            /* Messages ends after the header --> Heartbeat */
            return rv;
        }

        switch(dst->header.fieldEncoding) {
            case UA_FIELDENCODING_VARIANT:
                rv = UA_UInt16_decodeBinary(src, offset, &dst->data.keyFrameData.fieldCount);
                UA_CHECK_STATUS(rv, return rv);
                dst->data.keyFrameData.dataSetFields =
                    (UA_DataValue *)UA_Array_new(dst->data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
                for(UA_UInt16 i = 0; i < dst->data.keyFrameData.fieldCount; i++) {
                    UA_DataValue_init(&dst->data.keyFrameData.dataSetFields[i]);
                    rv = UA_decodeBinaryInternal(src, offset, &dst->data.keyFrameData.dataSetFields[i].value, &UA_TYPES[UA_TYPES_VARIANT], customTypes);
                    UA_CHECK_STATUS(rv, return rv);

                    dst->data.keyFrameData.dataSetFields[i].hasValue = true;
                }
                break;
            case UA_FIELDENCODING_DATAVALUE:
                rv = UA_UInt16_decodeBinary(src, offset, &dst->data.keyFrameData.fieldCount);
                UA_CHECK_STATUS(rv, return rv);
                dst->data.keyFrameData.dataSetFields =
                    (UA_DataValue *)UA_Array_new(dst->data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
                for(UA_UInt16 i = 0; i < dst->data.keyFrameData.fieldCount; i++) {
                    rv = UA_decodeBinaryInternal(src, offset,
                                                 &dst->data.keyFrameData.dataSetFields[i],
                                                 &UA_TYPES[UA_TYPES_DATAVALUE], customTypes);
                    UA_CHECK_STATUS(rv, return rv);
                }
                break;
            case UA_FIELDENCODING_RAWDATA:
                dst->data.keyFrameData.rawFields.data = &src->data[*offset];
                dst->data.keyFrameData.rawFields.length = dsmSize;
                if(dsmSize == 0) {
                    size_t dsmOffset = *offset;
                    if(dsm != NULL) {
                        // calculate the length of the DSM-Payload for a single DSM
                        dst->data.keyFrameData.fieldCount = (UA_UInt16)dsm->fieldsSize;
                        for(size_t i = 0; i < dsm->fieldsSize; i++) {
                            /* TODO The datatype reference should be part of the internal
                             * pubsub configuration to avoid the time-expensive lookup */
                            const UA_DataType *type =
                                UA_findDataTypeWithCustom(&dsm->fields[i].dataType, customTypes);
                            if(!type)
                                return UA_STATUSCODE_BADDECODINGERROR;;

                            if(type->typeKind == UA_DATATYPEKIND_STRING ||
                               type->typeKind == UA_DATATYPEKIND_BYTESTRING) {
                                /* Strings need to have a MaxStringLength */
                                if(dsm->fields[i].maxStringLength == 0)
                                    return UA_STATUSCODE_BADDECODINGERROR;
                                /* Cannot handle an array of strings */
                                if(dsm->fields[i].arrayDimensionsSize > 0)
                                    return UA_STATUSCODE_BADDECODINGERROR;
                            } else if(!type->pointerFree) {
                                return UA_STATUSCODE_BADDECODINGERROR;
                            }

                            if(type->typeKind == UA_DATATYPEKIND_STRING ||
                               type->typeKind == UA_DATATYPEKIND_BYTESTRING) {
                                /* Scalar string */
                                UA_ByteString tmp = UA_BYTESTRING_NULL;
                                rv = UA_decodeBinaryInternal(&dst->data.keyFrameData.rawFields,
                                                             offset, &tmp, type, NULL);
                                UA_CHECK_STATUS(rv, return rv); 
                                size_t padding = dsm->fields[i].maxStringLength - tmp.length;
                                *offset += padding;
                                UA_ByteString_clear(&tmp);
                            } else {
                                /* Array of fixed-size members */
                                size_t arraySize = 1;
                                for(size_t j = 0; j < dsm->fields[i].arrayDimensionsSize; j++)
                                    arraySize *= dsm->fields[i].arrayDimensions[j];
                                if(*offset + (arraySize * type->memSize) > src->length)
                                    return UA_STATUSCODE_BADDECODINGERROR;
                                *offset += arraySize * type->memSize;
                            }
                        }
                    } else {
                        //TODO calculate the length of the DSM-Payload for a single DSM
                        //Problem: Size is not set and MetaData information are needed.
                        //Increase offset to avoid endless chunk loop. Needs to be fixed when
                        //pubsub security footer and signatur is enabled.
                        *offset += 1500;
                    }
                    dst->data.keyFrameData.rawFields.length += (*offset - dsmOffset);
                } else {
                    *offset += (dsmSize - (*offset - initialOffset));
                }
                break;
            default:
                return UA_STATUSCODE_BADINTERNALERROR;
        }
    } else if(dst->header.dataSetMessageType == UA_DATASETMESSAGE_DATADELTAFRAME) {
        switch(dst->header.fieldEncoding) {
            case UA_FIELDENCODING_VARIANT: {
                rv = UA_UInt16_decodeBinary(src, offset, &dst->data.deltaFrameData.fieldCount);
                UA_CHECK_STATUS(rv, return rv);
                size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dst->data.deltaFrameData.fieldCount;
                dst->data.deltaFrameData.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);
                for(UA_UInt16 i = 0; i < dst->data.deltaFrameData.fieldCount; i++) {
                    rv = UA_UInt16_decodeBinary(src, offset, &dst->data.deltaFrameData.deltaFrameFields[i].fieldIndex);
                    UA_CHECK_STATUS(rv, return rv);

                    UA_DataValue_init(&dst->data.deltaFrameData.deltaFrameFields[i].fieldValue);
                    rv = UA_decodeBinaryInternal(src, offset, &dst->data.deltaFrameData.deltaFrameFields[i].fieldValue.value, &UA_TYPES[UA_TYPES_VARIANT], customTypes);
                    UA_CHECK_STATUS(rv, return rv);

                    dst->data.deltaFrameData.deltaFrameFields[i].fieldValue.hasValue = true;
                }
                break;
            }
            case UA_FIELDENCODING_DATAVALUE: {
                rv = UA_UInt16_decodeBinary(src, offset, &dst->data.deltaFrameData.fieldCount);
                UA_CHECK_STATUS(rv, return rv);
                size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dst->data.deltaFrameData.fieldCount;
                dst->data.deltaFrameData.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);
                for(UA_UInt16 i = 0; i < dst->data.deltaFrameData.fieldCount; i++) {
                    rv = UA_UInt16_decodeBinary(src, offset, &dst->data.deltaFrameData.deltaFrameFields[i].fieldIndex);
                    UA_CHECK_STATUS(rv, return rv);

                    rv = UA_decodeBinaryInternal(src, offset,
                                                 &dst->data.deltaFrameData.deltaFrameFields[i].fieldValue,
                                                 &UA_TYPES[UA_TYPES_DATAVALUE], customTypes);

                    UA_CHECK_STATUS(rv, return rv);
                }
                break;
            }
            case UA_FIELDENCODING_RAWDATA: {
                return UA_STATUSCODE_BADNOTIMPLEMENTED;
            }
            default:
                return UA_STATUSCODE_BADINTERNALERROR;
        }
    } else if(dst->header.dataSetMessageType != UA_DATASETMESSAGE_KEEPALIVE) {
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    /* Keep-Alive Message contains no Payload Data */
    return UA_STATUSCODE_GOOD;
}

size_t
UA_DataSetMessage_calcSizeBinary(UA_DataSetMessage* p,
                                 UA_NetworkMessageOffsetBuffer *offsetBuffer,
                                 size_t currentOffset) {
    size_t size = currentOffset;

    if(offsetBuffer) {
        size_t pos = offsetBuffer->offsetsSize;
        if(!increaseOffsetArray(offsetBuffer))
            return 0;
        offsetBuffer->offsets[pos].offset = size;
        UA_DataValue_init(&offsetBuffer->offsets[pos].content.value);
        UA_Variant_setScalar(&offsetBuffer->offsets[pos].content.value.value,
                             &p->header.fieldEncoding, &UA_TYPES[UA_TYPES_UINT32]);
        offsetBuffer->offsets[pos].contentType =
            UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_FIELDENCDODING;
    }

    size += 1; /* byte: DataSetMessage Type + Flags */
    if(UA_DataSetMessageHeader_DataSetFlags2Enabled(&p->header))
        size += 1; /* byte */

    if(p->header.dataSetMessageSequenceNrEnabled) {
        if(offsetBuffer) {
            size_t pos = offsetBuffer->offsetsSize;
            if(!increaseOffsetArray(offsetBuffer))
                return 0;
            offsetBuffer->offsets[pos].offset = size;
            offsetBuffer->offsets[pos].content.sequenceNumber =
                p->header.dataSetMessageSequenceNr;
            offsetBuffer->offsets[pos].contentType =
                UA_PUBSUB_OFFSETTYPE_DATASETMESSAGE_SEQUENCENUMBER;
        }
        size += 2; /* UA_UInt16_calcSizeBinary(&p->header.dataSetMessageSequenceNr) */
    }

    if(p->header.timestampEnabled)
        size += 8; /* UA_DateTime_calcSizeBinary(&p->header.timestamp) */

    if(p->header.picoSecondsIncluded)
        size += 2; /* UA_UInt16_calcSizeBinary(&p->header.picoSeconds) */

    if(p->header.statusEnabled)
        size += 2; /* UA_UInt16_calcSizeBinary(&p->header.status) */

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
            UA_NetworkMessageOffset *nmo = NULL;
            const UA_DataValue *v = &p->data.keyFrameData.dataSetFields[i];

            /* Allocate an offset buffer element and store the offset */
            if(offsetBuffer) {
                size_t pos = offsetBuffer->offsetsSize;
                if(!increaseOffsetArray(offsetBuffer))
                    return 0;
                nmo = &offsetBuffer->offsets[pos];
                nmo->offset = size;
            }

            /* Variant encoding */
            if(p->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
                if(offsetBuffer)
                    nmo->contentType = UA_PUBSUB_OFFSETTYPE_PAYLOAD_VARIANT;
                size += UA_calcSizeBinary(&v->value, &UA_TYPES[UA_TYPES_VARIANT]);
                continue;
            }

            /* Datavalue encoding */
            if(p->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
                if(offsetBuffer)
                    nmo->contentType = UA_PUBSUB_OFFSETTYPE_PAYLOAD_DATAVALUE;
                size += UA_calcSizeBinary(v, &UA_TYPES[UA_TYPES_DATAVALUE]);
                continue;
            }

            /* Raw encoding */
            if(p->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
                /* Undecoded raw data */
                if(p->data.keyFrameData.rawFields.length > 0) {
                    if(offsetBuffer) {
                        nmo->contentType = UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW;
                        nmo->content.rawMessageLength = p->data.keyFrameData.rawFields.length;
                    }
                    size += p->data.keyFrameData.rawFields.length;
                    continue;
                }

                /* Decoded data for raw encoding */
                UA_FieldMetaData *fmd =
                    &p->data.keyFrameData.dataSetMetaDataType->fields[i];
                const UA_DataType *type = v->value.type;

                /* String with maxStringLength (padding) */
                if(fmd->maxStringLength > 0 &&
                   (v->value.type->typeKind == UA_DATATYPEKIND_STRING ||
                    v->value.type->typeKind == UA_DATATYPEKIND_BYTESTRING)) {
                    if(offsetBuffer) {
                        nmo->contentType = UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW;
                        nmo->content.rawMessageLength += 4 + fmd->maxStringLength;
                    }
                    size += 4 + fmd->maxStringLength;
                    continue;
                }

                /* Only strings (with a maxStringLength for padding) can have
                 * dynamically allocated members */
                if(!type->pointerFree)
                    return 0;

                /* Handle arrays */
                size_t arraySize = 1;
                for(size_t i = 0; i < fmd->arrayDimensionsSize; i++)
                    arraySize *= fmd->arrayDimensions[i];

                /* Handle types with a different encoding and in-memory size */
                size_t elemSize = type->memSize;
                if(arraySize > 0 && !type->overlayable)
                    elemSize = UA_calcSizeBinary(v->value.data, v->value.type);

                if(offsetBuffer) {
                    nmo->contentType = UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW;
                    nmo->content.rawMessageLength += elemSize * arraySize;
                }
                size += elemSize * arraySize;
            }
        }
    } else if(p->header.dataSetMessageType == UA_DATASETMESSAGE_DATADELTAFRAME) {
        if(offsetBuffer)
            return 0; /* Not supported for RT */

        if(p->header.fieldEncoding == UA_FIELDENCODING_RAWDATA)
            return 0; /* not supported */

        size += 2; /* p->data.deltaFrameData.fieldCount */
        size += (size_t)(2LU * p->data.deltaFrameData.fieldCount); /* fieldIndex per field */

        for(UA_UInt16 i = 0; i < p->data.deltaFrameData.fieldCount; i++) {
            const UA_DataValue *v = &p->data.deltaFrameData.deltaFrameFields[i].fieldValue;
            if(p->header.fieldEncoding == UA_FIELDENCODING_VARIANT)
                size += UA_calcSizeBinary(&v->value, &UA_TYPES[UA_TYPES_VARIANT]);
            else if(p->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE)
                size += UA_calcSizeBinary(v, &UA_TYPES[UA_TYPES_DATAVALUE]);
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

void
UA_NetworkMessageOffsetBuffer_clear(UA_NetworkMessageOffsetBuffer *nmob) {
    UA_ByteString_clear(&nmob->buffer);

    if(nmob->nm) {
        UA_NetworkMessage_clear(nmob->nm);
        UA_free(nmob->nm);
    }

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_ByteString_clear(&nmob->encryptBuffer);
#endif

    if(nmob->offsetsSize == 0)
        return;

    for(size_t i = 0; i < nmob->offsetsSize; i++) {
        UA_NetworkMessageOffset *offset = &nmob->offsets[i];
        if(offset->contentType == UA_PUBSUB_OFFSETTYPE_PAYLOAD_VARIANT ||
           offset->contentType == UA_PUBSUB_OFFSETTYPE_PAYLOAD_DATAVALUE ||
           offset->contentType == UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW) {
            UA_DataValue_clear(&offset->content.value);
            continue;
        }

        if(offset->contentType == UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_FIELDENCDODING) {
            offset->content.value.value.data = NULL;
            UA_DataValue_clear(&offset->content.value);
        }
    }

    UA_free(nmob->offsets);

    memset(nmob, 0, sizeof(UA_NetworkMessageOffsetBuffer));
}

#endif /* UA_ENABLE_PUBSUB */
