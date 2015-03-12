#include <string.h>
#include "ua_types_encoding_binary.h"
#include "ua_util.h"
#include "ua_statuscodes.h"
#include "ua_types_generated.h"

#define UA_TYPE_CALCSIZEBINARY_MEMSIZE(TYPE) \
    size_t TYPE##_calcSizeBinary(TYPE const *p) { return sizeof(TYPE); }

#define UA_TYPE_BINARY_ENCODING_AS(TYPE, TYPE_AS)                       \
    size_t TYPE##_calcSizeBinary(TYPE const *p) {                       \
        return TYPE_AS##_calcSizeBinary((const TYPE_AS *)p);            \
    }                                                                   \
    UA_StatusCode TYPE##_encodeBinary(TYPE const *src, UA_ByteString *dst, size_t *offset) { \
        return TYPE_AS##_encodeBinary((const TYPE_AS *)src, dst, offset); \
    }                                                                   \
    UA_StatusCode TYPE##_decodeBinary(UA_ByteString const *src, size_t *offset, TYPE *dst) { \
        return TYPE_AS##_decodeBinary(src, offset, (TYPE_AS *)dst);     \
    }

/* Boolean */
UA_TYPE_CALCSIZEBINARY_MEMSIZE(UA_Boolean)
UA_StatusCode UA_Boolean_encodeBinary(const UA_Boolean *src, UA_ByteString *dst, size_t *offset) {
    if(*offset + sizeof(UA_Boolean) > (size_t)dst->length)
        return UA_STATUSCODE_BADENCODINGERROR;
    dst->data[*offset] = (UA_Byte)*src;
    (*offset)++;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Boolean_decodeBinary(UA_ByteString const *src, size_t *offset, UA_Boolean *dst) {
    if(*offset + sizeof(UA_Boolean) > (size_t)src->length )
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (src->data[*offset] > 0) ? UA_TRUE : UA_FALSE;
    (*offset)++;
    return UA_STATUSCODE_GOOD;
}

/* SByte */
UA_TYPE_BINARY_ENCODING_AS(UA_SByte, UA_Byte)

/* Byte */
UA_TYPE_CALCSIZEBINARY_MEMSIZE(UA_Byte)
UA_StatusCode UA_Byte_encodeBinary(const UA_Byte *src, UA_ByteString *dst, size_t *offset) {
    if(*offset + sizeof(UA_Byte) > (size_t)dst->length)
        return UA_STATUSCODE_BADENCODINGERROR;
    dst->data[*offset] = (UA_Byte)*src;
    (*offset)++;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Byte_decodeBinary(UA_ByteString const *src, size_t *offset, UA_Byte *dst) {
    if(*offset + sizeof(UA_Byte) > (size_t)src->length )
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = src->data[*offset];
    (*offset)++;
    return UA_STATUSCODE_GOOD;
}

/* Int16 */
UA_TYPE_BINARY_ENCODING_AS(UA_Int16, UA_UInt16)

/* UInt16 */
UA_TYPE_CALCSIZEBINARY_MEMSIZE(UA_UInt16)
UA_StatusCode UA_UInt16_encodeBinary(UA_UInt16 const *src, UA_ByteString * dst, size_t *offset) {
    if(*offset + sizeof(UA_UInt16) > (size_t)dst->length )
        return UA_STATUSCODE_BADENCODINGERROR;
    dst->data[(*offset)++] = (UA_Byte)((*src & 0x00FF) >> 0);
    dst->data[(*offset)++] = (UA_Byte)((*src & 0xFF00) >> 8);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_UInt16_decodeBinary(UA_ByteString const *src, size_t *offset, UA_UInt16 * dst) {
    if(*offset + sizeof(UA_UInt16) > (size_t)src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst =  (UA_UInt16)src->data[(*offset)++] << 0;
    *dst |= (UA_UInt16)src->data[(*offset)++] << 8;
    return UA_STATUSCODE_GOOD;
}

/* Int32 */
UA_TYPE_BINARY_ENCODING_AS(UA_Int32, UA_UInt32)

/* UInt32 */
UA_TYPE_CALCSIZEBINARY_MEMSIZE(UA_UInt32)
UA_StatusCode UA_UInt32_encodeBinary(UA_UInt32 const *src, UA_ByteString * dst, size_t *offset) {
    if(*offset + sizeof(UA_UInt32) > (size_t)dst->length )
        return UA_STATUSCODE_BADENCODINGERROR;
    dst->data[(*offset)++] = (UA_Byte)((*src & 0x000000FF) >> 0);
    dst->data[(*offset)++] = (UA_Byte)((*src & 0x0000FF00) >> 8);
    dst->data[(*offset)++] = (UA_Byte)((*src & 0x00FF0000) >> 16);
    dst->data[(*offset)++] = (UA_Byte)((*src & 0xFF000000) >> 24);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_UInt32_decodeBinary(UA_ByteString const *src, size_t *offset, UA_UInt32 * dst) {
    if(*offset + sizeof(UA_UInt32) > (size_t)src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst  = (UA_UInt32)src->data[(*offset)++] << 0;
    *dst |= (UA_UInt32)src->data[(*offset)++] << 8;
    *dst |= (UA_UInt32)src->data[(*offset)++] << 16;
    *dst |= (UA_UInt32)src->data[(*offset)++] << 24;
    return UA_STATUSCODE_GOOD;
}

/* Int64 */
UA_TYPE_BINARY_ENCODING_AS(UA_Int64, UA_UInt64)

/* UInt64 */
UA_TYPE_CALCSIZEBINARY_MEMSIZE(UA_UInt64)
UA_StatusCode UA_UInt64_encodeBinary(UA_UInt64 const *src, UA_ByteString *dst, size_t *offset) {
    if(*offset + sizeof(UA_UInt64) > (size_t)dst->length )
        return UA_STATUSCODE_BADENCODINGERROR;
    dst->data[(*offset)++] = (UA_Byte)((*src & 0x00000000000000FF) >> 0);
    dst->data[(*offset)++] = (UA_Byte)((*src & 0x000000000000FF00) >> 8);
    dst->data[(*offset)++] = (UA_Byte)((*src & 0x0000000000FF0000) >> 16);
    dst->data[(*offset)++] = (UA_Byte)((*src & 0x00000000FF000000) >> 24);
    dst->data[(*offset)++] = (UA_Byte)((*src & 0x000000FF00000000) >> 32);
    dst->data[(*offset)++] = (UA_Byte)((*src & 0x0000FF0000000000) >> 40);
    dst->data[(*offset)++] = (UA_Byte)((*src & 0x00FF000000000000) >> 48);
    dst->data[(*offset)++] = (UA_Byte)((*src & 0xFF00000000000000) >> 56);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_UInt64_decodeBinary(UA_ByteString const *src, size_t *offset, UA_UInt64 * dst) {
    if(*offset + sizeof(UA_UInt64) > (size_t)src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst  = (UA_UInt64)src->data[(*offset)++] << 0;
    *dst |= (UA_UInt64)src->data[(*offset)++] << 8;
    *dst |= (UA_UInt64)src->data[(*offset)++] << 16;
    *dst |= (UA_UInt64)src->data[(*offset)++] << 24;
    *dst |= (UA_UInt64)src->data[(*offset)++] << 32;
    *dst |= (UA_UInt64)src->data[(*offset)++] << 40;
    *dst |= (UA_UInt64)src->data[(*offset)++] << 48;
    *dst |= (UA_UInt64)src->data[(*offset)++] << 56;
    return UA_STATUSCODE_GOOD;
}

/* Float */
UA_TYPE_CALCSIZEBINARY_MEMSIZE(UA_Float)
// FIXME: Implement NaN, Inf and Zero(s)
UA_Byte UA_FLOAT_ZERO[] = { 0x00, 0x00, 0x00, 0x00 };
UA_StatusCode UA_Float_decodeBinary(UA_ByteString const *src, size_t *offset, UA_Float * dst) {
    if(*offset + sizeof(UA_Float) > (size_t)src->length )
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_Float mantissa;
    UA_UInt32 biasedExponent;
    UA_Float sign;
    if(memcmp(&src->data[*offset], UA_FLOAT_ZERO, 4) == 0)
        return UA_Int32_decodeBinary(src, offset, (UA_Int32 *)dst);
    mantissa = (UA_Float)(src->data[*offset] & 0xFF);                                   // bits 0-7
    mantissa = (mantissa / (UA_Float)256.0 ) + (UA_Float)(src->data[*offset+1] & 0xFF); // bits 8-15
    mantissa = (mantissa / (UA_Float)256.0 ) + (UA_Float)(src->data[*offset+2] & 0x7F); // bits 16-22
    biasedExponent  = (src->data[*offset+2] & 0x80) >>  7;                              // bits 23
    biasedExponent |= (UA_UInt32)(src->data[*offset+3] & 0x7F) <<  1;                   // bits 24-30
    sign = ( src->data[*offset+ 3] & 0x80 ) ? -1.0 : 1.0;                               // bit 31
    if(biasedExponent >= 127)
        *dst = (UA_Float)sign * (1ULL << (biasedExponent-127)) * (1.0 + mantissa / 128.0 );
    else
        *dst = (UA_Float)sign * 2.0 * (1.0 + mantissa / 128.0 ) / ((UA_Float)(biasedExponent-127));
    *offset += 4;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Float_encodeBinary(UA_Float const *src, UA_ByteString * dst, size_t *offset) {
    return UA_Int32_encodeBinary((const UA_Int32 *)src, dst, offset);
}

/* Double */
UA_TYPE_CALCSIZEBINARY_MEMSIZE(UA_Double)
// FIXME: Implement NaN, Inf and Zero(s)
UA_Byte UA_DOUBLE_ZERO[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UA_StatusCode UA_Double_decodeBinary(UA_ByteString const *src, size_t *offset, UA_Double * dst) {
#include "stdio.h"
    if(*offset + sizeof(UA_Double) > (UA_UInt32)src->length )
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_Double sign;
    UA_Double mantissa;
    UA_UInt32 biasedExponent;
    if(memcmp(&src->data[*offset], UA_DOUBLE_ZERO, 8) == 0)
        return UA_Int64_decodeBinary(src, offset, (UA_Int64 *)dst);
    mantissa = (UA_Double)(src->data[*offset] & 0xFF);                         // bits 0-7
    mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+1] & 0xFF); // bits 8-15
    mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+2] & 0xFF); // bits 16-23
    mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+3] & 0xFF); // bits 24-31
    mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+4] & 0xFF); // bits 32-39
    mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+5] & 0xFF); // bits 40-47
    mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+6] & 0x0F); // bits 48-51
    biasedExponent  = (src->data[*offset+6] & 0xF0) >>  4; // bits 52-55
    biasedExponent |= ((UA_UInt32)(src->data[*offset+7] & 0x7F)) <<  4; // bits 56-62
    sign = ( src->data[*offset+7] & 0x80 ) ? -1.0 : 1.0; // bit 63
    if(biasedExponent >= 1023)
        *dst = (UA_Double)sign * (1ULL << (biasedExponent-1023)) * (1.0 + mantissa / 8.0 );
    else
        *dst = (UA_Double)sign * 2.0 *
            (1.0 + mantissa / 8.0 ) / ((UA_Double)(biasedExponent-1023));
    *offset += 8;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Double_encodeBinary(UA_Double const *src, UA_ByteString * dst, size_t *offset) {
    return UA_Int64_encodeBinary((const UA_Int64*)src, dst, offset);
}

