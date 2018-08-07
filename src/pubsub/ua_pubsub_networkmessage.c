/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Tino Bischoff)
 */

#include "ua_types.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_types_generated_handling.h"
#include "ua_log_stdout.h"
#include "ua_types_encoding_json.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include "ua_pubsub_networkmessage.h"

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
UA_NetworkMessage_encodeBinary(const UA_NetworkMessage* src, UA_Byte **bufPos,
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
    if(rv != UA_STATUSCODE_GOOD)
        return rv;

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
        if(rv != UA_STATUSCODE_GOOD)
            return rv;

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
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
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
    
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    // DataSetClassId
    if(src->dataSetClassIdEnabled) {
        rv = UA_Guid_encodeBinary(&(src->dataSetClassId), bufPos, bufEnd);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    // Group Header
    if(src->groupHeaderEnabled) {
        v = 0;

        if(src->groupHeader.writerGroupIdEnabled)
            v |= GROUP_HEADER_WRITER_GROUPID_ENABLED;

        if(src->groupHeader.groupVersionEnabled)
            v |= GROUP_HEADER_GROUP_VERSION_ENABLED;

        if(src->groupHeader.networkMessageNumberEnabled)
            v |= GROUP_HEADER_NM_NUMBER_ENABLED;

        if(src->groupHeader.sequenceNumberEnabled)
            v |= GROUP_HEADER_SEQUENCE_NUMBER_ENABLED;

        rv = UA_Byte_encodeBinary(&v, bufPos, bufEnd);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;

        if(src->groupHeader.writerGroupIdEnabled) {
            rv = UA_UInt16_encodeBinary(&(src->groupHeader.writerGroupId), bufPos, bufEnd);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }

        if(src->groupHeader.groupVersionEnabled) { 
            rv = UA_UInt32_encodeBinary(&(src->groupHeader.groupVersion), bufPos, bufEnd);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }

        if(src->groupHeader.networkMessageNumberEnabled) {
            rv = UA_UInt16_encodeBinary(&(src->groupHeader.networkMessageNumber), bufPos, bufEnd);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }

        if(src->groupHeader.sequenceNumberEnabled) {
            rv = UA_UInt16_encodeBinary(&(src->groupHeader.sequenceNumber), bufPos, bufEnd);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }
    }

    // Payload-Header
    if(src->payloadHeaderEnabled) {
        if(src->networkMessageType != UA_NETWORKMESSAGE_DATASET)
            return UA_STATUSCODE_BADNOTIMPLEMENTED;
            
        rv = UA_Byte_encodeBinary(&(src->payloadHeader.dataSetPayloadHeader.count), bufPos, bufEnd);

        if(src->payloadHeader.dataSetPayloadHeader.dataSetWriterIds == NULL)
            return UA_STATUSCODE_BADENCODINGERROR;
            
        for(UA_Byte i = 0; i < src->payloadHeader.dataSetPayloadHeader.count; i++) {
            rv = UA_UInt16_encodeBinary(&(src->payloadHeader.dataSetPayloadHeader.dataSetWriterIds[i]),
                                        bufPos, bufEnd);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }
    }

    // Timestamp
    if(src->timestampEnabled)
        rv = UA_DateTime_encodeBinary(&(src->timestamp), bufPos, bufEnd);

    // Picoseconds
    if(src->picosecondsEnabled)
        rv = UA_UInt16_encodeBinary(&(src->picoseconds), bufPos, bufEnd);

    // PromotedFields
    if(src->promotedFieldsEnabled) {
        /* Size (calculate & encode) */
        UA_UInt16 pfSize = 0;
        for(UA_UInt16 i = 0; i < src->promotedFieldsSize; i++)
            pfSize = (UA_UInt16) (pfSize + UA_Variant_calcSizeBinary(&src->promotedFields[i]));
        rv |= UA_UInt16_encodeBinary(&pfSize, bufPos, bufEnd);

        for (UA_UInt16 i = 0; i < src->promotedFieldsSize; i++)
            rv |= UA_Variant_encodeBinary(&(src->promotedFields[i]), bufPos, bufEnd);
    }

    // SecurityHeader
    if(src->securityEnabled) {
        // SecurityFlags
        v = 0;
        if(src->securityHeader.networkMessageSigned)
            v |= SECURITY_HEADER_NM_SIGNED;

        if(src->securityHeader.networkMessageEncrypted)
            v |= SECURITY_HEADER_NM_ENCRYPTED;

        if(src->securityHeader.securityFooterEnabled)
            v |= SECURITY_HEADER_SEC_FOOTER_ENABLED;

        if(src->securityHeader.forceKeyReset)
            v |= SECURITY_HEADER_FORCE_KEY_RESET;

        rv = UA_Byte_encodeBinary(&v, bufPos, bufEnd);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;

        // SecurityTokenId
        rv = UA_UInt32_encodeBinary(&src->securityHeader.securityTokenId, bufPos, bufEnd);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;

        // NonceLength
        rv = UA_Byte_encodeBinary(&src->securityHeader.nonceLength, bufPos, bufEnd);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;

        // MessageNonce
        for (UA_Byte i = 0; i < src->securityHeader.nonceLength; i++) {
            rv = UA_Byte_encodeBinary(&(src->securityHeader.messageNonce.data[i]), bufPos, bufEnd);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }

        // SecurityFooterSize
        if(src->securityHeader.securityFooterEnabled) {
            rv = UA_UInt16_encodeBinary(&src->securityHeader.securityFooterSize, bufPos, bufEnd);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }
    }

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
                    sz = (UA_UInt16)UA_DataSetMessage_calcSizeBinary(&src->payload.dataSetPayload.dataSetMessages[i]);
                }

                rv = UA_UInt16_encodeBinary(&sz, bufPos, bufEnd);
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;
            }
        }
    }

    for(UA_Byte i = 0; i < count; i++) {
        rv = UA_DataSetMessage_encodeBinary(&(src->payload.dataSetPayload.dataSetMessages[i]), bufPos, bufEnd);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    if(src->securityEnabled) {
        // SecurityFooter
        if(src->securityHeader.securityFooterEnabled) {
            for(UA_Byte i = 0; i < src->securityHeader.securityFooterSize; i++) {
                rv = UA_Byte_encodeBinary(&(src->securityFooter.data[i]), bufPos, bufEnd);
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;
            }
        }

        // Signature
        if(src->securityHeader.networkMessageSigned) {
            rv = UA_ByteString_encodeBinary(&(src->signature), bufPos, bufEnd);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_NetworkMessage_decodeBinaryInternal(const UA_ByteString *src, size_t *offset,
                                       UA_NetworkMessage* dst) {
    memset(dst, 0, sizeof(UA_NetworkMessage));
    UA_Byte v = 0;
    UA_StatusCode rv = UA_Byte_decodeBinary(src, offset, &v);
    if(rv != UA_STATUSCODE_GOOD)
        return rv;

    dst->version = v & NM_VERSION_MASK;
    
    if((v & NM_PUBLISHER_ID_ENABLED_MASK) != 0)
        dst->publisherIdEnabled = true;

    if((v & NM_GROUP_HEADER_ENABLED_MASK) != 0)
        dst->groupHeaderEnabled = true;

    if((v & NM_PAYLOAD_HEADER_ENABLED_MASK) != 0)
        dst->payloadHeaderEnabled = true;
    
    if((v & NM_EXTENDEDFLAGS1_ENABLED_MASK) != 0) {
        v = 0;
        rv = UA_Byte_decodeBinary(src, offset, &v);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;

        dst->publisherIdType = (UA_PublisherIdDatatype)(v & NM_PUBLISHER_ID_MASK);
        if((v & NM_DATASET_CLASSID_ENABLED_MASK) != 0)
            dst->dataSetClassIdEnabled = true;

        if((v & NM_SECURITY_ENABLED_MASK) != 0)
            dst->securityEnabled = true;

        if((v & NM_TIMESTAMP_ENABLED_MASK) != 0)
            dst->timestampEnabled = true;

        if((v & NM_PICOSECONDS_ENABLED_MASK) != 0)
            dst->picosecondsEnabled = true;

        if((v & NM_EXTENDEDFLAGS2_ENABLED_MASK) != 0) {
            v = 0;
            rv = UA_Byte_decodeBinary(src, offset, &v);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;

            if((v & NM_CHUNK_MESSAGE_MASK) != 0)
                dst->chunkMessage = true;

            if((v & NM_PROMOTEDFIELDS_ENABLED_MASK) != 0)
                dst->promotedFieldsEnabled = true;

            v = v & NM_NETWORK_MSG_TYPE_MASK;
            v = (UA_Byte) (v >> NM_SHIFT_LEN);
            dst->networkMessageType = (UA_NetworkMessageType)v;
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

        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    if(dst->dataSetClassIdEnabled) {
        rv = UA_Guid_decodeBinary(src, offset, &(dst->dataSetClassId));
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    // GroupHeader
    if(dst->groupHeaderEnabled) { 
        v = 0;
        rv = UA_Byte_decodeBinary(src, offset, &v);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;

        if((v & GROUP_HEADER_WRITER_GROUPID_ENABLED) != 0)
            dst->groupHeader.writerGroupIdEnabled = true;

        if((v & GROUP_HEADER_GROUP_VERSION_ENABLED) != 0)
            dst->groupHeader.groupVersionEnabled = true;

        if((v & GROUP_HEADER_NM_NUMBER_ENABLED) != 0)
            dst->groupHeader.networkMessageNumberEnabled = true;

        if((v & GROUP_HEADER_SEQUENCE_NUMBER_ENABLED) != 0)
            dst->groupHeader.sequenceNumberEnabled = true;

        if(dst->groupHeader.writerGroupIdEnabled) {
            rv = UA_UInt16_decodeBinary(src, offset, &dst->groupHeader.writerGroupId);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }

        if(dst->groupHeader.groupVersionEnabled) {
            rv = UA_UInt32_decodeBinary(src, offset, &dst->groupHeader.groupVersion);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }

        if(dst->groupHeader.networkMessageNumberEnabled) {
            rv = UA_UInt16_decodeBinary(src, offset, &dst->groupHeader.networkMessageNumber);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }

        if(dst->groupHeader.sequenceNumberEnabled) {
            rv = UA_UInt16_decodeBinary(src, offset, &dst->groupHeader.sequenceNumber);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }
    }

    // Payload-Header
    if(dst->payloadHeaderEnabled) {
        if(dst->networkMessageType != UA_NETWORKMESSAGE_DATASET)
            return UA_STATUSCODE_BADNOTIMPLEMENTED;

        rv = UA_Byte_decodeBinary(src, offset, &dst->payloadHeader.dataSetPayloadHeader.count);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;

        dst->payloadHeader.dataSetPayloadHeader.dataSetWriterIds =
            (UA_UInt16 *)UA_Array_new(dst->payloadHeader.dataSetPayloadHeader.count,
                                      &UA_TYPES[UA_TYPES_UINT16]);
        for (UA_Byte i = 0; i < dst->payloadHeader.dataSetPayloadHeader.count; i++) {
            rv = UA_UInt16_decodeBinary(src, offset,
                                        &dst->payloadHeader.dataSetPayloadHeader.dataSetWriterIds[i]);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }
    }

    // Timestamp
    if(dst->timestampEnabled) {
        rv = UA_DateTime_decodeBinary(src, offset, &(dst->timestamp));
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    // Picoseconds
    if(dst->picosecondsEnabled) {
        rv = UA_UInt16_decodeBinary(src, offset, &(dst->picoseconds));
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    // PromotedFields 
    if(dst->promotedFieldsEnabled) {
        // Size
        UA_UInt16 promotedFieldsSize = 0;
        rv = UA_UInt16_decodeBinary(src, offset, &promotedFieldsSize);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;

        // promotedFieldsSize: here size in Byte, not the number of objects!
        if(promotedFieldsSize > 0) {
            // store offset, later compared with promotedFieldsSize 
            size_t offsetEnd = (*offset) + promotedFieldsSize;

            unsigned int counter = 0;
            do {
                if(counter == 0) {
                    dst->promotedFields = (UA_Variant*)UA_malloc(UA_TYPES[UA_TYPES_VARIANT].memSize);
                    // set promotedFieldsSize to the number of objects
                    dst->promotedFieldsSize = (UA_UInt16) (counter + 1);
                } else {
                    dst->promotedFields = (UA_Variant*)
                        UA_realloc(dst->promotedFields,
                                   UA_TYPES[UA_TYPES_VARIANT].memSize * (counter + 1));
                    // set promotedFieldsSize to the number of objects
                    dst->promotedFieldsSize = (UA_UInt16) (counter + 1);
                }

                UA_Variant_init(&dst->promotedFields[counter]);
                rv = UA_Variant_decodeBinary(src, offset, &dst->promotedFields[counter]);
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;
                counter++;
            } while ((*offset) < offsetEnd);
        }
    }

    // SecurityHeader
    if(dst->securityEnabled) {
        // SecurityFlags
        v = 0;
        rv = UA_Byte_decodeBinary(src, offset, &v);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;

        if((v & SECURITY_HEADER_NM_SIGNED) != 0)
            dst->securityHeader.networkMessageSigned = true;

        if((v & SECURITY_HEADER_NM_ENCRYPTED) != 0)
            dst->securityHeader.networkMessageEncrypted = true;

        if((v & SECURITY_HEADER_SEC_FOOTER_ENABLED) != 0)
            dst->securityHeader.securityFooterEnabled = true;

        if((v & SECURITY_HEADER_FORCE_KEY_RESET) != 0)
            dst->securityHeader.forceKeyReset = true;

        // SecurityTokenId
        rv = UA_UInt32_decodeBinary(src, offset, &dst->securityHeader.securityTokenId);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;

        // NonceLength
        rv = UA_Byte_decodeBinary(src, offset, &dst->securityHeader.nonceLength);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;

        // MessageNonce
        if(dst->securityHeader.nonceLength > 0) {
            rv = UA_ByteString_allocBuffer(&dst->securityHeader.messageNonce,
                                           dst->securityHeader.nonceLength);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;

            for (UA_Byte i = 0; i < dst->securityHeader.nonceLength; i++) {
                rv = UA_Byte_decodeBinary(src, offset, &(dst->securityHeader.messageNonce.data[i]));
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;
            }
        }

        // SecurityFooterSize
        if(dst->securityHeader.securityFooterEnabled) {
            rv = UA_UInt16_decodeBinary(src, offset, &dst->securityHeader.securityFooterSize);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }
    }

    // Payload
    if(dst->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    UA_Byte count = 1;
    if(dst->payloadHeaderEnabled) {
        count = dst->payloadHeader.dataSetPayloadHeader.count;
        if(count > 1) {
            dst->payload.dataSetPayload.sizes = (UA_UInt16 *)UA_Array_new(count, &UA_TYPES[UA_TYPES_UINT16]);
            for (UA_Byte i = 0; i < count; i++) {
                rv = UA_UInt16_decodeBinary(src, offset, &(dst->payload.dataSetPayload.sizes[i]));
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;
            }
        }
    }

    dst->payload.dataSetPayload.dataSetMessages = (UA_DataSetMessage*)
        UA_calloc(count, sizeof(UA_DataSetMessage));
    for(UA_Byte i = 0; i < count; i++) {
        rv = UA_DataSetMessage_decodeBinary(src, offset, &(dst->payload.dataSetPayload.dataSetMessages[i]));
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    if(rv != UA_STATUSCODE_GOOD)
        return rv;

    if(dst->securityEnabled) {
        // SecurityFooter
        if(dst->securityHeader.securityFooterEnabled && (dst->securityHeader.securityFooterSize > 0)) {
            rv = UA_ByteString_allocBuffer(&dst->securityFooter, dst->securityHeader.securityFooterSize);
            if (rv != UA_STATUSCODE_GOOD)
                return rv;

            for (UA_Byte i = 0; i < dst->securityHeader.securityFooterSize; i++) {
                rv = UA_Byte_decodeBinary(src, offset, &(dst->securityFooter.data[i]));
                if (rv != UA_STATUSCODE_GOOD)
                    return rv;
            }
        }

        // Signature
        if(dst->securityHeader.networkMessageSigned) {
            rv = UA_ByteString_decodeBinary(src, offset, &(dst->signature));
            if (rv != UA_STATUSCODE_GOOD)
                return rv;
        }
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NetworkMessage_decodeBinary(const UA_ByteString *src, size_t *offset, UA_NetworkMessage* dst) {
    UA_StatusCode retval = UA_NetworkMessage_decodeBinaryInternal(src, offset, dst);

    if(retval != UA_STATUSCODE_GOOD)
        UA_NetworkMessage_deleteMembers(dst);

    return retval;
}

size_t UA_NetworkMessage_calcSizeBinary(const UA_NetworkMessage* p) {
    size_t retval = 0;
    UA_Byte byte;
    size_t size = UA_Byte_calcSizeBinary(&byte); // UADPVersion + UADPFlags
    if(UA_NetworkMessage_ExtendedFlags1Enabled(p)) {
        size += UA_Byte_calcSizeBinary(&byte);
        if(UA_NetworkMessage_ExtendedFlags2Enabled(p))
            size += UA_Byte_calcSizeBinary(&byte);
    }

    if(p->publisherIdEnabled) {
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

        if(p->groupHeader.writerGroupIdEnabled)
            size += UA_UInt16_calcSizeBinary(&p->groupHeader.writerGroupId);

        if(p->groupHeader.groupVersionEnabled)
            size += UA_UInt32_calcSizeBinary(&p->groupHeader.groupVersion);

        if(p->groupHeader.networkMessageNumberEnabled)
            size += UA_UInt16_calcSizeBinary(&p->groupHeader.networkMessageNumber);

        if(p->groupHeader.sequenceNumberEnabled)
            size += UA_UInt16_calcSizeBinary(&p->groupHeader.sequenceNumber);
    }

    // Payload Header
    if(p->payloadHeaderEnabled) {
        if(p->networkMessageType == UA_NETWORKMESSAGE_DATASET) {
            size += UA_Byte_calcSizeBinary(&p->payloadHeader.dataSetPayloadHeader.count);
            if(p->payloadHeader.dataSetPayloadHeader.dataSetWriterIds != NULL) {
                size += UA_UInt16_calcSizeBinary(&p->payloadHeader.dataSetPayloadHeader.dataSetWriterIds[0]) *
                    p->payloadHeader.dataSetPayloadHeader.count;
            } else {
                return 0; /* no dataSetWriterIds given! */
            }
        } else {
            // not implemented
        }
    }

    if(p->timestampEnabled)
        size += UA_DateTime_calcSizeBinary(&p->timestamp);

    if(p->picosecondsEnabled)
        size += UA_UInt16_calcSizeBinary(&p->picoseconds);

    if(p->promotedFieldsEnabled) { 
        size += UA_UInt16_calcSizeBinary(&p->promotedFieldsSize);
        for (UA_UInt16 i = 0; i < p->promotedFieldsSize; i++)
            size += UA_Variant_calcSizeBinary(&p->promotedFields[i]);
    }

    if(p->securityEnabled) {
        size += UA_Byte_calcSizeBinary(&byte);
        size += UA_UInt32_calcSizeBinary(&p->securityHeader.securityTokenId);
        size += UA_Byte_calcSizeBinary(&p->securityHeader.nonceLength);
        if(p->securityHeader.nonceLength > 0)
            size += (UA_Byte_calcSizeBinary(&p->securityHeader.messageNonce.data[0]) * p->securityHeader.nonceLength);
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

        for (size_t i = 0; i < count; i++)
            size += UA_DataSetMessage_calcSizeBinary(&(p->payload.dataSetPayload.dataSetMessages[i]));
    }

    if (p->securityEnabled) {
        if (p->securityHeader.securityFooterEnabled)
            size += p->securityHeader.securityFooterSize;

        if (p->securityHeader.networkMessageSigned)
            size += UA_ByteString_calcSizeBinary(&p->signature);
    }

    retval = size;
    return retval;
}

void
UA_NetworkMessage_deleteMembers(UA_NetworkMessage* p) {
    if(p->promotedFieldsEnabled)
        UA_Array_delete(p->promotedFields, p->promotedFieldsSize, &UA_TYPES[UA_TYPES_VARIANT]);

    if(p->securityEnabled && (p->securityHeader.nonceLength > 0))
        UA_ByteString_deleteMembers(&p->securityHeader.messageNonce);

    if(p->networkMessageType == UA_NETWORKMESSAGE_DATASET) {
        if(p->payloadHeaderEnabled) {
            if(p->payloadHeader.dataSetPayloadHeader.dataSetWriterIds != NULL) {
                UA_Array_delete(p->payloadHeader.dataSetPayloadHeader.dataSetWriterIds,
                                p->payloadHeader.dataSetPayloadHeader.count, &UA_TYPES[UA_TYPES_UINT16]);
            }

            if(p->payload.dataSetPayload.sizes != NULL) { 
                UA_Array_delete(p->payload.dataSetPayload.sizes,
                                p->payloadHeader.dataSetPayloadHeader.count, &UA_TYPES[UA_TYPES_UINT16]);
            }
        }

        if(p->payload.dataSetPayload.dataSetMessages != NULL) {
            UA_Byte count = 1;
            if(p->payloadHeaderEnabled)
                count = p->payloadHeader.dataSetPayloadHeader.count;
            
            for (size_t i = 0; i < count; i++)
                UA_DataSetMessage_free(&(p->payload.dataSetPayload.dataSetMessages[i]));

            UA_free(p->payload.dataSetPayload.dataSetMessages);
        }
    }

    if(p->securityHeader.securityFooterEnabled && (p->securityHeader.securityFooterSize > 0))
        UA_ByteString_deleteMembers(&p->securityFooter);

    if(p->messageIdEnabled){
        UA_String_deleteMembers(&p->messageId);
    }
    
    if(p->publisherIdEnabled && p->publisherIdType == UA_PUBLISHERDATATYPE_STRING){
        UA_String_deleteMembers(&p->publisherId.publisherIdString);
    }
    
    memset(p, 0, sizeof(UA_NetworkMessage));
}

void UA_NetworkMessage_delete(UA_NetworkMessage* p) {
    UA_NetworkMessage_deleteMembers(p);
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
    UA_StatusCode retval = UA_STATUSCODE_BADNOTIMPLEMENTED;

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
    if(rv != UA_STATUSCODE_GOOD)
        return rv;
    
    // DataSetFlags2
    if(UA_DataSetMessageHeader_DataSetFlags2Enabled(src)) {
        v = (UA_Byte)src->dataSetMessageType;

        if(src->timestampEnabled)
            v |= DS_MESSAGEHEADER_TIMESTAMP_ENABLED_MASK;

        if(src->picoSecondsIncluded)
            v |= DS_MESSAGEHEADER_PICOSECONDS_INCLUDED_MASK;

        rv = UA_Byte_encodeBinary(&v, bufPos, bufEnd);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    // DataSetMessageSequenceNr
    if(src->dataSetMessageSequenceNrEnabled) { 
        rv = UA_UInt16_encodeBinary(&src->dataSetMessageSequenceNr, bufPos, bufEnd);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    // Timestamp
    if(src->timestampEnabled) {
        rv = UA_DateTime_encodeBinary(&(src->timestamp), bufPos, bufEnd); /* UtcTime */
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    // PicoSeconds
    if(src->picoSecondsIncluded) {
        rv = UA_UInt16_encodeBinary(&(src->picoSeconds), bufPos, bufEnd);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    // Status
    if(src->statusEnabled) {
        rv = UA_UInt16_encodeBinary(&(src->status), bufPos, bufEnd);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    // ConfigVersionMajorVersion
    if(src->configVersionMajorVersionEnabled) {
        rv = UA_UInt32_encodeBinary(&(src->configVersionMajorVersion), bufPos, bufEnd);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    // ConfigVersionMinorVersion
    if(src->configVersionMinorVersionEnabled) {
        rv = UA_UInt32_encodeBinary(&(src->configVersionMinorVersion), bufPos, bufEnd);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    retval = UA_STATUSCODE_GOOD;
    return retval;
}

UA_StatusCode
UA_DataSetMessageHeader_decodeBinary(const UA_ByteString *src, size_t *offset,
                                     UA_DataSetMessageHeader* dst) {
    UA_StatusCode retval = UA_STATUSCODE_BADNOTIMPLEMENTED;

    memset(dst, 0, sizeof(UA_DataSetMessageHeader));
    UA_Byte v = 0;
    UA_StatusCode rv = UA_Byte_decodeBinary(src, offset, &v);
    if(rv != UA_STATUSCODE_GOOD)
        return rv;

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
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
        
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
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    } else {
        dst->dataSetMessageSequenceNr = 0;
    }

    if(dst->timestampEnabled) {
        rv = UA_DateTime_decodeBinary(src, offset, &dst->timestamp); /* UtcTime */
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    } else {
        dst->timestamp = 0;
    }

    if(dst->picoSecondsIncluded) {
        rv = UA_UInt16_decodeBinary(src, offset, &dst->picoSeconds);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    } else {
        dst->picoSeconds = 0;
    }

    if(dst->statusEnabled) {
        rv = UA_UInt16_decodeBinary(src, offset, &dst->status);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    } else {
        dst->status = 0;
    }

    if(dst->configVersionMajorVersionEnabled) {
        rv = UA_UInt32_decodeBinary(src, offset, &dst->configVersionMajorVersion);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    } else {
        dst->configVersionMajorVersion = 0;
    }

    if(dst->configVersionMinorVersionEnabled) {
        rv = UA_UInt32_decodeBinary(src, offset, &dst->configVersionMinorVersion);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    } else {
        dst->configVersionMinorVersion = 0;
    }

    retval = UA_STATUSCODE_GOOD;
    return retval;
}

size_t
UA_DataSetMessageHeader_calcSizeBinary(const UA_DataSetMessageHeader* p) {
    UA_Byte byte;
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
    if(rv != UA_STATUSCODE_GOOD)
        return rv;

    if(src->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
        if(src->header.fieldEncoding != UA_FIELDENCODING_RAWDATA) {
            rv = UA_UInt16_encodeBinary(&(src->data.keyFrameData.fieldCount), bufPos, bufEnd);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }

        if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
            for (UA_UInt16 i = 0; i < src->data.keyFrameData.fieldCount; i++) {
                rv = UA_Variant_encodeBinary(&(src->data.keyFrameData.dataSetFields[i].value), bufPos, bufEnd);
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;
            }
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
            return UA_STATUSCODE_BADNOTIMPLEMENTED;
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
            for (UA_UInt16 i = 0; i < src->data.keyFrameData.fieldCount; i++) {
                rv = UA_DataValue_encodeBinary(&(src->data.keyFrameData.dataSetFields[i]), bufPos, bufEnd);
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;
            }
        }
    } else if(src->header.dataSetMessageType == UA_DATASETMESSAGE_DATADELTAFRAME) {
        // Encode Delta Frame
        // Here the FieldCount is always present
        rv = UA_UInt16_encodeBinary(&(src->data.keyFrameData.fieldCount), bufPos, bufEnd);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;

        if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
            for (UA_UInt16 i = 0; i < src->data.deltaFrameData.fieldCount; i++) {
                rv = UA_UInt16_encodeBinary(&(src->data.deltaFrameData.deltaFrameFields[i].fieldIndex), bufPos, bufEnd);
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;
                
                rv = UA_Variant_encodeBinary(&(src->data.deltaFrameData.deltaFrameFields[i].fieldValue.value), bufPos, bufEnd);
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;
            }
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
            return UA_STATUSCODE_BADNOTIMPLEMENTED;
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
            for (UA_UInt16 i = 0; i < src->data.deltaFrameData.fieldCount; i++) {
                rv = UA_UInt16_encodeBinary(&(src->data.deltaFrameData.deltaFrameFields[i].fieldIndex), bufPos, bufEnd);
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;

                rv = UA_DataValue_encodeBinary(&(src->data.deltaFrameData.deltaFrameFields[i].fieldValue), bufPos, bufEnd);
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;
            }
        }
    } else if(src->header.dataSetMessageType != UA_DATASETMESSAGE_KEEPALIVE) {
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    /* Keep-Alive Message contains no Payload Data */
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_DataSetMessage_decodeBinary(const UA_ByteString *src, size_t *offset, UA_DataSetMessage* dst) {
    memset(dst, 0, sizeof(UA_DataSetMessage));
    UA_StatusCode rv = UA_DataSetMessageHeader_decodeBinary(src, offset, &dst->header);
    if(rv != UA_STATUSCODE_GOOD)
        return rv;

    if(dst->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
        if(dst->header.fieldEncoding != UA_FIELDENCODING_RAWDATA) {
            rv = UA_UInt16_decodeBinary(src, offset, &dst->data.keyFrameData.fieldCount);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;

            if(dst->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
                dst->data.keyFrameData.dataSetFields =
                    (UA_DataValue *)UA_Array_new(dst->data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
                for (UA_UInt16 i = 0; i < dst->data.keyFrameData.fieldCount; i++) {
                    UA_DataValue_init(&dst->data.keyFrameData.dataSetFields[i]);
                    rv = UA_Variant_decodeBinary(src, offset, &dst->data.keyFrameData.dataSetFields[i].value);
                    if(rv != UA_STATUSCODE_GOOD)
                        return rv;
                    dst->data.keyFrameData.dataSetFields[i].hasValue = true;
                }
            } else if(dst->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
                return UA_STATUSCODE_BADNOTIMPLEMENTED;
            } else if(dst->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
                dst->data.keyFrameData.dataSetFields =
                    (UA_DataValue *)UA_Array_new(dst->data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
                for (UA_UInt16 i = 0; i < dst->data.keyFrameData.fieldCount; i++) {
                    rv = UA_DataValue_decodeBinary(src, offset, &(dst->data.keyFrameData.dataSetFields[i]));
                    if(rv != UA_STATUSCODE_GOOD)
                        return rv;
                }
            }
        }
    } else if(dst->header.dataSetMessageType == UA_DATASETMESSAGE_DATADELTAFRAME) {
        if(dst->header.fieldEncoding != UA_FIELDENCODING_RAWDATA) {
            rv = UA_UInt16_decodeBinary(src, offset, &dst->data.deltaFrameData.fieldCount);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;

            if(dst->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
                size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dst->data.deltaFrameData.fieldCount;
                dst->data.deltaFrameData.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);
                for (UA_UInt16 i = 0; i < dst->data.deltaFrameData.fieldCount; i++) {
                    rv = UA_UInt16_decodeBinary(src, offset, &dst->data.deltaFrameData.deltaFrameFields[i].fieldIndex);
                    if(rv != UA_STATUSCODE_GOOD)
                        return rv;
                    
                    UA_DataValue_init(&dst->data.deltaFrameData.deltaFrameFields[i].fieldValue);
                    rv = UA_Variant_decodeBinary(src, offset, &dst->data.deltaFrameData.deltaFrameFields[i].fieldValue.value);
                    if(rv != UA_STATUSCODE_GOOD)
                        return rv;

                    dst->data.deltaFrameData.deltaFrameFields[i].fieldValue.hasValue = true;
                }
            } else if(dst->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
                return UA_STATUSCODE_BADNOTIMPLEMENTED;
            } else if(dst->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
                size_t memsize = sizeof(UA_DataSetMessage_DeltaFrameField) * dst->data.deltaFrameData.fieldCount;
                dst->data.deltaFrameData.deltaFrameFields = (UA_DataSetMessage_DeltaFrameField*)UA_malloc(memsize);
                for (UA_UInt16 i = 0; i < dst->data.deltaFrameData.fieldCount; i++) {
                    rv = UA_UInt16_decodeBinary(src, offset, &dst->data.deltaFrameData.deltaFrameFields[i].fieldIndex);
                    if(rv != UA_STATUSCODE_GOOD)
                        return rv;
                    
                    rv = UA_DataValue_decodeBinary(src, offset, &(dst->data.deltaFrameData.deltaFrameFields[i].fieldValue));
                    if(rv != UA_STATUSCODE_GOOD)
                        return rv;
                }
            }
        }
    } else if(dst->header.dataSetMessageType != UA_DATASETMESSAGE_KEEPALIVE) {
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    /* Keep-Alive Message contains no Payload Data */
    return UA_STATUSCODE_GOOD;
}

size_t
UA_DataSetMessage_calcSizeBinary(const UA_DataSetMessage* p) {
    size_t size = UA_DataSetMessageHeader_calcSizeBinary(&p->header);

    if(p->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
        if(p->header.fieldEncoding != UA_FIELDENCODING_RAWDATA)
            size += UA_calcSizeBinary(&p->data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_UINT16]);

        if(p->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
            for (UA_UInt16 i = 0; i < p->data.keyFrameData.fieldCount; i++)
                size += UA_calcSizeBinary(&p->data.keyFrameData.dataSetFields[i].value, &UA_TYPES[UA_TYPES_VARIANT]);
        } else if(p->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
            // not implemented
        } else if(p->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
            for (UA_UInt16 i = 0; i < p->data.keyFrameData.fieldCount; i++)
                size += UA_calcSizeBinary(&p->data.keyFrameData.dataSetFields[i], &UA_TYPES[UA_TYPES_DATAVALUE]);
        }
    } else if(p->header.dataSetMessageType == UA_DATASETMESSAGE_DATADELTAFRAME) {
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

void UA_DataSetMessage_free(const UA_DataSetMessage* p) {
    if(p->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
        if(p->data.keyFrameData.dataSetFields != NULL)
            UA_Array_delete(p->data.keyFrameData.dataSetFields, p->data.keyFrameData.fieldCount,
                            &UA_TYPES[UA_TYPES_DATAVALUE]);
        
        /* Json keys */
        if(p->data.keyFrameData.fieldNames != NULL){
            UA_Array_delete(p->data.keyFrameData.fieldNames, p->data.keyFrameData.fieldCount,
                            &UA_TYPES[UA_TYPES_STRING]);
        }
    } else if(p->header.dataSetMessageType == UA_DATASETMESSAGE_DATADELTAFRAME) {
        if(p->data.deltaFrameData.deltaFrameFields != NULL) {
            for(UA_UInt16 i = 0; i < p->data.deltaFrameData.fieldCount; i++) {
                if(p->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
                    UA_DataValue_deleteMembers(&p->data.deltaFrameData.deltaFrameFields[i].fieldValue);
                } else if(p->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
                    UA_Variant_deleteMembers(&p->data.deltaFrameData.deltaFrameFields[i].fieldValue.value);
                }
            }
            UA_free(p->data.deltaFrameData.deltaFrameFields);
            
            UA_Array_delete(p->data.deltaFrameData.fieldNames, p->data.deltaFrameData.fieldCount,
                            &UA_TYPES[UA_TYPES_STRING]);
        }
    }
}

/* ----Json------ */

static UA_StatusCode
UA_DataSetMessage_encodeJson(const UA_DataSetMessage* src, UA_UInt16 dataSetWriterId, UA_Byte **bufPos,
                               const UA_Byte *bufEnd, UA_Boolean useReversible) {
    CtxJson ctx;
    ctx.pos = *bufPos;
    ctx.end = bufEnd;
    ctx.depth = 0;
    
    status rv = UA_STATUSCODE_GOOD;
    
    encodingJsonStartObject(&ctx);
    
    /* DataSetWriterId */
    rv = writeKey(&ctx, "DataSetWriterId", UA_FALSE);
    rv = UA_encodeJson(&dataSetWriterId, &UA_TYPES[UA_TYPES_UINT16], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);
    if(rv != UA_STATUSCODE_GOOD)
        return rv;
    
    
    /* DataSetMessageSequenceNr */
    if(src->header.dataSetMessageSequenceNrEnabled) { 
        rv = writeKey(&ctx, "SequenceNumber", UA_TRUE);
        rv = UA_encodeJson(&(src->header.dataSetMessageSequenceNr), &UA_TYPES[UA_TYPES_UINT16], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    
    /* MetaDataVersion */
    if(src->header.configVersionMajorVersionEnabled || src->header.configVersionMinorVersionEnabled) {
        rv = writeKey(&ctx, "MetaDataVersion", UA_TRUE);
        UA_ConfigurationVersionDataType cvd;
        cvd.majorVersion = src->header.configVersionMajorVersion;
        cvd.minorVersion = src->header.configVersionMinorVersion;
        rv = UA_encodeJson(&cvd, &UA_TYPES[UA_TYPES_CONFIGURATIONVERSIONDATATYPE], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }
    
    /* Timestamp */
    if(src->header.timestampEnabled) {
        rv = writeKey(&ctx, "Timestamp", UA_TRUE);
        rv = UA_encodeJson(&(src->header.timestamp), &UA_TYPES[UA_TYPES_DATETIME], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    /* Status */
    if(src->header.statusEnabled) {
        rv = writeKey(&ctx, "Status", UA_TRUE);
        rv = UA_encodeJson(&(src->header.status), &UA_TYPES[UA_TYPES_STATUSCODE], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }
    
    rv = writeKey(&ctx, "Payload", UA_TRUE);
    encodingJsonStartObject(&ctx); //Payload
    
    /* TODO: currently no difference between delta and key frames. Own dataSetMessageType for json?*/
    if(src->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
        
        if(src->data.keyFrameData.fieldNames == NULL){
            return UA_STATUSCODE_BADENCODINGERROR;
        }
        
        if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {

            for (UA_UInt16 i = 0; i < src->data.keyFrameData.fieldCount; i++) {
                writeKey_UA_String(&ctx, &src->data.keyFrameData.fieldNames[i], i==0?UA_FALSE: UA_TRUE);
                rv = UA_encodeJson(&(src->data.keyFrameData.dataSetFields[i].value), 
                        &UA_TYPES[UA_TYPES_VARIANT], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, ctx.useReversible);
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;
            }
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
            return UA_STATUSCODE_BADNOTIMPLEMENTED;
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
            for (UA_UInt16 i = 0; i < src->data.keyFrameData.fieldCount; i++) {
                writeKey_UA_String(&ctx, &src->data.keyFrameData.fieldNames[i], i==0?UA_FALSE: UA_TRUE);
                rv = UA_encodeJson(&(src->data.keyFrameData.dataSetFields[i]), &UA_TYPES[UA_TYPES_DATAVALUE], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;
            }
        }
    }else if(src->header.dataSetMessageType == UA_DATASETMESSAGE_DATADELTAFRAME){
        
        if(src->data.deltaFrameData.fieldNames == NULL){
            return UA_STATUSCODE_BADENCODINGERROR;
        }
        
        if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
             for (UA_UInt16 i = 0; i < src->data.deltaFrameData.fieldCount; i++) {
                writeKey_UA_String(&ctx, &src->data.deltaFrameData.fieldNames[i], i==0?UA_FALSE: UA_TRUE);
                rv = UA_encodeJson(&(src->data.deltaFrameData.deltaFrameFields[i].fieldValue.value), &UA_TYPES[UA_TYPES_VARIANT], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;
            }
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
            return UA_STATUSCODE_BADNOTIMPLEMENTED;
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
            for (UA_UInt16 i = 0; i < src->data.deltaFrameData.fieldCount; i++) {
                writeKey_UA_String(&ctx, &src->data.deltaFrameData.fieldNames[i], i==0?UA_FALSE: UA_TRUE);
                rv = UA_encodeJson(&(src->data.deltaFrameData.deltaFrameFields[i].fieldValue), &UA_TYPES[UA_TYPES_DATAVALUE], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;
            }
        }
    }
    encodingJsonEndObject(&ctx); /* Payload */
    
    encodingJsonEndObject(&ctx); /* DataSetMessage */
    
    *bufPos = ctx.pos;
    bufEnd = ctx.end;
    return rv;
}

UA_StatusCode
UA_NetworkMessage_encodeJson(const UA_NetworkMessage* src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd, UA_Boolean useReversible) {
    
    status rv = UA_STATUSCODE_GOOD;
    if(src->networkMessageType == UA_NETWORKMESSAGE_DATASET) {
        CtxJson ctx;
        ctx.pos = *bufPos;
        ctx.end = bufEnd;
        ctx.depth = 0;
        encodingJsonStartObject(&ctx);

        /* MessageId */
        rv = writeKey(&ctx, "MessageId", UA_FALSE);
        UA_Guid guid = UA_Guid_random();
        rv = UA_encodeJson(&guid, &UA_TYPES[UA_TYPES_GUID], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);

        /* MessageType */
        rv = writeKey(&ctx, "MessageType", UA_TRUE);
        UA_String s = UA_STRING("ua-data");
        rv = UA_encodeJson(&s, &UA_TYPES[UA_TYPES_STRING], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);

        /* PublisherId */
        if(src->publisherIdEnabled) {

            rv = writeKey(&ctx, "PublisherId", UA_TRUE);

            switch (src->publisherIdType) {
            case UA_PUBLISHERDATATYPE_BYTE:
                rv = UA_encodeJson(&src->publisherId.publisherIdByte, &UA_TYPES[UA_TYPES_BYTE], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);
                break;

            case UA_PUBLISHERDATATYPE_UINT16:
                rv = UA_encodeJson(&src->publisherId.publisherIdUInt16, &UA_TYPES[UA_TYPES_UINT16], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);
                break;

            case UA_PUBLISHERDATATYPE_UINT32:
                rv = UA_encodeJson(&src->publisherId.publisherIdUInt32, &UA_TYPES[UA_TYPES_UINT32], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);
                break;

            case UA_PUBLISHERDATATYPE_UINT64:
                rv = UA_encodeJson(&src->publisherId.publisherIdUInt64, &UA_TYPES[UA_TYPES_UINT64], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);
                break;

            case UA_PUBLISHERDATATYPE_STRING:
                rv = UA_encodeJson(&src->publisherId.publisherIdString, &UA_TYPES[UA_TYPES_STRING], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);
                break;

            default:
                rv = UA_STATUSCODE_BADINTERNALERROR;
                break;
            }
        }

        if(rv != UA_STATUSCODE_GOOD)
            return rv;


        /* DataSetClassId */
        if(src->dataSetClassIdEnabled) {
            rv = writeKey(&ctx, "DataSetClassId", UA_TRUE);
            rv = UA_encodeJson(&src->dataSetClassId, &UA_TYPES[UA_TYPES_GUID], &ctx.pos, &ctx.end, NULL, 0, NULL, 0, useReversible);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }

    
        /* Payload: DataSetMessages */
        UA_Byte count = src->payloadHeader.dataSetPayloadHeader.count;
        if(count > 0){
            
            UA_UInt16 *dataSetWriterIds = src->payloadHeader.dataSetPayloadHeader.dataSetWriterIds;
            if(!dataSetWriterIds){
                return UA_STATUSCODE_BADENCODINGERROR;
            }
            
            UA_Boolean commaNeeded = UA_FALSE;
            rv = writeKey(&ctx, "Messages", UA_TRUE);
            encodingJsonStartArray(&ctx);
            for (UA_UInt16 i = 0; i < count; i++) {
                writeComma(&ctx, commaNeeded);
                commaNeeded = UA_TRUE;
                rv = UA_DataSetMessage_encodeJson(&(src->payload.dataSetPayload.dataSetMessages[i]), dataSetWriterIds[i], &ctx.pos, ctx.end, useReversible);
                if(rv != UA_STATUSCODE_GOOD)
                    return rv;
            }

            encodingJsonEndArray(&ctx);
        }
   
        encodingJsonEndObject(&ctx);
        *bufPos = ctx.pos;
        bufEnd = ctx.end;
    
     } else {
        rv = UA_STATUSCODE_BADNOTIMPLEMENTED;
    }
    return rv;
}

static status MetaDataVersion_decodeJsonInternal(void* cvd, const UA_DataType *type, CtxJson *ctx, ParseCtx *parseCtx, UA_Boolean moveToken){
    return decodeJsonInternal(cvd, &UA_TYPES[UA_TYPES_CONFIGURATIONVERSIONDATATYPE], ctx, parseCtx, UA_TRUE);
}

const char * UA_DECODEKEY_DS_TYPE = ("Type");

static status DataSetPayload_decodeJsonInternal(void* dsmP, const UA_DataType *type, CtxJson *ctx, ParseCtx *parseCtx, UA_Boolean moveToken){
    UA_DataSetMessage* dsm = (UA_DataSetMessage*)dsmP;
    dsm->header.dataSetMessageValid = UA_TRUE;
    if(isJsonNull(ctx, parseCtx)){
        (*parseCtx->index)++;
        //TODO: set size, etc.
        return UA_STATUSCODE_GOOD;
    }
    
    size_t length = (size_t)parseCtx->tokenArray[*parseCtx->index].size;
    //UA_Variant *var = (UA_Variant*)UA_calloc(length, sizeof(UA_Variant));
    //dsm->data.keyFrameData
    UA_String *fieldNames = (UA_String*)UA_calloc(length, sizeof(UA_String));
    //parseCtx->fieldNames = fieldNames;
    //parseCtx->fieldNamesCount = length;
    dsm->data.keyFrameData.fieldNames = fieldNames;
    
    //UA_KeyValuePair *keyValuePairs = (UA_KeyValuePair*)UA_Array_new(dsm->data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    
    dsm->data.keyFrameData.fieldCount = (UA_UInt16)length;
    
    dsm->data.keyFrameData.dataSetFields =
                    (UA_DataValue *)UA_Array_new(dsm->data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
                    
    
    status ret = UA_STATUSCODE_GOOD;
    (*parseCtx->index)++; // We go to first Object key!
    for(size_t i = 0; i < length; ++i) {
        
        ret = getDecodeSignature(UA_TYPES_STRING)(&fieldNames[i], type, ctx, parseCtx, UA_TRUE);
        //ret = getDecodeSignature(UA_TYPES_STRING)(&keyValuePairs[i].key.name, type, ctx, parseCtx, UA_TRUE);
        if(ret != UA_STATUSCODE_GOOD){
            //TODO: handle error, free mem
        }
        
        //Is field a variant or datavalue?
        UA_Boolean isVariant = UA_TRUE;
        size_t searchResultBody = 0;
        lookAheadForKey(UA_DECODEKEY_DS_TYPE, ctx, parseCtx, &searchResultBody);
        if(searchResultBody == 0){
            isVariant = UA_FALSE;
            dsm->header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
        }else{
            dsm->header.fieldEncoding = UA_FIELDENCODING_VARIANT;
        }
        
        UA_DataValue_init(&dsm->data.keyFrameData.dataSetFields[i]);
        
        if(isVariant){
            ret = getDecodeSignature(UA_TYPES_VARIANT)(&dsm->data.keyFrameData.dataSetFields[i].value, type, ctx, parseCtx, UA_TRUE);
            dsm->data.keyFrameData.dataSetFields[i].hasValue = UA_TRUE;
        }else{
            ret = getDecodeSignature(UA_TYPES_DATAVALUE)(&dsm->data.keyFrameData.dataSetFields[i], type, ctx, parseCtx, UA_TRUE);
            dsm->data.keyFrameData.dataSetFields[i].hasValue = UA_TRUE;
        }
        
        if(ret != UA_STATUSCODE_GOOD){
            //TODO: handle error, free mem
            return ret;
        }
    }
    
    return ret;
}

const char * UA_DECODEKEY_DATASETWRITERID = ("DataSetWriterId");
const char * UA_DECODEKEY_SEQUENCENUMBER = ("SequenceNumber");
const char * UA_DECODEKEY_METADATAVERSION = ("MetaDataVersion");
const char * UA_DECODEKEY_TIMESTAMP = ("Timestamp");
const char * UA_DECODEKEY_DSM_STATUS = ("Status");
const char * UA_DECODEKEY_PAYLOAD = ("Payload");


static status
DatasetMessage_Payload_decodeJsonInternal(UA_DataSetMessage* dsm, const UA_DataType *type, CtxJson *ctx, ParseCtx *parseCtx, UA_Boolean moveToken) {
    UA_ConfigurationVersionDataType cvd;
    UA_UInt16 dataSetWriterId; //TODO: Where to store?
    
    dsm->header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    
    const char* fieldNames[6] = {
        UA_DECODEKEY_DATASETWRITERID, 
        UA_DECODEKEY_SEQUENCENUMBER, 
        UA_DECODEKEY_METADATAVERSION,
        UA_DECODEKEY_TIMESTAMP,
        UA_DECODEKEY_DSM_STATUS, 
        UA_DECODEKEY_PAYLOAD};
    UA_Boolean found[6] = {UA_FALSE, UA_FALSE, UA_FALSE, UA_FALSE, UA_FALSE, UA_FALSE};
    void *fieldPointer[6] = {
        &dataSetWriterId, 
        &dsm->header.dataSetMessageSequenceNr, 
        &cvd, 
        &dsm->header.timestamp, 
        &dsm->header.status, 
        dsm};
    decodeJsonSignature functions[] = {
        getDecodeSignature(UA_TYPES_UINT16),
        getDecodeSignature(UA_TYPES_UINT16),
        &MetaDataVersion_decodeJsonInternal,
        getDecodeSignature(UA_TYPES_DATETIME),
        getDecodeSignature(UA_TYPES_UINT16),
        &DataSetPayload_decodeJsonInternal
    };
    
    DecodeContext decodeCtx = {fieldNames, fieldPointer, functions, found, 6};
    status ret = decodeFields(ctx, parseCtx, &decodeCtx, NULL);
    
    if(!found[0]){
        /* no dataSetwriterid. Is mandatory. Abort. */
        ret = UA_STATUSCODE_BADDECODINGERROR;
        goto cleanup;
    }else{
        if(parseCtx->custom != NULL){
            UA_UInt16* dataSetWriterIdsArray = (UA_UInt16*)parseCtx->custom;
            
            if(*parseCtx->currentCustomIndex  < parseCtx->numCustom){
                 dataSetWriterIdsArray[*parseCtx->currentCustomIndex] = dataSetWriterId;
                 (*parseCtx->currentCustomIndex)++;
            }else{
                ret = UA_STATUSCODE_BADDECODINGERROR;
                goto cleanup;
            }
        }else{
            ret = UA_STATUSCODE_BADDECODINGERROR;
            goto cleanup;
        }
    }
    dsm->header.dataSetMessageSequenceNrEnabled = found[1];
    dsm->header.configVersionMajorVersion = cvd.majorVersion;
    dsm->header.configVersionMinorVersion = cvd.minorVersion;
    dsm->header.configVersionMajorVersionEnabled = found[2];
    dsm->header.configVersionMinorVersionEnabled = found[2];
    dsm->header.timestampEnabled = found[3];
    dsm->header.statusEnabled = found[4];
    if(!found[5]){
        //No payload found
        ret = UA_STATUSCODE_BADDECODINGERROR;
        goto cleanup;
    }
    
    dsm->header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dsm->header.picoSecondsIncluded = UA_FALSE;
    dsm->header.dataSetMessageValid = UA_TRUE;
    dsm->header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    
cleanup:
    //UA_ConfigurationVersionDataType_delete(cvd);   
    return ret;
}

static status
DatasetMessage_Array_decodeJsonInternal(void *UA_RESTRICT dst, const UA_DataType *type, CtxJson *ctx, ParseCtx *parseCtx, UA_Boolean moveToken) {
    /* Array! */
    if(getJsmnType(parseCtx) != JSMN_ARRAY){
        return UA_STATUSCODE_BADDECODINGERROR;
    }
    status ret;
    
    size_t length = (size_t)parseCtx->tokenArray[*parseCtx->index].size;
    
    /* Return early for empty arrays */
    if(length == 0) {
        return UA_STATUSCODE_GOOD;
    }

    /* Allocate memory */
    UA_DataSetMessage *dsm = (UA_DataSetMessage*)UA_calloc(length, sizeof(UA_DataSetMessage));
    if(dsm == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    
    memcpy(dst, &dsm, sizeof(void*)); //Copy new Pointer do dest
    

    (*parseCtx->index)++; // We go to first Array member!
    
    /* Decode array members */
    for(size_t i = 0; i < length; ++i) {
        //ret = decodeJsonJumpTable[decode_index]((void*)dsm[i], type, ctx, parseCtx, UA_TRUE);
       
        ret = DatasetMessage_Payload_decodeJsonInternal(&dsm[i], NULL, ctx, parseCtx, UA_TRUE);
        
        if(ret != UA_STATUSCODE_GOOD){
            //TODO: handle error, free mem
        }
    }
    
    return ret;
}

const char * UA_DECODEKEY_MESSAGES = ("Messages");
const char * UA_DECODEKEY_MESSAGETYPE = ("MessageType");
const char * UA_DECODEKEY_MESSAGEID = ("MessageId");
const char * UA_DECODEKEY_PUBLISHERID = ("PublisherId");
const char * UA_DECODEKEY_DATASETCLASSID = ("DataSetClassId");

static status NetworkMessage_decodeJsonInternal(UA_NetworkMessage *dst, CtxJson *ctx, ParseCtx *parseCtx){
    
    memset(dst, 0, sizeof(UA_NetworkMessage));
    
    dst->chunkMessage = UA_FALSE;
    dst->groupHeaderEnabled = UA_FALSE;
    dst->payloadHeaderEnabled = UA_FALSE;
    dst->picosecondsEnabled = UA_FALSE;
    dst->promotedFieldsEnabled = UA_FALSE;
    
    /* Look forward for publisheId, if present check if type if primitve (Number) or String. */
    u8 publishIdTypeIndex = UA_TYPES_STRING;
    {
        size_t searchResultPublishIdType = 0;
        lookAheadForKey(UA_DECODEKEY_MESSAGES, ctx, parseCtx, &searchResultPublishIdType);
        if(searchResultPublishIdType != 0){
            jsmntok_t publishIdToken = parseCtx->tokenArray[searchResultPublishIdType];
            //size_t sizeOfPublishId = (size_t)(parseCtx->tokenArray[searchResultPublishIdType].end 
            //        - parseCtx->tokenArray[searchResultPublishIdType].start);
            if(publishIdToken.type == JSMN_PRIMITIVE){
                publishIdTypeIndex = UA_TYPES_UINT64;
                dst->publisherIdType = UA_PUBLISHERDATATYPE_UINT64; //store in biggest possible
            }else if(publishIdToken.type == JSMN_STRING){
                //String
                publishIdTypeIndex = UA_TYPES_STRING;
                dst->publisherIdType = UA_PUBLISHERDATATYPE_STRING;
                /*if(sizeOfPublishId == 36){
                    char *buf = (char*)(ctx->pos + parseCtx->tokenArray[*parseCtx->index].start);
                    // 8 13 18 23 should be -
                    if(buf[8] == '-' && buf[13] == '-' && buf[18] == '-' && buf[23] == '-'){
                        publishIdTypeIndex = UA_TYPES_GUID;
                    }
                }*/
            }
        }
    }
    
    size_t messageCount = 0;    
    {
        //Is Messages an Array? How big?
        size_t searchResultMessages = 0;
        lookAheadForKey(UA_DECODEKEY_MESSAGES, ctx, parseCtx, &searchResultMessages);
        if(searchResultMessages != 0){
            jsmntok_t bodyToken = parseCtx->tokenArray[searchResultMessages];
            if(bodyToken.type == JSMN_ARRAY){
                messageCount = (size_t)parseCtx->tokenArray[searchResultMessages].size;
            }
        }else{
            //DataSetmessages are in a Array!
            return UA_STATUSCODE_BADNOTIMPLEMENTED;
        }
    }
    
    //Set up custom context for the dataSetwriterId
    size_t currentCustomIndex = 0;
    parseCtx->custom = (void*)UA_calloc(messageCount, sizeof(UA_UInt16));
    parseCtx->currentCustomIndex = &currentCustomIndex;
    parseCtx->numCustom = messageCount;
    
    /* MessageType */
    UA_Boolean isUaData = UA_TRUE;
    size_t searchResultMessageType = 0;
    lookAheadForKey(UA_DECODEKEY_MESSAGETYPE, ctx, parseCtx, &searchResultMessageType);
    if(searchResultMessageType == 0){
        return UA_STATUSCODE_BADDECODINGERROR;
    }else{
        size_t size = (size_t)(parseCtx->tokenArray[searchResultMessageType].end - parseCtx->tokenArray[searchResultMessageType].start);
        char* msgType = (char*)(ctx->pos + parseCtx->tokenArray[searchResultMessageType].start);
        if(size == 7){ //ua-data
            if(strncmp(msgType, "ua-data", size) != 0){
                return UA_STATUSCODE_BADDECODINGERROR;
            }else{
                isUaData = UA_TRUE;
            }
        }else if(size == 11){ //ua-metadata
            if(strncmp(msgType, "ua-metadata", size) != 0){
                return UA_STATUSCODE_BADDECODINGERROR;
            }else{
                isUaData = UA_FALSE;
            }
        }else{
            return UA_STATUSCODE_BADDECODINGERROR;
        }
    }

    if(isUaData){
        /* Network Message */
        status ret = UA_STATUSCODE_GOOD;

        UA_String messageType;
        const char* fieldNames[5] = {UA_DECODEKEY_MESSAGEID, 
            UA_DECODEKEY_MESSAGETYPE, 
            UA_DECODEKEY_PUBLISHERID, 
            UA_DECODEKEY_DATASETCLASSID, 
            UA_DECODEKEY_MESSAGES};
        UA_Boolean found[5] = {UA_FALSE, UA_FALSE, UA_FALSE, UA_FALSE, UA_FALSE};
        void *fieldPointer[5] = {&dst->messageId, &messageType, &dst->publisherId.publisherIdString, &dst->dataSetClassId, &dst->payload.dataSetPayload.dataSetMessages};
        
        //Store publisherId in correct union
        if(publishIdTypeIndex == UA_TYPES_UINT64)
            fieldPointer[2] = &dst->publisherId.publisherIdUInt64;
        
        decodeJsonSignature functions[5] = {
            getDecodeSignature(UA_TYPES_STRING),
            NULL,
            getDecodeSignature(publishIdTypeIndex),
            getDecodeSignature(UA_TYPES_GUID),
            &DatasetMessage_Array_decodeJsonInternal
        };
        DecodeContext decodeCtx = {fieldNames, fieldPointer, functions, found, 5};

        ret = decodeFields(ctx, parseCtx, &decodeCtx, NULL);
        
        dst->messageIdEnabled = found[0];
        dst->publisherIdEnabled = found[2];
        if(dst->publisherIdEnabled){
            dst->publisherIdType = UA_PUBLISHERDATATYPE_STRING;
        }
        dst->dataSetClassIdEnabled = found[3];
        dst->payloadHeaderEnabled = UA_TRUE;
        dst->payloadHeader.dataSetPayloadHeader.count = (UA_Byte)messageCount;
        
        
        //Set the dataSetWriterIds. They are filled in the dataSet decoding.
        dst->payloadHeader.dataSetPayloadHeader.dataSetWriterIds = (UA_UInt16*)parseCtx->custom;
        return ret;
    }else{
        //TODO: MetaData
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }
}


status UA_NetworkMessage_decodeJson(UA_NetworkMessage *dst, UA_ByteString *src){
    /* Set up the context */
    CtxJson ctx;
    ParseCtx parseCtx;
    memset(&parseCtx, 0, sizeof(ParseCtx));
    
    parseCtx.tokenArray = (jsmntok_t*)malloc(sizeof(jsmntok_t) * TOKENCOUNT);
    memset(parseCtx.tokenArray, 0, sizeof(jsmntok_t) * TOKENCOUNT);
    
    status ret = UA_STATUSCODE_GOOD;

    UA_UInt16 tokenIndex = 0;
    ret = tokenize(&parseCtx, &ctx, src, &tokenIndex);
    if(ret != UA_STATUSCODE_GOOD){
        return ret;
    }
    
    ret = NetworkMessage_decodeJsonInternal(dst, &ctx, &parseCtx);
    free(parseCtx.tokenArray);
    return ret;
}



/* Calc size */
static status
UA_DataSetMessage_calcSizeJson(const UA_DataSetMessage* src, UA_UInt16 dataSetWriterId, CtxJson *ctx) {

    status rv = UA_STATUSCODE_GOOD;
    
    //encodingJsonStartObject(&ctx);
    ctx->pos++;
    
    /* DataSetWriterId */
    rv = calcWriteKey(ctx, "DataSetWriterId", UA_FALSE);
    rv = calcJsonInternal(&dataSetWriterId, &UA_TYPES[UA_TYPES_UINT16], ctx);
    if(rv != UA_STATUSCODE_GOOD)
        return 0;
    
    
    /* DataSetMessageSequenceNr */
    if(src->header.dataSetMessageSequenceNrEnabled) { 
        rv = calcWriteKey(ctx, "SequenceNumber", UA_TRUE);
        rv = calcJsonInternal(&(src->header.dataSetMessageSequenceNr), &UA_TYPES[UA_TYPES_UINT16],ctx);
        if(rv != UA_STATUSCODE_GOOD)
            return 0;
    }

    
    /* MetaDataVersion */
    if(src->header.configVersionMajorVersionEnabled || src->header.configVersionMinorVersionEnabled) {
        rv = calcWriteKey(ctx, "MetaDataVersion", UA_TRUE);
        UA_ConfigurationVersionDataType cvd;
        cvd.majorVersion = src->header.configVersionMajorVersion;
        cvd.minorVersion = src->header.configVersionMinorVersion;
        rv = calcJsonInternal(&cvd, &UA_TYPES[UA_TYPES_CONFIGURATIONVERSIONDATATYPE], ctx);
        if(rv != UA_STATUSCODE_GOOD)
            return 0;
    }
    
    /* Timestamp */
    if(src->header.timestampEnabled) {
        rv = calcWriteKey(ctx, "Timestamp", UA_TRUE);
        rv = calcJsonInternal(&(src->header.timestamp), &UA_TYPES[UA_TYPES_DATETIME], ctx);
        if(rv != UA_STATUSCODE_GOOD)
            return 0;
    }

    /* Status */
    if(src->header.statusEnabled) {
        rv = calcWriteKey(ctx, "Status", UA_TRUE);
        rv = calcJsonInternal(&(src->header.status), &UA_TYPES[UA_TYPES_STATUSCODE], ctx);
        if(rv != UA_STATUSCODE_GOOD)
            return 0;
    }
    
    rv = calcWriteKey(ctx, "Payload", UA_TRUE);
    //encodingJsonStartObject(&ctx); //Payload
    ctx->pos++;
    
    /* TODO: currently no difference between delta and key frames. Own dataSetMessageType for json?*/
    if(src->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
        
        if(src->data.keyFrameData.fieldNames == NULL){
            return UA_STATUSCODE_BADENCODINGERROR;
        }
        
        if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {

            for (UA_UInt16 i = 0; i < src->data.keyFrameData.fieldCount; i++) {
                calcWriteKey_UA_String(ctx, &src->data.keyFrameData.fieldNames[i], i==0?UA_FALSE: UA_TRUE);
                rv = calcJsonInternal(&(src->data.keyFrameData.dataSetFields[i].value), 
                        &UA_TYPES[UA_TYPES_VARIANT], ctx);
                if(rv != UA_STATUSCODE_GOOD)
                    return 0;
            }
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
            return UA_STATUSCODE_BADNOTIMPLEMENTED;
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
            for (UA_UInt16 i = 0; i < src->data.keyFrameData.fieldCount; i++) {
                calcWriteKey_UA_String(ctx, &src->data.keyFrameData.fieldNames[i], i==0?UA_FALSE: UA_TRUE);
                rv = calcJsonInternal(&(src->data.keyFrameData.dataSetFields[i]), &UA_TYPES[UA_TYPES_DATAVALUE], ctx);
                if(rv != UA_STATUSCODE_GOOD)
                    return 0;
            }
        }
    }else if(src->header.dataSetMessageType == UA_DATASETMESSAGE_DATADELTAFRAME){
        
        if(src->data.deltaFrameData.fieldNames == NULL){
            return 0;
        }
        
        if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
             for (UA_UInt16 i = 0; i < src->data.deltaFrameData.fieldCount; i++) {
                calcWriteKey_UA_String(ctx, &src->data.deltaFrameData.fieldNames[i], i==0?UA_FALSE: UA_TRUE);
                rv = calcJsonInternal(&(src->data.deltaFrameData.deltaFrameFields[i].fieldValue.value), &UA_TYPES[UA_TYPES_VARIANT], ctx);
                if(rv != UA_STATUSCODE_GOOD)
                    return 0;
            }
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
            return UA_STATUSCODE_BADNOTIMPLEMENTED;
        } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
            for (UA_UInt16 i = 0; i < src->data.deltaFrameData.fieldCount; i++) {
                calcWriteKey_UA_String(ctx, &src->data.deltaFrameData.fieldNames[i], i==0?UA_FALSE: UA_TRUE);
                rv = calcJsonInternal(&(src->data.deltaFrameData.deltaFrameFields[i].fieldValue), &UA_TYPES[UA_TYPES_DATAVALUE], ctx);
                if(rv != UA_STATUSCODE_GOOD)
                    return 0;
            }
        }
    }
    //encodingJsonEndObject(&ctx); /* Payload */
    ctx->pos++;
    
    //encodingJsonEndObject(&ctx); /* DataSetMessage */
    ctx->pos++;
    
    return rv;
}

static status
UA_NetworkMessage_calcSizeJson_internal(const UA_NetworkMessage* src, CtxJson *ctx) {
    
    status rv = UA_STATUSCODE_GOOD;
    if(src->networkMessageType == UA_NETWORKMESSAGE_DATASET) {
        
        //encodingJsonStartObject(&ctx);
        ctx->pos++;

        /* MessageId */
        rv = calcWriteKey(ctx, "MessageId", UA_FALSE);
        UA_Guid guid = UA_Guid_random();
        rv = calcJsonInternal(&guid, &UA_TYPES[UA_TYPES_GUID], ctx);

        /* MessageType */
        rv = calcWriteKey(ctx, "MessageType", UA_TRUE);
        UA_String s = UA_STRING("ua-data");
        rv = calcJsonInternal(&s, &UA_TYPES[UA_TYPES_STRING], ctx);

        /* PublisherId */
        if(src->publisherIdEnabled) {

            rv = calcWriteKey(ctx, "PublisherId", UA_TRUE);

            switch (src->publisherIdType) {
            case UA_PUBLISHERDATATYPE_BYTE:
                rv = calcJsonInternal(&src->publisherId.publisherIdByte, &UA_TYPES[UA_TYPES_BYTE], ctx);
                break;

            case UA_PUBLISHERDATATYPE_UINT16:
                rv = calcJsonInternal(&src->publisherId.publisherIdUInt16, &UA_TYPES[UA_TYPES_UINT16], ctx);
                break;

            case UA_PUBLISHERDATATYPE_UINT32:
                rv = calcJsonInternal(&src->publisherId.publisherIdUInt32, &UA_TYPES[UA_TYPES_UINT32], ctx);
                break;

            case UA_PUBLISHERDATATYPE_UINT64:
                rv = calcJsonInternal(&src->publisherId.publisherIdUInt64, &UA_TYPES[UA_TYPES_UINT64], ctx);
                break;

            case UA_PUBLISHERDATATYPE_STRING:
                rv = calcJsonInternal(&src->publisherId.publisherIdString, &UA_TYPES[UA_TYPES_STRING], ctx);
                break;

            default:
                rv = 0;
                break;
            }
        }

        if(rv != UA_STATUSCODE_GOOD)
            return 0;


        /* DataSetClassId */
        if(src->dataSetClassIdEnabled) {
            rv = calcWriteKey(ctx, "DataSetClassId", UA_TRUE);
            rv = calcJsonInternal(&src->dataSetClassId, &UA_TYPES[UA_TYPES_GUID], ctx);
            if(rv != UA_STATUSCODE_GOOD)
                return 0;
        }

    
        /* Payload: DataSetMessages */
        UA_Byte count = src->payloadHeader.dataSetPayloadHeader.count;
        if(count > 0){
            
            UA_UInt16 *dataSetWriterIds = src->payloadHeader.dataSetPayloadHeader.dataSetWriterIds;
            if(!dataSetWriterIds){
                return 0;
            }
            
            UA_Boolean commaNeeded = UA_FALSE;
            rv = calcWriteKey(ctx, "Messages", UA_TRUE);
            //encodingJsonStartArray(&ctx);
            ctx->pos++;
            for (UA_UInt16 i = 0; i < count; i++) {
                calcWriteComma(ctx, commaNeeded);
                commaNeeded = UA_TRUE;
                rv = UA_DataSetMessage_calcSizeJson(&(src->payload.dataSetPayload.dataSetMessages[i]), dataSetWriterIds[i], ctx);
                if(rv != UA_STATUSCODE_GOOD)
                    return 0;
            }

            //encodingJsonEndArray(&ctx);
            ctx->pos++;
        }
   
        //encodingJsonEndObject(&ctx);
        ctx->pos++;
        
        return rv;
    } else {
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }
}

size_t
UA_NetworkMessage_calcSizeJson(const UA_NetworkMessage* src, UA_Boolean useReversible){
    
    CtxJson ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.pos = 0;
    ctx.depth = 0;
    ctx.useReversible = useReversible;
    //ctx.namespaces = namespaces;
    //ctx.namespacesSize = namespaceSize;
    //ctx.serverUris = serverUris;
    //ctx.serverUrisSize = serverUriSize;
    
    status rv = UA_STATUSCODE_GOOD;
    
    UA_NetworkMessage_calcSizeJson_internal(src, &ctx);
    
    if(rv != UA_STATUSCODE_GOOD){
        return 0;
    }else{
        return (size_t)ctx.pos;
    }
}

#endif /* UA_ENABLE_PUBSUB */
