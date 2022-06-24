/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Lukas Meling)
 */

#include <open62541/config.h>
#include <open62541/types_generated.h>
#include <open62541/types_generated_handling.h>

#include "ua_types_encoding_json.h"
#include "ua_types_encoding_binary.h"

#include <float.h>
#include <math.h>

#include "../deps/itoa.h"
#include "../deps/parse_num.h"
#include "../deps/string_escape.h"
#include "../deps/base64.h"
#include "../deps/libc_time.h"

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

/* Max length of numbers for the allocation of temp buffers. Don't forget that
 * printf adds an additional \0 at the end!
 *
 * Sources:
 * https://www.exploringbinary.com/maximum-number-of-decimal-digits-in-binary-floating-point-numbers/
 *
 * UInt16: 3 + 1
 * SByte: 3 + 1
 * UInt32:
 * Int32:
 * UInt64:
 * Int64:
 * Float: 149 + 1
 * Double: 767 + 1
 */

/************/
/* Encoding */
/************/

#define ENCODE_JSON(TYPE) static status \
    TYPE##_encodeJson(const UA_##TYPE *src, const UA_DataType *type, CtxJson *ctx)

#define ENCODE_DIRECT_JSON(SRC, TYPE) \
    TYPE##_encodeJson((const UA_##TYPE*)SRC, NULL, ctx)

/* Forward declarations */
UA_String UA_DateTime_toJSON(UA_DateTime t);
ENCODE_JSON(ByteString);