/* String */
size_t UA_String_calcSizeBinary(UA_String const *string) {
    if(string->length > 0)
        return sizeof(UA_Int32) + (string->length * sizeof(UA_Byte));
    else
        return sizeof(UA_Int32);
}

UA_StatusCode UA_String_encodeBinary(UA_String const *src, UA_ByteString *dst, size_t *offset) {
    if(*offset + UA_String_calcSizeBinary(src) > (size_t)dst->length)
        return UA_STATUSCODE_BADENCODINGERROR;

    UA_StatusCode retval = UA_Int32_encodeBinary(&src->length, dst, offset);
    if(src->length > 0) {
        UA_memcpy(&dst->data[*offset], src->data, src->length);
        *offset += src->length;
    }
    return retval;
}

UA_StatusCode UA_String_decodeBinary(UA_ByteString const *src, size_t *offset, UA_String *dst) {
    UA_String_init(dst);
    UA_Int32 length;
    if(UA_Int32_decodeBinary(src, offset, &length))
        return UA_STATUSCODE_BADINTERNALERROR;
    if(length <= 0) {
        if(length == 0)
            dst->length = 0;
        else
            dst->length = -1;
        return UA_STATUSCODE_GOOD;
    }
        
    if(*offset + (size_t)length > (size_t)src->length)
        return UA_STATUSCODE_BADINTERNALERROR;
    
    if(!(dst->data = UA_malloc(length)))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_memcpy(dst->data, &src->data[*offset], length);
    dst->length = length;
    *offset += length;
    return UA_STATUSCODE_GOOD;
}

/* DateTime */
UA_TYPE_BINARY_ENCODING_AS(UA_DateTime, UA_Int64)

/* Guid */
size_t UA_Guid_calcSizeBinary(UA_Guid const *p) {
    return 16;
}

UA_StatusCode UA_Guid_encodeBinary(UA_Guid const *src, UA_ByteString * dst, size_t *offset) {
    UA_StatusCode retval = UA_UInt32_encodeBinary(&src->data1, dst, offset);
    retval |= UA_UInt16_encodeBinary(&src->data2, dst, offset);
    retval |= UA_UInt16_encodeBinary(&src->data3, dst, offset);
    for(UA_Int32 i = 0;i < 8;i++)
        retval |= UA_Byte_encodeBinary(&src->data4[i], dst, offset);
    return retval;
}

UA_StatusCode UA_Guid_decodeBinary(UA_ByteString const *src, size_t *offset, UA_Guid * dst) {
    // This could be done with a single memcpy (if the compiler does no fancy realigning of structs)
    UA_StatusCode retval = UA_UInt32_decodeBinary(src, offset, &dst->data1);
    retval |= UA_UInt16_decodeBinary(src, offset, &dst->data2);
    retval |= UA_UInt16_decodeBinary(src, offset, &dst->data3);
    for(size_t i = 0;i < 8;i++)
        retval |= UA_Byte_decodeBinary(src, offset, &dst->data4[i]);
    if(retval)
        UA_Guid_deleteMembers(dst);
    return retval;
}

