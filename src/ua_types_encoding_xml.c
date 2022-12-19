/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/config.h>
#include <open62541/types.h>

#include "ua_types_encoding_xml.h"

#include "../deps/itoa.h"
#include "../deps/parse_num.h"
#include "../deps/base64.h"
#include "../deps/libc_time.h"
#include "../deps/dtoa.h"

#include <libxml/parser.h>

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

/* Map for decoding a XML complex object type. An array of this is passed to the
 * decodeXmlFields function. If the xml element with name "fieldName" is found
 * in the xml complex object (mark as found) decode the value with the "function"
 * and write result into "fieldPointer" (destination). */
typedef struct {
    const char *fieldName;
    void *fieldPointer;
    decodeXmlSignature function;
    UA_Boolean found;
    const UA_DataType *type; /* Must be set for values that can be "null". If
                              * the function is not set, decode via the
                              * type->typeKind. */
} XmlDecodeEntry;

XmlEncTypeDef xmlEncTypeDefs[UA_DATATYPEKINDS] = {
    {"<xs:element name=\"Boolean\" nillable=\"true\" type=\"xs:boolean\"/>", 62},           /* Boolean */
    {"<xs:element name=\"SByte\" nillable=\"true\" type=\"xs:byte\"/>", 57},                /* SByte */
    {"<xs:element name=\"Byte\" nillable=\"true\" type=\"xs:unsignedByte\"/>", 64},         /* Byte */
    {"<xs:element name=\"Int16\" nillable=\"true\" type=\"xs:short\"/>", 58},               /* Int16 */
    {"<xs:element name=\"UInt16\" nillable=\"true\" type=\"xs:unsignedShort\"/>", 67},      /* UInt16 */
    {"<xs:element name=\"Int32\" nillable=\"true\" type=\"xs:int\"/>", 56},                 /* Int32 */
    {"<xs:element name=\"UInt32\" nillable=\"true\" type=\"xs:unsignedInt\"/>", 65},        /* UInt32 */
    {"<xs:element name=\"Int64\" nillable=\"true\" type=\"xs:long\"/>", 57},                /* Int64 */
    {"<xs:element name=\"UInt64\" nillable=\"true\" type=\"xs:unsignedLong\"/>", 66},       /* UInt64 */
    {"<xs:element name=\"Float\" nillable=\"true\" type=\"xs:float\"/>", 58},               /* Float */
    {"<xs:element name=\"Double\" nillable=\"true\" type=\"xs:double\"/>", 60},             /* Double */
    {"<xs:element name=\"String\" nillable=\"true\" type=\"xs:string\"/>", 60},             /* String */
    {"<xs:element name=\"DateTime\" nillable=\"true\" type=\"xs:dateTime\"/>", 64},         /* DateTime */
    {"<xs:complexType name=\"Guid\">"
       "<xs:sequence>"
         "<xs:element name=\"String\" type=\"xs:string\" minOccurs=\"0\" />"
       "</xs:sequence>"
     "</xs:complexType>", 131},                                                             /* Guid */
    {"<xs:element name=\"ByteString\" nillable=\"true\" type=\"xs:base64Binary\"/>", 70},   /* ByteString */
    {"", 0},                                                                                /* XmlElement */
    {"<xs:complexType name=\"NodeId\">"
       "<xs:sequence>"
         "<xs:element name=\"Identifier\" type=\"xs:string\" minOccurs=\"0\" />"
       "</xs:sequence>"
     "</xs:complexType>", 137},                                                             /* NodeId */
    {"<xs:complexType name=\"ExpandedNodeId\">"
       "<xs:sequence>"
         "<xs:element name=\"Identifier\" type=\"xs:string\" minOccurs=\"0\" />"
       "</xs:sequence>"
     "</xs:complexType>", 145},                                                             /* ExpandedNodeId */
    {"<xs:complexType name=\"StatusCode\">"
       "<xs:sequence>"
          "<xs:element name=\"Code\" type=\"xs:unsignedInt\" minOccurs=\"0\" />"
       "</xs:sequence>"
     "</xs:complexType>", 140},                                                             /* StatusCode */
    {"<xs:complexType name=\"QualifiedName\">"
       "<xs:sequence>"
         "<xs:element name=\"NamespaceIndex\" type=\"xs:int\" minOccurs=\"0\" />"
         "<xs:element name=\"Name\" type=\"xs:string\" minOccurs=\"0\" />"
       "</xs:sequence>"
     "</xs:complexType>", 202},                                                             /* QualifiedName */
    {"<xs:complexType name=\"LocalizedText\">"
       "<xs:sequence>"
         "<xs:element name=\"Locale\" type=\"xs:string\" minOccurs=\"0\" />"
         "<xs:element name=\"Text\" type=\"xs:string\" minOccurs=\"0\" />"
       "</xs:sequence>"
     "</xs:complexType>", 197},                                                             /* LocalizedText */
    {"", 0},                                                                                /* ExtensionObject */
    {"", 0},                                                                                /* DataValue */
    {"", 0},                                                                                /* Variant */
    {"", 0},                                                                                /* DiagnosticInfo */
    {"", 0},                                                                                /* Decimal */
    {"", 0},                                                                                /* Enum */
    {"", 0},                                                                                /* Structure */
    {"", 0},                                                                                /* Structure with optional fields */
    {"", 0},                                                                                /* Union */
    {"", 0}                                                                                 /* BitfieldCluster */
};

