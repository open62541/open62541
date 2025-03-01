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
#include "../deps/yxml.h"

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

/* Replicate yxml_isNameStart and yxml_isName from yxml */
static UA_String
backtrackName(const char *xml, unsigned end) {
    unsigned pos = end;
    for(; pos > 0; pos--) {
        unsigned char c = (unsigned char)xml[pos-1];
        if(c >= 'a' && c <= 'z') continue; /* isAlpha */
        if(c >= 'A' && c <= 'Z') continue; /* isAlpha */
        if(c >= '0' && c <= '9') continue; /* isNum */
        if(c == ':' || c == '_' || c >= 128 || c == '-'|| c == '.') continue;
        break;
    }
    UA_String s = {end - pos, (UA_Byte*)(uintptr_t)xml + pos};
    return s;
}

xml_result
xml_tokenize(const char *xml, unsigned int len,
             xml_token *tokens, unsigned int max_tokens) {
    xml_result res;
    memset(&res, 0, sizeof(xml_result));
    res.tokens = tokens;

    yxml_t ctx;
    char buf[512];
    yxml_init(&ctx, buf, 512);

    unsigned char top = 0;
    unsigned tokenPos = 0;
    xml_token *stack[32]; /* Max nesting depth is 32 */
    xml_token backup_tokens[32]; /* To be used when the tokens run out */

    stack[top] = &backup_tokens[top];
    memset(stack[top], 0, sizeof(xml_token));

    unsigned val_begin = 0;
    unsigned pos = 0;
    for(; pos < len; pos++) {
        yxml_ret_t status = yxml_parse(&ctx, xml[pos]);
        switch(status) {
        case YXML_EEOF:
        case YXML_EREF:
        case YXML_ECLOSE:
        case YXML_ESTACK:
        case YXML_ESYN:
        default:
            goto errout;
        case YXML_OK:
            continue;
        case YXML_ELEMSTART:
        case YXML_ATTRSTART:
            if(status == YXML_ELEMSTART)
                stack[top]->children++;
            else
                stack[top]->attributes++;
            top++;
            if(top >= 32)
                goto errout; /* nesting too deep */
            stack[top] = (tokenPos < max_tokens) ? &tokens[tokenPos] : &backup_tokens[top];
            memset(stack[top], 0, sizeof(xml_token));
            stack[top]->type = (status == YXML_ELEMSTART) ? XML_TOKEN_ELEMENT : XML_TOKEN_ATTRIBUTE;
            stack[top]->name = backtrackName(xml, pos);
            const char *start = xml + pos;
            if(status == YXML_ELEMSTART) {
                while(*start != '<')
                    start--;
            }
            stack[top]->start = (unsigned)(start - xml);
            tokenPos++;
            break;
        case YXML_CONTENT:
        case YXML_ATTRVAL:
            if(val_begin == 0)
                val_begin = pos;
            stack[top]->end = pos;
            break;
        case YXML_ELEMEND:
        case YXML_ATTREND:
            if(val_begin > 0) {
                stack[top]->content.data = (UA_Byte*)(uintptr_t)xml + val_begin;
                stack[top]->content.length = stack[top]->end + 1 - val_begin;
            }
            stack[top]->end = pos;
            if(status == YXML_ELEMEND) {
                while(xml[stack[top]->end] != '>')
                    stack[top]->end++;
                stack[top]->end++;
            }
            val_begin = 0;
            top--;
            break;
        case YXML_PISTART:
        case YXML_PICONTENT:
        case YXML_PIEND:
            continue; /* Ignore processing instructions */
        }
    }

    res.num_tokens = tokenPos;
    if(tokenPos >= max_tokens) 
        res.error = XML_ERROR_OVERFLOW;
    return res;

 errout:
    res.error_pos = pos;
    if(yxml_eof(&ctx) != YXML_OK)
        res.error = XML_ERROR_INVALID;

    return res;
}

/* Map for decoding a XML complex object type. An array of this is passed to the
 * decodeXmlFields function. If the xml element with name "fieldName" is found
 * in the xml complex object (mark as found) decode the value with the "function"
 * and write result into "fieldPointer" (destination). */
typedef struct {
    UA_String name;
    void *fieldPointer;
    decodeXmlSignature function;
    UA_Boolean found;
    const UA_DataType *type; /* Must be set for values that can be "null". If
                              * the function is not set, decode via the
                              * type->typeKind. */
} XmlDecodeEntry;

