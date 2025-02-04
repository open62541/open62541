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

#include "../ua_types_encoding_binary.h"

#ifdef UA_ENABLE_PUBSUB

_UA_BEGIN_DECLS

/**
 * DataSetMessage
 * ^^^^^^^^^^^^^^ */

UA_StatusCode
UA_DataSetMessageHeader_encodeBinary(const UA_DataSetMessageHeader *src,
                                     UA_Byte **bufPos, const UA_Byte *bufEnd);

UA_StatusCode
UA_DataSetMessageHeader_decodeBinary(Ctx *ctx, UA_DataSetMessageHeader *dst);

UA_StatusCode
UA_DataSetMessage_encodeBinary(const UA_DataSetMessage *src, UA_Byte **bufPos,
                               const UA_Byte *bufEnd);

UA_StatusCode
UA_DataSetMessage_decodeBinary(Ctx *ctx, UA_DataSetMessage *dst, UA_UInt16 dsmSize);

size_t
UA_DataSetMessage_calcSizeBinary(UA_DataSetMessage *p, UA_PubSubOffsetTable *ot,
                                 size_t currentOffset);

void UA_DataSetMessage_clear(UA_DataSetMessage *p);

/**
 * NetworkMessage Encoding
 * ^^^^^^^^^^^^^^^^^^^^^^^ */

size_t
UA_NetworkMessage_calcSizeBinaryWithOffsetTable(const UA_NetworkMessage *p,
                                                UA_PubSubOffsetTable *ot);

UA_StatusCode
UA_NetworkMessage_encodeHeaders(const UA_NetworkMessage *src,
                               UA_Byte **bufPos, const UA_Byte *bufEnd);

UA_StatusCode
UA_NetworkMessage_encodePayload(const UA_NetworkMessage *src,
                               UA_Byte **bufPos, const UA_Byte *bufEnd);

UA_StatusCode
UA_NetworkMessage_encodeFooters(const UA_NetworkMessage *src,
                               UA_Byte **bufPos, const UA_Byte *bufEnd);

/**
 * NetworkMessage Decoding
 * ^^^^^^^^^^^^^^^^^^^^^^^ */

UA_StatusCode
UA_NetworkMessage_decodeHeaders(Ctx *ctx, UA_NetworkMessage *dst);

UA_StatusCode
UA_NetworkMessage_decodePayload(Ctx *ctx, UA_NetworkMessage *dst);

UA_StatusCode
UA_NetworkMessage_decodeFooters(Ctx *ctx, UA_NetworkMessage *dst);
                          
UA_StatusCode
UA_NetworkMessage_encodeJsonInternal(const UA_NetworkMessage *src,
                                     UA_Byte **bufPos, const UA_Byte **bufEnd,
                                     UA_NamespaceMapping *namespaceMapping,
                                     UA_String *serverUris, size_t serverUriSize,
                                     UA_Boolean useReversible);

size_t
UA_NetworkMessage_calcSizeJsonInternal(const UA_NetworkMessage *src,
                                       UA_NamespaceMapping *namespaceMapping,
                                       UA_String *serverUris, size_t serverUriSize,
                                       UA_Boolean useReversible);

UA_StatusCode
UA_NetworkMessage_encodeBinaryWithEncryptStart(const UA_NetworkMessage* src,
                                               UA_Byte **bufPos, const UA_Byte *bufEnd,
                                               UA_Byte **dataToEncryptStart);

UA_StatusCode
UA_NetworkMessage_signEncrypt(UA_NetworkMessage *nm, UA_MessageSecurityMode securityMode,
                              UA_PubSubSecurityPolicy *policy, void *policyContext,
                              UA_Byte *messageStart, UA_Byte *encryptStart,
                              UA_Byte *sigStart);

_UA_END_DECLS

#endif /* UA_ENABLE_PUBSUB */

#endif /* UA_PUBSUB_NETWORKMESSAGE_H_ */
