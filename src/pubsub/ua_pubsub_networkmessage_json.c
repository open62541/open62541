/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Lukas Meling)
 */

#include <open62541/types.h>
#include <open62541/server_pubsub.h>

#include "ua_pubsub_networkmessage.h"
#include "../ua_types_encoding_json.h"

/* Json keys for dsm */
const char * UA_DECODEKEY_MESSAGES = "Messages";
const char * UA_DECODEKEY_MESSAGETYPE = "MessageType";
const char * UA_DECODEKEY_MESSAGEID = "MessageId";
const char * UA_DECODEKEY_PUBLISHERID = "PublisherId";
const char * UA_DECODEKEY_DATASETCLASSID = "DataSetClassId";

/* Json keys for dsm */
const char * UA_DECODEKEY_DATASETWRITERID = "DataSetWriterId";
const char * UA_DECODEKEY_SEQUENCENUMBER = "SequenceNumber";
const char * UA_DECODEKEY_METADATAVERSION = "MetaDataVersion";
const char * UA_DECODEKEY_TIMESTAMP = "Timestamp";
const char * UA_DECODEKEY_DSM_STATUS = "Status";
const char * UA_DECODEKEY_PAYLOAD = "Payload";
const char * UA_DECODEKEY_DS_TYPE = "Type";

/* -- json encoding/decoding -- */
static UA_StatusCode writeJsonKey_UA_String(CtxJson *ctx, UA_String *in) {
    UA_STACKARRAY(char, out, in->length + 1);
    memcpy(out, in->data, in->length);
    out[in->length] = 0;
    return writeJsonKey(ctx, out);
}

