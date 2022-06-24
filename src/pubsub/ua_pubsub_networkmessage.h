/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Tino Bischoff)
 * Copyright (c) 2017-2019 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#ifndef UA_PUBSUB_NETWORKMESSAGE_H_
#define UA_PUBSUB_NETWORKMESSAGE_H_

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/plugin/securitypolicy.h>

_UA_BEGIN_DECLS

#define UA_NETWORKMESSAGE_MAX_NONCE_LENGTH 16

/* DataSet Payload Header */
typedef struct {
    UA_Byte count;
    UA_UInt16* dataSetWriterIds;
} UA_DataSetPayloadHeader;

/* FieldEncoding Enum  */
typedef enum {
    UA_FIELDENCODING_VARIANT = 0,
    UA_FIELDENCODING_RAWDATA = 1,
    UA_FIELDENCODING_DATAVALUE = 2,
    UA_FIELDENCODING_UNKNOWN = 3
} UA_FieldEncoding;

/* DataSetMessage Type */
typedef enum {
    UA_DATASETMESSAGE_DATAKEYFRAME = 0,
    UA_DATASETMESSAGE_DATADELTAFRAME = 1,
    UA_DATASETMESSAGE_EVENT = 2,
    UA_DATASETMESSAGE_KEEPALIVE = 3
} UA_DataSetMessageType;

/* DataSetMessage Header */
typedef struct {
    UA_Boolean dataSetMessageValid;
    UA_FieldEncoding fieldEncoding;
    UA_Boolean dataSetMessageSequenceNrEnabled;
    UA_Boolean timestampEnabled;
    UA_Boolean statusEnabled;
    UA_Boolean configVersionMajorVersionEnabled;
    UA_Boolean configVersionMinorVersionEnabled;
    UA_DataSetMessageType dataSetMessageType;
    UA_Boolean picoSecondsIncluded;
    UA_UInt16 dataSetMessageSequenceNr;
    UA_UtcTime timestamp;
    UA_UInt16 picoSeconds;
    UA_UInt16 status;
    UA_UInt32 configVersionMajorVersion;
    UA_UInt32 configVersionMinorVersion;
} UA_DataSetMessageHeader;

/**
 * DataSetMessage
 * ^^^^^^^^^^^^^^ */

typedef struct {
    UA_UInt16 fieldCount;
    UA_DataValue* dataSetFields;
    UA_ByteString rawFields;
    /* Json keys for the dataSetFields: TODO: own dataSetMessageType for json? */
    UA_String* fieldNames;
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
    UA_DataSetMessageHeader header;
    union {
        UA_DataSetMessage_DataKeyFrameData keyFrameData;
        UA_DataSetMessage_DataDeltaFrameData deltaFrameData;
    } data;
} UA_DataSetMessage;

typedef struct {
    UA_UInt16* sizes;
    UA_DataSetMessage* dataSetMessages;
} UA_DataSetPayload;

typedef enum {
    UA_PUBLISHERDATATYPE_BYTE = 0,
    UA_PUBLISHERDATATYPE_UINT16 = 1,
    UA_PUBLISHERDATATYPE_UINT32 = 2,
    UA_PUBLISHERDATATYPE_UINT64 = 3,
    UA_PUBLISHERDATATYPE_STRING = 4
} UA_PublisherIdDatatype;

typedef enum {
    UA_NETWORKMESSAGE_DATASET = 0,
    UA_NETWORKMESSAGE_DISCOVERY_REQUEST = 1,
    UA_NETWORKMESSAGE_DISCOVERY_RESPONSE = 2
} UA_NetworkMessageType;

/**
 * UA_NetworkMessageGroupHeader
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
typedef struct {
    UA_Boolean writerGroupIdEnabled;
    UA_Boolean groupVersionEnabled;
    UA_Boolean networkMessageNumberEnabled;
    UA_Boolean sequenceNumberEnabled;
    UA_UInt16 writerGroupId;
    UA_UInt32 groupVersion; // spec: type "VersionTime"
    UA_UInt16 networkMessageNumber;
    UA_UInt16 sequenceNumber;
} UA_NetworkMessageGroupHeader;

/**
 * UA_NetworkMessageSecurityHeader
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
typedef struct {
    UA_Boolean networkMessageSigned;
    UA_Boolean networkMessageEncrypted;
    UA_Boolean securityFooterEnabled;
    UA_Boolean forceKeyReset;
    UA_UInt32 securityTokenId;      // spec: IntegerId
    UA_Byte messageNonce[UA_NETWORKMESSAGE_MAX_NONCE_LENGTH];
    UA_UInt16 messageNonceSize;
    UA_UInt16 securityFooterSize;
} UA_NetworkMessageSecurityHeader;

/**
 * UA_NetworkMessage
 * ^^^^^^^^^^^^^^^^^ */
