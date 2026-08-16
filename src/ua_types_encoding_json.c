/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2018, 2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Lukas Meling)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

/**
 * This file contains the JSON encoding/decoding following the v1.05 OPC UA
 * specification. The changes in the v1.05 specification are breaking. The
 * encoding is not compatible with previous versions. */

#include <open62541/config.h>
#include <open62541/types.h>

#if defined(UA_ENABLE_JSON_ENCODING)

#include "ua_types_encoding_json.h"

#include <float.h>
#include <math.h>

#include "../deps/utf8.h"
#include "../deps/itoa.h"
#include "../deps/dtoa.h"
#include "../deps/parse_num.h"
#include "../deps/base64.h"
#include "../deps/libc_time.h"

#if defined(_MSC_VER)
# pragma warning(disable: 4756)
# pragma warning(disable: 4056)
#endif

/************/
/* Encoding */
/************/

static status
encodeJsonStructureContent(CtxJson *ctx, const void *src,
                           const UA_DataType *type);

static status
encodeJsonUnionContent(CtxJson *ctx, const void *src,
                       const UA_DataType *type);

static status
decodeJsonStructure(ParseCtx *ctx, void *dst, const UA_DataType *type);

static status
decodeJsonStructureExtensionObject(ParseCtx *ctx, void *dst,
                                   const UA_DataType *type);

static status
decodeJsonUnionExtensionObject(ParseCtx *ctx, void *dst,
                               const UA_DataType *type);

#define ENCODE_JSON(TYPE) static status \
    TYPE##_encodeJson(CtxJson *ctx, const void *p, const UA_DataType *type)

