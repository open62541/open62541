/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Lukas Meling)
 */

#include <open62541/types.h>
#include <open62541/types_generated_handling.h>

#include "ua_pubsub_networkmessage.h"
#include "ua_types_encoding_json.h"

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
                                      UA_UInt16 dataSetWriterId,
                                      CtxJson *ctx) {
    status rv = writeJsonObjStart(ctx);

    /* DataSetWriterId */
    rv |= writeJsonObjElm(ctx, UA_DECODEKEY_DATASETWRITERID,
                          &dataSetWriterId, &UA_TYPES[UA_TYPES_UINT16]);
    if(rv != UA_STATUSCODE_GOOD)
        return rv;

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

    rv |= writeJsonKey(ctx, UA_DECODEKEY_PAYLOAD);
    rv |= writeJsonObjStart(ctx);

    /* TODO: currently no difference between delta and key frames. Own
     * dataSetMessageType for json?. If the field names are not defined, write
     * out empty field names. */
    if(src->header.dataSetMessageType != UA_DATASETMESSAGE_DATAKEYFRAME)
        return UA_STATUSCODE_BADNOTSUPPORTED; /* Delta frames not supported */

    if(src->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
        /* KEYFRAME VARIANT */
        for(UA_UInt16 i = 0; i < src->data.keyFrameData.fieldCount; i++) {
            if(src->data.keyFrameData.fieldNames)
                rv |= writeJsonKey_UA_String(ctx, &src->data.keyFrameData.fieldNames[i]);
            else
                rv |= writeJsonKey(ctx, "");
            rv |= encodeJsonInternal(&(src->data.keyFrameData.dataSetFields[i].value),
                                     &UA_TYPES[UA_TYPES_VARIANT], ctx);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
        }
    } else if(src->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
        /* KEYFRAME DATAVALUE */
        for(UA_UInt16 i = 0; i < src->data.keyFrameData.fieldCount; i++) {
            if(src->data.keyFrameData.fieldNames)
                rv |= writeJsonKey_UA_String(ctx, &src->data.keyFrameData.fieldNames[i]);
            else
                rv |= writeJsonKey(ctx, "");
            rv |= encodeJsonInternal(&src->data.keyFrameData.dataSetFields[i],
                                     &UA_TYPES[UA_TYPES_DATAVALUE], ctx);
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
    status rv = UA_STATUSCODE_GOOD;
    /* currently only ua-data is supported, no discovery message implemented */
    if(src->networkMessageType != UA_NETWORKMESSAGE_DATASET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    writeJsonObjStart(ctx);

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

    /* PublisherId */
    if(src->publisherIdEnabled) {
        rv = writeJsonKey(ctx, UA_DECODEKEY_PUBLISHERID);
        switch (src->publisherIdType) {
        case UA_PUBLISHERDATATYPE_BYTE:
            rv |= encodeJsonInternal(&src->publisherId.publisherIdByte,
                                     &UA_TYPES[UA_TYPES_BYTE], ctx);
            break;

        case UA_PUBLISHERDATATYPE_UINT16:
            rv |= encodeJsonInternal(&src->publisherId.publisherIdUInt16,
                                     &UA_TYPES[UA_TYPES_UINT16], ctx);
            break;

        case UA_PUBLISHERDATATYPE_UINT32:
            rv |= encodeJsonInternal(&src->publisherId.publisherIdUInt32,
                                     &UA_TYPES[UA_TYPES_UINT32], ctx);
            break;

        case UA_PUBLISHERDATATYPE_UINT64:
            rv |= encodeJsonInternal(&src->publisherId.publisherIdUInt64,
                                     &UA_TYPES[UA_TYPES_UINT64], ctx);
            break;

        case UA_PUBLISHERDATATYPE_STRING:
            rv |= encodeJsonInternal(&src->publisherId.publisherIdString,
                                     &UA_TYPES[UA_TYPES_STRING], ctx);
            break;
        }
    }
    if(rv != UA_STATUSCODE_GOOD)
        return rv;

    /* DataSetClassId */
    if(src->dataSetClassIdEnabled) {
        rv |= writeJsonObjElm(ctx, UA_DECODEKEY_DATASETCLASSID,
                              &src->dataSetClassId, &UA_TYPES[UA_TYPES_GUID]);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    /* Payload: DataSetMessages */
    UA_Byte count = src->payloadHeader.dataSetPayloadHeader.count;
    if(count > 0) {
        UA_UInt16 *dataSetWriterIds =
            src->payloadHeader.dataSetPayloadHeader.dataSetWriterIds;
        if(!dataSetWriterIds)
            return UA_STATUSCODE_BADENCODINGERROR;

        rv |= writeJsonKey(ctx, UA_DECODEKEY_MESSAGES);
        rv |= writeJsonArrStart(ctx); /* start array */

        const UA_DataSetMessage *dataSetMessages =
            src->payload.dataSetPayload.dataSetMessages;
        for(UA_UInt16 i = 0; i < count; i++) {
            writeJsonCommaIfNeeded(ctx);
            rv |= UA_DataSetMessage_encodeJson_internal(&dataSetMessages[i],
                                                        dataSetWriterIds[i], ctx);
            if(rv != UA_STATUSCODE_GOOD)
                return rv;
            /* comma is needed if more dsm are present */
            ctx->commaNeeded[ctx->depth] = true;
        }
        rv |= writeJsonArrEnd(ctx); /* end array */
    }

    rv |= writeJsonObjEnd(ctx);
    return rv;
}

UA_StatusCode
UA_NetworkMessage_encodeJson(const UA_NetworkMessage *src,
                             UA_Byte **bufPos, const UA_Byte **bufEnd,
                             UA_String *namespaces, size_t namespaceSize,
                             UA_String *serverUris, size_t serverUriSize,
                             UA_Boolean useReversible) {
    /* Set up the context */
    CtxJson ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.pos = *bufPos;
    ctx.end = *bufEnd;
    ctx.depth = 0;
    ctx.namespaces = namespaces;
    ctx.namespacesSize = namespaceSize;
    ctx.serverUris = serverUris;
    ctx.serverUrisSize = serverUriSize;
    ctx.useReversible = useReversible;
    ctx.calcOnly = false;

    status ret = UA_NetworkMessage_encodeJson_internal(src, &ctx);

    *bufPos = ctx.pos;
    *bufEnd = ctx.end;
    return ret;
}

size_t
UA_NetworkMessage_calcSizeJson(const UA_NetworkMessage *src,
                               UA_String *namespaces, size_t namespaceSize,
                               UA_String *serverUris, size_t serverUriSize,
                               UA_Boolean useReversible){
    /* Set up the context */
    CtxJson ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.pos = 0;
    ctx.end = (const UA_Byte*)(uintptr_t)SIZE_MAX;
    ctx.depth = 0;
    ctx.namespaces = namespaces;
    ctx.namespacesSize = namespaceSize;
    ctx.serverUris = serverUris;
    ctx.serverUrisSize = serverUriSize;
    ctx.useReversible = useReversible;
    ctx.calcOnly = true;

    status ret = UA_NetworkMessage_encodeJson_internal(src, &ctx);
    if(ret != UA_STATUSCODE_GOOD)
        return 0;
    return (size_t)ctx.pos;
}

/* decode json */
static status
MetaDataVersion_decodeJsonInternal(void* cvd, const UA_DataType *type,
                                   CtxJson *ctx, ParseCtx *parseCtx) {
    return decodeJsonJumpTable[UA_DATATYPEKIND_STRUCTURE]
        (cvd, &UA_TYPES[UA_TYPES_CONFIGURATIONVERSIONDATATYPE], ctx, parseCtx);
}

static status
DataSetPayload_decodeJsonInternal(void* dsmP, const UA_DataType *type,
                                  CtxJson *ctx, ParseCtx *parseCtx) {
    UA_DataSetMessage* dsm = (UA_DataSetMessage*)dsmP;
    dsm->header.dataSetMessageValid = true;
    if(isJsonNull(ctx, parseCtx)) {
        parseCtx->index++;
        return UA_STATUSCODE_GOOD;
    }

    size_t length = (size_t)parseCtx->tokenArray[parseCtx->index].size;
    UA_String *fieldNames = (UA_String*)UA_calloc(length, sizeof(UA_String));
    dsm->data.keyFrameData.fieldNames = fieldNames;
    dsm->data.keyFrameData.fieldCount = (UA_UInt16)length;
    dsm->data.keyFrameData.dataSetFields = (UA_DataValue *)
        UA_Array_new(dsm->data.keyFrameData.fieldCount, &UA_TYPES[UA_TYPES_DATAVALUE]);

    status ret = UA_STATUSCODE_GOOD;

    parseCtx->index++; // We go to first Object key!

    /* iterate over the key/value pairs in the object. Keys are stored in fieldnames. */
    for(size_t i = 0; i < length; ++i) {
        ret = decodeJsonJumpTable[UA_DATATYPEKIND_STRING](&fieldNames[i], type, ctx, parseCtx);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;

        //TODO: Is field value a variant or datavalue? Current check if type and body present.
        size_t searchResult = 0;
        status foundType = lookAheadForKey("Type", ctx, parseCtx, &searchResult);
        status foundBody = lookAheadForKey("Body", ctx, parseCtx, &searchResult);
        if(foundType == UA_STATUSCODE_GOOD && foundBody == UA_STATUSCODE_GOOD){
            dsm->header.fieldEncoding = UA_FIELDENCODING_VARIANT;
            ret = decodeJsonJumpTable[UA_DATATYPEKIND_VARIANT]
                (&dsm->data.keyFrameData.dataSetFields[i].value, type, ctx, parseCtx);
            dsm->data.keyFrameData.dataSetFields[i].hasValue = true;
        } else {
            dsm->header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
            ret = decodeJsonJumpTable[UA_DATATYPEKIND_DATAVALUE]
                (&dsm->data.keyFrameData.dataSetFields[i], type, ctx, parseCtx);
            dsm->data.keyFrameData.dataSetFields[i].hasValue = true;
        }

        if(ret != UA_STATUSCODE_GOOD)
            return ret;
    }

    return ret;
}

static status
DatasetMessage_Payload_decodeJsonInternal(UA_DataSetMessage* dsm, const UA_DataType *type,
                                          CtxJson *ctx, ParseCtx *parseCtx) {
    UA_ConfigurationVersionDataType cvd;
    UA_UInt16 dataSetWriterId;

    dsm->header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;

    DecodeEntry entries[6] = {
        {UA_DECODEKEY_DATASETWRITERID, &dataSetWriterId, NULL, false, &UA_TYPES[UA_TYPES_UINT16]},
        {UA_DECODEKEY_SEQUENCENUMBER, &dsm->header.dataSetMessageSequenceNr, NULL, false, &UA_TYPES[UA_TYPES_UINT16]},
        {UA_DECODEKEY_METADATAVERSION, &cvd, &MetaDataVersion_decodeJsonInternal, false, NULL},
        {UA_DECODEKEY_TIMESTAMP, &dsm->header.timestamp, NULL, false, &UA_TYPES[UA_TYPES_DATETIME]},
        {UA_DECODEKEY_DSM_STATUS, &dsm->header.status, NULL, false, &UA_TYPES[UA_TYPES_UINT16]},
        {UA_DECODEKEY_PAYLOAD, dsm, &DataSetPayload_decodeJsonInternal, false, NULL}
    };
    status ret = decodeFields(ctx, parseCtx, entries, 6);

    /* Error or no DatasetWriterId found or no payload found */
    if(ret != UA_STATUSCODE_GOOD || !entries[0].found || !entries[5].found)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Set the DatasetWriterId in the context */
    if(!parseCtx->custom)
        return UA_STATUSCODE_BADDECODINGERROR;
    if(parseCtx->currentCustomIndex >= parseCtx->numCustom)
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_UInt16* dataSetWriterIdsArray = (UA_UInt16*)parseCtx->custom;
    dataSetWriterIdsArray[parseCtx->currentCustomIndex] = dataSetWriterId;
    parseCtx->currentCustomIndex++;

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
DatasetMessage_Array_decodeJsonInternal(void *UA_RESTRICT dst, const UA_DataType *type,
                                        CtxJson *ctx, ParseCtx *parseCtx) {
    /* Array! */
    if(getJsmnType(parseCtx) != JSMN_ARRAY)
        return UA_STATUSCODE_BADDECODINGERROR;
    size_t length = (size_t)parseCtx->tokenArray[parseCtx->index].size;

    /* Return early for empty arrays */
    if(length == 0)
        return UA_STATUSCODE_GOOD;

    /* Allocate memory */
    UA_DataSetMessage *dsm = (UA_DataSetMessage*)
        UA_calloc(length, sizeof(UA_DataSetMessage));
    if(!dsm)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Copy new Pointer do dest */
    memcpy(dst, &dsm, sizeof(void*));

    /* We go to first Array member! */
    parseCtx->index++;

    status ret = UA_STATUSCODE_BADDECODINGERROR;
    /* Decode array members */
    for(size_t i = 0; i < length; ++i) {
        ret = DatasetMessage_Payload_decodeJsonInternal(&dsm[i], NULL, ctx, parseCtx);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
    }

    return ret;
}

static status
NetworkMessage_decodeJsonInternal(UA_NetworkMessage *dst, CtxJson *ctx,
                                  ParseCtx *parseCtx) {
    memset(dst, 0, sizeof(UA_NetworkMessage));
    dst->chunkMessage = false;
    dst->groupHeaderEnabled = false;
    dst->payloadHeaderEnabled = false;
    dst->picosecondsEnabled = false;
    dst->promotedFieldsEnabled = false;

    /* Look forward for publisheId, if present check if type if primitve (Number) or String. */
    const UA_DataType *pubIdType = &UA_TYPES[UA_TYPES_STRING];
    size_t searchResultPublishIdType = 0;
    status found = lookAheadForKey(UA_DECODEKEY_PUBLISHERID, ctx,
                                   parseCtx, &searchResultPublishIdType);
    if(found == UA_STATUSCODE_GOOD) {
        jsmntok_t publishIdToken = parseCtx->tokenArray[searchResultPublishIdType];
        if(publishIdToken.type == JSMN_PRIMITIVE) {
            pubIdType = &UA_TYPES[UA_TYPES_UINT64];
            dst->publisherIdType = UA_PUBLISHERDATATYPE_UINT64; //store in biggest possible
        } else if(publishIdToken.type == JSMN_STRING) {
            dst->publisherIdType = UA_PUBLISHERDATATYPE_STRING;
        } else {
            return UA_STATUSCODE_BADDECODINGERROR;
        }
    }

    /* Is Messages an Array? How big? */
    size_t searchResultMessages = 0;
    found = lookAheadForKey(UA_DECODEKEY_MESSAGES, ctx, parseCtx, &searchResultMessages);
    if(found != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    jsmntok_t bodyToken = parseCtx->tokenArray[searchResultMessages];
    if(bodyToken.type != JSMN_ARRAY)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    size_t messageCount = (size_t)parseCtx->tokenArray[searchResultMessages].size;

    /* Set up custom context for the dataSetwriterId */
    parseCtx->custom = (void*)UA_calloc(messageCount, sizeof(UA_UInt16));
    parseCtx->currentCustomIndex = 0;
    parseCtx->numCustom = messageCount;

    /* MessageType */
    UA_Boolean isUaData = true;
    size_t searchResultMessageType = 0;
    found = lookAheadForKey(UA_DECODEKEY_MESSAGETYPE, ctx, parseCtx, &searchResultMessageType);
    if(found != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;
    size_t size = (size_t)(parseCtx->tokenArray[searchResultMessageType].end -
                           parseCtx->tokenArray[searchResultMessageType].start);
    char* msgType = (char*)(ctx->pos + parseCtx->tokenArray[searchResultMessageType].start);
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

    /* Network Message */
    UA_String messageType;
    DecodeEntry entries[5] = {
        {UA_DECODEKEY_MESSAGEID, &dst->messageId, NULL, false, &UA_TYPES[UA_TYPES_STRING]},
        {UA_DECODEKEY_MESSAGETYPE, &messageType, NULL, false, NULL},
        {UA_DECODEKEY_PUBLISHERID, &dst->publisherId.publisherIdString, NULL, false, pubIdType},
        {UA_DECODEKEY_DATASETCLASSID, &dst->dataSetClassId, NULL, false, &UA_TYPES[UA_TYPES_GUID]},
        {UA_DECODEKEY_MESSAGES, &dst->payload.dataSetPayload.dataSetMessages, &DatasetMessage_Array_decodeJsonInternal, false, NULL}
    };

    //Store publisherId in correct union
    if(pubIdType == &UA_TYPES[UA_TYPES_UINT64])
        entries[2].fieldPointer = &dst->publisherId.publisherIdUInt64;

    status ret = decodeFields(ctx, parseCtx, entries, 5);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    dst->messageIdEnabled = entries[0].found;
    dst->publisherIdEnabled = entries[2].found;
    if(dst->publisherIdEnabled)
        dst->publisherIdType = UA_PUBLISHERDATATYPE_STRING;
    dst->dataSetClassIdEnabled = entries[3].found;
    dst->payloadHeaderEnabled = true;
    dst->payloadHeader.dataSetPayloadHeader.count = (UA_Byte)messageCount;

    /* Set the dataSetWriterIds. They are filled in the dataSet decoding. */
    dst->payloadHeader.dataSetPayloadHeader.dataSetWriterIds = (UA_UInt16*)parseCtx->custom;
    return ret;
}

status
UA_NetworkMessage_decodeJson(UA_NetworkMessage *dst, const UA_ByteString *src) {
    /* Set up the context */
    CtxJson ctx;
    memset(&ctx, 0, sizeof(CtxJson));
    ParseCtx parseCtx;
    memset(&parseCtx, 0, sizeof(ParseCtx));
    parseCtx.tokenArray = (jsmntok_t*)
        UA_malloc(sizeof(jsmntok_t) * UA_JSON_MAXTOKENCOUNT);
    if(!parseCtx.tokenArray)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(parseCtx.tokenArray, 0, sizeof(jsmntok_t) * UA_JSON_MAXTOKENCOUNT);
    status ret = tokenize(&parseCtx, &ctx, src, UA_JSON_MAXTOKENCOUNT);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;
    ret = NetworkMessage_decodeJsonInternal(dst, &ctx, &parseCtx);
    UA_free(parseCtx.tokenArray);
    return ret;
}