/* Elements for XML complex types */

/* Guid */
static const char* UA_XML_GUID_STRING = "String"; // String

/* NodeId */
static const char* UA_XML_NODEID_IDENTIFIER = "Identifier"; //String

/* ExpandedNodeId */
static const char* UA_XML_EXPANDEDNODEID_IDENTIFIER = "Identifier"; //String

/* StatusCode */
static const char* UA_XML_STATUSCODE_CODE = "Code"; // UInt32

/* QualifiedName */
static const char* UA_XML_QUALIFIEDNAME_NAMESPACEINDEX = "NamespaceIndex"; // Int32
static const char* UA_XML_QUALIFIEDNAME_NAME = "Name";                     // String

/* LocalizedText */
static const char* UA_XML_LOCALIZEDTEXT_LOCALE = "Locale"; // String
static const char* UA_XML_LOCALIZEDTEXT_TEXT = "Text";     // String

/************/
/* Encoding */
/************/

#define ENCODE_XML(TYPE) static status \
    TYPE##_encodeXml(CtxXml *ctx, const UA_##TYPE *src, const UA_DataType *type)

#define ENCODE_DIRECT_XML(SRC, TYPE) \
    TYPE##_encodeXml(ctx, (const UA_##TYPE*)SRC, NULL)

#define XML_TYPE_IS_PRIMITIVE \
    (type->typeKind < UA_DATATYPEKIND_GUID || \
    type->typeKind == UA_DATATYPEKIND_BYTESTRING)

static status UA_FUNC_ATTR_WARN_UNUSED_RESULT
xmlEncodeWriteChars(CtxXml *ctx, const char *c, size_t len) {
    if(ctx->pos + len > ctx->end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(!ctx->calcOnly)
        memcpy(ctx->pos, c, len);
    ctx->pos += len;
    return UA_STATUSCODE_GOOD;
}

static status UA_FUNC_ATTR_WARN_UNUSED_RESULT
writeXmlElemNameBegin(CtxXml *ctx, const char* name) {
    status ret = UA_STATUSCODE_GOOD;
    if(!ctx->printValOnly) {
        ret |= xmlEncodeWriteChars(ctx, "<", 1);
        ret |= xmlEncodeWriteChars(ctx, name, strlen(name));
        ret |= xmlEncodeWriteChars(ctx, ">", 1);
    }
    return ret;
}

static status UA_FUNC_ATTR_WARN_UNUSED_RESULT
writeXmlElemNameEnd(CtxXml *ctx, const char* name) {
    status ret = UA_STATUSCODE_GOOD;
    if(!ctx->printValOnly) {
        ret |= xmlEncodeWriteChars(ctx, "</", 2);
        ret |= xmlEncodeWriteChars(ctx, name, strlen(name));
        ret |= xmlEncodeWriteChars(ctx, ">", 1);
    }
    return ret;
}

static status UA_FUNC_ATTR_WARN_UNUSED_RESULT
writeXmlElement(CtxXml *ctx, const char *name,
                const void *value, const UA_DataType *type) {
    status ret = UA_STATUSCODE_GOOD;
    UA_Boolean prevPrintVal = ctx->printValOnly;
    ctx->printValOnly = false;
    ret |= writeXmlElemNameBegin(ctx, name);
    ctx->printValOnly = XML_TYPE_IS_PRIMITIVE;
    ret |= encodeXmlJumpTable[type->typeKind](ctx, value, type);
    ctx->printValOnly = false;
    ret |= writeXmlElemNameEnd(ctx, name);
    ctx->printValOnly = prevPrintVal;
    return ret;
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
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_ByteString hexBuf;
    UA_ByteString_allocBuffer(&hexBuf, 36);
    UA_Guid_to_hex(src, hexBuf.data, false);

    ret |= writeXmlElement(ctx, UA_XML_GUID_STRING,
                           &hexBuf, &UA_TYPES[UA_TYPES_STRING]);

    UA_ByteString_clear(&hexBuf);

    return ret;
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

/* ByteString */
ENCODE_XML(ByteString) {
    if(!src->data)
        return xmlEncodeWriteChars(ctx, "null", 4);

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

    return UA_STATUSCODE_GOOD;
}

/* NodeId */
ENCODE_XML(NodeId) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_String out = UA_STRING_NULL;

    ret |= UA_NodeId_print(src, &out);
    ret |= writeXmlElement(ctx, UA_XML_NODEID_IDENTIFIER,
                           &out, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&out);

    return ret;
}

/* ExpandedNodeId */
ENCODE_XML(ExpandedNodeId) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_String out = UA_STRING_NULL;

    ret |= UA_ExpandedNodeId_print(src, &out);
    ret |= writeXmlElement(ctx, UA_XML_EXPANDEDNODEID_IDENTIFIER,
                           &out, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&out);

    return ret;
}