/* ByteString */
UA_TYPE_BINARY_ENCODING_AS(UA_ByteString, UA_String)

/* XmlElement */
UA_TYPE_BINARY_ENCODING_AS(UA_XmlElement, UA_String)

/* NodeId */

/* The shortened numeric nodeid types. */
#define UA_NODEIDTYPE_TWOBYTE 0
#define UA_NODEIDTYPE_FOURBYTE 1

size_t UA_NodeId_calcSizeBinary(UA_NodeId const *p) {
    size_t length = 0;
    switch(p->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        if(p->identifier.numeric > UA_UINT16_MAX || p->namespaceIndex > UA_BYTE_MAX)
            length = sizeof(UA_Byte) + sizeof(UA_UInt16) + sizeof(UA_UInt32);
        else if(p->identifier.numeric > UA_BYTE_MAX || p->namespaceIndex > 0)
            length = 4;  /* UA_NODEIDTYPE_FOURBYTE */
        else
            length = 2;  /* UA_NODEIDTYPE_TWOBYTE*/
        break;
    case UA_NODEIDTYPE_STRING:
        length = sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_String_calcSizeBinary(&p->identifier.string);
        break;
    case UA_NODEIDTYPE_GUID:
        length = sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_Guid_calcSizeBinary(&p->identifier.guid);
        break;
    case UA_NODEIDTYPE_BYTESTRING:
        length = sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_ByteString_calcSizeBinary(&p->identifier.byteString);
        break;
    default:
        UA_assert(UA_FALSE); // this must never happen
        break;
    }
    return length;
}

UA_StatusCode UA_NodeId_encodeBinary(UA_NodeId const *src, UA_ByteString * dst, size_t *offset) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    // temporary variables for endian-save code
    UA_Byte srcByte;
    UA_UInt16 srcUInt16;
    switch(src->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        if(src->identifier.numeric > UA_UINT16_MAX || src->namespaceIndex > UA_BYTE_MAX) {
            srcByte = UA_NODEIDTYPE_NUMERIC;
            retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
            retval |= UA_UInt16_encodeBinary(&src->namespaceIndex, dst, offset);
            retval |= UA_UInt32_encodeBinary(&src->identifier.numeric, dst, offset);
        } else if(src->identifier.numeric > UA_BYTE_MAX || src->namespaceIndex > 0) { /* UA_NODEIDTYPE_FOURBYTE */
            srcByte = UA_NODEIDTYPE_FOURBYTE;
            retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
            srcByte = src->namespaceIndex;
            srcUInt16 = src->identifier.numeric;
            retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
            retval |= UA_UInt16_encodeBinary(&srcUInt16, dst, offset);
        } else { /* UA_NODEIDTYPE_TWOBYTE */
            srcByte = UA_NODEIDTYPE_TWOBYTE;
            retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
            srcByte = src->identifier.numeric;
            retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
        }
        break;
    case UA_NODEIDTYPE_STRING:
        srcByte = UA_NODEIDTYPE_STRING;
        retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
        retval |= UA_UInt16_encodeBinary(&src->namespaceIndex, dst, offset);
        retval |= UA_String_encodeBinary(&src->identifier.string, dst, offset);
        break;
    case UA_NODEIDTYPE_GUID:
        srcByte = UA_NODEIDTYPE_GUID;
        retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
        retval |= UA_UInt16_encodeBinary(&src->namespaceIndex, dst, offset);
        retval |= UA_Guid_encodeBinary(&src->identifier.guid, dst, offset);
        break;
    case UA_NODEIDTYPE_BYTESTRING:
        srcByte = UA_NODEIDTYPE_BYTESTRING;
        retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
        retval |= UA_UInt16_encodeBinary(&src->namespaceIndex, dst, offset);
        retval |= UA_ByteString_encodeBinary(&src->identifier.byteString, dst, offset);
        break;
    default:
        UA_assert(UA_FALSE);
    }
    return retval;
}

UA_StatusCode UA_NodeId_decodeBinary(UA_ByteString const *src, size_t *offset, UA_NodeId *dst) {
    // temporary variables to overcome decoder's non-endian-saveness for datatypes with different length
    UA_Byte   dstByte = 0;
    UA_UInt16 dstUInt16 = 0;
    UA_Byte   encodingByte = 0;

    UA_StatusCode retval = UA_Byte_decodeBinary(src, offset, &encodingByte);
    if(retval) {
        UA_NodeId_init(dst);
        return retval;
    }
    
    switch(encodingByte) {
    case UA_NODEIDTYPE_TWOBYTE: // Table 7
        dst->identifierType     = UA_NODEIDTYPE_NUMERIC;
        retval = UA_Byte_decodeBinary(src, offset, &dstByte);
        dst->identifier.numeric = dstByte;
        dst->namespaceIndex     = 0; // default namespace
        break;
    case UA_NODEIDTYPE_FOURBYTE: // Table 8
        dst->identifierType     = UA_NODEIDTYPE_NUMERIC;
        retval |= UA_Byte_decodeBinary(src, offset, &dstByte);
        dst->namespaceIndex     = dstByte;
        retval |= UA_UInt16_decodeBinary(src, offset, &dstUInt16);
        dst->identifier.numeric = dstUInt16;
        break;
    case UA_NODEIDTYPE_NUMERIC: // Table 6, first entry
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        retval |= UA_UInt16_decodeBinary(src, offset, &dst->namespaceIndex);
        retval |= UA_UInt32_decodeBinary(src, offset, &dst->identifier.numeric);
        break;
    case UA_NODEIDTYPE_STRING: // Table 6, second entry
        dst->identifierType = UA_NODEIDTYPE_STRING;
        retval |= UA_UInt16_decodeBinary(src, offset, &dst->namespaceIndex);
        retval |= UA_String_decodeBinary(src, offset, &dst->identifier.string);
        break;
    case UA_NODEIDTYPE_GUID: // Table 6, third entry
        dst->identifierType = UA_NODEIDTYPE_GUID;
        retval |= UA_UInt16_decodeBinary(src, offset, &dst->namespaceIndex);
        retval |= UA_Guid_decodeBinary(src, offset, &dst->identifier.guid);
        break;
    case UA_NODEIDTYPE_BYTESTRING: // Table 6, "OPAQUE"
        dst->identifierType = UA_NODEIDTYPE_BYTESTRING;
        retval |= UA_UInt16_decodeBinary(src, offset, &dst->namespaceIndex);
        retval |= UA_ByteString_decodeBinary(src, offset, &dst->identifier.byteString);
        break;
    default:
        UA_NodeId_init(dst);
        retval |= UA_STATUSCODE_BADINTERNALERROR; // the client sends an encodingByte we do not recognize
        break;
    }
    if(retval)
        UA_NodeId_deleteMembers(dst);
    return retval;
}

/* ExpandedNodeId */
size_t UA_ExpandedNodeId_calcSizeBinary(UA_ExpandedNodeId const *p) {
    size_t length = UA_NodeId_calcSizeBinary(&p->nodeId);
    if(p->namespaceUri.length > 0)
        length += UA_String_calcSizeBinary(&p->namespaceUri);
    if(p->serverIndex > 0)
        length += sizeof(UA_UInt32);
    return length;
}

