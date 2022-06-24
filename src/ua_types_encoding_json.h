/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Lukas Meling)
 */

#ifndef UA_TYPES_ENCODING_JSON_H_
#define UA_TYPES_ENCODING_JSON_H_

#include <open62541/types.h>

#include "ua_types_encoding_binary.h"
#include "ua_types_encoding_json.h"
#include "ua_util_internal.h"

#include "../deps/jsmn/jsmn.h"

_UA_BEGIN_DECLS

#define UA_JSON_MAXTOKENCOUNT 2048

/* Returns the number of bytes the value src takes in json encoding. Returns
 * zero if an error occurs. */
size_t
UA_calcSizeJsonInternal(const void *src, const UA_DataType *type,
                        const UA_String *namespaces, size_t namespaceSize,
                        const UA_String *serverUris, size_t serverUriSize,
                        UA_Boolean useReversible) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

/* Encodes the scalar value described by type to json encoding.
 *
 * @param src The value. Must not be NULL.
 * @param type The value type. Must not be NULL.
 * @param bufPos Points to a pointer to the current position in the encoding
 *        buffer. Must not be NULL.
 * @param bufEnd Points to a pointer to the end of the encoding buffer (encoding
 *        always stops before *buf_end). Must not be NULL.
 * @param namespaces An array of namespaces
 * @param namespaceSize The size of the namespaces array
 * @param serverUris An array of serverUris
 * @param serverUriSize The size of the serverUris array
 * @param useReversible preserve datatypes in json encoding
 * @return Returns a statuscode whether encoding succeeded. */
UA_StatusCode
UA_encodeJsonInternal(const void *src, const UA_DataType *type, uint8_t **bufPos,
                      const uint8_t **bufEnd, const UA_String *namespaces,
                      size_t namespaceSize, const UA_String *serverUris,
                      size_t serverUriSize,
                      UA_Boolean useReversible) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

/* Interal Definitions
 *
 * For future by the PubSub encoding */

#define UA_JSON_ENCODING_MAX_RECURSION 100
typedef struct {
    uint8_t *pos;
    const uint8_t *end;

    uint16_t depth; /* How often did we en-/decoding recurse? */
    UA_Boolean commaNeeded[UA_JSON_ENCODING_MAX_RECURSION];
    UA_Boolean useReversible;
    UA_Boolean calcOnly; /* Only compute the length of the decoding */

    size_t namespacesSize;
    const UA_String *namespaces;

    size_t serverUrisSize;
    const UA_String *serverUris;
} CtxJson;

UA_StatusCode writeJsonObjStart(CtxJson *ctx);
UA_StatusCode writeJsonObjElm(CtxJson *ctx, const char *key,
                              const void *value, const UA_DataType *type);
UA_StatusCode writeJsonObjEnd(CtxJson *ctx);

UA_StatusCode writeJsonArrStart(CtxJson *ctx);
UA_StatusCode writeJsonArrElm(CtxJson *ctx, const void *value,
                              const UA_DataType *type);
UA_StatusCode writeJsonArrEnd(CtxJson *ctx);

UA_StatusCode writeJsonKey(CtxJson *ctx, const char* key);
UA_StatusCode writeJsonCommaIfNeeded(CtxJson *ctx);
UA_StatusCode writeJsonNull(CtxJson *ctx);

/* The encoding length is returned in ctx->pos */
static UA_INLINE UA_StatusCode
calcJsonObjStart(CtxJson *ctx) {
    UA_assert(ctx->calcOnly);
    return writeJsonObjStart(ctx);
}

static UA_INLINE UA_StatusCode
calcJsonObjElm(CtxJson *ctx, const char *key,
               const void *value, const UA_DataType *type) {
    UA_assert(ctx->calcOnly);
    return writeJsonObjElm(ctx, key, value, type);
}

static UA_INLINE UA_StatusCode
calcJsonObjEnd(CtxJson *ctx) {
    UA_assert(ctx->calcOnly);
    return writeJsonObjEnd(ctx);
}

static UA_INLINE UA_StatusCode
calcJsonArrStart(CtxJson *ctx) {
    UA_assert(ctx->calcOnly);
    return writeJsonArrStart(ctx);
}

static UA_INLINE UA_StatusCode
calcJsonArrElm(CtxJson *ctx, const void *value,
               const UA_DataType *type) {
    UA_assert(ctx->calcOnly);
    return writeJsonArrElm(ctx, value, type);
}

static UA_INLINE UA_StatusCode
calcJsonArrEnd(CtxJson *ctx) {
    UA_assert(ctx->calcOnly);
    return writeJsonArrEnd(ctx);
}

status
encodeJsonInternal(const void *src, const UA_DataType *type, CtxJson *ctx);

typedef struct {
    jsmntok_t *tokenArray;
    UA_Int32 tokenCount;
    UA_UInt16 index;

    /* Additonal data for special cases such as networkmessage/datasetmessage
     * Currently only used for dataSetWriterIds */
    size_t numCustom;
    void * custom;
    size_t currentCustomIndex;

    const UA_DataTypeArray *customTypes;
} ParseCtx;

typedef UA_StatusCode
(*encodeJsonSignature)(const void *src, const UA_DataType *type, CtxJson *ctx);

typedef UA_StatusCode
(*decodeJsonSignature)(void *dst, const UA_DataType *type,
                       CtxJson *ctx, ParseCtx *parseCtx);

/* Map for decoding a Json Object. An array of this is passed to the
 * decodeFields function. If the key "fieldName" is found in the json object
 * (mark as found and) decode the value with the "function" and write result
 * into "fieldPointer" (destination). */
typedef struct {
    const char *fieldName;
    void *fieldPointer;
    decodeJsonSignature function;
    UA_Boolean found;
    const UA_DataType *type; /* Must be set for values that can be "null". If
                              * the function is not set, decode via the
                              * type->typeKind. */
} DecodeEntry;

UA_StatusCode
decodeFields(CtxJson *ctx, ParseCtx *parseCtx,
             DecodeEntry *entries, size_t entryCount);

/* Expose the jump tables and some methods for PubSub JSON decoding */
extern const encodeJsonSignature encodeJsonJumpTable[UA_DATATYPEKINDS];
extern const decodeJsonSignature decodeJsonJumpTable[UA_DATATYPEKINDS];

UA_StatusCode lookAheadForKey(const char* search, CtxJson *ctx,
                              ParseCtx *parseCtx, size_t *resultIndex);
UA_StatusCode tokenize(ParseCtx *parseCtx, CtxJson *ctx,
                       const UA_ByteString *src, size_t tokensSize);
UA_Boolean isJsonNull(const CtxJson *ctx, const ParseCtx *parseCtx);

static UA_INLINE
jsmntype_t getJsmnType(const ParseCtx *parseCtx) {
    return parseCtx->tokenArray[parseCtx->index].type;
}

_UA_END_DECLS

#endif /* UA_TYPES_ENCODING_JSON_H_ */