/* StatusCode */
ENCODE_XML(StatusCode) {
    return writeXmlElement(ctx, UA_XML_STATUSCODE_CODE,
                           src, &UA_TYPES[UA_TYPES_UINT32]);
}

/* QualifiedName */
ENCODE_XML(QualifiedName) {
    UA_StatusCode ret =
        writeXmlElement(ctx, UA_XML_QUALIFIEDNAME_NAMESPACEINDEX,
                        &src->namespaceIndex, &UA_TYPES[UA_TYPES_INT32]);

    if(ret == UA_STATUSCODE_GOOD)
        ret = writeXmlElement(ctx, UA_XML_QUALIFIEDNAME_NAME,
                              &src->name, &UA_TYPES[UA_TYPES_STRING]);
    return ret;
}

/* LocalizedText */
ENCODE_XML(LocalizedText) {
    UA_StatusCode ret =
        writeXmlElement(ctx, UA_XML_LOCALIZEDTEXT_LOCALE,
                        &src->locale, &UA_TYPES[UA_TYPES_STRING]);

    if(ret == UA_STATUSCODE_GOOD)
        ret = writeXmlElement(ctx, UA_XML_LOCALIZEDTEXT_TEXT,
                              &src->text, &UA_TYPES[UA_TYPES_STRING]);
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
    (encodeXmlSignature)ByteString_encodeXml,       /* ByteString */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* XmlElement */
    (encodeXmlSignature)NodeId_encodeXml,           /* NodeId */
    (encodeXmlSignature)ExpandedNodeId_encodeXml,   /* ExpandedNodeId */
    (encodeXmlSignature)StatusCode_encodeXml,       /* StatusCode */
    (encodeXmlSignature)QualifiedName_encodeXml,    /* QualifiedName */
    (encodeXmlSignature)LocalizedText_encodeXml,    /* LocalizedText */
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
    ctx.printValOnly = false;
    if(options)
        ctx.prettyPrint = options->prettyPrint;

    /* Encode */
    res |= xmlEncodeWriteChars(&ctx,
            xmlEncTypeDefs[type->typeKind].xmlEncTypeDef,
            xmlEncTypeDefs[type->typeKind].xmlEncTypeDefLen);
    res |= writeXmlElement(&ctx, type->typeName, src, type);

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
    ctx.printValOnly = false;
    if(options) {
        ctx.prettyPrint = options->prettyPrint;
    }

    ctx.calcOnly = true;

    /* Encode */
    status ret = xmlEncodeWriteChars(&ctx,
                    xmlEncTypeDefs[type->typeKind].xmlEncTypeDef,
                    xmlEncTypeDefs[type->typeKind].xmlEncTypeDefLen);
    ret |= writeXmlElement(&ctx, type->typeName, src, type);
    if(ret != UA_STATUSCODE_GOOD)
        return 0;
    return (size_t)ctx.pos;
}

/**********/
/* Decode */
/**********/

#define CHECK_DATA_BOUNDS                           \
    do {                                            \
        if(ctx->index >= ctx->membersSize)          \
            return UA_STATUSCODE_BADDECODINGERROR;  \
    } while(0)

#define GET_DATA_VALUE                                                  \
    const char *data = NULL;                                            \
    size_t length = 0;                                                  \
    if(ctx->dataMembers[ctx->index]->type == XML_DATA_TYPE_PRIMITIVE) { \
        data = ctx->dataMembers[ctx->index]->value.primitive.value;     \
        length = ctx->dataMembers[ctx->index]->value.primitive.length;  \
    }                                                                   \
    do {} while(0)

