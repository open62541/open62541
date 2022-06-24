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
UA_NetworkMessage_updateBufferedMessage(UA_NetworkMessageOffsetBuffer *buffer){
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < buffer->offsetsSize; ++i) {
        UA_NetworkMessageOffset *nmo = &buffer->offsets[i];
        const UA_Byte *bufEnd = &buffer->buffer.data[buffer->buffer.length];
        UA_Byte *bufPos = &buffer->buffer.data[nmo->offset];
        switch(nmo->contentType) {
            case UA_PUBSUB_OFFSETTYPE_DATASETMESSAGE_SEQUENCENUMBER:
            case UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_SEQUENCENUMBER:
                rv = UA_UInt16_encodeBinary((UA_UInt16 *)nmo->offsetData.value.value->value.data, &bufPos, bufEnd);
                if(*((UA_UInt16 *)nmo->offsetData.value.value->value.data) < UA_UINT16_MAX){
                    (*((UA_UInt16 *)nmo->offsetData.value.value->value.data))++;
                } else {
                    (*((UA_UInt16 *)nmo->offsetData.value.value->value.data)) = 0;
                }
                break;
            case UA_PUBSUB_OFFSETTYPE_PAYLOAD_DATAVALUE:
                rv = UA_DataValue_encodeBinary(nmo->offsetData.value.value,
                                               &bufPos, bufEnd);
                break;
            case UA_PUBSUB_OFFSETTYPE_PAYLOAD_VARIANT:
                rv = UA_Variant_encodeBinary(&nmo->offsetData.value.value->value,
                                             &bufPos, bufEnd);
                break;
            case UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW:
                rv = UA_encodeBinaryInternal(nmo->offsetData.value.value->value.data,
                                             nmo->offsetData.value.value->value.type,
                                             &bufPos, &bufEnd, NULL, NULL);
                break;
            case UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_FIELDENCDODING:
                break;
            default:
                return UA_STATUSCODE_BADNOTSUPPORTED;
        }
    }
    return rv;
}

UA_StatusCode
UA_NetworkMessage_updateBufferedNwMessage(UA_NetworkMessageOffsetBuffer *buffer,
                                          const UA_ByteString *src, size_t *bufferPosition){
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    size_t payloadCounter = 0;
    size_t offset = 0;
    UA_DataSetMessage* dsm = buffer->nm->payload.dataSetPayload.dataSetMessages; //Considering one DSM in RT TODO: Clarify multiple DSM
    UA_DataSetMessageHeader header;
    size_t smallestRawOffset = UA_UINT32_MAX;

    for (size_t i = 0; i < buffer->offsetsSize; ++i) {
        offset = buffer->offsets[i].offset + *bufferPosition;
        switch (buffer->offsets[i].contentType) {
        case UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_FIELDENCDODING:
            rv = UA_DataSetMessageHeader_decodeBinary(src, &offset, &header);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
            break;
        case UA_PUBSUB_OFFSETTYPE_PUBLISHERID:
            switch (buffer->nm->publisherIdType) {
            case UA_PUBLISHERDATATYPE_BYTE:
                rv = UA_Byte_decodeBinary(src, &offset, &(buffer->nm->publisherId.publisherIdByte));
                break;
            case UA_PUBLISHERDATATYPE_UINT16:
                rv = UA_UInt16_decodeBinary(src, &offset, &(buffer->nm->publisherId.publisherIdUInt16));
                break;
            case UA_PUBLISHERDATATYPE_UINT32:
                rv = UA_UInt32_decodeBinary(src, &offset, &(buffer->nm->publisherId.publisherIdUInt32));
                break;
            case UA_PUBLISHERDATATYPE_UINT64:
                rv = UA_UInt64_decodeBinary(src, &offset, &(buffer->nm->publisherId.publisherIdUInt64));
                break;
            default:
                return UA_STATUSCODE_BADNOTSUPPORTED;
            }
            break;
        case UA_PUBSUB_OFFSETTYPE_WRITERGROUPID:
            rv = UA_UInt16_decodeBinary(src, &offset, &buffer->nm->groupHeader.writerGroupId);
            UA_CHECK_STATUS(rv, return rv);
            break;
        case UA_PUBSUB_OFFSETTYPE_DATASETWRITERID:
            rv = UA_UInt16_decodeBinary(src, &offset,
                                        &buffer->nm->payloadHeader.dataSetPayloadHeader.dataSetWriterIds[0]); /* TODO */
            UA_CHECK_STATUS(rv, return rv);
            break;
        case UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_SEQUENCENUMBER:
            rv = UA_UInt16_decodeBinary(src, &offset, &buffer->nm->groupHeader.sequenceNumber);
            UA_CHECK_STATUS(rv, return rv);
            break;
        case UA_PUBSUB_OFFSETTYPE_DATASETMESSAGE_SEQUENCENUMBER:
            rv = UA_UInt16_decodeBinary(src, &offset, &(dsm->header.dataSetMessageSequenceNr));
            UA_CHECK_STATUS(rv, return rv);
            break;
        case UA_PUBSUB_OFFSETTYPE_PAYLOAD_DATAVALUE:
            rv = UA_DataValue_decodeBinary(src, &offset,
                                           &(dsm->data.keyFrameData.dataSetFields[payloadCounter]));
            UA_CHECK_STATUS(rv, return rv);
            payloadCounter++;
            break;
        case UA_PUBSUB_OFFSETTYPE_PAYLOAD_VARIANT:
            rv = UA_Variant_decodeBinary(src, &offset,
                                         &dsm->data.keyFrameData.dataSetFields[payloadCounter].value);
            UA_CHECK_STATUS(rv, return rv);
            dsm->data.keyFrameData.dataSetFields[payloadCounter].hasValue = true;
            payloadCounter++;
            break;
        case UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW:
            /* We need only the start address of the raw fields */
            if (smallestRawOffset > offset){
                smallestRawOffset = offset;
                dsm->data.keyFrameData.rawFields.data = &src->data[offset];
                dsm->data.keyFrameData.rawFields.length = buffer->rawMessageLength;
            }
            payloadCounter++;
            break;
        default:
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }
    }
    //check if the frame is of type "raw" payload
    if(smallestRawOffset != UA_UINT32_MAX){
        *bufferPosition = smallestRawOffset + buffer->rawMessageLength;
    } else {
        *bufferPosition = offset;
    }

    return rv;
}

