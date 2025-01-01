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
    {"<xs:complexType name=\"ExtensionObject\">"
       "<xs:sequence>"
         "<xs:element name=\"TypeId\" type=\"tns:NodeId\" minOccurs=\"0\" nillable=\"true\"/>"
         "<xs:element name=\"Body\" minOccurs=\"0\" nillable=\"true\">"
           "<xs:complexType>"
             "<xs:sequence>"
               "<xs:any minOccurs=\"0\" processContents=\"lax\"/>"
             "</xs:sequence>"
           "</xs:complexType>"
         "</xs:element>"
       "</xs:sequence>"
     "</xs:complexType>", 330},                                                             /* ExtensionObject */
    {"", 0},                                                                                /* DataValue */
    {"<xs:complexType name=\"Variant\">"
       "<xs:sequence>"
         "<xs:element name=\"Value\" minOccurs=\"0\" nillable=\"true\">"
           "<xs:complexType>"
             "<xs:sequence>"
               "<xs:any minOccurs=\"0\" processContents=\"lax\"/>"
             "</xs:sequence>"
           "</xs:complexType>"
         "</xs:element>"
       "</xs:sequence>"
     "</xs:complexType>", 248},                                                             /* Variant */
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

/* ExtensionObject */
static const char* UA_XML_EXTENSIONOBJECT_TYPEID = "TypeId"; // NodeId
static const char* UA_XML_EXTENSIONOBJECT_BODY = "Body";
static const char* UA_XML_EXTENSIONOBJECT_BYTESTRING = "ByteString";

/* Variant */
static const char* UA_XML_VARIANT_VALUE = "Value";

const char* xsdTypeNames[UA_DATATYPEKINDS] = {
  "xs:boolean",           /* Boolean */
  "xs:byte",              /* SByte */
  "xs:unsignedByte",      /* Byte */
  "xs:short",             /* Int16 */
  "xs:unsignedShort",     /* UInt16 */
  "xs:int",               /* Int32 */
  "xs:unsignedInt",       /* UInt32 */
  "xs:long",              /* Int64 */
  "xs:unsignedLong",      /* UInt64 */
  "xs:float",             /* Float */
  "xs:double",            /* Double */
  "xs:string",            /* String */
  "xs:dateTime",          /* DateTime */
  "tns:Guid",             /* Guid */
  "xs:base64Binary",      /* ByteString */
  "tns:XmlElement",       /* XmlElement */
  "tns:NodeId",           /* NodeId */
  "tns:ExpandedNodeId",   /* ExpandedNodeId */
  "tns:StatusCode",       /* StatusCode */
  "tns:QualifiedName",    /* QualifiedName */
  "tns:LocalizedText",    /* LocalizedText */
  "tns:ExtensionObject",  /* ExtensionObject */
  "tns:DataValue",        /* DataValue */
  "tns:Variant",          /* Variant */
  "tns:DiagnosticInfo",   /* DiagnosticInfo */
  "tns:Decimal",          /* Decimal */
  NULL,                   /* Enum */
  NULL,                   /* Structure */
  NULL,                   /* Structure with optional fields */
  NULL,                   /* Union */
  NULL                    /* BitfieldCluster */
};

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
    char* arrName[128];
    size_t arrNameLen = strlen("ListOf") + strlen(type->typeName);
    if(arrNameLen >= 128)
        return UA_STATUSCODE_BADENCODINGERROR;
    memcpy(arrName, "ListOf", strlen("ListOf"));
    memcpy(arrName + strlen("ListOf"), type-typeName, strlen(type->typeName));
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
    UA_String str = {length, (UA_Byte*)(uintptr_t)data};
    return decodeDateTime(str, dst);
}

static status
decodeXmlFields(ParseCtxXml *ctx, XmlDecodeEntry *entries, size_t entryCount) {
    CHECK_DATA_BOUNDS;

    if(ctx->depth >= UA_XML_ENCODING_MAX_RECURSION - 1)
        return UA_STATUSCODE_BADENCODINGERROR;
    ctx->depth++;

    size_t objectCount = 0;
    if(ctx->dataMembers[ctx->index]->type == XML_DATA_TYPE_COMPLEX)
        objectCount = ctx->dataMembers[ctx->index]->value.complex.membersSize;

    /* Empty object, nothing to decode */
    if(objectCount == 0) {
        ctx->depth--;
        ctx->index++; /* Jump to the element after the empty object */
        return UA_STATUSCODE_GOOD;
    }

    ctx->index++; /* Go to first entry element */

    /* CHECK_DATA_BOUNDS */
    if(ctx->index >= ctx->membersSize) {
        ctx->depth--;
        return UA_STATUSCODE_BADDECODINGERROR;
    }

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
            if(ctx->index >= ctx->membersSize) {
                ret = UA_STATUSCODE_BADDECODINGERROR;
                goto cleanup;
            }

            if(strcmp(ctx->dataMembers[ctx->index]->name, entries[index].fieldName))
                continue;

            /* Duplicate key found, abort */
            if(entries[index].found) {
                ret = UA_STATUSCODE_BADDECODINGERROR;
                goto cleanup;
            }

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
                goto cleanup;
            break;
        }
    }

