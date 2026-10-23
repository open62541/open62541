/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015 (c) Sten Grüner
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Lukas Meling)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#ifndef UA_TYPES_ENCODING_BINARY_H_
#define UA_TYPES_ENCODING_BINARY_H_

#include <open62541/types.h>

#ifdef UA_ENABLE_JSON_ENCODING
# include "../deps/cj5.h"
#endif

_UA_BEGIN_DECLS

/*******************/
/* Binary Encoding */
/*******************/

typedef UA_StatusCode
(*UA_exchangeEncodeBuffer)(void *handle, UA_Byte **bufPos,
                           const UA_Byte **bufEnd);

typedef struct {
    /* Pointers to the current and last buffer position */
    UA_Byte *pos;
    const UA_Byte *end;

    /* How often did we en-/decoding recurse? */
    UA_Byte depth;

    UA_DecodeBinaryOptions opts;

    UA_exchangeEncodeBuffer exchangeBufferCallback;
    void *exchangeBufferCallbackHandle;
} Ctx;

void * ctxCalloc(Ctx *ctx, size_t nelem, size_t elsize);
void ctxFree(Ctx *ctx, void *p);
void ctxClear(Ctx *ctx, void *p, const UA_DataType *type);

/* Jumptables for de-/encoding and computing the buffer length. The methods in
 * the decoding jumptable do not memset-zero initially and do not clean up their
 * allocated memory when an error occurs. So a final _clear needs to be called
 * before returning to the user. */
typedef UA_StatusCode
(*encodeBinarySignature)(Ctx *UA_RESTRICT ctx, const void *UA_RESTRICT src,
                         const UA_DataType *type);
typedef UA_StatusCode
(*decodeBinarySignature)(Ctx *UA_RESTRICT ctx, void *UA_RESTRICT dst,
                         const UA_DataType *type);
extern const encodeBinarySignature encodeBinaryJumpTable[UA_DATATYPEKINDS];
extern const decodeBinarySignature decodeBinaryJumpTable[UA_DATATYPEKINDS];

