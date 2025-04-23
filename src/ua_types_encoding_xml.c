/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/config.h>
#include <open62541/types.h>

#include "ua_types_encoding_xml.h"

#include <float.h>
#include <math.h>

#include "../deps/itoa.h"
#include "../deps/parse_num.h"
#include "../deps/base64.h"
#include "../deps/libc_time.h"
#include "../deps/dtoa.h"
#include "../deps/yxml.h"

#ifndef UA_ENABLE_PARSING
#error UA_ENABLE_PARSING required for XML encoding
#endif

#ifndef UA_ENABLE_TYPEDESCRIPTION
#error UA_ENABLE_TYPEDESCRIPTION required for XML encoding
#endif

/* Replicate yxml_isNameStart and yxml_isName from yxml. But differently we
 * already break at the first colon, so "uax:String" becomes "String". */
static UA_String
backtrackName(const char *xml, unsigned end) {
    unsigned pos = end;
    for(; pos > 0; pos--) {
        unsigned char c = (unsigned char)xml[pos-1];
        if(c >= 'a' && c <= 'z') continue; /* isAlpha */
        if(c >= 'A' && c <= 'Z') continue; /* isAlpha */
        if(c >= '0' && c <= '9') continue; /* isNum */
        if(c == '_' || c >= 128 || c == '-'|| c == '.') continue;
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
            if(status == YXML_ELEMSTART) {
                stack[top]->children++;
                stack[top]->content = UA_STRING_NULL; /* Only the leaf elements have content */
            } else {
                stack[top]->attributes++;
            }
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
            val_begin = 0; /* if the previous non-leaf element started to collect content */
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
                /* Saw "</", looking for the closing ">" */
                while(stack[top]->end < len && xml[stack[top]->end] != '>')
                    stack[top]->end++;
                stack[top]->end++;
                if(stack[top]->end > len)
                    goto errout;
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

    /* Check that all elements were closed */
    if(yxml_eof(&ctx) != YXML_OK)
        goto errout;

    res.num_tokens = tokenPos;
    if(tokenPos > max_tokens)
        res.error = XML_ERROR_OVERFLOW;
    return res;

 errout:
    res.error_pos = pos;
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
    ret |= xmlEncodeWriteChars(ctx, "<", 1);
    ret |= xmlEncodeWriteChars(ctx, name, strlen(name));
    ret |= xmlEncodeWriteChars(ctx, ">", 1);
    ctx->depth++;
    return ret;
}

static status UA_FUNC_ATTR_WARN_UNUSED_RESULT
writeXmlElemNameEnd(CtxXml *ctx, const char* name) {
    if(ctx->depth == 0)
        return UA_STATUSCODE_BADENCODINGERROR;
    ctx->depth--;
    status ret = UA_STATUSCODE_GOOD;
    ret |= xmlEncodeWriteChars(ctx, "</", 2);
    ret |= xmlEncodeWriteChars(ctx, name, strlen(name));
    ret |= xmlEncodeWriteChars(ctx, ">", 1);
    return ret;
}

static status UA_FUNC_ATTR_WARN_UNUSED_RESULT
writeXmlElement(CtxXml *ctx, const char *name,
                const void *value, const UA_DataType *type) {
    status ret = UA_STATUSCODE_GOOD;
    ret |= writeXmlElemNameBegin(ctx, name);
    ret |= encodeXmlJumpTable[type->typeKind](ctx, value, type);
    ret |= writeXmlElemNameEnd(ctx, name);
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
    ret |= UA_NodeId_printEx(src, &out, ctx->namespaceMapping);
    ret |= writeXmlElement(ctx, UA_XML_NODEID_IDENTIFIER,
                           &out, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&out);
    return ret;
}

/* ExpandedNodeId */
ENCODE_XML(ExpandedNodeId) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_String out = UA_STRING_NULL;
    ret |= UA_ExpandedNodeId_printEx(src, &out,
                                     ctx->namespaceMapping,
                                     ctx->serverUrisSize,
                                     ctx->serverUris);
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
    /* Map the NamespaceIndex */
    UA_UInt16 index = src->namespaceIndex;
    if(ctx->namespaceMapping)
        index = UA_NamespaceMapping_local2Remote(ctx->namespaceMapping, index);

    /* Write out the elements */
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    ret |= writeXmlElement(ctx, UA_XML_QUALIFIEDNAME_NAMESPACEINDEX,
                           &index, &UA_TYPES[UA_TYPES_UINT16]);
    ret |= writeXmlElement(ctx, UA_XML_QUALIFIEDNAME_NAME,
                           &src->name, &UA_TYPES[UA_TYPES_STRING]);
    return ret;
}