cleanup:
    ctx->depth--;
    return ret;
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

static UA_FUNC_ATTR_WARN_UNUSED_RESULT status
lookAheadForXmlElemName(XmlData *data, const char *name) {
    status ret = UA_STATUSCODE_BADNOTFOUND;
    if(!strcmp(data->name, name)) {
        ret = UA_STATUSCODE_GOOD;
        goto finish;
    }
    if(data->type != XML_DATA_TYPE_PRIMITIVE) {
        for(size_t i = 0; i < data->value.complex.membersSize; ++i) {
            ret = lookAheadForXmlElemName(data->value.complex.members[i], name);
            if(ret == UA_STATUSCODE_GOOD)
                goto finish;
        }
    }

finish:
    return ret;
}

static UA_FUNC_ATTR_WARN_UNUSED_RESULT size_t
calcExtObjStrSize(XmlData* data) {
    size_t retSize = 0;
    UA_Boolean emptyElement = false;
    /* XML element start sequence. */
    retSize += strlen("<>");
    retSize += strlen(data->name);
    if(data->type == XML_DATA_TYPE_PRIMITIVE) {
        /* Empty primitive XML object. */
        if(data->value.primitive.value == NULL)
            emptyElement = true;
        else
            retSize += strlen(data->value.primitive.value);
    }
    else {
        /* Empty complex XML object. */
        if(data->value.complex.membersSize == 0)
            emptyElement = true;
        else {
            for(size_t i = 0; i < data->value.complex.membersSize; ++i)
                retSize += calcExtObjStrSize(data->value.complex.members[i]);
        }
    }
    /* XML element end sequence. */
    if(emptyElement) {
        retSize += strlen(" /");
        return retSize;
    }

    retSize += strlen(data->name);
    retSize += strlen("</>");
    return retSize;
}

static __attribute__ ((unused)) size_t
xmlExtObjBody(ParseCtxXml *ctx, UA_Byte* outData) {
    size_t curPos = 0;
    UA_Boolean emptyElement = false;
    XmlData *data = ctx->dataMembers[ctx->index];

    /* XML element start sequence. */
    outData[curPos++] = '<';
    for(size_t i = 0; i < strlen(data->name); ++i)
        outData[curPos + i] = (UA_Byte)data->name[i];
    curPos += strlen(data->name);

    if(data->type == XML_DATA_TYPE_PRIMITIVE) {
        /* Empty primitive XML object. */
        if(data->value.primitive.value == NULL)
            emptyElement = true;
        else {
            outData[curPos++] = '>';
            for(size_t i = 0; i < strlen(data->value.primitive.value); ++i)
                outData[curPos + i] = (UA_Byte)data->value.primitive.value[i];
            curPos += strlen(data->value.primitive.value);
        }
        ctx->index++;
    }
    else {
        /* Empty complex XML object. */
        if(data->value.complex.membersSize == 0)
            emptyElement = true;
        else {
            outData[curPos++] = '>';
            /* Go to the next element inside complex type. */
            ctx->index++;
            for(size_t i = 0; i < data->value.complex.membersSize; ++i)
                curPos += xmlExtObjBody(ctx, outData + curPos);
        }
    }

    /* XML element end sequence. */
    if(emptyElement) {
        outData[curPos++] = ' ';
        outData[curPos++] = '/';
        outData[curPos++] = '>';
        return curPos;
    }

    outData[curPos++] = '<';
    outData[curPos++] = '/';
    for(size_t i = 0; i < strlen(data->name); ++i)
        outData[curPos + i] = (UA_Byte)data->name[i];
    curPos += strlen(data->name);
    outData[curPos++] = '>';

    return curPos;
}