static status UA_INTERNAL_FUNC_ATTR_WARN_UNUSED_RESULT
writeChar(CtxJson *ctx, char c) {
    if(ctx->pos >= ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        *ctx->pos = (UA_Byte)c;
    ctx->pos++;
    return UA_STATUSCODE_GOOD;
}

static status UA_INTERNAL_FUNC_ATTR_WARN_UNUSED_RESULT
writeChars(CtxJson *ctx, const char *c, size_t len) {
    if(ctx->pos + len > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        memcpy(ctx->pos, c, len);
    ctx->pos += len;
    return UA_STATUSCODE_GOOD;
}

#define WRITE_JSON_ELEMENT(ELEM)                                     \
    UA_INTERNAL_FUNC_ATTR_WARN_UNUSED_RESULT status                  \
    writeJson##ELEM(CtxJson *ctx)

static WRITE_JSON_ELEMENT(Quote) {
    return writeChar(ctx, '\"');
}

UA_StatusCode
writeJsonBeforeElement(CtxJson *ctx, UA_Boolean distinct) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    /* Comma if needed */
    if(ctx->commaNeeded[ctx->depth])
        res |= writeChar(ctx, ',');
    if(ctx->prettyPrint) {
        if(distinct) {
            /* Newline and indent if needed */
            res |= writeChar(ctx, '\n');
            for(size_t i = 0; i < ctx->depth; i++)
                res |= writeChar(ctx, '\t');
        } else if(ctx->commaNeeded[ctx->depth]) {
            /* Space after the comma if no newline */
            res |= writeChar(ctx, ' ');
        }
    }
    return res;
}

WRITE_JSON_ELEMENT(ObjStart) {
    /* Increase depth, save: before first key-value no comma needed. */
    if(ctx->depth >= UA_JSON_ENCODING_MAX_RECURSION)
        return UA_STATUSCODE_BADENCODINGERROR;
    ctx->depth++;
    ctx->commaNeeded[ctx->depth] = false;
    return writeChar(ctx, '{');
}

WRITE_JSON_ELEMENT(ObjEnd) {
    if(ctx->depth == 0)
        return UA_STATUSCODE_BADENCODINGERROR;

    UA_Boolean have_elem = ctx->commaNeeded[ctx->depth];
    ctx->depth--;
    ctx->commaNeeded[ctx->depth] = true;

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(ctx->prettyPrint && have_elem) {
        res |= writeChar(ctx, '\n');
        for(size_t i = 0; i < ctx->depth; i++)
            res |= writeChar(ctx, '\t');
    }
    return res | writeChar(ctx, '}');
}

WRITE_JSON_ELEMENT(ArrStart) {
    /* Increase depth, save: before first array entry no comma needed. */
    if(ctx->depth >= UA_JSON_ENCODING_MAX_RECURSION)
        return UA_STATUSCODE_BADENCODINGERROR;
    ctx->depth++;
    ctx->commaNeeded[ctx->depth] = false;
    return writeChar(ctx, '[');
}

status
writeJsonArrEnd(CtxJson *ctx, const UA_DataType *type) {
    if(ctx->depth == 0)
        return UA_STATUSCODE_BADENCODINGERROR;
    UA_Boolean have_elem = ctx->commaNeeded[ctx->depth];
    ctx->depth--;
    ctx->commaNeeded[ctx->depth] = true;

    /* If the array does not contain JSON objects (with a newline after), then
     * add the closing ] on the same line */
    UA_Boolean distinct = (!type || type->typeKind > UA_DATATYPEKIND_DOUBLE);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(ctx->prettyPrint && have_elem && distinct) {
        res |= writeChar(ctx, '\n');
        for(size_t i = 0; i < ctx->depth; i++)
            res |= writeChar(ctx, '\t');
    }
    return res | writeChar(ctx, ']');
}

status
writeJsonArrElm(CtxJson *ctx, const void *value,
                const UA_DataType *type) {
    UA_Boolean distinct = (type->typeKind > UA_DATATYPEKIND_DOUBLE);
    status ret = writeJsonBeforeElement(ctx, distinct);
    ctx->commaNeeded[ctx->depth] = true;
    return ret | encodeJsonJumpTable[type->typeKind](ctx, value, type);
}

status
writeJsonObjElm(CtxJson *ctx, const char *key,
                const void *value, const UA_DataType *type) {
    status ret = writeJsonKey(ctx, key);
    return ret | encodeJsonJumpTable[type->typeKind](ctx, value, type);
}

/* LocalizedText */
static const char* UA_JSONKEY_LOCALE = "Locale";
static const char* UA_JSONKEY_TEXT = "Text";

/* Variant */
static const char* UA_JSONKEY_TYPE = "UaType";
static const char* UA_JSONKEY_VALUE = "Value";
static const char* UA_JSONKEY_DIMENSIONS = "Dimensions";

/* DataValue */
static const char* UA_JSONKEY_STATUS = "Status";
static const char* UA_JSONKEY_SOURCETIMESTAMP = "SourceTimestamp";
static const char* UA_JSONKEY_SOURCEPICOSECONDS = "SourcePicoseconds";
static const char* UA_JSONKEY_SERVERTIMESTAMP = "ServerTimestamp";
static const char* UA_JSONKEY_SERVERPICOSECONDS = "ServerPicoseconds";

/* ExtensionObject */
static const char* UA_JSONKEY_ENCODING = "UaEncoding";
static const char* UA_JSONKEY_TYPEID = "UaTypeId";
static const char* UA_JSONKEY_BODY = "UaBody";

/* Structures and Unions */
static const char* UA_JSONKEY_ENCODINGMASK = "EncodingMask";
static const char* UA_JSONKEY_SWITCHFIELD = "SwitchField";

/* StatusCode */
static const char* UA_JSONKEY_CODE = "Code";
static const char* UA_JSONKEY_SYMBOL = "Symbol";

/* DiagnosticInfo */
static const char* UA_JSONKEY_SYMBOLICID = "SymbolicId";
static const char* UA_JSONKEY_NAMESPACEURI = "NamespaceUri";
static const char* UA_JSONKEY_LOCALIZEDTEXT = "LocalizedText";
static const char* UA_JSONKEY_ADDITIONALINFO = "AdditionalInfo";
static const char* UA_JSONKEY_INNERSTATUSCODE = "InnerStatusCode";
static const char* UA_JSONKEY_INNERDIAGNOSTICINFO = "InnerDiagnosticInfo";

/* Writes null terminated string to output buffer (current ctx->pos). Writes
 * comma in front of key if needed. Encapsulates key in quotes. */
status UA_INTERNAL_FUNC_ATTR_WARN_UNUSED_RESULT
writeJsonKey(CtxJson *ctx, const char* key) {
    status ret = writeJsonBeforeElement(ctx, true);
    ctx->commaNeeded[ctx->depth] = true;
    if(!ctx->unquotedKeys)
        ret |= writeChar(ctx, '\"');
    ret |= writeChars(ctx, key, strlen(key));
    if(!ctx->unquotedKeys)
        ret |= writeChar(ctx, '\"');
    ret |= writeChar(ctx, ':');
    if(ctx->prettyPrint)
        ret |= writeChar(ctx, ' ');
    return ret;
}

static UA_Boolean
isJsonNullable(const UA_DataType *type) {
    return (type->typeKind >= UA_DATATYPEKIND_STRING &&
            type->typeKind <= UA_DATATYPEKIND_DIAGNOSTICINFO &&
            type->typeKind != UA_DATATYPEKIND_STATUSCODE);
}

static bool
isNull(const void *p, const UA_DataType *type) {
    if(!isJsonNullable(type))
        return false;
    UA_STACKARRAY(char, buf, type->memSize);
    memset(buf, 0, type->memSize);
    return UA_equal(buf, p, type);
}

ENCODE_JSON(Boolean) {
    const UA_Boolean *src = (const UA_Boolean*)p;
    if(*src == true)
        return writeChars(ctx, "true", 4);
    return writeChars(ctx, "false", 5);
}

ENCODE_JSON(Byte) {
    const UA_Byte *src = (const UA_Byte*)p;
    char buf[4];
    UA_UInt16 digits = itoaUnsigned(*src, buf, 10);

    /* Ensure destination can hold the data- */
    if(ctx->pos + digits > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;

    /* Copy digits to the output string/buffer. */
    if(!ctx->calcOnly)
        memcpy(ctx->pos, buf, digits);
    ctx->pos += digits;
    return UA_STATUSCODE_GOOD;
}

ENCODE_JSON(SByte) {
    const UA_SByte *src = (const UA_SByte*)p;
    char buf[5];
    UA_UInt16 digits = itoaSigned(*src, buf);
    if(ctx->pos + digits > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        memcpy(ctx->pos, buf, digits);
    ctx->pos += digits;
    return UA_STATUSCODE_GOOD;
}

ENCODE_JSON(UInt16) {
    const UA_UInt16 *src = (const UA_UInt16*)p;
    char buf[6];
    UA_UInt16 digits = itoaUnsigned(*src, buf, 10);
    if(ctx->pos + digits > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        memcpy(ctx->pos, buf, digits);
    ctx->pos += digits;
    return UA_STATUSCODE_GOOD;
}

ENCODE_JSON(Int16) {
    const UA_Int16 *src = (const UA_Int16*)p;
    char buf[7];
    UA_UInt16 digits = itoaSigned(*src, buf);
    if(ctx->pos + digits > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        memcpy(ctx->pos, buf, digits);
    ctx->pos += digits;
    return UA_STATUSCODE_GOOD;
}

ENCODE_JSON(UInt32) {
    const UA_UInt32 *src = (const UA_UInt32*)p;
    char buf[11];
    UA_UInt16 digits = itoaUnsigned(*src, buf, 10);
    if(ctx->pos + digits > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        memcpy(ctx->pos, buf, digits);
    ctx->pos += digits;
    return UA_STATUSCODE_GOOD;
}

ENCODE_JSON(Int32) {
    const UA_Int32 *src = (const UA_Int32*)p;
    char buf[12];
    UA_UInt16 digits = itoaSigned(*src, buf);
    if(ctx->pos + digits > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        memcpy(ctx->pos, buf, digits);
    ctx->pos += digits;
    return UA_STATUSCODE_GOOD;
}

ENCODE_JSON(UInt64) {
    const UA_UInt64 *src = (const UA_UInt64*)p;
    char buf[23];
    buf[0] = '\"';
    UA_UInt16 digits = itoaUnsigned(*src, buf + 1, 10);
    buf[digits + 1] = '\"';
    UA_UInt16 length = (UA_UInt16)(digits + 2);
    if(ctx->pos + length > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        memcpy(ctx->pos, buf, length);
    ctx->pos += length;
    return UA_STATUSCODE_GOOD;
}

ENCODE_JSON(Int64) {
    const UA_Int64 *src = (const UA_Int64*)p;
    char buf[23];
    buf[0] = '\"';
    UA_UInt16 digits = itoaSigned(*src, buf + 1);
    buf[digits + 1] = '\"';
    UA_UInt16 length = (UA_UInt16)(digits + 2);
    if(ctx->pos + length > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        memcpy(ctx->pos, buf, length);
    ctx->pos += length;
    return UA_STATUSCODE_GOOD;
}

ENCODE_JSON(Float) {
    const UA_Float *src = (const UA_Float*)p;
    char buffer[32];
    size_t len;
    if(*src != *src)
        return writeChars(ctx, "\"NaN\"", 5);
    if(*src == INFINITY)
        return writeChars(ctx, "\"Infinity\"", 10);
    if(*src == -INFINITY)
        return writeChars(ctx, "\"-Infinity\"", 11);
    len = dtoa((UA_Double)*src, buffer);
    if(ctx->pos + len > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        memcpy(ctx->pos, buffer, len);
    ctx->pos += len;
    return UA_STATUSCODE_GOOD;
}

ENCODE_JSON(Double) {
    const UA_Double *src = (const UA_Double*)p;
    char buffer[32];
    size_t len;
    if(*src != *src)
        return writeChars(ctx, "\"NaN\"", 5);
    if(*src == INFINITY)
        return writeChars(ctx, "\"Infinity\"", 10);
    if(*src == -INFINITY)
        return writeChars(ctx, "\"-Infinity\"", 11);
    len = dtoa(*src, buffer);
    if(ctx->pos + len > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        memcpy(ctx->pos, buffer, len);
    ctx->pos += len;
    return UA_STATUSCODE_GOOD;
}

static status
encodeJsonArray(CtxJson *ctx, const void *ptr, size_t length,
                const UA_DataType *type) {
    /* Null-arrays (length -1) are written as empty arrays '[]'.
     * TODO: Clarify the difference between length -1 and length 0 in JSON. */
    status ret = writeJsonArrStart(ctx);
    if(!ptr)
        return ret | writeJsonArrEnd(ctx, type);

    uintptr_t uptr = (uintptr_t)ptr;
    encodeJsonSignature encodeType = encodeJsonJumpTable[type->typeKind];
    UA_Boolean distinct = (type->typeKind > UA_DATATYPEKIND_DOUBLE);
    for(size_t i = 0; i < length && ret == UA_STATUSCODE_GOOD; ++i) {
        ret |= writeJsonBeforeElement(ctx, distinct);
        if(isNull((const void*)uptr, type))
            ret |= writeChars(ctx, "null", 4);
        else
            ret |= encodeType(ctx, (const void*)uptr, type);
        ctx->commaNeeded[ctx->depth] = true;
        uptr += type->memSize;
    }
    return ret | writeJsonArrEnd(ctx, type);
}

static const char hexmap[16] =
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

ENCODE_JSON(String) {
    const UA_String *src = (const UA_String*)p;
    if(!src->data)
        return writeChars(ctx, "null", 4);

    status ret = UA_STATUSCODE_GOOD;
    if(src->length == 0) {
        ret |= writeJsonQuote(ctx);
        ret |= writeJsonQuote(ctx);
        return ret;
    }

    ret |= writeJsonQuote(ctx);

    const unsigned char *end = src->data + src->length;
    for(const unsigned char *pos = src->data; pos < end; pos++) {
        /* Skip to the first character that needs escaping */
        const unsigned char *start = pos;
        for(; pos < end; pos++) {
            if(*pos < ' ' || *pos == 127 || *pos == '\\' || *pos == '\"')
                break;
        }

        /* Write out the unescaped sequence */
        if(ctx->pos + (pos - start) > ctx->end)
            return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
        if(!ctx->calcOnly)
            memcpy(ctx->pos, start, (size_t)(pos - start));
        ctx->pos += pos - start;

        /* The unescaped sequence reached the end */
        if(pos == end)
            break;

        /* Write an escaped character */
        char *escape_text;
        char escape_buf[6];
        size_t escape_len = 2;
        switch(*pos) {
        case '\b': escape_text = "\\b"; break;
        case '\f': escape_text = "\\f"; break;
        case '\n': escape_text = "\\n"; break;
        case '\r': escape_text = "\\r"; break;
        case '\t': escape_text = "\\t"; break;
        default:
            escape_text = escape_buf;
            if(*pos >= ' ' && *pos != 127) {
                /* Escape \ or " */
                escape_buf[0] = '\\';
                escape_buf[1] = (char)*pos;
            } else {
                /* Unprintable characters need to be escaped */
                escape_buf[0] = '\\';
                escape_buf[1] = 'u';
                escape_buf[2] = '0';
                escape_buf[3] = '0';
                escape_buf[4] = hexmap[*pos >> 4];
                escape_buf[5] = hexmap[*pos & 0x0f];
                escape_len = 6;
            }
            break;
        }

        /* Enough space? */
        if(ctx->pos + escape_len > ctx->end)
            return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;

        /* Write the escaped character */
        if(!ctx->calcOnly)
            memcpy(ctx->pos, escape_text, escape_len);
        ctx->pos += escape_len;
    }

    return ret | writeJsonQuote(ctx);
}

ENCODE_JSON(ByteString) {
    const UA_ByteString *src = (const UA_ByteString*)p;
    if(!src->data)
        return writeChars(ctx, "null", 4);

    if(src->length == 0) {
        status retval = writeJsonQuote(ctx);
        retval |= writeJsonQuote(ctx);
        return retval;
    }

    status ret = writeJsonQuote(ctx);
    size_t flen = 0;
    unsigned char *ba64 = UA_base64(src->data, src->length, &flen);

    /* Not converted, no mem */
    if(!ba64)
        return UA_STATUSCODE_BADENCODINGERROR;

    if(ctx->pos + flen > ctx->end) {
        UA_free(ba64);
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    }

    /* Copy flen bytes to output stream. */
    if(!ctx->calcOnly)
        memcpy(ctx->pos, ba64, flen);
    ctx->pos += flen;

    /* Base64 result no longer needed */
    UA_free(ba64);

    return ret | writeJsonQuote(ctx);
}

ENCODE_JSON(Guid) {
    const UA_Guid *src = (const UA_Guid*)p;
    if(ctx->pos + 38 > ctx->end) /* 36 + 2 (") */
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    status ret = writeJsonQuote(ctx);
    if(!ctx->calcOnly)
        UA_Guid_to_hex(src, ctx->pos, false);
    ctx->pos += 36;
    return ret | writeJsonQuote(ctx);
}

ENCODE_JSON(DateTime) {
    const UA_DateTime *src = (const UA_DateTime*)p;
    if(*src == 0)
        return writeChars(ctx, "\"0001-01-01T00:00:00Z\"", 22);

    UA_DateTimeStruct date = UA_DateTime_toStruct(*src);
    if(date.year < 1)
        return writeChars(ctx, "\"0001-01-01T00:00:00Z\"", 22);
    if(date.year > 9999)
        return writeChars(ctx, "\"9999-12-31T23:59:59.9999999Z\"", 30);

    UA_Byte buffer[40];
    UA_String str = {40, buffer};
    encodeDateTime(*src, &str);
    return String_encodeJson(ctx, &str, NULL);
}

ENCODE_JSON(NodeId) {
    const UA_NodeId *src = (const UA_NodeId*)p;
    UA_String out = UA_STRING_NULL;
    UA_StatusCode ret =
        UA_NodeId_printEx(src, &out, ctx->namespaceMapping);
    ret |= String_encodeJson(ctx, &out, NULL);
    UA_String_clear(&out);
    return ret;
}

ENCODE_JSON(ExpandedNodeId) {
    const UA_ExpandedNodeId *src = (const UA_ExpandedNodeId*)p;
    UA_String out = UA_STRING_NULL;
    UA_StatusCode ret =
        UA_ExpandedNodeId_printEx(src, &out, ctx->namespaceMapping,
                                  ctx->serverUrisSize, ctx->serverUris);
    ret |= String_encodeJson(ctx, &out, NULL);
    UA_String_clear(&out);
    return ret;
}

ENCODE_JSON(LocalizedText) {
    const UA_LocalizedText *src = (const UA_LocalizedText*)p;
    status ret = writeJsonObjStart(ctx);
    if(src->locale.length > 0) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_LOCALE);
        ret |= String_encodeJson(ctx, &src->locale, NULL);
    }
    if(src->text.length > 0) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_TEXT);
        ret |= String_encodeJson(ctx, &src->text, NULL);
    }
    return ret | writeJsonObjEnd(ctx);
}

ENCODE_JSON(QualifiedName) {
    const UA_QualifiedName *src = (const UA_QualifiedName*)p;
    if(src->namespaceIndex == 0 && src->name.data == NULL)
        return writeChars(ctx, "null", 4);
    UA_String out = UA_STRING_NULL;
    UA_StatusCode ret =
        UA_QualifiedName_printEx(src, &out, ctx->namespaceMapping);
    ret |= String_encodeJson(ctx, &out, NULL);
    UA_String_clear(&out);
    return ret;
}

ENCODE_JSON(StatusCode) {
    const UA_StatusCode *src = (const UA_StatusCode*)p;
    const char *codename = UA_StatusCode_name(*src);
    UA_String statusDescription = UA_STRING((char*)(uintptr_t)codename);
    status ret = UA_STATUSCODE_GOOD;
    ret |= writeJsonObjStart(ctx);
    if(*src > UA_STATUSCODE_GOOD) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_CODE);
        ret |= UInt32_encodeJson(ctx, src, NULL);
        if(codename && codename[0] != '\0') {
            ret |= writeJsonKey(ctx, UA_JSONKEY_SYMBOL);
            ret |= String_encodeJson(ctx, &statusDescription, NULL);
        }
    }
    ret |= writeJsonObjEnd(ctx);
    return ret;
}

ENCODE_JSON(ExtensionObject) {
    const UA_ExtensionObject *src = (const UA_ExtensionObject*)p;
    if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY)
        return writeChars(ctx, "{}", 2);

    /* Unknown JSON datatypes retain their complete wire representation. */
    if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_JSON) {
        if(src->content.encoded.body.length < 2 ||
           src->content.encoded.body.data[0] != '{' ||
           src->content.encoded.body.data[src->content.encoded.body.length - 1] != '}')
            return UA_STATUSCODE_BADENCODINGERROR;
        return writeChars(ctx, (const char*)src->content.encoded.body.data,
                          src->content.encoded.body.length);
    }

    /* Must have a type set if data is decoded */
    if(src->encoding != UA_EXTENSIONOBJECT_ENCODED_BYTESTRING &&
       src->encoding != UA_EXTENSIONOBJECT_ENCODED_XML &&
       !src->content.decoded.type)
        return UA_STATUSCODE_BADENCODINGERROR;

    status ret = writeJsonObjStart(ctx);

    /* Write the type NodeId */
    ret |= writeJsonKey(ctx, UA_JSONKEY_TYPEID);
    if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING ||
       src->encoding == UA_EXTENSIONOBJECT_ENCODED_XML)
        ret |= NodeId_encodeJson(ctx, &src->content.encoded.typeId, NULL);
    else
        ret |= NodeId_encodeJson(ctx, &src->content.decoded.type->typeId, NULL);

    /* Write the encoding type and body if encoded */
    if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING ||
       src->encoding == UA_EXTENSIONOBJECT_ENCODED_XML) {
        if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING) {
            ret |= writeJsonKey(ctx, UA_JSONKEY_ENCODING);
            ret |= writeChar(ctx, '1');
        } else {
            ret |= writeJsonKey(ctx, UA_JSONKEY_ENCODING);
            ret |= writeChar(ctx, '2');
        }
        ret |= writeJsonKey(ctx, UA_JSONKEY_BODY);
        ret |= ByteString_encodeJson(ctx, &src->content.encoded.body, NULL);
        return ret | writeJsonObjEnd(ctx);
    }

    const UA_DataType *t = src->content.decoded.type;
    if(t->typeKind == UA_DATATYPEKIND_STRUCTURE ||
       t->typeKind == UA_DATATYPEKIND_OPTSTRUCT) {
        /* Write structures in-situ. */
        ret |= encodeJsonStructureContent(ctx, src->content.decoded.data, t);
    } else if(t->typeKind == UA_DATATYPEKIND_UNION) {
        ret |= encodeJsonUnionContent(ctx, src->content.decoded.data, t);
    } else {
        /* NON-STANDARD: The standard 1.05 doesn't let us print non-structure
         * types in ExtensionObjects (e.g. enums). Print them in the body. */
        ret |= writeJsonKey(ctx, UA_JSONKEY_BODY);
        ret |= encodeJsonJumpTable[t->typeKind](ctx, src->content.decoded.data, t);
    }

    return ret | writeJsonObjEnd(ctx);
}

/* Non-builtin types get wrapped in an ExtensionObject */
static status
encodeScalarJsonWrapExtensionObject(CtxJson *ctx, const UA_Variant *src) {
    const UA_Boolean isBuiltin =
        (src->type->typeKind <= UA_DATATYPEKIND_DIAGNOSTICINFO);
    const void *ptr = src->data;
    const UA_DataType *type = src->type;

    /* Set up a temporary ExtensionObject to wrap the data */
    UA_ExtensionObject eo;
    if(!isBuiltin) {
        UA_ExtensionObject_init(&eo);
        eo.encoding = UA_EXTENSIONOBJECT_DECODED;
        eo.content.decoded.type = src->type;
        eo.content.decoded.data = src->data;
        ptr = &eo;
        type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
    }

    return encodeJsonJumpTable[type->typeKind](ctx, ptr, type);
}

