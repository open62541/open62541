/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Lukas Meling)
 */

#ifndef UA_TYPES_ENCODING_JSON_H_
#define UA_TYPES_ENCODING_JSON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_util_internal.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_encoding_json.h"
#include "ua_types.h"
#include "../deps/jsmn/jsmn.h"
 
#define TOKENCOUNT 1000
    
size_t
UA_calcSizeJson(const void *src, const UA_DataType *type,
        UA_String *namespaces, 
        size_t namespaceSize,
        UA_String *serverUris,
        size_t serverUriSize,
        UA_Boolean useReversible) UA_FUNC_ATTR_WARN_UNUSED_RESULT;


status
UA_encodeJson(const void *src, const UA_DataType *type,
        u8 **bufPos, 
        const u8 **bufEnd, 
        UA_String *namespaces, 
        size_t namespaceSize,
        UA_String *serverUris,
        size_t serverUriSize,
        UA_Boolean useReversible) UA_FUNC_ATTR_WARN_UNUSED_RESULT;


UA_StatusCode
UA_decodeJson(const UA_ByteString *src, void *dst,
                const UA_DataType *type) UA_FUNC_ATTR_WARN_UNUSED_RESULT;





/* 
 * Functions for future use for the pubsub NetworkMessage and DataSetMessage.
 * Don't bother using them. 
 */

typedef struct {
    u8 *pos;
    const u8 *end;

    u16 depth; /* How often did we en-/decoding recurse? */

    size_t namespacesSize;
    UA_String *namespaces;
    
    size_t serverUrisSize;
    UA_String *serverUris;
    
    UA_Boolean useReversible;
} CtxJson;

status writeKey_UA_String(CtxJson *ctx, UA_String *key, UA_Boolean commaNeeded);
status writeKey(CtxJson *ctx, const char* key, UA_Boolean commaNeeded);
status encodingJsonStartObject(CtxJson *ctx);
size_t encodingJsonEndObject(CtxJson *ctx);
status encodingJsonStartArray(CtxJson *ctx);
size_t encodingJsonEndArray(CtxJson *ctx);
status writeComma(CtxJson *ctx, UA_Boolean commaNeeded);
status writeNull(CtxJson *ctx);

status calcWriteKey_UA_String(CtxJson *ctx, UA_String *key, UA_Boolean commaNeeded);
status calcWriteKey(CtxJson *ctx, const char* key,
                    UA_Boolean commaNeeded) UA_FUNC_ATTR_WARN_UNUSED_RESULT;
status encodingCalcJsonStartObject(CtxJson *ctx);
size_t encodingCalcJsonEndObject(CtxJson *ctx);
status encodingCalcJsonStartArray(CtxJson *ctx);
size_t encodingCalcJsonEndArray(CtxJson *ctx);
status calcWriteComma(CtxJson *ctx, UA_Boolean commaNeeded);
status calcWriteNull(CtxJson *ctx);
status calcJsonInternal(const void *src, const UA_DataType *type, CtxJson *ctx);


typedef struct {
    jsmntok_t *tokenArray;
    UA_Int32 tokenCount;
    UA_UInt16 *index;
    
    /* Additonal data for special cases such as networkmessage/datasetmessage
     * Currently only used for dataSetWriterIds*/
    size_t numCustom;
    void * custom;
    size_t* currentCustomIndex;
} ParseCtx;

typedef status(*encodeJsonSignature)(const void *UA_RESTRICT src, const UA_DataType *type,
        CtxJson *UA_RESTRICT ctx);

typedef status(*calcSizeJsonSignature)(const void *UA_RESTRICT src, const UA_DataType *type,
        CtxJson *UA_RESTRICT ctx);

typedef status (*decodeJsonSignature)(void *UA_RESTRICT dst, const UA_DataType *type,
                                        CtxJson *UA_RESTRICT ctx, ParseCtx *parseCtx, UA_Boolean moveToken);

typedef struct {
    const char ** fieldNames;
    void ** fieldPointer;
    decodeJsonSignature * functions;
    UA_Boolean * found;
    u8 memberSize;
} DecodeContext;

status 
decodeFields(CtxJson *ctx, ParseCtx *parseCtx, DecodeContext *decodeContext, const UA_DataType *type);

status
decodeJsonInternal(void *dst, const UA_DataType *type, CtxJson *ctx, ParseCtx *parseCtx, UA_Boolean moveToken);

/* workaround: TODO generate functions for UA_xxx_decodeJson */
decodeJsonSignature getDecodeSignature(u8 index);
status lookAheadForKey(const char* search, CtxJson *ctx, ParseCtx *parseCtx, size_t *resultIndex);

jsmntype_t getJsmnType(const ParseCtx *parseCtx);
status tokenize(ParseCtx *parseCtx, CtxJson *ctx, const UA_ByteString *src, UA_UInt16 *tokenIndex);
UA_Boolean isJsonNull(const CtxJson *ctx, const ParseCtx *parseCtx);

#ifdef __cplusplus
}
#endif

#endif /* UA_TYPES_ENCODING_JSON_H_ */