static status UA_FUNC_ATTR_WARN_UNUSED_RESULT
writeChar(CtxJson *ctx, char c) {
    if(ctx->pos >= ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        *ctx->pos = (UA_Byte)c;
    ctx->pos++;
    return UA_STATUSCODE_GOOD;
}

#define WRITE_JSON_ELEMENT(ELEM)                            \
    UA_FUNC_ATTR_WARN_UNUSED_RESULT status                  \
    writeJson##ELEM(CtxJson *ctx)

static WRITE_JSON_ELEMENT(Quote) {
    return writeChar(ctx, '\"');
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
    ctx->depth--;
    ctx->commaNeeded[ctx->depth] = true;
    return writeChar(ctx, '}');
}

WRITE_JSON_ELEMENT(ArrStart) {
    /* increase depth, save: before first array entry no comma needed. */
    if(ctx->depth >= UA_JSON_ENCODING_MAX_RECURSION - 1)
        return UA_STATUSCODE_BADENCODINGERROR;
    ctx->depth++;
    ctx->commaNeeded[ctx->depth] = false;
    return writeChar(ctx, '[');
}

WRITE_JSON_ELEMENT(ArrEnd) {
    if(ctx->depth == 0)
        return UA_STATUSCODE_BADENCODINGERROR;
    ctx->depth--;
    ctx->commaNeeded[ctx->depth] = true;
    return writeChar(ctx, ']');
}

WRITE_JSON_ELEMENT(CommaIfNeeded) {
    if(ctx->commaNeeded[ctx->depth])
        return writeChar(ctx, ',');
    return UA_STATUSCODE_GOOD;
}

status
writeJsonArrElm(CtxJson *ctx, const void *value,
                const UA_DataType *type) {
    status ret = writeJsonCommaIfNeeded(ctx);
    ctx->commaNeeded[ctx->depth] = true;
    ret |= encodeJsonInternal(value, type, ctx);
    return ret;
}

status
writeJsonObjElm(CtxJson *ctx, const char *key,
                const void *value, const UA_DataType *type) {
    return writeJsonKey(ctx, key) | encodeJsonInternal(value, type, ctx);
}

status
writeJsonNull(CtxJson *ctx) {
    if(ctx->pos + 4 > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(ctx->calcOnly) {
        ctx->pos += 4;
    } else {
        *(ctx->pos++) = 'n';
        *(ctx->pos++) = 'u';
        *(ctx->pos++) = 'l';
        *(ctx->pos++) = 'l';
    }
    return UA_STATUSCODE_GOOD;
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
    size_t size = strlen(key);
    if(ctx->pos + size + 4 > ctx->end) /* +4 because of " " : and , */
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    status ret = writeJsonCommaIfNeeded(ctx);
    ctx->commaNeeded[ctx->depth] = true;
    if(ctx->calcOnly) {
        ctx->pos += 3;
        ctx->pos += size;
        return ret;
    }

    ret |= writeChar(ctx, '\"');
    for(size_t i = 0; i < size; i++) {
        *(ctx->pos++) = (u8)key[i];
    }
    ret |= writeChar(ctx, '\"');
    ret |= writeChar(ctx, ':');
    return ret;
}

static bool
isNull(const void *p, const UA_DataType *type) {
    if(UA_DataType_isNumeric(type) || type->typeKind == UA_DATATYPEKIND_BOOLEAN)
        return false;
    UA_STACKARRAY(char, buf, type->memSize);
    memset(buf, 0, type->memSize);
    return (UA_order(buf, p, type) == UA_ORDER_EQ);
}

/* Boolean */
ENCODE_JSON(Boolean) {
    size_t sizeOfJSONBool;
    if(*src == true) {
        sizeOfJSONBool = 4; /* true */
    } else {
        sizeOfJSONBool = 5; /* false */
    }

    if(ctx->calcOnly) {
        ctx->pos += sizeOfJSONBool;
        return UA_STATUSCODE_GOOD;
    }

    if(ctx->pos + sizeOfJSONBool > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;

    if(*src) {
        *(ctx->pos++) = 't';
        *(ctx->pos++) = 'r';
        *(ctx->pos++) = 'u';
        *(ctx->pos++) = 'e';
    } else {
        *(ctx->pos++) = 'f';
        *(ctx->pos++) = 'a';
        *(ctx->pos++) = 'l';
        *(ctx->pos++) = 's';
        *(ctx->pos++) = 'e';
    }
    return UA_STATUSCODE_GOOD;
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
    char buffer[200];
    if(*src != *src) {
        strcpy(buffer, "\"NaN\"");
    } else if(*src == INFINITY) {
        strcpy(buffer, "\"Infinity\"");
    } else if(*src == -INFINITY) {
        strcpy(buffer, "\"-Infinity\"");
    } else {
        UA_snprintf(buffer, 200, "%.149g", (UA_Double)*src);
    }

    size_t len = strlen(buffer);
    if(len == 0)
        return UA_STATUSCODE_BADENCODINGERROR;

    if(ctx->pos + len > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;

    if(!ctx->calcOnly)
        memcpy(ctx->pos, buffer, len);

    ctx->pos += len;
    return UA_STATUSCODE_GOOD;
}

ENCODE_JSON(Double) {
    char buffer[2000];
    if(*src != *src) {
        strcpy(buffer, "\"NaN\"");
    } else if(*src == INFINITY) {
        strcpy(buffer, "\"Infinity\"");
    } else if(*src == -INFINITY) {
        strcpy(buffer, "\"-Infinity\"");
    } else {
        UA_snprintf(buffer, 2000, "%.1074g", *src);
    }

    size_t len = strlen(buffer);
    if(len == 0)
        return UA_STATUSCODE_BADENCODINGERROR;

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
        return ret | writeJsonArrEnd(ctx);

    uintptr_t uptr = (uintptr_t)ptr;
    encodeJsonSignature encodeType = encodeJsonJumpTable[type->typeKind];
    for(size_t i = 0; i < length && ret == UA_STATUSCODE_GOOD; ++i) {
        ret |= writeJsonCommaIfNeeded(ctx);
        if(isNull((const void*)uptr, type))
            ret |= writeJsonNull(ctx); /* null values are written as "null" */
        else
            ret |= encodeType((const void*)uptr, type, ctx);
        ctx->commaNeeded[ctx->depth] = true;
        uptr += type->memSize;
    }
    ret |= writeJsonArrEnd(ctx);
    return ret;
}

static const u8 hexmapLower[16] =
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
static const u8 hexmapUpper[16] =
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

ENCODE_JSON(String) {
    if(!src->data)
        return writeJsonNull(ctx);

    if(src->length == 0)
        return writeJsonQuote(ctx) | writeJsonQuote(ctx);

    UA_StatusCode ret = writeJsonQuote(ctx);

    /* Escaping adapted from https://github.com/akheron/jansson dump.c */

    const char *str = (char*)src->data;
    const char *pos = str;
    const char *end = str;
    const char *lim = str + src->length;
    UA_UInt32 codepoint = 0;
    while(1) {
        const char *text;
        u8 seq[13];
        size_t length;

        while(end < lim) {
            end = utf8_iterate(pos, (size_t)(lim - pos), (int32_t *)&codepoint);
            if(!end)  {
                /* A malformed utf8 character. Print anyway and let the
                 * receiving side choose how to handle it. */
                pos++;
                end = pos;
                continue;
            }

            /* mandatory escape or control char */
            if(codepoint == '\\' || codepoint == '"' || codepoint < 0x20)
                break;

            /* TODO: Why is this commented? */
            /* slash
            if((flags & JSON_ESCAPE_SLASH) && codepoint == '/')
                break;*/

            /* non-ASCII
            if((flags & JSON_ENSURE_ASCII) && codepoint > 0x7F)
                break;*/

            pos = end;
        }

        if(pos != str) {
            if(ctx->pos + (pos - str) > ctx->end)
                return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
            if(!ctx->calcOnly)
                memcpy(ctx->pos, str, (size_t)(pos - str));
            ctx->pos += pos - str;
        }

        if(end == pos)
            break;

        /* handle \, /, ", and control codes */
        length = 2;
        switch(codepoint) {
        case '\\': text = "\\\\"; break;
        case '\"': text = "\\\""; break;
        case '\b': text = "\\b"; break;
        case '\f': text = "\\f"; break;
        case '\n': text = "\\n"; break;
        case '\r': text = "\\r"; break;
        case '\t': text = "\\t"; break;
        case '/':  text = "\\/"; break;
        default:
            if(codepoint < 0x10000) {
                /* codepoint is in BMP */
                seq[0] = '\\';
                seq[1] = 'u';
                UA_Byte b1 = (UA_Byte)(codepoint >> 8u);
                UA_Byte b2 = (UA_Byte)(codepoint >> 0u);
                seq[2] = hexmapLower[(b1 & 0xF0u) >> 4u];
                seq[3] = hexmapLower[b1 & 0x0Fu];
                seq[4] = hexmapLower[(b2 & 0xF0u) >> 4u];
                seq[5] = hexmapLower[b2 & 0x0Fu];
                length = 6;
            } else {
                /* not in BMP -> construct a UTF-16 surrogate pair */
                codepoint -= 0x10000;
                UA_UInt32 first = 0xD800u | ((codepoint & 0xffc00u) >> 10u);
                UA_UInt32 last = 0xDC00u | (codepoint & 0x003ffu);

                UA_Byte fb1 = (UA_Byte)(first >> 8u);
                UA_Byte fb2 = (UA_Byte)(first >> 0u);

                UA_Byte lb1 = (UA_Byte)(last >> 8u);
                UA_Byte lb2 = (UA_Byte)(last >> 0u);

                seq[0] = '\\';
                seq[1] = 'u';
                seq[2] = hexmapLower[(fb1 & 0xF0u) >> 4u];
                seq[3] = hexmapLower[fb1 & 0x0Fu];
                seq[4] = hexmapLower[(fb2 & 0xF0u) >> 4u];
                seq[5] = hexmapLower[fb2 & 0x0Fu];

                seq[6] = '\\';
                seq[7] = 'u';
                seq[8] = hexmapLower[(lb1 & 0xF0u) >> 4u];
                seq[9] = hexmapLower[lb1 & 0x0Fu];
                seq[10] = hexmapLower[(lb2 & 0xF0u) >> 4u];
                seq[11] = hexmapLower[lb2 & 0x0Fu];
                length = 12;
            }
            text = (char*)seq;
            break;
        }

        if(ctx->pos + length > ctx->end)
            return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
        if(!ctx->calcOnly)
            memcpy(ctx->pos, text, length);
        ctx->pos += length;
        str = pos = end;
    }

    ret |= writeJsonQuote(ctx);
    return ret;
}

ENCODE_JSON(ByteString) {
    if(!src->data)
        return writeJsonNull(ctx);

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

    ret |= writeJsonQuote(ctx);
    return ret;
}

/* Converts Guid to a hexadecimal represenation */
static void UA_Guid_to_hex(const UA_Guid *guid, u8* out) {
    /*
                          16 byte
       +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
       |   data1   |data2|data3|          data4        |
       +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
       |aa aa aa aa-bb bb-cc cc-dd dd-ee ee ee ee ee ee|
       +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                          36 character
    */

#ifdef hexCharlowerCase
    const u8 *hexmap = hexmapLower;
#else
    const u8 *hexmap = hexmapUpper;
#endif
    size_t i = 0, j = 28;
    for(; i<8;i++,j-=4)         /* pos 0-7, 4byte, (a) */
        out[i] = hexmap[(guid->data1 >> j) & 0x0Fu];
    out[i++] = '-';             /* pos 8 */
    for(j=12; i<13;i++,j-=4)    /* pos 9-12, 2byte, (b) */
        out[i] = hexmap[(uint16_t)(guid->data2 >> j) & 0x0Fu];
    out[i++] = '-';             /* pos 13 */
    for(j=12; i<18;i++,j-=4)    /* pos 14-17, 2byte (c) */
        out[i] = hexmap[(uint16_t)(guid->data3 >> j) & 0x0Fu];
    out[i++] = '-';             /* pos 18 */
    for(j=0;i<23;i+=2,j++) {     /* pos 19-22, 2byte (d) */
        out[i] = hexmap[(guid->data4[j] & 0xF0u) >> 4u];
        out[i+1] = hexmap[guid->data4[j] & 0x0Fu];
    }
    out[i++] = '-';             /* pos 23 */
    for(j=2; i<36;i+=2,j++) {    /* pos 24-35, 6byte (e) */
        out[i] = hexmap[(guid->data4[j] & 0xF0u) >> 4u];
        out[i+1] = hexmap[guid->data4[j] & 0x0Fu];
    }
}

/* Guid */
ENCODE_JSON(Guid) {
    if(ctx->pos + 38 > ctx->end) /* 36 + 2 (") */
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    status ret = writeJsonQuote(ctx);
    u8 *buf = ctx->pos;
    if(!ctx->calcOnly)
        UA_Guid_to_hex(src, buf);
    ctx->pos += 36;
    ret |= writeJsonQuote(ctx);
    return ret;
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
NodeId_encodeJsonInternal(UA_NodeId const *src, CtxJson *ctx) {
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
    UA_StatusCode ret = writeJsonObjStart(ctx);
    ret |= NodeId_encodeJsonInternal(src, ctx);
    if(ctx->useReversible) {
        if(src->namespaceIndex > 0) {
            ret |= writeJsonKey(ctx, UA_JSONKEY_NAMESPACE);
            ret |= ENCODE_DIRECT_JSON(&src->namespaceIndex, UInt16);
        }
    } else {
        /* For the non-reversible encoding, the field is the NamespaceUri
         * associated with the NamespaceIndex, encoded as a JSON string.
         * A NamespaceIndex of 1 is always encoded as a JSON number. */
        if(src->namespaceIndex == 1) {
            ret |= writeJsonKey(ctx, UA_JSONKEY_NAMESPACE);
            ret |= ENCODE_DIRECT_JSON(&src->namespaceIndex, UInt16);
        } else {
            ret |= writeJsonKey(ctx, UA_JSONKEY_NAMESPACE);

            /* Check if Namespace given and in range */
            if(src->namespaceIndex < ctx->namespacesSize && ctx->namespaces != NULL) {
                UA_String namespaceEntry = ctx->namespaces[src->namespaceIndex];
                ret |= ENCODE_DIRECT_JSON(&namespaceEntry, String);
            } else {
                return UA_STATUSCODE_BADNOTFOUND;
            }
        }
    }

    ret |= writeJsonObjEnd(ctx);
    return ret;
}

/* ExpandedNodeId */
ENCODE_JSON(ExpandedNodeId) {
    status ret = writeJsonObjStart(ctx);

    /* Encode the NodeId */
    ret |= NodeId_encodeJsonInternal(&src->nodeId, ctx);

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

        if(src->namespaceUri.data) {
            ret |= writeJsonKey(ctx, UA_JSONKEY_NAMESPACE);
            ret |= ENCODE_DIRECT_JSON(&src->namespaceUri, String);
        } else {
            if(src->nodeId.namespaceIndex == 1) {
                ret |= writeJsonKey(ctx, UA_JSONKEY_NAMESPACE);
                ret |= ENCODE_DIRECT_JSON(&src->nodeId.namespaceIndex, UInt16);
            } else {
                /* Check if Namespace given and in range */
                if(src->nodeId.namespaceIndex >= ctx->namespacesSize || !ctx->namespaces)
                    return UA_STATUSCODE_BADNOTFOUND;
                UA_String namespaceEntry = ctx->namespaces[src->nodeId.namespaceIndex];
                ret |= writeJsonKey(ctx, UA_JSONKEY_NAMESPACE);
                ret |= ENCODE_DIRECT_JSON(&namespaceEntry, String);
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

    ret |= writeJsonObjEnd(ctx);
    return ret;
}

/* LocalizedText */
ENCODE_JSON(LocalizedText) {
    if(ctx->useReversible) {
        status ret = writeJsonObjStart(ctx);
        ret |= writeJsonKey(ctx, UA_JSONKEY_LOCALE);
        ret |= ENCODE_DIRECT_JSON(&src->locale, String);
        ret |= writeJsonKey(ctx, UA_JSONKEY_TEXT);
        ret |= ENCODE_DIRECT_JSON(&src->text, String);
        ret |= writeJsonObjEnd(ctx);
        return ret;
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
        if(src->namespaceIndex == 1) {
            ret |= writeJsonKey(ctx, UA_JSONKEY_URI);
            ret |= ENCODE_DIRECT_JSON(&src->namespaceIndex, UInt16);
        } else {
            ret |= writeJsonKey(ctx, UA_JSONKEY_URI);

             /* Check if Namespace given and in range */
            if(src->namespaceIndex < ctx->namespacesSize && ctx->namespaces != NULL) {
                UA_String namespaceEntry = ctx->namespaces[src->namespaceIndex];
                ret |= ENCODE_DIRECT_JSON(&namespaceEntry, String);
            } else {
                /* If not encode as number */
                ret |= ENCODE_DIRECT_JSON(&src->namespaceIndex, UInt16);
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
        return writeJsonNull(ctx);

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
       src->encoding == UA_EXTENSIONOBJECT_ENCODED_XML)
        ret |= ENCODE_DIRECT_JSON(&src->content.encoded.body, String);
    else
        ret |= encodeJsonInternal(src->content.decoded.data,
                                  src->content.decoded.type, ctx);

    ret |= writeJsonObjEnd(ctx);
    return ret;
}

static status
Variant_encodeJsonWrapExtensionObject(const UA_Variant *src, const bool isArray,
                                      CtxJson *ctx) {
    size_t length = 1;
    if(isArray) {
        if(src->arrayLength > UA_INT32_MAX)
            return UA_STATUSCODE_BADENCODINGERROR;

        length = src->arrayLength;
    }

    /* Set up a temporary ExtensionObject to wrap the data */
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    eo.encoding = UA_EXTENSIONOBJECT_DECODED;
    eo.content.decoded.type = src->type;

    if(isArray) {
        u16 memSize = src->type->memSize;
        uintptr_t ptr = (uintptr_t)src->data;
        status ret = writeJsonArrStart(ctx);
        for(size_t i = 0; i < length && ret == UA_STATUSCODE_GOOD; ++i) {
            eo.content.decoded.data = (void*)ptr;
            ret |= writeJsonArrElm(ctx, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
            ptr += memSize;
        }
        return ret | writeJsonArrEnd(ctx);
    }

    eo.content.decoded.data = src->data;
    return ExtensionObject_encodeJson(&eo, NULL, ctx);
}

static status
addMultiArrayContentJSON(CtxJson *ctx, void* array, const UA_DataType *type,
                         size_t *index, UA_UInt32 *arrayDimensions, size_t dimensionIndex,
                         size_t dimensionSize) {
    /* Stop recursion: The inner Arrays are written */
    status ret;
    if(dimensionIndex == (dimensionSize - 1)) {
        ret = encodeJsonArray(ctx, ((u8*)array) + (type->memSize * *index),
                              arrayDimensions[dimensionIndex], type);
        (*index) += arrayDimensions[dimensionIndex];
        return ret;
    }

    /* Recurse to the next dimension */
    ret = writeJsonArrStart(ctx);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;
    for(size_t i = 0; i < arrayDimensions[dimensionIndex]; i++) {
        ret |= writeJsonCommaIfNeeded(ctx);
        ret |= addMultiArrayContentJSON(ctx, array, type, index, arrayDimensions,
                                        dimensionIndex + 1, dimensionSize);
        ctx->commaNeeded[ctx->depth] = true;
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
    }
    ret |= writeJsonArrEnd(ctx);
    return ret;
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

    status ret = writeJsonObjStart(ctx);

    if(ctx->useReversible) {
        /* Write the NodeId */
        UA_UInt32 typeId = src->type->typeId.identifier.numeric;
        if(!isBuiltin)
            typeId = UA_TYPES[UA_TYPES_EXTENSIONOBJECT].typeId.identifier.numeric;
        ret |= writeJsonKey(ctx, UA_JSONKEY_TYPE);
        ret |= ENCODE_DIRECT_JSON(&typeId, UInt32);

        /* Write the reversible form body */
        if(!isBuiltin) {
            /* Not builtin. Can it be encoded? Wrap in extension object. */
            if(src->arrayDimensionsSize > 1)
                return UA_STATUSCODE_BADNOTIMPLEMENTED;
            ret |= writeJsonKey(ctx, UA_JSONKEY_BODY);
            ret |= Variant_encodeJsonWrapExtensionObject(src, isArray, ctx);
        } else if(!isArray) {
            ret |= writeJsonKey(ctx, UA_JSONKEY_BODY);
            ret |= encodeJsonInternal(src->data, src->type, ctx);
        } else {
            ret |= writeJsonKey(ctx, UA_JSONKEY_BODY);
            ret |= encodeJsonArray(ctx, src->data, src->arrayLength, src->type);
        }

        /* Write the dimensions */
        if(hasDimensions) {
            ret |= writeJsonKey(ctx, UA_JSONKEY_DIMENSION);
            ret |= encodeJsonArray(ctx, src->arrayDimensions, src->arrayDimensionsSize,
                                   &UA_TYPES[UA_TYPES_INT32]);
        }
    } else {
        /* Non-Reversible form. Variant values encoded as a JSON object
         * containing only the value of the Body field. The Type and Dimensions
         * fields are dropped. Multi-dimensional arrays are encoded as a multi
         * dimensional JSON array as described in 5.4.5. */
        if(!isBuiltin) {
            /* Not builtin. Can it be encoded? Wrap in extension object. */
            if(src->arrayDimensionsSize > 1)
                return UA_STATUSCODE_BADNOTIMPLEMENTED;
            ret |= writeJsonKey(ctx, UA_JSONKEY_BODY);
            ret |= Variant_encodeJsonWrapExtensionObject(src, isArray, ctx);
        } else if(!isArray) {
            ret |= writeJsonKey(ctx, UA_JSONKEY_BODY);
            ret |= encodeJsonInternal(src->data, src->type, ctx);
        } else {
            ret |= writeJsonKey(ctx, UA_JSONKEY_BODY);
            if(src->arrayDimensionsSize > 1) {
                size_t index = 0;
                size_t dimensionIndex = 0;
                ret |= addMultiArrayContentJSON(ctx, src->data, src->type, &index,
                                                src->arrayDimensions, dimensionIndex,
                                                src->arrayDimensionsSize);
            } else {
                ret |= encodeJsonArray(ctx, src->data, src->arrayLength, src->type);
            }
        }
    }

    ret |= writeJsonObjEnd(ctx);
    return ret;
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
        ret |= ENCODE_DIRECT_JSON(&src->symbolicId, UInt32);
    }

    if(src->hasNamespaceUri) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_NAMESPACEURI);
        ret |= ENCODE_DIRECT_JSON(&src->namespaceUri, UInt32);
    }

    if(src->hasLocalizedText) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_LOCALIZEDTEXT);
        ret |= ENCODE_DIRECT_JSON(&src->localizedText, UInt32);
    }

    if(src->hasLocale) {
        ret |= writeJsonKey(ctx, UA_JSONKEY_LOCALE);
        ret |= ENCODE_DIRECT_JSON(&src->locale, UInt32);
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
        ret |= encodeJsonInternal(src->innerDiagnosticInfo,
                                  &UA_TYPES[UA_TYPES_DIAGNOSTICINFO], ctx);
    }

    return ret | writeJsonObjEnd(ctx);
}

static status
encodeJsonStructure(const void *src, const UA_DataType *type, CtxJson *ctx) {
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
            ret |= encodeJsonJumpTable[mt->typeKind]((const void*) ptr, mt, ctx);
            ptr += memSize;
        } else {
            ptr += m->padding;
            const size_t length = *((const size_t*) ptr);
            ptr += sizeof (size_t);
            ret |= encodeJsonArray(ctx, *(void * const *)ptr, length, mt);
            ptr += sizeof (void*);
        }
    }

    ret |= writeJsonObjEnd(ctx);
    return ret;
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

status
encodeJsonInternal(const void *src, const UA_DataType *type, CtxJson *ctx) {
    return encodeJsonJumpTable[type->typeKind](src, type, ctx);
}

status UA_FUNC_ATTR_WARN_UNUSED_RESULT
UA_encodeJsonInternal(const void *src, const UA_DataType *type,
              u8 **bufPos, const u8 **bufEnd, const UA_String *namespaces,
              size_t namespaceSize, const UA_String *serverUris,
              size_t serverUriSize, UA_Boolean useReversible) {
    if(!src || !type)
        return UA_STATUSCODE_BADINTERNALERROR;

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

    /* Encode */
    status ret = encodeJsonJumpTable[type->typeKind](src, type, &ctx);

    *bufPos = ctx.pos;
    *bufEnd = ctx.end;
    return ret;
}

UA_StatusCode
UA_encodeJson(const void *src, const UA_DataType *type, UA_ByteString *outBuf,
              const UA_EncodeJsonOptions *options) {
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

    /* Encode */
    u8 *pos = outBuf->data;
    const u8 *posEnd = &outBuf->data[outBuf->length];
    if(options) {
        res = UA_encodeJsonInternal(src, type, &pos, &posEnd, options->namespaces,
                                    options->namespacesSize, options->serverUris,
                                    options->serverUrisSize, options->useReversible);
    } else {
        res = UA_encodeJsonInternal(src, type, &pos, &posEnd, NULL, 0u, NULL, 0u, true);
    }

    /* Clean up */
    if(res == UA_STATUSCODE_GOOD) {
        outBuf->length = (size_t)((uintptr_t)pos - (uintptr_t)outBuf->data);
    } else if(allocated) {
        UA_ByteString_clear(outBuf);
    }
    return res;
}

/************/
/* CalcSize */
/************/

size_t
UA_calcSizeJsonInternal(const void *src, const UA_DataType *type,
                const UA_String *namespaces, size_t namespaceSize,
                const UA_String *serverUris, size_t serverUriSize,
                UA_Boolean useReversible) {
    if(!src || !type)
        return UA_STATUSCODE_BADINTERNALERROR;

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

    /* Encode */
    status ret = encodeJsonJumpTable[type->typeKind](src, type, &ctx);
    if(ret != UA_STATUSCODE_GOOD)
        return 0;
    return (size_t)ctx.pos;
}

size_t
UA_calcSizeJson(const void *src, const UA_DataType *type,
                const UA_EncodeJsonOptions *options) {
    if(options) {
        return UA_calcSizeJsonInternal(src, type, options->namespaces,
                                       options->namespacesSize, options->serverUris,
                                       options->serverUrisSize, options->useReversible);
    }
    return UA_calcSizeJsonInternal(src, type, &UA_STRING_NULL, 0u, &UA_STRING_NULL, 0u,
                                   true);
}

/**********/
/* Decode */
/**********/

/* Macro which gets current size and char pointer of current Token. Needs
 * ParseCtx (parseCtx) and CtxJson (ctx). Does NOT increment index of Token. */
#define GET_TOKEN                                                             \
    size_t tokenSize = (size_t)(parseCtx->tokenArray[parseCtx->index].end -   \
                                parseCtx->tokenArray[parseCtx->index].start); \
    char* tokenData = (char*)(ctx->pos + parseCtx->tokenArray[parseCtx->index].start); \
    do {} while(0)

#define CHECK_TOKEN_BOUNDS do {                   \
    if(parseCtx->index >= parseCtx->tokenCount)   \
        return UA_STATUSCODE_BADDECODINGERROR;    \
    } while(0)

#define CHECK_PRIMITIVE do {                      \
    if(getJsmnType(parseCtx) != JSMN_PRIMITIVE) { \
        return UA_STATUSCODE_BADDECODINGERROR;    \
    }} while(0)

#define CHECK_STRING do {                      \
    if(getJsmnType(parseCtx) != JSMN_STRING) { \
        return UA_STATUSCODE_BADDECODINGERROR; \
    }} while(0)

#define CHECK_OBJECT do {                      \
    if(getJsmnType(parseCtx) != JSMN_OBJECT) { \
        return UA_STATUSCODE_BADDECODINGERROR; \
    }} while(0)