typedef struct {
    UA_Byte version;
    UA_Boolean messageIdEnabled;
    UA_String messageId; /* For Json NetworkMessage */
    UA_Boolean publisherIdEnabled;
    UA_Boolean groupHeaderEnabled;
    UA_Boolean payloadHeaderEnabled;
    UA_PublisherIdDatatype publisherIdType;
    UA_Boolean dataSetClassIdEnabled;
    UA_Boolean securityEnabled;
    UA_Boolean timestampEnabled;
    UA_Boolean picosecondsEnabled;
    UA_Boolean chunkMessage;
    UA_Boolean promotedFieldsEnabled;
    UA_NetworkMessageType networkMessageType;
    union {
        UA_Byte publisherIdByte;
        UA_UInt16 publisherIdUInt16;
        UA_UInt32 publisherIdUInt32;
        UA_UInt64 publisherIdUInt64;
        UA_Guid publisherIdGuid;
        UA_String publisherIdString;
    } publisherId;
    UA_Guid dataSetClassId;

    UA_NetworkMessageGroupHeader groupHeader;

    union {
        UA_DataSetPayloadHeader dataSetPayloadHeader;
    } payloadHeader;

    UA_DateTime timestamp;
    UA_UInt16 picoseconds;
    UA_UInt16 promotedFieldsSize;
    UA_Variant* promotedFields; /* BaseDataType */

    UA_NetworkMessageSecurityHeader securityHeader;

    union {
        UA_DataSetPayload dataSetPayload;
    } payload;

    UA_ByteString securityFooter;
} UA_NetworkMessage;

/**********************************************/
/*          Network Message Offsets           */
/**********************************************/

/* Offsets for buffered messages in the PubSub fast path. */
typedef enum {
    UA_PUBSUB_OFFSETTYPE_DATASETMESSAGE_SEQUENCENUMBER,
    UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_SEQUENCENUMBER,
    UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_FIELDENCDODING,
    UA_PUBSUB_OFFSETTYPE_TIMESTAMP_PICOSECONDS,
    UA_PUBSUB_OFFSETTYPE_TIMESTAMP,     /* source pointer */
    UA_PUBSUB_OFFSETTYPE_TIMESTAMP_NOW, /* no source */
    UA_PUBSUB_OFFSETTYPE_PAYLOAD_DATAVALUE,
    UA_PUBSUB_OFFSETTYPE_PAYLOAD_VARIANT,
    UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW,
    /* For subscriber RT */
    UA_PUBSUB_OFFSETTYPE_PUBLISHERID,
    UA_PUBSUB_OFFSETTYPE_WRITERGROUPID,
    UA_PUBSUB_OFFSETTYPE_DATASETWRITERID
    /* Add more offset types as needed */
} UA_NetworkMessageOffsetType;

typedef struct {
    UA_NetworkMessageOffsetType contentType;
    union {
        struct {
            UA_DataValue *value;
            size_t valueBinarySize;
        } value;
        UA_DateTime *timestamp;
    } offsetData;
    size_t offset;
} UA_NetworkMessageOffset;

typedef struct {
    UA_ByteString buffer; /* The precomputed message buffer */
    UA_NetworkMessageOffset *offsets; /* Offsets for changes in the message buffer */
    size_t offsetsSize;
    UA_Boolean RTsubscriberEnabled; /* Addtional offsets computation like
                                     * publisherId, WGId if this bool enabled */
    UA_NetworkMessage *nm; /* The precomputed NetworkMessage for subscriber */
    size_t rawMessageLength;
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_ByteString encryptBuffer; /* The precomputed message buffer is copied
                                  * into the encrypt buffer for encryption and
                                  * signing*/
    UA_Byte *payloadPosition; /* Payload Position of the message to encrypt*/
#endif
} UA_NetworkMessageOffsetBuffer;

void UA_NetworkMessageOffsetBuffer_clear(UA_NetworkMessageOffsetBuffer *nmob);

/**
 * DataSetMessage
 * ^^^^^^^^^^^^^^ */

UA_StatusCode
UA_DataSetMessageHeader_encodeBinary(const UA_DataSetMessageHeader* src,
                                     UA_Byte **bufPos, const UA_Byte *bufEnd);

UA_StatusCode
UA_DataSetMessageHeader_decodeBinary(const UA_ByteString *src, size_t *offset,
                                     UA_DataSetMessageHeader* dst);