/* Elements for XML complex types */
#define UA_XML_GUID_STRING "String"
#define UA_XML_NODEID_IDENTIFIER "Identifier"
#define UA_XML_EXPANDEDNODEID_IDENTIFIER "Identifier"
#define UA_XML_STATUSCODE_CODE "Code"
#define UA_XML_QUALIFIEDNAME_NAMESPACEINDEX "NamespaceIndex"
#define UA_XML_QUALIFIEDNAME_NAME "Name"
#define UA_XML_LOCALIZEDTEXT_LOCALE "Locale"
#define UA_XML_LOCALIZEDTEXT_TEXT "Text"
#define UA_XML_EXTENSIONOBJECT_TYPEID "TypeId"
#define UA_XML_EXTENSIONOBJECT_BODY "Body"
#define UA_XML_EXTENSIONOBJECT_BYTESTRING "ByteString"
#define UA_XML_VARIANT_VALUE "Value"

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

static bool
isNullXml(const void *p, const UA_DataType *type) {
    if(UA_DataType_isNumeric(type) || type->typeKind == UA_DATATYPEKIND_BOOLEAN)
        return false;
    UA_STACKARRAY(char, buf, type->memSize);
    memset(buf, 0, type->memSize);
    return (UA_order(buf, p, type) == UA_ORDER_EQ);
}

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
    if(ctx->depth >= UA_XML_ENCODING_MAX_RECURSION - 1)
        return UA_STATUSCODE_BADENCODINGERROR;
    status ret = UA_STATUSCODE_GOOD;
    if(!ctx->printValOnly) {
        if(ctx->prettyPrint) {
            ret |= xmlEncodeWriteChars(ctx, "\n", 1);
            for(size_t i = 0; i < ctx->depth; ++i)
                ret |= xmlEncodeWriteChars(ctx, "\t", 1);
        }
        ret |= xmlEncodeWriteChars(ctx, "<", 1);
        ret |= xmlEncodeWriteChars(ctx, name, strlen(name));
        ret |= xmlEncodeWriteChars(ctx, ">", 1);
    }
    ctx->depth++;
    return ret;
}

static status UA_FUNC_ATTR_WARN_UNUSED_RESULT
writeXmlElemNameEnd(CtxXml *ctx, const char* name,
                    const UA_DataType *type) {
    if(ctx->depth == 0)
        return UA_STATUSCODE_BADENCODINGERROR;
    ctx->depth--;
    status ret = UA_STATUSCODE_GOOD;
    if(!ctx->printValOnly) {
        if(ctx->prettyPrint && !XML_TYPE_IS_PRIMITIVE) {
            ret |= xmlEncodeWriteChars(ctx, "\n", 1);
            for(size_t i = 0; i < ctx->depth; ++i)
                ret |= xmlEncodeWriteChars(ctx, "\t", 1);
        }
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
    ret |= writeXmlElemNameEnd(ctx, name, type);
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
    UA_Byte buf[36];
    UA_ByteString hexBuf = {36, buf};
    UA_Guid_to_hex(src, hexBuf.data, false);
    return writeXmlElement(ctx, UA_XML_GUID_STRING, &hexBuf,
                           &UA_TYPES[UA_TYPES_STRING]);
}

/* DateTime */
ENCODE_XML(DateTime) {
    UA_Byte buffer[40];
    UA_String str = {40, buffer};
    encodeDateTime(*src, &str);
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

/* ExtensionObject */
ENCODE_XML(ExtensionObject) {
    if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY)
        return xmlEncodeWriteChars(ctx, "null", 4);

    /* The body of the ExtensionObject contains a single element
     * which is either a ByteString or XML encoded Structure:
     * https://reference.opcfoundation.org/Core/Part6/v104/docs/5.3.1.16. */
    if(src->encoding != UA_EXTENSIONOBJECT_ENCODED_BYTESTRING &&
       src->encoding != UA_EXTENSIONOBJECT_ENCODED_XML)
        return UA_STATUSCODE_BADENCODINGERROR;

    /* Write the type NodeId */
    UA_StatusCode ret =
        writeXmlElement(ctx, UA_XML_EXTENSIONOBJECT_TYPEID,
                        &src->content.encoded.typeId, &UA_TYPES[UA_TYPES_NODEID]);

    /* Write the body */
    UA_Boolean prevPrintVal = ctx->printValOnly;
    ctx->printValOnly = false;
    ret |= writeXmlElemNameBegin(ctx, UA_XML_EXTENSIONOBJECT_BODY);
    if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING)
        ret |= writeXmlElemNameBegin(ctx, UA_XML_EXTENSIONOBJECT_BYTESTRING);

    ctx->printValOnly = true;
    ret |= ENCODE_DIRECT_XML(&src->content.encoded.body, String);
    ctx->printValOnly = false;

    if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING)
        ret |= writeXmlElemNameEnd(ctx, UA_XML_EXTENSIONOBJECT_BYTESTRING,
                                   &UA_TYPES[UA_TYPES_BYTESTRING]);
    ret |= writeXmlElemNameEnd(ctx, UA_XML_EXTENSIONOBJECT_BODY,
                               &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    ctx->printValOnly = prevPrintVal;

    return ret;
}

