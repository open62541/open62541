/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Lukas Meling)
 */

/**
 * This file contains the JSON encoding/decoding from before the v1.05 OPC UA
 * specification. The changes in the v1.05 specification are breaking. The
 * encoding is not compatible with new versions. Disable
 * UA_ENABLE_JSON_ENCODING_LEGACY to use the new JSON encoding instead.
 */

#include <open62541/config.h>
#include <open62541/types.h>

#ifdef UA_ENABLE_JSON_ENCODING_LEGACY

#include "ua_types_encoding_json.h"

#include <float.h>
#include <math.h>

#include "../deps/utf8.h"
#include "../deps/itoa.h"
#include "../deps/dtoa.h"
#include "../deps/parse_num.h"
#include "../deps/base64.h"
#include "../deps/libc_time.h"

#ifndef UA_ENABLE_PARSING
#error UA_ENABLE_PARSING required for JSON encoding
#endif

#ifndef UA_ENABLE_TYPEDESCRIPTION
#error UA_ENABLE_TYPEDESCRIPTION required for JSON encoding
#endif

/* vs2008 does not have INFINITY and NAN defined */
#ifndef INFINITY
# define INFINITY ((UA_Double)(DBL_MAX+DBL_MAX))
#endif
#ifndef NAN
# define NAN ((UA_Double)(INFINITY-INFINITY))
#endif

#if defined(_MSC_VER)
# pragma warning(disable: 4756)
# pragma warning(disable: 4056)
#endif

/* Have some slack at the end. E.g. for negative and very long years. */
#define UA_JSON_DATETIME_LENGTH 40

/************/
/* Encoding */
/************/

#define ENCODE_JSON(TYPE) static status \
    TYPE##_encodeJson(CtxJson *ctx, const UA_##TYPE *src, const UA_DataType *type)

#define ENCODE_DIRECT_JSON(SRC, TYPE) \
    TYPE##_encodeJson(ctx, (const UA_##TYPE*)SRC, NULL)