static UA_StatusCode
UA_DataSetMessage_encodeJson_internal(const UA_DataSetMessage* src,
                                      CtxJson *ctx) {
    status rv = writeJsonObjStart(ctx);

    /* DataSetWriterId */
    rv |= writeJsonObjElm(ctx, UA_DECODEKEY_DATASETWRITERID,
                          &src->dataSetWriterId, &UA_TYPES[UA_TYPES_UINT16]);
    if(rv != UA_STATUSCODE_GOOD)
        return rv;

    /* TODO: Encode DataSetWriterName */

    /* TODO: Encode PublisherId (omitted if in the NetworkMessage header) */

    /* TODO: Encode WriterGroupName (omitted if in the NetworkMessage header) */

    /* DataSetMessageSequenceNr */
    if(src->header.dataSetMessageSequenceNrEnabled) {
        rv |= writeJsonObjElm(ctx, UA_DECODEKEY_SEQUENCENUMBER,
                              &src->header.dataSetMessageSequenceNr,
                              &UA_TYPES[UA_TYPES_UINT16]);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    /* MetaDataVersion */
    if(src->header.configVersionMajorVersionEnabled ||
       src->header.configVersionMinorVersionEnabled) {
        UA_ConfigurationVersionDataType cvd;
        cvd.majorVersion = src->header.configVersionMajorVersion;
        cvd.minorVersion = src->header.configVersionMinorVersion;
        rv |= writeJsonObjElm(ctx, UA_DECODEKEY_METADATAVERSION, &cvd,
                              &UA_TYPES[UA_TYPES_CONFIGURATIONVERSIONDATATYPE]);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    /* TODO: MinorVersion (omitted if the MetaDataVersion is sent) */

    /* Timestamp */
    if(src->header.timestampEnabled) {
        rv |= writeJsonObjElm(ctx, UA_DECODEKEY_TIMESTAMP, &src->header.timestamp,
                              &UA_TYPES[UA_TYPES_DATETIME]);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    /* Status */
    if(src->header.statusEnabled) {
        rv |= writeJsonObjElm(ctx, UA_DECODEKEY_DSM_STATUS,
                              &src->header.status, &UA_TYPES[UA_TYPES_UINT16]);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    /* MessageType */
    if(src->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
        UA_String s = UA_STRING("ua-keyframe");
        rv |= writeJsonObjElm(ctx, UA_DECODEKEY_MESSAGETYPE,
                              &s, &UA_TYPES[UA_TYPES_STRING]);
    } else {
        /* TODO: Support other message types */
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    rv |= writeJsonKey(ctx, UA_DECODEKEY_PAYLOAD);
    rv |= writeJsonObjStart(ctx);

    if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
        /* Variant */
        for(UA_UInt16 i = 0; i < src->data.keyFrameData.fieldCount; i++) {
            if(src->data.keyFrameData.fieldNames)
                rv |= writeJsonKey_UA_String(ctx, &src->data.keyFrameData.fieldNames[i]);
            else
                rv |= writeJsonKey(ctx, "");
            rv |= encodeJsonJumpTable[UA_DATATYPEKIND_VARIANT]
                (ctx, &src->data.keyFrameData.dataSetFields[i].value, NULL);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }
    } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
        /* DataValue */
        for(UA_UInt16 i = 0; i < src->data.keyFrameData.fieldCount; i++) {
            if(src->data.keyFrameData.fieldNames)
                rv |= writeJsonKey_UA_String(ctx, &src->data.keyFrameData.fieldNames[i]);
            else
                rv |= writeJsonKey(ctx, "");
            rv |= encodeJsonJumpTable[UA_DATATYPEKIND_DATAVALUE]
                (ctx, &src->data.keyFrameData.dataSetFields[i], NULL);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }
    } else {
        /* RawData */
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }
    rv |= writeJsonObjEnd(ctx); /* Payload */
    rv |= writeJsonObjEnd(ctx); /* DataSetMessage */
    return rv;
}

static UA_StatusCode
UA_NetworkMessage_encodeJson_internal(const UA_NetworkMessage* src, CtxJson *ctx) {
    /* currently only ua-data is supported, no discovery message implemented */
    if(src->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    status rv = writeJsonObjStart(ctx);

    /* Table 91 – JSON NetworkMessage Definition
     * MessageId | String | A globally unique identifier for the message.
     * This value is mandatory. But we don't check uniqueness in the
     * encoding layer. */
    rv |= writeJsonObjElm(ctx, UA_DECODEKEY_MESSAGEID,
                          &src->messageId, &UA_TYPES[UA_TYPES_STRING]);

    /* MessageType */
    UA_String s = UA_STRING("ua-data");
    rv |= writeJsonObjElm(ctx, UA_DECODEKEY_MESSAGETYPE,
                          &s, &UA_TYPES[UA_TYPES_STRING]);

    /* PublisherId, always encode as a JSON string */
    if(src->publisherIdEnabled) {
        UA_Byte buf[512];
        UA_ByteString bs = {512, buf};
        UA_Variant v;
        UA_PublisherId_toVariant(&src->publisherId, &v);
        rv |= UA_encodeJson(v.data, v.type, &bs, NULL);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
        rv |= writeJsonKey(ctx, UA_DECODEKEY_PUBLISHERID);
        rv |= encodeJsonJumpTable[UA_DATATYPEKIND_STRING](ctx, &bs, NULL);
    }
    if(rv != UA_STATUSCODE_GOOD)
        return rv;

    /* TODO: Encode WriterGroupName */

    /* DataSetClassId */
    if(src->dataSetClassIdEnabled) {
        rv |= writeJsonObjElm(ctx, UA_DECODEKEY_DATASETCLASSID,
                              &src->dataSetClassId, &UA_TYPES[UA_TYPES_GUID]);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    /* Payload: DataSetMessages */
    size_t count = src->payload.dataSetPayload.dataSetMessagesSize;
    if(count > 0) {
        rv |= writeJsonKey(ctx, UA_DECODEKEY_MESSAGES);
        rv |= writeJsonArrStart(ctx); /* start array */

        const UA_DataSetMessage *dataSetMessages =
            src->payload.dataSetPayload.dataSetMessages;
        for(size_t i = 0; i < count; i++) {
            rv |= writeJsonBeforeElement(ctx, true);
            rv |= UA_DataSetMessage_encodeJson_internal(&dataSetMessages[i], ctx);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
            /* comma is needed if more dsm are present */
            ctx->commaNeeded[ctx->depth] = true;
        }

        rv |= writeJsonArrEnd(ctx, NULL); /* end array */
    }

    rv |= writeJsonObjEnd(ctx);
    return rv;
}

UA_StatusCode
UA_NetworkMessage_encodeJsonInternal(const UA_NetworkMessage *src,
                                     UA_Byte **bufPos, const UA_Byte **bufEnd,
                                     UA_NamespaceMapping *namespaceMapping,
                                     UA_String *serverUris, size_t serverUriSize,
                                     UA_Boolean useReversible) {
    /* Set up the context */
    CtxJson ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.pos = *bufPos;
    ctx.end = *bufEnd;
    ctx.depth = 0;
    ctx.namespaceMapping = namespaceMapping;
    ctx.serverUris = serverUris;
    ctx.serverUrisSize = serverUriSize;
    ctx.useReversible = useReversible;
    ctx.calcOnly = false;

    status ret = UA_NetworkMessage_encodeJson_internal(src, &ctx);

    *bufPos = ctx.pos;
    *bufEnd = ctx.end;
    return ret;
}

UA_StatusCode
UA_NetworkMessage_encodeJson(const UA_NetworkMessage *src,
                             UA_ByteString *outBuf,
                             const UA_EncodeJsonOptions *options) {
    UA_Boolean alloced = (outBuf->length == 0);
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if(alloced) {
        size_t length = UA_NetworkMessage_calcSizeJson(src, options);
        if(length == 0)
            return UA_STATUSCODE_BADENCODINGERROR;
        ret = UA_ByteString_allocBuffer(outBuf, length);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
    }

    /* Set up the context */
    CtxJson ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.pos = outBuf->data;
    ctx.end = ctx.pos + outBuf->length;
    ctx.calcOnly = false;
    if(options) {
        ctx.useReversible = options->useReversible;
        ctx.namespaceMapping = options->namespaceMapping;
        ctx.serverUrisSize = options->serverUrisSize;
        ctx.serverUris = options->serverUris;
        ctx.prettyPrint = options->prettyPrint;
        ctx.unquotedKeys = options->unquotedKeys;
        ctx.stringNodeIds = options->stringNodeIds;
    }

    ret = UA_NetworkMessage_encodeJson_internal(src, &ctx);

    /* In case the buffer was supplied externally and is longer than the encoded
     * string */
    if(UA_LIKELY(ret == UA_STATUSCODE_GOOD))
        outBuf->length = (size_t)((uintptr_t)ctx.pos - (uintptr_t)outBuf->data);

    if(alloced && ret != UA_STATUSCODE_GOOD)
        UA_String_clear(outBuf);
    return ret;
}

size_t
UA_NetworkMessage_calcSizeJsonInternal(const UA_NetworkMessage *src,
                                       UA_NamespaceMapping *namespaceMapping,
                                       UA_String *serverUris, size_t serverUriSize,
                                       UA_Boolean useReversible) {
    /* Set up the context */
    CtxJson ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.pos = 0;
    ctx.end = (const UA_Byte*)(uintptr_t)SIZE_MAX;
    ctx.depth = 0;
    ctx.namespaceMapping = namespaceMapping;
    ctx.serverUris = serverUris;
    ctx.serverUrisSize = serverUriSize;
    ctx.useReversible = useReversible;
    ctx.calcOnly = true;

    status ret = UA_NetworkMessage_encodeJson_internal(src, &ctx);
    if(ret != UA_STATUSCODE_GOOD)
        return 0;
    return (size_t)ctx.pos;
}

size_t
UA_NetworkMessage_calcSizeJson(const UA_NetworkMessage *src,
                               const UA_EncodeJsonOptions *options) {
    /* Set up the context */
    CtxJson ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.end = (const UA_Byte*)(uintptr_t)SIZE_MAX;
    ctx.calcOnly = true;
    if(options) {
        ctx.useReversible = options->useReversible;
        ctx.namespaceMapping = options->namespaceMapping;
        ctx.serverUrisSize = options->serverUrisSize;
        ctx.serverUris = options->serverUris;
        ctx.prettyPrint = options->prettyPrint;
        ctx.unquotedKeys = options->unquotedKeys;
        ctx.stringNodeIds = options->stringNodeIds;
    }

    status ret = UA_NetworkMessage_encodeJson_internal(src, &ctx);
    if(ret != UA_STATUSCODE_GOOD)
        return 0;

    return (size_t)ctx.pos;
}

/* decode json */
static status
MetaDataVersion_decodeJsonInternal(ParseCtx *ctx, void* cvd, const UA_DataType *type) {
    return decodeJsonJumpTable[UA_DATATYPEKIND_STRUCTURE]
        (ctx, cvd, &UA_TYPES[UA_TYPES_CONFIGURATIONVERSIONDATATYPE]);
}

static status
DataSetPayload_decodeJsonInternal(ParseCtx *ctx, void* dsmP, const UA_DataType *type) {
    UA_DataSetMessage* dsm = (UA_DataSetMessage*)dsmP;
    dsm->header.dataSetMessageValid = true;
    if(currentTokenType(ctx) == CJ5_TOKEN_NULL) {
        ctx->index++;
        return UA_STATUSCODE_GOOD;
    }

    if(currentTokenType(ctx) != CJ5_TOKEN_OBJECT)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* The number of key-value pairs */
    UA_assert(ctx->tokens[ctx->index].size % 2 == 0);
    size_t length = (size_t)(ctx->tokens[ctx->index].size) / 2;

    UA_String *fieldNames = (UA_String*)UA_calloc(length, sizeof(UA_String));
    if(!fieldNames)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    dsm->data.keyFrameData.fieldNames = fieldNames;
    dsm->data.keyFrameData.fieldCount = (UA_UInt16)length;

    dsm->data.keyFrameData.dataSetFields = (UA_DataValue *)
        UA_Array_new(dsm->data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);
    if(!dsm->data.keyFrameData.dataSetFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    ctx->index++; /* Go to the first key */

    /* Iterate over the key/value pairs in the object. Keys are stored in fieldnames. */
    status ret = UA_STATUSCODE_GOOD;
    dsm->header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    for(size_t i = 0; i < length; ++i) {
        UA_assert(currentTokenType(ctx) == CJ5_TOKEN_STRING);
        ret = decodeJsonJumpTable[UA_DATATYPEKIND_STRING](ctx, &fieldNames[i], type);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;

        /* TODO: Is field value a variant or datavalue? */
        ret = decodeJsonJumpTable[UA_DATATYPEKIND_DATAVALUE]
            (ctx, &dsm->data.keyFrameData.dataSetFields[i], NULL);

        if(ret != UA_STATUSCODE_GOOD)
            return ret;
    }

    return ret;
}

static status
DatasetMessage_Payload_decodeJsonInternal(ParseCtx *ctx, UA_DataSetMessage* dsm,
                                          const UA_DataType *type) {
    UA_ConfigurationVersionDataType cvd;
    DecodeEntry entries[7] = {
        {UA_DECODEKEY_DATASETWRITERID, &dsm->dataSetWriterId, NULL, false, &UA_TYPES[UA_TYPES_UINT16]},
        {UA_DECODEKEY_SEQUENCENUMBER, &dsm->header.dataSetMessageSequenceNr, NULL, false, &UA_TYPES[UA_TYPES_UINT16]},
        {UA_DECODEKEY_METADATAVERSION, &cvd, &MetaDataVersion_decodeJsonInternal, false, NULL},
        {UA_DECODEKEY_TIMESTAMP, &dsm->header.timestamp, NULL, false, &UA_TYPES[UA_TYPES_DATETIME]},
        {UA_DECODEKEY_DSM_STATUS, &dsm->header.status, NULL, false, &UA_TYPES[UA_TYPES_UINT16]},
        {UA_DECODEKEY_MESSAGETYPE, NULL, NULL, false, NULL},
        {UA_DECODEKEY_PAYLOAD, dsm, &DataSetPayload_decodeJsonInternal, false, NULL}
    };
    status ret = decodeFields(ctx, entries, 7);

    /* Error or no DatasetWriterId found or no payload found */
    if(ret != UA_STATUSCODE_GOOD || !entries[0].found || !entries[6].found)
        return UA_STATUSCODE_BADDECODINGERROR;

    dsm->header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    dsm->header.dataSetMessageSequenceNrEnabled = entries[1].found;
    dsm->header.configVersionMajorVersion = cvd.majorVersion;
    dsm->header.configVersionMinorVersion = cvd.minorVersion;
    dsm->header.configVersionMajorVersionEnabled = entries[2].found;
    dsm->header.configVersionMinorVersionEnabled = entries[2].found;
    dsm->header.timestampEnabled = entries[3].found;
    dsm->header.statusEnabled = entries[4].found;

    dsm->header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dsm->header.picoSecondsIncluded = false;
    dsm->header.dataSetMessageValid = true;
    dsm->header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    return UA_STATUSCODE_GOOD;
}

static status
DatasetMessage_Array_decodeJsonInternal(ParseCtx *ctx, void *UA_RESTRICT dst,
                                        const UA_DataType *type) {
    /* Array or object */
    size_t length = 1;
    if(currentTokenType(ctx) == CJ5_TOKEN_ARRAY) {
        length = (size_t)ctx->tokens[ctx->index].size;

        /* Go to the first array member */
        ctx->index++;

        /* Return early for empty arrays */
        if(length == 0)
            return UA_STATUSCODE_GOOD;
    } else if(currentTokenType(ctx) != CJ5_TOKEN_OBJECT) {
        return UA_STATUSCODE_BADDECODINGERROR;
    }

    /* Decode array members */
    UA_DataSetMessage *dsm = (UA_DataSetMessage*)dst;
    for(size_t i = 0; i < length; ++i) {
        status ret = DatasetMessage_Payload_decodeJsonInternal(ctx, &dsm[i], NULL);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
    }

    return UA_STATUSCODE_GOOD;
}

static status
decodePublisherIdJsonInternal(ParseCtx *ctx, void *UA_RESTRICT dst,
                              const UA_DataType *type) {
    UA_PublisherId *p = (UA_PublisherId*)dst;
    if(currentTokenType(ctx) == CJ5_TOKEN_NUMBER) {
        /* Store in biggest possible integer. The problem is that with a UInt64
         * is that a string is expected for it in JSON. Therefore, the maximum
         * value is set to UInt32. */
        p->idType = UA_PUBLISHERIDTYPE_UINT32;
        return decodeJsonJumpTable[UA_DATATYPEKIND_UINT32](ctx, &p->id.uint32, NULL);
    } else if(currentTokenType(ctx) == CJ5_TOKEN_STRING) {
        p->idType = UA_PUBLISHERIDTYPE_STRING;
        return decodeJsonJumpTable[UA_DATATYPEKIND_STRING](ctx, &p->id.string, NULL);
    }
    return UA_STATUSCODE_BADDECODINGERROR;
}

static status
NetworkMessage_decodeJsonInternal(ParseCtx *ctx, UA_NetworkMessage *dst) {
    memset(dst, 0, sizeof(UA_NetworkMessage));
    dst->chunkMessage = false;
    dst->groupHeaderEnabled = false;
    dst->payloadHeaderEnabled = false;
    dst->picosecondsEnabled = false;
    dst->promotedFieldsEnabled = false;

    /* Is Messages an Array? How big? */
    size_t searchResultMessages = 0;
    status found = lookAheadForKey(ctx, UA_DECODEKEY_MESSAGES, &searchResultMessages);
    if(found != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    const cj5_token *bodyToken = &ctx->tokens[searchResultMessages];
    size_t messageCount = 1;
    if(bodyToken->type == CJ5_TOKEN_ARRAY)
        messageCount = (size_t)bodyToken->size;

    /* MessageType */
    UA_Boolean isUaData = true;
    size_t searchResultMessageType = 0;
    found = lookAheadForKey(ctx, UA_DECODEKEY_MESSAGETYPE, &searchResultMessageType);
    if(found != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;
    size_t size = getTokenLength(&ctx->tokens[searchResultMessageType]);
    const char* msgType = &ctx->json5[ctx->tokens[searchResultMessageType].start];
    if(size == 7) { //ua-data
        if(strncmp(msgType, "ua-data", size) != 0)
            return UA_STATUSCODE_BADDECODINGERROR;
        isUaData = true;
    } else if(size == 11) { //ua-metadata
        if(strncmp(msgType, "ua-metadata", size) != 0)
            return UA_STATUSCODE_BADDECODINGERROR;
        isUaData = false;
    } else {
        return UA_STATUSCODE_BADDECODINGERROR;
    }

    //TODO: MetaData
    if(!isUaData)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    dst->payload.dataSetPayload.dataSetMessages = (UA_DataSetMessage*)
        UA_calloc(messageCount, sizeof(UA_DataSetMessage));
    if(!dst->payload.dataSetPayload.dataSetMessages)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    dst->payload.dataSetPayload.dataSetMessagesSize = messageCount;

    /* Network Message */
    UA_String messageType;
    DecodeEntry entries[5] = {
        {UA_DECODEKEY_MESSAGEID, &dst->messageId, NULL, false, &UA_TYPES[UA_TYPES_STRING]},
        {UA_DECODEKEY_MESSAGETYPE, &messageType, NULL, false, NULL},
        {UA_DECODEKEY_PUBLISHERID, &dst->publisherId, decodePublisherIdJsonInternal, false, NULL},
        {UA_DECODEKEY_DATASETCLASSID, &dst->dataSetClassId, NULL, false, &UA_TYPES[UA_TYPES_GUID]},
        {UA_DECODEKEY_MESSAGES, dst->payload.dataSetPayload.dataSetMessages,
         &DatasetMessage_Array_decodeJsonInternal, false, NULL}
    };

    status ret = decodeFields(ctx, entries, 5);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    dst->messageIdEnabled = entries[0].found;
    dst->publisherIdEnabled = entries[2].found;
    dst->dataSetClassIdEnabled = entries[3].found;
    dst->payloadHeaderEnabled = true;

    return ret;
}

UA_StatusCode
UA_NetworkMessage_decodeJson(const UA_ByteString *src,
                             UA_NetworkMessage *dst,
                             const UA_DecodeJsonOptions *options) {
    /* Set up the context */
    cj5_token tokens[UA_JSON_MAXTOKENCOUNT];
    ParseCtx ctx;
    memset(&ctx, 0, sizeof(ParseCtx));
    ctx.tokens = tokens;
    if(options) {
        ctx.namespaceMapping = options->namespaceMapping;
        ctx.serverUrisSize = options->serverUrisSize;
        ctx.serverUris = options->serverUris;
        ctx.customTypes = options->customTypes;
    }

    status ret = tokenize(&ctx, src, UA_JSON_MAXTOKENCOUNT, NULL);
    if(ret != UA_STATUSCODE_GOOD)
        goto cleanup;

    ret = NetworkMessage_decodeJsonInternal(&ctx, dst);
    if(ret != UA_STATUSCODE_GOOD)
        UA_NetworkMessage_clear(dst);

 cleanup:
    /* Free token array on the heap */
    if(ctx.tokens != tokens)
        UA_free((void*)(uintptr_t)ctx.tokens);
    return ret;
}
