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
 * Raw PubSub
 * ==========
 *
 * The following definitions enable to work directly with PubSub messages. This
 * is not required when :ref:`PubSub is integrated with a server<pubsub>`.
 *
 * DataSet Message
 * ^^^^^^^^^^^^^^^ */

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
    UA_UInt16 fieldCount;
    UA_DataValue* dataSetFields;
    UA_ByteString rawFields;
    /* Json keys for the dataSetFields: TODO: own dataSetMessageType for json? */
    UA_String* fieldNames;
    /* This information is for proper en- and decoding needed */
    UA_DataSetMetaDataType *dataSetMetaDataType;
} UA_DataSetMessage_DataKeyFrameData;

typedef struct {
    UA_UInt16 fieldIndex;
    UA_DataValue fieldValue;
} UA_DataSetMessage_DeltaFrameField;

typedef struct {
    UA_UInt16 fieldCount;
    UA_DataSetMessage_DeltaFrameField* deltaFrameFields;
} UA_DataSetMessage_DataDeltaFrameData;

typedef struct {
    UA_UInt16 dataSetWriterId; /* Goes into the payload header */

    UA_DataSetMessageHeader header;
    union {
        UA_DataSetMessage_DataKeyFrameData keyFrameData;
        UA_DataSetMessage_DataDeltaFrameData deltaFrameData;
    } data;
    size_t configuredSize;
} UA_DataSetMessage;

/**
 * Network Message
 * ^^^^^^^^^^^^^^^ */

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

typedef struct {
    UA_Byte version;

    /* Fields defined via the UADPFlags */
    UA_Boolean publisherIdEnabled;
    UA_PublisherId publisherId;

    UA_Boolean groupHeaderEnabled;
    UA_NetworkMessageGroupHeader groupHeader;

    UA_Boolean payloadHeaderEnabled;

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

    UA_NetworkMessageType networkMessageType;

    /* For Json NetworkMessage */
    UA_Boolean messageIdEnabled;
    UA_String messageId;

    union {
        struct {
            UA_DataSetMessage *dataSetMessages;
            size_t dataSetMessagesSize; /* Goes into the payload header */
        } dataSetPayload;
        /* Extended with other payload types in the future */
    } payload;

    UA_ByteString securityFooter;
} UA_NetworkMessage;

UA_EXPORT void
UA_NetworkMessage_clear(UA_NetworkMessage* p);

/**
 * NetworkMessage Encoding
 * ^^^^^^^^^^^^^^^^^^^^^^^ */

/* The output buffer is allocated to the required size if initially empty.
 * Otherwise, upon success, the length is adjusted. */
UA_EXPORT UA_StatusCode
UA_NetworkMessage_encodeBinary(const UA_NetworkMessage* src,
                               UA_ByteString *outBuf);

UA_EXPORT size_t
UA_NetworkMessage_calcSizeBinary(const UA_NetworkMessage *p);

/* The customTypes can be NULL */
UA_EXPORT UA_StatusCode
UA_NetworkMessage_decodeBinary(const UA_ByteString *src,
                               UA_NetworkMessage* dst,
                               const UA_DecodeBinaryOptions *options);

#ifdef UA_ENABLE_JSON_ENCODING

/* The output buffer is allocated to the required size if initially empty.
 * Otherwise, upon success, the length is adjusted.
 * The encoding options can be NULL. */
UA_EXPORT UA_StatusCode
UA_NetworkMessage_encodeJson(const UA_NetworkMessage *src,
                             UA_ByteString *outBuf,
                             const UA_EncodeJsonOptions *options);

/* The encoding options can be NULL */
UA_EXPORT size_t
UA_NetworkMessage_calcSizeJson(const UA_NetworkMessage *src,
                               const UA_EncodeJsonOptions *options);

/* The encoding options can be NULL */
UA_EXPORT UA_StatusCode
UA_NetworkMessage_decodeJson(const UA_ByteString *src,
                             UA_NetworkMessage *dst,
                             const UA_DecodeJsonOptions *options);

#endif

#endif /* UA_ENABLE_PUBSUB */

_UA_END_DECLS

#endif /* UA_PUBSUB_H */
