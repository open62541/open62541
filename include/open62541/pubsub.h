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
 * .. _pubsub-messages:
 *
 * PubSub Network Messages
 * =======================
 *
 * The following definitions enable to work directly with PubSub messages. This
 * is not required when :ref:`PubSub is integrated with a server<pubsub>`. */

#define UA_NETWORKMESSAGE_MAX_NONCE_LENGTH 16

/**
* DataSet Message
* ^^^^^^^^^^^^^^^ */

typedef struct {
	UA_Byte count;
	UA_UInt16* dataSetWriterIds;
} UA_DataSetPayloadHeader;

typedef enum {
	UA_FIELDENCODING_VARIANT   = 0,
	UA_FIELDENCODING_RAWDATA   = 1,
	UA_FIELDENCODING_DATAVALUE = 2,
	UA_FIELDENCODING_UNKNOWN   = 3
} UA_FieldEncoding;

typedef enum {
	UA_DATASETMESSAGE_DATAKEYFRAME   = 0,
	UA_DATASETMESSAGE_DATADELTAFRAME = 1,
	UA_DATASETMESSAGE_EVENT          = 2,
	UA_DATASETMESSAGE_KEEPALIVE      = 3
} UA_DataSetMessageType;

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
	UA_UInt16* sizes;
	UA_DataSetMessage* dataSetMessages;
} UA_DataSetPayload;

typedef struct {
	UA_Boolean writerGroupIdEnabled;
	UA_Boolean groupVersionEnabled;
	UA_Boolean networkMessageNumberEnabled;
	UA_Boolean sequenceNumberEnabled;
	UA_UInt16 writerGroupId;
	UA_UInt32 groupVersion;
	UA_UInt16 networkMessageNumber;
	UA_UInt16 sequenceNumber;
} UA_NetworkMessageGroupHeader;

typedef struct {
	UA_Boolean networkMessageSigned;
	UA_Boolean networkMessageEncrypted;
	UA_Boolean securityFooterEnabled;
	UA_Boolean forceKeyReset;
	UA_UInt32 securityTokenId;
	UA_Byte messageNonce[UA_NETWORKMESSAGE_MAX_NONCE_LENGTH];
	UA_UInt16 messageNonceSize;
	UA_UInt16 securityFooterSize;
} UA_NetworkMessageSecurityHeader;

typedef struct {
	UA_Byte version;
	UA_Boolean messageIdEnabled;
	UA_String messageId; /* For Json NetworkMessage */
	UA_Boolean publisherIdEnabled;
	UA_Boolean groupHeaderEnabled;
	UA_Boolean payloadHeaderEnabled;
	UA_Boolean dataSetClassIdEnabled;
	UA_Boolean securityEnabled;
	UA_Boolean timestampEnabled;
	UA_Boolean picosecondsEnabled;
	UA_Boolean chunkMessage;
	UA_Boolean promotedFieldsEnabled;
	UA_NetworkMessageType networkMessageType;
	UA_PublisherIdType publisherIdType;
	UA_PublisherId publisherId;
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

/**
 * NetworkMessage Encoding
 * ^^^^^^^^^^^^^^^^^^^^^^^ */

/* If dataToEncryptStart not-NULL, then it will be set to the start-position of
 * the payload in the buffer. */
UA_EXPORT UA_StatusCode
UA_NetworkMessage_encodeBinary(const UA_NetworkMessage* src,
                               UA_Byte **bufPos, const UA_Byte *bufEnd,
                               UA_Byte **dataToEncryptStart);

UA_EXPORT UA_StatusCode
UA_NetworkMessage_decodeBinary(const UA_ByteString *src, size_t *offset,
                               UA_NetworkMessage* dst,
                               const UA_DataTypeArray *customTypes);

UA_EXPORT size_t
UA_NetworkMessage_calcSizeBinary(const UA_NetworkMessage *p);

UA_EXPORT void
UA_NetworkMessage_clear(UA_NetworkMessage* p);

#ifdef UA_ENABLE_JSON_ENCODING

UA_EXPORT UA_StatusCode
UA_NetworkMessage_encodeJson(const UA_NetworkMessage *src,
                             UA_Byte **bufPos, const UA_Byte **bufEnd,
                             UA_String *namespaces, size_t namespaceSize,
                             UA_String *serverUris, size_t serverUriSize,
                             UA_Boolean useReversible);

UA_EXPORT size_t
UA_NetworkMessage_calcSizeJson(const UA_NetworkMessage *src,
                               UA_String *namespaces, size_t namespaceSize,
                               UA_String *serverUris, size_t serverUriSize,
                               UA_Boolean useReversible);

UA_EXPORT UA_StatusCode
UA_NetworkMessage_decodeJson(UA_NetworkMessage *dst,
                             const UA_ByteString *src);

#endif

#endif /* UA_ENABLE_PUBSUB */

_UA_END_DECLS

#endif /* UA_PUBSUB_H */
