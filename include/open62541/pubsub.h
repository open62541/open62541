/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Tino Bischoff)
 * Copyright (c) 2017-2019 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2024 Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_PUBSUB_H
#define UA_PUBSUB_H

#include <open62541/common.h>
#include <open62541/server_pubsub.h>

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB

/**
 * .. _raw-pubsub:
 *
 * PubSub NetworkMessage
 * ---------------------
 *
 * The following definitions enable to work directly with PubSub messages. This
 * is not required when :ref:`PubSub is integrated with a server<pubsub>`.
 *
 * DataSet Message
 * ~~~~~~~~~~~~~~~ */

typedef enum {
    UA_FIELDENCODING_VARIANT   = 0,
    UA_FIELDENCODING_RAWDATA   = 1,
    UA_FIELDENCODING_DATAVALUE = 2,
    UA_FIELDENCODING_UNKNOWN   = 3
} UA_FieldEncoding;

typedef enum {
    UA_DATASETMESSAGETYPE_DATAKEYFRAME   = 0,
    UA_DATASETMESSAGE_DATAKEYFRAME       = 0,
    UA_DATASETMESSAGETYPE_DATADELTAFRAME = 1,
    UA_DATASETMESSAGE_DATADELTAFRAME     = 1,
    UA_DATASETMESSAGETYPE_EVENT          = 2,
    UA_DATASETMESSAGE_EVENT              = 2,
    UA_DATASETMESSAGETYPE_KEEPALIVE      = 3,
    UA_DATASETMESSAGE_KEEPALIVE          = 3
} UA_DataSetMessageType;

typedef struct {
    /* Settings and message fields enabled with the DataSetFlags1 */
    UA_Boolean dataSetMessageValid;

    UA_FieldEncoding fieldEncoding;

    UA_Boolean dataSetMessageSequenceNrEnabled;
    UA_UInt16 dataSetMessageSequenceNr;

    UA_Boolean statusEnabled;
    UA_UInt16 status;

    UA_Boolean configVersionMajorVersionEnabled;
    UA_UInt32 configVersionMajorVersion;

    UA_Boolean configVersionMinorVersionEnabled;
    UA_UInt32 configVersionMinorVersion;

    /* Settings and message fields enabled with the DataSetFlags2 */
    UA_DataSetMessageType dataSetMessageType;

    UA_Boolean timestampEnabled;
    UA_UtcTime timestamp;

    UA_Boolean picoSecondsIncluded;
    UA_UInt16 picoSeconds;
} UA_DataSetMessageHeader;

typedef struct {
    UA_UInt16 index;
    UA_DataValue value;
} UA_DataSetMessage_DeltaFrameField;

typedef struct {
    UA_DataSetMessageHeader header;
    UA_UInt16 fieldCount;
    union { /* Array of fields (cf. header->dataSetMessageType) */
        UA_DataValue *keyFrameFields;
        UA_DataSetMessage_DeltaFrameField *deltaFrameFields;
    } data;
} UA_DataSetMessage;

void UA_DataSetMessage_clear(UA_DataSetMessage *p);

/**
 * Network Message
 * ~~~~~~~~~~~~~~~ */

typedef enum {
    UA_NETWORKMESSAGE_DATASET = 0,
    UA_NETWORKMESSAGE_DISCOVERY_REQUEST = 1,
    UA_NETWORKMESSAGE_DISCOVERY_RESPONSE = 2
} UA_NetworkMessageType;

typedef struct {
    UA_Boolean writerGroupIdEnabled;
    UA_UInt16 writerGroupId;

    UA_Boolean groupVersionEnabled;
    UA_UInt32 groupVersion;

    UA_Boolean networkMessageNumberEnabled;
    UA_UInt16 networkMessageNumber;

    UA_Boolean sequenceNumberEnabled;
    UA_UInt16 sequenceNumber;
} UA_NetworkMessageGroupHeader;

#define UA_NETWORKMESSAGE_MAX_NONCE_LENGTH 16

typedef struct {
    UA_Boolean networkMessageSigned;

    UA_Boolean networkMessageEncrypted;

    UA_Boolean securityFooterEnabled;
    UA_UInt16 securityFooterSize;

    UA_Boolean forceKeyReset;

    UA_UInt32 securityTokenId;

    UA_UInt16 messageNonceSize;
    UA_Byte messageNonce[UA_NETWORKMESSAGE_MAX_NONCE_LENGTH];
} UA_NetworkMessageSecurityHeader;

#define UA_NETWORKMESSAGE_MAXMESSAGECOUNT 32