/* Forward declarations*/
#define DECODE_JSON(TYPE) static status                        \
    TYPE##_decodeJson(UA_##TYPE *dst, const UA_DataType *type, \
                      CtxJson *ctx, ParseCtx *parseCtx)

/* If parseCtx->index points to the beginning of an object, move the index to
 * the next token after this object. Attention! The index can be moved after the
 * last parsed token. So the array length has to be checked afterwards. */
static void
skipObject(ParseCtx *parseCtx) {
    int end = parseCtx->tokenArray[parseCtx->index].end;
    do {
        parseCtx->index++;
    } while(parseCtx->index < parseCtx->tokenCount &&
            parseCtx->tokenArray[parseCtx->index].start < end);
}

static status
Array_decodeJson(void **dst, const UA_DataType *type,
                 CtxJson *ctx, ParseCtx *parseCtx);

static status
Variant_decodeJsonUnwrapExtensionObject(void *p, const UA_DataType *type,
                                        CtxJson *ctx, ParseCtx *parseCtx);

UA_Boolean
isJsonNull(const CtxJson *ctx, const ParseCtx *parseCtx) {
    if(parseCtx->index >= parseCtx->tokenCount)
        return false;

    if(parseCtx->tokenArray[parseCtx->index].type != JSMN_PRIMITIVE) {
        return false;
    }
    char* elem = (char*)(ctx->pos + parseCtx->tokenArray[parseCtx->index].start);
    return (elem[0] == 'n' && elem[1] == 'u' && elem[2] == 'l' && elem[3] == 'l');
}