static status
Array_encodeXml(CtxXml *ctx, const void *ptr, size_t length,
                const UA_DataType *type) {
    char arrName[128];
    size_t arrNameLen = strlen("ListOf") + strlen(type->typeName);
    if(arrNameLen >= 128)
        return UA_STATUSCODE_BADENCODINGERROR;
    memcpy(arrName, "ListOf", strlen("ListOf"));
    memcpy(arrName + strlen("ListOf"), type->typeName, strlen(type->typeName));
    arrName[arrNameLen] = '\0';

    status ret = writeXmlElemNameBegin(ctx, arrName);

    if(!ptr)
        goto finish;

    uintptr_t uptr = (uintptr_t)ptr;
    for(size_t i = 0; i < length && ret == UA_STATUSCODE_GOOD; ++i) {
        if(isNullXml((const void*)uptr, type))
            ret |= xmlEncodeWriteChars(ctx, "null", 4);
        else
            ret |= writeXmlElement(ctx, type->typeName, (const void*)uptr, type);
        uptr += type->memSize;
    }

finish:
    ret |= writeXmlElemNameEnd(ctx, arrName, &UA_TYPES[UA_TYPES_VARIANT]);
    return ret;
}

ENCODE_XML(Variant) {
    if(!src->type)
        return UA_STATUSCODE_BADENCODINGERROR;

    /* Set the content type in the encoding mask */
    const UA_Boolean isBuiltin = (src->type->typeKind <= UA_DATATYPEKIND_DIAGNOSTICINFO);

    /* Set the array type in the encoding mask */
    const bool isArray = src->arrayLength > 0 || src->data <= UA_EMPTY_ARRAY_SENTINEL;

    if((!isArray && !isBuiltin) || src->arrayDimensionsSize > 1)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    status ret = writeXmlElemNameBegin(ctx, UA_XML_VARIANT_VALUE);

    ret |= Array_encodeXml(ctx, src->data, src->arrayLength, src->type);

    return ret | writeXmlElemNameEnd(ctx, UA_XML_VARIANT_VALUE, &UA_TYPES[UA_TYPES_VARIANT]);
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
    (encodeXmlSignature)ExtensionObject_encodeXml,  /* ExtensionObject */
    (encodeXmlSignature)encodeXmlNotImplemented,    /* DataValue */
    (encodeXmlSignature)Variant_encodeXml,          /* Variant */
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
    res = writeXmlElement(&ctx, type->typeName, src, type);

    /* Clean up */
    if(res == UA_STATUSCODE_GOOD)
        outBuf->length = (size_t)((uintptr_t)ctx.pos - (uintptr_t)outBuf->data);
    else if(allocated)
        UA_ByteString_clear(outBuf);

    return res;
}

UA_StatusCode
UA_printXml(const void *p, const UA_DataType *type, UA_String *output) {
    if(!p || !type || !output)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_EncodeXmlOptions options;
    memset(&options, 0, sizeof(UA_EncodeXmlOptions));
    options.prettyPrint = true;

    return UA_encodeXml(p, type, output, &options);
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
    status ret = writeXmlElement(&ctx, type->typeName, src, type);
    if(ret != UA_STATUSCODE_GOOD)
        return 0;
    return (size_t)ctx.pos;
}

/**********/
/* Decode */
/**********/