static status UA_FUNC_ATTR_WARN_UNUSED_RESULT
writeChar(CtxJson *ctx, char c) {
    if(ctx->pos >= ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        *ctx->pos = (UA_Byte)c;
    ctx->pos++;
    return UA_STATUSCODE_GOOD;
}

static status UA_FUNC_ATTR_WARN_UNUSED_RESULT
writeChars(CtxJson *ctx, const char *c, size_t len) {
    if(ctx->pos + len > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        memcpy(ctx->pos, c, len);
    ctx->pos += len;
    return UA_STATUSCODE_GOOD;
}

#define WRITE_JSON_ELEMENT(ELEM)                            \
    UA_FUNC_ATTR_WARN_UNUSED_RESULT status                  \
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
    /* increase depth, save: before first key-value no comma needed. */
    if(ctx->depth >= UA_JSON_ENCODING_MAX_RECURSION - 1)
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
    /* increase depth, save: before first array entry no comma needed. */
    if(ctx->depth >= UA_JSON_ENCODING_MAX_RECURSION - 1)
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
    return writeJsonKey(ctx, key) | encodeJsonJumpTable[type->typeKind](ctx, value, type);
}

/* Keys for JSON */

/* LocalizedText */
static const char* UA_JSONKEY_LOCALE = "Locale";
static const char* UA_JSONKEY_TEXT = "Text";

/* QualifiedName */
static const char* UA_JSONKEY_NAME = "Name";
static const char* UA_JSONKEY_URI = "Uri";

/* NodeId */
static const char* UA_JSONKEY_ID = "Id";
static const char* UA_JSONKEY_IDTYPE = "IdType";
static const char* UA_JSONKEY_NAMESPACE = "Namespace";

/* ExpandedNodeId */
static const char* UA_JSONKEY_SERVERURI = "ServerUri";

/* Variant */
static const char* UA_JSONKEY_TYPE = "Type";
static const char* UA_JSONKEY_BODY = "Body";
static const char* UA_JSONKEY_DIMENSION = "Dimension";

/* DataValue */
static const char* UA_JSONKEY_VALUE = "Value";
static const char* UA_JSONKEY_STATUS = "Status";
static const char* UA_JSONKEY_SOURCETIMESTAMP = "SourceTimestamp";
static const char* UA_JSONKEY_SOURCEPICOSECONDS = "SourcePicoseconds";
static const char* UA_JSONKEY_SERVERTIMESTAMP = "ServerTimestamp";
static const char* UA_JSONKEY_SERVERPICOSECONDS = "ServerPicoseconds";

/* ExtensionObject */
static const char* UA_JSONKEY_ENCODING = "Encoding";
static const char* UA_JSONKEY_TYPEID = "TypeId";

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
status UA_FUNC_ATTR_WARN_UNUSED_RESULT
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

static bool
isNull(const void *p, const UA_DataType *type) {
    if(UA_DataType_isNumeric(type) || type->typeKind == UA_DATATYPEKIND_BOOLEAN)
        return false;
    UA_STACKARRAY(char, buf, type->memSize);
    memset(buf, 0, type->memSize);
    return UA_equal(buf, p, type);
}

/* Boolean */
ENCODE_JSON(Boolean) {
    if(*src == true)
        return writeChars(ctx, "true", 4);
    return writeChars(ctx, "false", 5);
}

/* Byte */
ENCODE_JSON(Byte) {
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

/* signed Byte */
ENCODE_JSON(SByte) {
    char buf[5];
    UA_UInt16 digits = itoaSigned(*src, buf);
    if(ctx->pos + digits > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        memcpy(ctx->pos, buf, digits);
    ctx->pos += digits;
    return UA_STATUSCODE_GOOD;
}

/* UInt16 */
ENCODE_JSON(UInt16) {
    char buf[6];
    UA_UInt16 digits = itoaUnsigned(*src, buf, 10);

    if(ctx->pos + digits > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;

    if(!ctx->calcOnly)
        memcpy(ctx->pos, buf, digits);
    ctx->pos += digits;
    return UA_STATUSCODE_GOOD;
}

/* Int16 */
ENCODE_JSON(Int16) {
    char buf[7];
    UA_UInt16 digits = itoaSigned(*src, buf);

    if(ctx->pos + digits > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;

    if(!ctx->calcOnly)
        memcpy(ctx->pos, buf, digits);
    ctx->pos += digits;
    return UA_STATUSCODE_GOOD;
}

/* UInt32 */
ENCODE_JSON(UInt32) {
    char buf[11];
    UA_UInt16 digits = itoaUnsigned(*src, buf, 10);

    if(ctx->pos + digits > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;

    if(!ctx->calcOnly)
        memcpy(ctx->pos, buf, digits);
    ctx->pos += digits;
    return UA_STATUSCODE_GOOD;
}

/* Int32 */
ENCODE_JSON(Int32) {
    char buf[12];
    UA_UInt16 digits = itoaSigned(*src, buf);

    if(ctx->pos + digits > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;

    if(!ctx->calcOnly)
        memcpy(ctx->pos, buf, digits);
    ctx->pos += digits;
    return UA_STATUSCODE_GOOD;
}

/* UInt64 */
ENCODE_JSON(UInt64) {
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

/* Int64 */
ENCODE_JSON(Int64) {
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
    char buffer[32];
    size_t len;
    if(*src != *src) {
        strcpy(buffer, "\"NaN\"");
        len = strlen(buffer);
    } else if(*src == INFINITY) {
        strcpy(buffer, "\"Infinity\"");
        len = strlen(buffer);
    } else if(*src == -INFINITY) {
        strcpy(buffer, "\"-Infinity\"");
        len = strlen(buffer);
    } else {
        len = dtoa((UA_Double)*src, buffer);
    }

    if(ctx->pos + len > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;

    if(!ctx->calcOnly)
        memcpy(ctx->pos, buffer, len);
    ctx->pos += len;
    return UA_STATUSCODE_GOOD;
}

ENCODE_JSON(Double) {
    char buffer[32];
    size_t len;
    if(*src != *src) {
        strcpy(buffer, "\"NaN\"");
        len = strlen(buffer);
    } else if(*src == INFINITY) {
        strcpy(buffer, "\"Infinity\"");
        len = strlen(buffer);
    } else if(*src == -INFINITY) {
        strcpy(buffer, "\"-Infinity\"");
        len = strlen(buffer);
    } else {
        len = dtoa(*src, buffer);
    }

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

static const u8 hexmap[16] =
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

ENCODE_JSON(String) {
    if(!src->data)
        return writeChars(ctx, "null", 4);

    if(src->length == 0)
        return writeJsonQuote(ctx) | writeJsonQuote(ctx);

    UA_StatusCode ret = writeJsonQuote(ctx);

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
                escape_buf[1] = *pos;
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

/* Guid */
ENCODE_JSON(Guid) {
    if(ctx->pos + 38 > ctx->end) /* 36 + 2 (") */
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    status ret = writeJsonQuote(ctx);
    if(!ctx->calcOnly)
        UA_Guid_to_hex(src, ctx->pos, false);
    ctx->pos += 36;
    return ret | writeJsonQuote(ctx);
}

static u8
printNumber(i32 n, char *pos, u8 min_digits) {
    char digits[10];
    u8 len = 0;
    /* Handle negative values */
    if(n < 0) {
        pos[len++] = '-';
        n = -n;
    }

    /* Extract the digits */
    u8 i = 0;
    for(; i < min_digits || n > 0; i++) {
        digits[i] = (char)((n % 10) + '0');
        n /= 10;
    }

    /* Print in reverse order and return */
    for(; i > 0; i--)
        pos[len++] = digits[i-1];
    return len;
}

ENCODE_JSON(DateTime) {
    UA_DateTimeStruct tSt = UA_DateTime_toStruct(*src);

    /* Format: -yyyy-MM-dd'T'HH:mm:ss.SSSSSSSSS'Z' is used. max 31 bytes.
     * Note the optional minus for negative years. */
    char buffer[UA_JSON_DATETIME_LENGTH];
    char *pos = buffer;
    pos += printNumber(tSt.year, pos, 4);
    *(pos++) = '-';
    pos += printNumber(tSt.month, pos, 2);
    *(pos++) = '-';
    pos += printNumber(tSt.day, pos, 2);
    *(pos++) = 'T';
    pos += printNumber(tSt.hour, pos, 2);
    *(pos++) = ':';
    pos += printNumber(tSt.min, pos, 2);
    *(pos++) = ':';
    pos += printNumber(tSt.sec, pos, 2);
    *(pos++) = '.';
    pos += printNumber(tSt.milliSec, pos, 3);
    pos += printNumber(tSt.microSec, pos, 3);
    pos += printNumber(tSt.nanoSec, pos, 3);

    UA_assert(pos <= &buffer[UA_JSON_DATETIME_LENGTH]);

    /* Remove trailing zeros */
    pos--;
    while(*pos == '0')
        pos--;
    if(*pos == '.')
        pos--;

    *(++pos) = 'Z';
    UA_String str = {((uintptr_t)pos - (uintptr_t)buffer)+1, (UA_Byte*)buffer};
    return ENCODE_DIRECT_JSON(&str, String);
}

/* NodeId */
static status
NodeId_encodeJsonInternal(CtxJson *ctx, UA_NodeId const *src) {
    status ret = UA_STATUSCODE_GOOD;
    switch(src->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        ret |= writeJsonKey(ctx, UA_JSONKEY_ID);
        ret |= ENCODE_DIRECT_JSON(&src->identifier.numeric, UInt32);
        break;
    case UA_NODEIDTYPE_STRING:
        ret |= writeJsonKey(ctx, UA_JSONKEY_IDTYPE);
        ret |= writeChar(ctx, '1');
        ret |= writeJsonKey(ctx, UA_JSONKEY_ID);
        ret |= ENCODE_DIRECT_JSON(&src->identifier.string, String);
        break;
    case UA_NODEIDTYPE_GUID:
        ret |= writeJsonKey(ctx, UA_JSONKEY_IDTYPE);
        ret |= writeChar(ctx, '2');
        ret |= writeJsonKey(ctx, UA_JSONKEY_ID); /* Id */
        ret |= ENCODE_DIRECT_JSON(&src->identifier.guid, Guid);
        break;
    case UA_NODEIDTYPE_BYTESTRING:
        ret |= writeJsonKey(ctx, UA_JSONKEY_IDTYPE);
        ret |= writeChar(ctx, '3');
        ret |= writeJsonKey(ctx, UA_JSONKEY_ID); /* Id */
        ret |= ENCODE_DIRECT_JSON(&src->identifier.byteString, ByteString);
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return ret;
}

ENCODE_JSON(NodeId) {
    /* Encode as string (non-standard). Encode with the standard utf8 escaping.
     * As the NodeId can contain quote characters, etc. */
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if(ctx->stringNodeIds) {
        UA_String out = UA_STRING_NULL;
        ret |= UA_NodeId_print(src, &out);
        ret |= ENCODE_DIRECT_JSON(&out, String);
        UA_String_clear(&out);
        return ret;
    }

    /* Encode as object */
    ret |= writeJsonObjStart(ctx);
    ret |= NodeId_encodeJsonInternal(ctx, src);
    if(ctx->useReversible) {
        if(src->namespaceIndex > 0) {
            ret |= writeJsonKey(ctx, UA_JSONKEY_NAMESPACE);
            ret |= ENCODE_DIRECT_JSON(&src->namespaceIndex, UInt16);
        }
    } else {
        /* For the non-reversible encoding, the field is the NamespaceUri
         * associated with the NamespaceIndex, encoded as a JSON string.
         * A NamespaceIndex of 1 is always encoded as a JSON number. */
        ret |= writeJsonKey(ctx, UA_JSONKEY_NAMESPACE);
        if(src->namespaceIndex == 1) {
            ret |= ENCODE_DIRECT_JSON(&src->namespaceIndex, UInt16);
        } else {
            /* Check if Namespace given and in range */
            UA_String nsUri = UA_STRING_NULL;
            UA_UInt16 ns = src->namespaceIndex;
            if(ctx->namespaceMapping)
                UA_NamespaceMapping_index2Uri(ctx->namespaceMapping, ns, &nsUri);
            if(nsUri.length > 0) {
                ret |= ENCODE_DIRECT_JSON(&nsUri, String);
            } else {
                /* If not found, print the identifier */
                ret |= ENCODE_DIRECT_JSON(&ns, UInt16);
            }
        }
    }

    return ret | writeJsonObjEnd(ctx);
}

/* ExpandedNodeId */
ENCODE_JSON(ExpandedNodeId) {
    /* Encode as string (non-standard). Encode with utf8 escaping as the NodeId
     * can contain quote characters, etc. */
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if(ctx->stringNodeIds) {
        UA_String out = UA_STRING_NULL;
        ret |= UA_ExpandedNodeId_print(src, &out);
        ret |= ENCODE_DIRECT_JSON(&out, String);
        UA_String_clear(&out);
        return ret;
    }

    /* Encode as object */
    ret |= writeJsonObjStart(ctx);

    /* Encode the identifier portion */
    ret |= NodeId_encodeJsonInternal(ctx, &src->nodeId);

    if(ctx->useReversible) {
        /* Reversible Case */

        if(src->namespaceUri.data) {
            /* If the NamespaceUri is specified it is encoded as a JSON string
             * in this field */
            ret |= writeJsonKey(ctx, UA_JSONKEY_NAMESPACE);
            ret |= ENCODE_DIRECT_JSON(&src->namespaceUri, String);
        } else if(src->nodeId.namespaceIndex > 0) {
            /* If the NamespaceUri is not specified, the NamespaceIndex is
             * encoded. Encoded as a JSON number for the reversible encoding.
             * Omitted if the NamespaceIndex equals 0. */
            ret |= writeJsonKey(ctx, UA_JSONKEY_NAMESPACE);
            ret |= ENCODE_DIRECT_JSON(&src->nodeId.namespaceIndex, UInt16);
        }

        /* Encode the serverIndex/Url. As a JSON number for the reversible
         * encoding. Omitted if the ServerIndex equals 0. */
        if(src->serverIndex > 0) {
            ret |= writeJsonKey(ctx, UA_JSONKEY_SERVERURI);
            ret |= ENCODE_DIRECT_JSON(&src->serverIndex, UInt32);
        }
    } else {
        /* Non-Reversible Case */

        /* If the NamespaceUri is not specified, the NamespaceIndex is encoded
         * with these rules: For the non-reversible encoding the field is the
         * NamespaceUri associated with the NamespaceIndex encoded as a JSON
         * string. A NamespaceIndex of 1 is always encoded as a JSON number. */

        ret |= writeJsonKey(ctx, UA_JSONKEY_NAMESPACE);
        if(src->namespaceUri.data) {
            ret |= ENCODE_DIRECT_JSON(&src->namespaceUri, String);
        } else {
            if(src->nodeId.namespaceIndex == 1) {
                ret |= ENCODE_DIRECT_JSON(&src->nodeId.namespaceIndex, UInt16);
            } else {
                /* Check if Namespace given and in range */
                UA_String nsUri = UA_STRING_NULL;
                UA_UInt16 ns = src->nodeId.namespaceIndex;
                if(ctx->namespaceMapping)
                    UA_NamespaceMapping_index2Uri(ctx->namespaceMapping, ns, &nsUri);
                if(nsUri.length > 0) {
                    ret |= ENCODE_DIRECT_JSON(&nsUri, String);
                } else {
                    ret |= ENCODE_DIRECT_JSON(&ns, UInt16);
                }
            }
        }

        /* For the non-reversible encoding, this field is the ServerUri
         * associated with the ServerIndex portion of the ExpandedNodeId,
         * encoded as a JSON string. */

        /* Check if server given and in range */
        if(src->serverIndex >= ctx->serverUrisSize || !ctx->serverUris)
            return UA_STATUSCODE_BADNOTFOUND;

        UA_String serverUriEntry = ctx->serverUris[src->serverIndex];
        ret |= writeJsonKey(ctx, UA_JSONKEY_SERVERURI);
        ret |= ENCODE_DIRECT_JSON(&serverUriEntry, String);
    }

    return ret | writeJsonObjEnd(ctx);
}

/* LocalizedText */
ENCODE_JSON(LocalizedText) {
    if(ctx->useReversible) {
        status ret = writeJsonObjStart(ctx);
        ret |= writeJsonKey(ctx, UA_JSONKEY_LOCALE);
        ret |= ENCODE_DIRECT_JSON(&src->locale, String);
        ret |= writeJsonKey(ctx, UA_JSONKEY_TEXT);
        ret |= ENCODE_DIRECT_JSON(&src->text, String);
        return ret | writeJsonObjEnd(ctx);
    }

    /* For the non-reversible form, LocalizedText value shall be encoded as a
     * JSON string containing the Text component.*/
    return ENCODE_DIRECT_JSON(&src->text, String);
}

ENCODE_JSON(QualifiedName) {
    status ret = writeJsonObjStart(ctx);
    ret |= writeJsonKey(ctx, UA_JSONKEY_NAME);
    ret |= ENCODE_DIRECT_JSON(&src->name, String);

    if(ctx->useReversible) {
        if(src->namespaceIndex != 0) {
            ret |= writeJsonKey(ctx, UA_JSONKEY_URI);
            ret |= ENCODE_DIRECT_JSON(&src->namespaceIndex, UInt16);
        }
    } else {
        /* For the non-reversible form, the NamespaceUri associated with the
         * NamespaceIndex portion of the QualifiedName is encoded as JSON string
         * unless the NamespaceIndex is 1 or if NamespaceUri is unknown. In
         * these cases, the NamespaceIndex is encoded as a JSON number. */
        ret |= writeJsonKey(ctx, UA_JSONKEY_URI);
        if(src->namespaceIndex == 1) {
            ret |= ENCODE_DIRECT_JSON(&src->namespaceIndex, UInt16);
        } else {
            /* Check if Namespace given and in range */
            UA_String nsUri = UA_STRING_NULL;
            UA_UInt16 ns = src->namespaceIndex;
            if(ctx->namespaceMapping)
                UA_NamespaceMapping_index2Uri(ctx->namespaceMapping, ns, &nsUri);
            if(nsUri.length > 0) {
                ret |= ENCODE_DIRECT_JSON(&nsUri, String);
            } else {
                ret |= ENCODE_DIRECT_JSON(&ns, UInt16); /* If not encode as number */
            }
        }
    }

    return ret | writeJsonObjEnd(ctx);
}

ENCODE_JSON(StatusCode) {
    if(ctx->useReversible)
        return ENCODE_DIRECT_JSON(src, UInt32);

    const char *codename = UA_StatusCode_name(*src);
    UA_String statusDescription = UA_STRING((char*)(uintptr_t)codename);

    status ret = UA_STATUSCODE_GOOD;
    ret |= writeJsonObjStart(ctx);
    ret |= writeJsonKey(ctx, UA_JSONKEY_CODE);
    ret |= ENCODE_DIRECT_JSON(src, UInt32);
    ret |= writeJsonKey(ctx, UA_JSONKEY_SYMBOL);
    ret |= ENCODE_DIRECT_JSON(&statusDescription, String);
    ret |= writeJsonObjEnd(ctx);
    return ret;
}

/* ExtensionObject */
ENCODE_JSON(ExtensionObject) {
    if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY)
        return writeChars(ctx, "null", 4);

    /* Must have a type set if data is decoded */
    if(src->encoding != UA_EXTENSIONOBJECT_ENCODED_BYTESTRING &&
       src->encoding != UA_EXTENSIONOBJECT_ENCODED_XML &&
       !src->content.decoded.type)
        return UA_STATUSCODE_BADENCODINGERROR;

    status ret = writeJsonObjStart(ctx);

    /* Reversible encoding */
    if(ctx->useReversible) {
        /* Write the type NodeId */
        ret |= writeJsonKey(ctx, UA_JSONKEY_TYPEID);
        if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING ||
           src->encoding == UA_EXTENSIONOBJECT_ENCODED_XML)
            ret |= ENCODE_DIRECT_JSON(&src->content.encoded.typeId, NodeId);
        else
            ret |= ENCODE_DIRECT_JSON(&src->content.decoded.type->typeId, NodeId);

        /* Write the encoding */
        if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING) {
            ret |= writeJsonKey(ctx, UA_JSONKEY_ENCODING);
            ret |= writeChar(ctx, '1');
        } else if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_XML) {
            ret |= writeJsonKey(ctx, UA_JSONKEY_ENCODING);
            ret |= writeChar(ctx, '2');
        }
    }

    /* Write the body */
    ret |= writeJsonKey(ctx, UA_JSONKEY_BODY);
    if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING ||
       src->encoding == UA_EXTENSIONOBJECT_ENCODED_XML) {
        ret |= ENCODE_DIRECT_JSON(&src->content.encoded.body, String);
    } else {
        const UA_DataType *t = src->content.decoded.type;
        ret |= encodeJsonJumpTable[t->typeKind]
            (ctx, src->content.decoded.data, t);
    }

    return ret | writeJsonObjEnd(ctx);
}

/* Non-builtin types get wrapped in an ExtensionObject */
static status
encodeScalarJsonWrapExtensionObject(CtxJson *ctx, const UA_Variant *src) {
    const UA_Boolean isBuiltin = (src->type->typeKind <= UA_DATATYPEKIND_DIAGNOSTICINFO);
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
    const UA_Boolean isBuiltin = (type->typeKind <= UA_DATATYPEKIND_DIAGNOSTICINFO);
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

static status
addMultiArrayContentJSON(CtxJson *ctx, void* array, const UA_DataType *type,
                         size_t *index, UA_UInt32 *arrayDimensions, size_t dimensionIndex,
                         size_t dimensionSize) {
    /* Stop recursion: The inner arrays are written */
    status ret;
    if(dimensionIndex == (dimensionSize - 1)) {
        u8 *ptr = ((u8 *)array) + (type->memSize * *index);
        u32 size = arrayDimensions[dimensionIndex];
        (*index) += arrayDimensions[dimensionIndex];
        return encodeArrayJsonWrapExtensionObject(ctx, ptr, size, type);
    }

    /* Recurse to the next dimension */
    ret = writeJsonArrStart(ctx);
    for(size_t i = 0; i < arrayDimensions[dimensionIndex]; i++) {
        ret |= writeJsonBeforeElement(ctx, true);
        ret |= addMultiArrayContentJSON(ctx, array, type, index, arrayDimensions,
                                        dimensionIndex + 1, dimensionSize);
        ctx->commaNeeded[ctx->depth] = true;
    }
    return ret | writeJsonArrEnd(ctx, type);
}

ENCODE_JSON(Variant) {
    /* If type is 0 (NULL) the Variant contains a NULL value and the containing
     * JSON object shall be omitted or replaced by the JSON literal ‘null’ (when
     * an element of a JSON array). */
    if(!src->type)
        return writeJsonObjStart(ctx) | writeJsonObjEnd(ctx);

    /* Set the content type in the encoding mask */
    const UA_Boolean isBuiltin = (src->type->typeKind <= UA_DATATYPEKIND_DIAGNOSTICINFO);

    /* Set the array type in the encoding mask */
    const bool isArray = src->arrayLength > 0 || src->data <= UA_EMPTY_ARRAY_SENTINEL;
    const bool hasDimensions = isArray && src->arrayDimensionsSize > 0;

    /* We cannot directly encode a variant inside a variant (but arrays of
     * variant are possible) */
    UA_Boolean wrapEO = !isBuiltin;
    if(src->type == &UA_TYPES[UA_TYPES_VARIANT] && !isArray)
        wrapEO = true;
    if(ctx->prettyPrint)
        wrapEO = false; /* Don't wrap values in ExtensionObjects for pretty-printing */

    status ret = writeJsonObjStart(ctx);

    /* Write the type NodeId */
    if(ctx->useReversible) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_TYPE);
        if(ctx->prettyPrint) {
            ret |= writeChars(ctx, src->type->typeName, strlen(src->type->typeName));
        } else {
            /* Write the NodeId for the reversible form */
            UA_UInt32 typeId = src->type->typeId.identifier.numeric;
            if(wrapEO)
                typeId = UA_TYPES[UA_TYPES_EXTENSIONOBJECT].typeId.identifier.numeric;
            ret |= ENCODE_DIRECT_JSON(&typeId, UInt32);
        }
    }

    /* Write the Variant body */
    ret |= writeJsonKey(ctx, UA_JSONKEY_BODY);

    if(!isArray) {
        ret |= encodeScalarJsonWrapExtensionObject(ctx, src);
    } else {
        if(ctx->useReversible || !hasDimensions) {
            ret |= encodeArrayJsonWrapExtensionObject(ctx, src->data,
                                                      src->arrayLength, src->type);
            if(hasDimensions) {
                ret |= writeJsonKey(ctx, UA_JSONKEY_DIMENSION);
                ret |= encodeJsonArray(ctx, src->arrayDimensions, src->arrayDimensionsSize,
                                       &UA_TYPES[UA_TYPES_INT32]);
            }
        } else {
            /* Special case of non-reversible array with dimensions */
            size_t index = 0;
            ret |= addMultiArrayContentJSON(ctx, src->data, src->type, &index,
                                            src->arrayDimensions, 0,
                                            src->arrayDimensionsSize);
        }
    }

    return ret | writeJsonObjEnd(ctx);
}

/* DataValue */
ENCODE_JSON(DataValue) {
    UA_Boolean hasValue = src->hasValue;
    UA_Boolean hasStatus = src->hasStatus;
    UA_Boolean hasSourceTimestamp = src->hasSourceTimestamp;
    UA_Boolean hasSourcePicoseconds = src->hasSourcePicoseconds;
    UA_Boolean hasServerTimestamp = src->hasServerTimestamp;
    UA_Boolean hasServerPicoseconds = src->hasServerPicoseconds;

    status ret = writeJsonObjStart(ctx);

    if(hasValue) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_VALUE);
        ret |= ENCODE_DIRECT_JSON(&src->value, Variant);
    }

    if(hasStatus) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_STATUS);
        ret |= ENCODE_DIRECT_JSON(&src->status, StatusCode);
    }

    if(hasSourceTimestamp) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_SOURCETIMESTAMP);
        ret |= ENCODE_DIRECT_JSON(&src->sourceTimestamp, DateTime);
    }

    if(hasSourcePicoseconds) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_SOURCEPICOSECONDS);
        ret |= ENCODE_DIRECT_JSON(&src->sourcePicoseconds, UInt16);
    }

    if(hasServerTimestamp) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_SERVERTIMESTAMP);
        ret |= ENCODE_DIRECT_JSON(&src->serverTimestamp, DateTime);
    }

    if(hasServerPicoseconds) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_SERVERPICOSECONDS);
        ret |= ENCODE_DIRECT_JSON(&src->serverPicoseconds, UInt16);
    }

    return ret | writeJsonObjEnd(ctx);
}

