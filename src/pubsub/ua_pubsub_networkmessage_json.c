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
static UA_StatusCode writeJsonKey_UA_String(CtxJson *ctx, const UA_String *in) {
    UA_STACKARRAY(char, out, in->length + 1);
    memcpy(out, in->data, in->length);
    out[in->length] = 0;
    return writeJsonKey(ctx, out);
}

static UA_StatusCode
UA_DataSetMessage_encodeJson_internal(CtxJson *ctx,
                                      const UA_DataSetMessage_EncodingMetaData *emd,
                                      const UA_DataSetMessage *src) {
    status rv = writeJsonObjStart(ctx);

    /* DataSetWriterId */
    rv |= writeJsonObjElm(ctx, UA_DECODEKEY_DATASETWRITERID,
                          &emd->dataSetWriterId, &UA_TYPES[UA_TYPES_UINT16]);
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
        for(UA_UInt16 i = 0; i < src->fieldCount; i++) {
            const UA_FieldMetaData *fmd = getFieldMetaData(emd, i);
            if(fmd)
                rv |= writeJsonKey_UA_String(ctx, &fmd->name);
            else
                rv |= writeJsonKey(ctx, "");
            rv |= encodeJsonJumpTable[UA_DATATYPEKIND_VARIANT]
                (ctx, &src->data.keyFrameFields[i].value, NULL);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }
    } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
        /* DataValue */
        for(UA_UInt16 i = 0; i < src->fieldCount; i++) {
            const UA_FieldMetaData *fmd = getFieldMetaData(emd, i);
            if(fmd)
                rv |= writeJsonKey_UA_String(ctx, &fmd->name);
            else
                rv |= writeJsonKey(ctx, "");
            rv |= encodeJsonJumpTable[UA_DATATYPEKIND_DATAVALUE]
                (ctx, &src->data.keyFrameFields[i], NULL);
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

UA_StatusCode
UA_NetworkMessage_encodeJsonInternal(PubSubEncodeJsonCtx *ctx,
                                     const UA_NetworkMessage *src) {
    /* currently only ua-data is supported, no discovery message implemented */
    if(src->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    status rv = writeJsonObjStart(&ctx->ctx);

    /* Table 91 – JSON NetworkMessage Definition
     * MessageId | String | A globally unique identifier for the message.
     * This value is mandatory. But we don't check uniqueness in the
     * encoding layer. */
    rv |= writeJsonObjElm(&ctx->ctx, UA_DECODEKEY_MESSAGEID,
                          &src->messageId, &UA_TYPES[UA_TYPES_STRING]);

    /* MessageType */
    UA_String s = UA_STRING("ua-data");
    rv |= writeJsonObjElm(&ctx->ctx, UA_DECODEKEY_MESSAGETYPE,
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
        rv |= writeJsonKey(&ctx->ctx, UA_DECODEKEY_PUBLISHERID);
        rv |= encodeJsonJumpTable[UA_DATATYPEKIND_STRING](&ctx->ctx, &bs, NULL);
    }
    if(rv != UA_STATUSCODE_GOOD)
        return rv;

    /* TODO: Encode WriterGroupName */

    /* DataSetClassId */
    if(src->dataSetClassIdEnabled) {
        rv |= writeJsonObjElm(&ctx->ctx, UA_DECODEKEY_DATASETCLASSID,
                              &src->dataSetClassId, &UA_TYPES[UA_TYPES_GUID]);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    /* Payload: DataSetMessages */
    size_t count = src->messageCount;
    if(count > 0) {
        rv |= writeJsonKey(&ctx->ctx, UA_DECODEKEY_MESSAGES);
        rv |= writeJsonArrStart(&ctx->ctx); /* start array */
        const UA_DataSetMessage *dsm = src->payload.dataSetMessages;
        for(size_t i = 0; i < count; i++) {
            const UA_DataSetMessage_EncodingMetaData *emd =
                findEncodingMetaData(&ctx->eo, src->dataSetWriterIds[i]);
            rv |= writeJsonBeforeElement(&ctx->ctx, true);
            rv |= UA_DataSetMessage_encodeJson_internal(&ctx->ctx, emd, &dsm[i]);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
            /* comma is needed if more dsm are present */
            ctx->ctx.commaNeeded[ctx->ctx.depth] = true;
        }

        rv |= writeJsonArrEnd(&ctx->ctx, NULL); /* end array */
    }

    rv |= writeJsonObjEnd(&ctx->ctx);
    return rv;
}

UA_StatusCode
UA_NetworkMessage_encodeJson(const UA_NetworkMessage *src,
                             UA_ByteString *outBuf,
                             const UA_NetworkMessage_EncodingOptions *eo,
                             const UA_EncodeJsonOptions *jo) {
    UA_Boolean alloced = (outBuf->length == 0);
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if(alloced) {
        size_t length = UA_NetworkMessage_calcSizeJson(src, eo, jo);
        if(length == 0)
            return UA_STATUSCODE_BADENCODINGERROR;
        ret = UA_ByteString_allocBuffer(outBuf, length);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
    }

    /* Set up the context */
    PubSubEncodeJsonCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.ctx.pos = outBuf->data;
    ctx.ctx.end = outBuf->data + outBuf->length;
    ctx.ctx.calcOnly = false;
    if(eo)
        ctx.eo = *eo;
    if(jo) {
        ctx.ctx.useReversible = jo->useReversible;
        ctx.ctx.namespaceMapping = jo->namespaceMapping;
        ctx.ctx.serverUrisSize = jo->serverUrisSize;
        ctx.ctx.serverUris = jo->serverUris;
        ctx.ctx.prettyPrint = jo->prettyPrint;
        ctx.ctx.unquotedKeys = jo->unquotedKeys;
        ctx.ctx.stringNodeIds = jo->stringNodeIds;
    }

    ret = UA_NetworkMessage_encodeJsonInternal(&ctx, src);

    /* In case the buffer was supplied externally and is longer than the encoded
     * string */
    if(UA_LIKELY(ret == UA_STATUSCODE_GOOD))
        outBuf->length = (size_t)((uintptr_t)ctx.ctx.pos - (uintptr_t)outBuf->data);

    if(alloced && ret != UA_STATUSCODE_GOOD)
        UA_String_clear(outBuf);
    return ret;
}

size_t
UA_NetworkMessage_calcSizeJson(const UA_NetworkMessage *src,
                               const UA_NetworkMessage_EncodingOptions *eo,
                               const UA_EncodeJsonOptions *jo) {
    /* Set up the context */
    PubSubEncodeJsonCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.ctx.end = (const UA_Byte*)(uintptr_t)SIZE_MAX;
    ctx.ctx.calcOnly = true;
    if(eo)
        ctx.eo = *eo;
    if(jo) {
        ctx.ctx.useReversible = jo->useReversible;
        ctx.ctx.namespaceMapping = jo->namespaceMapping;
        ctx.ctx.serverUrisSize = jo->serverUrisSize;
        ctx.ctx.serverUris = jo->serverUris;
        ctx.ctx.prettyPrint = jo->prettyPrint;
        ctx.ctx.unquotedKeys = jo->unquotedKeys;
        ctx.ctx.stringNodeIds = jo->stringNodeIds;
    }

    status ret = UA_NetworkMessage_encodeJsonInternal(&ctx, src);
    if(ret != UA_STATUSCODE_GOOD)
        return 0;

    return (size_t)ctx.ctx.pos;
}

/* decode json */
static status
MetaDataVersion_decodeJsonInternal(ParseCtx *ctx, void* cvd, const UA_DataType *_) {
    return decodeJsonJumpTable[UA_DATATYPEKIND_STRUCTURE]
        (ctx, cvd, &UA_TYPES[UA_TYPES_CONFIGURATIONVERSIONDATATYPE]);
}

static size_t
decodingFieldIndex(const UA_DataSetMessage_EncodingMetaData *emd,
                   UA_String name, size_t origIndex) {
    if(!emd)
        return origIndex;
    for(size_t i = 0; i < emd->fieldsSize; i++) {
        if(UA_String_equal(&name, &emd->fields[i].name))
            return i;
    }
    return origIndex;
}

struct PayloadData {
    UA_NetworkMessage *nm;
    size_t dsmIndex;
};

static status
DataSetPayload_decodeJsonInternal(PubSubDecodeJsonCtx *ctx, void *data, const UA_DataType *_) {
    struct PayloadData *pd = (struct PayloadData*)data;
    UA_NetworkMessage *nm = pd->nm;
    UA_DataSetMessage *dsm = &nm->payload.dataSetMessages[pd->dsmIndex];

    dsm->header.dataSetMessageValid = true;

    if(currentTokenType(&ctx->ctx) == CJ5_TOKEN_NULL) {
        ctx->ctx.index++;
        return UA_STATUSCODE_GOOD;
    }

    if(currentTokenType(&ctx->ctx) != CJ5_TOKEN_OBJECT)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* The number of key-value pairs */
    UA_assert(ctx->ctx.tokens[ctx->ctx.index].size % 2 == 0);
    size_t length = (size_t)(ctx->ctx.tokens[ctx->ctx.index].size) / 2;

    dsm->data.keyFrameFields = (UA_DataValue *)
        UA_Array_new(length, &UA_TYPES[UA_TYPES_DATAVALUE]);
    if(!dsm->data.keyFrameFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    dsm->fieldCount = (UA_UInt16)length;

    dsm->header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;

    const UA_DataSetMessage_EncodingMetaData *emd =
            findEncodingMetaData(&ctx->eo, nm->dataSetWriterIds[pd->dsmIndex]);

    /* Iterate over the key/value pairs in the object. Keys are stored in fieldnames. */
    ctx->ctx.index++; /* Go to the first key */
    status ret = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < length; ++i) {
        UA_assert(currentTokenType(&ctx->ctx) == CJ5_TOKEN_STRING);
        UA_String fieldName = UA_STRING_NULL;
        ret = decodeJsonJumpTable[UA_DATATYPEKIND_STRING](&ctx->ctx, &fieldName, NULL);
        UA_CHECK_STATUS(ret, return ret);

        size_t index = decodingFieldIndex(emd, fieldName, i);
        UA_DataValue_clear(&dsm->data.keyFrameFields[index]);
        UA_String_clear(&fieldName);
        ret = decodeJsonJumpTable[UA_DATATYPEKIND_DATAVALUE]
            (&ctx->ctx, &dsm->data.keyFrameFields[index], NULL);
        UA_CHECK_STATUS(ret, return ret);
    }

    return ret;
}

static status
DatasetMessage_Payload_decodeJsonInternal(PubSubDecodeJsonCtx *ctx, UA_NetworkMessage *nm,
                                          size_t dsmIndex) {
    UA_DataSetMessage *dsm = &nm->payload.dataSetMessages[dsmIndex];
    UA_ConfigurationVersionDataType cvd;
    struct PayloadData pd;
    pd.nm = nm;
    pd.dsmIndex = dsmIndex;

    DecodeEntry entries[7] = {
        {UA_DECODEKEY_DATASETWRITERID, &nm->dataSetWriterIds[dsmIndex], NULL, false, &UA_TYPES[UA_TYPES_UINT16]},
        {UA_DECODEKEY_SEQUENCENUMBER, &dsm->header.dataSetMessageSequenceNr, NULL, false, &UA_TYPES[UA_TYPES_UINT16]},
        {UA_DECODEKEY_METADATAVERSION, &cvd, &MetaDataVersion_decodeJsonInternal, false, NULL},
        {UA_DECODEKEY_TIMESTAMP, &dsm->header.timestamp, NULL, false, &UA_TYPES[UA_TYPES_DATETIME]},
        {UA_DECODEKEY_DSM_STATUS, &dsm->header.status, NULL, false, &UA_TYPES[UA_TYPES_UINT16]},
        {UA_DECODEKEY_MESSAGETYPE, NULL, NULL, false, NULL},
        {UA_DECODEKEY_PAYLOAD, &pd, (decodeJsonSignature)DataSetPayload_decodeJsonInternal, false, NULL}
    };
    status ret = decodeFields(&ctx->ctx, entries, 7);

    /* Error or no DatasetWriterId found or no payload found */
    if(ret != UA_STATUSCODE_GOOD || !entries[0].found || !entries[6].found)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* TODO: Check FieldEncoding1 and FieldEncoding2 to determine the field encoding */
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
DatasetMessage_Array_decodeJsonInternal(PubSubDecodeJsonCtx *ctx, void *UA_RESTRICT dst,
                                        const UA_DataType *_) {
    /* Array or object */
    size_t length = 1;
    if(currentTokenType(&ctx->ctx) == CJ5_TOKEN_ARRAY) {
        length = (size_t)ctx->ctx.tokens[ctx->ctx.index].size;

        /* Go to the first array member */
        ctx->ctx.index++;

        /* Return early for empty arrays */
        if(length == 0)
            return UA_STATUSCODE_GOOD;
    } else if(currentTokenType(&ctx->ctx) != CJ5_TOKEN_OBJECT) {
        return UA_STATUSCODE_BADDECODINGERROR;
    }

    /* Decode array members */
    UA_NetworkMessage *nm = (UA_NetworkMessage*)dst;
    for(size_t i = 0; i < length; ++i) {
        status ret = DatasetMessage_Payload_decodeJsonInternal(ctx, nm, i);
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
NetworkMessage_decodeJsonInternal(PubSubDecodeJsonCtx *ctx,
                                  UA_NetworkMessage *dst) {
    memset(dst, 0, sizeof(UA_NetworkMessage));
    dst->chunkMessage = false;
    dst->groupHeaderEnabled = false;
    dst->payloadHeaderEnabled = false;
    dst->picosecondsEnabled = false;
    dst->promotedFieldsEnabled = false;

    /* Is Messages an Array? How big? */
    size_t searchResultMessages = 0;
    status found = lookAheadForKey(&ctx->ctx, UA_DECODEKEY_MESSAGES, &searchResultMessages);
    if(found != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    const cj5_token *bodyToken = &ctx->ctx.tokens[searchResultMessages];
    size_t messageCount = 1;
    if(bodyToken->type == CJ5_TOKEN_ARRAY)
        messageCount = (size_t)bodyToken->size;

    /* Too many DataSetMessages */
    if(messageCount > UA_NETWORKMESSAGE_MAXMESSAGECOUNT)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* MessageType */
    UA_Boolean isUaData = true;
    size_t searchResultMessageType = 0;
    found = lookAheadForKey(&ctx->ctx, UA_DECODEKEY_MESSAGETYPE, &searchResultMessageType);
    if(found != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;
    size_t size = getTokenLength(&ctx->ctx.tokens[searchResultMessageType]);
    const char* msgType = &ctx->ctx.json5[ctx->ctx.tokens[searchResultMessageType].start];
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

    dst->payload.dataSetMessages = (UA_DataSetMessage*)
        UA_calloc(messageCount, sizeof(UA_DataSetMessage));
    if(!dst->payload.dataSetMessages)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    dst->messageCount = messageCount;

    /* Network Message */
    UA_String messageType;
    DecodeEntry entries[5] = {
        {UA_DECODEKEY_MESSAGEID, &dst->messageId, NULL, false, &UA_TYPES[UA_TYPES_STRING]},
        {UA_DECODEKEY_MESSAGETYPE, &messageType, NULL, false, NULL},
        {UA_DECODEKEY_PUBLISHERID, &dst->publisherId, decodePublisherIdJsonInternal, false, NULL},
        {UA_DECODEKEY_DATASETCLASSID, &dst->dataSetClassId, NULL, false, &UA_TYPES[UA_TYPES_GUID]},
        {UA_DECODEKEY_MESSAGES, dst, (decodeJsonSignature)DatasetMessage_Array_decodeJsonInternal, false, NULL}
    };

    status ret = decodeFields(&ctx->ctx, entries, 5);
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
                             const UA_NetworkMessage_EncodingOptions *eo,
                             const UA_DecodeJsonOptions *jo) {
    /* Set up the context */
    cj5_token tokens[UA_JSON_MAXTOKENCOUNT];
    PubSubDecodeJsonCtx ctx;
    memset(&ctx, 0, sizeof(PubSubDecodeJsonCtx));
    ctx.ctx.tokens = tokens;
    if(eo)
        ctx.eo = *eo;
    if(jo) {
        ctx.ctx.namespaceMapping = jo->namespaceMapping;
        ctx.ctx.serverUrisSize = jo->serverUrisSize;
        ctx.ctx.serverUris = jo->serverUris;
        ctx.ctx.customTypes = jo->customTypes;
    }

    status ret = tokenize(&ctx.ctx, src, UA_JSON_MAXTOKENCOUNT, NULL);
    if(ret != UA_STATUSCODE_GOOD)
        goto cleanup;

    ret = NetworkMessage_decodeJsonInternal(&ctx, dst);
    if(ret != UA_STATUSCODE_GOOD)
        UA_NetworkMessage_clear(dst);

 cleanup:
    /* Free token array on the heap */
    if(ctx.ctx.tokens != tokens)
        UA_free((void*)(uintptr_t)ctx.ctx.tokens);
    return ret;
}