static
UA_StatusCode
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
        case UA_PUBLISHERDATATYPE_BYTE:
            rv = UA_Byte_encodeBinary(&(src->publisherId.publisherIdByte), bufPos, bufEnd);
            break;

        case UA_PUBLISHERDATATYPE_UINT16:
            rv = UA_UInt16_encodeBinary(&(src->publisherId.publisherIdUInt16), bufPos, bufEnd);
            break;

        case UA_PUBLISHERDATATYPE_UINT32:
            rv = UA_UInt32_encodeBinary(&(src->publisherId.publisherIdUInt32), bufPos, bufEnd);
            break;

        case UA_PUBLISHERDATATYPE_UINT64:
            rv = UA_UInt64_encodeBinary(&(src->publisherId.publisherIdUInt64), bufPos, bufEnd);
            break;

        case UA_PUBLISHERDATATYPE_STRING:
            rv = UA_String_encodeBinary(&(src->publisherId.publisherIdString), bufPos, bufEnd);
            break;

        default:
            rv = UA_STATUSCODE_BADINTERNALERROR;
            break;
        }
        UA_CHECK_STATUS(rv, return rv);
    }

    // DataSetClassId
    if(src->dataSetClassIdEnabled) {
        rv = UA_Guid_encodeBinary(&(src->dataSetClassId), bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
}

static
UA_StatusCode
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
    UA_CHECK_STATUS(rv, return rv);

    if(src->groupHeader.writerGroupIdEnabled) {
        rv = UA_UInt16_encodeBinary(&(src->groupHeader.writerGroupId), bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    if(src->groupHeader.groupVersionEnabled) {
        rv = UA_UInt32_encodeBinary(&(src->groupHeader.groupVersion), bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    if(src->groupHeader.networkMessageNumberEnabled) {
        rv = UA_UInt16_encodeBinary(&(src->groupHeader.networkMessageNumber), bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    if(src->groupHeader.sequenceNumberEnabled) {
        rv = UA_UInt16_encodeBinary(&(src->groupHeader.sequenceNumber), bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
}

static
UA_StatusCode
UA_PayloadHeader_encodeBinary(const UA_NetworkMessage* src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd) {

    if(src->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    UA_StatusCode rv = UA_Byte_encodeBinary(&(src->payloadHeader.dataSetPayloadHeader.count), bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

    if(src->payloadHeader.dataSetPayloadHeader.dataSetWriterIds == NULL)
        return UA_STATUSCODE_BADENCODINGERROR;

    for(UA_Byte i = 0; i < src->payloadHeader.dataSetPayloadHeader.count; i++) {
        rv = UA_UInt16_encodeBinary(&(src->payloadHeader.dataSetPayloadHeader.dataSetWriterIds[i]),
                                    bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
}

static
UA_StatusCode
UA_ExtendedNetworkMessageHeader_encodeBinary(const UA_NetworkMessage* src, UA_Byte **bufPos,
                                             const UA_Byte *bufEnd) {

    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    // Timestamp
    if(src->timestampEnabled)
        rv = UA_DateTime_encodeBinary(&(src->timestamp), bufPos, bufEnd);

    UA_CHECK_STATUS(rv, return rv);
    // Picoseconds
    if(src->picosecondsEnabled)
        rv = UA_UInt16_encodeBinary(&(src->picoseconds), bufPos, bufEnd);

    UA_CHECK_STATUS(rv, return rv);
    // PromotedFields
    if(src->promotedFieldsEnabled) {
        /* Size (calculate & encode) */
        UA_UInt16 pfSize = 0;
        for(UA_UInt16 i = 0; i < src->promotedFieldsSize; i++)
            pfSize = (UA_UInt16) (pfSize + UA_Variant_calcSizeBinary(&src->promotedFields[i]));
        rv |= UA_UInt16_encodeBinary(&pfSize, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);

        for (UA_UInt16 i = 0; i < src->promotedFieldsSize; i++)
            rv |= UA_Variant_encodeBinary(&(src->promotedFields[i]), bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
}

static
UA_StatusCode
UA_SecurityHeader_encodeBinary(const UA_NetworkMessage* src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd) {
    // SecurityFlags
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
    UA_CHECK_STATUS(rv, return rv);
    // SecurityTokenId
    rv = UA_UInt32_encodeBinary(&src->securityHeader.securityTokenId, bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

    // NonceLength
    UA_Byte nonceLength = (UA_Byte)src->securityHeader.messageNonceSize;
    rv = UA_Byte_encodeBinary(&nonceLength, bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

    // MessageNonce
    for(size_t i = 0; i < src->securityHeader.messageNonceSize; i++) {
        rv = UA_Byte_encodeBinary(&src->securityHeader.messageNonce[i],
                                  bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    // SecurityFooterSize
    if(src->securityHeader.securityFooterEnabled) {
        rv = UA_UInt16_encodeBinary(&src->securityHeader.securityFooterSize,
                                    bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NetworkMessage_encodeHeaders(const UA_NetworkMessage* src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd) {

    UA_StatusCode rv = UA_NetworkMessageHeader_encodeBinary(src, bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);
    // Group Header
    if(src->groupHeaderEnabled) {
        rv = UA_GroupHeader_encodeBinary(src, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    // Payload Header
    if(src->payloadHeaderEnabled) {
        rv = UA_PayloadHeader_encodeBinary(src, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    // Extended Network Message Header
    rv = UA_ExtendedNetworkMessageHeader_encodeBinary(src, bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);
    // SecurityHeader
    if(src->securityEnabled) {
        rv = UA_SecurityHeader_encodeBinary(src, bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
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
            for (UA_Byte i = 0; i < count; i++) {
                // initially calculate the size, if not specified
                UA_UInt16 sz = 0;
                if((src->payload.dataSetPayload.sizes != NULL) &&
                   (src->payload.dataSetPayload.sizes[i] != 0)) {
                    sz = src->payload.dataSetPayload.sizes[i];
                } else {
                    sz = (UA_UInt16) UA_DataSetMessage_calcSizeBinary(&src->payload.dataSetPayload.dataSetMessages[i],
                                                                      NULL, 0);
                }

                rv = UA_UInt16_encodeBinary(&sz, bufPos, bufEnd);
                UA_CHECK_STATUS(rv, return rv);
            }
        }
    }

    for(UA_Byte i = 0; i < count; i++) {
        rv = UA_DataSetMessage_encodeBinary(&(src->payload.dataSetPayload.dataSetMessages[i]), bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NetworkMessage_encodeFooters(const UA_NetworkMessage* src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd) {

    if(src->securityEnabled) {
        // SecurityFooter
        if(src->securityHeader.securityFooterEnabled) {
            for(size_t i = 0; i < src->securityHeader.securityFooterSize; i++) {
                UA_StatusCode rv = UA_Byte_encodeBinary(&(src->securityFooter.data[i]), bufPos, bufEnd);
                UA_CHECK_STATUS(rv, return rv);
            }
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NetworkMessage_encodeBinary(const UA_NetworkMessage* src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd, UA_Byte **dataToEncryptStart) {

    UA_StatusCode rv = UA_NetworkMessage_encodeHeaders(src, bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

    if(dataToEncryptStart) {
        *dataToEncryptStart = *bufPos;
    }

    rv = UA_NetworkMessage_encodePayload(src, bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

    rv = UA_NetworkMessage_encodeFooters(src, bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

    return UA_STATUSCODE_GOOD;
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

        dst->publisherIdType = (UA_PublisherIdDatatype)(decoded & NM_PUBLISHER_ID_MASK);
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
            case UA_PUBLISHERDATATYPE_BYTE:
                rv = UA_Byte_decodeBinary(src, offset, &(dst->publisherId.publisherIdByte));
                break;

            case UA_PUBLISHERDATATYPE_UINT16:
                rv = UA_UInt16_decodeBinary(src, offset, &(dst->publisherId.publisherIdUInt16));
                break;

            case UA_PUBLISHERDATATYPE_UINT32:
                rv = UA_UInt32_decodeBinary(src, offset, &(dst->publisherId.publisherIdUInt32));
                break;

            case UA_PUBLISHERDATATYPE_UINT64:
                rv = UA_UInt64_decodeBinary(src, offset, &(dst->publisherId.publisherIdUInt64));
                break;

            case UA_PUBLISHERDATATYPE_STRING:
                rv = UA_String_decodeBinary(src, offset, &(dst->publisherId.publisherIdString));
                break;

            default:
                rv = UA_STATUSCODE_BADINTERNALERROR;
                break;
        }
        UA_CHECK_STATUS(rv, return rv);
    }

    if(dst->dataSetClassIdEnabled) {
        rv = UA_Guid_decodeBinary(src, offset, &(dst->dataSetClassId));
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_GroupHeader_decodeBinary(const UA_ByteString *src, size_t *offset,
                         UA_NetworkMessage* dst) {
    UA_Byte decoded = 0;
    UA_StatusCode rv = UA_Byte_decodeBinary(src, offset, &decoded);
    UA_CHECK_STATUS(rv, return rv);

    if((decoded & GROUP_HEADER_WRITER_GROUPID_ENABLED) != 0)
        dst->groupHeader.writerGroupIdEnabled = true;

    if((decoded & GROUP_HEADER_GROUP_VERSION_ENABLED) != 0)
        dst->groupHeader.groupVersionEnabled = true;

    if((decoded & GROUP_HEADER_NM_NUMBER_ENABLED) != 0)
        dst->groupHeader.networkMessageNumberEnabled = true;

    if((decoded & GROUP_HEADER_SEQUENCE_NUMBER_ENABLED) != 0)
        dst->groupHeader.sequenceNumberEnabled = true;

    if(dst->groupHeader.writerGroupIdEnabled) {
        rv = UA_UInt16_decodeBinary(src, offset, &dst->groupHeader.writerGroupId);
        UA_CHECK_STATUS(rv, return rv);
    }
    if(dst->groupHeader.groupVersionEnabled) {
        rv = UA_UInt32_decodeBinary(src, offset, &dst->groupHeader.groupVersion);
        UA_CHECK_STATUS(rv, return rv);
    }
    if(dst->groupHeader.networkMessageNumberEnabled) {
        rv = UA_UInt16_decodeBinary(src, offset, &dst->groupHeader.networkMessageNumber);
        UA_CHECK_STATUS(rv, return rv);
    }
    if(dst->groupHeader.sequenceNumberEnabled) {
        rv = UA_UInt16_decodeBinary(src, offset, &dst->groupHeader.sequenceNumber);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
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
    for (UA_Byte i = 0; i < dst->payloadHeader.dataSetPayloadHeader.count; i++) {
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
        rv = UA_DateTime_decodeBinary(src, offset, &(dst->timestamp));
        UA_CHECK_STATUS(rv, goto error);
    }

    // Picoseconds
    if(dst->picosecondsEnabled) {
        rv = UA_UInt16_decodeBinary(src, offset, &(dst->picoseconds));
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

    if (dst->groupHeaderEnabled) {
        rv = UA_GroupHeader_decodeBinary(src, offset, dst);
        UA_CHECK_STATUS(rv, return rv);
    }

    if (dst->payloadHeaderEnabled) {
        rv = UA_PayloadHeader_decodeBinary(src, offset, dst);
        UA_CHECK_STATUS(rv, return rv);
    }

    if (dst->securityEnabled) {
        rv = UA_SecurityHeader_decodeBinary(src, offset, dst);
        UA_CHECK_STATUS(rv, return rv);
    }

    rv = UA_ExtendedNetworkMessageHeader_decodeBinary(src, offset, dst);
    UA_CHECK_STATUS(rv, return rv);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NetworkMessage_decodePayload(const UA_ByteString *src, size_t *offset, UA_NetworkMessage *dst) {

    // Payload
    if(dst->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    UA_StatusCode rv;

    UA_Byte count = 1;
    if(dst->payloadHeaderEnabled) {
        count = dst->payloadHeader.dataSetPayloadHeader.count;
        if(count > 1) {
            dst->payload.dataSetPayload.sizes = (UA_UInt16 *)UA_Array_new(count, &UA_TYPES[UA_TYPES_UINT16]);
            for (UA_Byte i = 0; i < count; i++) {
                rv = UA_UInt16_decodeBinary(src, offset, &(dst->payload.dataSetPayload.sizes[i]));
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
                                            &(dst->payload.dataSetPayload.dataSetMessages[0]),
                                            0);
    else {
        for(UA_Byte i = 0; i < count; i++) {
            rv = UA_DataSetMessage_decodeBinary(src, offset,
                                                &(dst->payload.dataSetPayload.dataSetMessages[i]),
                                                dst->payload.dataSetPayload.sizes[i]);
        }
    }
    UA_CHECK_STATUS(rv, return rv);

    return UA_STATUSCODE_GOOD;

    /**
     * TODO: check if making the cleanup to free its own allocated memory is better,
     *       currently the free happens in a parent context
     */
    // error:
    // if (dst->payload.dataSetPayload.dataSetMessages) {
    //     UA_free(dst->payload.dataSetPayload.dataSetMessages);
    // }
    // return rv;
}

UA_StatusCode
UA_NetworkMessage_decodeFooters(const UA_ByteString *src, size_t *offset, UA_NetworkMessage *dst) {

    if (dst->securityEnabled) {
        // SecurityFooter
        if(dst->securityHeader.securityFooterEnabled &&
           (dst->securityHeader.securityFooterSize > 0)) {
            UA_StatusCode rv = UA_ByteString_allocBuffer(&dst->securityFooter,
                                           dst->securityHeader.securityFooterSize);
            UA_CHECK_STATUS(rv, return rv);

            for(UA_UInt16 i = 0; i < dst->securityHeader.securityFooterSize; i++) {
                rv = UA_Byte_decodeBinary(src, offset, &(dst->securityFooter.data[i]));
                UA_CHECK_STATUS(rv, return rv);
            }
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NetworkMessage_decodeBinary(const UA_ByteString *src, size_t *offset,
                               UA_NetworkMessage* dst) {

    UA_StatusCode rv = UA_STATUSCODE_GOOD;

    /* headers only need to be decoded when not in encryption mode
     * because headers are already decoded when encryption mode is enabled
     * to check for security parameters and decrypt/verify
     *
     * TODO: check if there is a workaround to use this function
     *       also when encryption is enabled
     */
    // #ifndef UA_ENABLE_PUBSUB_ENCRYPTION
    // if (*offset == 0) {
    //    rv = UA_NetworkMessage_decodeHeaders(src, offset, dst);
    //    UA_CHECK_STATUS(rv, return rv);
    // }
    // #endif

    rv = UA_NetworkMessage_decodeHeaders(src, offset, dst);
    UA_CHECK_STATUS(rv, return rv);

    rv = UA_NetworkMessage_decodePayload(src, offset, dst);
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

    offsetBuffer->offsets = tmpOffsets;
    offsetBuffer->offsetsSize++;
    return true;
}

size_t
UA_NetworkMessage_calcSizeBinary(UA_NetworkMessage *p,
                                 UA_NetworkMessageOffsetBuffer *offsetBuffer) {
    size_t retval = 0;
    UA_Byte byte = 0;
    size_t size = UA_Byte_calcSizeBinary(&byte); // UADPVersion + UADPFlags
    if(UA_NetworkMessage_ExtendedFlags1Enabled(p)) {
        size += UA_Byte_calcSizeBinary(&byte);
        if(UA_NetworkMessage_ExtendedFlags2Enabled(p))
            size += UA_Byte_calcSizeBinary(&byte);
    }

    if(p->publisherIdEnabled) {
        if(offsetBuffer && offsetBuffer->RTsubscriberEnabled){
            size_t pos = offsetBuffer->offsetsSize;
            if(!increaseOffsetArray(offsetBuffer))
                return 0;

            offsetBuffer->offsets[pos].offset = size;
            offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_PUBLISHERID;
        }
        switch (p->publisherIdType) {
            case UA_PUBLISHERDATATYPE_BYTE:
                size += UA_Byte_calcSizeBinary(&p->publisherId.publisherIdByte);
                break;

            case UA_PUBLISHERDATATYPE_UINT16:
                size += UA_UInt16_calcSizeBinary(&p->publisherId.publisherIdUInt16);
                break;

            case UA_PUBLISHERDATATYPE_UINT32:
                size += UA_UInt32_calcSizeBinary(&p->publisherId.publisherIdUInt32);
                break;

            case UA_PUBLISHERDATATYPE_UINT64:
                size += UA_UInt64_calcSizeBinary(&p->publisherId.publisherIdUInt64);
                break;

            case UA_PUBLISHERDATATYPE_STRING:
                size += UA_String_calcSizeBinary(&p->publisherId.publisherIdString);
                break;
        }
    }

    if(p->dataSetClassIdEnabled)
        size += UA_Guid_calcSizeBinary(&p->dataSetClassId);

    // Group Header
    if(p->groupHeaderEnabled) {
        size += UA_Byte_calcSizeBinary(&byte);

        if(p->groupHeader.writerGroupIdEnabled) {
            if(offsetBuffer && offsetBuffer->RTsubscriberEnabled){
                size_t pos = offsetBuffer->offsetsSize;
                if(!increaseOffsetArray(offsetBuffer))
                    return 0;

                offsetBuffer->offsets[pos].offset = size;
                offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_WRITERGROUPID;
            }
            size += UA_UInt16_calcSizeBinary(&p->groupHeader.writerGroupId);
        }

        if(p->groupHeader.groupVersionEnabled)
            size += UA_UInt32_calcSizeBinary(&p->groupHeader.groupVersion);

        if(p->groupHeader.networkMessageNumberEnabled) {
            size += UA_UInt16_calcSizeBinary(&p->groupHeader.networkMessageNumber);
        }

        if(p->groupHeader.sequenceNumberEnabled){
            if(offsetBuffer){
                size_t pos = offsetBuffer->offsetsSize;
                if(!increaseOffsetArray(offsetBuffer))
                    return 0;
                offsetBuffer->offsets[pos].offset = size;
                offsetBuffer->offsets[pos].offsetData.value.value = UA_DataValue_new();
                UA_DataValue_init(offsetBuffer->offsets[pos].offsetData.value.value);
                UA_Variant_setScalarCopy(&offsetBuffer->offsets[pos].offsetData.value.value->value,
                                     &p->groupHeader.sequenceNumber, &UA_TYPES[UA_TYPES_UINT16]);
                offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_SEQUENCENUMBER;
            }
            size += UA_UInt16_calcSizeBinary(&p->groupHeader.sequenceNumber);
        }
    }

    // Payload Header
    if(p->payloadHeaderEnabled) {
        if(p->networkMessageType == UA_NETWORKMESSAGE_DATASET) {
            size += UA_Byte_calcSizeBinary(&p->payloadHeader.dataSetPayloadHeader.count);
            if(p->payloadHeader.dataSetPayloadHeader.dataSetWriterIds != NULL) {
                if(offsetBuffer && offsetBuffer->RTsubscriberEnabled){
                    size_t pos = offsetBuffer->offsetsSize;
                    if(!increaseOffsetArray(offsetBuffer))
                        return 0;
                    offsetBuffer->offsets[pos].offset = size;
                    offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_DATASETWRITERID;
                }
                size += UA_UInt16_calcSizeBinary(&p->payloadHeader.dataSetPayloadHeader.dataSetWriterIds[0]) *
                        p->payloadHeader.dataSetPayloadHeader.count;
            } else {
                return 0; /* no dataSetWriterIds given! */
            }
        } else {
            // not implemented
        }
    }

    if(p->timestampEnabled) {
        if(offsetBuffer){
            size_t pos = offsetBuffer->offsetsSize;
            if(!increaseOffsetArray(offsetBuffer))
                return 0;
            offsetBuffer->offsets[pos].offset = size;
            offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_TIMESTAMP;
        }
        size += UA_DateTime_calcSizeBinary(&p->timestamp);
    }

    if(p->picosecondsEnabled){
        if (offsetBuffer) {
            size_t pos = offsetBuffer->offsetsSize;
            if(!increaseOffsetArray(offsetBuffer))
                return 0;
            offsetBuffer->offsets[pos].offset = size;
            offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_TIMESTAMP_PICOSECONDS;
        }
        size += UA_UInt16_calcSizeBinary(&p->picoseconds);
    }

    if(p->promotedFieldsEnabled) {
        size += UA_UInt16_calcSizeBinary(&p->promotedFieldsSize);
        for (UA_UInt16 i = 0; i < p->promotedFieldsSize; i++)
            size += UA_Variant_calcSizeBinary(&p->promotedFields[i]);
    }

    if(p->securityEnabled) {
        size += UA_Byte_calcSizeBinary(&byte);
        size += UA_UInt32_calcSizeBinary(&p->securityHeader.securityTokenId);
        size += 1; /* UA_Byte_calcSizeBinary(&p->securityHeader.nonceLength); */
        size += p->securityHeader.messageNonceSize;
        if(p->securityHeader.securityFooterEnabled)
            size += UA_UInt16_calcSizeBinary(&p->securityHeader.securityFooterSize);
    }

    if(p->networkMessageType == UA_NETWORKMESSAGE_DATASET) {
        UA_Byte count = 1;
        if(p->payloadHeaderEnabled) {
            count = p->payloadHeader.dataSetPayloadHeader.count;
            if(count > 1)
                size += UA_UInt16_calcSizeBinary(&(p->payload.dataSetPayload.sizes[0])) * count;
        }

        for (size_t i = 0; i < count; i++) {
            if (offsetBuffer)
                UA_DataSetMessage_calcSizeBinary(&(p->payload.dataSetPayload.dataSetMessages[i]), offsetBuffer,
                                                 size);
            size += UA_DataSetMessage_calcSizeBinary(&(p->payload.dataSetPayload.dataSetMessages[i]), NULL, 0);
        }
    }

    if(p->securityEnabled) {
        if(p->securityHeader.securityFooterEnabled)
            size += p->securityHeader.securityFooterSize;
    }

    retval = size;
    return retval;
}

void
UA_NetworkMessage_clear(UA_NetworkMessage* p) {
    if(p->promotedFieldsEnabled) {
        UA_Array_delete(p->promotedFields, p->promotedFieldsSize,
                        &UA_TYPES[UA_TYPES_VARIANT]);
    }

    if(p->networkMessageType == UA_NETWORKMESSAGE_DATASET) {
        if(p->payloadHeader.dataSetPayloadHeader.dataSetWriterIds)
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
       p->publisherIdType == UA_PUBLISHERDATATYPE_STRING)
       UA_String_clear(&p->publisherId.publisherIdString);

    memset(p, 0, sizeof(UA_NetworkMessage));
}

UA_Boolean
UA_NetworkMessage_ExtendedFlags1Enabled(const UA_NetworkMessage* src) {
    UA_Boolean retval = false;

    if((src->publisherIdType != UA_PUBLISHERDATATYPE_BYTE)
        || src->dataSetClassIdEnabled
        || src->securityEnabled
        || src->timestampEnabled
        || src->picosecondsEnabled
        || UA_NetworkMessage_ExtendedFlags2Enabled(src))
    {
        retval = true;
    }

    return retval;
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
        rv = UA_DateTime_encodeBinary(&(src->timestamp), bufPos, bufEnd); /* UtcTime */
        UA_CHECK_STATUS(rv, return rv);
    }

    // PicoSeconds
    if(src->picoSecondsIncluded) {
        rv = UA_UInt16_encodeBinary(&(src->picoSeconds), bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    // Status
    if(src->statusEnabled) {
        rv = UA_UInt16_encodeBinary(&(src->status), bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    // ConfigVersionMajorVersion
    if(src->configVersionMajorVersionEnabled) {
        rv = UA_UInt32_encodeBinary(&(src->configVersionMajorVersion), bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);
    }

    // ConfigVersionMinorVersion
    if(src->configVersionMinorVersionEnabled) {
        rv = UA_UInt32_encodeBinary(&(src->configVersionMinorVersion), bufPos, bufEnd);
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

size_t
UA_DataSetMessageHeader_calcSizeBinary(const UA_DataSetMessageHeader* p) {
    UA_Byte byte = 0;
    size_t size = UA_Byte_calcSizeBinary(&byte); // DataSetMessage Type + Flags
    if(UA_DataSetMessageHeader_DataSetFlags2Enabled(p))
        size += UA_Byte_calcSizeBinary(&byte);

    if(p->dataSetMessageSequenceNrEnabled)
        size += UA_UInt16_calcSizeBinary(&p->dataSetMessageSequenceNr);

    if(p->timestampEnabled)
        size += UA_DateTime_calcSizeBinary(&p->timestamp); /* UtcTime */

    if(p->picoSecondsIncluded)
        size += UA_UInt16_calcSizeBinary(&p->picoSeconds);

    if(p->statusEnabled)
        size += UA_UInt16_calcSizeBinary(&p->status);

    if(p->configVersionMajorVersionEnabled)
        size += UA_UInt32_calcSizeBinary(&p->configVersionMajorVersion);

    if(p->configVersionMinorVersionEnabled)
        size += UA_UInt32_calcSizeBinary(&p->configVersionMinorVersion);

    return size;
}

UA_StatusCode
UA_DataSetMessage_encodeBinary(const UA_DataSetMessage* src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd) {
    UA_StatusCode rv = UA_DataSetMessageHeader_encodeBinary(&src->header, bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

    if(src->data.keyFrameData.fieldCount == 0) {
        /* Heartbeat: "DataSetMessage is a key frame that only contains header information" */
        return rv;
    }

    if(src->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
        if(src->header.fieldEncoding != UA_FIELDENCODING_RAWDATA) {
            rv = UA_UInt16_encodeBinary(&(src->data.keyFrameData.fieldCount), bufPos, bufEnd);
            UA_CHECK_STATUS(rv, return rv);
        }
        if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
            for (UA_UInt16 i = 0; i < src->data.keyFrameData.fieldCount; i++) {
                rv = UA_Variant_encodeBinary(&(src->data.keyFrameData.dataSetFields[i].value), bufPos, bufEnd);
                UA_CHECK_STATUS(rv, return rv);
            }
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
            for (UA_UInt16 i = 0; i < src->data.keyFrameData.fieldCount; i++) {
                rv = UA_encodeBinaryInternal(src->data.keyFrameData.dataSetFields[i].value.data,
                                             src->data.keyFrameData.dataSetFields[i].value.type,
                                             bufPos, &bufEnd, NULL, NULL);
                UA_CHECK_STATUS(rv, return rv);
            }
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
            for (UA_UInt16 i = 0; i < src->data.keyFrameData.fieldCount; i++) {
                rv = UA_DataValue_encodeBinary(&(src->data.keyFrameData.dataSetFields[i]), bufPos, bufEnd);
                UA_CHECK_STATUS(rv, return rv);
            }
        }
    } else if(src->header.dataSetMessageType == UA_DATASETMESSAGE_DATADELTAFRAME) {
        // Encode Delta Frame
        // Here the FieldCount is always present
        rv = UA_UInt16_encodeBinary(&(src->data.keyFrameData.fieldCount), bufPos, bufEnd);
        UA_CHECK_STATUS(rv, return rv);

        if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
            for (UA_UInt16 i = 0; i < src->data.deltaFrameData.fieldCount; i++) {
                rv = UA_UInt16_encodeBinary(&(src->data.deltaFrameData.deltaFrameFields[i].fieldIndex), bufPos, bufEnd);
                UA_CHECK_STATUS(rv, return rv);

                rv = UA_Variant_encodeBinary(&(src->data.deltaFrameData.deltaFrameFields[i].fieldValue.value), bufPos, bufEnd);
                UA_CHECK_STATUS(rv, return rv);
            }
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
            return UA_STATUSCODE_BADNOTIMPLEMENTED;
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
            for (UA_UInt16 i = 0; i < src->data.deltaFrameData.fieldCount; i++) {
                rv = UA_UInt16_encodeBinary(&(src->data.deltaFrameData.deltaFrameFields[i].fieldIndex), bufPos, bufEnd);
                UA_CHECK_STATUS(rv, return rv);

                rv = UA_DataValue_encodeBinary(&(src->data.deltaFrameData.deltaFrameFields[i].fieldValue), bufPos, bufEnd);
                UA_CHECK_STATUS(rv, return rv);
            }
        }
    } else if(src->header.dataSetMessageType != UA_DATASETMESSAGE_KEEPALIVE) {
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    /* Keep-Alive Message contains no Payload Data */
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_DataSetMessage_decodeBinary(const UA_ByteString *src, size_t *offset, UA_DataSetMessage* dst, UA_UInt16 dsmSize) {
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
            {
                rv = UA_UInt16_decodeBinary(src, offset, &dst->data.keyFrameData.fieldCount);
                UA_CHECK_STATUS(rv, return rv);
                dst->data.keyFrameData.dataSetFields =
                    (UA_DataValue *)UA_Array_new(dst->data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
                for (UA_UInt16 i = 0; i < dst->data.keyFrameData.fieldCount; i++) {
                    UA_DataValue_init(&dst->data.keyFrameData.dataSetFields[i]);
                    rv = UA_Variant_decodeBinary(src, offset, &dst->data.keyFrameData.dataSetFields[i].value);
                    UA_CHECK_STATUS(rv, return rv);

                    dst->data.keyFrameData.dataSetFields[i].hasValue = true;
                }
                break;
            }
            case UA_FIELDENCODING_DATAVALUE:
            {
                rv = UA_UInt16_decodeBinary(src, offset, &dst->data.keyFrameData.fieldCount);
                UA_CHECK_STATUS(rv, return rv);
                dst->data.keyFrameData.dataSetFields =
                    (UA_DataValue *)UA_Array_new(dst->data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
                for (UA_UInt16 i = 0; i < dst->data.keyFrameData.fieldCount; i++) {
                    rv = UA_DataValue_decodeBinary(src, offset, &(dst->data.keyFrameData.dataSetFields[i]));
                    UA_CHECK_STATUS(rv, return rv);
                }
                break;
            }
            case UA_FIELDENCODING_RAWDATA:
            {
                dst->data.keyFrameData.rawFields.data = &src->data[*offset];
                dst->data.keyFrameData.rawFields.length = dsmSize;
                if(dsmSize == 0){
                    //TODO calculate the length of the DSM-Payload for a single DSM
                    //Problem: Size is not set and MetaData information are needed.
                    //Increase offset to avoid endless chunk loop. Needs to be fixed when
                    //pubsub security footer and signatur is enabled.
                    *offset += 1500;
                } else {
                    *offset += (dsmSize - (*offset - initialOffset));
                }
                break;
            }
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
                for (UA_UInt16 i = 0; i < dst->data.deltaFrameData.fieldCount; i++) {
                    rv = UA_UInt16_decodeBinary(src, offset, &dst->data.deltaFrameData.deltaFrameFields[i].fieldIndex);
                    UA_CHECK_STATUS(rv, return rv);

                    UA_DataValue_init(&dst->data.deltaFrameData.deltaFrameFields[i].fieldValue);
                    rv = UA_Variant_decodeBinary(src, offset, &dst->data.deltaFrameData.deltaFrameFields[i].fieldValue.value);
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
                for (UA_UInt16 i = 0; i < dst->data.deltaFrameData.fieldCount; i++) {
                    rv = UA_UInt16_decodeBinary(src, offset, &dst->data.deltaFrameData.deltaFrameFields[i].fieldIndex);
                    UA_CHECK_STATUS(rv, return rv);

                    rv = UA_DataValue_decodeBinary(src, offset, &(dst->data.deltaFrameData.deltaFrameFields[i].fieldValue));
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
UA_DataSetMessage_calcSizeBinary(UA_DataSetMessage* p, UA_NetworkMessageOffsetBuffer *offsetBuffer, size_t currentOffset) {
    size_t size = currentOffset;

    if (offsetBuffer) {
        size_t pos = offsetBuffer->offsetsSize;
        if(!increaseOffsetArray(offsetBuffer))
            return 0;
        offsetBuffer->offsets[pos].offset = size;
        offsetBuffer->offsets[pos].offsetData.value.value = UA_DataValue_new();
        UA_DataValue_init(offsetBuffer->offsets[pos].offsetData.value.value);
        UA_Variant_setScalar(&offsetBuffer->offsets[pos].offsetData.value.value->value,
                             &p->header.fieldEncoding, &UA_TYPES[UA_TYPES_UINT32]);
        offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_FIELDENCDODING;
    }

    UA_Byte byte = 0;
    size += UA_Byte_calcSizeBinary(&byte); // DataSetMessage Type + Flags
    if(UA_DataSetMessageHeader_DataSetFlags2Enabled(&p->header))
        size += UA_Byte_calcSizeBinary(&byte);

    if(p->header.dataSetMessageSequenceNrEnabled) {
        if (offsetBuffer) {
            size_t pos = offsetBuffer->offsetsSize;
            if(!increaseOffsetArray(offsetBuffer))
                return 0;
            offsetBuffer->offsets[pos].offset = size;
            offsetBuffer->offsets[pos].offsetData.value.value = UA_DataValue_new();
            UA_DataValue_init(offsetBuffer->offsets[pos].offsetData.value.value);
            UA_Variant_setScalarCopy(&offsetBuffer->offsets[pos].offsetData.value.value->value,
                                 &p->header.dataSetMessageSequenceNr, &UA_TYPES[UA_TYPES_UINT16]);
            offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_DATASETMESSAGE_SEQUENCENUMBER;
        }
        size += UA_UInt16_calcSizeBinary(&p->header.dataSetMessageSequenceNr);
    }

    if(p->header.timestampEnabled)
        size += UA_DateTime_calcSizeBinary(&p->header.timestamp); /* UtcTime */

    if(p->header.picoSecondsIncluded)
        size += UA_UInt16_calcSizeBinary(&p->header.picoSeconds);

    if(p->header.statusEnabled)
        size += UA_UInt16_calcSizeBinary(&p->header.status);

    if(p->header.configVersionMajorVersionEnabled)
        size += UA_UInt32_calcSizeBinary(&p->header.configVersionMajorVersion);

    if(p->header.configVersionMinorVersionEnabled)
        size += UA_UInt32_calcSizeBinary(&p->header.configVersionMinorVersion);

    /* Keyframe with no fields is a heartbeat, stop counting then */
    if(p->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME && p->data.keyFrameData.fieldCount != 0) {
        if(p->header.fieldEncoding != UA_FIELDENCODING_RAWDATA){
            size += UA_calcSizeBinary(&p->data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_UINT16]);
        }
        if(p->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
            for (UA_UInt16 i = 0; i < p->data.keyFrameData.fieldCount; i++){
                if (offsetBuffer) {
                    size_t pos = offsetBuffer->offsetsSize;
                    if(!increaseOffsetArray(offsetBuffer))
                        return 0;
                    offsetBuffer->offsets[pos].offset = size;
                    offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_PAYLOAD_VARIANT;
                    //TODO check value source and alloc!
                    //offsetBuffer->offsets[pos].offsetData.value.value = p->data.keyFrameData.dataSetFields;
                    offsetBuffer->offsets[pos].offsetData.value.value = UA_DataValue_new();
                    UA_Variant_setScalar(&offsetBuffer->offsets[pos].offsetData.value.value->value,
                                         p->data.keyFrameData.dataSetFields[i].value.data,
                                         p->data.keyFrameData.dataSetFields[i].value.type);
                    offsetBuffer->offsets[pos].offsetData.value.value->value.storageType = UA_VARIANT_DATA_NODELETE;
                }
                size += UA_calcSizeBinary(&p->data.keyFrameData.dataSetFields[i].value, &UA_TYPES[UA_TYPES_VARIANT]);
            }
        } else if(p->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
            for (UA_UInt16 i = 0; i < p->data.keyFrameData.fieldCount; i++){
                if (offsetBuffer) {
                    size_t pos = offsetBuffer->offsetsSize;
                    if(!increaseOffsetArray(offsetBuffer))
                        return 0;
                    offsetBuffer->offsets[pos].offset = size;
                    offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW;
                    offsetBuffer->offsets[pos].offsetData.value.value = UA_DataValue_new();
                    //init offset buffer with the latest value
                    UA_Variant_setScalar(&offsetBuffer->offsets[pos].offsetData.value.value->value,
                                         p->data.keyFrameData.dataSetFields[i].value.data,
                                         p->data.keyFrameData.dataSetFields[i].value.type);
                    offsetBuffer->offsets[pos].offsetData.value.value->value.storageType = UA_VARIANT_DATA_NODELETE;
                    //count the memory size of the specific field
                    offsetBuffer->rawMessageLength += p->data.keyFrameData.dataSetFields[i].value.type->memSize;
                }
                size += UA_calcSizeBinary(p->data.keyFrameData.dataSetFields[i].value.data,
                                          p->data.keyFrameData.dataSetFields[i].value.type);
            }
        } else if(p->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
            for (UA_UInt16 i = 0; i < p->data.keyFrameData.fieldCount; i++) {
                if (offsetBuffer) {
                    size_t pos = offsetBuffer->offsetsSize;
                    if(!increaseOffsetArray(offsetBuffer))
                        return 0;
                    offsetBuffer->offsets[pos].offset = size;
                    offsetBuffer->offsets[pos].contentType = UA_PUBSUB_OFFSETTYPE_PAYLOAD_DATAVALUE;
                    //TODO check value source, change implementation to 'variant'
                    offsetBuffer->offsets[pos].offsetData.value.value = p->data.keyFrameData.dataSetFields;
                }
                size += UA_calcSizeBinary(&p->data.keyFrameData.dataSetFields[i], &UA_TYPES[UA_TYPES_DATAVALUE]);
            }
        }
    } else if(p->header.dataSetMessageType == UA_DATASETMESSAGE_DATADELTAFRAME) {
        //TODO clarify how to handle DATADELTAFRAME messages with RT
        if(p->header.fieldEncoding != UA_FIELDENCODING_RAWDATA)
            size += UA_calcSizeBinary(&p->data.deltaFrameData.fieldCount, &UA_TYPES[UA_TYPES_UINT16]);

        if(p->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
            for (UA_UInt16 i = 0; i < p->data.deltaFrameData.fieldCount; i++) {
                size += UA_calcSizeBinary(&p->data.deltaFrameData.deltaFrameFields[i].fieldIndex, &UA_TYPES[UA_TYPES_UINT16]);
                size += UA_calcSizeBinary(&p->data.deltaFrameData.deltaFrameFields[i].fieldValue.value, &UA_TYPES[UA_TYPES_VARIANT]);
            }
        } else if(p->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
            // not implemented
        } else if(p->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
            for (UA_UInt16 i = 0; i < p->data.deltaFrameData.fieldCount; i++) {
                size += UA_calcSizeBinary(&p->data.deltaFrameData.deltaFrameFields[i].fieldIndex, &UA_TYPES[UA_TYPES_UINT16]);
                size += UA_calcSizeBinary(&p->data.deltaFrameData.deltaFrameFields[i].fieldValue, &UA_TYPES[UA_TYPES_DATAVALUE]);
            }
        }
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
                if(p->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
                    UA_DataValue_clear(&f->fieldValue);
                } else if(p->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
                    UA_Variant_clear(&f->fieldValue.value);
                }
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
           offset->contentType == UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW ||
           offset->contentType == UA_PUBSUB_OFFSETTYPE_DATASETMESSAGE_SEQUENCENUMBER ||
           offset->contentType == UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_SEQUENCENUMBER) {
            UA_DataValue_delete(offset->offsetData.value.value);
            continue;
        }

        if(offset->contentType == UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_FIELDENCDODING) {
            offset->offsetData.value.value->value.data = NULL;
            UA_DataValue_delete(offset->offsetData.value.value);
        }
    }

    UA_free(nmob->offsets);

    memset(nmob, 0, sizeof(UA_NetworkMessageOffsetBuffer));
}


#endif /* UA_ENABLE_PUBSUB */