/* Non-builtin types get wrapped in an ExtensionObject */
static status
encodeArrayJsonWrapExtensionObject(CtxJson *ctx, const void *data,
                                   size_t size, const UA_DataType *type) {
    if(size > UA_INT32_MAX)
        return UA_STATUSCODE_BADENCODINGERROR;

    status ret = writeJsonArrStart(ctx);

    u16 memSize = type->memSize;
    const UA_Boolean isBuiltin =
        (type->typeKind <= UA_DATATYPEKIND_DIAGNOSTICINFO);
    if(isBuiltin) {
        uintptr_t ptr = (uintptr_t)data;
        for(size_t i = 0; i < size && ret == UA_STATUSCODE_GOOD; ++i) {
            ret |= writeJsonArrElm(ctx, (const void*)ptr, type);
            ptr += memSize;
        }
    } else {
        /* Set up a temporary ExtensionObject to wrap the data */
        UA_ExtensionObject eo;
        UA_ExtensionObject_init(&eo);
        eo.encoding = UA_EXTENSIONOBJECT_DECODED;
        eo.content.decoded.type = type;
        eo.content.decoded.data = (void*)(uintptr_t)data;
        for(size_t i = 0; i < size && ret == UA_STATUSCODE_GOOD; ++i) {
            ret |= writeJsonArrElm(ctx, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
            eo.content.decoded.data = (void*)
                ((uintptr_t)eo.content.decoded.data + memSize);
        }
    }

    return ret | writeJsonArrEnd(ctx, type);
}

static UA_Boolean
variantDimensionsValid(size_t arrayLength, size_t dimensionsSize,
                       const UA_UInt32 *dimensions) {
    if(dimensionsSize == 0 || !dimensions)
        return false;

    size_t total = 1;
    for(size_t i = 0; i < dimensionsSize; i++) {
        UA_UInt32 dimension = dimensions[i];
        if(dimension == 0 || total > SIZE_MAX / dimension)
            return false;
        total *= dimension;
    }
    return total == arrayLength;
}

static UA_StatusCode
encodeVariantInner(CtxJson *ctx, const UA_Variant *src,
                   UA_Boolean insideDataValue) {
    /* If type is 0 (NULL) the Variant contains a NULL value and the containing
     * JSON object shall be omitted or replaced by the JSON literal ‘null’ (when
     * an element of a JSON array). */
    if(!src->type)
        return UA_STATUSCODE_GOOD;

    /* These combinations are excluded by the Variant definition. */
    if(src->type->typeKind == UA_DATATYPEKIND_DIAGNOSTICINFO ||
       (insideDataValue && src->type->typeKind == UA_DATATYPEKIND_DATAVALUE))
        return UA_STATUSCODE_BADENCODINGERROR;

    /* Set the array type in the encoding mask */
    const bool isArray = src->arrayLength > 0 || src->data <= UA_EMPTY_ARRAY_SENTINEL;
    const bool hasDimensions = isArray && src->arrayDimensionsSize > 1;
    if(src->type->typeKind == UA_DATATYPEKIND_VARIANT && !isArray)
        return UA_STATUSCODE_BADENCODINGERROR;
    if(!isArray && src->arrayDimensionsSize > 0)
        return UA_STATUSCODE_BADENCODINGERROR;
    if(hasDimensions &&
       !variantDimensionsValid(src->arrayLength, src->arrayDimensionsSize,
                               src->arrayDimensions))
        return UA_STATUSCODE_BADENCODINGERROR;

    /* Enumerations lose their concrete type in a Variant and are represented
     * as Int32 in both the Compact and Verbose encodings. */
    const UA_DataType *valueType = src->type;
    if(valueType->typeKind == UA_DATATYPEKIND_ENUM)
        valueType = &UA_TYPES[UA_TYPES_INT32];

    /* Non-builtin values are wrapped in ExtensionObjects. */
    UA_Boolean wrapEO =
        (valueType->typeKind > UA_DATATYPEKIND_DIAGNOSTICINFO);

    status ret = UA_STATUSCODE_GOOD;

    /* Write the type number */
    UA_UInt32 typeId = valueType->typeKind + 1;
    if(wrapEO)
        typeId = UA_TYPES[UA_TYPES_EXTENSIONOBJECT].typeKind + 1;
    ret |= writeJsonKey(ctx, UA_JSONKEY_TYPE);
    ret |= UInt32_encodeJson(ctx, &typeId, NULL);

    /* A nullable scalar with its default value has no Value field. */
    UA_Boolean nullValue = false;
    if(!isArray && isJsonNullable(valueType))
        nullValue = (!src->data || isNull(src->data, valueType));

    if(!nullValue) {
        if(!src->data && !isArray)
            return UA_STATUSCODE_BADENCODINGERROR;
        ret |= writeJsonKey(ctx, UA_JSONKEY_VALUE);
        if(!isArray) {
            UA_Variant value = *src;
            value.type = valueType;
            ret |= encodeScalarJsonWrapExtensionObject(ctx, &value);
        } else {
            ret |= encodeArrayJsonWrapExtensionObject(ctx, src->data,
                                                      src->arrayLength, valueType);
        }
    }

    /* Write the dimensions */
    if(hasDimensions) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_DIMENSIONS);
        ret |= encodeJsonArray(ctx, src->arrayDimensions, src->arrayDimensionsSize,
                               &UA_TYPES[UA_TYPES_UINT32]);
    }

    return ret;
}

ENCODE_JSON(Variant) {
    const UA_Variant *src = (const UA_Variant*)p;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= writeJsonObjStart(ctx);
    res |= encodeVariantInner(ctx, src, false);
    res |= writeJsonObjEnd(ctx);
    return res;
}

ENCODE_JSON(DataValue) {
    const UA_DataValue *src = (const UA_DataValue*)p;
    UA_Boolean hasValue = src->hasValue && src->value.type;
    UA_Boolean hasStatus = src->hasStatus && src->status != UA_STATUSCODE_GOOD;
    UA_Boolean hasSourceTimestamp =
        src->hasSourceTimestamp && src->sourceTimestamp != 0;
    UA_Boolean hasSourcePicoseconds =
        src->hasSourcePicoseconds && src->sourcePicoseconds != 0;
    UA_Boolean hasServerTimestamp =
        src->hasServerTimestamp && src->serverTimestamp != 0;
    UA_Boolean hasServerPicoseconds =
        src->hasServerPicoseconds && src->serverPicoseconds != 0;

    status ret = writeJsonObjStart(ctx);

    if(hasValue)
        ret |= encodeVariantInner(ctx, &src->value, true);

    if(hasStatus) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_STATUS);
        ret |= StatusCode_encodeJson(ctx, &src->status, NULL);
    }

    if(hasSourceTimestamp) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_SOURCETIMESTAMP);
        ret |= DateTime_encodeJson(ctx, &src->sourceTimestamp, NULL);
    }

    if(hasSourcePicoseconds) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_SOURCEPICOSECONDS);
        ret |= UInt16_encodeJson(ctx, &src->sourcePicoseconds, NULL);
    }

    if(hasServerTimestamp) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_SERVERTIMESTAMP);
        ret |= DateTime_encodeJson(ctx, &src->serverTimestamp, NULL);
    }

    if(hasServerPicoseconds) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_SERVERPICOSECONDS);
        ret |= UInt16_encodeJson(ctx, &src->serverPicoseconds, NULL);
    }

    return ret | writeJsonObjEnd(ctx);
}

static UA_Boolean
diagnosticInfoHasContent(const UA_DiagnosticInfo *src, size_t depth) {
    if((src->hasSymbolicId && src->symbolicId != -1) ||
       (src->hasNamespaceUri && src->namespaceUri != -1) ||
       (src->hasLocalizedText && src->localizedText != -1) ||
       (src->hasLocale && src->locale != -1) ||
       (src->hasAdditionalInfo && src->additionalInfo.length > 0) ||
       (src->hasInnerStatusCode &&
        src->innerStatusCode != UA_STATUSCODE_GOOD))
        return true;
    if(depth >= UA_JSON_ENCODING_MAX_RECURSION)
        return true;
    return (src->hasInnerDiagnosticInfo && src->innerDiagnosticInfo &&
            diagnosticInfoHasContent(src->innerDiagnosticInfo, depth + 1));
}

ENCODE_JSON(DiagnosticInfo) {
    const UA_DiagnosticInfo *src = (const UA_DiagnosticInfo*)p;
    status ret = writeJsonObjStart(ctx);

    if(src->hasSymbolicId && src->symbolicId != -1) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_SYMBOLICID);
        ret |= Int32_encodeJson(ctx, &src->symbolicId, NULL);
    }

    if(src->hasNamespaceUri && src->namespaceUri != -1) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_NAMESPACEURI);
        ret |= Int32_encodeJson(ctx, &src->namespaceUri, NULL);
    }

    if(src->hasLocalizedText && src->localizedText != -1) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_LOCALIZEDTEXT);
        ret |= Int32_encodeJson(ctx, &src->localizedText, NULL);
    }

    if(src->hasLocale && src->locale != -1) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_LOCALE);
        ret |= Int32_encodeJson(ctx, &src->locale, NULL);
    }

    if(src->hasAdditionalInfo && src->additionalInfo.length > 0) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_ADDITIONALINFO);
        ret |= String_encodeJson(ctx, &src->additionalInfo, NULL);
    }

    if(src->hasInnerStatusCode &&
       src->innerStatusCode != UA_STATUSCODE_GOOD) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_INNERSTATUSCODE);
        ret |= StatusCode_encodeJson(ctx, &src->innerStatusCode, NULL);
    }

    if(src->hasInnerDiagnosticInfo && src->innerDiagnosticInfo &&
       diagnosticInfoHasContent(src->innerDiagnosticInfo, 0)) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_INNERDIAGNOSTICINFO);
        ret |= DiagnosticInfo_encodeJson(ctx, src->innerDiagnosticInfo, NULL);
    }

    return ret | writeJsonObjEnd(ctx);
}

static status
encodeJsonStructureContent(CtxJson *ctx, const void *src,
                           const UA_DataType *type) {
    uintptr_t ptr = (uintptr_t) src;
    u8 membersSize = type->membersSize;
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < membersSize && ret == UA_STATUSCODE_GOOD; ++i) {
        const UA_DataTypeMember *m = &type->members[i];
        const UA_DataType *mt = m->memberType;
        if(m->memberName == NULL)
            return UA_STATUSCODE_BADENCODINGERROR;
        ptr += m->padding;

        if(m->isArray) {
            const size_t length = *(const size_t*)ptr;
            ptr += sizeof(size_t);
            const void *data = *(void *const *)ptr;
            ptr += sizeof(void*);
            if(m->isOptional && !data)
                continue;
            ret |= writeJsonKey(ctx, m->memberName);
            ret |= encodeJsonArray(ctx, data, length, mt);
            continue;
        }

        if(m->isOptional) {
            const void *data = *(void *const *)ptr;
            ptr += sizeof(void*);
            if(!data)
                continue;
            ret |= writeJsonKey(ctx, m->memberName);
            ret |= encodeJsonJumpTable[mt->typeKind](ctx, data, mt);
            continue;
        }

        ret |= writeJsonKey(ctx, m->memberName);
        ret |= encodeJsonJumpTable[mt->typeKind](ctx, (const void*)ptr, mt);
        ptr += mt->memSize;
    }
    return ret;
}

static status
encodeJsonUnionContent(CtxJson *ctx, const void *src,
                       const UA_DataType *type) {
    const UA_UInt32 selection = *(const UA_UInt32*)src;
    if(selection == 0)
        return UA_STATUSCODE_GOOD;
    if(selection > type->membersSize)
        return UA_STATUSCODE_BADENCODINGERROR;

    const UA_DataTypeMember *m = &type->members[selection - 1];
    if(!m->memberName)
        return UA_STATUSCODE_BADENCODINGERROR;
    const UA_DataType *mt = m->memberType;
    uintptr_t ptr = (uintptr_t)src + m->padding;
    status ret = writeJsonKey(ctx, m->memberName);
    if(!m->isArray)
        return ret | encodeJsonJumpTable[mt->typeKind](ctx, (const void*)ptr, mt);

    const size_t length = *(const size_t*)ptr;
    ptr += sizeof(size_t);
    return ret | encodeJsonArray(ctx, *(void *const *)ptr, length, mt);
}

static status
encodeJsonStructure(CtxJson *ctx, const void *src, const UA_DataType *type) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= writeJsonObjStart(ctx);
    res |= encodeJsonStructureContent(ctx, src, type);
    res |= writeJsonObjEnd(ctx);
    return res;
}

static status
encodeJsonUnion(CtxJson *ctx, const void *src, const UA_DataType *type) {
    status ret = writeJsonObjStart(ctx);
    ret |= encodeJsonUnionContent(ctx, src, type);
    return ret | writeJsonObjEnd(ctx);
}