/* LocalizedText */
ENCODE_XML(LocalizedText) {
    return writeXmlElement(ctx, UA_XML_LOCALIZEDTEXT_LOCALE,
                           &src->locale, &UA_TYPES[UA_TYPES_STRING]) |
        writeXmlElement(ctx, UA_XML_LOCALIZEDTEXT_TEXT,
                        &src->text, &UA_TYPES[UA_TYPES_STRING]);
}

/* ExtensionObject */
ENCODE_XML(ExtensionObject) {
    if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY)
        return UA_STATUSCODE_GOOD;

    /* The body of the ExtensionObject contains a single element
     * which is either a ByteString or XML encoded Structure:
     * https://reference.opcfoundation.org/Core/Part6/v104/docs/5.3.1.16. */
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING ||
       src->encoding == UA_EXTENSIONOBJECT_ENCODED_XML) {
        /* Write the type NodeId */
        ret = writeXmlElement(ctx, UA_XML_EXTENSIONOBJECT_TYPEID,
                              &src->content.encoded.typeId,
                              &UA_TYPES[UA_TYPES_NODEID]);

        /* Write the body */
        ret |= writeXmlElemNameBegin(ctx, UA_XML_EXTENSIONOBJECT_BODY);
        if(src->encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING)
           ret |= writeXmlElement(ctx, "ByteString", &src->content.encoded.body,
                                  &UA_TYPES[UA_TYPES_BYTESTRING]);
        else
            ret |= ENCODE_DIRECT_XML(&src->content.encoded.body, String);
        ret |= writeXmlElemNameEnd(ctx, UA_XML_EXTENSIONOBJECT_BODY);
    } else {
        /* Write the decoded value */
        const UA_DataType *type = src->content.decoded.type;

        /* Write the type NodeId */
        ret = writeXmlElement(ctx, UA_XML_EXTENSIONOBJECT_TYPEID,
                              &type->typeId, &UA_TYPES[UA_TYPES_NODEID]);

        /* Write the body */
        ret |= writeXmlElemNameBegin(ctx, UA_XML_EXTENSIONOBJECT_BODY);
        ret |= writeXmlElement(ctx, type->typeName, src->content.decoded.data, type);
        ret |= writeXmlElemNameEnd(ctx, UA_XML_EXTENSIONOBJECT_BODY);
    }

    return ret;
}

static status
Array_encodeXml(CtxXml *ctx, const void *ptr, size_t length,
                const UA_DataType *type) {
    char arrName[128];
    UA_ExtensionObject eo;

    UA_Boolean wrapEO = (type->typeKind > UA_DATATYPEKIND_DIAGNOSTICINFO);
    if(wrapEO) {
        UA_ExtensionObject_setValue(&eo, (void*)(uintptr_t)ptr, type);
        type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
    }

    size_t arrNameLen = strlen("ListOf") + strlen(type->typeName);
    if(arrNameLen >= 128)
        return UA_STATUSCODE_BADENCODINGERROR;
    memcpy(arrName, "ListOf", strlen("ListOf"));
    memcpy(arrName + strlen("ListOf"), type->typeName, strlen(type->typeName));
    arrName[arrNameLen] = '\0';

    uintptr_t uptr = (uintptr_t)ptr;
    status ret = writeXmlElemNameBegin(ctx, arrName);
    for(size_t i = 0; i < length && ret == UA_STATUSCODE_GOOD; ++i) {
        if(!wrapEO) {
            ret |= writeXmlElement(ctx, type->typeName, (const void*)uptr, type);
        } else {
            eo.content.decoded.data = (void*)uptr;
            ret |= writeXmlElement(ctx, type->typeName, &eo, type);
        }
        uptr += type->memSize;
    }
    ret |= writeXmlElemNameEnd(ctx, arrName);
    return ret;
}