size_t
UA_DataSetMessageHeader_calcSizeBinary(const UA_DataSetMessageHeader* p);

UA_StatusCode
UA_DataSetMessage_encodeBinary(const UA_DataSetMessage* src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd);

UA_StatusCode
UA_DataSetMessage_decodeBinary(const UA_ByteString *src, size_t *offset,
                               UA_DataSetMessage* dst, UA_UInt16 dsmSize);

size_t
UA_DataSetMessage_calcSizeBinary(UA_DataSetMessage *p,
                                 UA_NetworkMessageOffsetBuffer *offsetBuffer,
                                 size_t currentOffset);

void UA_DataSetMessage_clear(UA_DataSetMessage* p);

/**
 * NetworkMessage
 * ^^^^^^^^^^^^^^ */

UA_StatusCode
UA_NetworkMessage_updateBufferedMessage(UA_NetworkMessageOffsetBuffer *buffer);

UA_StatusCode
UA_NetworkMessage_updateBufferedNwMessage(UA_NetworkMessageOffsetBuffer *buffer,
                                          const UA_ByteString *src, size_t *bufferPosition);


/**
 * NetworkMessage Encoding
 * ^^^^^^^^^^^^^^^^^^^^^^^ */

/* If dataToEncryptStart not-NULL, then it will be set to the start-position of
 * the payload in the buffer. */
UA_StatusCode
UA_NetworkMessage_encodeBinary(const UA_NetworkMessage* src,
                               UA_Byte **bufPos, const UA_Byte *bufEnd,
                               UA_Byte **dataToEncryptStart);

UA_StatusCode
UA_NetworkMessage_encodeHeaders(const UA_NetworkMessage* src,
                               UA_Byte **bufPos, const UA_Byte *bufEnd);

UA_StatusCode
UA_NetworkMessage_encodePayload(const UA_NetworkMessage* src,
                               UA_Byte **bufPos, const UA_Byte *bufEnd);

UA_StatusCode
UA_NetworkMessage_encodeFooters(const UA_NetworkMessage* src,
                               UA_Byte **bufPos, const UA_Byte *bufEnd);

/**
 * NetworkMessage Decoding
 * ^^^^^^^^^^^^^^^^^^^^^^^ */

UA_StatusCode
UA_NetworkMessage_decodeHeaders(const UA_ByteString *src, size_t *offset,
                                UA_NetworkMessage *dst);

UA_StatusCode
UA_NetworkMessage_decodePayload(const UA_ByteString *src, size_t *offset,
                                UA_NetworkMessage *dst);

UA_StatusCode
UA_NetworkMessage_decodeFooters(const UA_ByteString *src, size_t *offset,
                                UA_NetworkMessage *dst);

UA_StatusCode
UA_NetworkMessage_decodeBinary(const UA_ByteString *src, size_t *offset,
                               UA_NetworkMessage* dst);


UA_StatusCode
UA_NetworkMessageHeader_decodeBinary(const UA_ByteString *src, size_t *offset,
                                     UA_NetworkMessage *dst);

/* Also stores the offset if offsetBuffer != NULL */
size_t
UA_NetworkMessage_calcSizeBinary(UA_NetworkMessage *p,
                                 UA_NetworkMessageOffsetBuffer *offsetBuffer);

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION

UA_StatusCode
UA_NetworkMessage_signEncrypt(UA_NetworkMessage *nm, UA_MessageSecurityMode securityMode,
                              UA_PubSubSecurityPolicy *policy, void *policyContext,
                              UA_Byte *messageStart, UA_Byte *encryptStart,
                              UA_Byte *sigStart);
#endif

void
UA_NetworkMessage_clear(UA_NetworkMessage* p);

#ifdef UA_ENABLE_JSON_ENCODING
UA_StatusCode
UA_NetworkMessage_encodeJson(const UA_NetworkMessage *src,
                             UA_Byte **bufPos, const UA_Byte **bufEnd, UA_String *namespaces,
                             size_t namespaceSize, UA_String *serverUris,
                             size_t serverUriSize, UA_Boolean useReversible);

size_t
UA_NetworkMessage_calcSizeJson(const UA_NetworkMessage *src,
                               UA_String *namespaces, size_t namespaceSize,
                               UA_String *serverUris, size_t serverUriSize,
                               UA_Boolean useReversible);

UA_StatusCode UA_NetworkMessage_decodeJson(UA_NetworkMessage *dst, const UA_ByteString *src);
#endif

_UA_END_DECLS

#endif /* UA_PUBSUB_NETWORKMESSAGE_H_ */