static status
encodeJsonNotImplemented(CtxJson *ctx, const void *src, const UA_DataType *type) {
    (void) src, (void) type, (void)ctx;
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

const encodeJsonSignature encodeJsonJumpTable[UA_DATATYPEKINDS] = {
    Boolean_encodeJson,
    SByte_encodeJson, /* SByte */
    Byte_encodeJson,
    Int16_encodeJson, /* Int16 */
    UInt16_encodeJson,
    Int32_encodeJson, /* Int32 */
    UInt32_encodeJson,
    Int64_encodeJson, /* Int64 */
    UInt64_encodeJson,
    Float_encodeJson,
    Double_encodeJson,
    String_encodeJson,
    DateTime_encodeJson, /* DateTime */
    Guid_encodeJson,
    ByteString_encodeJson, /* ByteString */
    String_encodeJson, /* XmlElement */
    NodeId_encodeJson,
    ExpandedNodeId_encodeJson,
    StatusCode_encodeJson, /* StatusCode */
    QualifiedName_encodeJson, /* QualifiedName */
    LocalizedText_encodeJson,
    ExtensionObject_encodeJson,
    DataValue_encodeJson,
    Variant_encodeJson,
    DiagnosticInfo_encodeJson,
    encodeJsonNotImplemented, /* Decimal */
    Int32_encodeJson, /* Enum */
    encodeJsonStructure,
    encodeJsonStructure, /* Structure with optional fields */
    encodeJsonUnion,
    encodeJsonNotImplemented /* BitfieldCluster */
};

UA_StatusCode
UA_encodeJson(const void *src, const UA_DataType *type,
              UA_ByteString *outBuf,
              const UA_EncodeJsonOptions *options) {
    if(!src || !type)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate buffer */
    UA_Boolean allocated = false;
    status res = UA_STATUSCODE_GOOD;
    if(outBuf->length == 0) {
        size_t len = UA_calcSizeJson(src, type, options);
        res = UA_ByteString_allocBuffer(outBuf, len);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        allocated = true;
    }

    /* Set up the context */
    CtxJson ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.pos = outBuf->data;
    ctx.end = &outBuf->data[outBuf->length];
    ctx.depth = 0;
    ctx.calcOnly = false;
    if(options) {
        ctx.namespaceMapping = options->namespaceMapping;
        ctx.serverUris = options->serverUris;
        ctx.serverUrisSize = options->serverUrisSize;
        ctx.prettyPrint = options->prettyPrint;
        ctx.unquotedKeys = options->unquotedKeys;
    }

    /* Encode */
    res = encodeJsonJumpTable[type->typeKind](&ctx, src, type);

    /* Clean up */
    if(res == UA_STATUSCODE_GOOD)
        outBuf->length = (size_t)((uintptr_t)ctx.pos - (uintptr_t)outBuf->data);
    else if(allocated)
        UA_ByteString_clear(outBuf);
    return res;
}

UA_StatusCode
UA_print(const void *p, const UA_DataType *type, UA_String *output) {
    if(!p || !type || !output)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_EncodeJsonOptions options;
    memset(&options, 0, sizeof(UA_EncodeJsonOptions));
    options.prettyPrint = true;
    options.unquotedKeys = true;
    return UA_encodeJson(p, type, output, &options);
}

/************/
/* CalcSize */
/************/

/* _calcSizeBinary reuses the encoding code path. It sets the end position to
 * SIZE_MAX to indicate that no bytes are ever written. We use 0x01 as the
 * starting position to avoid UB warnings for adding to a NULL pointer. */
size_t
UA_calcSizeJson(const void *src, const UA_DataType *type,
                const UA_EncodeJsonOptions *options) {
    if(!src || !type)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Set up the context */
    CtxJson ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.pos = (UA_Byte*)0x01;
    ctx.end = (const UA_Byte*)(uintptr_t)SIZE_MAX;
    ctx.depth = 0;
    if(options) {
        ctx.namespaceMapping = options->namespaceMapping;
        ctx.serverUris = options->serverUris;
        ctx.serverUrisSize = options->serverUrisSize;
        ctx.prettyPrint = options->prettyPrint;
        ctx.unquotedKeys = options->unquotedKeys;
    }

    ctx.calcOnly = true;

    /* Encode */
    status ret = encodeJsonJumpTable[type->typeKind](&ctx, src, type);
    if(ret != UA_STATUSCODE_GOOD)
        return 0;
    return ((size_t)ctx.pos) - 1u;
}

/**********/
/* Decode */
/**********/

#define GET_TOKEN                                                       \
    size_t tokenSize = getTokenLength(&ctx->tokens[ctx->index]);        \
    const char* tokenData = &ctx->json5[ctx->tokens[ctx->index].start]; \
    do {} while(0)

#define CHECK_TOKEN_BOUNDS do {                   \
    if(ctx->index >= ctx->tokensSize)             \
        return UA_STATUSCODE_BADDECODINGERROR;    \
    } while(0)

#define CHECK_NUMBER do {                                \
    if(currentTokenType(ctx) != CJ5_TOKEN_NUMBER) {      \
        return UA_STATUSCODE_BADDECODINGERROR;           \
    }} while(0)

#define CHECK_BOOL do {                                \
    if(currentTokenType(ctx) != CJ5_TOKEN_BOOL) {      \
        return UA_STATUSCODE_BADDECODINGERROR;         \
    }} while(0)

#define CHECK_STRING do {                                \
    if(currentTokenType(ctx) != CJ5_TOKEN_STRING) {      \
        return UA_STATUSCODE_BADDECODINGERROR;           \
    }} while(0)

#define CHECK_OBJECT do {                                \
    if(currentTokenType(ctx) != CJ5_TOKEN_OBJECT) {      \
        return UA_STATUSCODE_BADDECODINGERROR;           \
    }} while(0)

#define CHECK_NULL_SKIP do {                         \
    if(currentTokenType(ctx) == CJ5_TOKEN_NULL) {    \
        ctx->index++;                                \
        return UA_STATUSCODE_GOOD;                   \
    }} while(0)

/* Forward declarations*/
#define DECODE_JSON(TYPE) static status                   \
    TYPE##_decodeJson(ParseCtx *ctx, void *p,             \
                      const UA_DataType *type)

/* If ctx->index points to the beginning of an object, move the index to the
 * next token after this object. Attention! The index can be moved after the
 * last parsed token. So the array length has to be checked afterwards. */
static void
skipObject(ParseCtx *ctx) {
    unsigned int end = ctx->tokens[ctx->index].end;
    do {
        ctx->index++;
    } while(ctx->index < ctx->tokensSize &&
            ctx->tokens[ctx->index].start < end);
}

static status
Array_decodeJson(ParseCtx *ctx, void *dst_, const UA_DataType *type);

static status
Variant_decodeJsonUnwrapExtensionObject(ParseCtx *ctx, void *p,
                                        const UA_DataType *type);

static UA_SByte
jsoneq(const char *json, const cj5_token *tok, const char *searchKey) {
    /* TODO: necessary?
       if(json == NULL
            || tok == NULL
            || searchKey == NULL) {
        return -1;
    } */

    size_t len = getTokenLength(tok);
    if(tok->type == CJ5_TOKEN_STRING &&
       strlen(searchKey) ==  len &&
       strncmp(json + tok->start, (const char*)searchKey, len) == 0)
        return 0;

    return -1;
}

/* Fields whose value position is collected while walking an object once in
 * forward direction. Decoding can then jump directly to the value, independent
 * of the order in which the fields appeared. */
typedef struct {
    const char *fieldName;
    size_t valueIndex;
} JsonFieldIndex;

/* Scan the current object once, reject duplicate names and collect the value
 * positions for the requested fields. The context is advanced past the object. */