/* DiagnosticInfo */
ENCODE_JSON(DiagnosticInfo) {
    status ret = writeJsonObjStart(ctx);

    if(src->hasSymbolicId) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_SYMBOLICID);
        ret |= ENCODE_DIRECT_JSON(&src->symbolicId, Int32);
    }

    if(src->hasNamespaceUri) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_NAMESPACEURI);
        ret |= ENCODE_DIRECT_JSON(&src->namespaceUri, Int32);
    }

    if(src->hasLocalizedText) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_LOCALIZEDTEXT);
        ret |= ENCODE_DIRECT_JSON(&src->localizedText, Int32);
    }

    if(src->hasLocale) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_LOCALE);
        ret |= ENCODE_DIRECT_JSON(&src->locale, Int32);
    }

    if(src->hasAdditionalInfo) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_ADDITIONALINFO);
        ret |= ENCODE_DIRECT_JSON(&src->additionalInfo, String);
    }

    if(src->hasInnerStatusCode) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_INNERSTATUSCODE);
        ret |= ENCODE_DIRECT_JSON(&src->innerStatusCode, StatusCode);
    }

    if(src->hasInnerDiagnosticInfo && src->innerDiagnosticInfo) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_INNERDIAGNOSTICINFO);
        ret |= encodeJsonJumpTable[UA_DATATYPEKIND_DIAGNOSTICINFO]
            (ctx, src->innerDiagnosticInfo, NULL);
    }

    return ret | writeJsonObjEnd(ctx);
}