static void
skipXmlObject(ParseCtxXml *ctx) {
    if(ctx->value->data->type == XML_DATA_TYPE_PRIMITIVE)
        ctx->index++;
    else {
        size_t objMemberIdx = 0;
        do {
            ctx->index++;
            objMemberIdx++;
        } while(ctx->index < ctx->membersSize &&
                objMemberIdx < ctx->dataMembers[ctx->index]->value.complex.membersSize);
    }
}

/* Forward declarations*/
#define DECODE_XML(TYPE) static status                   \
    TYPE##_decodeXml(ParseCtxXml *ctx, UA_##TYPE *dst,  \
                      const UA_DataType *type)

DECODE_XML(Boolean) {
    CHECK_DATA_BOUNDS;
    GET_DATA_VALUE;

    if(length == 4 &&
       data[0] == 't' && data[1] == 'r' &&
       data[2] == 'u' && data[3] == 'e') {
        *dst = true;
    } else if(length == 5 &&
              data[0] == 'f' && data[1] == 'a' &&
              data[2] == 'l' && data[3] == 's' &&
              data[4] == 'e') {
        *dst = false;
    } else {
        return UA_STATUSCODE_BADDECODINGERROR;
    }

    ctx->index++;
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
    CHECK_DATA_BOUNDS;
    GET_DATA_VALUE;

    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(data, length, &out);

    if(s != UA_STATUSCODE_GOOD || out < UA_SBYTE_MIN || out > UA_SBYTE_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_SByte)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Byte) {
    /* TODO:
     *   1. Add support for optional "+" sign.
     *   2. Add support for optional leading zeros.
     *   3. Check if the value is in hex, octal or binray.
     *   4. Check if decimal point exists. */
    CHECK_DATA_BOUNDS;
    GET_DATA_VALUE;

    UA_UInt64 out = 0;
    UA_StatusCode s = decodeUnsigned(data, length, &out);

    if(s != UA_STATUSCODE_GOOD || out > UA_BYTE_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Byte)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Int16) {
    /* TODO:
     *   1. Add support for optional "+" sign.
     *   2. Add support for optional leading zeros.
     *   3. Check if the value is in hex, octal or binray.
     *   4. Check if decimal point exists. */
    CHECK_DATA_BOUNDS;
    GET_DATA_VALUE;

    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(data, length, &out);

    if(s != UA_STATUSCODE_GOOD || out < UA_INT16_MIN || out > UA_INT16_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Int16)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(UInt16) {
    /* TODO:
     *   1. Add support for optional "+" sign.
     *   2. Add support for optional leading zeros.
     *   3. Check if the value is in hex, octal or binray.
     *   4. Check if decimal point exists. */
    CHECK_DATA_BOUNDS;
    GET_DATA_VALUE;

    UA_UInt64 out = 0;
    UA_StatusCode s = decodeUnsigned(data, length, &out);

    if(s != UA_STATUSCODE_GOOD || out > UA_UINT16_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_UInt16)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Int32) {
    /* TODO:
     *   1. Add support for optional "+" sign.
     *   2. Add support for optional leading zeros.
     *   3. Check if the value is in hex, octal or binray.
     *   4. Check if decimal point exists.
     *   5. Check "-0" and "+0", and just remove the sign. */
    CHECK_DATA_BOUNDS;
    GET_DATA_VALUE;

    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(data, length, &out);

    if(s != UA_STATUSCODE_GOOD || out < UA_INT32_MIN || out > UA_INT32_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Int32)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(UInt32) {
    /* TODO:
     *   1. Add support for optional "+" sign.
     *   2. Add support for optional leading zeros.
     *   3. Check if the value is in hex, octal or binray.
     *   4. Check if decimal point exists. */
    CHECK_DATA_BOUNDS;
    GET_DATA_VALUE;

    UA_UInt64 out = 0;
    UA_StatusCode s = decodeUnsigned(data, length, &out);

    if(s != UA_STATUSCODE_GOOD || out > UA_UINT32_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_UInt32)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Int64) {
    /* TODO:
     *   1. Add support for optional "+" sign.
     *   2. Add support for optional leading zeros.
     *   3. Check if the value is in hex, octal or binray.
     *   4. Check if decimal point exists. */
    CHECK_DATA_BOUNDS;
    GET_DATA_VALUE;

    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(data, length, &out);

    if(s != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Int64)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(UInt64) {
    /* TODO:
     *   1. Add support for optional "+" sign.
     *   2. Add support for optional leading zeros.
     *   3. Check if the value is in hex, octal or binray.
     *   4. Check if decimal point exists. */
    CHECK_DATA_BOUNDS;
    GET_DATA_VALUE;

    UA_UInt64 out = 0;
    UA_StatusCode s = decodeUnsigned(data, length, &out);

    if(s != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_UInt64)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Double) {
    CHECK_DATA_BOUNDS;
    GET_DATA_VALUE;

    /* https://www.exploringbinary.com/maximum-number-of-decimal-digits-in-binary-floating-point-numbers/
     * Maximum digit counts for select IEEE floating-point formats: 1074
     * Sanity check.
     */
    if(length > 1075)
        return UA_STATUSCODE_BADDECODINGERROR;

    ctx->index++;

    if(length == 3 && memcmp(data, "INF", 3) == 0) {
        *dst = INFINITY;
        return UA_STATUSCODE_GOOD;
    }

    if(length == 4 && memcmp(data, "-INF", 4) == 0) {
        *dst = -INFINITY;
        return UA_STATUSCODE_GOOD;
    }

    if(length == 3 && memcmp(data, "NaN", 3) == 0) {
        *dst = NAN;
        return UA_STATUSCODE_GOOD;
    }

    size_t len = parseDouble(data, length, dst);
    if(len == 0)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* There must only be whitespace between the end of the parsed number and
     * the end of the token */
    for(size_t i = len; i < length; i++) {
        if(data[i] != ' ' && data[i] -'\t' >= 5)
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
    CHECK_DATA_BOUNDS;
    GET_DATA_VALUE;

    /* Empty string? */
    if(length == 0) {
        dst->data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
        dst->length = 0;
        ctx->index++;
        return UA_STATUSCODE_GOOD;
    }

    /* Set the output */
    dst->length = length;
    if(dst->length > 0) {
        UA_String str = {length, (UA_Byte*)(uintptr_t)data};
        UA_String_copy(&str, dst);
    } else {
        dst->data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
    }

    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(DateTime) {
    CHECK_DATA_BOUNDS;
    GET_DATA_VALUE;

    /* The last character has to be 'Z'. We can omit some length checks later on
     * because we know the atoi functions stop before the 'Z'. */
    if(length == 0 || data[length - 1] != 'Z')
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
    if(data[0] == '-' || data[0] == '+')
        pos++;
    UA_Int64 year = 0;
    len = parseInt64(&data[pos], 5, &year);
    pos += len;
    if(len != 4 && data[pos] != '-')
        return UA_STATUSCODE_BADDECODINGERROR;
    if(data[0] == '-')
        year = -year;
    dts.tm_year = (UA_Int16)year - 1900;
    if(data[pos] == '-')
        pos++;

    /* Parse the month */
    UA_UInt64 month = 0;
    len = parseUInt64(&data[pos], 2, &month);
    pos += len;
    UA_CHECK(len == 2, return UA_STATUSCODE_BADDECODINGERROR);
    dts.tm_mon = (UA_UInt16)month - 1;
    if(data[pos] == '-')
        pos++;

    /* Parse the day and check the T between date and time */
    UA_UInt64 day = 0;
    len = parseUInt64(&data[pos], 2, &day);
    pos += len;
    UA_CHECK(len == 2 || data[pos] != 'T',
             return UA_STATUSCODE_BADDECODINGERROR);
    dts.tm_mday = (UA_UInt16)day;
    pos++;

    /* Parse the hour */
    UA_UInt64 hour = 0;
    len = parseUInt64(&data[pos], 2, &hour);
    pos += len;
    UA_CHECK(len == 2, return UA_STATUSCODE_BADDECODINGERROR);
    dts.tm_hour = (UA_UInt16)hour;
    if(data[pos] == ':')
        pos++;

    /* Parse the minute */
    UA_UInt64 min = 0;
    len = parseUInt64(&data[pos], 2, &min);
    pos += len;
    UA_CHECK(len == 2, return UA_STATUSCODE_BADDECODINGERROR);
    dts.tm_min = (UA_UInt16)min;
    if(data[pos] == ':')
        pos++;

    /* Parse the second */
    UA_UInt64 sec = 0;
    len = parseUInt64(&data[pos], 2, &sec);
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
    if(data[pos] == ',' || data[pos] == '.') {
        pos++;
        double frac = 0.0;
        double denom = 0.1;
        while(pos < length && data[pos] >= '0' && data[pos] <= '9') {
            frac += denom * (data[pos] - '0');
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
    if(pos != length - 1)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = dt;

    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

static status
decodeXmlFields(ParseCtxXml *ctx, XmlDecodeEntry *entries, size_t entryCount) {
    CHECK_DATA_BOUNDS;

    size_t objectCount = 0;
    if(ctx->dataMembers[ctx->index]->type == XML_DATA_TYPE_COMPLEX)
        objectCount = ctx->dataMembers[ctx->index]->value.complex.membersSize;

    /* Empty object, nothing to decode */
    if(objectCount == 0) {
        ctx->index++; /* Jump to the element after the empty object */
        return UA_STATUSCODE_GOOD;
    }

    ctx->index++; /* Go to first entry element */

    status ret = UA_STATUSCODE_GOOD;
    for(size_t currObj = 0; currObj < objectCount &&
            ctx->index < ctx->membersSize; currObj++) {
        /* For every object -> check if any of the entries
         * match this (order of entries is not needed).
         * Start searching at the index of currObj */
        for(size_t i = currObj; i < entryCount + currObj; i++) {
            /* Search for key, if found outer loop will be one less. Best case
             * if objectCount is in order! */
            size_t index = i % entryCount;

            /* CHECK_DATA_BOUNDS */
            if(ctx->index >= ctx->membersSize)
                return UA_STATUSCODE_BADDECODINGERROR;

            if(strcmp(ctx->dataMembers[ctx->index]->name, entries[index].fieldName))
                continue;

            /* Duplicate key found, abort */
            if(entries[index].found)
                return UA_STATUSCODE_BADDECODINGERROR;

            entries[index].found = true;

            /* An entry that was expected, but shall not be decoded.
             * Jump over it. */
            if(!entries[index].function && !entries[index].type) {
                skipXmlObject(ctx);
                break;
            }

            /* A null-value -> skip the decoding. */
            if(!entries[index].fieldPointer && entries[index].type) {
                ctx->index++;
                break;
            }

            /* Decode */
            if(entries[index].function) /* Specialized decoding function */
                ret = entries[index].function(ctx, entries[index].fieldPointer,
                                              entries[index].type);
            else /* Decode by type-kind */
                ret = decodeXmlJumpTable[entries[index].type->typeKind]
                    (ctx, entries[index].fieldPointer, entries[index].type);
            if(ret != UA_STATUSCODE_GOOD)
                return ret;
            break;
        }
    }

    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Guid) {
    CHECK_DATA_BOUNDS;

    UA_String str;
    UA_String_init(&str);
    XmlDecodeEntry entry = {
        UA_XML_GUID_STRING, &str, NULL, false, &UA_TYPES[UA_TYPES_STRING]
    };

    status ret = decodeXmlFields(ctx, &entry, 1);
    ret |= UA_Guid_parse(dst, str);

    UA_String_clear(&str);
    return ret;
}

DECODE_XML(ByteString) {
    CHECK_DATA_BOUNDS;
    GET_DATA_VALUE;

    /* Empty bytestring? */
    if(length == 0) {
        dst->data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
        dst->length = 0;
    } else {
        size_t flen = 0;
        unsigned char* unB64 =
            UA_unbase64((const unsigned char*)data, length, &flen);
        if(unB64 == 0)
            return UA_STATUSCODE_BADDECODINGERROR;
        dst->data = (UA_Byte*)unB64;
        dst->length = flen;
    }

    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(NodeId) {
    CHECK_DATA_BOUNDS;

    UA_String str;
    UA_String_init(&str);
    XmlDecodeEntry entry = {
        UA_XML_NODEID_IDENTIFIER, &str, NULL, false, &UA_TYPES[UA_TYPES_STRING]
    };

    status ret = decodeXmlFields(ctx, &entry, 1);
    ret |= UA_NodeId_parse(dst, str);

    UA_String_clear(&str);
    return ret;
}

DECODE_XML(ExpandedNodeId) {
    CHECK_DATA_BOUNDS;

    UA_String str;
    UA_String_init(&str);
    XmlDecodeEntry entry = {
        UA_XML_EXPANDEDNODEID_IDENTIFIER, &str, NULL, false, &UA_TYPES[UA_TYPES_STRING]
    };

    status ret = decodeXmlFields(ctx, &entry, 1);
    ret |= UA_ExpandedNodeId_parse(dst, str);

    UA_String_clear(&str);
    return ret;
}

DECODE_XML(StatusCode) {
    CHECK_DATA_BOUNDS;

    XmlDecodeEntry entry = {
        UA_XML_STATUSCODE_CODE, dst, NULL, false, &UA_TYPES[UA_TYPES_UINT32]
    };

    return decodeXmlFields(ctx, &entry, 1);
}

DECODE_XML(QualifiedName) {
    CHECK_DATA_BOUNDS;

    XmlDecodeEntry entries[2] = {
        {UA_XML_QUALIFIEDNAME_NAMESPACEINDEX, &dst->namespaceIndex, NULL, false, &UA_TYPES[UA_TYPES_UINT16]},
        {UA_XML_QUALIFIEDNAME_NAME, &dst->name, NULL, false, &UA_TYPES[UA_TYPES_STRING]}
    };

    return decodeXmlFields(ctx, entries, 2);
}

DECODE_XML(LocalizedText) {
    CHECK_DATA_BOUNDS;

    XmlDecodeEntry entries[2] = {
        {UA_XML_LOCALIZEDTEXT_LOCALE, &dst->locale, NULL, false, &UA_TYPES[UA_TYPES_STRING]},
        {UA_XML_LOCALIZEDTEXT_TEXT, &dst->text, NULL, false, &UA_TYPES[UA_TYPES_STRING]}
    };

    return decodeXmlFields(ctx, entries, 2);
}

static status
Array_decodeXml(ParseCtxXml *ctx, void **dst, const UA_DataType *type) {
    (void)dst, (void)type, (void)ctx;
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
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
    (decodeXmlSignature)ByteString_decodeXml,       /* ByteString */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* XmlElement */
    (decodeXmlSignature)NodeId_decodeXml,           /* NodeId */
    (decodeXmlSignature)ExpandedNodeId_decodeXml,   /* ExpandedNodeId */
    (decodeXmlSignature)StatusCode_decodeXml,       /* StatusCode */
    (decodeXmlSignature)QualifiedName_decodeXml,    /* QualifiedName */
    (decodeXmlSignature)LocalizedText_decodeXml,    /* LocalizedText */
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

static XmlData*
newXmlData(const char *name, XmlDataType type) {
    XmlData *newData = (XmlData*)calloc(1, sizeof(XmlData));
    newData->type = type;
    size_t nameLen = strlen(name) + 1;
    newData->name = (const char*)calloc(1, nameLen);
    memcpy((void*)(uintptr_t)newData->name, (void*)(uintptr_t)name, nameLen);
    return newData;
}

static XmlData*
addNewXmlMember(XmlData *parent, const char *name) {
    parent->type = XML_DATA_TYPE_COMPLEX;
    parent->value.complex.members =
        (XmlData**)UA_realloc(parent->value.complex.members,
        (parent->value.complex.membersSize + 1) * sizeof(XmlData*));

    XmlData *newData = (XmlData*)UA_calloc(1, sizeof(XmlData));
    parent->value.complex.members[parent->value.complex.membersSize] = newData;

    parent->value.complex.membersSize++;
    newData->type = XML_DATA_TYPE_PRIMITIVE;
    size_t nameLen = strlen(name) + 1;
    newData->name = (const char*)calloc(1, nameLen);
    memcpy((void*)(uintptr_t)newData->name, (void*)(uintptr_t)name, nameLen);
    return newData;
}

static void
deleteData(XmlData *data) {
    if(data->type == XML_DATA_TYPE_PRIMITIVE) {
        if(data->value.primitive.value != NULL)
            UA_free((void*)(uintptr_t)data->value.primitive.value);
    }
    else {
        for(size_t cnt = 0LU; cnt < data->value.complex.membersSize; ++cnt)
            deleteData(data->value.complex.members[cnt]);
        UA_free(data->value.complex.members);
    }
    UA_free((void*)(uintptr_t)data->name);
    UA_free(data);
}

static void
OnStartElementNsXml(void *ctx, const xmlChar *localname,
                    const xmlChar *prefix, const xmlChar *URI,
                    int nb_namespaces, const xmlChar **namespaces,
                    int nb_attributes, int nb_defaulted,
                    const xmlChar **attributes) {

    ParseCtxXml *ctxt = (ParseCtxXml*)ctx;
    XmlParsingCtx *pctxt = ctxt->parseCtx;
    XmlValue *val = ctxt->value;
    const char *localNameChr = (const char*)localname;

    if(!strncmp(localNameChr, "ListOf", strlen("ListOf")))
        val->isArray = UA_TRUE;

    if(!pctxt->data) {
        val->data = newXmlData(localNameChr, XML_DATA_TYPE_PRIMITIVE);
        pctxt->data = val->data;
    }
    else {
        XmlData *newData = addNewXmlMember(pctxt->data, localNameChr);
        XmlData *parent = pctxt->data;
        pctxt->data = newData;
        pctxt->data->parent = parent;
    }
    ctxt->dataMembers[ctxt->membersSize++] = pctxt->data;
}

static void
OnEndElementNsXml(void *ctx, const xmlChar *localname,
                  const xmlChar *prefix, const xmlChar *URI) {
    ParseCtxXml *ctxt = (ParseCtxXml*)ctx;
    XmlParsingCtx *pctxt = ctxt->parseCtx;
    __attribute__((unused)) const char *localNameChr = (const char*)localname;

    /* Names must be the same for the start and end segment. */
    UA_assert(!strcmp(localNameChr, pctxt->data->name));

    if(pctxt->onCharacters != NULL) {
        pctxt->data->value.primitive.value = pctxt->onCharacters;
        pctxt->data->value.primitive.length = pctxt->onCharLength;
        pctxt->onCharacters = NULL;
        pctxt->onCharLength = 0;
    }

    pctxt->data = pctxt->data->parent;
}

static void
OnCharactersXml(void *ctx, const xmlChar *ch, int len) {

    ParseCtxXml *ctxt = (ParseCtxXml*)ctx;
    XmlParsingCtx *pctxt = ctxt->parseCtx;
    size_t length = (size_t)len;
    size_t pos = 0;
    if(pctxt->onCharacters == NULL) {
        char *newValue = (char*)UA_malloc(length + 1);
        pctxt->onCharacters = newValue;
        memset(pctxt->onCharacters, 0, length + 1);
    }
    else {
        pos += pctxt->onCharLength;
        pctxt->onCharacters = (char*)UA_realloc(pctxt->onCharacters, length + 1);
        void* pStart = pctxt->onCharacters + pos;
        memset(pStart, 0, length + 1);
    }

    memcpy(pctxt->onCharacters + pos, (const char*)ch, length);
    pctxt->onCharLength += length;
}

static status
parseXml(ParseCtxXml *ctx, const char *data, size_t length) {

    xmlSAXHandler SAXHander;

    memset(&SAXHander, 0, sizeof(xmlSAXHandler));

    SAXHander.initialized = XML_SAX2_MAGIC;
    SAXHander.startElementNs = OnStartElementNsXml;
    SAXHander.endElementNs = OnEndElementNsXml;
    SAXHander.characters = OnCharactersXml;
 
    xmlParserCtxtPtr ctxt = xmlCreatePushParserCtxt(&SAXHander, (void*)ctx, NULL, 0, NULL);

    int ret = xmlParseChunk(ctxt, data, (int)length, 0);
    if(ret != XML_ERR_OK) {
        xmlParserError(ctxt, "xmlParseChunk");
        return UA_STATUSCODE_BADDECODINGERROR;
    }

    xmlParseChunk(ctxt, data, 0, 1);
    xmlFreeParserCtxt(ctxt);
    xmlCleanupParser();
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_decodeXml(const UA_ByteString *src, void *dst, const UA_DataType *type,
              const UA_DecodeXmlOptions *options) {
    if(!dst || !src || !type)
        return UA_STATUSCODE_BADARGUMENTSMISSING;

    /* Set up the context */
    ParseCtxXml ctx;
    XmlParsingCtx parseCtx;
    XmlValue value;

    memset(&value, 0, sizeof(XmlValue));
    memset(&parseCtx, 0, sizeof(XmlParsingCtx));
    memset(&ctx, 0, sizeof(ParseCtxXml));
    memset(dst, 0, type->memSize);

    ctx.dataMembers = (XmlData**)UA_malloc(UA_XML_MAXMEMBERSCOUNT * sizeof(XmlData*));
    ctx.parseCtx = &parseCtx;
    ctx.value = &value;
    ctx.index = 0;
    if(options) {
        ctx.customTypes = options->customTypes;
    }

    status ret = parseXml(&ctx, (const char*)src->data, src->length);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_clear(dst, type);
        UA_free(ctx.dataMembers);
        memset(dst, 0, type->memSize);
        goto finish;
    }

    /* Decode */
    memset(dst, 0, type->memSize); /* Initialize the value */

    if(ctx.value->isArray)
        ret = Array_decodeXml(&ctx, (void**)dst, type);
    else
        ret = decodeXmlJumpTable[type->typeKind](&ctx, dst, type);

    if(ret != UA_STATUSCODE_GOOD) {
        deleteData(ctx.value->data);
        UA_free(ctx.dataMembers);
        UA_clear(dst, type);
        memset(dst, 0, type->memSize);
        goto finish;
    }

    deleteData(ctx.value->data);
    UA_free(ctx.dataMembers);

finish:
    return ret;
}
