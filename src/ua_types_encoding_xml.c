/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/config.h>
#include <open62541/types_generated.h>
#include <open62541/types_generated_handling.h>

#include "ua_types_encoding_xml.h"

#include "../deps/itoa.h"
#include "../deps/parse_num.h"
#include "../deps/libc_time.h"
#include "../deps/dtoa.h"

#ifndef UA_ENABLE_PARSING
#error UA_ENABLE_PARSING required for XML encoding
#endif

#ifndef UA_ENABLE_TYPEDESCRIPTION
#error UA_ENABLE_TYPEDESCRIPTION required for XML encoding
#endif

/* vs2008 does not have INFINITY and NAN defined */
#ifndef INFINITY
# define INFINITY ((UA_Double)(DBL_MAX+DBL_MAX))
#endif
#ifndef NAN
# define NAN ((UA_Double)(INFINITY-INFINITY))
#endif

/* Have some slack at the end. E.g. for negative and very long years. */
#define UA_XML_DATETIME_LENGTH 40

/************/
/* Encoding */
/************/

#define ENCODE_XML(TYPE) static status \
    TYPE##_encodeXml(CtxXml *ctx, const UA_##TYPE *src, const UA_DataType *type)

static status UA_FUNC_ATTR_WARN_UNUSED_RESULT
xmlEncodeWriteChars(CtxXml *ctx, const char *c, size_t len) {
    if(ctx->pos + len > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        memcpy(ctx->pos, c, len);
    ctx->pos += len;
    return UA_STATUSCODE_GOOD;
}

/* Boolean */
ENCODE_XML(Boolean) {
    if(*src == true)
        return xmlEncodeWriteChars(ctx, "true", 4);
    return xmlEncodeWriteChars(ctx, "false", 5);
}

static status encodeSigned(CtxXml *ctx, UA_Int64 value, char* buffer) {
    UA_UInt16 digits = itoaSigned(value, buffer);
    return xmlEncodeWriteChars(ctx, buffer, digits);
}

static status encodeUnsigned(CtxXml *ctx, UA_UInt64 value, char* buffer) {
    UA_UInt16 digits = itoaUnsigned(value, buffer, 10);
    return xmlEncodeWriteChars(ctx, buffer, digits);
}

/* signed Byte */
ENCODE_XML(SByte) {
    char buf[5];
    return encodeSigned(ctx, *src, buf);
}

/* Byte */
ENCODE_XML(Byte) {
    char buf[4];
    return encodeUnsigned(ctx, *src, buf);
}

/* Int16 */
ENCODE_XML(Int16) {
    char buf[7];
    return encodeSigned(ctx, *src, buf);
}

/* UInt16 */
ENCODE_XML(UInt16) {
    char buf[6];
    return encodeUnsigned(ctx, *src, buf);
}

/* Int32 */
ENCODE_XML(Int32) {
    char buf[12];
    return encodeSigned(ctx, *src, buf);
}

/* UInt32 */
ENCODE_XML(UInt32) {
    char buf[11];
    return encodeUnsigned(ctx, *src, buf);
}

/* Int64 */
ENCODE_XML(Int64) {
    char buf[23];
    return encodeSigned(ctx, *src, buf);
}

/* UInt64 */
ENCODE_XML(UInt64) {
    char buf[23];
    return encodeUnsigned(ctx, *src, buf);
}

/* Float */
ENCODE_XML(Float) {
    char buffer[32];
    size_t len;
    if(*src != *src) {
        strcpy(buffer, "NaN");
        len = strlen(buffer);
    } else if(*src == INFINITY) {
        strcpy(buffer, "INF");
        len = strlen(buffer);
    } else if(*src == -INFINITY) {
        strcpy(buffer, "-INF");
        len = strlen(buffer);
    } else {
        len = dtoa((UA_Double)*src, buffer);
    }
    return xmlEncodeWriteChars(ctx, buffer, len);
}

/* Double */
ENCODE_XML(Double) {
    char buffer[32];
    size_t len;
    if(*src != *src) {
        strcpy(buffer, "NaN");
        len = strlen(buffer);
    } else if(*src == INFINITY) {
        strcpy(buffer, "INF");
        len = strlen(buffer);
    } else if(*src == -INFINITY) {
        strcpy(buffer, "-INF");
        len = strlen(buffer);
    } else {
        len = dtoa(*src, buffer);
    }
    return xmlEncodeWriteChars(ctx, buffer, len);
}

/* String */
ENCODE_XML(String) {
    if(!src->data)
        return xmlEncodeWriteChars(ctx, "null", 4);
    return xmlEncodeWriteChars(ctx, (const char*)src->data, src->length);
}

/* Guid */
ENCODE_XML(Guid) {
    if(ctx->pos + 36 > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        UA_Guid_to_hex(src, ctx->pos, false);
    ctx->pos += 36;
    return UA_STATUSCODE_GOOD;
}

/* DateTime */
static u8
xmlEncodePrintNumber(i32 n, char *pos, u8 min_digits) {
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

ENCODE_XML(DateTime) {
    UA_DateTimeStruct tSt = UA_DateTime_toStruct(*src);

    /* Format: -yyyy-MM-dd'T'HH:mm:ss.SSSSSSSSS'Z' is used. max 31 bytes.
     * Note the optional minus for negative years. */
    char buffer[UA_XML_DATETIME_LENGTH];
    char *pos = buffer;
    pos += xmlEncodePrintNumber(tSt.year, pos, 4);
    *(pos++) = '-';
    pos += xmlEncodePrintNumber(tSt.month, pos, 2);
    *(pos++) = '-';
    pos += xmlEncodePrintNumber(tSt.day, pos, 2);
    *(pos++) = 'T';
    pos += xmlEncodePrintNumber(tSt.hour, pos, 2);
    *(pos++) = ':';
    pos += xmlEncodePrintNumber(tSt.min, pos, 2);
    *(pos++) = ':';
    pos += xmlEncodePrintNumber(tSt.sec, pos, 2);
    *(pos++) = '.';
    pos += xmlEncodePrintNumber(tSt.milliSec, pos, 3);
    pos += xmlEncodePrintNumber(tSt.microSec, pos, 3);
    pos += xmlEncodePrintNumber(tSt.nanoSec, pos, 3);

    UA_assert(pos <= &buffer[UA_XML_DATETIME_LENGTH]);

    /* Remove trailing zeros */
    pos--;
    while(*pos == '0')
        pos--;
    if(*pos == '.')
        pos--;

    *(++pos) = 'Z';
    UA_String str = {((uintptr_t)pos - (uintptr_t)buffer)+1, (UA_Byte*)buffer};

    return xmlEncodeWriteChars(ctx, (const char*)str.data, str.length);
}

/* NodeId */
ENCODE_XML(NodeId) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_String out = UA_STRING_NULL;

    ret |= UA_NodeId_print(src, &out);
    ret |= encodeXmlJumpTable[UA_DATATYPEKIND_STRING](ctx, &out, NULL);

    UA_String_clear(&out);
    return ret;
}

/* ExpandedNodeId */
ENCODE_XML(ExpandedNodeId) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_String out = UA_STRING_NULL;

    ret |= UA_ExpandedNodeId_print(src, &out);
    ret |= encodeXmlJumpTable[UA_DATATYPEKIND_STRING](ctx, &out, NULL);

    UA_String_clear(&out);
    return ret;
}

static status
encodeXmlNotImplemented(CtxXml *ctx, const void *src, const UA_DataType *type) {
    (void)ctx, (void)src, (void)type;
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

const encodeXmlSignature encodeXmlJumpTable[UA_DATATYPEKINDS] = {
    (encodeXmlSignature)Boolean_encodeXml,          /* Boolean */
    (encodeXmlSignature)SByte_encodeXml,            /* SByte */
    (encodeXmlSignature)Byte_encodeXml,             /* Byte */
    (encodeXmlSignature)Int16_encodeXml,            /* Int16 */
    (encodeXmlSignature)UInt16_encodeXml,           /* UInt16 */
    (encodeXmlSignature)Int32_encodeXml,            /* Int32 */
    (encodeXmlSignature)UInt32_encodeXml,           /* UInt32 */
    (encodeXmlSignature)Int64_encodeXml,            /* Int64 */
    (encodeXmlSignature)UInt64_encodeXml,           /* UInt64 */
    (encodeXmlSignature)Float_encodeXml,            /* Float */
    (encodeXmlSignature)Double_encodeXml,           /* Double */
    (encodeXmlSignature)String_encodeXml,           /* String */
    (encodeXmlSignature)DateTime_encodeXml,         /* DateTime */
    (encodeXmlSignature)Guid_encodeXml,             /* Guid */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* ByteString */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* XmlElement */
    (encodeXmlSignature)NodeId_encodeXml,           /* NodeId */
    (encodeXmlSignature)ExpandedNodeId_encodeXml,   /* ExpandedNodeId */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* StatusCode */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* QualifiedName */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* LocalizedText */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* ExtensionObject */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* DataValue */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* Variant */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* DiagnosticInfo */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* Decimal */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* Enum */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* Structure */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* Structure with optional fields */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* Union */
    (encodeXmlSignature)encodeXmlNotImplemented     /* BitfieldCluster */
};

UA_StatusCode
UA_encodeXml(const void *src, const UA_DataType *type, UA_ByteString *outBuf,
             const UA_EncodeXmlOptions *options) {
    if(!src || !type)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate buffer */
    UA_Boolean allocated = false;
    status res = UA_STATUSCODE_GOOD;
    if(outBuf->length == 0) {
        size_t len = UA_calcSizeXml(src, type, options);
        res = UA_ByteString_allocBuffer(outBuf, len);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        allocated = true;
    }

    /* Set up the context */
    CtxXml ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.pos = outBuf->data;
    ctx.end = &outBuf->data[outBuf->length];
    ctx.depth = 0;
    ctx.calcOnly = false;
    if(options)
        ctx.prettyPrint = options->prettyPrint;

    /* Encode */
    res = encodeXmlJumpTable[type->typeKind](&ctx, src, type);

    /* Clean up */
    if(res == UA_STATUSCODE_GOOD)
        outBuf->length = (size_t)((uintptr_t)ctx.pos - (uintptr_t)outBuf->data);
    else if(allocated)
        UA_ByteString_clear(outBuf);
    return res;
}

/************/
/* CalcSize */
/************/

size_t
UA_calcSizeXml(const void *src, const UA_DataType *type,
               const UA_EncodeXmlOptions *options) {
    if(!src || !type)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Set up the context */
    CtxXml ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.pos = NULL;
    ctx.end = (const UA_Byte*)(uintptr_t)SIZE_MAX;
    ctx.depth = 0;
    if(options) {
        ctx.prettyPrint = options->prettyPrint;
    }

    ctx.calcOnly = true;

    /* Encode */
    status ret = encodeXmlJumpTable[type->typeKind](&ctx, src, type);
    if(ret != UA_STATUSCODE_GOOD)
        return 0;
    return (size_t)ctx.pos;
}

/**********/
/* Decode */
/**********/

#define CHECK_TOKEN_BOUNDS do {                   \
    if(ctx->index >= ctx->tokensSize)             \
        return UA_STATUSCODE_BADDECODINGERROR;    \
    } while(0)

/* Forward declarations*/
#define DECODE_XML(TYPE) static status                   \
    TYPE##_decodeXml(ParseCtxXml *ctx, UA_##TYPE *dst,  \
                      const UA_DataType *type)