DECODE_XML(ExtensionObject) {
    CHECK_DATA_BOUNDS;

    /* Empty object -> Null ExtensionObject */
    if(ctx->dataMembers[ctx->index]->type == XML_DATA_TYPE_PRIMITIVE &&
       !strcmp(ctx->dataMembers[ctx->index]->value.primitive.value, "null")) {
        ctx->index++; /* Skip the empty ExtensionObject */
        return UA_STATUSCODE_GOOD;
    }

    /* Search for ByteString encoding */
    status ret = lookAheadForXmlElemName(ctx->dataMembers[ctx->index], UA_XML_EXTENSIONOBJECT_BYTESTRING);

    /* ByteString encoding found */
    if(ret == UA_STATUSCODE_GOOD)
        dst->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    else
        dst->encoding = UA_EXTENSIONOBJECT_ENCODED_XML;

    /* Check fields of the ExtensionObject typeId. */
    XmlDecodeEntry entryTypeId = {
        UA_XML_EXTENSIONOBJECT_TYPEID, &dst->content.encoded.typeId, NULL, false, &UA_TYPES[UA_TYPES_NODEID]
    };
    ret = decodeXmlFields(ctx, &entryTypeId, 1);

    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    if(dst->encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING) {
        /* Check ExtensionObject body separate from TypeId (Body element
         * is not of an XML complex object type, rather a wrapper around
         * either a ByteString or XML encoded Structure (String)). */
        XmlDecodeEntry entriesBody[2] = {
            {UA_XML_EXTENSIONOBJECT_BODY, NULL, NULL, false, &UA_TYPES[UA_TYPES_BYTESTRING]},
            {UA_XML_EXTENSIONOBJECT_BYTESTRING, &dst->content.encoded.body, NULL, false, &UA_TYPES[UA_TYPES_STRING]}
        };
        return decodeXmlFields(ctx, entriesBody, 2);
    }

    /* Current element must have `Body` name, and also must include
     * additional XML elements. */
    if(strcmp(ctx->dataMembers[ctx->index]->name, UA_XML_EXTENSIONOBJECT_BODY) ||
       ctx->dataMembers[ctx->index]->type == XML_DATA_TYPE_PRIMITIVE)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Go to the next element inside Body segment. */
    ctx->index++;

    size_t xmlBodySize = calcExtObjStrSize(ctx->dataMembers[ctx->index]);
    UA_ByteString_allocBuffer(&dst->content.encoded.body, xmlBodySize);

    (void)xmlExtObjBody(ctx, dst->content.encoded.body.data);

    return UA_STATUSCODE_GOOD;
}

static status
Array_decodeXml(ParseCtxXml *ctx, void **dst, const UA_DataType *type) {
    if(strncmp(ctx->dataMembers[ctx->index]->name, "ListOf", strlen("ListOf")))
        return UA_STATUSCODE_BADDECODINGERROR;

    if(ctx->dataMembers[ctx->index]->type == XML_DATA_TYPE_PRIMITIVE) {
        /* Return early for empty arrays */
        *dst = UA_EMPTY_ARRAY_SENTINEL;
        return UA_STATUSCODE_GOOD;
    }

    size_t length = ctx->dataMembers[ctx->index]->value.complex.membersSize;
    ctx->index++; /* Go to first array member. */

    /* Allocate memory */
    *dst = UA_calloc(length, type->memSize);
    if(*dst == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Decode array members */
    uintptr_t ptr = (uintptr_t)*dst;
    for(size_t i = 0; i < length; ++i) {
        status ret = decodeXmlJumpTable[type->typeKind](ctx, (void*)ptr, type);
        if(ret != UA_STATUSCODE_GOOD) {
            UA_Array_delete(*dst, i + 1, type);
            *dst = NULL;
            return ret;
        }
        ptr += type->memSize;
    }

    return UA_STATUSCODE_GOOD;
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
    if(data != NULL) {
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

    ctx.dataMembers = (XmlData**)UA_malloc(UA_XML_MAXMEMBERSCOUNT * sizeof(XmlData*));
    ctx.parseCtx = &parseCtx;
    ctx.value = &value;
    ctx.index = 0;
    ctx.depth = 0;
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
    if(ctx.value->isArray)
        ret = Array_decodeXml(&ctx, (void**)dst, type);
    else {
        memset(dst, 0, type->memSize); /* Initialize the value */
        ret = decodeXmlJumpTable[type->typeKind](&ctx, dst, type);
    }

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