static status
scanObjectFields(ParseCtx *ctx, JsonFieldIndex *fields, size_t fieldsSize) {
    UA_assert(currentTokenType(ctx) == CJ5_TOKEN_OBJECT);
    size_t keyCount = (size_t)ctx->tokens[ctx->index].size / 2;
    UA_STACKARRAY(size_t, keys, keyCount);
    ctx->index++;

    for(size_t i = 0; i < keyCount; i++) {
        UA_assert(currentTokenType(ctx) == CJ5_TOKEN_STRING);
        const cj5_token *key = &ctx->tokens[ctx->index];
        size_t keyLength = getTokenLength(key);

        /* Duplicate names are invalid, including unknown in-situ Structure
         * fields of an ExtensionObject. */
        for(size_t j = 0; j < i; j++) {
            const cj5_token *previous = &ctx->tokens[keys[j]];
            if(keyLength == getTokenLength(previous) &&
               memcmp(&ctx->json5[key->start], &ctx->json5[previous->start],
                      keyLength) == 0)
                return UA_STATUSCODE_BADDECODINGERROR;
        }
        keys[i] = ctx->index;

        /* Record the value position of interesting fields. */
        ctx->index++;
        UA_assert(ctx->index < ctx->tokensSize);
        for(size_t j = 0; j < fieldsSize; j++) {
            if(jsoneq(ctx->json5, key, fields[j].fieldName) == 0) {
                fields[j].valueIndex = ctx->index;
                break;
            }
        }

        skipObject(ctx);
    }
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(Boolean) {
    UA_Boolean *dst = (UA_Boolean*)p;
    CHECK_TOKEN_BOUNDS;
    CHECK_BOOL;
    GET_TOKEN;

    if(tokenSize == 4 &&
       (tokenData[0] | 32) == 't' && (tokenData[1] | 32) == 'r' &&
       (tokenData[2] | 32) == 'u' && (tokenData[3] | 32) == 'e') {
        *dst = true;
    } else if(tokenSize == 5 &&
              (tokenData[0] | 32) == 'f' && (tokenData[1] | 32) == 'a' &&
              (tokenData[2] | 32) == 'l' && (tokenData[3] | 32) == 's' &&
              (tokenData[4] | 32) == 'e') {
        *dst = false;
    } else {
        return UA_STATUSCODE_BADDECODINGERROR;
    }

    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
parseUnsignedInteger(const char *tokenData, size_t tokenSize, UA_UInt64 *dst) {
    size_t len = parseUInt64(tokenData, tokenSize, dst);
    if(len == 0)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* There must only be whitespace between the end of the parsed number and
     * the end of the token */
    for(size_t i = len; i < tokenSize; i++) {
        if(tokenData[i] != ' ' && tokenData[i] -'\t' >= 5)
            return UA_STATUSCODE_BADDECODINGERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
parseSignedInteger(const char *tokenData, size_t tokenSize, UA_Int64 *dst) {
    size_t len = parseInt64(tokenData, tokenSize, dst);
    if(len == 0)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* There must only be whitespace between the end of the parsed number and
     * the end of the token */
    for(size_t i = len; i < tokenSize; i++) {
        if(tokenData[i] != ' ' && tokenData[i] -'\t' >= 5)
            return UA_STATUSCODE_BADDECODINGERROR;
    }

    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(Byte) {
    UA_Byte *dst = (UA_Byte*)p;
    CHECK_TOKEN_BOUNDS;
    CHECK_NUMBER;
    GET_TOKEN;
    UA_UInt64 out = 0;
    UA_StatusCode s = parseUnsignedInteger(tokenData, tokenSize, &out);
    if(s != UA_STATUSCODE_GOOD || out > UA_BYTE_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (UA_Byte)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(UInt16) {
    UA_UInt16 *dst = (UA_UInt16*)p;
    CHECK_TOKEN_BOUNDS;
    CHECK_NUMBER;
    GET_TOKEN;
    UA_UInt64 out = 0;
    UA_StatusCode s = parseUnsignedInteger(tokenData, tokenSize, &out);
    if(s != UA_STATUSCODE_GOOD || out > UA_UINT16_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (UA_UInt16)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(UInt32) {
    UA_UInt32 *dst = (UA_UInt32*)p;
    CHECK_TOKEN_BOUNDS;
    CHECK_NUMBER;
    GET_TOKEN;
    UA_UInt64 out = 0;
    UA_StatusCode s = parseUnsignedInteger(tokenData, tokenSize, &out);
    if(s != UA_STATUSCODE_GOOD || out > UA_UINT32_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (UA_UInt32)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(UInt64) {
    UA_UInt64 *dst = (UA_UInt64*)p;
    CHECK_TOKEN_BOUNDS;
    GET_TOKEN;
    UA_StatusCode s = parseUnsignedInteger(tokenData, tokenSize, dst);
    if(s != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(SByte) {
    UA_SByte *dst = (UA_SByte*)p;
    CHECK_TOKEN_BOUNDS;
    CHECK_NUMBER;
    GET_TOKEN;
    UA_Int64 out = 0;
    UA_StatusCode s = parseSignedInteger(tokenData, tokenSize, &out);
    if(s != UA_STATUSCODE_GOOD || out < UA_SBYTE_MIN || out > UA_SBYTE_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (UA_SByte)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(Int16) {
    UA_Int16 *dst = (UA_Int16*)p;
    CHECK_TOKEN_BOUNDS;
    CHECK_NUMBER;
    GET_TOKEN;
    UA_Int64 out = 0;
    UA_StatusCode s = parseSignedInteger(tokenData, tokenSize, &out);
    if(s != UA_STATUSCODE_GOOD || out < UA_INT16_MIN || out > UA_INT16_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (UA_Int16)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(Int32) {
    UA_Int32 *dst = (UA_Int32*)p;
    CHECK_TOKEN_BOUNDS;
    CHECK_NUMBER;
    GET_TOKEN;
    UA_Int64 out = 0;
    UA_StatusCode s = parseSignedInteger(tokenData, tokenSize, &out);
    if(s != UA_STATUSCODE_GOOD || out < UA_INT32_MIN || out > UA_INT32_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (UA_Int32)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(Int64) {
    UA_Int64 *dst = (UA_Int64*)p;
    CHECK_TOKEN_BOUNDS;
    GET_TOKEN;
    UA_StatusCode s = parseSignedInteger(tokenData, tokenSize, dst);
    if(s != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

/* Either a STRING or NUMBER token */
DECODE_JSON(Double) {
    UA_Double *dst = (UA_Double*)p;
    CHECK_TOKEN_BOUNDS;
    GET_TOKEN;

    /* https://www.exploringbinary.com/maximum-number-of-decimal-digits-in-binary-floating-point-numbers/
     * Maximum digit counts for select IEEE floating-point formats: 1074
     * Sanity check.
     */
    if(tokenSize > 2000)
        return UA_STATUSCODE_BADDECODINGERROR;

    cj5_token_type tokenType = currentTokenType(ctx);

    /* It could be a String with Nan, Infinity */
    if(tokenType == CJ5_TOKEN_STRING) {
        ctx->index++;

        if(tokenSize == 8 && memcmp(tokenData, "Infinity", 8) == 0) {
            *dst = INFINITY;
            return UA_STATUSCODE_GOOD;
        }

        if(tokenSize == 9 && memcmp(tokenData, "-Infinity", 9) == 0) {
            /* workaround an MSVC 2013 issue */
            *dst = -INFINITY;
            return UA_STATUSCODE_GOOD;
        }

        if(tokenSize == 3 && memcmp(tokenData, "NaN", 3) == 0) {
            *dst = NAN;
            return UA_STATUSCODE_GOOD;
        }

        if(tokenSize == 4 && memcmp(tokenData, "-NaN", 4) == 0) {
            *dst = NAN;
            return UA_STATUSCODE_GOOD;
        }

        return UA_STATUSCODE_BADDECODINGERROR;
    }

    if(tokenType != CJ5_TOKEN_NUMBER)
        return UA_STATUSCODE_BADDECODINGERROR;

    size_t len = parseDouble(tokenData, tokenSize, dst);
    if(len == 0)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* There must only be whitespace between the end of the parsed number and
     * the end of the token */
    for(size_t i = len; i < tokenSize; i++) {
        if(tokenData[i] != ' ' && tokenData[i] -'\t' >= 5)
            return UA_STATUSCODE_BADDECODINGERROR;
    }

    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(Float) {
    UA_Float *dst = (UA_Float*)p;
    UA_Double v = 0.0;
    UA_StatusCode res = Double_decodeJson(ctx, &v, NULL);
    *dst = (UA_Float)v;
    return res;
}

DECODE_JSON(Guid) {
    UA_Guid *dst = (UA_Guid*)p;
    CHECK_TOKEN_BOUNDS;
    CHECK_STRING;
    GET_TOKEN;
    UA_String str = {tokenSize, (UA_Byte*)(uintptr_t)tokenData};
    ctx->index++;
    return UA_Guid_parse(dst, str);
}

DECODE_JSON(String) {
    UA_String *dst = (UA_String*)p;
    CHECK_TOKEN_BOUNDS;
    CHECK_NULL_SKIP;
    CHECK_STRING;
    GET_TOKEN;
    (void)tokenData;

    /* Empty string? */
    if(tokenSize == 0) {
        dst->data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
        dst->length = 0;
        ctx->index++;
        return UA_STATUSCODE_GOOD;
    }

    /* The decoded utf8 is at most of the same length as the source string */
    char *outBuf = (char*)UA_malloc(tokenSize+1);
    if(!outBuf)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Decode the string */
    cj5_result r;
    r.tokens = ctx->tokens;
    r.num_tokens = (unsigned int)ctx->tokensSize;
    r.json5 = ctx->json5;
    unsigned int len = 0;
    cj5_error_code err = cj5_get_str(&r, (unsigned int)ctx->index, outBuf, &len);
    if(err != CJ5_ERROR_NONE) {
        UA_free(outBuf);
        return UA_STATUSCODE_BADDECODINGERROR;
    }

    /* Set the output */
    dst->length = len;
    if(dst->length > 0) {
        dst->data = (UA_Byte*)outBuf;
    } else {
        dst->data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
        UA_free(outBuf);
    }

    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(ByteString) {
    UA_ByteString *dst = (UA_ByteString*)p;
    CHECK_TOKEN_BOUNDS;
    CHECK_NULL_SKIP;
    CHECK_STRING;
    GET_TOKEN;

    /* Empty bytestring? */
    if(tokenSize == 0) {
        dst->data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
        dst->length = 0;
    } else {
        size_t flen = 0;
        unsigned char* unB64 =
            UA_unbase64((const unsigned char*)tokenData, tokenSize, &flen);
        if(unB64 == 0)
            return UA_STATUSCODE_BADDECODINGERROR;
        dst->data = (u8*)unB64;
        dst->length = flen;
    }

    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(LocalizedText) {
    UA_LocalizedText *dst = (UA_LocalizedText*)p;
    CHECK_OBJECT;
    DecodeEntry entries[2] = {
        {UA_JSONKEY_LOCALE, &dst->locale, NULL, false, &UA_TYPES[UA_TYPES_STRING]},
        {UA_JSONKEY_TEXT, &dst->text, NULL, false, &UA_TYPES[UA_TYPES_STRING]}
    };
    return decodeFields(ctx, entries, 2);
}

DECODE_JSON(QualifiedName) {
    UA_QualifiedName *dst = (UA_QualifiedName*)p;
    CHECK_NULL_SKIP;
    UA_String str;
    UA_String_init(&str);
    status res = String_decodeJson(ctx, &str, NULL);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_QualifiedName_parseEx(dst, str, ctx->namespaceMapping);
    UA_String_clear(&str);
    return res;
}

status
lookAheadForKey(ParseCtx *ctx, const char *key, size_t *resultIndex) {
    /* The current index must point to the beginning of an object.
     * This has to be ensured by the caller. */
    UA_assert(currentTokenType(ctx) == CJ5_TOKEN_OBJECT);

    status ret = UA_STATUSCODE_BADNOTFOUND;
    size_t oldIndex = ctx->index; /* Save index for later restore */
    unsigned int end = ctx->tokens[ctx->index].end;
    ctx->index++; /* Move to the first key */
    while(ctx->index < ctx->tokensSize &&
          ctx->tokens[ctx->index].start < end) {
        /* Key must be a string */
        UA_assert(currentTokenType(ctx) == CJ5_TOKEN_STRING);

        /* Move index to the value */
        ctx->index++;

        /* Value for the key must exist */
        UA_assert(ctx->index < ctx->tokensSize);

        /* Compare the key (previous index) */
        if(jsoneq(ctx->json5, &ctx->tokens[ctx->index-1], key) == 0) {
            *resultIndex = ctx->index; /* Point result to the current index */
            ret = UA_STATUSCODE_GOOD;
            break;
        }

        skipObject(ctx); /* Jump over the value (can also be an array or object) */
    }
    ctx->index = oldIndex; /* Restore the old index */
    return ret;
}

DECODE_JSON(NodeId) {
    UA_NodeId *dst = (UA_NodeId*)p;
    UA_String str;
    UA_String_init(&str);
    status res = String_decodeJson(ctx, &str, NULL);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_NodeId_parseEx(dst, str, ctx->namespaceMapping);
    UA_String_clear(&str);
    return res;
}

DECODE_JSON(ExpandedNodeId) {
    UA_ExpandedNodeId *dst = (UA_ExpandedNodeId*)p;
    UA_String str;
    UA_String_init(&str);
    status res = String_decodeJson(ctx, &str, NULL);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_ExpandedNodeId_parseEx(dst, str, ctx->namespaceMapping,
                                        ctx->serverUrisSize, ctx->serverUris);
    UA_String_clear(&str);
    return res;
}

DECODE_JSON(DateTime) {
    UA_DateTime *dst = (UA_DateTime*)p;
    CHECK_TOKEN_BOUNDS;
    CHECK_NULL_SKIP;
    CHECK_STRING;
    GET_TOKEN;

    /* YYYY-MM-DDTHH:MM:SS[.fffffff]Z */
    if(tokenSize < 20 || tokenData[tokenSize-1] != 'Z' ||
       tokenData[4] != '-' || tokenData[7] != '-' ||
       tokenData[10] != 'T' || tokenData[13] != ':' ||
       tokenData[16] != ':')
        return UA_STATUSCODE_BADDECODINGERROR;

    const size_t positions[6] = {0, 5, 8, 11, 14, 17};
    UA_UInt16 values[6];
    for(size_t i = 0; i < 6; i++) {
        UA_UInt64 value = 0;
        size_t digits = (i == 0) ? 4 : 2;
        if(parseUInt64(&tokenData[positions[i]], digits, &value) != digits)
            return UA_STATUSCODE_BADDECODINGERROR;
        values[i] = (UA_UInt16)value;
    }

    UA_UInt16 daysInMonth = 31;
    switch(values[1]) {
    case 2:
        daysInMonth = (UA_UInt16)
            (((values[0] % 4 == 0 && values[0] % 100 != 0) ||
              values[0] % 400 == 0) ? 29 : 28);
        break;
    case 4: case 6: case 9: case 11:
        daysInMonth = 30;
        break;
    default:
        break;
    }
    if(values[0] < 1 || values[1] < 1 || values[1] > 12 ||
       values[2] < 1 || values[2] > daysInMonth || values[3] > 23 ||
       values[4] > 59 || values[5] > 59)
        return UA_STATUSCODE_BADDECODINGERROR;

    size_t pos = 19;
    UA_UInt32 fraction = 0;
    size_t fractionDigits = 0;
    if(pos < tokenSize - 1 &&
       (tokenData[pos] == '.' || tokenData[pos] == ',')) {
        pos++;
        while(pos < tokenSize - 1 && tokenData[pos] >= '0' &&
              tokenData[pos] <= '9') {
            if(fractionDigits < 7)
                fraction = fraction * 10 + (UA_UInt32)(tokenData[pos] - '0');
            fractionDigits++;
            pos++;
        }
        if(fractionDigits == 0)
            return UA_STATUSCODE_BADDECODINGERROR;
        while(fractionDigits < 7) {
            fraction *= 10;
            fractionDigits++;
        }
    }
    if(pos != tokenSize - 1)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* The minimum JSON DateTime is the null value. */
    if(values[0] == 1 && values[1] == 1 && values[2] == 1 &&
       values[3] == 0 && values[4] == 0 && values[5] == 0 && fraction == 0) {
        *dst = 0;
    } else {
        UA_DateTimeStruct date;
        memset(&date, 0, sizeof(date));
        date.year = (UA_Int16)values[0];
        date.month = values[1];
        date.day = values[2];
        date.hour = values[3];
        date.min = values[4];
        date.sec = values[5];
        date.milliSec = (UA_UInt16)(fraction / 10000);
        date.microSec = (UA_UInt16)((fraction % 10000) / 10);
        date.nanoSec = (UA_UInt16)((fraction % 10) * 100);
        *dst = UA_DateTime_fromStruct(date);
    }

    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(StatusCode) {
    UA_StatusCode *dst = (UA_StatusCode*)p;
    CHECK_OBJECT;
    DecodeEntry entries[2] = {
        {UA_JSONKEY_CODE, dst, NULL, false, &UA_TYPES[UA_TYPES_UINT32]},
        {UA_JSONKEY_SYMBOL, NULL, NULL, false, NULL}
    };
    return decodeFields(ctx, entries, 2);
}

/* Get type type encoded by the ExtensionObject at ctx->index.
 * Returns NULL if that fails (type unknown or otherwise). */
static const UA_DataType *
getExtensionObjectType(ParseCtx *ctx) {
    if(currentTokenType(ctx) != CJ5_TOKEN_OBJECT)
        return NULL;

    /* Get the type NodeId index */
    size_t typeIdIndex = 0;
    UA_StatusCode ret = lookAheadForKey(ctx, UA_JSONKEY_TYPEID, &typeIdIndex);
    if(ret != UA_STATUSCODE_GOOD)
        return NULL;

    size_t oldIndex = ctx->index;
    ctx->index = (UA_UInt16)typeIdIndex;

    /* Decode the type NodeId */
    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    ret = NodeId_decodeJson(ctx, &typeId, &UA_TYPES[UA_TYPES_NODEID]);
    ctx->index = oldIndex;
    if(ret != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&typeId); /* We don't have the global cleanup */
        return NULL;
    }

    /* Lookup an return */
    const UA_DataType *type = UA_findDataTypeWithCustom(&typeId, ctx->customTypes);
    UA_NodeId_clear(&typeId);
    return type;
}

/* Check if all array members are ExtensionObjects of the same type. Return this
 * type or NULL. */
static const UA_DataType *
getArrayUnwrapType(ParseCtx *ctx) {
    UA_assert(ctx->tokens[ctx->index].type == CJ5_TOKEN_ARRAY);

    /* Return early for empty arrays */
    size_t length = (size_t)ctx->tokens[ctx->index].size;
    if(length == 0)
        return NULL;

    /* Save the original index and go to the first array member */
    size_t oldIndex = ctx->index;
    ctx->index++;

    /* Lookup the type for the first array member */
    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    const UA_DataType *typeOfBody = getExtensionObjectType(ctx);
    if(!typeOfBody) {
        ctx->index = oldIndex; /* Restore the index */
        return NULL;
    }

    /* Get the TypeId encoding for faster comparison below.
     * Cannot fail as getExtensionObjectType already looked this up. */
    size_t typeIdIndex = 0;
    UA_StatusCode ret = lookAheadForKey(ctx, UA_JSONKEY_TYPEID, &typeIdIndex);
    (void)ret;
    UA_assert(ret == UA_STATUSCODE_GOOD);
    const char* typeIdData = &ctx->json5[ctx->tokens[typeIdIndex].start];
    size_t typeIdSize = getTokenLength(&ctx->tokens[typeIdIndex]);

    /* Loop over all members and check whether they can be unwrapped. Don't skip
     * the first member. We still haven't checked the encoding type. */
    for(size_t i = 0; i < length; i++) {
        /* Array element must be an object */
        if(currentTokenType(ctx) != CJ5_TOKEN_OBJECT) {
            ctx->index = oldIndex; /* Restore the index */
            return NULL;
        }

        /* Check for non-JSON encoding */
        size_t encIndex = 0;
        ret = lookAheadForKey(ctx, UA_JSONKEY_ENCODING, &encIndex);
        if(ret == UA_STATUSCODE_GOOD) {
            ctx->index = oldIndex; /* Restore the index */
            return NULL;
        }

        /* Get the type NodeId index */
        size_t memberTypeIdIndex = 0;
        ret = lookAheadForKey(ctx, UA_JSONKEY_TYPEID, &memberTypeIdIndex);
        if(ret != UA_STATUSCODE_GOOD) {
            ctx->index = oldIndex; /* Restore the index */
            return NULL;
        }

        /* Is it the same type? Compare raw NodeId string */
        const char* memberTypeIdData = &ctx->json5[ctx->tokens[memberTypeIdIndex].start];
        size_t memberTypeIdSize = getTokenLength(&ctx->tokens[memberTypeIdIndex]);
        if(typeIdSize != memberTypeIdSize ||
           memcmp(typeIdData, memberTypeIdData, typeIdSize) != 0) {
            ctx->index = oldIndex; /* Restore the index */
            return NULL;
        }

        /* Skip to the next array member */
        skipObject(ctx);
    }

    ctx->index = oldIndex; /* Restore the index */
    return typeOfBody;
}

static status
Array_decodeJsonUnwrapExtensionObject(ParseCtx *ctx, void **dst,
                                      const UA_DataType *type) {
    size_t *size_ptr = (size_t*) dst - 1; /* Save the length pointer of the array */
    size_t length = (size_t)ctx->tokens[ctx->index].size;

    /* Known from the previous unwrapping-check */
    UA_assert(currentTokenType(ctx) == CJ5_TOKEN_ARRAY);
    UA_assert(length > 0);

    ctx->index++; /* Go to first array member */

    /* Allocate memory */
    *dst = UA_calloc(length, type->memSize);
    if(*dst == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Decode array members */
    status ret = UA_STATUSCODE_GOOD;
    uintptr_t ptr = (uintptr_t)*dst;
    for(size_t i = 0; i < length; i++) {
        UA_assert(ctx->tokens[ctx->index].type == CJ5_TOKEN_OBJECT);
        if(type->typeKind == UA_DATATYPEKIND_STRUCTURE ||
           type->typeKind == UA_DATATYPEKIND_OPTSTRUCT) {
            /* Decode structure in-situ in the ExtensionObject */
            ret = decodeJsonStructureExtensionObject(ctx, (void*)ptr, type);
        } else if(type->typeKind == UA_DATATYPEKIND_UNION) {
            /* Decode union in-situ in the ExtensionObject */
            ret = decodeJsonUnionExtensionObject(ctx, (void*)ptr, type);
        } else {
            /* Get the body field and decode it */
            DecodeEntry entries[3] = {
                {UA_JSONKEY_TYPEID, NULL, NULL, false, NULL},
                {UA_JSONKEY_BODY, (void*)ptr, NULL, false, type},
                {UA_JSONKEY_ENCODING, NULL, NULL, false, NULL}
            };
            ret = decodeFields(ctx, entries, 3);
        }
        if(ret != UA_STATUSCODE_GOOD) {
            UA_Array_delete(*dst, i+1, type);
            *dst = NULL;
            return ret;
        }
        ptr += type->memSize;
    }

    *size_ptr = length; /* All good, set the size */
    return UA_STATUSCODE_GOOD;
}

static status
decodeJSONVariant(ParseCtx *ctx, UA_Variant *dst,
                  UA_Boolean insideDataValue) {
    /* Empty variant == null */
    if(ctx->tokens[ctx->index].size == 0) {
        ctx->index++;
        return UA_STATUSCODE_GOOD;
    }

    /* Search the value field */
    size_t valueIndex = 0;
    lookAheadForKey(ctx, UA_JSONKEY_VALUE, &valueIndex);

    /* Search for the dimensions field */
    size_t dimIndex = 0;
    lookAheadForKey(ctx, UA_JSONKEY_DIMENSIONS, &dimIndex);

    /* Parse the type kind */
    size_t typeIndex = 0;
    lookAheadForKey(ctx, UA_JSONKEY_TYPE, &typeIndex);
    if(typeIndex == 0 || ctx->tokens[typeIndex].type != CJ5_TOKEN_NUMBER)
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_UInt64 typeKind = 0;
    size_t len = parseUInt64(&ctx->json5[ctx->tokens[typeIndex].start],
                             getTokenLength(&ctx->tokens[typeIndex]), &typeKind);
    if(len == 0)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Shift to get the datatype index. The type must be a builtin data type.
     * All not-builtin types are wrapped in an ExtensionObject. */
    typeKind--;
    if(typeKind > UA_DATATYPEKIND_DIAGNOSTICINFO)
        return UA_STATUSCODE_BADDECODINGERROR;
    const UA_DataType *type = &UA_TYPES[typeKind];

    /* These combinations are excluded by the Variant definition. */
    if(type->typeKind == UA_DATATYPEKIND_DIAGNOSTICINFO ||
       (insideDataValue && type->typeKind == UA_DATATYPEKIND_DATAVALUE))
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Value is an array? */
    UA_Boolean isArray =
        (valueIndex > 0 && ctx->tokens[valueIndex].type == CJ5_TOKEN_ARRAY);
    if(type->typeKind == UA_DATATYPEKIND_VARIANT && !isArray)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Adjust the depth and set the value index as current */
    if(ctx->depth >= UA_JSON_ENCODING_MAX_RECURSION)
        return UA_STATUSCODE_BADDECODINGERROR;
    size_t beginIndex = ctx->index;
    ctx->index = valueIndex;
    ctx->depth++;

    /* Decode the value */
    status res = UA_STATUSCODE_GOOD;
    if(!isArray) {
        /* Scalar with dimensions -> error */
        if(dimIndex > 0) {
            res = UA_STATUSCODE_BADDECODINGERROR;
            goto out;
        }

        /* Missing values are only valid for nullable types. */
        if(valueIndex == 0 && !isJsonNullable(type)) {
            res = UA_STATUSCODE_BADDECODINGERROR;
            goto out;
        }

        /* JSON null is only valid for nullable types. */
        if(valueIndex > 0 && ctx->tokens[valueIndex].type == CJ5_TOKEN_NULL &&
           !isJsonNullable(type)) {
            res = UA_STATUSCODE_BADDECODINGERROR;
            goto out;
        }

        /* Decode a value wrapped in an ExtensionObject */
        if(valueIndex > 0 && type->typeKind == UA_DATATYPEKIND_EXTENSIONOBJECT) {
            res = Variant_decodeJsonUnwrapExtensionObject(ctx, dst, NULL);
            goto out;
        }

        /* Allocate memory for the value */
        dst->data = UA_new(type);
        if(!dst->data) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
            goto out;
        }
        dst->type = type;

        /* Decode the value */
        if(valueIndex > 0 && ctx->tokens[valueIndex].type != CJ5_TOKEN_NULL)
            res = decodeJsonJumpTable[type->typeKind](ctx, dst->data, type);
    } else {
        /* Decode an array. Try to unwrap ExtensionObjects in the array. The
         * members must all have the same type. */
        const UA_DataType *unwrapType = NULL;
        if(type == &UA_TYPES[UA_TYPES_EXTENSIONOBJECT])
            unwrapType = getArrayUnwrapType(ctx);
        if(unwrapType) {
            dst->type = unwrapType;
            res = Array_decodeJsonUnwrapExtensionObject(ctx, &dst->data, unwrapType);
        } else {
            dst->type = type;
            res = Array_decodeJson(ctx, &dst->data, type);
        }

        /* Decode array dimensions */
        if(dimIndex > 0) {
            ctx->index = dimIndex;
            res |= Array_decodeJson(ctx, (void**)&dst->arrayDimensions,
                                    &UA_TYPES[UA_TYPES_UINT32]);

            /* Help clang-analyzer */
            UA_assert(dst->arrayDimensionsSize == 0 || dst->arrayDimensions);

            /* Validate the dimensions */
            if(res == UA_STATUSCODE_GOOD &&
               !variantDimensionsValid(dst->arrayLength,
                                       dst->arrayDimensionsSize,
                                       dst->arrayDimensions))
                res = UA_STATUSCODE_BADDECODINGERROR;

            /* Only keep >= 2 dimensions */
            if(dst->arrayDimensionsSize == 1) {
                UA_free(dst->arrayDimensions);
                dst->arrayDimensions = NULL;
                dst->arrayDimensionsSize = 0;
            }
        }
    }

 out:
    ctx->index = beginIndex;
    skipObject(ctx);
    ctx->depth--;
    return res;
}

DECODE_JSON(Variant) {
    UA_Variant *dst = (UA_Variant*)p;
    CHECK_NULL_SKIP; /* Treat null as an empty variant */
    CHECK_OBJECT;
    return decodeJSONVariant(ctx, dst, false);
}

DECODE_JSON(DataValue) {
    UA_DataValue *dst = (UA_DataValue*)p;
    CHECK_NULL_SKIP; /* Treat a null value as an empty DataValue */
    CHECK_OBJECT;

    /* Decode the Variant in-situ */
    size_t beginIndex = ctx->index;
    status ret = decodeJSONVariant(ctx, &dst->value, true);
    ctx->index = beginIndex;
    dst->hasValue = (dst->value.type != NULL);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Decode the other members (skip the Variant members) */
    DecodeEntry entries[8] = {
        {UA_JSONKEY_TYPE, NULL, NULL, false, NULL},
        {UA_JSONKEY_VALUE, NULL, NULL, false, NULL},
        {UA_JSONKEY_DIMENSIONS, NULL, NULL, false, NULL},
        {UA_JSONKEY_STATUS, &dst->status, NULL, false, &UA_TYPES[UA_TYPES_STATUSCODE]},
        {UA_JSONKEY_SOURCETIMESTAMP, &dst->sourceTimestamp, NULL,
         false, &UA_TYPES[UA_TYPES_DATETIME]},
        {UA_JSONKEY_SOURCEPICOSECONDS, &dst->sourcePicoseconds, NULL,
         false, &UA_TYPES[UA_TYPES_UINT16]},
        {UA_JSONKEY_SERVERTIMESTAMP, &dst->serverTimestamp, NULL,
         false, &UA_TYPES[UA_TYPES_DATETIME]},
        {UA_JSONKEY_SERVERPICOSECONDS, &dst->serverPicoseconds, NULL,
         false, &UA_TYPES[UA_TYPES_UINT16]}
    };

    ret = decodeFields(ctx, entries, 8);
    dst->hasStatus = entries[3].found;
    dst->hasSourceTimestamp = entries[4].found;
    dst->hasSourcePicoseconds = entries[5].found;
    dst->hasServerTimestamp = entries[6].found;
    dst->hasServerPicoseconds = entries[7].found;
    return ret;
}

/* Move the entire current token into the target bytestring */
static UA_StatusCode
tokenToByteString(ParseCtx *ctx, UA_ByteString *p) {
    GET_TOKEN;
    UA_StatusCode res = UA_ByteString_allocBuffer(p, tokenSize);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    memcpy(p->data, tokenData, tokenSize);
    skipObject(ctx);
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(ExtensionObject) {
    UA_ExtensionObject *dst = (UA_ExtensionObject*)p;
    CHECK_NULL_SKIP; /* Treat a null value as an empty DataValue */
    CHECK_OBJECT;

    /* Empty object -> Null ExtensionObject */
    if(ctx->tokens[ctx->index].size == 0) {
        ctx->index++; /* Skip the empty ExtensionObject */
        return UA_STATUSCODE_GOOD;
    }

    /* Scan the object once for metadata and duplicate keys. */
    size_t beginIndex = ctx->index;
    JsonFieldIndex fields[3] = {
        {UA_JSONKEY_TYPEID, SIZE_MAX},
        {UA_JSONKEY_ENCODING, SIZE_MAX},
        {UA_JSONKEY_BODY, SIZE_MAX}
    };
    status ret = scanObjectFields(ctx, fields, 3);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Decode the optional non-JSON encoding. */
    UA_UInt64 encoding = 0;
    size_t encIndex = fields[1].valueIndex;
    if(encIndex != SIZE_MAX) {
        const char *extObjEncoding = &ctx->json5[ctx->tokens[encIndex].start];
        size_t len = parseUInt64(extObjEncoding,
                                 getTokenLength(&ctx->tokens[encIndex]),
                                 &encoding);
        if(len == 0 || encoding > 2)
            return UA_STATUSCODE_BADDECODINGERROR;
    }

    /* Decode the type NodeId. */
    size_t typeIdIndex = fields[0].valueIndex;
    if(typeIdIndex == SIZE_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    ParseCtx decodeCtx = *ctx;
    decodeCtx.index = typeIdIndex;
    ret = NodeId_decodeJson(&decodeCtx, &typeId, &UA_TYPES[UA_TYPES_NODEID]);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&typeId); /* We don't have the global cleanup */
        return UA_STATUSCODE_BADDECODINGERROR;
    }

    size_t bodyIndex = fields[2].valueIndex;

    /* Binary and XML bodies stay encoded even if the datatype is known. */
    if(encoding != 0) {
        dst->encoding = (encoding == 1) ?
            UA_EXTENSIONOBJECT_ENCODED_BYTESTRING :
            UA_EXTENSIONOBJECT_ENCODED_XML;
        dst->content.encoded.typeId = typeId;
        if(bodyIndex == SIZE_MAX)
            return UA_STATUSCODE_BADDECODINGERROR;
        decodeCtx.index = bodyIndex;
        return ByteString_decodeJson(&decodeCtx,
                                     &dst->content.encoded.body, NULL);
    }

    /* Lookup the JSON datatype. */
    type = UA_findDataTypeWithCustom(&typeId, ctx->customTypes);
    if(!type) {
        dst->encoding = UA_EXTENSIONOBJECT_ENCODED_JSON;
        dst->content.encoded.typeId = typeId;
        decodeCtx.index = beginIndex;
        return tokenToByteString(&decodeCtx, &dst->content.encoded.body);
    }

    /* No need to keep the TypeId */
    UA_NodeId_clear(&typeId);

    /* Disallow directly nested ExtensionObjects */
    if(type == &UA_TYPES[UA_TYPES_EXTENSIONOBJECT])
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Allocate memory for the decoded data */
    dst->content.decoded.data = UA_new(type);
    if(!dst->content.decoded.data)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    dst->content.decoded.type = type;
    dst->encoding = UA_EXTENSIONOBJECT_DECODED;

    decodeJsonSignature decodeType = decodeJsonJumpTable[type->typeKind];
    if(bodyIndex != SIZE_MAX) {
        decodeCtx.index = bodyIndex;
        return decodeType(&decodeCtx, dst->content.decoded.data, type);
    }

    /* Only JSON structures without an explicit encoding can be in-situ. */
    if(encoding != 0)
        return UA_STATUSCODE_BADDECODINGERROR;

    decodeCtx.index = beginIndex;
    if(type->typeKind == UA_DATATYPEKIND_STRUCTURE ||
       type->typeKind == UA_DATATYPEKIND_OPTSTRUCT)
        return decodeJsonStructureExtensionObject(
            &decodeCtx, dst->content.decoded.data, type);
    if(type->typeKind == UA_DATATYPEKIND_UNION)
        return decodeJsonUnionExtensionObject(
            &decodeCtx, dst->content.decoded.data, type);
    return decodeType(&decodeCtx, dst->content.decoded.data, type);
}

static status
Variant_decodeJsonUnwrapExtensionObject(ParseCtx *ctx, void *p,
                                        const UA_DataType *type) {
    (void) type;
    UA_Variant *dst = (UA_Variant*)p;

    /* ExtensionObject with null body */
    if(currentTokenType(ctx) == CJ5_TOKEN_NULL) {
        dst->data = UA_ExtensionObject_new();
        dst->type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
        ctx->index++;
        return UA_STATUSCODE_GOOD;
    }

    /* Decode the ExtensionObject */
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode ret = ExtensionObject_decodeJson(ctx, &eo, NULL);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_ExtensionObject_clear(&eo); /* We don't have the global cleanup */
        return ret;
    }

    /* The content is still encoded, cannot unwrap */
    if(eo.encoding != UA_EXTENSIONOBJECT_DECODED)
        goto use_eo;

    /* The content is a builtin type that could have been directly encoded in
     * the Variant, there was no need to wrap in an ExtensionObject. But this
     * means for us, that somebody made an extra effort to explicitly get an
     * ExtensionObject. So we keep it. As an added advantage we will generate
     * the same JSON again when encoding again. */
    if(eo.content.decoded.type->typeKind <= UA_DATATYPEKIND_DIAGNOSTICINFO)
        goto use_eo;

    /* Unwrap the ExtensionObject */
    dst->data = eo.content.decoded.data;
    dst->type = eo.content.decoded.type;
    return UA_STATUSCODE_GOOD;

 use_eo:
    /* Don't unwrap */
    dst->data = UA_new(&UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    if(!dst->data) {
        UA_ExtensionObject_clear(&eo);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    dst->type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
    *(UA_ExtensionObject*)dst->data = eo;
    return UA_STATUSCODE_GOOD;
}

status
DiagnosticInfoInner_decodeJson(ParseCtx* ctx, void* dst, const UA_DataType* type);

DECODE_JSON(DiagnosticInfo) {
    UA_DiagnosticInfo *dst = (UA_DiagnosticInfo*)p;
    CHECK_NULL_SKIP; /* Treat a null value as an empty DiagnosticInfo */
    CHECK_OBJECT;

    DecodeEntry entries[7] = {
        {UA_JSONKEY_SYMBOLICID, &dst->symbolicId, NULL,
         false, &UA_TYPES[UA_TYPES_INT32]},
        {UA_JSONKEY_NAMESPACEURI, &dst->namespaceUri, NULL,
         false, &UA_TYPES[UA_TYPES_INT32]},
        {UA_JSONKEY_LOCALIZEDTEXT, &dst->localizedText, NULL,
         false, &UA_TYPES[UA_TYPES_INT32]},
        {UA_JSONKEY_LOCALE, &dst->locale, NULL,
         false, &UA_TYPES[UA_TYPES_INT32]},
        {UA_JSONKEY_ADDITIONALINFO, &dst->additionalInfo, NULL,
         false, &UA_TYPES[UA_TYPES_STRING]},
        {UA_JSONKEY_INNERSTATUSCODE, &dst->innerStatusCode, NULL,
         false, &UA_TYPES[UA_TYPES_STATUSCODE]},
        {UA_JSONKEY_INNERDIAGNOSTICINFO, &dst->innerDiagnosticInfo,
         DiagnosticInfoInner_decodeJson, false, NULL}
    };
    status ret = decodeFields(ctx, entries, 7);

    dst->hasSymbolicId = entries[0].found;
    dst->hasNamespaceUri = entries[1].found;
    dst->hasLocalizedText = entries[2].found;
    dst->hasLocale = entries[3].found;
    dst->hasAdditionalInfo = entries[4].found;
    dst->hasInnerStatusCode = entries[5].found;
    dst->hasInnerDiagnosticInfo = entries[6].found;
    return ret;
}

status
DiagnosticInfoInner_decodeJson(ParseCtx* ctx, void* dst, const UA_DataType* type) {
    UA_DiagnosticInfo *inner = (UA_DiagnosticInfo*)
        UA_calloc(1, sizeof(UA_DiagnosticInfo));
    if(!inner)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_DiagnosticInfo **dst2 = (UA_DiagnosticInfo**)dst;
    *dst2 = inner;  /* Copy new Pointer do dest */
    return DiagnosticInfo_decodeJson(ctx, inner, type);
}

status
decodeFields(ParseCtx *ctx, DecodeEntry *entries, size_t entryCount) {
    CHECK_TOKEN_BOUNDS;
    CHECK_NULL_SKIP; /* null is treated like an empty object */

    if(ctx->depth >= UA_JSON_ENCODING_MAX_RECURSION)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Keys and values are counted separately */
    CHECK_OBJECT;
    UA_assert(ctx->tokens[ctx->index].size % 2 == 0);
    size_t keyCount = (size_t)(ctx->tokens[ctx->index].size) / 2;

    ctx->index++; /* Go to first key - or jump after the empty object */
    ctx->depth++;

    status ret = UA_STATUSCODE_GOOD;
    for(size_t key = 0; key < keyCount; key++) {
        /* Key must be a string */
        UA_assert(ctx->index < ctx->tokensSize);
        UA_assert(currentTokenType(ctx) == CJ5_TOKEN_STRING);

        /* Search for the decoding entry matching the key. Start at the key
         * index to speed-up the case where they key-order is the same as the
         * entry-order. */
        DecodeEntry *entry = NULL;
        for(size_t i = key; i < key + entryCount; i++) {
            size_t ii = i;
            while(ii >= entryCount)
                ii -= entryCount;

            /* Compare the key */
            if(jsoneq(ctx->json5, &ctx->tokens[ctx->index],
                      entries[ii].fieldName) != 0)
                continue;

            /* Key was already used -> duplicate, abort */
            if(entries[ii].found) {
                ctx->depth--;
                return UA_STATUSCODE_BADDECODINGERROR;
            }

            /* Found the key */
            entries[ii].found = true;
            entry = &entries[ii];
            break;
        }

        /* The key is unknown */
        if(!entry) {
            ret = UA_STATUSCODE_BADDECODINGERROR;
            break;
        }

        /* Go from key to value */
        ctx->index++;
        UA_assert(ctx->index < ctx->tokensSize);

        /* An entry that was expected but shall not be decoded.
         * Jump over the value. */
        if(!entry->function && !entry->type) {
            skipObject(ctx);
            continue;
        }

        /* A null-value is only valid for nullable fields. */
        if(currentTokenType(ctx) == CJ5_TOKEN_NULL && !entry->function) {
            if(!entry->type || !isJsonNullable(entry->type)) {
                ctx->depth--;
                return UA_STATUSCODE_BADDECODINGERROR;
            }
            ctx->index++; /* skip null value */
            continue;
        }

        /* Decode. This also moves to the next key or right after the object for
         * the last value. */
        decodeJsonSignature decodeFunc = (entry->function) ?
            entry->function : decodeJsonJumpTable[entry->type->typeKind];
        ret = decodeFunc(ctx, entry->fieldPointer, entry->type);
        if(ret != UA_STATUSCODE_GOOD)
            break;
    }

    ctx->depth--;
    return ret;
}

static status
Array_decodeJson(ParseCtx *ctx, void *dst_, const UA_DataType *type) {
    void **dst = (void**)dst_;

    /* Save the length of the array */
    size_t *size_ptr = (size_t*) dst - 1;

    /* A null JSON array represents a null OPC UA array. */
    if(currentTokenType(ctx) == CJ5_TOKEN_NULL) {
        *size_ptr = 0;
        *dst = NULL;
        ctx->index++;
        return UA_STATUSCODE_GOOD;
    }

    if(currentTokenType(ctx) != CJ5_TOKEN_ARRAY)
        return UA_STATUSCODE_BADDECODINGERROR;

    size_t length = (size_t)ctx->tokens[ctx->index].size;

    ctx->index++; /* Go to first array member or to the first element after
                   * the array (if empty) */

    /* Return early for empty arrays */
    if(length == 0) {
        *size_ptr = length;
        *dst = UA_EMPTY_ARRAY_SENTINEL;
        return UA_STATUSCODE_GOOD;
    }

    /* Allocate memory */
    *dst = UA_calloc(length, type->memSize);
    if(*dst == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Decode array members */
    decodeJsonSignature decodeFunc = decodeJsonJumpTable[type->typeKind];
    uintptr_t ptr = (uintptr_t)*dst;
    for(size_t i = 0; i < length; ++i) {
        if(ctx->tokens[ctx->index].type == CJ5_TOKEN_NULL) {
            if(!isJsonNullable(type)) {
                UA_Array_delete(*dst, i, type);
                *dst = NULL;
                return UA_STATUSCODE_BADDECODINGERROR;
            }
            ptr += type->memSize;
            ctx->index++;
            continue;
        }

        status ret = decodeFunc(ctx, (void*)ptr, type);
        ptr += type->memSize;
        if(ret != UA_STATUSCODE_GOOD) {
            UA_Array_delete(*dst, i+1, type);
            *dst = NULL;
            return ret;
        }
    }

    *size_ptr = length; /* All good, set the size */
    return UA_STATUSCODE_GOOD;
}

static status
OptionalScalar_decodeJson(ParseCtx *ctx, void *dst,
                          const UA_DataType *type) {
    void *value = UA_calloc(1, type->memSize);
    if(!value)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *(void**)dst = value;
    if(currentTokenType(ctx) == CJ5_TOKEN_NULL) {
        ctx->index++;
        return UA_STATUSCODE_GOOD;
    }
    return decodeJsonJumpTable[type->typeKind](ctx, value, type);
}

static status
OptionalArray_decodeJson(ParseCtx *ctx, void *dst,
                         const UA_DataType *type) {
    if(currentTokenType(ctx) == CJ5_TOKEN_NULL) {
        size_t *length = (size_t*)dst - 1;
        *length = 0;
        *(void**)dst = UA_EMPTY_ARRAY_SENTINEL;
        ctx->index++;
        return UA_STATUSCODE_GOOD;
    }
    return Array_decodeJson(ctx, dst, type);
}

static status
decodeJsonStructureInternal(ParseCtx *ctx, void *dst, const UA_DataType *type,
                            UA_Boolean extensionObject) {
    uintptr_t ptr = (uintptr_t)dst;
    status ret = UA_STATUSCODE_GOOD;
    const UA_Boolean optional = (type->typeKind == UA_DATATYPEKIND_OPTSTRUCT);
    size_t offset = (extensionObject ? 1 : 0) + (optional ? 1 : 0);
    size_t membersSize = type->membersSize + offset;
    UA_STACKARRAY(DecodeEntry, entries, membersSize);
    memset(entries, 0, sizeof(DecodeEntry) * membersSize);
    size_t entryIndex = 0;
    if(extensionObject) {
        entries[entryIndex++] =
            (DecodeEntry){UA_JSONKEY_TYPEID, NULL, NULL, false, NULL};
    }
    UA_UInt32 encodingMask = 0;
    size_t encodingMaskIndex = SIZE_MAX;
    if(optional) {
        encodingMaskIndex = entryIndex;
        entries[entryIndex++] =
            (DecodeEntry){UA_JSONKEY_ENCODINGMASK, &encodingMask, NULL, false,
                          &UA_TYPES[UA_TYPES_UINT32]};
    }
    for(size_t i = 0; i < type->membersSize; ++i, ++entryIndex) {
        const UA_DataTypeMember *m = &type->members[i];
        const UA_DataType *mt = m->memberType;
        entries[entryIndex].type = mt;
        entries[entryIndex].fieldName = m->memberName;
        if(!m->isArray) {
            ptr += m->padding;
            entries[entryIndex].fieldPointer = (void*)ptr;
            if(m->isOptional)
                entries[entryIndex].function = OptionalScalar_decodeJson;
            ptr += m->isOptional ? sizeof(void*) : mt->memSize;
        } else {
            ptr += m->padding;
            ptr += sizeof(size_t);
            entries[entryIndex].fieldPointer = (void*)ptr;
            entries[entryIndex].function = m->isOptional ?
                OptionalArray_decodeJson : Array_decodeJson;
            ptr += sizeof(void*);
        }
    }

    ret = decodeFields(ctx, entries, membersSize);

    /* Compact JSON uses EncodingMask. Materialize optional fields whose bit is
     * set even when their default value was omitted from the object. */
    if(ret == UA_STATUSCODE_GOOD && optional &&
       entries[encodingMaskIndex].found) {
        size_t optionalIndex = 0;
        entryIndex = offset;
        for(size_t i = 0; i < type->membersSize; i++, entryIndex++) {
            const UA_DataTypeMember *m = &type->members[i];
            if(!m->isOptional)
                continue;
            if(optionalIndex >= 32) {
                ret = UA_STATUSCODE_BADDECODINGERROR;
                break;
            }
            UA_Boolean inMask =
                (encodingMask & ((UA_UInt32)1 << optionalIndex)) != 0;
            if(entries[entryIndex].found != inMask) {
                if(entries[entryIndex].found) {
                    ret = UA_STATUSCODE_BADDECODINGERROR;
                    break;
                }
                if(m->isArray) {
                    *(void**)entries[entryIndex].fieldPointer =
                        UA_EMPTY_ARRAY_SENTINEL;
                } else {
                    void *value = UA_calloc(1, m->memberType->memSize);
                    if(!value) {
                        ret = UA_STATUSCODE_BADOUTOFMEMORY;
                        break;
                    }
                    *(void**)entries[entryIndex].fieldPointer = value;
                }
            }
            optionalIndex++;
        }
        if(optionalIndex < 32 &&
           (encodingMask >> optionalIndex) != 0)
            ret = UA_STATUSCODE_BADDECODINGERROR;
    }

    return ret;
}

static status
decodeJsonStructure(ParseCtx *ctx, void *dst, const UA_DataType *type) {
    return decodeJsonStructureInternal(ctx, dst, type, false);
}

static status
decodeJsonStructureExtensionObject(ParseCtx *ctx, void *dst,
                                   const UA_DataType *type) {
    return decodeJsonStructureInternal(ctx, dst, type, true);
}

static status
decodeJsonUnionInternal(ParseCtx *ctx, void *dst, const UA_DataType *type,
                        UA_Boolean extensionObject) {
    CHECK_OBJECT;

    /* Determine the selected member before decoding. This prevents two member
     * fields from being decoded into the same union storage on malformed input. */
    ParseCtx scan = *ctx;
    size_t keyCount = (size_t)scan.tokens[scan.index].size / 2;
    scan.index++;
    UA_UInt32 selection = 0;
    UA_UInt32 switchField = 0;
    UA_Boolean switchFound = false;
    UA_Boolean typeIdFound = false;
    for(size_t key = 0; key < keyCount; key++) {
        UA_assert(currentTokenType(&scan) == CJ5_TOKEN_STRING);
        const cj5_token *keyToken = &scan.tokens[scan.index++];
        size_t valueIndex = scan.index;

        if(jsoneq(scan.json5, keyToken, UA_JSONKEY_TYPEID) == 0) {
            if(!extensionObject || typeIdFound)
                return UA_STATUSCODE_BADDECODINGERROR;
            typeIdFound = true;
        } else if(jsoneq(scan.json5, keyToken, UA_JSONKEY_SWITCHFIELD) == 0) {
            if(switchFound)
                return UA_STATUSCODE_BADDECODINGERROR;
            switchFound = true;
            status ret = UInt32_decodeJson(&scan, &switchField,
                                           &UA_TYPES[UA_TYPES_UINT32]);
            if(ret != UA_STATUSCODE_GOOD)
                return ret;
        } else {
            UA_UInt32 memberSelection = 0;
            for(size_t i = 0; i < type->membersSize; i++) {
                if(jsoneq(scan.json5, keyToken,
                          type->members[i].memberName) == 0) {
                    memberSelection = (UA_UInt32)i + 1;
                    break;
                }
            }
            if(memberSelection == 0 || selection != 0)
                return UA_STATUSCODE_BADDECODINGERROR;
            selection = memberSelection;
        }

        scan.index = valueIndex;
        skipObject(&scan);
    }

    if(switchField > type->membersSize ||
       (selection != 0 && switchFound && switchField != selection))
        return UA_STATUSCODE_BADDECODINGERROR;
    if(selection == 0)
        selection = switchField;

    DecodeEntry entries[3];
    memset(entries, 0, sizeof(entries));
    size_t entriesSize = 0;
    if(extensionObject)
        entries[entriesSize++] =
            (DecodeEntry){UA_JSONKEY_TYPEID, NULL, NULL, false, NULL};
    entries[entriesSize++] =
        (DecodeEntry){UA_JSONKEY_SWITCHFIELD, NULL, NULL, false, NULL};
    if(selection != 0) {
        const UA_DataTypeMember *m = &type->members[selection - 1];
        entries[entriesSize].fieldName = m->memberName;
        entries[entriesSize].type = m->memberType;
        uintptr_t ptr = (uintptr_t)dst + m->padding;
        if(m->isArray) {
            ptr += sizeof(size_t);
            entries[entriesSize].function = Array_decodeJson;
        }
        entries[entriesSize].fieldPointer = (void*)ptr;
        entriesSize++;
    }

    *(UA_UInt32*)dst = selection;
    return decodeFields(ctx, entries, entriesSize);
}

static status
decodeJsonUnion(ParseCtx *ctx, void *dst, const UA_DataType *type) {
    return decodeJsonUnionInternal(ctx, dst, type, false);
}

static status
decodeJsonUnionExtensionObject(ParseCtx *ctx, void *dst,
                               const UA_DataType *type) {
    return decodeJsonUnionInternal(ctx, dst, type, true);
}

static status
decodeJsonNotImplemented(ParseCtx *ctx, void *dst, const UA_DataType *type) {
    (void)dst, (void)type, (void)ctx;
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

const decodeJsonSignature decodeJsonJumpTable[UA_DATATYPEKINDS] = {
    Boolean_decodeJson,
    SByte_decodeJson, /* SByte */
    Byte_decodeJson,
    Int16_decodeJson, /* Int16 */
    UInt16_decodeJson,
    Int32_decodeJson, /* Int32 */
    UInt32_decodeJson,
    Int64_decodeJson, /* Int64 */
    UInt64_decodeJson,
    Float_decodeJson,
    Double_decodeJson,
    String_decodeJson,
    DateTime_decodeJson, /* DateTime */
    Guid_decodeJson,
    ByteString_decodeJson, /* ByteString */
    String_decodeJson, /* XmlElement */
    NodeId_decodeJson,
    ExpandedNodeId_decodeJson,
    StatusCode_decodeJson, /* StatusCode */
    QualifiedName_decodeJson, /* QualifiedName */
    LocalizedText_decodeJson,
    ExtensionObject_decodeJson,
    DataValue_decodeJson,
    Variant_decodeJson,
    DiagnosticInfo_decodeJson,
    decodeJsonNotImplemented, /* Decimal */
    Int32_decodeJson, /* Enum */
    decodeJsonStructure,
    decodeJsonStructure, /* Structure with optional fields */
    decodeJsonUnion,
    decodeJsonNotImplemented /* BitfieldCluster */
};

status
tokenize(ParseCtx *ctx, const UA_ByteString *src, size_t tokensSize,
         size_t *decodedLength) {
    /* Tokenize */
    cj5_options options;
    options.stop_early = (decodedLength != NULL);
    cj5_result r = cj5_parse((char*)src->data, (unsigned int)src->length,
                             ctx->tokens, (unsigned int)tokensSize, &options);

    /* Handle overflow error by allocating the number of tokens the parser would
     * have needed */
    if(r.error == CJ5_ERROR_OVERFLOW &&
       tokensSize != r.num_tokens) {
        ctx->tokens = (cj5_token*)
            UA_malloc(sizeof(cj5_token) * r.num_tokens);
        if(!ctx->tokens)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        return tokenize(ctx, src, r.num_tokens, decodedLength);
    }

    /* Cannot recover from other errors */
    if(r.error != CJ5_ERROR_NONE)
        return UA_STATUSCODE_BADDECODINGERROR;

    if(decodedLength)
        *decodedLength = ctx->tokens[0].end + 1;

    /* Set up the context */
    ctx->json5 = (char*)src->data;
    ctx->depth = 0;
    ctx->tokensSize = r.num_tokens;
    ctx->index = 0;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_decodeJson(const UA_ByteString *src, void *dst, const UA_DataType *type,
              const UA_DecodeJsonOptions *options) {
    if(!dst || !src || !type)
        return UA_STATUSCODE_BADARGUMENTSMISSING;

    /* The destination is always initialized, including tokenizer failures. */
    memset(dst, 0, type->memSize);

    /* Set up the context */
    cj5_token tokens[UA_JSON_MAXTOKENCOUNT];
    ParseCtx ctx;
    memset(&ctx, 0, sizeof(ParseCtx));
    ctx.tokens = tokens;

    if(options) {
        ctx.namespaceMapping = options->namespaceMapping;
        ctx.serverUris = options->serverUris;
        ctx.serverUrisSize = options->serverUrisSize;
        ctx.customTypes = options->customTypes;
    }

    /* Decode */
    status ret = tokenize(&ctx, src, UA_JSON_MAXTOKENCOUNT,
                          options ? options->decodedLength : NULL);
    if(ret != UA_STATUSCODE_GOOD)
        goto cleanup;

    ret = decodeJsonJumpTable[type->typeKind](&ctx, dst, type);

    /* Boundary decoding intentionally stops after the first JSON value. */
    if((!options || !options->decodedLength) &&
       ctx.index != ctx.tokensSize)
        ret = UA_STATUSCODE_BADDECODINGERROR;

 cleanup:

    /* Free token array on the heap */
    if(ctx.tokens != tokens)
        UA_free((void*)(uintptr_t)ctx.tokens);

    if(ret != UA_STATUSCODE_GOOD)
        UA_clear(dst, type);
    return ret;
}

#endif /* defined(UA_ENABLE_JSON_ENCODING) */
