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
#include <open62541/pubsub.h>
#include <open62541/server_pubsub.h>

#ifdef UA_ENABLE_PUBSUB

_UA_BEGIN_DECLS

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
		UA_UInt16 sequenceNumber;
		UA_DataValue value;
	} content;
	size_t offset;
} UA_NetworkMessageOffset;

typedef struct {
	UA_ByteString buffer; /* The precomputed message buffer */
	UA_NetworkMessageOffset *offsets; /* Offsets for changes in the message buffer */
	size_t offsetsSize;
	UA_NetworkMessage *nm; /* The precomputed NetworkMessage for subscriber */
	size_t rawMessageLength;
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
	UA_ByteString encryptBuffer; /* The precomputed message buffer is copied
								 * into the encrypt buffer for encryption and
								 * signing*/
	UA_Byte *payloadPosition; /* Payload Position of the message to encrypt*/
#endif
} UA_NetworkMessageOffsetBuffer;

void
UA_NetworkMessageOffsetBuffer_clear(UA_NetworkMessageOffsetBuffer *nmob);

UA_StatusCode
UA_NetworkMessage_updateBufferedMessage(UA_NetworkMessageOffsetBuffer *buffer);

UA_StatusCode
UA_NetworkMessage_updateBufferedNwMessage(UA_NetworkMessageOffsetBuffer *buffer,
                                          const UA_ByteString *src, size_t *bufferPosition);

size_t
UA_NetworkMessage_calcSizeBinaryWithOffsetBuffer(
    const UA_NetworkMessage *p, UA_NetworkMessageOffsetBuffer *offsetBuffer);

/**
 * DataSetMessage
 * ^^^^^^^^^^^^^^ */

UA_StatusCode
UA_DataSetMessageHeader_encodeBinary(const UA_DataSetMessageHeader* src,
                                     UA_Byte **bufPos, const UA_Byte *bufEnd);

UA_StatusCode
UA_DataSetMessageHeader_decodeBinary(const UA_ByteString *src, size_t *offset,
                                     UA_DataSetMessageHeader* dst);

UA_StatusCode
UA_DataSetMessage_encodeBinary(const UA_DataSetMessage* src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd);

UA_StatusCode
UA_DataSetMessage_decodeBinary(const UA_ByteString *src, size_t *offset,
                               UA_DataSetMessage* dst, UA_UInt16 dsmSize,
                               const UA_DataTypeArray *customTypes,
                               UA_DataSetMetaDataType *dsm);

size_t
UA_DataSetMessage_calcSizeBinary(UA_DataSetMessage *p,
                                 UA_NetworkMessageOffsetBuffer *offsetBuffer,
                                 size_t currentOffset);

void UA_DataSetMessage_clear(UA_DataSetMessage* p);

/**
 * NetworkMessage Encoding
 * ^^^^^^^^^^^^^^^^^^^^^^^ */

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
                                UA_NetworkMessage *dst, const UA_DataTypeArray *customTypes,
                                UA_DataSetMetaDataType *dsm);

UA_StatusCode
UA_NetworkMessage_decodeFooters(const UA_ByteString *src, size_t *offset,
                                UA_NetworkMessage *dst);
                          
UA_StatusCode
UA_NetworkMessageHeader_decodeBinary(const UA_ByteString *src, size_t *offset,
                                     UA_NetworkMessage *dst);

UA_StatusCode
UA_NetworkMessage_encodeJsonInternal(const UA_NetworkMessage *src,
                                     UA_Byte **bufPos, const UA_Byte **bufEnd,
                                     UA_String *namespaces, size_t namespaceSize,
                                     UA_String *serverUris, size_t serverUriSize,
                                     UA_Boolean useReversible);

size_t
UA_NetworkMessage_calcSizeJsonInternal(const UA_NetworkMessage *src,
                                       UA_String *namespaces, size_t namespaceSize,
                                       UA_String *serverUris, size_t serverUriSize,
                                       UA_Boolean useReversible);

UA_StatusCode
UA_NetworkMessage_encodeBinaryWithEncryptStart(const UA_NetworkMessage* src,
                                               UA_Byte **bufPos, const UA_Byte *bufEnd,
                                               UA_Byte **dataToEncryptStart);

UA_StatusCode
UA_NetworkMessage_decodeBinaryWithOffset(const UA_ByteString *src, size_t *offset,
                                         UA_NetworkMessage* dst,
                                         const UA_DataTypeArray *customTypes);

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION

UA_StatusCode
UA_NetworkMessage_signEncrypt(UA_NetworkMessage *nm, UA_MessageSecurityMode securityMode,
                              UA_PubSubSecurityPolicy *policy, void *policyContext,
                              UA_Byte *messageStart, UA_Byte *encryptStart,
                              UA_Byte *sigStart);
#endif

_UA_END_DECLS

#endif /* UA_ENABLE_PUBSUB */

#endif /* UA_PUBSUB_NETWORKMESSAGE_H_ */