#define CHECK_DATA_BOUNDS                           \
    if(ctx->index >= ctx->tokensSize)               \
        return UA_STATUSCODE_BADDECODINGERROR;      \
    do { } while(0)

#define GET_ELEM_CONTENT                                     \
    const UA_Byte *data = ctx->tokens[ctx->index].content.data;    \
    size_t length = ctx->tokens[ctx->index].content.length;  \
    do {} while(0)

static void
skipXmlObject(ParseCtxXml *ctx) {
    size_t end_parent = ctx->tokens[ctx->index].end;
    do {
        ctx->index++;
    } while(ctx->tokens[ctx->index].end <= end_parent);
}

/* Forward declarations */
#define DECODE_XML(TYPE) static status                  \
    TYPE##_decodeXml(ParseCtxXml *ctx, UA_##TYPE *dst,  \
                      const UA_DataType *type)

DECODE_XML(Boolean) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;

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
decodeSigned(const UA_Byte *data, size_t dataSize, UA_Int64 *dst) {
    size_t len = parseInt64((const char*)data, dataSize, dst);
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
decodeUnsigned(const UA_Byte *data, size_t dataSize, UA_UInt64 *dst) {
    size_t len = parseUInt64((const char*)data, dataSize, dst);
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
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;

    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(data, length, &out);

    if(s != UA_STATUSCODE_GOOD || out < UA_SBYTE_MIN || out > UA_SBYTE_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_SByte)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Byte) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;

    UA_UInt64 out = 0;
    UA_StatusCode s = decodeUnsigned(data, length, &out);

    if(s != UA_STATUSCODE_GOOD || out > UA_BYTE_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Byte)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Int16) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;

    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(data, length, &out);

    if(s != UA_STATUSCODE_GOOD || out < UA_INT16_MIN || out > UA_INT16_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Int16)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(UInt16) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;

    UA_UInt64 out = 0;
    UA_StatusCode s = decodeUnsigned(data, length, &out);

    if(s != UA_STATUSCODE_GOOD || out > UA_UINT16_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_UInt16)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Int32) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;

    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(data, length, &out);

    if(s != UA_STATUSCODE_GOOD || out < UA_INT32_MIN || out > UA_INT32_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Int32)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(UInt32) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;

    UA_UInt64 out = 0;
    UA_StatusCode s = decodeUnsigned(data, length, &out);

    if(s != UA_STATUSCODE_GOOD || out > UA_UINT32_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_UInt32)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Int64) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;

    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(data, length, &out);

    if(s != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Int64)out;
    ctx->index++;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(UInt64) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;

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
    GET_ELEM_CONTENT;

    /* https://www.exploringbinary.com/maximum-number-of-decimal-digits-in-binary-floating-point-numbers/
     * Maximum digit counts for select IEEE floating-point formats: 1074
     * Sanity check. */
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

    size_t len = parseDouble((const char*)data, length, dst);
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
    GET_ELEM_CONTENT;

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
    GET_ELEM_CONTENT;
    UA_String str = {length, (UA_Byte*)(uintptr_t)data};
    return decodeDateTime(str, dst);
}