DECODE_XML(Boolean) {
    if(ctx->length == 4 &&
       ctx->data[0] == 't' && ctx->data[1] == 'r' &&
       ctx->data[2] == 'u' && ctx->data[3] == 'e') {
        *dst = true;
    } else if(ctx->length == 5 &&
              ctx->data[0] == 'f' && ctx->data[1] == 'a' &&
              ctx->data[2] == 'l' && ctx->data[3] == 's' &&
              ctx->data[4] == 'e') {
        *dst = false;
    } else {
        return UA_STATUSCODE_BADDECODINGERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
decodeSigned(const char *data, size_t dataSize, UA_Int64 *dst) {
    size_t len = parseInt64(data, dataSize, dst);
    if(len == 0)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* There must only be whitespace between the end of the parsed number and
     * the end of the XML section */
    for(size_t i = len; i < dataSize; i++) {
        if(data[i] != ' ' && data[i] - '\t' >= 5)
            return UA_STATUSCODE_BADDECODINGERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
decodeUnsigned(const char *data, size_t dataSize, UA_UInt64 *dst) {
    size_t len = parseUInt64(data, dataSize, dst);
    if(len == 0)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* There must only be whitespace between the end of the parsed number and
     * the end of the XML section */
    for(size_t i = len; i < dataSize; i++) {
        if(data[i] != ' ' && data[i] - '\t' >= 5)
            return UA_STATUSCODE_BADDECODINGERROR;
    }

    return UA_STATUSCODE_GOOD;
}

DECODE_XML(SByte) {
    /* TODO:
     *   1. Add support for optional "+" sign.
     *   2. Add support for optional leading zeros.
     *   3. Check if the value is in hex, octal or binray. */
    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(ctx->data, ctx->length, &out);

    if(s != UA_STATUSCODE_GOOD || out < UA_SBYTE_MIN || out > UA_SBYTE_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_SByte)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Byte) {
    /* TODO:
     *   1. Add support for optional "+" sign.
     *   2. Add support for optional leading zeros.
     *   3. Check if the value is in hex, octal or binray.
     *   4. Check if decimal point exists. */
    UA_UInt64 out = 0;
    UA_StatusCode s = decodeUnsigned(ctx->data, ctx->length, &out);

    if(s != UA_STATUSCODE_GOOD || out > UA_BYTE_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Byte)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Int16) {
    /* TODO:
     *   1. Add support for optional "+" sign.
     *   2. Add support for optional leading zeros.
     *   3. Check if the value is in hex, octal or binray.
     *   4. Check if decimal point exists. */
    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(ctx->data, ctx->length, &out);

    if(s != UA_STATUSCODE_GOOD || out < UA_INT16_MIN || out > UA_INT16_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Int16)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(UInt16) {
    /* TODO:
     *   1. Add support for optional "+" sign.
     *   2. Add support for optional leading zeros.
     *   3. Check if the value is in hex, octal or binray.
     *   4. Check if decimal point exists. */
    UA_UInt64 out = 0;
    UA_StatusCode s = decodeUnsigned(ctx->data, ctx->length, &out);

    if(s != UA_STATUSCODE_GOOD || out > UA_UINT16_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_UInt16)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Int32) {
    /* TODO:
     *   1. Add support for optional "+" sign.
     *   2. Add support for optional leading zeros.
     *   3. Check if the value is in hex, octal or binray.
     *   4. Check if decimal point exists.
     *   5. Check "-0" and "+0", and just remove the sign. */
    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(ctx->data, ctx->length, &out);

    if(s != UA_STATUSCODE_GOOD || out < UA_INT32_MIN || out > UA_INT32_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Int32)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(UInt32) {
    /* TODO:
     *   1. Add support for optional "+" sign.
     *   2. Add support for optional leading zeros.
     *   3. Check if the value is in hex, octal or binray.
     *   4. Check if decimal point exists. */
    UA_UInt64 out = 0;
    UA_StatusCode s = decodeUnsigned(ctx->data, ctx->length, &out);

    if(s != UA_STATUSCODE_GOOD || out > UA_UINT32_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_UInt32)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Int64) {
    /* TODO:
     *   1. Add support for optional "+" sign.
     *   2. Add support for optional leading zeros.
     *   3. Check if the value is in hex, octal or binray.
     *   4. Check if decimal point exists. */
    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(ctx->data, ctx->length, &out);

    if(s != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Int64)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(UInt64) {
    /* TODO:
     *   1. Add support for optional "+" sign.
     *   2. Add support for optional leading zeros.
     *   3. Check if the value is in hex, octal or binray.
     *   4. Check if decimal point exists. */
    UA_UInt64 out = 0;
    UA_StatusCode s = decodeUnsigned(ctx->data, ctx->length, &out);

    if(s != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_UInt64)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Double) {

    /* https://www.exploringbinary.com/maximum-number-of-decimal-digits-in-binary-floating-point-numbers/
     * Maximum digit counts for select IEEE floating-point formats: 1074
     * Sanity check.
     */
    if(ctx->length > 1075)
        return UA_STATUSCODE_BADDECODINGERROR;

    if(ctx->length == 3 && memcmp(ctx->data, "INF", 3) == 0) {
        *dst = INFINITY;
        return UA_STATUSCODE_GOOD;
    }

    if(ctx->length == 4 && memcmp(ctx->data, "-INF", 4) == 0) {
        *dst = -INFINITY;
        return UA_STATUSCODE_GOOD;
    }

    if(ctx->length == 3 && memcmp(ctx->data, "NaN", 3) == 0) {
        *dst = NAN;
        return UA_STATUSCODE_GOOD;
    }

    size_t len = parseDouble(ctx->data, ctx->length, dst);
    if(len == 0)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* There must only be whitespace between the end of the parsed number and
     * the end of the token */
    for(size_t i = len; i < ctx->length; i++) {
        if(ctx->data[i] != ' ' && ctx->data[i] -'\t' >= 5)
            return UA_STATUSCODE_BADDECODINGERROR;
    }

    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Float) {
    UA_Double v = 0.0;
    UA_StatusCode res = Double_decodeXml(ctx, &v, NULL);
    *dst = (UA_Float)v;
    return res;
}

DECODE_XML(String) {
    /* Empty string? */
    if(ctx->length == 0) {
        dst->data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
        dst->length = 0;
        return UA_STATUSCODE_GOOD;
    }

    /* Set the output */
    dst->length = ctx->length;
    if(dst->length > 0) {
        dst->data = (UA_Byte*)(uintptr_t)ctx->data;
    } else {
        dst->data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
    }

    return UA_STATUSCODE_GOOD;
}

DECODE_XML(DateTime) {
    /* The last character has to be 'Z'. We can omit some length checks later on
     * because we know the atoi functions stop before the 'Z'. */
    if(ctx->length == 0 || ctx->data[ctx->length - 1] != 'Z')
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
    if(ctx->data[0] == '-' || ctx->data[0] == '+')
        pos++;
    UA_Int64 year = 0;
    len = parseInt64(&ctx->data[pos], 5, &year);
    pos += len;
    if(len != 4 && ctx->data[pos] != '-')
        return UA_STATUSCODE_BADDECODINGERROR;
    if(ctx->data[0] == '-')
        year = -year;
    dts.tm_year = (UA_Int16)year - 1900;
    if(ctx->data[pos] == '-')
        pos++;

    /* Parse the month */
    UA_UInt64 month = 0;
    len = parseUInt64(&ctx->data[pos], 2, &month);
    pos += len;
    UA_CHECK(len == 2, return UA_STATUSCODE_BADDECODINGERROR);
    dts.tm_mon = (UA_UInt16)month - 1;
    if(ctx->data[pos] == '-')
        pos++;

    /* Parse the day and check the T between date and time */
    UA_UInt64 day = 0;
    len = parseUInt64(&ctx->data[pos], 2, &day);
    pos += len;
    UA_CHECK(len == 2 || ctx->data[pos] != 'T',
             return UA_STATUSCODE_BADDECODINGERROR);
    dts.tm_mday = (UA_UInt16)day;
    pos++;

    /* Parse the hour */
    UA_UInt64 hour = 0;
    len = parseUInt64(&ctx->data[pos], 2, &hour);
    pos += len;
    UA_CHECK(len == 2, return UA_STATUSCODE_BADDECODINGERROR);
    dts.tm_hour = (UA_UInt16)hour;
    if(ctx->data[pos] == ':')
        pos++;

    /* Parse the minute */
    UA_UInt64 min = 0;
    len = parseUInt64(&ctx->data[pos], 2, &min);
    pos += len;
    UA_CHECK(len == 2, return UA_STATUSCODE_BADDECODINGERROR);
    dts.tm_min = (UA_UInt16)min;
    if(ctx->data[pos] == ':')
        pos++;

    /* Parse the second */
    UA_UInt64 sec = 0;
    len = parseUInt64(&ctx->data[pos], 2, &sec);
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
    if(ctx->data[pos] == ',' || ctx->data[pos] == '.') {
        pos++;
        double frac = 0.0;
        double denom = 0.1;
        while(pos < ctx->length && ctx->data[pos] >= '0' && ctx->data[pos] <= '9') {
            frac += denom * (ctx->data[pos] - '0');
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
    if(pos != ctx->length - 1)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = dt;

    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Guid) {
    UA_String str = {ctx->length, (UA_Byte*)(uintptr_t)ctx->data};
    return UA_Guid_parse(dst, str);
}

DECODE_XML(NodeId) {
    UA_String str = {ctx->length, (UA_Byte*)(uintptr_t)ctx->data};
    return UA_NodeId_parse(dst, str);
}

DECODE_XML(ExpandedNodeId) {
    UA_String str = {ctx->length, (UA_Byte*)(uintptr_t)ctx->data};
    return UA_ExpandedNodeId_parse(dst, str);
}

static status
decodeXmlNotImplemented(ParseCtxXml *ctx, void *dst, const UA_DataType *type) {
    (void)dst, (void)type, (void)ctx;
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

const decodeXmlSignature decodeXmlJumpTable[UA_DATATYPEKINDS] = {
    (decodeXmlSignature)Boolean_decodeXml,          /* Boolean */
    (decodeXmlSignature)SByte_decodeXml,            /* SByte */
    (decodeXmlSignature)Byte_decodeXml,             /* Byte */
    (decodeXmlSignature)Int16_decodeXml,            /* Int16 */
    (decodeXmlSignature)UInt16_decodeXml,           /* UInt16 */
    (decodeXmlSignature)Int32_decodeXml,            /* Int32 */
    (decodeXmlSignature)UInt32_decodeXml,           /* UInt32 */
    (decodeXmlSignature)Int64_decodeXml,            /* Int64 */
    (decodeXmlSignature)UInt64_decodeXml,           /* UInt64 */
    (decodeXmlSignature)Float_decodeXml,            /* Float */
    (decodeXmlSignature)Double_decodeXml,           /* Double */
    (decodeXmlSignature)String_decodeXml,           /* String */
    (decodeXmlSignature)DateTime_decodeXml,         /* DateTime */
    (decodeXmlSignature)Guid_decodeXml,             /* Guid */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* ByteString */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* XmlElement */
    (decodeXmlSignature)NodeId_decodeXml,           /* NodeId */
    (decodeXmlSignature)ExpandedNodeId_decodeXml,   /* ExpandedNodeId */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* StatusCode */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* QualifiedName */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* LocalizedText */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* ExtensionObject */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* DataValue */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* Variant */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* DiagnosticInfo */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* Decimal */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* Enum */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* Structure */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* Structure with optional fields */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* Union */
    (decodeXmlSignature)decodeXmlNotImplemented     /* BitfieldCluster */
};

UA_StatusCode
UA_decodeXml(const UA_ByteString *src, void *dst, const UA_DataType *type,
              const UA_DecodeXmlOptions *options) {
    if(!dst || !src || !type)
        return UA_STATUSCODE_BADARGUMENTSMISSING;

    /* Set up the context */
    ParseCtxXml ctx;
    memset(&ctx, 0, sizeof(ParseCtxXml));
    ctx.data = (const char*)src->data;
    ctx.length = src->length;
    ctx.depth = 0;
    if(options) {
        ctx.customTypes = options->customTypes;
    }

    /* Decode */
    memset(dst, 0, type->memSize); /* Initialize the value */
    status ret = decodeXmlJumpTable[type->typeKind](&ctx, dst, type);

    if(ret != UA_STATUSCODE_GOOD) {
        UA_clear(dst, type);
        memset(dst, 0, type->memSize);
    }
    return ret;
}
