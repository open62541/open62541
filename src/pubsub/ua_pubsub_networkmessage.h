/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Tino Bischoff)
 * Copyright (c) 2017-2019 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2025 Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_PUBSUB_NETWORKMESSAGE_H_
#define UA_PUBSUB_NETWORKMESSAGE_H_

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/pubsub.h>

#include "../ua_types_encoding_binary.h"
#include "../ua_types_encoding_json.h"

#ifdef UA_ENABLE_PUBSUB

_UA_BEGIN_DECLS

typedef struct {
    Ctx ctx;
    UA_NetworkMessage_EncodingOptions eo;
    UA_PubSubOffsetTable *ot;
} PubSubEncodeCtx;

typedef struct {
    Ctx ctx;
    UA_NetworkMessage_EncodingOptions eo;
} PubSubDecodeCtx;

typedef struct {
    CtxJson ctx;
    UA_NetworkMessage_EncodingOptions eo;
} PubSubEncodeJsonCtx;

typedef struct {
    ParseCtx ctx;
    UA_NetworkMessage_EncodingOptions eo;
} PubSubDecodeJsonCtx;

const UA_FieldMetaData *
getFieldMetaData(const UA_DataSetMessage_EncodingMetaData *emd,
                 size_t index);

const UA_DataSetMessage_EncodingMetaData *
findEncodingMetaData(const UA_NetworkMessage_EncodingOptions *eo,
                     const UA_DataSetMessage *dsm);

/******************/
/* DataSetMessage */
/******************/

UA_StatusCode
UA_DataSetMessageHeader_encodeBinary(PubSubEncodeCtx *ctx,
                                     const UA_DataSetMessageHeader *src);

UA_StatusCode
UA_DataSetMessageHeader_decodeBinary(PubSubDecodeCtx *ctx,
                                     UA_DataSetMessageHeader *dst);

UA_StatusCode
UA_DataSetMessage_encodeBinary(PubSubEncodeCtx *ctx,
                               const UA_DataSetMessage_EncodingMetaData *emd,
                               const UA_DataSetMessage *src);

UA_StatusCode
UA_DataSetMessage_decodeBinary(PubSubDecodeCtx *ctx,
                               UA_DataSetMessage *dsm,
                               size_t dsmSize);

size_t
UA_DataSetMessage_calcSizeBinary(PubSubEncodeCtx *ctx,
                                 const UA_DataSetMessage_EncodingMetaData *em,
                                 UA_DataSetMessage *src,
                                 size_t currentOffset);

/******************/
/* NetworkMessage */
/******************/

size_t
UA_NetworkMessage_calcSizeBinaryInternal(PubSubEncodeCtx *ctx,
                                         const UA_NetworkMessage *src);

UA_StatusCode
UA_NetworkMessage_encodeHeaders(PubSubEncodeCtx *ctx,
                                const UA_NetworkMessage *src);

UA_StatusCode
UA_NetworkMessage_encodePayload(PubSubEncodeCtx *ctx,
                                const UA_NetworkMessage *src);

UA_StatusCode
UA_NetworkMessage_encodeFooters(PubSubEncodeCtx *ctx,
                                const UA_NetworkMessage *src);

UA_StatusCode
UA_NetworkMessage_decodeHeaders(PubSubDecodeCtx *ctx,
                                UA_NetworkMessage *dst);

UA_StatusCode
UA_NetworkMessage_decodePayload(PubSubDecodeCtx *ctx,
                                UA_NetworkMessage *dst);

UA_StatusCode
UA_NetworkMessage_decodeFooters(PubSubDecodeCtx *ctx,
                                UA_NetworkMessage *dst);

UA_StatusCode
UA_NetworkMessage_encodeBinaryWithEncryptStart(PubSubEncodeCtx *ctx,
                                               const UA_NetworkMessage *src,
                                               UA_Byte **dataToEncryptStart);

UA_StatusCode
UA_NetworkMessage_signEncrypt(UA_NetworkMessage *nm,
                              UA_MessageSecurityMode securityMode,
                              UA_PubSubSecurityPolicy *policy,
                              void *policyContext,
                              UA_Byte *messageStart,
                              UA_Byte *encryptStart,
                              UA_Byte *sigStart);

UA_StatusCode
UA_NetworkMessage_encodeJsonInternal(PubSubEncodeJsonCtx *ctx,
                                     const UA_NetworkMessage *src);

_UA_END_DECLS

#endif /* UA_ENABLE_PUBSUB */

#endif /* UA_PUBSUB_NETWORKMESSAGE_H_ */