ENCODE_XML(Variant) {
    if(!src->type)
        return UA_STATUSCODE_BADENCODINGERROR;

    /* Set the array type in the encoding mask */
    const bool isArray = src->arrayLength > 0 || src->data <= UA_EMPTY_ARRAY_SENTINEL;

    if(src->arrayDimensionsSize > 1)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    ret |= writeXmlElemNameBegin(ctx, UA_XML_VARIANT_VALUE);
    if(!isArray) {
        const UA_DataType *type = src->type;
        void *ptr = src->data;
        UA_ExtensionObject eo;
        if(type->typeKind > UA_DATATYPEKIND_DIAGNOSTICINFO) {
            /* Wrap value in an ExtensionObject */
            UA_ExtensionObject_setValue(&eo, (void*)(uintptr_t)ptr, type);
            ptr = &eo;
            type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
        }
        ret |= writeXmlElement(ctx, type->typeName, ptr, type);
    } else {
        ret |= Array_encodeXml(ctx, src->data, src->arrayLength, src->type);
    }
    ret |= writeXmlElemNameEnd(ctx, UA_XML_VARIANT_VALUE);
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
    if(options) {
        ctx.namespaceMapping = options->namespaceMapping;
        ctx.serverUris = options->serverUris;
        ctx.serverUrisSize = options->serverUrisSize;
    }

    /* Encode */
    res = writeXmlElement(&ctx, type->typeName, src, type);

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
        ctx.namespaceMapping = options->namespaceMapping;
        ctx.serverUris = options->serverUris;
        ctx.serverUrisSize = options->serverUrisSize;
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

#define DECODE_XML(TYPE) static status                  \
    TYPE##_decodeXml(ParseCtxXml *ctx, UA_##TYPE *dst, const UA_DataType *type)

#define CHECK_DATA_BOUNDS                           \
    if(ctx->index >= ctx->tokensSize)               \
        return UA_STATUSCODE_BADDECODINGERROR;      \
    do { } while(0)

#define GET_ELEM_CONTENT                                           \
    const UA_Byte *data = ctx->tokens[ctx->index].content.data;    \
    size_t length = ctx->tokens[ctx->index].content.length;        \
    do {} while(0)

static void
skipXmlObject(ParseCtxXml *ctx) {
    size_t end_parent = ctx->tokens[ctx->index].end;
    while(ctx->index < ctx->tokensSize &&
          ctx->tokens[ctx->index].end <= end_parent) {
        ctx->index++;
    }
}

DECODE_XML(Boolean) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;
    skipXmlObject(ctx);

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
    skipXmlObject(ctx);

    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(data, length, &out);
    if(s != UA_STATUSCODE_GOOD || out < UA_SBYTE_MIN || out > UA_SBYTE_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_SByte)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Byte) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;
    skipXmlObject(ctx);

    UA_UInt64 out = 0;
    UA_StatusCode s = decodeUnsigned(data, length, &out);
    if(s != UA_STATUSCODE_GOOD || out > UA_BYTE_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Byte)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Int16) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;
    skipXmlObject(ctx);

    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(data, length, &out);
    if(s != UA_STATUSCODE_GOOD || out < UA_INT16_MIN || out > UA_INT16_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Int16)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(UInt16) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;
    skipXmlObject(ctx);

    UA_UInt64 out = 0;
    UA_StatusCode s = decodeUnsigned(data, length, &out);
    if(s != UA_STATUSCODE_GOOD || out > UA_UINT16_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_UInt16)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Int32) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;
    skipXmlObject(ctx);

    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(data, length, &out);
    if(s != UA_STATUSCODE_GOOD || out < UA_INT32_MIN || out > UA_INT32_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Int32)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(UInt32) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;
    skipXmlObject(ctx);

    UA_UInt64 out = 0;
    UA_StatusCode s = decodeUnsigned(data, length, &out);
    if(s != UA_STATUSCODE_GOOD || out > UA_UINT32_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_UInt32)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Int64) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;
    skipXmlObject(ctx);

    UA_Int64 out = 0;
    UA_StatusCode s = decodeSigned(data, length, &out);
    if(s != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_Int64)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(UInt64) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;
    skipXmlObject(ctx);

    UA_UInt64 out = 0;
    UA_StatusCode s = decodeUnsigned(data, length, &out);
    if(s != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = (UA_UInt64)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(Double) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;
    skipXmlObject(ctx);

    /* https://www.exploringbinary.com/maximum-number-of-decimal-digits-in-binary-floating-point-numbers/
     * Maximum digit counts for select IEEE floating-point formats: 1074
     * Sanity check. */
    if(length > 1075)
        return UA_STATUSCODE_BADDECODINGERROR;

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

    if(length == 3 && memcmp(data, "-NaN", 3) == 0) {
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
    skipXmlObject(ctx);

    /* Empty string? */
    if(length == 0) {
        dst->data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
        dst->length = 0;
        return UA_STATUSCODE_GOOD;
    }

    UA_String str = {length, (UA_Byte*)(uintptr_t)data};
    return UA_String_copy(&str, dst);
}

DECODE_XML(DateTime) {
    CHECK_DATA_BOUNDS;
    GET_ELEM_CONTENT;
    skipXmlObject(ctx);
    UA_String str = {length, (UA_Byte*)(uintptr_t)data};
    return decodeDateTime(str, dst);
}

/* Find the child with the given name and return its content.
 * This does not allocate/copy the content again. */
static status
getChildContent(ParseCtxXml *ctx, UA_String name, UA_String *out) {
    size_t oldIndex = ctx->index;
    size_t children = ctx->tokens[ctx->index].children;

    /* Skip the attributes and go to the first child */
    ctx->index += 1 + ctx->tokens[ctx->index].attributes;

    /* Find the child of the name */
    UA_StatusCode res = UA_STATUSCODE_BADDECODINGERROR;
    for(size_t i = 0; i < children; i++) {
        if(!UA_String_equal(&name, &ctx->tokens[ctx->index].name)) {
            skipXmlObject(ctx);
            continue;
        }
        *out = ctx->tokens[ctx->index].content;
        res = UA_STATUSCODE_GOOD;
        break;
    }

    ctx->index = oldIndex;
    return res;
}

static status
decodeXmlFields(ParseCtxXml *ctx, XmlDecodeEntry *entries, size_t entryCount) {
    CHECK_DATA_BOUNDS;

    if(ctx->depth >= UA_XML_ENCODING_MAX_RECURSION)
        return UA_STATUSCODE_BADENCODINGERROR;

    size_t childCount = ctx->tokens[ctx->index].children;

    /* Empty object */
    if(childCount == 0) {
        skipXmlObject(ctx);
        return UA_STATUSCODE_GOOD;
    }

    /* Go to first entry element */
    ctx->depth++;
    ctx->index += 1 + ctx->tokens[ctx->index].attributes;

    status ret = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < childCount; i++) {
        xml_token *elem = &ctx->tokens[ctx->index];
        XmlDecodeEntry *entry = NULL;
        for(size_t j = i; j < entryCount + i; j++) {
            /* Search for key, if found outer loop will be one less. Best case
             * if objectCount is in order! */
            size_t index = j % entryCount;
            if(!UA_String_equal_ignorecase(&elem->name, &entries[index].name))
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
    skipXmlObject(ctx);

    /* Empty bytestring? */
    if(length == 0) {
        dst->data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
        dst->length = 0;
    } else {
        size_t flen = 0;
        unsigned char* unB64 = UA_unbase64((const unsigned char*)data, length, &flen);
        if(!unB64)
            return UA_STATUSCODE_BADDECODINGERROR;
        dst->data = (UA_Byte*)unB64;
        dst->length = flen;
    }

    return UA_STATUSCODE_GOOD;
}

DECODE_XML(NodeId) {
    CHECK_DATA_BOUNDS;
    UA_String str;
    static UA_String identifier = UA_STRING_STATIC(UA_XML_NODEID_IDENTIFIER);
    status ret = getChildContent(ctx, identifier, &str);
    if(ret != UA_STATUSCODE_GOOD) return ret;
    skipXmlObject(ctx);
    return UA_NodeId_parseEx(dst, str, ctx->namespaceMapping);
}

DECODE_XML(ExpandedNodeId) {
    CHECK_DATA_BOUNDS;
    UA_String str;
    static UA_String expidentifier = UA_STRING_STATIC(UA_XML_EXPANDEDNODEID_IDENTIFIER);
    status ret = getChildContent(ctx, expidentifier, &str);
    if(ret != UA_STATUSCODE_GOOD) return ret;
    skipXmlObject(ctx);
    return UA_ExpandedNodeId_parseEx(dst, str, ctx->namespaceMapping,
                                     ctx->serverUrisSize, ctx->serverUris);
}

DECODE_XML(StatusCode) {
    CHECK_DATA_BOUNDS;
    UA_String str;
    static UA_String statusidentifier = UA_STRING_STATIC(UA_XML_STATUSCODE_CODE);
    status ret = getChildContent(ctx, statusidentifier, &str);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;
    skipXmlObject(ctx);
    UA_UInt64 out = 0;
    ret = decodeUnsigned(str.data, str.length, &out);
    if(ret != UA_STATUSCODE_GOOD || out > UA_UINT32_MAX)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (UA_StatusCode)out;
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(QualifiedName) {
    CHECK_DATA_BOUNDS;

    /* Decode the elements */
    XmlDecodeEntry entries[2] = {
        {UA_STRING_STATIC(UA_XML_QUALIFIEDNAME_NAMESPACEINDEX), &dst->namespaceIndex,
         NULL, false, &UA_TYPES[UA_TYPES_UINT16]},
        {UA_STRING_STATIC(UA_XML_QUALIFIEDNAME_NAME), &dst->name,
         NULL, false, &UA_TYPES[UA_TYPES_STRING]}
    };
    UA_StatusCode res = decodeXmlFields(ctx, entries, 2);

    /* Map the NamespaceIndex */
    if(ctx->namespaceMapping)
        dst->namespaceIndex =
            UA_NamespaceMapping_remote2Local(ctx->namespaceMapping, dst->namespaceIndex);

    return res;
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

/* Compare both typeId and xmlEncodingId */
static const UA_DataType *
lookupXmlType(ParseCtxXml *ctx, UA_NodeId *typeId) {
    /* Search in the builtin types */
    for(size_t i = 0; i < UA_TYPES_COUNT; ++i) {
        if(UA_NodeId_equal(typeId, &UA_TYPES[i].typeId) ||
           UA_NodeId_equal(typeId, &UA_TYPES[i].xmlEncodingId))
            return &UA_TYPES[i];
    }

    /* Search in the customTypes */
    const UA_DataTypeArray *customTypes = ctx->customTypes;
    while(customTypes) {
        for(size_t i = 0; i < customTypes->typesSize; ++i) {
            const UA_DataType *type = &customTypes->types[i];
            if(UA_NodeId_equal(typeId, &type->typeId) ||
               UA_NodeId_equal(typeId, &type->xmlEncodingId))
                return type;
        }
        customTypes = customTypes->next;
    }
    return NULL;
}

static UA_StatusCode
decodeExtensionObjectBody(ParseCtxXml *ctx, void *dst, const UA_DataType *type) {
    UA_ExtensionObject *eo = (UA_ExtensionObject*)dst;

    xml_token *tok = &ctx->tokens[ctx->index];
    if(tok->children != 1)
        return UA_STATUSCODE_BADDECODINGERROR; /* Only one child allowed */

    if(UA_NodeId_isNull(&eo->content.encoded.typeId))
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Find the datatype of the body */
    type = lookupXmlType(ctx, &eo->content.encoded.typeId);

    /* Allocate decoded content */
    void *decoded = NULL;
    if(type) {
        decoded = UA_new(type);
        if(!decoded)
            return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Jump to the first child element */
    ctx->index += 1 + tok->attributes;
    tok = &ctx->tokens[ctx->index];

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_String bs = UA_STRING_STATIC(UA_XML_EXTENSIONOBJECT_BYTESTRING);
    if(UA_String_equal(&tok->name, &bs)) {
        /* Decode binary ByteString Body */
        eo->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
        ret = decodeXmlJumpTable[UA_DATATYPEKIND_BYTESTRING](ctx, &eo->content.encoded.body, NULL);
        if(!type)
            return ret;
        UA_DecodeBinaryOptions opts;
        memset(&opts, 0, sizeof(UA_DecodeBinaryOptions));
        ret = UA_decodeBinary(&eo->content.encoded.body, decoded, type, &opts);
    } else {
        /* Decode XML Body */
        eo->encoding = UA_EXTENSIONOBJECT_ENCODED_XML;
        UA_String body = {tok->end - tok->start, (UA_Byte*)(uintptr_t)ctx->xml + tok->start};
        skipXmlObject(ctx); /* Skip over the body */
        if(!type)
            return UA_String_copy(&body, &eo->content.encoded.body);
        UA_DecodeXmlOptions opts;
        memset(&opts, 0, sizeof(UA_DecodeXmlOptions));
        opts.namespaceMapping = ctx->namespaceMapping;
        opts.serverUris = ctx->serverUris;
        opts.serverUrisSize = ctx->serverUrisSize;
        opts.customTypes = ctx->customTypes;
        ret = UA_decodeXml(&body, decoded, type, &opts);
    }

    if(ret != UA_STATUSCODE_GOOD) {
        UA_free(decoded); /* Return the un-decoded content if decoding fails */
        return UA_STATUSCODE_GOOD;
    }

    UA_ExtensionObject_clear(eo); /* Also clears the already decoded TypeId */
    UA_ExtensionObject_setValue(eo, decoded, type);
    return UA_STATUSCODE_GOOD;
}

DECODE_XML(ExtensionObject) {
    CHECK_DATA_BOUNDS;
    xml_token *tok = &ctx->tokens[ctx->index];
    if(tok->children == 0)
        return UA_STATUSCODE_GOOD; /* _NO_BODY */
    dst->encoding = UA_EXTENSIONOBJECT_ENCODED_XML; /* default so the typeId gets cleaned up */
    XmlDecodeEntry entries[2] = {
        {UA_STRING_STATIC(UA_XML_EXTENSIONOBJECT_TYPEID), &dst->content.encoded.typeId,
         NULL, false, &UA_TYPES[UA_TYPES_NODEID]},
        {UA_STRING_STATIC(UA_XML_EXTENSIONOBJECT_BODY), dst,
         decodeExtensionObjectBody, false, NULL},
    };
    return decodeXmlFields(ctx, entries, 2);
}

static status
Array_decodeXml(ParseCtxXml *ctx, size_t *dstSize, const UA_DataType *type) {
    /* Allocate memory */
    size_t length = ctx->tokens[ctx->index].children;
    void **dst = (void**)((uintptr_t)dstSize + sizeof(void*));
    *dst = UA_Array_new(length, type);
    if(!*dst)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *dstSize = length;

    /* Go to first array member. */
    ctx->index += 1 + ctx->tokens[ctx->index].attributes;

    /* Decode array members */
    uintptr_t ptr = (uintptr_t)*dst;
    for(size_t i = 0; i < length; ++i) {
        status ret = decodeXmlJumpTable[type->typeKind](ctx, (void*)ptr, type);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
        ptr += type->memSize;
    }

    return UA_STATUSCODE_GOOD;
}

static const UA_DataType *
lookupTypeByName(ParseCtxXml *ctx, UA_String typeName) {
    /* Search in the builtin types */
    for(size_t i = 0; i < UA_TYPES_COUNT; ++i) {
        if(strncmp((char*)typeName.data, UA_TYPES[i].typeName, typeName.length) == 0)
            return &UA_TYPES[i];
    }

    /* Search in the customTypes */
    const UA_DataTypeArray *customTypes = ctx->customTypes;
    while(customTypes) {
        for(size_t i = 0; i < customTypes->typesSize; ++i) {
            const UA_DataType *type = &customTypes->types[i];
            if(strncmp((char*)typeName.data, type->typeName, typeName.length) == 0)
                return type;
        }
        customTypes = customTypes->next;
    }
    return NULL;
}

static void
unwrapVariantExtensionObject(UA_Variant *dst, UA_Boolean isArray) {
    if(dst->type != &UA_TYPES[UA_TYPES_EXTENSIONOBJECT])
        return;
    if(isArray && dst->arrayLength == 0)
        return;

    UA_ExtensionObject *eo = (UA_ExtensionObject*)dst->data;
    if(eo->encoding != UA_EXTENSIONOBJECT_DECODED)
        return;

    const UA_DataType *type = eo->content.decoded.type;

    /* Scalar */
    if(!isArray) {
        dst->data = eo->content.decoded.data;
        dst->type = type;
        UA_free(eo);
        return;
    }

    /* Array. Check that all members can be unpacked */
    for(size_t i = 0; i < dst->arrayLength; i++, eo++) {
        if(eo->encoding != UA_EXTENSIONOBJECT_DECODED)
            return;
        if(eo->content.decoded.type != type)
            return;
    }

    /* Allocate the array */
    void *unpacked = UA_calloc(dst->arrayLength, type->memSize);
    if(!unpacked)
        return;

    /* Unpack the content and set the new array */
    uintptr_t uptr = (uintptr_t)unpacked;
    eo = (UA_ExtensionObject*)dst->data;
    for(size_t i = 0; i < dst->arrayLength; i++, eo++) {
        /* Move the value content */
        memcpy((void*)uptr, eo->content.decoded.data, type->memSize);
        UA_free(eo->content.decoded.data); /* Free the old value location */
        uptr += type->memSize;
    }
    UA_free(dst->data); /* Remove the old array of ExtensionObjects */
    dst->data = unpacked;
    dst->type = type;
}

static UA_StatusCode
decodeMatrixVariant(ParseCtxXml *ctx, UA_Variant *dst) {
    /* The <Matrix> token needs two children: <Dimensions> and <Elements> */
    xml_token *tok = &ctx->tokens[ctx->index];
    if(tok->children != 2)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Jump to the child of the <Matrix> token */
    ctx->index += 1 + ctx->tokens[ctx->index].attributes;
    tok = &ctx->tokens[ctx->index];

    UA_assert(tok->type == XML_TOKEN_ELEMENT);
    static UA_String dimName = UA_STRING_STATIC("Dimensions");
    if(!UA_String_equal(&tok->name, &dimName))
        return UA_STATUSCODE_BADDECODINGERROR;

    UA_StatusCode ret =
        Array_decodeXml(ctx, &dst->arrayDimensionsSize, &UA_TYPES[UA_TYPES_INT32]);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    UA_assert(tok->type == XML_TOKEN_ELEMENT);
    static UA_String elemName = UA_STRING_STATIC("Elements");
    tok = &ctx->tokens[ctx->index];
    if(!UA_String_equal(&tok->name, &elemName))
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Get the type of the first element */
    size_t oldIndex = ctx->index;
    ctx->index += 1 + ctx->tokens[ctx->index].attributes;
    tok = &ctx->tokens[ctx->index];
    UA_assert(tok->type == XML_TOKEN_ELEMENT);

    UA_String typeName = tok->name;
    dst->type = lookupTypeByName(ctx, typeName);
    if(!dst->type)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Decode the array */
    ctx->index = oldIndex;
    ret = Array_decodeXml(ctx, &dst->arrayLength, dst->type);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Check that the ArrayDimensions match */
    size_t dimLen = 1;
    for(size_t i = 0; i < dst->arrayDimensionsSize; i++)
        dimLen *= dst->arrayDimensions[i];

    return (dimLen == dst->arrayLength) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADDECODINGERROR;
}

DECODE_XML(Variant) {
    CHECK_DATA_BOUNDS;

    if(ctx->depth >= UA_XML_ENCODING_MAX_RECURSION)
        return UA_STATUSCODE_BADENCODINGERROR;

    xml_token *tok = &ctx->tokens[ctx->index];
    if(tok->children == 0)
        return UA_STATUSCODE_GOOD;
    if(tok->children != 1)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Move forward to the content */
    if(ctx->index + 2 >= ctx->tokensSize)
        return UA_STATUSCODE_BADDECODINGERROR;

    ctx->index += 1 + ctx->tokens[ctx->index].attributes;
    tok = &ctx->tokens[ctx->index];
    static UA_String valName = UA_STRING_STATIC("Value");
    if(!UA_String_equal(&tok->name, &valName))
        return UA_STATUSCODE_BADDECODINGERROR;
    if(tok->children != 1)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Jump to the child of the <Value> token */
    ctx->depth++;
    ctx->index += 1 + ctx->tokens[ctx->index].attributes;
    tok = &ctx->tokens[ctx->index];

    /* Special case for multi-dimensional arrays */
    static UA_String matrName = UA_STRING_STATIC("Matrix");
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if(UA_String_equal(&tok->name, &matrName)) {
        ret = decodeMatrixVariant(ctx, dst);
        unwrapVariantExtensionObject(dst, true);
        ctx->depth--;
        return ret;
    }

    /* Get the Data type / array type */
    UA_Boolean isArray = false;
    static char *lo = "ListOf";
    UA_String typeName = tok->name;
    if(tok->name.length > strlen(lo) &&
       strncmp((char*)tok->name.data, lo, strlen(lo)) == 0) {
        isArray = true;
        typeName.data += strlen(lo);
        typeName.length -= strlen(lo);
    }

    /* Look up the DataType from the name */
    dst->type = lookupTypeByName(ctx, typeName);
    if(!dst->type) {
        ctx->depth--;
        return UA_STATUSCODE_BADDECODINGERROR;
    }

    /* Decode */
    if(!isArray) {
        dst->data = UA_new(dst->type);
        if(!dst->data) {
            ctx->depth--;
            return UA_STATUSCODE_BADDECODINGERROR;
        }
        ret = decodeXmlJumpTable[dst->type->typeKind](ctx, dst->data, dst->type);
    } else {
        ret = Array_decodeXml(ctx, &dst->arrayLength, dst->type);
    }

    /* Unwrap ExtensionObject values in the variant */
    unwrapVariantExtensionObject(dst, isArray);

    ctx->depth--;
    return ret;
}

static status
decodeXmlStructure(ParseCtxXml *ctx, void *dst, const UA_DataType *type) {
    /* Check the recursion limit */
    if(ctx->depth >= UA_XML_ENCODING_MAX_RECURSION - 1)
        return UA_STATUSCODE_BADENCODINGERROR;
    ctx->depth++;

    uintptr_t ptr = (uintptr_t)dst;
    status ret = UA_STATUSCODE_GOOD;
    u8 membersSize = type->membersSize;
    UA_STACKARRAY(XmlDecodeEntry, entries, membersSize);
    for(size_t i = 0; i < membersSize; ++i) {
        const UA_DataTypeMember *m = &type->members[i];
        const UA_DataType *mt = m->memberType;
        entries[i].type = mt;
        entries[i].name = UA_STRING((char*)(uintptr_t)m->memberName);
        entries[i].found = false;
        ptr += m->padding;
        entries[i].fieldPointer = (void*)ptr;
        if(!m->isArray) {
            entries[i].function = NULL;
            ptr += mt->memSize;
        } else {
            entries[i].function = (decodeXmlSignature)Array_decodeXml;
            ptr += sizeof(size_t) + sizeof(void*);
        }
    }

    ret = decodeXmlFields(ctx, entries, membersSize);

    if(ctx->depth == 0)
        return UA_STATUSCODE_BADENCODINGERROR;
    ctx->depth--;
    return ret;
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
    (decodeXmlSignature)ExtensionObject_decodeXml,  /* ExtensionObject */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* DataValue */
    (decodeXmlSignature)Variant_decodeXml,          /* Variant */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* DiagnosticInfo */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* Decimal */
    (decodeXmlSignature)Int32_decodeXml,            /* Enum */
    (decodeXmlSignature)decodeXmlStructure,         /* Structure */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* Structure with optional fields */
    (decodeXmlSignature)decodeXmlNotImplemented,    /* Union */
    (decodeXmlSignature)decodeXmlNotImplemented     /* BitfieldCluster */
};

UA_StatusCode
UA_decodeXml(const UA_ByteString *src, void *dst, const UA_DataType *type,
             const UA_DecodeXmlOptions *options) {
    if(!dst || !src || !type)
        return UA_STATUSCODE_BADARGUMENTSMISSING;

    /* Tokenize. Add a fake wrapper element if options->unwrapped is enabled. */
    unsigned tokensSize = 63;
    xml_token tokenbuf[64];
    xml_token *tokens = tokenbuf;

    xml_result res = xml_tokenize((char*)src->data, (unsigned)src->length,
                                  tokens + 1, tokensSize);
    if(res.error == XML_ERROR_OVERFLOW) {
        tokens = (xml_token*)UA_malloc(sizeof(xml_token) * (res.num_tokens + 1));
        if(!tokens)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        res = xml_tokenize((char*)src->data, (unsigned)src->length,
                           tokens + 1, res.num_tokens);
    }

    if(res.error != XML_ERROR_NONE || res.num_tokens == 0) {
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
    if(options) {
        ctx.customTypes = options->customTypes;
        ctx.namespaceMapping = options->namespaceMapping;
        ctx.serverUris = options->serverUris;
        ctx.serverUrisSize = options->serverUrisSize;
    }

    if(options && options->unwrapped) {
        /* Set up the fake wrapper element */
        xml_token *tok = tokens;
        memset(tok, 0, sizeof(xml_token));
        tok->type = XML_TOKEN_ELEMENT;
        tok->name = UA_STRING((char*)(uintptr_t)type->typeName);
        tok->children = 1;
        tok->start = 0;
        tok->end = (unsigned)src->length;
        ctx.tokensSize++;
    } else {
        ctx.tokens++; /* Skip the first token */
    }

    /* Decode */
    UA_StatusCode ret = decodeXmlJumpTable[type->typeKind](&ctx, dst, type);
    if(ret != UA_STATUSCODE_GOOD)
        UA_clear(dst, type);

    /* Clean up */
    if(tokens != tokenbuf)
        UA_free(tokens);
    return ret;
}
