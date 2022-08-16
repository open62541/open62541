/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/types_generated.h>
#include <open62541/types_generated_handling.h>

#include "open62541_queue.h"
#include "ua_util_internal.h"

/* Printing of NodeIds is always enabled. We need it for logging. */

UA_StatusCode
UA_NodeId_print(const UA_NodeId *id, UA_String *output) {
    UA_String_clear(output);
    if(!id)
        return UA_STATUSCODE_GOOD;

    char *nsStr = NULL;
    long snprintfLen = 0;
    size_t nsLen = 0;
    if(id->namespaceIndex != 0) {
        nsStr = (char*)UA_malloc(9+1); // strlen("ns=XXXXX;") = 9 + Nullbyte
        if(!nsStr)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        snprintfLen = UA_snprintf(nsStr, 10, "ns=%d;", id->namespaceIndex);
        if(snprintfLen < 0 || snprintfLen >= 10) {
            UA_free(nsStr);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        nsLen = (size_t)(snprintfLen);
    }

    UA_ByteString byteStr = UA_BYTESTRING_NULL;
    switch (id->identifierType) {
        case UA_NODEIDTYPE_NUMERIC:
            /* ns (2 byte, 65535) = 5 chars, numeric (4 byte, 4294967295) = 10
             * chars, delim = 1 , nullbyte = 1-> 17 chars */
            output->length = nsLen + 2 + 10 + 1;
            output->data = (UA_Byte*)UA_malloc(output->length);
            if(output->data == NULL) {
                output->length = 0;
                UA_free(nsStr);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            snprintfLen = UA_snprintf((char*)output->data, output->length, "%si=%lu",
                                      nsLen > 0 ? nsStr : "",
                                      (unsigned long )id->identifier.numeric);
            break;
        case UA_NODEIDTYPE_STRING:
            /* ns (16bit) = 5 chars, strlen + nullbyte */
            output->length = nsLen + 2 + id->identifier.string.length + 1;
            output->data = (UA_Byte*)UA_malloc(output->length);
            if(output->data == NULL) {
                output->length = 0;
                UA_free(nsStr);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            snprintfLen = UA_snprintf((char*)output->data, output->length, "%ss=%.*s",
                                      nsLen > 0 ? nsStr : "", (int)id->identifier.string.length,
                                      id->identifier.string.data);
            break;
        case UA_NODEIDTYPE_GUID:
            /* ns (16bit) = 5 chars + strlen(A123456C-0ABC-1A2B-815F-687212AAEE1B)=36 + nullbyte */
            output->length = nsLen + 2 + 36 + 1;
            output->data = (UA_Byte*)UA_malloc(output->length);
            if(output->data == NULL) {
                output->length = 0;
                UA_free(nsStr);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            snprintfLen = UA_snprintf((char*)output->data, output->length,
                                      "%sg=" UA_PRINTF_GUID_FORMAT, nsLen > 0 ? nsStr : "",
                                      UA_PRINTF_GUID_DATA(id->identifier.guid));
            break;
        case UA_NODEIDTYPE_BYTESTRING:
            UA_ByteString_toBase64(&id->identifier.byteString, &byteStr);
            /* ns (16bit) = 5 chars + LEN + nullbyte */
            output->length = nsLen + 2 + byteStr.length + 1;
            output->data = (UA_Byte*)UA_malloc(output->length);
            if(output->data == NULL) {
                output->length = 0;
                UA_String_clear(&byteStr);
                UA_free(nsStr);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            snprintfLen = UA_snprintf((char*)output->data, output->length, "%sb=%.*s",
                                      nsLen > 0 ? nsStr : "",
                                      (int)byteStr.length, byteStr.data);
            UA_String_clear(&byteStr);
            break;
    }
    UA_free(nsStr);

    if(snprintfLen < 0 || snprintfLen >= (long) output->length) {
        UA_free(output->data);
        output->data = NULL;
        output->length = 0;
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    output->length = (size_t)snprintfLen;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ExpandedNodeId_print(const UA_ExpandedNodeId *id, UA_String *output) {
    /* Don't print the namespace-index if a NamespaceUri is set */
    UA_NodeId nid = id->nodeId;
    if(id->namespaceUri.data != NULL)
        nid.namespaceIndex = 0;

    /* Encode the NodeId */
    UA_String outNid = UA_STRING_NULL;
    UA_StatusCode res = UA_NodeId_print(&nid, &outNid);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Encode the ServerIndex */
    char svr[100];
    if(id->serverIndex == 0)
        svr[0] = 0;
    else
        UA_snprintf(svr, 100, "svr=%"PRIu32";", id->serverIndex);
    size_t svrlen = strlen(svr);

    /* Encode the NamespaceUri */
    char nsu[100];
    if(id->namespaceUri.data == NULL)
        nsu[0] = 0;
    else
        UA_snprintf(nsu, 100, "nsu=%.*s;", (int)id->namespaceUri.length, id->namespaceUri.data);
    size_t nsulen = strlen(nsu);

    /* Combine everything */
    res = UA_ByteString_allocBuffer((UA_String*)output, outNid.length + svrlen + nsulen);
    if(res == UA_STATUSCODE_GOOD) {
        memcpy(output->data, svr, svrlen);
        memcpy(&output->data[svrlen], nsu, nsulen);
        memcpy(&output->data[svrlen+nsulen], outNid.data, outNid.length);
    }

    UA_String_clear(&outNid);
    return res;
}

#ifdef UA_ENABLE_TYPEDESCRIPTION

/***********************/
/* Jumptable Signature */
/***********************/

typedef struct UA_PrintElement {
    TAILQ_ENTRY(UA_PrintElement) next;
    size_t length;
    UA_Byte data[];
} UA_PrintOutput;

typedef struct {
    size_t depth;
    TAILQ_HEAD(, UA_PrintElement) outputs;
} UA_PrintContext;

typedef UA_StatusCode
(*UA_printSignature)(UA_PrintContext *ctx, const void *p,
                     const UA_DataType *type);

extern const UA_printSignature printJumpTable[UA_DATATYPEKINDS];

/********************/
/* Helper Functions */
/********************/

static UA_PrintOutput *
UA_PrintContext_addOutput(UA_PrintContext *ctx, size_t length) {
    /* Protect against overlong output in pretty-printing */
    if(length > 2<<16)
        return NULL;
    UA_PrintOutput *output = (UA_PrintOutput*)UA_malloc(sizeof(UA_PrintOutput) + length + 1);
    if(!output)
        return NULL;
    output->length = length;
    TAILQ_INSERT_TAIL(&ctx->outputs, output, next);
    return output;
}

static UA_StatusCode
UA_PrintContext_addNewlineTabs(UA_PrintContext *ctx, size_t tabs) {
    UA_PrintOutput *out = UA_PrintContext_addOutput(ctx, tabs+1);
    if(!out)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    out->data[0] = '\n';
    for(size_t i = 1; i <= tabs; i++)
        out->data[i] = '\t';
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_PrintContext_addName(UA_PrintContext *ctx, const char *name) {
    size_t nameLen = strlen(name);
    UA_PrintOutput *out = UA_PrintContext_addOutput(ctx, nameLen+2);
    if(!out)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memcpy(&out->data, name, nameLen);
    out->data[nameLen] = ':';
    out->data[nameLen+1] = ' ';
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_PrintContext_addString(UA_PrintContext *ctx, const char *str) {
    size_t len = strlen(str);
    UA_PrintOutput *out = UA_PrintContext_addOutput(ctx, len);
    if(!out)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memcpy(&out->data, str, len);
    return UA_STATUSCODE_GOOD;
}

/*********************/
/* Printing Routines */
/*********************/

static UA_StatusCode
printArray(UA_PrintContext *ctx, const void *p, const size_t length,
           const UA_DataType *type);

static UA_StatusCode
printBoolean(UA_PrintContext *ctx, const UA_Boolean *p, const UA_DataType *_) {
    if(*p)
        return UA_PrintContext_addString(ctx, "true");
    return UA_PrintContext_addString(ctx, "false");
}

static UA_StatusCode
printSByte(UA_PrintContext *ctx, const UA_SByte *p, const UA_DataType *_) {
    char out[32];
    UA_snprintf(out, 32, "%"PRIi8, *p);
    return UA_PrintContext_addString(ctx, out);
}

static UA_StatusCode
printByte(UA_PrintContext *ctx, const UA_Byte *p, const UA_DataType *_) {
    char out[32];
    UA_snprintf(out, 32, "%"PRIu8, *p);
    return UA_PrintContext_addString(ctx, out);
}

static UA_StatusCode
printInt16(UA_PrintContext *ctx, const UA_Int16 *p, const UA_DataType *_) {
    char out[32];
    UA_snprintf(out, 32, "%"PRIi16, *p);
    return UA_PrintContext_addString(ctx, out);
}

static UA_StatusCode
printUInt16(UA_PrintContext *ctx, const UA_UInt16 *p, const UA_DataType *_) {
    char out[32];
    UA_snprintf(out, 32, "%"PRIu16, *p);
    return UA_PrintContext_addString(ctx, out);
}

static UA_StatusCode
printInt32(UA_PrintContext *ctx, const UA_Int32 *p, const UA_DataType *_) {
    char out[32];
    UA_snprintf(out, 32, "%"PRIi32, *p);
    return UA_PrintContext_addString(ctx, out);
}

static UA_StatusCode
printUInt32(UA_PrintContext *ctx, const UA_UInt32 *p, const UA_DataType *_) {
    char out[32];
    UA_snprintf(out, 32, "%"PRIu32, *p);
    return UA_PrintContext_addString(ctx, out);
}

static UA_StatusCode
printInt64(UA_PrintContext *ctx, const UA_Int64 *p, const UA_DataType *_) {
    char out[64];
    UA_snprintf(out, 64, "%"PRIi64, *p);
    return UA_PrintContext_addString(ctx, out);
}

static UA_StatusCode
printUInt64(UA_PrintContext *ctx, const UA_UInt64 *p, const UA_DataType *_) {
    char out[64];
    UA_snprintf(out, 64, "%"PRIu64, *p);
    return UA_PrintContext_addString(ctx, out);
}

static UA_StatusCode
printFloat(UA_PrintContext *ctx, const UA_Float *p, const UA_DataType *_) {
    char out[64];
    UA_snprintf(out, 32, "%f", *p);
    return UA_PrintContext_addString(ctx, out);
}

static UA_StatusCode
printDouble(UA_PrintContext *ctx, const UA_Double *p, const UA_DataType *_) {
    char out[64];
    UA_snprintf(out, 64, "%lf", *p);
    return UA_PrintContext_addString(ctx, out);
}

static UA_StatusCode
printStatusCode(UA_PrintContext *ctx, const UA_StatusCode *p, const UA_DataType *_) {
    return UA_PrintContext_addString(ctx, UA_StatusCode_name(*p));
}

static UA_StatusCode
printNodeId(UA_PrintContext *ctx, const UA_NodeId *p, const UA_DataType *_) {
    UA_String out;
    UA_String_init(&out);
    UA_StatusCode res = UA_NodeId_print(p, &out);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_PrintOutput *po = UA_PrintContext_addOutput(ctx, out.length);
    if(po)
        memcpy(po->data, out.data, out.length);
    else
        res = UA_STATUSCODE_BADOUTOFMEMORY;
    UA_String_clear(&out);
    return res;
}

static UA_StatusCode
printExpandedNodeId(UA_PrintContext *ctx, const UA_ExpandedNodeId *p, const UA_DataType *_) {
    UA_String out;
    UA_String_init(&out);
    UA_StatusCode res = UA_ExpandedNodeId_print(p, &out);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_PrintOutput *po = UA_PrintContext_addOutput(ctx, out.length);
    if(!po)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memcpy(po->data, out.data, out.length);
    UA_String_clear(&out);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
printDateTime(UA_PrintContext *ctx, const UA_DateTime *p, const UA_DataType *_) {
    UA_Int64 tOffset = UA_DateTime_localTimeUtcOffset();
    UA_DateTimeStruct dts = UA_DateTime_toStruct(*p);
    char dateString[100];
    UA_snprintf((char*)dateString, 100,
                "%04u-%02u-%02u %02u:%02u:%02u.%03u (UTC%+05d)",
                dts.year, dts.month, dts.day, dts.hour, dts.min,
                dts.sec, dts.milliSec,
            (int)(tOffset / UA_DATETIME_SEC / 36));
    return UA_PrintContext_addString(ctx, dateString);
}

static UA_StatusCode
printGuid(UA_PrintContext *ctx, const UA_Guid *p, const UA_DataType *_) {
    char tmp[100];
    UA_snprintf(tmp, 100, UA_PRINTF_GUID_FORMAT, UA_PRINTF_GUID_DATA(*p));
    return UA_PrintContext_addString(ctx, tmp);
}

static UA_StatusCode
printString(UA_PrintContext *ctx, const UA_String *p, const UA_DataType *_) {
    if(!p->data)
        return UA_PrintContext_addString(ctx, "NullString");
    UA_PrintOutput *out = UA_PrintContext_addOutput(ctx, p->length+2);
    if(!out)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_snprintf((char*)out->data, p->length+3, "\"%.*s\"", (int)p->length, p->data);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
printByteString(UA_PrintContext *ctx, const UA_ByteString *p, const UA_DataType *_) {
    if(!p->data)
        return UA_PrintContext_addString(ctx, "NullByteString");
    UA_String str = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_ByteString_toBase64(p, &str);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = printString(ctx, &str, NULL);
    UA_String_clear(&str);
    return res;
}

static UA_StatusCode
printQualifiedName(UA_PrintContext *ctx, const UA_QualifiedName *p, const UA_DataType *_) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_PrintContext_addString(ctx, "{");
    ctx->depth++;
    retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
    retval |= UA_PrintContext_addName(ctx, "NamespaceIndex");
    retval |= printUInt16(ctx, &p->namespaceIndex, NULL);
    retval |= UA_PrintContext_addString(ctx, ",");
    retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
    retval |= UA_PrintContext_addName(ctx, "Name");
    retval |= printString(ctx, &p->name, NULL);
    ctx->depth--;
    retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
    retval |= UA_PrintContext_addString(ctx, "}");
    return retval;
}

static UA_StatusCode
printLocalizedText(UA_PrintContext *ctx, const UA_LocalizedText *p, const UA_DataType *_) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_PrintContext_addString(ctx, "{");
    ctx->depth++;
    retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
    retval |= UA_PrintContext_addName(ctx, "Locale");
    retval |= printString(ctx, &p->locale, NULL);
    retval |= UA_PrintContext_addString(ctx, ",");
    retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
    retval |= UA_PrintContext_addName(ctx, "Text");
    retval |= printString(ctx, &p->text, NULL);
    ctx->depth--;
    retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
    retval |= UA_PrintContext_addString(ctx, "}");
    return retval;
}

static UA_StatusCode
printVariant(UA_PrintContext *ctx, const UA_Variant *p, const UA_DataType *_) {
    if(!p->type)
        return UA_PrintContext_addString(ctx, "NullVariant");

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_PrintContext_addString(ctx, "{");
    ctx->depth++;

    retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
    retval |= UA_PrintContext_addName(ctx, "DataType");
    retval |= UA_PrintContext_addString(ctx, p->type->typeName);
    retval |= UA_PrintContext_addString(ctx, ",");

    retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
    retval |= UA_PrintContext_addName(ctx, "Value");
    if(UA_Variant_isScalar(p))
        retval |= printJumpTable[p->type->typeKind](ctx, p->data, p->type);
    else
        retval |= printArray(ctx, p->data, p->arrayLength, p->type);

    if(p->arrayDimensionsSize > 0) {
        retval |= UA_PrintContext_addString(ctx, ",");
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addName(ctx, "ArrayDimensions");
        retval |= printArray(ctx, p->arrayDimensions, p->arrayDimensionsSize,
                             &UA_TYPES[UA_TYPES_UINT32]);
    }

    ctx->depth--;
    retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
    retval |= UA_PrintContext_addString(ctx, "}");
    return retval;
}

static UA_StatusCode
printExtensionObject(UA_PrintContext *ctx, const UA_ExtensionObject*p,
                     const UA_DataType *_) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    switch(p->encoding) {
    case UA_EXTENSIONOBJECT_ENCODED_NOBODY:
        return UA_PrintContext_addString(ctx, "ExtensionObject(No Body)");
    case UA_EXTENSIONOBJECT_ENCODED_BYTESTRING:
        res |= UA_PrintContext_addString(ctx, "ExtensionObject(Binary Encoded) {");
        ctx->depth++;
        res |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        res |= UA_PrintContext_addName(ctx, "DataType");
        res |= printNodeId(ctx, &p->content.encoded.typeId, NULL);
        res |= UA_PrintContext_addString(ctx, ",");
        res |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        res |= UA_PrintContext_addName(ctx, "Body");
        res |= printByteString(ctx, &p->content.encoded.body, NULL);
        ctx->depth--;
        res |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        res |= UA_PrintContext_addName(ctx, "}");
        break;
    case UA_EXTENSIONOBJECT_ENCODED_XML:
        res |= UA_PrintContext_addString(ctx, "ExtensionObject(XML Encoded) {");
        ctx->depth++;
        res |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        res |= UA_PrintContext_addName(ctx, "DataType");
        res |= printNodeId(ctx, &p->content.encoded.typeId, NULL);
        res |= UA_PrintContext_addString(ctx, ",");
        res |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        res |= UA_PrintContext_addName(ctx, "Body");
        res |= printString(ctx, (const UA_String*)&p->content.encoded.body, NULL);
        ctx->depth--;
        res |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        res |= UA_PrintContext_addName(ctx, "}");
        break;
    case UA_EXTENSIONOBJECT_DECODED:
    case UA_EXTENSIONOBJECT_DECODED_NODELETE:
        res |= UA_PrintContext_addString(ctx, "ExtensionObject {");
        ctx->depth++;
        res |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        res |= UA_PrintContext_addName(ctx, "DataType");
        res |= UA_PrintContext_addString(ctx, p->content.decoded.type->typeName);
        res |= UA_PrintContext_addString(ctx, ",");
        res |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        res |= UA_PrintContext_addName(ctx, "Body");
        res |= printJumpTable[p->content.decoded.type->typeKind](ctx,
                                                                 p->content.decoded.data,
                                                                 p->content.decoded.type);
        ctx->depth--;
        res |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        res |= UA_PrintContext_addName(ctx, "}");
        break;
    default:
        res = UA_STATUSCODE_BADINTERNALERROR;
        break;
    }
    return res;
}

static UA_StatusCode
printDataValue(UA_PrintContext *ctx, const UA_DataValue *p, const UA_DataType *_) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_PrintContext_addString(ctx, "{");
    ctx->depth++;
    UA_Boolean comma = false;

    if(p->hasValue) {
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addName(ctx, "Value");
        retval |= printVariant(ctx, &p->value, NULL);
        comma = true;
    }

    if(p->hasStatus) {
        if(comma)
            retval |= UA_PrintContext_addString(ctx, ",");
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addName(ctx, "Status");
        retval |= printStatusCode(ctx, &p->status, NULL);
        comma = true;
    }

    if(p->hasSourceTimestamp) {
        if(comma)
            retval |= UA_PrintContext_addString(ctx, ",");
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addName(ctx, "SourceTimestamp");
        retval |= printDateTime(ctx, &p->sourceTimestamp, NULL);
        comma = true;
    }

    if(p->hasSourcePicoseconds) {
        if(comma)
            retval |= UA_PrintContext_addString(ctx, ",");
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addName(ctx, "SourcePicoseconds");
        retval |= printUInt16(ctx, &p->sourcePicoseconds, NULL);
        comma = true;
    }

    if(p->hasServerTimestamp) {
        if(comma)
            retval |= UA_PrintContext_addString(ctx, ",");
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addName(ctx, "ServerTimestamp");
        retval |= printDateTime(ctx, &p->serverTimestamp, NULL);
        comma = true;
    }

    if(p->hasServerPicoseconds) {
        if(comma)
            retval |= UA_PrintContext_addString(ctx, ",");
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addName(ctx, "ServerPicoseconds");
        retval |= printUInt16(ctx, &p->serverPicoseconds, NULL);
        comma = true;
    }

    ctx->depth--;
    if(comma) {
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addString(ctx, "}");
    } else {
        retval |= UA_PrintContext_addString(ctx, " }");
    }
    return retval;
}

static UA_StatusCode
printDiagnosticInfo(UA_PrintContext *ctx, const UA_DiagnosticInfo *p, const UA_DataType *_) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_PrintContext_addString(ctx, "{");
    ctx->depth++;
    UA_Boolean comma = false;

    if(p->hasSymbolicId) {
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addName(ctx, "SymbolicId");
        retval |= printInt32(ctx, &p->symbolicId, NULL);
        comma = true;
    }

    if(p->hasNamespaceUri) {
        if(comma)
            retval |= UA_PrintContext_addString(ctx, ",");
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addName(ctx, "NamespaceUri");
        retval |= printInt32(ctx, &p->namespaceUri, NULL);
        comma = true;
    }

    if(p->hasLocalizedText) {
        if(comma)
            retval |= UA_PrintContext_addString(ctx, ",");
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addName(ctx, "LocalizedText");
        retval |= printInt32(ctx, &p->localizedText, NULL);
        comma = true;
    }

    if(p->hasLocale) {
        if(comma)
            retval |= UA_PrintContext_addString(ctx, ",");
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addName(ctx, "Locale");
        retval |= printInt32(ctx, &p->locale, NULL);
        comma = true;
    }

    if(p->hasAdditionalInfo) {
        if(comma)
            retval |= UA_PrintContext_addString(ctx, ",");
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addName(ctx, "AdditionalInfo");
        retval |= printString(ctx, &p->additionalInfo, NULL);
        comma = true;
    }

    if(p->hasInnerStatusCode) {
        if(comma)
            retval |= UA_PrintContext_addString(ctx, ",");
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addName(ctx, "InnerStatusCode");
        retval |= printStatusCode(ctx, &p->innerStatusCode, NULL);
        comma = true;
    }

    if(p->hasInnerDiagnosticInfo) {
        if(comma)
            retval |= UA_PrintContext_addString(ctx, ",");
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addName(ctx, "InnerDiagnosticInfo");
        retval |= printDiagnosticInfo(ctx, p->innerDiagnosticInfo, NULL);
        comma = true;
    }

    ctx->depth--;
    if(comma) {
        retval |= UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        retval |= UA_PrintContext_addString(ctx, "}");
    } else {
        retval |= UA_PrintContext_addString(ctx, " }");
    }
    return retval;
}

static UA_StatusCode
printArray(UA_PrintContext *ctx, const void *p, const size_t length,
           const UA_DataType *type) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(!p) {
        retval |= UA_PrintContext_addString(ctx, "Array(-1, ");
        retval |= UA_PrintContext_addString(ctx, type->typeName);
        retval |= UA_PrintContext_addString(ctx, ")");
        return retval;
    }

    UA_UInt32 length32 = (UA_UInt32)length;
    retval |= UA_PrintContext_addString(ctx, "Array(");
    retval |= printUInt32(ctx, &length32, NULL);
    retval |= UA_PrintContext_addString(ctx, ", ");
    retval |= UA_PrintContext_addString(ctx, type->typeName);
    retval |= UA_PrintContext_addString(ctx, ") {");
    ctx->depth++;
    uintptr_t target = (uintptr_t)p;
    for(UA_UInt32 i = 0; i < length; i++) {
        UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        printUInt32(ctx, &i, NULL);
        retval |= UA_PrintContext_addString(ctx, ": ");
        printJumpTable[type->typeKind](ctx, (const void*)target, type);
        if(i < length - 1)
            retval |= UA_PrintContext_addString(ctx, ",");
        target += type->memSize;
    }
    ctx->depth--;
    UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
    retval |= UA_PrintContext_addString(ctx, "}");
    return retval;
}

static UA_StatusCode
printStructure(UA_PrintContext *ctx, const void *p, const UA_DataType *type) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    uintptr_t ptrs = (uintptr_t)p;
    retval |= UA_PrintContext_addString(ctx, "{");
    ctx->depth++;
    for(size_t i = 0; i < type->membersSize; ++i) {
        UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
        const UA_DataTypeMember *m = &type->members[i];
        const UA_DataType *mt = m->memberType;
        ptrs += m->padding;
        retval |= UA_PrintContext_addName(ctx, m->memberName);
        if(!m->isArray) {
            retval |= printJumpTable[mt->typeKind](ctx, (const void *)ptrs, mt);
            ptrs += mt->memSize;
        } else {
            const size_t size = *((const size_t*)ptrs);
            ptrs += sizeof(size_t);
            retval |= printArray(ctx, *(void* const*)ptrs, size, mt);
            ptrs += sizeof(void*);
        }
        if(i < (size_t)(type->membersSize - 1))
            retval |= UA_PrintContext_addString(ctx, ",");
    }
    ctx->depth--;
    UA_PrintContext_addNewlineTabs(ctx, ctx->depth);
    retval |= UA_PrintContext_addString(ctx, "}");
    return retval;
}

static UA_StatusCode
printNotImplemented(UA_PrintContext *ctx, const void *p, const UA_DataType *type) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_PrintContext_addString(ctx, type->typeName);
    res |= UA_PrintContext_addString(ctx, " (Printing Not Implemented)");
    return res;
}

const UA_printSignature printJumpTable[UA_DATATYPEKINDS] = {
    (UA_printSignature)printBoolean,
    (UA_printSignature)printSByte,
    (UA_printSignature)printByte,
    (UA_printSignature)printInt16,
    (UA_printSignature)printUInt16,
    (UA_printSignature)printInt32,
    (UA_printSignature)printUInt32,
    (UA_printSignature)printInt64,
    (UA_printSignature)printUInt64,
    (UA_printSignature)printFloat,
    (UA_printSignature)printDouble,
    (UA_printSignature)printString,
    (UA_printSignature)printDateTime,
    (UA_printSignature)printGuid,
    (UA_printSignature)printByteString,
    (UA_printSignature)printString,         /* XmlElement */
    (UA_printSignature)printNodeId,
    (UA_printSignature)printExpandedNodeId,
    (UA_printSignature)printStatusCode,
    (UA_printSignature)printQualifiedName,
    (UA_printSignature)printLocalizedText,
    (UA_printSignature)printExtensionObject,
    (UA_printSignature)printDataValue,
    (UA_printSignature)printVariant,
    (UA_printSignature)printDiagnosticInfo,
    (UA_printSignature)printNotImplemented, /* Decimal */
    (UA_printSignature)printUInt32,         /* Enumeration */
    (UA_printSignature)printStructure,
    (UA_printSignature)printNotImplemented, /* Structure with Optional Fields */
    (UA_printSignature)printNotImplemented, /* Union */
    (UA_printSignature)printNotImplemented  /* BitfieldCluster*/
};

UA_StatusCode
UA_print(const void *p, const UA_DataType *type, UA_String *output) {
    UA_PrintContext ctx;
    ctx.depth = 0;
    TAILQ_INIT(&ctx.outputs);

    /* Allocate before the goto */
    size_t total = 0;
    size_t pos = 0;
    UA_PrintOutput *out, *out_tmp;

    /* Encode */
    UA_StatusCode retval = printJumpTable[type->typeKind](&ctx, p, type);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* If printing succeeded the output cannot be empty*/
    TAILQ_FOREACH(out, &ctx.outputs, next)
        total += out->length;
    UA_assert(total > 0);

    if(output->length == 0) {
        /* Allocate memory for the output */
        retval = UA_ByteString_allocBuffer((UA_String*)output, total);
    } else {
        /* Check if the buffer is large enough */
        if(output->length >= total)
            output->length = total;
        else
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
    }
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Write to the output buffer */
    TAILQ_FOREACH(out, &ctx.outputs, next) {
        memcpy(&output->data[pos], out->data, out->length);
        pos += out->length;
    }

 cleanup:
    /* Free the context */
    TAILQ_FOREACH_SAFE(out, &ctx.outputs, next, out_tmp) {
        TAILQ_REMOVE(&ctx.outputs, out, next);
        UA_free(out);
    }
    return retval;
}

#endif /* UA_ENABLE_TYPEDESCRIPTION */