#define DECODE_BINARY(VAR, TYPE)                                    \
    decodeBinaryJumpTable[UA_DATATYPEKIND_##TYPE](ctx, VAR, NULL);

#define ENCODE_BINARY(VAR, TYPE)                                    \
    encodeBinaryJumpTable[UA_DATATYPEKIND_##TYPE](ctx, VAR, NULL);

/* Encodes the scalar value described by type in the binary encoding. Encoding
 * is thread-safe if thread-local variables are enabled. Encoding is also
 * reentrant and can be safely called from signal handlers or interrupts.
 *
 * @param src The value. Must not be NULL.
 * @param type The value type. Must not be NULL.
 * @param bufPos Points to a pointer to the current position in the encoding
 *        buffer. Must not be NULL. The pointer is advanced by the number of
 *        encoded bytes, or, if the buffer is exchanged, to the position in the
 *        new buffer.
 * @param bufEnd Points to a pointer to the end of the encoding buffer (encoding
 *        always stops before *buf_end). Must not be NULL. The pointer is
 *        changed when the buffer is exchanged.
 * @param exchangeCallback Called when the end of the buffer is reached. This is
          used to send out a message chunk before continuing with the encoding.
          Is ignored if NULL.
 * @param exchangeHandle Custom data passed into the exchangeCallback.
 * @return Returns a statuscode whether encoding succeeded. */
UA_StatusCode
UA_encodeBinaryInternal(const void *src, const UA_DataType *type,
                        UA_Byte **bufPos, const UA_Byte **bufEnd,
                        UA_EncodeBinaryOptions *options,
                        UA_exchangeEncodeBuffer exchangeCallback,
                        void *exchangeHandle)
    UA_INTERNAL_FUNC_ATTR_WARN_UNUSED_RESULT;

/* Decodes a scalar value described by type from binary encoding. Decoding is
 * reentrant and can be safely called from signal handlers or interrupts.
 *
 * @param src The buffer with the binary encoded value. Must not be NULL.
 * @param offset The current position in the buffer. Must not be NULL. The value
 *        is advanced as decoding progresses.
 * @param dst The target value. Must not be NULL. The target is assumed to have
 *        size type->memSize. The value is reset to zero before decoding. If
 *        decoding fails, members are deleted and the value is reset (zeroed)
 *        again.
 * @param type The value type. Must not be NULL.
 * @param customTypesSize The number of non-standard datatypes contained in the
 *        customTypes array.
 * @param customTypes An array of non-standard datatypes (not included in
 *        UA_TYPES). Can be NULL if customTypesSize is zero.
 * @return Returns a statuscode whether decoding succeeded. */
UA_StatusCode
UA_decodeBinaryInternal(const UA_ByteString *src, size_t *offset,
                        void *dst, const UA_DataType *type,
                        UA_DecodeBinaryOptions *options)
    UA_INTERNAL_FUNC_ATTR_WARN_UNUSED_RESULT;

const UA_DataType *
UA_findDataTypeByBinary(const UA_NodeId *typeId);

/******************/
/* JSON Encoding  */
/******************/

#ifdef UA_ENABLE_JSON_ENCODING

#define UA_JSON_MAXTOKENCOUNT 256
#define UA_JSON_ENCODING_MAX_RECURSION 100

typedef struct {
    uint8_t *pos;
    const uint8_t *end;

    uint16_t depth; /* How often did we en-/decoding recurse? */
    UA_Boolean commaNeeded[UA_JSON_ENCODING_MAX_RECURSION + 1];
    UA_Boolean calcOnly; /* Only compute the length of the decoding */

    UA_NamespaceMapping *namespaceMapping;

    size_t serverUrisSize;
    const UA_String *serverUris;

    UA_Boolean useCompactEncoding;
    UA_Boolean prettyPrint;
    UA_Boolean unquotedKeys;
} CtxJson;

UA_StatusCode
writeJsonObjStart(CtxJson *ctx);

UA_StatusCode
writeJsonObjElm(CtxJson *ctx, const char *key,
                const void *value, const UA_DataType *type);

UA_StatusCode
writeJsonObjEnd(CtxJson *ctx);

UA_StatusCode
writeJsonArrStart(CtxJson *ctx);

UA_StatusCode
writeJsonArrElm(CtxJson *ctx, const void *value,
                const UA_DataType *type);

UA_StatusCode
writeJsonArrEnd(CtxJson *ctx, const UA_DataType *type);

UA_StatusCode
writeJsonKey(CtxJson *ctx, const char* key);

/* Adds a comma if needed. Distinct elements go on a new line if pretty-printing
 * is enabled. */
UA_StatusCode
writeJsonBeforeElement(CtxJson *ctx, UA_Boolean distinct);

typedef struct {
    const char *json5;
    cj5_token *tokens;
    size_t tokensSize;
    size_t index;
    UA_Byte depth;

    UA_NamespaceMapping *namespaceMapping;

    size_t serverUrisSize;
    const UA_String *serverUris;

    const UA_DataTypeArray *customTypes;
} ParseCtx;

/* Expose the jump tables */
typedef UA_StatusCode
(*encodeJsonSignature)(CtxJson *ctx, const void *src, const UA_DataType *type);
typedef UA_StatusCode
(*decodeJsonSignature)(ParseCtx *ctx, void *dst, const UA_DataType *type);
extern const encodeJsonSignature encodeJsonJumpTable[UA_DATATYPEKINDS];
extern const decodeJsonSignature decodeJsonJumpTable[UA_DATATYPEKINDS];

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
decodeFields(ParseCtx *ctx, DecodeEntry *entries, size_t entryCount);

UA_StatusCode
decodeFieldsAllowUnknown(ParseCtx *ctx, DecodeEntry *entries,
                         size_t entryCount);

UA_StatusCode
lookAheadForKey(ParseCtx *ctx, const char *search, size_t *resultIndex);

UA_StatusCode
tokenize(ParseCtx *ctx, const UA_ByteString *src,
         size_t tokensSize, size_t *decodedLength);

static UA_INLINE
cj5_token_type currentTokenType(const ParseCtx *ctx) {
    return ctx->tokens[ctx->index].type;
}

static UA_INLINE
size_t getTokenLength(const cj5_token *t) {
    return (size_t)(1u + t->end - t->start);
}

#endif /* UA_ENABLE_JSON_ENCODING */

/****************/
/* XML Encoding */
/****************/

#ifdef UA_ENABLE_XML_ENCODING

#define UA_XML_MAXMEMBERSCOUNT 256
#define UA_XML_ENCODING_MAX_RECURSION 100

/* XML input gets parsed into a sequence of tokens first.
 * Processing isntructions, etc. get ignored. */

typedef enum {
    XML_TOKEN_ELEMENT = 0,
    XML_TOKEN_ATTRIBUTE,
} xml_token_type;

typedef struct {
    xml_token_type type;
    UA_String name;
    UA_String content;
    UA_Boolean contentEscaped; /* Content contains a character or entity reference */
    unsigned attributes;       /* For elements only: the number of attributes */
    unsigned children;         /* For elements only: the number of child elements */
    unsigned start;            /* First character of the token in the XML */
    unsigned end;              /* Position after the token ends */
} xml_token;

typedef enum {
    XML_ERROR_NONE = 0,
    XML_ERROR_INVALID,   /* Invalid character/syntax */
    XML_ERROR_OVERFLOW   /* Token buffer overflow */
} xml_error_code;

typedef struct {
    xml_error_code error;
    unsigned int error_pos;
    unsigned int num_tokens;
    const xml_token *tokens;
} xml_result;

/* Parse XML input into a token sequence */
xml_result
xml_tokenize(const char *xml, unsigned int len,
             xml_token *tokens, unsigned int max_tokens);

typedef struct {
    uint8_t *pos;
    const uint8_t *end;
    uint16_t depth; /* How often did we encoding recurse? */
    UA_Boolean calcOnly; /* Only compute the length of the decoding */
    const UA_DataTypeArray *customTypes;

    UA_NamespaceMapping *namespaceMapping;
    const UA_String *serverUris;
    size_t serverUrisSize;
} CtxXml;

typedef struct {
    size_t index;
    size_t depth;
    size_t tokensSize;
    xml_token *tokens;
    const UA_DataTypeArray *customTypes;
    const char *xml;

    UA_NamespaceMapping *namespaceMapping;
    const UA_String *serverUris;
    size_t serverUrisSize;
} ParseCtxXml;

/* Expose the jump tables */
typedef UA_StatusCode
(*encodeXmlSignature)(CtxXml *ctx, const void *src, const UA_DataType *type);
typedef UA_StatusCode
(*decodeXmlSignature)(ParseCtxXml *ctx, void *dst, const UA_DataType *type);
extern const encodeXmlSignature encodeXmlJumpTable[UA_DATATYPEKINDS];
extern const decodeXmlSignature decodeXmlJumpTable[UA_DATATYPEKINDS];

#endif /* UA_ENABLE_XML_ENCODING */

_UA_END_DECLS

#endif /* UA_TYPES_ENCODING_BINARY_H_ */