static UA_SByte jsoneq(const char *json, jsmntok_t *tok, const char *searchKey) {
    /* TODO: necessary?
       if(json == NULL
            || tok == NULL
            || searchKey == NULL) {
        return -1;
    } */

    if(tok->type == JSMN_STRING) {
         if(strlen(searchKey) == (size_t)(tok->end - tok->start) ) {
             if(strncmp(json + tok->start,
                        (const char*)searchKey, (size_t)(tok->end - tok->start)) == 0) {
                 return 0;
             }
         }
    }
    return -1;
}

DECODE_JSON(Boolean) {
    CHECK_TOKEN_BOUNDS;
    CHECK_PRIMITIVE;
    GET_TOKEN;

    if(tokenSize == 4 &&
       tokenData[0] == 't' && tokenData[1] == 'r' &&
       tokenData[2] == 'u' && tokenData[3] == 'e') {
        *dst = true;
    } else if(tokenSize == 5 &&
              tokenData[0] == 'f' && tokenData[1] == 'a' &&
              tokenData[2] == 'l' && tokenData[3] == 's' &&
              tokenData[4] == 'e') {
        *dst = false;
    } else {
        return UA_STATUSCODE_BADDECODINGERROR;
    }

    parseCtx->index++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
parseUnsignedInteger(char *tokenData, size_t tokenSize, UA_UInt64 *dst) {
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
parseSignedInteger(char *tokenData, size_t tokenSize, UA_Int64 *dst) {
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
    CHECK_PRIMITIVE;
    GET_TOKEN;

    UA_UInt64 out = 0;
    UA_StatusCode s = parseUnsignedInteger(tokenData, tokenSize, &out);
    if(s != UA_STATUSCODE_GOOD || out > UA_BYTE_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (UA_Byte)out;
    parseCtx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(UInt16) {
    CHECK_TOKEN_BOUNDS;
    CHECK_PRIMITIVE;
    GET_TOKEN;

    UA_UInt64 out = 0;
    UA_StatusCode s = parseUnsignedInteger(tokenData, tokenSize, &out);
    if(s != UA_STATUSCODE_GOOD || out > UA_UINT16_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (UA_UInt16)out;
    parseCtx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(UInt32) {
    CHECK_TOKEN_BOUNDS;
    CHECK_PRIMITIVE;
    GET_TOKEN;

    UA_UInt64 out = 0;
    UA_StatusCode s = parseUnsignedInteger(tokenData, tokenSize, &out);
    if(s != UA_STATUSCODE_GOOD || out > UA_UINT32_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (UA_UInt32)out;
    parseCtx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(UInt64) {
    CHECK_TOKEN_BOUNDS;
    CHECK_STRING;
    GET_TOKEN;

    UA_StatusCode s = parseUnsignedInteger(tokenData, tokenSize, dst);
    if(s != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;
    parseCtx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(SByte) {
    CHECK_TOKEN_BOUNDS;
    CHECK_PRIMITIVE;
    GET_TOKEN;

    UA_Int64 out = 0;
    UA_StatusCode s = parseSignedInteger(tokenData, tokenSize, &out);
    if(s != UA_STATUSCODE_GOOD || out < UA_SBYTE_MIN || out > UA_SBYTE_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (UA_SByte)out;
    parseCtx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(Int16) {
    CHECK_TOKEN_BOUNDS;
    CHECK_PRIMITIVE;
    GET_TOKEN;

    UA_Int64 out = 0;
    UA_StatusCode s = parseSignedInteger(tokenData, tokenSize, &out);
    if(s != UA_STATUSCODE_GOOD || out < UA_INT16_MIN || out > UA_INT16_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (UA_Int16)out;
    parseCtx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(Int32) {
    CHECK_TOKEN_BOUNDS;
    CHECK_PRIMITIVE;
    GET_TOKEN;

    UA_Int64 out = 0;
    UA_StatusCode s = parseSignedInteger(tokenData, tokenSize, &out);
    if(s != UA_STATUSCODE_GOOD || out < UA_INT32_MIN || out > UA_INT32_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (UA_Int32)out;
    parseCtx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(Int64) {
    CHECK_TOKEN_BOUNDS;
    CHECK_STRING;
    GET_TOKEN;

    UA_StatusCode s = parseSignedInteger(tokenData, tokenSize, dst);
    if(s != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;
    parseCtx->index++;
    return UA_STATUSCODE_GOOD;
}

static UA_UInt32 hex2int(char ch) {
    if(ch >= '0' && ch <= '9')
        return (UA_UInt32)(ch - '0');
    if(ch >= 'A' && ch <= 'F')
        return (UA_UInt32)(ch - 'A' + 10);
    if(ch >= 'a' && ch <= 'f')
        return (UA_UInt32)(ch - 'a' + 10);
    return 0;
}

/* Either a JSMN_STRING or JSMN_PRIMITIVE */
DECODE_JSON(Double) {
    CHECK_TOKEN_BOUNDS;
    GET_TOKEN;

    /* https://www.exploringbinary.com/maximum-number-of-decimal-digits-in-binary-floating-point-numbers/
     * Maximum digit counts for select IEEE floating-point formats: 1074
     * Sanity check.
     */
    if(tokenSize > 1075)
        return UA_STATUSCODE_BADDECODINGERROR;

    jsmntype_t tokenType = getJsmnType(parseCtx);

    /* It could be a String with Nan, Infinity */
    if(tokenType == JSMN_STRING) {
        parseCtx->index++;

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

    if(tokenType != JSMN_PRIMITIVE)
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

    parseCtx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(Float) {
    UA_Double v = 0.0;
    UA_StatusCode res = Double_decodeJson(&v, NULL, ctx, parseCtx);
    *dst = (UA_Float)v;
    return res;
}

/*
  Expects 36 chars in format    00000003-0009-000A-0807-060504030201
                                | data1| |d2| |d3| |d4| |  data4   |
*/
static UA_Guid UA_Guid_fromChars(const char* chars) {
    UA_Guid dst;
    UA_Guid_init(&dst);
    for(size_t i = 0; i < 8; i++)
        dst.data1 |= (UA_UInt32)(hex2int(chars[i]) << (28 - (i*4)));
    for(size_t i = 0; i < 4; i++) {
        dst.data2 |= (UA_UInt16)(hex2int(chars[9+i]) << (12 - (i*4)));
        dst.data3 |= (UA_UInt16)(hex2int(chars[14+i]) << (12 - (i*4)));
    }
    dst.data4[0] |= (UA_Byte)(hex2int(chars[19]) << 4u);
    dst.data4[0] |= (UA_Byte)(hex2int(chars[20]) << 0u);
    dst.data4[1] |= (UA_Byte)(hex2int(chars[21]) << 4u);
    dst.data4[1] |= (UA_Byte)(hex2int(chars[22]) << 0u);
    for(size_t i = 0; i < 6; i++) {
        dst.data4[2+i] |= (UA_Byte)(hex2int(chars[24 + i*2]) << 4u);
        dst.data4[2+i] |= (UA_Byte)(hex2int(chars[25 + i*2]) << 0u);
    }
    return dst;
}

DECODE_JSON(Guid) {
    CHECK_TOKEN_BOUNDS;
    CHECK_STRING;
    GET_TOKEN;

    if(tokenSize != 36)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* check if incorrect chars are present */
    for(size_t i = 0; i < tokenSize; i++) {
        if(!(tokenData[i] == '-' ||
             (tokenData[i] >= '0' && tokenData[i] <= '9') ||
             (tokenData[i] >= 'A' && tokenData[i] <= 'F') ||
             (tokenData[i] >= 'a' && tokenData[i] <= 'f'))) {
            return UA_STATUSCODE_BADDECODINGERROR;
        }
    }

    *dst = UA_Guid_fromChars(tokenData);

    parseCtx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(String) {
    CHECK_TOKEN_BOUNDS;
    CHECK_STRING;
    GET_TOKEN;

    /* Empty string? */
    if(tokenSize == 0) {
        dst->data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
        dst->length = 0;
        parseCtx->index++;
        return UA_STATUSCODE_GOOD;
    }

    /* The actual value is at most of the same length as the source string:
     * - Shortcut escapes (e.g. "\t") (length 2) are converted to 1 byte
     * - A single \uXXXX escape (length 6) is converted to at most 3 bytes
     * - Two \uXXXX escapes (length 12) forming an UTF-16 surrogate pair are
     *   converted to 4 bytes */
    uint8_t *outputBuffer = (uint8_t*)UA_malloc(tokenSize);
    if(!outputBuffer)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    const uint8_t *p = (uint8_t*)tokenData;
    const uint8_t *end = (uint8_t*)&tokenData[tokenSize];
    uint8_t *pos = outputBuffer;
    while(p < end) {
        /* No escaping */
        if(*p != '\\') {
            /* In the ASCII range, but not a printable character */
            /* if(*p < 32 || *p == 127) */
            /*     goto cleanup; */

            *(pos++) = *(p++);
            continue;
        }

        /* Escape character */
        p++;
        if(p == end)
            goto cleanup;

        if(*p != 'u') {
            switch(*p) {
            case '"': case '\\': case '/': *pos = *p; break;
            case 'b': *pos = '\b'; break;
            case 'f': *pos = '\f'; break;
            case 'n': *pos = '\n'; break;
            case 'r': *pos = '\r'; break;
            case 't': *pos = '\t'; break;
            default: goto cleanup;
            }
            pos++;
            p++;
            continue;
        }

        /* Unicode */
        if(p + 4 >= end)
            goto cleanup;
        int32_t value_signed = decode_unicode_escape((const char*)p);
        if(value_signed < 0)
            goto cleanup;
        uint32_t value = (uint32_t)value_signed;
        p += 5;

        if(0xD800 <= value && value <= 0xDBFF) {
            /* Surrogate pair */
            if(p + 5 >= end)
                goto cleanup;
            if(*p != '\\' || *(p + 1) != 'u')
                goto cleanup;
            int32_t value2 = decode_unicode_escape((const char*)p + 1);
            if(value2 < 0xDC00 || value2 > 0xDFFF)
                goto cleanup;
            value = ((value - 0xD800u) << 10u) + (uint32_t)((value2 - 0xDC00) + 0x10000);
            p += 6;
        } else if(0xDC00 <= value && value <= 0xDFFF) {
            /* Invalid Unicode '\\u%04X' */
            goto cleanup;
        }

        size_t length;
        if(utf8_encode((int32_t)value, (char*)pos, &length))
            goto cleanup;

        pos += length;
    }

    dst->length = (size_t)(pos - outputBuffer);
    if(dst->length > 0) {
        dst->data = (UA_Byte*)outputBuffer;
    } else {
        dst->data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
        UA_free(outputBuffer);
    }

    parseCtx->index++;
    return UA_STATUSCODE_GOOD;

cleanup:
    UA_free(outputBuffer);
    return UA_STATUSCODE_BADDECODINGERROR;
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
            UA_unbase64((unsigned char*)tokenData, tokenSize, &flen);
        if(unB64 == 0)
            return UA_STATUSCODE_BADDECODINGERROR;
        dst->data = (u8*)unB64;
        dst->length = flen;
    }

    parseCtx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(LocalizedText) {
    CHECK_OBJECT;

    DecodeEntry entries[2] = {
        {UA_JSONKEY_LOCALE, &dst->locale, NULL, false, &UA_TYPES[UA_TYPES_STRING]},
        {UA_JSONKEY_TEXT, &dst->text, NULL, false, &UA_TYPES[UA_TYPES_STRING]}
    };

    return decodeFields(ctx, parseCtx, entries, 2);
}

DECODE_JSON(QualifiedName) {
    CHECK_OBJECT;

    DecodeEntry entries[2] = {
        {UA_JSONKEY_NAME, &dst->name, NULL, false, &UA_TYPES[UA_TYPES_STRING]},
        {UA_JSONKEY_URI, &dst->namespaceIndex, NULL, false, &UA_TYPES[UA_TYPES_UINT16]}
    };

    return decodeFields(ctx, parseCtx, entries, 2);
}

UA_FUNC_ATTR_WARN_UNUSED_RESULT status
lookAheadForKey(const char *key, CtxJson *ctx,
                ParseCtx *parseCtx, size_t *resultIndex) {
    status ret = UA_STATUSCODE_BADNOTFOUND;
    UA_UInt16 oldIndex = parseCtx->index; /* Save index for later restore */
    int end = parseCtx->tokenArray[parseCtx->index].end;
    parseCtx->index++; /* Move to the first key */
    while(parseCtx->index < parseCtx->tokenCount &&
          parseCtx->tokenArray[parseCtx->index].start < end) {
        /* Key must be a string (TODO: Make this an assert after replacing jsmn) */
        if(getJsmnType(parseCtx) != JSMN_STRING)
            return UA_STATUSCODE_BADDECODINGERROR;

        /* Move index to the value */
        parseCtx->index++;

        /* Value for the key must exist (TODO: Make this an assert after replacing jsmn) */
        if(parseCtx->index >= parseCtx->tokenCount)
            return UA_STATUSCODE_BADDECODINGERROR;

        /* Compare the key (previous index) */
        if(jsoneq((char*)ctx->pos, &parseCtx->tokenArray[parseCtx->index-1], key) == 0) {
            *resultIndex = parseCtx->index; /* Point result to the current index */
            ret = UA_STATUSCODE_GOOD;
            break;
        }

        skipObject(parseCtx); /* Jump over the value (can also be an array or object) */
    }
    parseCtx->index = oldIndex; /* Restore the old index */
    return ret;
}

static status
prepareDecodeNodeIdJson(UA_NodeId *dst, CtxJson *ctx, ParseCtx *parseCtx,
                        u8 *fieldCount, DecodeEntry *entries) {
    /* possible keys: Id, IdType, NamespaceIndex */
    /* Id must always be present */
    entries[*fieldCount].fieldName = UA_JSONKEY_ID;
    entries[*fieldCount].found = false;
    entries[*fieldCount].type = NULL;
    entries[*fieldCount].function = NULL;

    /* IdType */
    size_t searchResult = 0;
    status ret = lookAheadForKey(UA_JSONKEY_IDTYPE, ctx, parseCtx, &searchResult);
    if(ret == UA_STATUSCODE_GOOD) {
        size_t size = (size_t)(parseCtx->tokenArray[searchResult].end -
                               parseCtx->tokenArray[searchResult].start);
        if(size < 1)
            return UA_STATUSCODE_BADDECODINGERROR;

        char *idType = (char*)(ctx->pos + parseCtx->tokenArray[searchResult].start);

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
    CHECK_OBJECT;

    u8 fieldCount = 0;
    DecodeEntry entries[3];
    status ret = prepareDecodeNodeIdJson(dst, ctx, parseCtx, &fieldCount, entries);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;
    return decodeFields(ctx, parseCtx, entries, fieldCount);
}

static status
decodeExpandedNodeIdNamespace(void *dst, const UA_DataType *type,
                              CtxJson *ctx, ParseCtx *parseCtx) {
    UA_ExpandedNodeId *en = (UA_ExpandedNodeId*)dst;

    /* Parse as a number */
    UA_UInt16 oldIndex = parseCtx->index;
    status ret = UInt16_decodeJson(&en->nodeId.namespaceIndex, NULL, ctx, parseCtx);
    if(ret == UA_STATUSCODE_GOOD)
        return ret;

    /* Parse as a string */
    parseCtx->index = oldIndex; /* Reset the index */
    ret = String_decodeJson(&en->namespaceUri, NULL, ctx, parseCtx);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Replace with the index if the URI is found. Otherwise keep the string. */
    for(size_t i = 0; i < ctx->namespacesSize; i++) {
        if(UA_String_equal(&en->namespaceUri, &ctx->namespaces[i])) {
            UA_String_clear(&en->namespaceUri);
            en->nodeId.namespaceIndex = (UA_UInt16)i;
            break;
        }
    }
    return UA_STATUSCODE_GOOD;
}

static status
decodeExpandedNodeIdServerUri(void *dst, const UA_DataType *type,
                              CtxJson *ctx, ParseCtx *parseCtx) {
    UA_ExpandedNodeId *en = (UA_ExpandedNodeId*)dst;

    /* Parse as a number */
    UA_UInt16 oldIndex = parseCtx->index;
    status ret = UInt32_decodeJson(&en->serverIndex, NULL, ctx, parseCtx);
    if(ret == UA_STATUSCODE_GOOD)
        return ret;

    /* Parse as a string */
    UA_String uri = UA_STRING_NULL;
    parseCtx->index = oldIndex; /* Reset the index */
    ret = String_decodeJson(&uri, NULL, ctx, parseCtx);
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
    CHECK_OBJECT;

    u8 fieldCount = 0;
    DecodeEntry entries[4];
    status ret = prepareDecodeNodeIdJson(&dst->nodeId, ctx, parseCtx,
                                         &fieldCount, entries);
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

    return decodeFields(ctx, parseCtx, entries, fieldCount);
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

    parseCtx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_JSON(StatusCode) {
    return UInt32_decodeJson(dst, NULL, ctx, parseCtx);
}

static status
VariantDimension_decodeJson(void *dst, const UA_DataType *type,
                            CtxJson *ctx, ParseCtx *parseCtx) {
    (void) type;
    const UA_DataType *dimType = &UA_TYPES[UA_TYPES_UINT32];
    return Array_decodeJson((void**)dst, dimType, ctx, parseCtx);
}

static UA_Boolean
tokenIsNull(CtxJson *ctx, ParseCtx *parseCtx, size_t tokenIndex) {
    jsmntok_t *tok = &parseCtx->tokenArray[tokenIndex];
    if(tok->type != JSMN_PRIMITIVE)
        return false;
    if(tok->end - tok->start != 4)
        return false;
    return (strncmp((const char*)ctx->pos + tok->start, "null", 4) == 0);
}

DECODE_JSON(Variant) {
    CHECK_OBJECT;

    /* First search for the variant type in the json object. */
    size_t searchResultType = 0;
    status ret = lookAheadForKey(UA_JSONKEY_TYPE, ctx, parseCtx, &searchResultType);
    if(ret != UA_STATUSCODE_GOOD) {
        skipObject(parseCtx);
        return UA_STATUSCODE_GOOD;
    }

    /* Parse the type */
    size_t size = ((size_t)parseCtx->tokenArray[searchResultType].end -
                   (size_t)parseCtx->tokenArray[searchResultType].start);
    if(size == 0 || parseCtx->tokenArray[searchResultType].type != JSMN_PRIMITIVE)
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_UInt64 idTypeDecoded = 0;
    char *idTypeEncoded = (char*)(ctx->pos + parseCtx->tokenArray[searchResultType].start);
    size_t len = parseUInt64(idTypeEncoded, size, &idTypeDecoded);
    if(len == 0)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* A NULL Variant */
    if(idTypeDecoded == 0) {
        skipObject(parseCtx);
        return UA_STATUSCODE_GOOD;
    }

    /* Set the type */
    UA_NodeId typeNodeId = UA_NODEID_NUMERIC(0, (UA_UInt32)idTypeDecoded);
    dst->type = UA_findDataTypeWithCustom(&typeNodeId, parseCtx->customTypes);
    if(!dst->type)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Search for body */
    size_t searchResultBody = 0;
    ret = lookAheadForKey(UA_JSONKEY_BODY, ctx, parseCtx, &searchResultBody);
    if(ret != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Value is an array? */
    UA_Boolean isArray = (parseCtx->tokenArray[searchResultBody].type == JSMN_ARRAY);

    /* TODO: Handling of null-arrays (length -1) needs to be clarified
     *
     * if(tokenIsNull(ctx, parseCtx, searchResultBody)) {
     *     isArray = true;
     *     dst->arrayLength = 0;
     * } */

    /* Has the variant dimension? */
    UA_Boolean hasDimension = false;
    size_t searchResultDim = 0;
    ret = lookAheadForKey(UA_JSONKEY_DIMENSION, ctx, parseCtx, &searchResultDim);
    if(ret == UA_STATUSCODE_GOOD)
        hasDimension = (parseCtx->tokenArray[searchResultDim].size > 0);

    /* No array but has dimension -> error */
    if(!isArray && hasDimension)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Get the datatype of the content. The type must be a builtin data type.
     * All not-builtin types are wrapped in an ExtensionObject. */
    if(dst->type->typeKind > UA_DATATYPEKIND_DIAGNOSTICINFO)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* A variant cannot contain a variant. But it can contain an array of
     * variants */
    if(dst->type->typeKind == UA_DATATYPEKIND_VARIANT && !isArray)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Decode an array */
    if(isArray) {
        DecodeEntry entries[3] = {
            {UA_JSONKEY_TYPE, NULL, NULL, false, NULL},
            {UA_JSONKEY_BODY, &dst->data, (decodeJsonSignature)Array_decodeJson, false, dst->type},
            {UA_JSONKEY_DIMENSION, &dst->arrayDimensions, VariantDimension_decodeJson, false, NULL}
        };
        size_t entriesCount = 3;
        if(!hasDimension)
            entriesCount = 2; /* Use the first 2 fields only */
        return decodeFields(ctx, parseCtx, entries, entriesCount);
    }

    /* Decode a value wrapped in an ExtensionObject */
    if(dst->type->typeKind == UA_DATATYPEKIND_EXTENSIONOBJECT) {
        DecodeEntry entries[2] = {
            {UA_JSONKEY_TYPE, NULL, NULL, false, NULL},
            {UA_JSONKEY_BODY, dst, Variant_decodeJsonUnwrapExtensionObject, false, NULL}
        };
        return decodeFields(ctx, parseCtx, entries, 2);
    }

    /* Allocate Memory for Body */
    dst->data = UA_new(dst->type);
    if(!dst->data)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    DecodeEntry entries[2] = {
        {UA_JSONKEY_TYPE, NULL, NULL, false, NULL},
        {UA_JSONKEY_BODY, dst->data, NULL, false, dst->type}
    };
    return decodeFields(ctx, parseCtx, entries, 2);
}

DECODE_JSON(DataValue) {
    CHECK_OBJECT;

    DecodeEntry entries[6] = {
        {UA_JSONKEY_VALUE, &dst->value, NULL, false, &UA_TYPES[UA_TYPES_VARIANT]},
        {UA_JSONKEY_STATUS, &dst->status, NULL, false, &UA_TYPES[UA_TYPES_STATUSCODE]},
        {UA_JSONKEY_SOURCETIMESTAMP, &dst->sourceTimestamp, NULL, false, &UA_TYPES[UA_TYPES_DATETIME]},
        {UA_JSONKEY_SOURCEPICOSECONDS, &dst->sourcePicoseconds, NULL, false, &UA_TYPES[UA_TYPES_UINT16]},
        {UA_JSONKEY_SERVERTIMESTAMP, &dst->serverTimestamp, NULL, false, &UA_TYPES[UA_TYPES_DATETIME]},
        {UA_JSONKEY_SERVERPICOSECONDS, &dst->serverPicoseconds, NULL, false, &UA_TYPES[UA_TYPES_UINT16]}
    };

    status ret = decodeFields(ctx, parseCtx, entries, 6);
    dst->hasValue = entries[0].found;
    dst->hasStatus = entries[1].found;
    dst->hasSourceTimestamp = entries[2].found;
    dst->hasSourcePicoseconds = entries[3].found;
    dst->hasServerTimestamp = entries[4].found;
    dst->hasServerPicoseconds = entries[5].found;
    return ret;
}

DECODE_JSON(ExtensionObject) {
    CHECK_OBJECT;

    /* Empty object -> Null ExtensionObject */
    if(parseCtx->tokenArray[parseCtx->index].size == 0)
        return UA_STATUSCODE_GOOD;

    /* Search for Encoding */
    size_t encodingPos = 0;
    status ret = lookAheadForKey(UA_JSONKEY_ENCODING, ctx, parseCtx, &encodingPos);

    /* UA_JSONKEY_ENCODING found */
    if(ret == UA_STATUSCODE_GOOD) {
        /* Parse the encoding */
        UA_UInt64 encoding = 0;
        char *extObjEncoding = (char*)(ctx->pos +
                                       parseCtx->tokenArray[encodingPos].start);
        size_t size = (size_t)(parseCtx->tokenArray[encodingPos].end -
                               parseCtx->tokenArray[encodingPos].start);
        parseUInt64(extObjEncoding, size, &encoding);

        if(encoding == 1) {
            dst->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING; /* ByteString in Json Body */
        } else if(encoding == 2) {
            dst->encoding = UA_EXTENSIONOBJECT_ENCODED_XML; /* XmlElement in Json Body */
        } else {
            return UA_STATUSCODE_BADDECODINGERROR;
        }

        DecodeEntry entries[3] = {
            {UA_JSONKEY_ENCODING, NULL, NULL, false, NULL},
            {UA_JSONKEY_BODY, &dst->content.encoded.body, NULL, false, &UA_TYPES[UA_TYPES_STRING]},
            {UA_JSONKEY_TYPEID, &dst->content.encoded.typeId, NULL, false, &UA_TYPES[UA_TYPES_NODEID]}
        };
        return decodeFields(ctx, parseCtx, entries, 3);
    }

    /* Decode the type NodeId */
    UA_NodeId typeId;
    UA_NodeId_init(&typeId);

    size_t searchTypeIdResult = 0;
    ret = lookAheadForKey(UA_JSONKEY_TYPEID, ctx, parseCtx, &searchTypeIdResult);
    if(ret != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADENCODINGERROR;

    UA_UInt16 index = parseCtx->index; /* to restore later */
    parseCtx->index = (UA_UInt16)searchTypeIdResult;
    ret = NodeId_decodeJson(&typeId, &UA_TYPES[UA_TYPES_NODEID], ctx, parseCtx);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Restore the index to the beginning of the object  */
    parseCtx->index = index;

    /* If the type is not known, decode the body as an opaque JSON decoding */
    const UA_DataType *typeOfBody =
        UA_findDataTypeWithCustom(&typeId, parseCtx->customTypes);
    if(!typeOfBody) {
        /* Dont decode body: 1. save as bytestring, 2. jump over */
        dst->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
        dst->content.encoded.typeId = typeId; /* Move the type NodeId */

        /* Check if an object */
        if(getJsmnType(parseCtx) != JSMN_OBJECT)
            return UA_STATUSCODE_BADDECODINGERROR;

        /* Search for body to save */
        size_t bodyIndex = 0;
        ret = lookAheadForKey(UA_JSONKEY_BODY, ctx, parseCtx, &bodyIndex);
        if(ret != UA_STATUSCODE_GOOD || bodyIndex >= (size_t)parseCtx->tokenCount)
            return UA_STATUSCODE_BADDECODINGERROR;

        /* Get the size of the Object as a string, not the Object key count! */
        size_t sizeOfJsonString = (size_t)
            (parseCtx->tokenArray[bodyIndex].end -
             parseCtx->tokenArray[bodyIndex].start);
        if(sizeOfJsonString == 0)
            return UA_STATUSCODE_BADDECODINGERROR;

        /* Copy body as bytestring. */
        char* bodyJsonString = (char*)(ctx->pos + parseCtx->tokenArray[bodyIndex].start);
        ret = UA_ByteString_allocBuffer(&dst->content.encoded.body, sizeOfJsonString);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;

        memcpy(dst->content.encoded.body.data, bodyJsonString, sizeOfJsonString);

        skipObject(parseCtx); /* parseCtx->index is still at the object
                               * beginning. Skip. */
        return UA_STATUSCODE_GOOD;
    }

    /* Type NodeId not used anymore */
    UA_NodeId_clear(&typeId);

    /* Allocate */
    dst->content.decoded.data = UA_new(typeOfBody);
    if(!dst->content.decoded.data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Set type */
    dst->content.decoded.type = typeOfBody;
    dst->encoding = UA_EXTENSIONOBJECT_DECODED;

    /* Decode body */
    DecodeEntry entries[2] = {
        {UA_JSONKEY_TYPEID, NULL, NULL, false, NULL},
        {UA_JSONKEY_BODY, dst->content.decoded.data, NULL, false, typeOfBody}
    };
    return decodeFields(ctx, parseCtx, entries, 2);
}

static status
Variant_decodeJsonUnwrapExtensionObject(void *p, const UA_DataType *type,
                                        CtxJson *ctx, ParseCtx *parseCtx) {
    (void) type;

    UA_Variant *dst = (UA_Variant*)p;
    if(isJsonNull(ctx, parseCtx)) {
        dst->data = UA_ExtensionObject_new();
        dst->type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
        parseCtx->index++;
        return UA_STATUSCODE_GOOD;
    }

    UA_UInt16 old_index = parseCtx->index; /* Store the start index of the ExtensionObject */

    /* Decode the DataType */
    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    size_t searchTypeIdResult = 0;
    status ret = lookAheadForKey(UA_JSONKEY_TYPEID, ctx, parseCtx, &searchTypeIdResult);
    if(ret != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;
    parseCtx->index = (UA_UInt16)searchTypeIdResult;
    ret = NodeId_decodeJson(&typeId, &UA_TYPES[UA_TYPES_NODEID], ctx, parseCtx);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&typeId);
        return ret;
    }

    parseCtx->index = old_index; /* restore the index */

    /* Find the DataType from the NodeId */
    const UA_DataType *typeOfBody = UA_findDataType(&typeId);
    UA_NodeId_clear(&typeId);

    /* Search for encoding (without advancing the index) */
    UA_UInt64 encoding = 0; /* If no encoding found it is structure encoding */
    size_t searchEncodingResult = 0;
    ret = lookAheadForKey(UA_JSONKEY_ENCODING, ctx, parseCtx, &searchEncodingResult);
    if(ret == UA_STATUSCODE_GOOD) {
        char *extObjEncoding = (char*)(ctx->pos + parseCtx->tokenArray[searchEncodingResult].start);
        size_t size = (size_t)(parseCtx->tokenArray[searchEncodingResult].end
                               - parseCtx->tokenArray[searchEncodingResult].start);
        parseUInt64(extObjEncoding, size, &encoding);
    }

    if(encoding == 0 && typeOfBody != NULL) {
        /* Found a valid type and it is structure encoded so it can be unwrapped */
        dst->data = UA_new(typeOfBody);
        if(!dst->data)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        dst->type = typeOfBody;

        /* Decode the content */
        DecodeEntry entries[3] = {
            {UA_JSONKEY_TYPEID, NULL, NULL, false, NULL},
            {UA_JSONKEY_BODY, dst->data, NULL, false, typeOfBody},
            {UA_JSONKEY_ENCODING, NULL, NULL, false, NULL}
        };
        ret = decodeFields(ctx, parseCtx, entries, 3);
    } else {
        /* Decode as ExtensionObject */
        dst->data = UA_new(&UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
        if(!dst->data)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        dst->type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];

        UA_assert(parseCtx->index == old_index); /* The index points to the start
                                                  * of the ExtensionObject */
        ret = ExtensionObject_decodeJson((UA_ExtensionObject*)dst->data, NULL, ctx, parseCtx);
    }
    if(ret != UA_STATUSCODE_GOOD) {
        UA_delete(dst->data, dst->type);
        dst->data = NULL;
        dst->type = NULL;
    }
    return ret;
}

status
DiagnosticInfoInner_decodeJson(void* dst, const UA_DataType* type,
                               CtxJson* ctx, ParseCtx* parseCtx);

DECODE_JSON(DiagnosticInfo) {
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
    status ret = decodeFields(ctx, parseCtx, entries, 7);

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
DiagnosticInfoInner_decodeJson(void* dst, const UA_DataType* type,
                               CtxJson* ctx, ParseCtx* parseCtx) {
    UA_DiagnosticInfo *inner = (UA_DiagnosticInfo*)
        UA_calloc(1, sizeof(UA_DiagnosticInfo));
    if(!inner)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_DiagnosticInfo **dst2 = (UA_DiagnosticInfo**)dst;
    *dst2 = inner;  /* Copy new Pointer do dest */
    return DiagnosticInfo_decodeJson(inner, type, ctx, parseCtx);
}

status
decodeFields(CtxJson *ctx, ParseCtx *parseCtx,
             DecodeEntry *entries, size_t entryCount) {
    CHECK_TOKEN_BOUNDS;
    CHECK_OBJECT;

    if(ctx->depth >= UA_JSON_ENCODING_MAX_RECURSION - 1)
        return UA_STATUSCODE_BADENCODINGERROR;
    ctx->depth++;

    /* Empty object, nothing to decode */
    size_t objectCount = (size_t)(parseCtx->tokenArray[parseCtx->index].size);
    if(objectCount == 0) {
        ctx->depth--;
        parseCtx->index++; /* Jump to the element after the empty object */
        return UA_STATUSCODE_GOOD;
    }

    parseCtx->index++; /* Go to first key */

    /* CHECK_TOKEN_BOUNDS */
    if(parseCtx->index >= parseCtx->tokenCount) {
        ctx->depth--;
        return UA_STATUSCODE_BADDECODINGERROR;
    }

    status ret = UA_STATUSCODE_GOOD;
    for(size_t currObj = 0; currObj < objectCount &&
            parseCtx->index < parseCtx->tokenCount; currObj++) {

        /* Key must be a string (TODO: Convert to assert when jsmn is replaced) */
        if(getJsmnType(parseCtx) != JSMN_STRING) {
            ret = UA_STATUSCODE_BADDECODINGERROR;
            goto cleanup;
        }

        /* Start searching at the index of currObj */
        for(size_t i = currObj; i < entryCount + currObj; i++) {
            /* Search for key, if found outer loop will be one less. Best case
             * if objectCount is in order! */
            size_t index = i % entryCount;

            /* CHECK_TOKEN_BOUNDS */
            if(parseCtx->index >= parseCtx->tokenCount) {
                ret = UA_STATUSCODE_BADDECODINGERROR;
                goto cleanup;
            }

            if(jsoneq((char*) ctx->pos, &parseCtx->tokenArray[parseCtx->index],
                      entries[index].fieldName) != 0)
                continue;

            /* Duplicate key found, abort */
            if(entries[index].found) {
                ret = UA_STATUSCODE_BADDECODINGERROR;
                goto cleanup;
            }

            entries[index].found = true;

            parseCtx->index++; /* Go from key to value */

            /* CHECK_TOKEN_BOUNDS */
            if(parseCtx->index >= parseCtx->tokenCount) {
                ret = UA_STATUSCODE_BADDECODINGERROR;
                goto cleanup;
            }

            /* An entry that was expected, but shall not be decoded.
             * Jump over it. */
            if(!entries[index].function && !entries[index].type) {
                skipObject(parseCtx);
                break;
            }

            /* A null-value -> skip the decoding (as a convention, if we know
             * the type here, the value must be already initialized) */
            if(isJsonNull(ctx, parseCtx) && entries[index].type) {
                parseCtx->index++;
                break;
            }

            /* Decode */
            if(entries[index].function) /* Specialized decoding function */
                ret = entries[index].function(entries[index].fieldPointer,
                                              entries[index].type, ctx, parseCtx);
            else /* Decode by type-kind */
                ret = decodeJsonJumpTable[entries[index].type->typeKind]
                    (entries[index].fieldPointer, entries[index].type, ctx, parseCtx);
            if(ret != UA_STATUSCODE_GOOD)
                goto cleanup;
            break;
        }
    }

 cleanup:
    ctx->depth--;
    return ret;
}

static status
Array_decodeJson(void **dst, const UA_DataType *type,
                 CtxJson *ctx, ParseCtx *parseCtx) {
    /* Save the length of the array */
    size_t *size_ptr = (size_t*) dst - 1;

    if(parseCtx->tokenArray[parseCtx->index].type != JSMN_ARRAY)
        return UA_STATUSCODE_BADDECODINGERROR;

    size_t length = (size_t)parseCtx->tokenArray[parseCtx->index].size;

    /* Return early for empty arrays */
    if(length == 0) {
        *size_ptr = length;
        *dst = UA_EMPTY_ARRAY_SENTINEL;
        parseCtx->index++; /* Jump over the array token */
        return UA_STATUSCODE_GOOD;
    }

    /* Allocate memory */
    *dst = UA_calloc(length, type->memSize);
    if(*dst == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    parseCtx->index++; /* Go to first array member */

    /* Decode array members */
    uintptr_t ptr = (uintptr_t)*dst;
    for(size_t i = 0; i < length; ++i) {
        if(!tokenIsNull(ctx, parseCtx, parseCtx->index)) {
            status ret =
                decodeJsonJumpTable[type->typeKind]((void*)ptr, type, ctx, parseCtx);
            if(ret != UA_STATUSCODE_GOOD) {
                UA_Array_delete(*dst, i+1, type);
                *dst = NULL;
                return ret;
            }
        } else {
            parseCtx->index++;
        }
        ptr += type->memSize;
    }

    *size_ptr = length; /* All good, set the size */
    return UA_STATUSCODE_GOOD;
}

static status
decodeJsonStructure(void *dst, const UA_DataType *type,
                    CtxJson *ctx, ParseCtx *parseCtx) {
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

    ret = decodeFields(ctx, parseCtx, entries, membersSize);

    if(ctx->depth == 0)
        return UA_STATUSCODE_BADENCODINGERROR;
    ctx->depth--;
    return ret;
}

static status
decodeJsonNotImplemented(void *dst, const UA_DataType *type, CtxJson *ctx,
                         ParseCtx *parseCtx) {
    (void)dst, (void)type, (void)ctx, (void)parseCtx;
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
tokenize(ParseCtx *parseCtx, CtxJson *ctx, const UA_ByteString *src, size_t tokensSize) {
    /* Set up the context */
    ctx->pos = &src->data[0];
    ctx->end = &src->data[src->length];
    ctx->depth = 0;
    parseCtx->tokenCount = 0;
    parseCtx->index = 0;

    /* Tokenize */
    jsmn_parser p;
    jsmn_init(&p);
    parseCtx->tokenCount = (UA_Int32)
        jsmn_parse(&p, (char*)src->data, src->length,
                   parseCtx->tokenArray, (unsigned int)tokensSize);
    if(parseCtx->tokenCount == JSMN_ERROR_NOMEM)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    if(parseCtx->tokenCount < 0)
        return UA_STATUSCODE_BADDECODINGERROR;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_decodeJson(const UA_ByteString *src, void *dst, const UA_DataType *type,
              const UA_DecodeJsonOptions *options) {
#ifndef UA_ENABLE_TYPEDESCRIPTION
    return UA_STATUSCODE_BADNOTSUPPORTED;
#endif

    if(!dst || !src || !type)
        return UA_STATUSCODE_BADARGUMENTSMISSING;

    /* Set up the context */
    jsmntok_t tokens[UA_JSON_MAXTOKENCOUNT / 8];
    CtxJson ctx;
    memset(&ctx, 0, sizeof(ctx));
    ParseCtx parseCtx;
    memset(&parseCtx, 0, sizeof(ParseCtx));
    parseCtx.tokenArray = tokens;

    if(options) {
        ctx.namespaces = options->namespaces;
        ctx.namespacesSize = options->namespacesSize;
        ctx.serverUris = options->serverUris;
        ctx.serverUrisSize = options->serverUrisSize;
        parseCtx.customTypes = options->customTypes;
    }

    /* Decode */
    status ret = tokenize(&parseCtx, &ctx, src, UA_JSON_MAXTOKENCOUNT / 8);

    /* Allocate larger token array on the heap and try again */
    if(ret == UA_STATUSCODE_BADOUTOFMEMORY) {
        parseCtx.tokenArray = (jsmntok_t*)
            UA_malloc(sizeof(jsmntok_t) * UA_JSON_MAXTOKENCOUNT);
        if(!parseCtx.tokenArray)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        ret = tokenize(&parseCtx, &ctx, src, UA_JSON_MAXTOKENCOUNT);
    }

    if(ret == UA_STATUSCODE_GOOD) {
        memset(dst, 0, type->memSize); /* Initialize the value */
        ret = decodeJsonJumpTable[type->typeKind](dst, type, &ctx, &parseCtx);

        /* Sanity check if all Tokens were processed */
        if(parseCtx.index != parseCtx.tokenCount &&
           parseCtx.index != parseCtx.tokenCount - 1)
            ret = UA_STATUSCODE_BADDECODINGERROR;
    }

    /* Free token array on the heap */
    if(parseCtx.tokenArray != tokens)
        UA_free(parseCtx.tokenArray);

    if(ret != UA_STATUSCODE_GOOD)
        UA_clear(dst, type); /* Clean up */
    return ret;
}