typedef struct {
    UA_Byte version;

    /* Fields defined via the UADPFlags */

    UA_Boolean publisherIdEnabled;
    UA_PublisherId publisherId;

    UA_Boolean groupHeaderEnabled;
    UA_NetworkMessageGroupHeader groupHeader;

    /* Fields defined via the Extended1Flags */

    UA_Boolean dataSetClassIdEnabled;
    UA_Guid dataSetClassId;

    UA_Boolean securityEnabled;
    UA_NetworkMessageSecurityHeader securityHeader;

    UA_Boolean timestampEnabled;
    UA_DateTime timestamp;

    UA_Boolean picosecondsEnabled;
    UA_UInt16 picoseconds;

    /* Fields defined via the Extended2Flags */

    UA_Boolean chunkMessage;

    UA_Boolean promotedFieldsEnabled;
    UA_UInt16 promotedFieldsSize;
    UA_Variant *promotedFields; /* BaseDataType */

    /* For Json NetworkMessage */
    UA_Boolean messageIdEnabled;
    UA_String messageId;

    /* The PayloadHeader contains the number of DataSetMessages and the
     * DataSetWriterId for each of them. If the PayloadHeader is disabled, then
     * the number of DataSetMessages is determined as follows:
     *
     * - If the UA_NetworkMessage_EncodingOptions contain metadata, they define
     *   the number and the order of the DataSetMessages.
     * - Otherwise we assume exactly one DataSetMessage which takes up all of the
     *   remaining NetworkMessage length.
     *
     * There is an upper bound for the number of DataSetMessages, so that the
     * DataSetWriterIds can be parsed as part of the headers without allocating
     * memory. */
    UA_Boolean payloadHeaderEnabled;
    UA_Byte messageCount;
    UA_UInt16 dataSetWriterIds[UA_NETWORKMESSAGE_MAXMESSAGECOUNT];
    UA_UInt16 dataSetMessageSizes[UA_NETWORKMESSAGE_MAXMESSAGECOUNT];

    /* TODO: Add support for Discovery Messages */
    UA_NetworkMessageType networkMessageType;
    union {
        /* The DataSetMessages are an array of messageCount length.
         * Can be NULL if only the headers have been decoded. */
        UA_DataSetMessage *dataSetMessages;
    } payload;

    UA_ByteString securityFooter;
} UA_NetworkMessage;

UA_EXPORT void
UA_NetworkMessage_clear(UA_NetworkMessage* p);

/**
 * NetworkMessage Encoding
 * ~~~~~~~~~~~~~~~~~~~~~~~
 * The en/decoding translates the NetworkMessage structure to/from a binary or
 * JSON encoding. The en/decoding of PubSub NetworkMessages can require
 * additional metadata. For example, during decoding, the DataType of raw
 * encoded fields must be already known. As an example for encoding, the
 * ``configuredSize`` may define zero-padding after a DataSetMessage.
 *
 * In the below methods, the different encoding options can be a NULL pointer
 * and will then be ignored. */

typedef struct {
    /* The WriterId is used to find the matching encoding metadata. If the
     * NetworkMessage/DataSetMessage does not transmit the identifier, then the
     * encoding metadata is used in-order for the received fields. */
    UA_UInt16 dataSetWriterId;

    /* FieldMetaData for JSON and RAW encoding */
    size_t fieldsSize;
    UA_FieldMetaData *fields;

    /* Zero-padding if the DataSetMessage is shorter (UADP) */
    UA_UInt16 configuredSize;
} UA_DataSetMessage_EncodingMetaData;

typedef struct {
    size_t metaDataSize;
    UA_DataSetMessage_EncodingMetaData *metaData;
} UA_NetworkMessage_EncodingOptions;

/* The output buffer is allocated to the required size if initially empty.
 * Otherwise, upon success, the length is adjusted. */
UA_EXPORT UA_StatusCode
UA_NetworkMessage_encodeBinary(const UA_NetworkMessage* src,
                               UA_ByteString *outBuf,
                               const UA_NetworkMessage_EncodingOptions *eo);

UA_EXPORT size_t
UA_NetworkMessage_calcSizeBinary(const UA_NetworkMessage *p,
                                 const UA_NetworkMessage_EncodingOptions *eo);

UA_EXPORT UA_StatusCode
UA_NetworkMessage_decodeBinary(const UA_ByteString *src,
                               UA_NetworkMessage *dst,
                               const UA_NetworkMessage_EncodingOptions *eo,
                               const UA_DecodeBinaryOptions *bo);

/* Decode only the headers before the payload */
UA_EXPORT UA_StatusCode
UA_NetworkMessage_decodeBinaryHeaders(const UA_ByteString *src,
                                      UA_NetworkMessage *dst,
                                      const UA_NetworkMessage_EncodingOptions *eo,
                                      const UA_DecodeBinaryOptions *bo,
                                      size_t *payloadOffset);

#ifdef UA_ENABLE_JSON_ENCODING

/* The output buffer is allocated to the required size if initially empty.
 * Otherwise, upon success, the length is adjusted. */
UA_EXPORT UA_StatusCode
UA_NetworkMessage_encodeJson(const UA_NetworkMessage *src,
                             UA_ByteString *outBuf,
                             const UA_NetworkMessage_EncodingOptions *eo,
                             const UA_EncodeJsonOptions *jo);

UA_EXPORT size_t
UA_NetworkMessage_calcSizeJson(const UA_NetworkMessage *src,
                               const UA_NetworkMessage_EncodingOptions *eo,
                               const UA_EncodeJsonOptions *jo);

UA_EXPORT UA_StatusCode
UA_NetworkMessage_decodeJson(const UA_ByteString *src,
                             UA_NetworkMessage *dst,
                             const UA_NetworkMessage_EncodingOptions *eo,
                             const UA_DecodeJsonOptions *jo);

#endif /* UA_ENABLE_JSON_ENCODING */
#endif /* UA_ENABLE_PUBSUB */

_UA_END_DECLS

#endif /* UA_PUBSUB_H */