#define UA_EXPANDEDNODEID_NAMESPACEURI_FLAG 0x80
#define UA_EXPANDEDNODEID_SERVERINDEX_FLAG 0x40

UA_StatusCode UA_ExpandedNodeId_encodeBinary(UA_ExpandedNodeId const *src, UA_ByteString * dst, size_t *offset) {
    UA_Byte flags = 0;
    UA_UInt32 start = *offset;
    UA_StatusCode retval = UA_NodeId_encodeBinary(&src->nodeId, dst, offset);
    if(src->namespaceUri.length > 0) {
        // TODO: Set namespaceIndex to 0 in the nodeid as the namespaceUri takes precedence
        retval |= UA_String_encodeBinary(&src->namespaceUri, dst, offset);
        flags |= UA_EXPANDEDNODEID_NAMESPACEURI_FLAG;
    }
    if(src->serverIndex > 0) {
        retval |= UA_UInt32_encodeBinary(&src->serverIndex, dst, offset);
        flags |= UA_EXPANDEDNODEID_SERVERINDEX_FLAG;
    }
    if(flags != 0)
        dst->data[start] |= flags;
    return retval;
}

UA_StatusCode UA_ExpandedNodeId_decodeBinary(UA_ByteString const *src, size_t *offset, UA_ExpandedNodeId *dst) {
    UA_ExpandedNodeId_init(dst);
    // get encodingflags and leave a "clean" nodeidtype
    if(*offset >= (size_t)src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_Byte encodingByte = src->data[*offset];
    src->data[*offset] = encodingByte & ~(UA_EXPANDEDNODEID_NAMESPACEURI_FLAG | UA_EXPANDEDNODEID_SERVERINDEX_FLAG);

    UA_StatusCode retval = UA_NodeId_decodeBinary(src, offset, &dst->nodeId);
    if(encodingByte & UA_EXPANDEDNODEID_NAMESPACEURI_FLAG) {
        dst->nodeId.namespaceIndex = 0;
        retval |= UA_String_decodeBinary(src, offset, &dst->namespaceUri);
    }
    if(encodingByte & UA_EXPANDEDNODEID_SERVERINDEX_FLAG)
        retval |= UA_UInt32_decodeBinary(src, offset, &dst->serverIndex);
    if(retval)
        UA_ExpandedNodeId_deleteMembers(dst);
    return retval;
}

/* StatusCode */
UA_TYPE_BINARY_ENCODING_AS(UA_StatusCode, UA_UInt32)

/* QualifiedName */
size_t UA_QualifiedName_calcSizeBinary(UA_QualifiedName const *p) {
    size_t length = sizeof(UA_UInt16); //qualifiedName->namespaceIndex
    // length += sizeof(UA_UInt16); //qualifiedName->reserved
    length += UA_String_calcSizeBinary(&p->name); //qualifiedName->name
    return length;
}

UA_StatusCode UA_QualifiedName_decodeBinary(UA_ByteString const *src, size_t *offset, UA_QualifiedName *dst) {
    UA_QualifiedName_init(dst);
    UA_StatusCode retval = UA_UInt16_decodeBinary(src, offset, &dst->namespaceIndex);
    retval |= UA_String_decodeBinary(src, offset, &dst->name);
    if(retval)
        UA_QualifiedName_deleteMembers(dst);
    return retval;
}

UA_StatusCode UA_QualifiedName_encodeBinary(UA_QualifiedName const *src, UA_ByteString* dst, size_t *offset) {
    UA_StatusCode retval = UA_UInt16_encodeBinary(&src->namespaceIndex, dst, offset);
    retval |= UA_String_encodeBinary(&src->name, dst, offset);
    return retval;
}

/* LocalizedText */
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE 0x01
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT 0x02

size_t UA_LocalizedText_calcSizeBinary(UA_LocalizedText const *p) {
    size_t length = 1; // for encodingMask
    if(p->locale.data != UA_NULL)
        length += UA_String_calcSizeBinary(&p->locale);
    if(p->text.data != UA_NULL)
        length += UA_String_calcSizeBinary(&p->text);
    return length;
}

UA_StatusCode UA_LocalizedText_encodeBinary(UA_LocalizedText const *src, UA_ByteString * dst, size_t *offset) {
    UA_Byte encodingMask = 0;
    if(src->locale.data != UA_NULL)
        encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE;
    if(src->text.data != UA_NULL)
        encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
    UA_StatusCode retval = UA_Byte_encodeBinary(&encodingMask, dst, offset);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
        retval |= UA_String_encodeBinary(&src->locale, dst, offset);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
        retval |= UA_String_encodeBinary(&src->text, dst, offset);
    return retval;
}

UA_StatusCode UA_LocalizedText_decodeBinary(UA_ByteString const *src, size_t *offset, UA_LocalizedText *dst) {
    UA_LocalizedText_init(dst);
    UA_Byte encodingMask = 0;
    UA_StatusCode retval = UA_Byte_decodeBinary(src, offset, &encodingMask);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
        retval |= UA_String_decodeBinary(src, offset, &dst->locale);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
        retval |= UA_String_decodeBinary(src, offset, &dst->text);
    if(retval)
        UA_LocalizedText_deleteMembers(dst);
    return retval;
}

/* ExtensionObject */
size_t UA_ExtensionObject_calcSizeBinary(UA_ExtensionObject const *p) {
    size_t length = UA_NodeId_calcSizeBinary(&p->typeId);
    length += 1; // encoding
    switch(p->encoding) {
    case UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED:
        break;
    case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING:
        length += UA_ByteString_calcSizeBinary(&p->body);
        break;
    case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML:
        length += UA_XmlElement_calcSizeBinary((const UA_XmlElement *)&p->body);
        break;
    default:
        UA_assert(UA_FALSE);
    }
    return length;
}

UA_StatusCode UA_ExtensionObject_encodeBinary(UA_ExtensionObject const *src, UA_ByteString * dst, size_t *offset) {
    UA_StatusCode retval = UA_NodeId_encodeBinary(&src->typeId, dst, offset);
    retval |= UA_Byte_encodeBinary((const UA_Byte*)&src->encoding, dst, offset);
    switch(src->encoding) {
    case UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED:
        break;
    case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING:
        retval |= UA_ByteString_encodeBinary(&src->body, dst, offset);
        break;
    case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML:
        retval |= UA_ByteString_encodeBinary(&src->body, dst, offset);
        break;
    default:
        UA_assert(UA_FALSE);
    }
    return retval;
}

UA_StatusCode UA_ExtensionObject_decodeBinary(UA_ByteString const *src, size_t *offset, UA_ExtensionObject *dst) {
    UA_ExtensionObject_init(dst);
    UA_Byte encoding = 0;
    UA_StatusCode retval = UA_NodeId_decodeBinary(src, offset, &dst->typeId);
    retval |= UA_Byte_decodeBinary(src, offset, &encoding);
    dst->encoding = encoding;
    retval |= UA_String_copy(&UA_STRING_NULL, (UA_String *)&dst->body);
    if(retval) {
        UA_ExtensionObject_init(dst);
        return retval;
    }
    switch(dst->encoding) {
    case UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED:
        break;
    case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING:
    case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML:
        retval |= UA_ByteString_decodeBinary(src, offset, &dst->body);
        break;
    default:
        UA_ExtensionObject_deleteMembers(dst);
        return UA_STATUSCODE_BADDECODINGERROR;
    }
    if(retval)
        UA_ExtensionObject_deleteMembers(dst);
    return retval;
}

/* DataValue */
size_t UA_DataValue_calcSizeBinary(UA_DataValue const *p) {
    size_t length = sizeof(UA_Byte);
    if(p->hasVariant)
        length += UA_Variant_calcSizeBinary(&p->value);
    if(p->hasStatus)
        length += sizeof(UA_UInt32);
    if(p->hasSourceTimestamp)
        length += sizeof(UA_DateTime);
    if(p->hasSourcePicoseconds)
        length += sizeof(UA_Int16);
    if(p->hasServerTimestamp)
        length += sizeof(UA_DateTime);
    if(p->hasServerPicoseconds)
        length += sizeof(UA_Int16);
    return length;
}

UA_StatusCode UA_DataValue_encodeBinary(UA_DataValue const *src, UA_ByteString *dst, size_t *offset) {
    UA_StatusCode retval = UA_Byte_encodeBinary((const UA_Byte*)src, dst, offset);
    if(src->hasVariant)
        retval |= UA_Variant_encodeBinary(&src->value, dst, offset);
    if(src->hasStatus)
        retval |= UA_StatusCode_encodeBinary(&src->status, dst, offset);
    if(src->hasSourceTimestamp)
        retval |= UA_DateTime_encodeBinary(&src->sourceTimestamp, dst, offset);
    if(src->hasSourcePicoseconds)
        retval |= UA_Int16_encodeBinary(&src->sourcePicoseconds, dst, offset);
    if(src->hasServerTimestamp)
        retval |= UA_DateTime_encodeBinary(&src->serverTimestamp, dst, offset);
    if(src->hasServerPicoseconds)
        retval |= UA_Int16_encodeBinary(&src->serverPicoseconds, dst, offset);
    return retval;
}

#define MAX_PICO_SECONDS 1000
UA_StatusCode UA_DataValue_decodeBinary(UA_ByteString const *src, size_t *offset, UA_DataValue *dst) {
    UA_DataValue_init(dst);
    UA_StatusCode retval = UA_Byte_decodeBinary(src, offset, (UA_Byte*)dst);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(dst->hasVariant)
        retval |= UA_Variant_decodeBinary(src, offset, &dst->value);
    if(dst->hasStatus)
        retval |= UA_StatusCode_decodeBinary(src, offset, &dst->status);
    if(dst->hasSourceTimestamp)
        retval |= UA_DateTime_decodeBinary(src, offset, &dst->sourceTimestamp);
    if(dst->hasSourcePicoseconds) {
        retval |= UA_Int16_decodeBinary(src, offset, &dst->sourcePicoseconds);
        if(dst->sourcePicoseconds > MAX_PICO_SECONDS)
            dst->sourcePicoseconds = MAX_PICO_SECONDS;
    }
    if(dst->hasServerTimestamp)
        retval |= UA_DateTime_decodeBinary(src, offset, &dst->serverTimestamp);
    if(dst->hasServerPicoseconds) {
        retval |= UA_Int16_decodeBinary(src, offset, &dst->serverPicoseconds);
        if(dst->serverPicoseconds > MAX_PICO_SECONDS)
            dst->serverPicoseconds = MAX_PICO_SECONDS;
    }
    if(retval)
        UA_DataValue_deleteMembers(dst);
    return retval;
}

/* Variant */
/* We can store all data types in a variant internally. But for communication we
 * encode them in an ExtensionObject if they are not one of the built in types.
 * Officially, only builtin types are contained in a variant.
 *
 * Every ExtensionObject incurrs an overhead of 4 byte (nodeid) + 1 byte (encoding) */

/* VariantBinaryEncoding - Part: 6, Chapter: 5.2.2.16, Page: 22 */
enum UA_VARIANT_ENCODINGMASKTYPE_enum {
    UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK = 0x3F,            // bits 0:5
    UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS  = (0x01 << 6),     // bit 6
    UA_VARIANT_ENCODINGMASKTYPE_ARRAY       = (0x01 << 7)      // bit 7
};

size_t UA_Variant_calcSizeBinary(UA_Variant const *p) {
    if(!p->type) // type is not set after init
        return 0;
    size_t length = sizeof(UA_Byte); //p->encodingMask
    UA_Int32 arrayLength = p->arrayLength;
    if(arrayLength <= 0 || p->dataPtr == UA_NULL) {
        length += 4;
        arrayLength = 0; // for adding the extensionobject overhead...
    }
    else if(arrayLength == 1)
        length += UA_calcSizeBinary(p->dataPtr, p->type);
    else
        length += UA_Array_calcSizeBinary(p->dataPtr, arrayLength, p->type);
        
    // if the type is not builtin, we encode it as an extensionobject
    if(p->type->typeIndex > 24 || !p->type->namespaceZero)
        length += 9 * arrayLength;  // overhead for extensionobjects: 4 byte nodeid + 1 byte
                                    // encoding + 4 byte bytestring length
    if(arrayLength != 1 && p->arrayDimensions != UA_NULL)
        length += UA_Array_calcSizeBinary(p->arrayDimensions, p->arrayDimensionsSize,
                                          &UA_TYPES[UA_TYPES_INT32]);
    return length;
}

UA_StatusCode UA_Variant_encodeBinary(UA_Variant const *src, UA_ByteString *dst, size_t *offset) {
    if(!src->type)
        return UA_STATUSCODE_BADINTERNALERROR;
    
    UA_Byte encodingByte = 0;
    UA_Boolean isArray = src->arrayLength != 1;  // a single element is not an array
    UA_Boolean hasDimensions = isArray && src->arrayDimensions != UA_NULL;
    UA_Boolean isBuiltin = (src->type->namespaceZero && src->type->typeIndex <= 24);

    if(isArray) {
        encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
        if(hasDimensions)
            encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS;
    }

    if(isBuiltin)
        encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK & (UA_Byte)src->type->typeId.identifier.numeric;
    else
        encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK & (UA_Byte)22;  // ExtensionObject

    UA_StatusCode retval = UA_Byte_encodeBinary(&encodingByte, dst, offset);

    if(isArray)
        retval |= UA_Array_encodeBinary(src->dataPtr, src->arrayLength, src->type, dst, offset);
    else if(!src->dataPtr)
        retval = UA_STATUSCODE_BADENCODINGERROR; // an array can be empty. a single element must be present.
    else {
        if(!isBuiltin) {
            // print the extensionobject header
        	if(src->type->isStructure == UA_TRUE){ //increase id by UA_ENCODINGOFFSET_BINARY for binary encoding of struct obejcts
        		UA_NodeId copy;
        		UA_NodeId_copy(&src->type->typeId, &copy);
        		copy.identifier.numeric=copy.identifier.numeric + UA_ENCODINGOFFSET_BINARY;
        		UA_NodeId_encodeBinary(&copy, dst, offset);
        	} else {
        		UA_NodeId_encodeBinary(&src->type->typeId, dst, offset);
        	}
            UA_Byte eoEncoding = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING;
            UA_Byte_encodeBinary(&eoEncoding, dst, offset);
            UA_Int32 eoEncodingLength = UA_calcSizeBinary(src->dataPtr, src->type);
            UA_Int32_encodeBinary(&eoEncodingLength, dst, offset);
        }
        retval |= UA_encodeBinary(src->dataPtr, src->type, dst, offset);
    }

    if(hasDimensions)
        retval |= UA_Array_encodeBinary(src->arrayDimensions, src->arrayDimensionsSize,
                                        &UA_TYPES[UA_TYPES_INT32], dst, offset);
    return retval;
}

/* The resulting variant always has the storagetype UA_VARIANT_DATA. */
UA_StatusCode UA_Variant_decodeBinary(UA_ByteString const *src, size_t *offset, UA_Variant *dst) {
    UA_Variant_init(dst);
    UA_Byte encodingByte;
    UA_StatusCode retval = UA_Byte_decodeBinary(src, offset, &encodingByte);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_Boolean isArray = encodingByte & UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
    UA_Boolean hasDimensions = isArray && (encodingByte & UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS);
    UA_NodeId typeid = { .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
                         .identifier.numeric = encodingByte & UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK };

    UA_UInt16 typeIndex;
    for(typeIndex = 0; typeIndex < UA_TYPES_COUNT; typeIndex++) {
        if(UA_TYPES[typeIndex].typeId.identifier.numeric == typeid.identifier.numeric)
            break;
    }

    if(typeIndex >= UA_TYPES_COUNT)
        return UA_STATUSCODE_BADDECODINGERROR;

    const UA_DataType *dataType = &UA_TYPES[typeIndex];

    if(!isArray) {
        if(!(dst->dataPtr = UA_malloc(dataType->memSize)))
            return UA_STATUSCODE_BADOUTOFMEMORY;
        retval |= UA_decodeBinary(src, offset, dst->dataPtr, dataType);
        if(retval) {
            UA_free(dst->dataPtr);
            return retval;
        }
        dst->arrayLength = 1;
    } else {
        retval |= UA_Int32_decodeBinary(src, offset, &dst->arrayLength);
        if(retval == UA_STATUSCODE_GOOD)
            retval |= UA_Array_decodeBinary(src, offset, dst->arrayLength, &dst->dataPtr, dataType);
        if(retval)
            dst->arrayLength = -1; // for deleteMembers
    }

    if(hasDimensions && retval == UA_STATUSCODE_GOOD) {
        retval |= UA_Int32_decodeBinary(src, offset, &dst->arrayDimensionsSize);
        if(retval == UA_STATUSCODE_GOOD)
            retval |= UA_Array_decodeBinary(src, offset, dst->arrayDimensionsSize,
                                            &dst->dataPtr, &UA_TYPES[UA_TYPES_INT32]);
        if(retval)
            dst->arrayLength = -1; // for deleteMembers
    }

    dst->type = dataType;

    if(retval)
        UA_Variant_deleteMembers(dst);
    return retval;
}

/* DiagnosticInfo */
size_t UA_DiagnosticInfo_calcSizeBinary(UA_DiagnosticInfo const *ptr) {
    size_t length = sizeof(UA_Byte);
    if(ptr->hasSymbolicId)
        length += sizeof(UA_Int32);
    if(ptr->hasNamespaceUri)
        length += sizeof(UA_Int32);
    if(ptr->hasLocalizedText)
        length += sizeof(UA_Int32);
    if(ptr->hasLocale)
        length += sizeof(UA_Int32);
    if(ptr->hasAdditionalInfo)
        length += UA_String_calcSizeBinary(&ptr->additionalInfo);
    if(ptr->hasInnerStatusCode)
        length += sizeof(UA_StatusCode);
    if(ptr->hasInnerDiagnosticInfo)
        length += UA_DiagnosticInfo_calcSizeBinary(ptr->innerDiagnosticInfo);
    return length;
}

UA_StatusCode UA_DiagnosticInfo_encodeBinary(const UA_DiagnosticInfo *src, UA_ByteString * dst, size_t *offset) {
    UA_StatusCode retval = UA_Byte_encodeBinary((const UA_Byte *)src, dst, offset);
    if(src->hasSymbolicId)
        retval |= UA_Int32_encodeBinary(&src->symbolicId, dst, offset);
    if(src->hasNamespaceUri)
        retval |= UA_Int32_encodeBinary( &src->namespaceUri, dst, offset);
    if(src->hasLocalizedText)
        retval |= UA_Int32_encodeBinary(&src->localizedText, dst, offset);
    if(src->hasLocale)
        retval |= UA_Int32_encodeBinary(&src->locale, dst, offset);
    if(src->hasAdditionalInfo)
        retval |= UA_String_encodeBinary(&src->additionalInfo, dst, offset);
    if(src->hasInnerStatusCode)
        retval |= UA_StatusCode_encodeBinary(&src->innerStatusCode, dst, offset);
    if(src->hasInnerDiagnosticInfo)
        retval |= UA_DiagnosticInfo_encodeBinary(src->innerDiagnosticInfo, dst, offset);
    return retval;
}

UA_StatusCode UA_DiagnosticInfo_decodeBinary(UA_ByteString const *src, size_t *offset, UA_DiagnosticInfo *dst) {
    UA_DiagnosticInfo_init(dst);
    UA_StatusCode retval = UA_Byte_decodeBinary(src, offset, (UA_Byte*)dst);
    if(!retval)
        return retval;
    if(dst->hasSymbolicId)
        retval |= UA_Int32_decodeBinary(src, offset, &dst->symbolicId);
    if(dst->hasNamespaceUri)
        retval |= UA_Int32_decodeBinary(src, offset, &dst->namespaceUri);
    if(dst->hasLocalizedText)
        retval |= UA_Int32_decodeBinary(src, offset, &dst->localizedText);
    if(dst->hasLocale)
        retval |= UA_Int32_decodeBinary(src, offset, &dst->locale);
    if(dst->hasAdditionalInfo)
        retval |= UA_String_decodeBinary(src, offset, &dst->additionalInfo);
    if(dst->hasInnerStatusCode)
        retval |= UA_StatusCode_decodeBinary(src, offset, &dst->innerStatusCode);
    if(dst->hasInnerDiagnosticInfo) {
        // innerDiagnosticInfo is a pointer to struct, therefore allocate
        if((dst->innerDiagnosticInfo = UA_malloc(sizeof(UA_DiagnosticInfo)))) {
            if(UA_DiagnosticInfo_decodeBinary(src, offset, dst->innerDiagnosticInfo) != UA_STATUSCODE_GOOD) {
                UA_free(dst->innerDiagnosticInfo);
                dst->innerDiagnosticInfo = UA_NULL;
                retval |= UA_STATUSCODE_BADINTERNALERROR;
            }
        } else {
            retval |= UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_DiagnosticInfo_deleteMembers(dst);
    return retval;
}

/********************/
/* Structured Types */
/********************/

size_t UA_calcSizeBinary(const void *p, const UA_DataType *dataType) {
    size_t size = 0;
    uintptr_t ptr = (uintptr_t)p;
    UA_Byte membersSize = dataType->membersSize;
    for(size_t i=0;i<membersSize; i++) {
        const UA_DataTypeMember *member = &dataType->members[i];
        const UA_DataType *memberType;
        if(member->namespaceZero)
            memberType = &UA_TYPES[member->memberTypeIndex];
        else
            memberType = dataType - dataType->typeIndex + member->memberTypeIndex;

        if(member->isArray) {
            ptr += (member->padding >> 3);
            const UA_Int32 noElements = *((const UA_Int32*)ptr);
            ptr += sizeof(UA_Int32) + (member->padding & 0x07);
            size += UA_Array_calcSizeBinary(*(void * const *)ptr, noElements, memberType);
            ptr += sizeof(void*);
            continue;
        }

        ptr += member->padding;
        if(!member->namespaceZero) {
            size += UA_calcSizeBinary((const void*)ptr, memberType);
            ptr += memberType->memSize;
            continue;
        }

        switch(member->memberTypeIndex) {
        case UA_TYPES_BOOLEAN:
        case UA_TYPES_SBYTE:
        case UA_TYPES_BYTE:
            size += 1;
            break;
        case UA_TYPES_INT16:
        case UA_TYPES_UINT16:
            size += 2;
            break;
        case UA_TYPES_INT32:
        case UA_TYPES_UINT32:
        case UA_TYPES_STATUSCODE:
        case UA_TYPES_FLOAT:
            size += 4;
            break;
        case UA_TYPES_INT64:
        case UA_TYPES_UINT64:
        case UA_TYPES_DOUBLE:
        case UA_TYPES_DATETIME:
            size += 8;
            break;
        case UA_TYPES_GUID:
            size += 16;
            break;
        case UA_TYPES_NODEID:
            size += UA_NodeId_calcSizeBinary((const UA_NodeId*)ptr);
            break;
        case UA_TYPES_EXPANDEDNODEID:
            size += UA_ExpandedNodeId_calcSizeBinary((const UA_ExpandedNodeId*)ptr);
            break;
        case UA_TYPES_QUALIFIEDNAME:
            size += UA_QualifiedName_calcSizeBinary((const UA_QualifiedName*)ptr);
            break;
        case UA_TYPES_LOCALIZEDTEXT:
            size += UA_LocalizedText_calcSizeBinary((const UA_LocalizedText*)ptr);
            break;
        case UA_TYPES_EXTENSIONOBJECT:
            size += UA_ExtensionObject_calcSizeBinary((const UA_ExtensionObject*)ptr);
            break;
        case UA_TYPES_DATAVALUE:
            size += UA_DataValue_calcSizeBinary((const UA_DataValue*)ptr);
            break;
        case UA_TYPES_VARIANT:
            size += UA_Variant_calcSizeBinary((const UA_Variant*)ptr);
            break;
        case UA_TYPES_DIAGNOSTICINFO:
            size += UA_DiagnosticInfo_calcSizeBinary((const UA_DiagnosticInfo*)ptr);
            break;
        case UA_TYPES_STRING:
        case UA_TYPES_BYTESTRING:
        case UA_TYPES_XMLELEMENT:
            size += UA_String_calcSizeBinary((const UA_String*)ptr);
            break;
        default:
            size += UA_calcSizeBinary((const void*)ptr, memberType);
        }
        ptr += memberType->memSize;
    }
    return size;
}

UA_StatusCode UA_encodeBinary(const void *src, const UA_DataType *dataType, UA_ByteString *dst, size_t *offset) {
    uintptr_t ptr = (uintptr_t)src;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Byte membersSize = dataType->membersSize;
    for(size_t i=0;i<membersSize && retval == UA_STATUSCODE_GOOD; i++) {
        const UA_DataTypeMember *member = &dataType->members[i];
        const UA_DataType *memberType;
        if(member->namespaceZero)
            memberType = &UA_TYPES[member->memberTypeIndex];
        else
            memberType = dataType - dataType->typeIndex + member->memberTypeIndex;

        if(member->isArray) {
            ptr += (member->padding >> 3);
            const UA_Int32 noElements = *((const UA_Int32*)ptr);
            ptr += sizeof(UA_Int32) + (member->padding & 0x07);
            retval = UA_Array_encodeBinary(*(void * const *)ptr, noElements, memberType, dst, offset);
            ptr += sizeof(void*);
            continue;
        }

        ptr += member->padding;
        if(!member->namespaceZero) {
            UA_encodeBinary((const void*)ptr, memberType, dst, offset);
            ptr += memberType->memSize;
            continue;
        }
        
        switch(member->memberTypeIndex) {
        case UA_TYPES_BOOLEAN:
        case UA_TYPES_SBYTE:
        case UA_TYPES_BYTE:
            retval = UA_Byte_encodeBinary((const UA_Byte*)ptr, dst, offset);
            break;
        case UA_TYPES_INT16:
            retval = UA_Int16_encodeBinary((const UA_Int16*)ptr, dst, offset);
            break;
        case UA_TYPES_UINT16:
            retval = UA_UInt16_encodeBinary((const UA_UInt16*)ptr, dst, offset);
            break;
        case UA_TYPES_INT32:
        case UA_TYPES_UINT32:
        case UA_TYPES_STATUSCODE:
            retval = UA_Int32_encodeBinary((const UA_Int32*)ptr, dst, offset);
            break;
        case UA_TYPES_FLOAT:
            retval = UA_Float_encodeBinary((const UA_Float*)ptr, dst, offset);
            break;
        case UA_TYPES_INT64:
        case UA_TYPES_UINT64:
        case UA_TYPES_DATETIME:
            retval = UA_Int64_encodeBinary((const UA_Int64*)ptr, dst, offset);
            break;
        case UA_TYPES_DOUBLE:
            retval = UA_Double_encodeBinary((const UA_Double*)ptr, dst, offset);
            break;
        case UA_TYPES_GUID:
            retval = UA_Guid_encodeBinary((const UA_Guid*)ptr, dst, offset);
            break;
        case UA_TYPES_NODEID:
            retval = UA_NodeId_encodeBinary((const UA_NodeId*)ptr, dst, offset);
            break;
        case UA_TYPES_EXPANDEDNODEID:
            retval = UA_ExpandedNodeId_encodeBinary((const UA_ExpandedNodeId*)ptr, dst, offset);
            break;
        case UA_TYPES_QUALIFIEDNAME:
            retval = UA_QualifiedName_encodeBinary((const UA_QualifiedName*)ptr, dst, offset);
            break;
        case UA_TYPES_LOCALIZEDTEXT:
            retval = UA_LocalizedText_encodeBinary((const UA_LocalizedText*)ptr, dst, offset);
            break;
        case UA_TYPES_EXTENSIONOBJECT:
            retval = UA_ExtensionObject_encodeBinary((const UA_ExtensionObject*)ptr, dst, offset);
            break;
        case UA_TYPES_DATAVALUE:
            retval = UA_DataValue_encodeBinary((const UA_DataValue*)ptr, dst, offset);
            break;
        case UA_TYPES_VARIANT:
            retval = UA_Variant_encodeBinary((const UA_Variant*)ptr, dst, offset);
            break;
        case UA_TYPES_DIAGNOSTICINFO:
            retval = UA_DiagnosticInfo_encodeBinary((const UA_DiagnosticInfo*)ptr, dst, offset);
            break;
        case UA_TYPES_STRING:
        case UA_TYPES_BYTESTRING:
        case UA_TYPES_XMLELEMENT:
            retval = UA_String_encodeBinary((const UA_String*)ptr, dst, offset);
            break;
        default:
            retval = UA_encodeBinary((const void*)ptr, memberType, dst, offset);
        }
        ptr += memberType->memSize;
    }
    return retval;
}

UA_StatusCode UA_decodeBinary(const UA_ByteString *src, size_t *offset, void *dst, const UA_DataType *dataType) {
    UA_init(dst, dataType);
    uintptr_t ptr = (uintptr_t)dst;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Byte membersSize = dataType->membersSize;
    for(size_t i=0;i<membersSize && retval == UA_STATUSCODE_GOOD; i++) {
        const UA_DataTypeMember *member = &dataType->members[i];
        const UA_DataType *memberType;
        if(member->namespaceZero)
            memberType = &UA_TYPES[member->memberTypeIndex];
        else
            memberType = dataType - dataType->typeIndex + member->memberTypeIndex;

        if(member->isArray) {
            ptr += (member->padding >> 3);
            UA_Int32 *noElements = (UA_Int32*)ptr;
            UA_Int32_decodeBinary(src, offset, noElements);
            ptr += sizeof(UA_Int32) + (member->padding & 0x07);
            retval = UA_Array_decodeBinary(src, offset, *noElements, (void**)ptr, memberType);
            ptr += sizeof(void*);
            continue;
        }

        ptr += member->padding;
        if(!member->namespaceZero) {
            UA_decodeBinary(src, offset, (void*)ptr, memberType);
            ptr += memberType->memSize;
            continue;
        }

        switch(member->memberTypeIndex) {
        case UA_TYPES_BOOLEAN:
        case UA_TYPES_SBYTE:
        case UA_TYPES_BYTE:
            retval = UA_Byte_decodeBinary(src, offset, (UA_Byte*)ptr);
            break;
        case UA_TYPES_INT16:
        case UA_TYPES_UINT16:
            retval = UA_Int16_decodeBinary(src, offset, (UA_Int16*)ptr);
            break;
        case UA_TYPES_INT32:
        case UA_TYPES_UINT32:
        case UA_TYPES_STATUSCODE:
            retval = UA_Int32_decodeBinary(src, offset, (UA_Int32*)ptr);
            break;
        case UA_TYPES_FLOAT:
            retval = UA_Float_decodeBinary(src, offset, (UA_Float*)ptr);
            break;
        case UA_TYPES_INT64:
        case UA_TYPES_UINT64:
        case UA_TYPES_DATETIME:
            retval = UA_Int64_decodeBinary(src, offset, (UA_Int64*)ptr);
            break;
        case UA_TYPES_DOUBLE:
            retval = UA_Double_decodeBinary(src, offset, (UA_Double*)ptr);
            break;
        case UA_TYPES_GUID:
            retval = UA_Guid_decodeBinary(src, offset, (UA_Guid*)ptr);
            break;
        case UA_TYPES_NODEID:
            retval = UA_NodeId_decodeBinary(src, offset, (UA_NodeId*)ptr);
            break;
        case UA_TYPES_EXPANDEDNODEID:
            retval = UA_ExpandedNodeId_decodeBinary(src, offset, (UA_ExpandedNodeId*)ptr);
            break;
        case UA_TYPES_QUALIFIEDNAME:
            retval = UA_QualifiedName_decodeBinary(src, offset, (UA_QualifiedName*)ptr);
            break;
        case UA_TYPES_LOCALIZEDTEXT:
            retval = UA_LocalizedText_decodeBinary(src, offset, (UA_LocalizedText*)ptr);
            break;
        case UA_TYPES_EXTENSIONOBJECT:
            retval = UA_ExtensionObject_decodeBinary(src, offset, (UA_ExtensionObject*)ptr);
            break;
        case UA_TYPES_DATAVALUE:
            retval = UA_DataValue_decodeBinary(src, offset, (UA_DataValue*)ptr);
            break;
        case UA_TYPES_VARIANT:
            retval = UA_Variant_decodeBinary(src, offset, (UA_Variant*)ptr);
            break;
        case UA_TYPES_DIAGNOSTICINFO:
            retval = UA_DiagnosticInfo_decodeBinary(src, offset, (UA_DiagnosticInfo*)ptr);
            break;
        case UA_TYPES_STRING:
        case UA_TYPES_BYTESTRING:
        case UA_TYPES_XMLELEMENT:
            retval = UA_String_decodeBinary(src, offset, (UA_String*)ptr);
            break;
        default:
            retval = UA_decodeBinary(src, offset, (void*)ptr, memberType);
        }
        ptr += memberType->memSize;
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_deleteMembers(dst, dataType);
    return retval;
}

/******************/
/* Array Handling */
/******************/

size_t UA_Array_calcSizeBinary(const void *p, UA_Int32 noElements, const UA_DataType *dataType) {
    if(noElements <= 0)
        return 4;
    size_t size = 4; // the array size encoding
    if(dataType->fixedSize) {
        size += noElements * UA_calcSizeBinary(p, dataType);
        return size;
    }
    uintptr_t ptr = (uintptr_t)p;
    for(int i=0;i<noElements;i++) {
        size += UA_calcSizeBinary((void*)ptr, dataType);
        ptr += dataType->memSize;
    }
    return size;
}

UA_StatusCode UA_Array_encodeBinary(const void *src, UA_Int32 noElements, const UA_DataType *dataType,
                                    UA_ByteString *dst, size_t *offset) {
    if(noElements <= -1)
        noElements = -1;
    UA_Int32_encodeBinary(&noElements, dst, offset);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    uintptr_t ptr = (uintptr_t)src;
    for(int i=0;i<noElements && retval == UA_STATUSCODE_GOOD;i++) {
        retval = UA_encodeBinary((const void*)ptr, dataType, dst, offset);
        ptr += dataType->memSize;
    }
    return retval;
}

UA_StatusCode UA_Array_decodeBinary(const UA_ByteString *src, size_t *offset, UA_Int32 noElements,
                                    void **dst, const UA_DataType *dataType) {
    if(noElements <= 0) {
        *dst = UA_NULL;
        return UA_STATUSCODE_GOOD;
    }

    if((UA_Int32)dataType->memSize * noElements < 0 || dataType->memSize * noElements > MAX_ARRAY_SIZE )
        return UA_STATUSCODE_BADOUTOFMEMORY;

    // filter out arrays that can obviously not be parsed
    if(*offset + ((dataType->memSize * noElements)/32) > (UA_UInt32)src->length)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = UA_malloc(dataType->memSize * noElements);
    if(!*dst)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    uintptr_t ptr = (uintptr_t)*dst;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Int32 i;
    for(i=0;i<noElements && retval == UA_STATUSCODE_GOOD;i++) {
        retval = UA_decodeBinary(src, offset, (void*)ptr, dataType);
        ptr += dataType->memSize;
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_Array_delete(*dst, dataType, i);
    return retval;
}