static status
decodeXmlFields(ParseCtxXml *ctx, XmlDecodeEntry *entries, size_t entryCount) {
    CHECK_DATA_BOUNDS;

    if(ctx->depth >= UA_XML_ENCODING_MAX_RECURSION)
        return UA_STATUSCODE_BADENCODINGERROR;

    size_t childCount = ctx->tokens[ctx->index].children;

    /* Empty object, nothing to decode */
    if(childCount == 0) {
        ctx->index++; /* Jump to the element after the empty object */
        return UA_STATUSCODE_GOOD;
    }

    ctx->depth++;
    ctx->index++; /* Go to first entry element */

    status ret = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < childCount; i++) {
        xml_token *elem = &ctx->tokens[ctx->index];
        XmlDecodeEntry *entry = NULL;
        for(size_t j = i; j < entryCount + i; j++) {
            /* Search for key, if found outer loop will be one less. Best case
             * if objectCount is in order! */
            size_t index = j % entryCount;
            if(!UA_String_equal(&elem->name, &entries[index].name))
                continue;
            entry = &entries[index];
            break;
        }

        /* Unknown child element */
        if(!entry)
            goto errout;

        /* An entry that was expected, but shall not be decoded.
         * Jump over it. */
        if(!entry->fieldPointer || (!entry->function && !entry->type)) {
            skipXmlObject(ctx);
            continue;
        }

        /* Duplicate child element */
        if(entry->found)
            goto errout;
        entry->found = true;

        /* Decode */
        if(entry->function) /* Specialized decoding function */
            ret = entry->function(ctx, entry->fieldPointer, entry->type);
        else /* Decode by type-kind */
            ret = decodeXmlJumpTable[entry->type->typeKind](ctx, entry->fieldPointer, entry->type);
        if(ret != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

cleanup:
    ctx->depth--;
    return ret;
errout:
    ctx->depth--;
    return UA_STATUSCODE_BADDECODINGERROR;
}

DECODE_XML(Guid) {
    CHECK_DATA_BOUNDS;
    UA_String str;
    UA_String_init(&str);
    XmlDecodeEntry entry = {UA_STRING_STATIC(UA_XML_GUID_STRING), &str,
                            NULL, false, &UA_TYPES[UA_TYPES_STRING]};
    status ret = decodeXmlFields(ctx, &entry, 1);
    ret |= UA_Guid_parse(dst, str);
    UA_String_clear(&str);
    return ret;
}

DECODE_XML(ByteString) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;

    /* Empty bytestring? */
    if(length == 0) {
        dst->data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
        dst->length = 0;
    } else {
        size_t flen = 0;
        unsigned char* unB64 = UA_unbase64((const unsigned char*)data, length, &flen);
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
    XmlDecodeEntry entry = {UA_STRING_STATIC(UA_XML_NODEID_IDENTIFIER), &str,
                            NULL, false, &UA_TYPES[UA_TYPES_STRING]};

    status ret = decodeXmlFields(ctx, &entry, 1);
    ret |= UA_NodeId_parse(dst, str);

    UA_String_clear(&str);
    return ret;
}

DECODE_XML(ExpandedNodeId) {
    CHECK_DATA_BOUNDS;

    UA_String str;
    UA_String_init(&str);
    XmlDecodeEntry entry = {UA_STRING_STATIC(UA_XML_EXPANDEDNODEID_IDENTIFIER), &str,
                            NULL, false, &UA_TYPES[UA_TYPES_STRING]};

    status ret = decodeXmlFields(ctx, &entry, 1);
    ret |= UA_ExpandedNodeId_parse(dst, str);

    UA_String_clear(&str);
    return ret;
}

DECODE_XML(StatusCode) {
    CHECK_DATA_BOUNDS;

    XmlDecodeEntry entry = {UA_STRING_STATIC(UA_XML_STATUSCODE_CODE), dst,
                            NULL, false, &UA_TYPES[UA_TYPES_UINT32]};

    return decodeXmlFields(ctx, &entry, 1);
}

DECODE_XML(QualifiedName) {
    CHECK_DATA_BOUNDS;

    XmlDecodeEntry entries[2] = {
        {UA_STRING_STATIC(UA_XML_QUALIFIEDNAME_NAMESPACEINDEX), &dst->namespaceIndex,
         NULL, false, &UA_TYPES[UA_TYPES_UINT16]},
        {UA_STRING_STATIC(UA_XML_QUALIFIEDNAME_NAME), &dst->name,
         NULL, false, &UA_TYPES[UA_TYPES_STRING]}
    };

    return decodeXmlFields(ctx, entries, 2);
}

DECODE_XML(LocalizedText) {
    CHECK_DATA_BOUNDS;

    XmlDecodeEntry entries[2] = {
        {UA_STRING_STATIC(UA_XML_LOCALIZEDTEXT_LOCALE), &dst->locale,
         NULL, false, &UA_TYPES[UA_TYPES_STRING]},
        {UA_STRING_STATIC(UA_XML_LOCALIZEDTEXT_TEXT), &dst->text,
         NULL, false, &UA_TYPES[UA_TYPES_STRING]}
    };

    return decodeXmlFields(ctx, entries, 2);
}

static UA_StatusCode
decodeExtensionObjectBody(ParseCtxXml *ctx, void *dst, const UA_DataType *type) {
    UA_ExtensionObject *eo = (UA_ExtensionObject*)dst;

    /* Does the body have a single "bytestring" member? */
    xml_token *tok = &ctx->tokens[ctx->index];

    if(tok->attributes > 0 || tok->children != 1)
        return UA_STATUSCODE_BADDECODINGERROR;

    ctx->index++;
    tok = &ctx->tokens[ctx->index];

    UA_String bs = UA_STRING_STATIC(UA_XML_EXTENSIONOBJECT_BYTESTRING);
    if(UA_String_equal(&tok->name, &bs)) {
        eo->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
        return decodeXmlJumpTable[UA_DATATYPEKIND_BYTESTRING]
            (ctx, &eo->content.encoded.body, NULL);
    }

    eo->encoding = UA_EXTENSIONOBJECT_ENCODED_XML;
    UA_String body = {tok->end - tok->start, (UA_Byte*)(uintptr_t)ctx->xml + tok->start};
    return UA_String_copy(&body, &eo->content.encoded.body);
}

DECODE_XML(ExtensionObject) {
    CHECK_DATA_BOUNDS;
    XmlDecodeEntry entries[2] = {
        {UA_STRING_STATIC(UA_XML_EXTENSIONOBJECT_TYPEID), &dst->content.encoded.typeId,
         NULL, false, &UA_TYPES[UA_TYPES_NODEID]},
        {UA_STRING_STATIC(UA_XML_EXTENSIONOBJECT_BODY), dst,
         decodeExtensionObjectBody, false, NULL},
    };
    return decodeXmlFields(ctx, entries, 2);
}

/* static status */
/* Array_decodeXml(ParseCtxXml *ctx, void **dst, const UA_DataType *type) { */
/*     if(strncmp(ctx->tokens[ctx->index]->name, "ListOf", strlen("ListOf"))) */
/*         return UA_STATUSCODE_BADDECODINGERROR; */

/*     if(ctx->dataMembers[ctx->index]->type == XML_DATA_TYPE_PRIMITIVE) { */
/*         /\* Return early for empty arrays *\/ */
/*         *dst = UA_EMPTY_ARRAY_SENTINEL; */
/*         return UA_STATUSCODE_GOOD; */
/*     } */

/*     size_t length = ctx->dataMembers[ctx->index]->value.complex.membersSize; */
/*     ctx->index++; /\* Go to first array member. *\/ */

/*     /\* Allocate memory *\/ */
/*     *dst = UA_calloc(length, type->memSize); */
/*     if(*dst == NULL) */
/*         return UA_STATUSCODE_BADOUTOFMEMORY; */

/*     /\* Decode array members *\/ */
/*     uintptr_t ptr = (uintptr_t)*dst; */
/*     for(size_t i = 0; i < length; ++i) { */
/*         status ret = decodeXmlJumpTable[type->typeKind](ctx, (void*)ptr, type); */
/*         if(ret != UA_STATUSCODE_GOOD) { */
/*             UA_Array_delete(*dst, i + 1, type); */
/*             *dst = NULL; */
/*             return ret; */
/*         } */
/*         ptr += type->memSize; */
/*     } */

/*     return UA_STATUSCODE_GOOD; */
/* } */

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
    (decodeXmlSignature)ExtensionObject_decodeXml,  /* ExtensionObject */
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

    /* Tokenize */
    xml_token tokenbuf[64];
    xml_token *tokens = tokenbuf;
    xml_result res = xml_tokenize((char*)src->data, src->length, tokens, 64);
    if(res.error == XML_ERROR_OVERFLOW) {
        tokens = (xml_token*)UA_malloc(sizeof(xml_token) * res.num_tokens);
        if(!tokens)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        res = xml_tokenize((char*)src->data, src->length, tokens, res.num_tokens);
    }

    if(res.error != XML_ERROR_NONE) {
        if(tokens != tokenbuf)
            UA_free(tokens);
        return UA_STATUSCODE_BADDECODINGERROR;
    }

    /* Set up the context */
    ParseCtxXml ctx;
    memset(&ctx, 0, sizeof(ParseCtxXml));
    ctx.xml = (const char*)src->data;
    ctx.tokens = tokens;
    ctx.tokensSize = res.num_tokens;
    if(options)
        ctx.customTypes = options->customTypes;

    /* Decode */
    UA_StatusCode ret = decodeXmlJumpTable[type->typeKind](&ctx, dst, type);
    if(ret != UA_STATUSCODE_GOOD)
        UA_clear(dst, type);

    /* Clean up */
    if(tokens != tokenbuf)
        UA_free(tokens);
    return ret;
}