static status
encodeJsonStructure(CtxJson *ctx, const void *src, const UA_DataType *type) {
    status ret = writeJsonObjStart(ctx);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    uintptr_t ptr = (uintptr_t) src;
    u8 membersSize = type->membersSize;
    for(size_t i = 0; i < membersSize && ret == UA_STATUSCODE_GOOD; ++i) {
        const UA_DataTypeMember *m = &type->members[i];
        const UA_DataType *mt = m->memberType;

        if(m->memberName != NULL && *m->memberName != 0)
            ret |= writeJsonKey(ctx, m->memberName);

        if(!m->isArray) {
            ptr += m->padding;
            size_t memSize = mt->memSize;
            ret |= encodeJsonJumpTable[mt->typeKind](ctx, (const void*) ptr, mt);
            ptr += memSize;
        } else {
            ptr += m->padding;
            const size_t length = *((const size_t*) ptr);
            ptr += sizeof (size_t);
            ret |= encodeJsonArray(ctx, *(void * const *)ptr, length, mt);
            ptr += sizeof (void*);
        }
    }

    return ret | writeJsonObjEnd(ctx);
}

static status
encodeJsonNotImplemented(const void *src, const UA_DataType *type, CtxJson *ctx) {
    (void) src, (void) type, (void)ctx;
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

const encodeJsonSignature encodeJsonJumpTable[UA_DATATYPEKINDS] = {
    (encodeJsonSignature)Boolean_encodeJson,
    (encodeJsonSignature)SByte_encodeJson, /* SByte */
    (encodeJsonSignature)Byte_encodeJson,
    (encodeJsonSignature)Int16_encodeJson, /* Int16 */
    (encodeJsonSignature)UInt16_encodeJson,
    (encodeJsonSignature)Int32_encodeJson, /* Int32 */
    (encodeJsonSignature)UInt32_encodeJson,
    (encodeJsonSignature)Int64_encodeJson, /* Int64 */
    (encodeJsonSignature)UInt64_encodeJson,
    (encodeJsonSignature)Float_encodeJson,
    (encodeJsonSignature)Double_encodeJson,
    (encodeJsonSignature)String_encodeJson,
    (encodeJsonSignature)DateTime_encodeJson, /* DateTime */
    (encodeJsonSignature)Guid_encodeJson,
    (encodeJsonSignature)ByteString_encodeJson, /* ByteString */
    (encodeJsonSignature)String_encodeJson, /* XmlElement */
    (encodeJsonSignature)NodeId_encodeJson,
    (encodeJsonSignature)ExpandedNodeId_encodeJson,
    (encodeJsonSignature)StatusCode_encodeJson, /* StatusCode */
    (encodeJsonSignature)QualifiedName_encodeJson, /* QualifiedName */
    (encodeJsonSignature)LocalizedText_encodeJson,
    (encodeJsonSignature)ExtensionObject_encodeJson,
    (encodeJsonSignature)DataValue_encodeJson,
    (encodeJsonSignature)Variant_encodeJson,
    (encodeJsonSignature)DiagnosticInfo_encodeJson,
    (encodeJsonSignature)encodeJsonNotImplemented, /* Decimal */
    (encodeJsonSignature)Int32_encodeJson, /* Enum */
    (encodeJsonSignature)encodeJsonStructure,
    (encodeJsonSignature)encodeJsonNotImplemented, /* Structure with optional fields */
    (encodeJsonSignature)encodeJsonNotImplemented, /* Union */
    (encodeJsonSignature)encodeJsonNotImplemented /* BitfieldCluster */
};

UA_StatusCode
UA_encodeJson(const void *src, const UA_DataType *type, UA_ByteString *outBuf,
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
    ctx.useReversible = true; /* default */
    if(options) {
        ctx.namespaceMapping = options->namespaceMapping;
        ctx.serverUris = options->serverUris;
        ctx.serverUrisSize = options->serverUrisSize;
        ctx.useReversible = options->useReversible;
        ctx.prettyPrint = options->prettyPrint;
        ctx.unquotedKeys = options->unquotedKeys;
        ctx.stringNodeIds = options->stringNodeIds;
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
    options.useReversible = true;
    options.prettyPrint = true;
    options.unquotedKeys = true;
    options.stringNodeIds = true;
    return UA_encodeJson(p, type, output, &options);
}

/************/
/* CalcSize */
/************/

size_t
UA_calcSizeJson(const void *src, const UA_DataType *type,
                const UA_EncodeJsonOptions *options) {
    if(!src || !type)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Set up the context */
    CtxJson ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.pos = NULL;
    ctx.end = (const UA_Byte*)(uintptr_t)SIZE_MAX;
    ctx.depth = 0;
    ctx.useReversible = true; /* default */
    if(options) {
        ctx.namespaceMapping = options->namespaceMapping;
        ctx.serverUris = options->serverUris;
        ctx.serverUrisSize = options->serverUrisSize;
        ctx.useReversible = options->useReversible;
        ctx.prettyPrint = options->prettyPrint;
        ctx.unquotedKeys = options->unquotedKeys;
        ctx.stringNodeIds = options->stringNodeIds;
    }

    ctx.calcOnly = true;

    /* Encode */
    status ret = encodeJsonJumpTable[type->typeKind](&ctx, src, type);
    if(ret != UA_STATUSCODE_GOOD)
        return 0;
    return (size_t)ctx.pos;
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
    TYPE##_decodeJson(ParseCtx *ctx, UA_##TYPE *dst,      \
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
Array_decodeJson(ParseCtx *ctx, void **dst, const UA_DataType *type);

static status
Variant_decodeJsonUnwrapExtensionObject(ParseCtx *ctx, void *p, const UA_DataType *type);

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

DECODE_JSON(Boolean) {
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
    CHECK_TOKEN_BOUNDS;
    GET_TOKEN;

    UA_StatusCode s = parseUnsignedInteger(tokenData, tokenSize, dst);
    if(s != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(SByte) {
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
    UA_Double v = 0.0;
    UA_StatusCode res = Double_decodeJson(ctx, &v, NULL);
    *dst = (UA_Float)v;
    return res;
}

DECODE_JSON(Guid) {
    CHECK_TOKEN_BOUNDS;
    CHECK_STRING;
    GET_TOKEN;

    /* Use the existing parsing routine if available */
    UA_String str = {tokenSize, (UA_Byte*)(uintptr_t)tokenData};
    ctx->index++;
    return UA_Guid_parse(dst, str);
}

DECODE_JSON(String) {
    CHECK_TOKEN_BOUNDS;
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
    CHECK_TOKEN_BOUNDS;
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
    CHECK_OBJECT;

    DecodeEntry entries[2] = {
        {UA_JSONKEY_LOCALE, &dst->locale, NULL, false, &UA_TYPES[UA_TYPES_STRING]},
        {UA_JSONKEY_TEXT, &dst->text, NULL, false, &UA_TYPES[UA_TYPES_STRING]}
    };

    return decodeFields(ctx, entries, 2);
}

DECODE_JSON(QualifiedName) {
    CHECK_OBJECT;

    DecodeEntry entries[2] = {
        {UA_JSONKEY_NAME, &dst->name, NULL, false, &UA_TYPES[UA_TYPES_STRING]},
        {UA_JSONKEY_URI, &dst->namespaceIndex, NULL, false, &UA_TYPES[UA_TYPES_UINT16]}
    };

    return decodeFields(ctx, entries, 2);
}

UA_FUNC_ATTR_WARN_UNUSED_RESULT status
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

static status
prepareDecodeNodeIdJson(ParseCtx *ctx, UA_NodeId *dst,
                        u8 *fieldCount, DecodeEntry *entries) {
    UA_assert(currentTokenType(ctx) == CJ5_TOKEN_OBJECT);

    /* possible keys: Id, IdType, NamespaceIndex */
    /* Id must always be present */
    entries[*fieldCount].fieldName = UA_JSONKEY_ID;
    entries[*fieldCount].found = false;
    entries[*fieldCount].type = NULL;
    entries[*fieldCount].function = NULL;

    /* IdType */
    size_t idIndex = 0;
    status ret = lookAheadForKey(ctx, UA_JSONKEY_IDTYPE, &idIndex);
    if(ret == UA_STATUSCODE_GOOD) {
        size_t size = getTokenLength(&ctx->tokens[idIndex]);
        if(size < 1)
            return UA_STATUSCODE_BADDECODINGERROR;

        const char *idType = &ctx->json5[ctx->tokens[idIndex].start];

        if(idType[0] == '2') {
            dst->identifierType = UA_NODEIDTYPE_GUID;
            entries[*fieldCount].fieldPointer = &dst->identifier.guid;
            entries[*fieldCount].type = &UA_TYPES[UA_TYPES_GUID];
        } else if(idType[0] == '1') {
            dst->identifierType = UA_NODEIDTYPE_STRING;
            entries[*fieldCount].fieldPointer = &dst->identifier.string;
            entries[*fieldCount].type = &UA_TYPES[UA_TYPES_STRING];
        } else if(idType[0] == '3') {
            dst->identifierType = UA_NODEIDTYPE_BYTESTRING;
            entries[*fieldCount].fieldPointer = &dst->identifier.byteString;
            entries[*fieldCount].type = &UA_TYPES[UA_TYPES_BYTESTRING];
        } else {
            return UA_STATUSCODE_BADDECODINGERROR;
        }

        /* Id always present */
        (*fieldCount)++;

        entries[*fieldCount].fieldName = UA_JSONKEY_IDTYPE;
        entries[*fieldCount].fieldPointer = NULL;
        entries[*fieldCount].function = NULL;
        entries[*fieldCount].found = false;
        entries[*fieldCount].type = NULL;

        /* IdType */
        (*fieldCount)++;
    } else {
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        entries[*fieldCount].fieldPointer = &dst->identifier.numeric;
        entries[*fieldCount].function = NULL;
        entries[*fieldCount].found = false;
        entries[*fieldCount].type = &UA_TYPES[UA_TYPES_UINT32];
        (*fieldCount)++;
    }

    /* NodeId has a NamespaceIndex (the ExpandedNodeId specialization may
     * overwrite this) */
    entries[*fieldCount].fieldName = UA_JSONKEY_NAMESPACE;
    entries[*fieldCount].fieldPointer = &dst->namespaceIndex;
    entries[*fieldCount].function = NULL;
    entries[*fieldCount].found = false;
    entries[*fieldCount].type = &UA_TYPES[UA_TYPES_UINT16];
    (*fieldCount)++;

    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(NodeId) {
    /* Non-standard decoding of NodeIds from the string representation */
    if(currentTokenType(ctx) == CJ5_TOKEN_STRING) {
        GET_TOKEN;
        UA_String str = {tokenSize, (UA_Byte*)(uintptr_t)tokenData};
        ctx->index++;
        return UA_NodeId_parse(dst, str);
    }

    /* Object representation */
    CHECK_OBJECT;

    u8 fieldCount = 0;
    DecodeEntry entries[3];
    status ret = prepareDecodeNodeIdJson(ctx, dst, &fieldCount, entries);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;
    return decodeFields(ctx, entries, fieldCount);
}

static status
decodeExpandedNodeIdNamespace(ParseCtx *ctx, void *dst, const UA_DataType *type) {
    UA_ExpandedNodeId *en = (UA_ExpandedNodeId*)dst;

    /* Parse as a number */
    size_t oldIndex = ctx->index;
    status ret = UInt16_decodeJson(ctx, &en->nodeId.namespaceIndex, NULL);
    if(ret == UA_STATUSCODE_GOOD)
        return ret;

    /* Parse as a string */
    ctx->index = oldIndex; /* Reset the index */
    ret = String_decodeJson(ctx, &en->namespaceUri, NULL);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Replace with the index if the URI is found. Otherwise keep the string. */
    if(ctx->namespaceMapping) {
        UA_StatusCode mapRes =
            UA_NamespaceMapping_uri2Index(ctx->namespaceMapping, en->namespaceUri,
                                          &en->nodeId.namespaceIndex);
        if(mapRes == UA_STATUSCODE_GOOD)
            UA_String_clear(&en->namespaceUri);
    }

    return UA_STATUSCODE_GOOD;
}

static status
decodeExpandedNodeIdServerUri(ParseCtx *ctx, void *dst, const UA_DataType *type) {
    UA_ExpandedNodeId *en = (UA_ExpandedNodeId*)dst;

    /* Parse as a number */
    size_t oldIndex = ctx->index;
    status ret = UInt32_decodeJson(ctx, &en->serverIndex, NULL);
    if(ret == UA_STATUSCODE_GOOD)
        return ret;

    /* Parse as a string */
    UA_String uri = UA_STRING_NULL;
    ctx->index = oldIndex; /* Reset the index */
    ret = String_decodeJson(ctx, &uri, NULL);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Try to translate the URI into an index */
    ret = UA_STATUSCODE_BADDECODINGERROR;
    for(size_t i = 0; i < ctx->serverUrisSize; i++) {
        if(UA_String_equal(&uri, &ctx->serverUris[i])) {
            en->serverIndex = (UA_UInt32)i;
            ret = UA_STATUSCODE_GOOD;
            break;
        }
    }

    UA_String_clear(&uri);
    return ret;
}

DECODE_JSON(ExpandedNodeId) {
    /* Non-standard decoding of ExpandedNodeIds from the string representation */
    if(currentTokenType(ctx) == CJ5_TOKEN_STRING) {
        GET_TOKEN;
        UA_String str = {tokenSize, (UA_Byte*)(uintptr_t)tokenData};
        ctx->index++;
        return UA_ExpandedNodeId_parse(dst, str);
    }

    /* Object representation */
    CHECK_OBJECT;

    u8 fieldCount = 0;
    DecodeEntry entries[4];
    status ret = prepareDecodeNodeIdJson(ctx, &dst->nodeId, &fieldCount, entries);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Overwrite the namespace entry */
    fieldCount--;
    entries[fieldCount].fieldPointer = dst;
    entries[fieldCount].function = decodeExpandedNodeIdNamespace;
    entries[fieldCount].type = NULL;
    fieldCount++;

    entries[fieldCount].fieldName = UA_JSONKEY_SERVERURI;
    entries[fieldCount].fieldPointer = dst;
    entries[fieldCount].function = decodeExpandedNodeIdServerUri;
    entries[fieldCount].found = false;
    entries[fieldCount].type = NULL;
    fieldCount++;

    return decodeFields(ctx, entries, fieldCount);
}

DECODE_JSON(DateTime) {
    CHECK_TOKEN_BOUNDS;
    CHECK_STRING;
    GET_TOKEN;

    /* The last character has to be 'Z'. We can omit some length checks later on
     * because we know the atoi functions stop before the 'Z'. */
    if(tokenSize == 0 || tokenData[tokenSize-1] != 'Z')
        return UA_STATUSCODE_BADDECODINGERROR;

    struct mytm dts;
    memset(&dts, 0, sizeof(dts));

    size_t pos = 0;
    size_t len;

    /* Parse the year. The ISO standard asks for four digits. But we accept up
     * to five with an optional plus or minus in front due to the range of the
     * DateTime 64bit integer. But in that case we require the year and the
     * month to be separated by a '-'. Otherwise we cannot know where the month
     * starts. */
    if(tokenData[0] == '-' || tokenData[0] == '+')
        pos++;
    UA_Int64 year = 0;
    len = parseInt64(&tokenData[pos], 5, &year);
    pos += len;
    if(len != 4 && tokenData[pos] != '-')
        return UA_STATUSCODE_BADDECODINGERROR;
    if(tokenData[0] == '-')
        year = -year;
    dts.tm_year = (UA_Int16)year - 1900;
    if(tokenData[pos] == '-')
        pos++;

    /* Parse the month */
    UA_UInt64 month = 0;
    len = parseUInt64(&tokenData[pos], 2, &month);
    pos += len;
    UA_CHECK(len == 2, return UA_STATUSCODE_BADDECODINGERROR);
    dts.tm_mon = (UA_UInt16)month - 1;
    if(tokenData[pos] == '-')
        pos++;

    /* Parse the day and check the T between date and time */
    UA_UInt64 day = 0;
    len = parseUInt64(&tokenData[pos], 2, &day);
    pos += len;
    UA_CHECK(len == 2 || tokenData[pos] != 'T',
             return UA_STATUSCODE_BADDECODINGERROR);
    dts.tm_mday = (UA_UInt16)day;
    pos++;

    /* Parse the hour */
    UA_UInt64 hour = 0;
    len = parseUInt64(&tokenData[pos], 2, &hour);
    pos += len;
    UA_CHECK(len == 2, return UA_STATUSCODE_BADDECODINGERROR);
    dts.tm_hour = (UA_UInt16)hour;
    if(tokenData[pos] == ':')
        pos++;

    /* Parse the minute */
    UA_UInt64 min = 0;
    len = parseUInt64(&tokenData[pos], 2, &min);
    pos += len;
    UA_CHECK(len == 2, return UA_STATUSCODE_BADDECODINGERROR);
    dts.tm_min = (UA_UInt16)min;
    if(tokenData[pos] == ':')
        pos++;

    /* Parse the second */
    UA_UInt64 sec = 0;
    len = parseUInt64(&tokenData[pos], 2, &sec);
    pos += len;
    UA_CHECK(len == 2, return UA_STATUSCODE_BADDECODINGERROR);
    dts.tm_sec = (UA_UInt16)sec;

    /* Compute the seconds since the Unix epoch */
    long long sinceunix = __tm_to_secs(&dts);

    /* Are we within the range that can be represented? */
    long long sinceunix_min =
        (long long)(UA_INT64_MIN / UA_DATETIME_SEC) -
        (long long)(UA_DATETIME_UNIX_EPOCH / UA_DATETIME_SEC) -
        (long long)1; /* manual correction due to rounding */
    long long sinceunix_max = (long long)
        ((UA_INT64_MAX - UA_DATETIME_UNIX_EPOCH) / UA_DATETIME_SEC);
    if(sinceunix < sinceunix_min || sinceunix > sinceunix_max)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Convert to DateTime. Add or subtract one extra second here to prevent
     * underflow/overflow. This is reverted once the fractional part has been
     * added. */
    sinceunix -= (sinceunix > 0) ? 1 : -1;
    UA_DateTime dt = (UA_DateTime)
        (sinceunix + (UA_DATETIME_UNIX_EPOCH / UA_DATETIME_SEC)) * UA_DATETIME_SEC;

    /* Parse the fraction of the second if defined */
    if(tokenData[pos] == ',' || tokenData[pos] == '.') {
        pos++;
        double frac = 0.0;
        double denom = 0.1;
        while(pos < tokenSize &&
              tokenData[pos] >= '0' && tokenData[pos] <= '9') {
            frac += denom * (tokenData[pos] - '0');
            denom *= 0.1;
            pos++;
        }
        frac += 0.00000005; /* Correct rounding when converting to integer */
        dt += (UA_DateTime)(frac * UA_DATETIME_SEC);
    }

    /* Remove the underflow/overflow protection (see above) */
    if(sinceunix > 0) {
        if(dt > UA_INT64_MAX - UA_DATETIME_SEC)
            return UA_STATUSCODE_BADDECODINGERROR;
        dt += UA_DATETIME_SEC;
    } else {
        if(dt < UA_INT64_MIN + UA_DATETIME_SEC)
            return UA_STATUSCODE_BADDECODINGERROR;
        dt -= UA_DATETIME_SEC;
    }

    /* We must be at the end of the string (ending with 'Z' as checked above) */
    if(pos != tokenSize - 1)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = dt;

    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(StatusCode) {
    return UInt32_decodeJson(ctx, dst, NULL);
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
getArrayUnwrapType(ParseCtx *ctx, size_t arrayIndex) {
    UA_assert(ctx->tokens[arrayIndex].type == CJ5_TOKEN_ARRAY);

    /* Save index to restore later */
    size_t oldIndex = ctx->index;
    ctx->index = arrayIndex;

    /* Return early for empty arrays */
    size_t length = (size_t)ctx->tokens[ctx->index].size;
    if(length == 0) {
        ctx->index = oldIndex; /* Restore the index */
        return NULL;
    }

    /* Go to first array member */
    ctx->index++;

    /* Lookup the type for the first array member */
    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    const UA_DataType *typeOfBody = getExtensionObjectType(ctx);
    if(!typeOfBody) {
        ctx->index = oldIndex; /* Restore the index */
        return NULL;
    }

    /* The content is a builtin type that could have been directly encoded in
     * the Variant, there was no need to wrap in an ExtensionObject. But this
     * means for us, that somebody made an extra effort to explicitly get an
     * ExtensionObject. So we keep it. As an added advantage we will generate
     * the same JSON again when encoding again. */
    UA_Boolean isBuiltin = (typeOfBody->typeKind <= UA_DATATYPEKIND_DIAGNOSTICINFO);
    if(isBuiltin) {
        ctx->index = oldIndex; /* Restore the index */
        return NULL;
    }

    /* Get the typeId index for faster comparison below.
     * Cannot fail as getExtensionObjectType already looked this up. */
    size_t typeIdIndex = 0;
    UA_StatusCode ret = lookAheadForKey(ctx, UA_JSONKEY_TYPEID, &typeIdIndex);
    (void)ret;
    UA_assert(ret == UA_STATUSCODE_GOOD);
    const char* typeIdData = &ctx->json5[ctx->tokens[typeIdIndex].start];
    size_t typeIdSize = getTokenLength(&ctx->tokens[typeIdIndex]);

    /* Loop over all members and check whether they can be unwrapped */
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
Array_decodeJsonUnwrapExtensionObject(ParseCtx *ctx, void **dst, const UA_DataType *type) {
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
    uintptr_t ptr = (uintptr_t)*dst;
    for(size_t i = 0; i < length; i++) {
        UA_assert(ctx->tokens[ctx->index].type == CJ5_TOKEN_OBJECT);

        /* Get the body field and decode it */
        DecodeEntry entries[3] = {
            {UA_JSONKEY_TYPEID, NULL, NULL, false, NULL},
            {UA_JSONKEY_BODY, (void*)ptr, NULL, false, type},
            {UA_JSONKEY_ENCODING, NULL, NULL, false, NULL}
        };
        status ret = decodeFields(ctx, entries, 3); /* Also skips to the next object */
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
decodeVariantBodyWithType(ParseCtx *ctx, UA_Variant *dst, size_t bodyIndex,
                          size_t *dimIndex, const UA_DataType *type) {
    /* Value is an array? */
    UA_Boolean isArray = (ctx->tokens[bodyIndex].type == CJ5_TOKEN_ARRAY);

    /* TODO: Handling of null-arrays (length -1) needs to be clarified
     *
     * if(tokenIsNull(ctx, bodyIndex)) {
     *     isArray = true;
     *     dst->arrayLength = 0;
     * } */

    /* No array but has dimension -> error */
    if(!isArray && dimIndex)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Get the datatype of the content. The type must be a builtin data type.
     * All not-builtin types are wrapped in an ExtensionObject. */
    if(type->typeKind > UA_DATATYPEKIND_DIAGNOSTICINFO)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* A variant cannot contain a variant. But it can contain an array of
     * variants */
    if(type->typeKind == UA_DATATYPEKIND_VARIANT && !isArray)
        return UA_STATUSCODE_BADDECODINGERROR;

    ctx->depth++;
    ctx->index = bodyIndex;

    /* Decode an array */
    status res = UA_STATUSCODE_GOOD;
    if(isArray) {
        /* Try to unwrap ExtensionObjects in the array.
         * The members must all have the same type. */
        const UA_DataType *unwrapType = NULL;
        if(type == &UA_TYPES[UA_TYPES_EXTENSIONOBJECT] &&
           (unwrapType = getArrayUnwrapType(ctx, bodyIndex))) {
            dst->type = unwrapType;
            res = Array_decodeJsonUnwrapExtensionObject(ctx, &dst->data, unwrapType);
        } else {
            dst->type = type;
            res = Array_decodeJson(ctx, &dst->data, type);
        }

        /* Decode array dimensions */
        if(dimIndex) {
            ctx->index = *dimIndex;
            res |= Array_decodeJson(ctx, (void**)&dst->arrayDimensions, &UA_TYPES[UA_TYPES_UINT32]);
        }
        ctx->depth--;
        return res;
    }

    /* Decode a value wrapped in an ExtensionObject */
    if(type->typeKind == UA_DATATYPEKIND_EXTENSIONOBJECT) {
        res = Variant_decodeJsonUnwrapExtensionObject(ctx, dst, NULL);
        goto out;
    }

    /* Allocate Memory for Body */
    dst->data = UA_new(type);
    if(!dst->data) {
        res = UA_STATUSCODE_BADOUTOFMEMORY;
        goto out;
    }

    /* Decode the body */
    dst->type = type;
    if(ctx->tokens[ctx->index].type != CJ5_TOKEN_NULL)
        res = decodeJsonJumpTable[type->typeKind](ctx, dst->data, type);

 out:
    ctx->depth--;
    return res;
}

DECODE_JSON(Variant) {
    CHECK_NULL_SKIP; /* Treat null as an empty variant */
    CHECK_OBJECT;

    /* First search for the variant type in the json object. */
    size_t typeIndex = 0;
    status ret = lookAheadForKey(ctx, UA_JSONKEY_TYPE, &typeIndex);
    if(ret != UA_STATUSCODE_GOOD) {
        skipObject(ctx);
        return UA_STATUSCODE_GOOD;
    }

    /* Parse the type */
    if(ctx->tokens[typeIndex].type != CJ5_TOKEN_NUMBER)
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_UInt64 idType = 0;
    size_t len = parseUInt64(&ctx->json5[ctx->tokens[typeIndex].start],
                             getTokenLength(&ctx->tokens[typeIndex]), &idType);
    if(len == 0)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* A NULL Variant */
    if(idType == 0) {
        skipObject(ctx);
        return UA_STATUSCODE_GOOD;
    }

    /* Set the type */
    UA_NodeId typeNodeId = UA_NODEID_NUMERIC(0, (UA_UInt32)idType);
    type = UA_findDataTypeWithCustom(&typeNodeId, ctx->customTypes);
    if(!type)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Search for body */
    size_t bodyIndex = 0;
    ret = lookAheadForKey(ctx, UA_JSONKEY_BODY, &bodyIndex);
    if(ret != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Search for the dimensions */
    size_t *dimPtr = NULL;
    size_t dimIndex = 0;
    ret = lookAheadForKey(ctx, UA_JSONKEY_DIMENSION, &dimIndex);
    if(ret == UA_STATUSCODE_GOOD && ctx->tokens[dimIndex].size > 0)
        dimPtr = &dimIndex;

    /* Decode the body */
    return decodeVariantBodyWithType(ctx, dst, bodyIndex, dimPtr, type);
}

DECODE_JSON(DataValue) {
    CHECK_NULL_SKIP; /* Treat a null value as an empty DataValue */
    CHECK_OBJECT;

    DecodeEntry entries[6] = {
        {UA_JSONKEY_VALUE, &dst->value, NULL, false, &UA_TYPES[UA_TYPES_VARIANT]},
        {UA_JSONKEY_STATUS, &dst->status, NULL, false, &UA_TYPES[UA_TYPES_STATUSCODE]},
        {UA_JSONKEY_SOURCETIMESTAMP, &dst->sourceTimestamp, NULL, false, &UA_TYPES[UA_TYPES_DATETIME]},
        {UA_JSONKEY_SOURCEPICOSECONDS, &dst->sourcePicoseconds, NULL, false, &UA_TYPES[UA_TYPES_UINT16]},
        {UA_JSONKEY_SERVERTIMESTAMP, &dst->serverTimestamp, NULL, false, &UA_TYPES[UA_TYPES_DATETIME]},
        {UA_JSONKEY_SERVERPICOSECONDS, &dst->serverPicoseconds, NULL, false, &UA_TYPES[UA_TYPES_UINT16]}
    };

    status ret = decodeFields(ctx, entries, 6);
    dst->hasValue = entries[0].found;
    dst->hasStatus = entries[1].found;
    dst->hasSourceTimestamp = entries[2].found;
    dst->hasSourcePicoseconds = entries[3].found;
    dst->hasServerTimestamp = entries[4].found;
    dst->hasServerPicoseconds = entries[5].found;
    return ret;
}

/* Move the entire current token into the target bytestring */
static UA_StatusCode
tokenToByteString(ParseCtx *ctx, UA_ByteString *p, const UA_DataType *type) {
    GET_TOKEN;
    UA_StatusCode res = UA_ByteString_allocBuffer(p, tokenSize);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    memcpy(p->data, tokenData, tokenSize);
    skipObject(ctx);
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(ExtensionObject) {
    CHECK_NULL_SKIP; /* Treat a null value as an empty DataValue */
    CHECK_OBJECT;

    /* Empty object -> Null ExtensionObject */
    if(ctx->tokens[ctx->index].size == 0) {
        ctx->index++; /* Skip the empty ExtensionObject */
        return UA_STATUSCODE_GOOD;
    }

    /* Search for non-JSON encoding */
    UA_UInt64 encoding = 0;
    size_t encIndex = 0;
    status ret = lookAheadForKey(ctx, UA_JSONKEY_ENCODING, &encIndex);
    if(ret == UA_STATUSCODE_GOOD) {
        const char *extObjEncoding = &ctx->json5[ctx->tokens[encIndex].start];
        size_t len = parseUInt64(extObjEncoding, getTokenLength(&ctx->tokens[encIndex]),
                                 &encoding);
        if(len == 0)
            return UA_STATUSCODE_BADDECODINGERROR;
    }

    /* Lookup the DataType for the ExtensionObject if the body can be decoded */
    const UA_DataType *typeOfBody = (encoding == 0) ? getExtensionObjectType(ctx) : NULL;

    /* Keep the encoded body */
    if(!typeOfBody) {
        DecodeEntry entries[3] = {
            {UA_JSONKEY_ENCODING, NULL, NULL, false, NULL},
            {UA_JSONKEY_TYPEID, &dst->content.encoded.typeId, NULL, false, &UA_TYPES[UA_TYPES_NODEID]},
            {UA_JSONKEY_BODY, &dst->content.encoded.body, NULL, false, &UA_TYPES[UA_TYPES_STRING]}
        };

        if(encoding == 0) {
            entries[2].function = (decodeJsonSignature)tokenToByteString;
            dst->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING; /* ByteString in Json Body */
        } else if(encoding == 1) {
            dst->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING; /* ByteString in Json Body */
        } else if(encoding == 2) {
            dst->encoding = UA_EXTENSIONOBJECT_ENCODED_XML; /* XmlElement in Json Body */
        } else {
            return UA_STATUSCODE_BADDECODINGERROR;
        }
        return decodeFields(ctx, entries, 3);
    }

    /* Allocate memory for the decoded data */
    dst->content.decoded.data = UA_new(typeOfBody);
    if(!dst->content.decoded.data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Set type */
    dst->content.decoded.type = typeOfBody;
    dst->encoding = UA_EXTENSIONOBJECT_DECODED;

    /* Decode body */
    DecodeEntry entries[3] = {
        {UA_JSONKEY_ENCODING, NULL, NULL, false, NULL},
        {UA_JSONKEY_TYPEID, NULL, NULL, false, NULL},
        {UA_JSONKEY_BODY, dst->content.decoded.data, NULL, false, typeOfBody}
    };
    return decodeFields(ctx, entries, 3);
}

static status
Variant_decodeJsonUnwrapExtensionObject(ParseCtx *ctx, void *p, const UA_DataType *type) {
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
    UA_Boolean isBuiltin =
        (eo.content.decoded.type->typeKind <= UA_DATATYPEKIND_DIAGNOSTICINFO);
    if(isBuiltin)
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
    CHECK_NULL_SKIP; /* Treat a null value as an empty DiagnosticInfo */
    CHECK_OBJECT;

    DecodeEntry entries[7] = {
        {UA_JSONKEY_SYMBOLICID, &dst->symbolicId, NULL, false, &UA_TYPES[UA_TYPES_INT32]},
        {UA_JSONKEY_NAMESPACEURI, &dst->namespaceUri, NULL, false, &UA_TYPES[UA_TYPES_INT32]},
        {UA_JSONKEY_LOCALIZEDTEXT, &dst->localizedText, NULL, false, &UA_TYPES[UA_TYPES_INT32]},
        {UA_JSONKEY_LOCALE, &dst->locale, NULL, false, &UA_TYPES[UA_TYPES_INT32]},
        {UA_JSONKEY_ADDITIONALINFO, &dst->additionalInfo, NULL, false, &UA_TYPES[UA_TYPES_STRING]},
        {UA_JSONKEY_INNERSTATUSCODE, &dst->innerStatusCode, NULL, false, &UA_TYPES[UA_TYPES_STATUSCODE]},
        {UA_JSONKEY_INNERDIAGNOSTICINFO, &dst->innerDiagnosticInfo, DiagnosticInfoInner_decodeJson, false, NULL}
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

    if(ctx->depth >= UA_JSON_ENCODING_MAX_RECURSION - 1)
        return UA_STATUSCODE_BADENCODINGERROR;

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

        /* A null-value, skip the decoding (the value is already initialized) */
        if(currentTokenType(ctx) == CJ5_TOKEN_NULL && !entry->function) {
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
Array_decodeJson(ParseCtx *ctx, void **dst, const UA_DataType *type) {
    /* Save the length of the array */
    size_t *size_ptr = (size_t*) dst - 1;

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
    uintptr_t ptr = (uintptr_t)*dst;
    for(size_t i = 0; i < length; ++i) {
        if(ctx->tokens[ctx->index].type != CJ5_TOKEN_NULL) {
            status ret = decodeJsonJumpTable[type->typeKind](ctx, (void*)ptr, type);
            if(ret != UA_STATUSCODE_GOOD) {
                UA_Array_delete(*dst, i+1, type);
                *dst = NULL;
                return ret;
            }
        } else {
            ctx->index++;
        }
        ptr += type->memSize;
    }

    *size_ptr = length; /* All good, set the size */
    return UA_STATUSCODE_GOOD;
}

static status
decodeJsonStructure(ParseCtx *ctx, void *dst, const UA_DataType *type) {
    /* Check the recursion limit */
    if(ctx->depth >= UA_JSON_ENCODING_MAX_RECURSION - 1)
        return UA_STATUSCODE_BADENCODINGERROR;
    ctx->depth++;

    uintptr_t ptr = (uintptr_t)dst;
    status ret = UA_STATUSCODE_GOOD;
    u8 membersSize = type->membersSize;
    UA_STACKARRAY(DecodeEntry, entries, membersSize);
    for(size_t i = 0; i < membersSize; ++i) {
        const UA_DataTypeMember *m = &type->members[i];
        const UA_DataType *mt = m->memberType;
        entries[i].type = mt;
        entries[i].fieldName = m->memberName;
        entries[i].found = false;
        if(!m->isArray) {
            ptr += m->padding;
            entries[i].fieldPointer = (void*)ptr;
            entries[i].function = NULL;
            ptr += mt->memSize;
        } else {
            ptr += m->padding;
            ptr += sizeof(size_t);
            entries[i].fieldPointer = (void*)ptr;
            entries[i].function = (decodeJsonSignature)Array_decodeJson;
            ptr += sizeof(void*);
        }
    }

    ret = decodeFields(ctx, entries, membersSize);

    if(ctx->depth == 0)
        return UA_STATUSCODE_BADENCODINGERROR;
    ctx->depth--;
    return ret;
}

static status
decodeJsonNotImplemented(ParseCtx *ctx, void *dst, const UA_DataType *type) {
    (void)dst, (void)type, (void)ctx;
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

const decodeJsonSignature decodeJsonJumpTable[UA_DATATYPEKINDS] = {
    (decodeJsonSignature)Boolean_decodeJson,
    (decodeJsonSignature)SByte_decodeJson, /* SByte */
    (decodeJsonSignature)Byte_decodeJson,
    (decodeJsonSignature)Int16_decodeJson, /* Int16 */
    (decodeJsonSignature)UInt16_decodeJson,
    (decodeJsonSignature)Int32_decodeJson, /* Int32 */
    (decodeJsonSignature)UInt32_decodeJson,
    (decodeJsonSignature)Int64_decodeJson, /* Int64 */
    (decodeJsonSignature)UInt64_decodeJson,
    (decodeJsonSignature)Float_decodeJson,
    (decodeJsonSignature)Double_decodeJson,
    (decodeJsonSignature)String_decodeJson,
    (decodeJsonSignature)DateTime_decodeJson, /* DateTime */
    (decodeJsonSignature)Guid_decodeJson,
    (decodeJsonSignature)ByteString_decodeJson, /* ByteString */
    (decodeJsonSignature)String_decodeJson, /* XmlElement */
    (decodeJsonSignature)NodeId_decodeJson,
    (decodeJsonSignature)ExpandedNodeId_decodeJson,
    (decodeJsonSignature)StatusCode_decodeJson, /* StatusCode */
    (decodeJsonSignature)QualifiedName_decodeJson, /* QualifiedName */
    (decodeJsonSignature)LocalizedText_decodeJson,
    (decodeJsonSignature)ExtensionObject_decodeJson,
    (decodeJsonSignature)DataValue_decodeJson,
    (decodeJsonSignature)Variant_decodeJson,
    (decodeJsonSignature)DiagnosticInfo_decodeJson,
    (decodeJsonSignature)decodeJsonNotImplemented, /* Decimal */
    (decodeJsonSignature)Int32_decodeJson, /* Enum */
    (decodeJsonSignature)decodeJsonStructure,
    (decodeJsonSignature)decodeJsonNotImplemented, /* Structure with optional fields */
    (decodeJsonSignature)decodeJsonNotImplemented, /* Union */
    (decodeJsonSignature)decodeJsonNotImplemented /* BitfieldCluster */
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

    memset(dst, 0, type->memSize); /* Initialize the value */
    ret = decodeJsonJumpTable[type->typeKind](&ctx, dst, type);

    /* Sanity check if all tokens were processed */
    if(ctx.index != ctx.tokensSize &&
       ctx.index != ctx.tokensSize - 1)
        ret = UA_STATUSCODE_BADDECODINGERROR;

 cleanup:

    /* Free token array on the heap */
    if(ctx.tokens != tokens)
        UA_free((void*)(uintptr_t)ctx.tokens);

    if(ret != UA_STATUSCODE_GOOD)
        UA_clear(dst, type);
    return ret;
}

#endif /* UA_ENABLE_JSON_ENCODING_LEGACY */
