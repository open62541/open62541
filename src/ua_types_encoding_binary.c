#include "ua_types_encoding_binary.h"
#include "ua_util.h"
#include "ua_namespace_0.h"
#include "ua_statuscodes.h"

static INLINE UA_Boolean is_builtin(const UA_NodeId *typeid ) {
    return typeid ->namespaceIndex == 0 && 1 <= typeid ->identifier.numeric &&
        typeid ->identifier.numeric <= 25;
}

/*********/
/* Array */
/*********/

/** The data-pointer may be null. Then the array is assumed to be empy. */
UA_UInt32 UA_Array_calcSizeBinary(UA_Int32 length, const UA_TypeVTable *vt, const void *data) {
    if(!data) //empty arrays are encoded with the length member either 0 or -1
        return sizeof(UA_Int32);

    UA_UInt32 l  = sizeof(UA_Int32);
    UA_UInt32 memSize = vt->memSize;
    const UA_Byte *cdata   = (const UA_Byte *)data;
    for(UA_Int32 i = 0;i < length;i++) {
        l += vt->encodings[UA_ENCODING_BINARY].calcSize(cdata);
        cdata  += memSize;
    }
    return l;
}

/** The data-pointer may be null. Then the array is assumed to be empy. */
static UA_UInt32 UA_Array_calcSizeBinary_asExtensionObject(UA_Int32 length, const UA_TypeVTable *vt, const void *data) {
    UA_UInt32 l = UA_Array_calcSizeBinary(length, vt, data);
    if(!is_builtin(&vt->typeId))
        l += 9*length;  // extensionobject header for each element
    return l;
}

/* The src-pointer may be null if the array length is <= 0. */
UA_StatusCode UA_Array_encodeBinary(const void *src, UA_Int32 length, const UA_TypeVTable *vt,
                                    UA_ByteString *dst, UA_UInt32 *offset) {
    //Null Arrays are encoded with length = -1 // part 6 - §5.24
    if(length < -1)
        length = -1;

    UA_StatusCode retval   = UA_Int32_encodeBinary(&length, dst, offset);
    const UA_Byte *csrc    = (const UA_Byte *)src;
    UA_UInt32      memSize = vt->memSize;
    for(UA_Int32 i = 0;i < length && !retval;i++) {
        retval |= vt->encodings[UA_ENCODING_BINARY].encode(csrc, dst, offset);
        csrc   += memSize;
    }
    return retval;
}

static UA_StatusCode UA_Array_encodeBinary_asExtensionObject(const void *src, UA_Int32 length, const UA_TypeVTable *vt,
                                                             UA_ByteString *dst, UA_UInt32 *offset) {
    //Null Arrays are encoded with length = -1 // part 6 - §5.24
    if(length < -1)
        length = -1;

    UA_StatusCode retval = UA_Int32_encodeBinary(&length, dst, offset);
    const UA_Byte *csrc = (const UA_Byte *)src;
    UA_UInt32 memSize = vt->memSize;
    UA_Boolean isBuiltin = is_builtin(&vt->typeId);
    for(UA_Int32 i = 0;i < length && !retval;i++) {
        if(!isBuiltin) {
            // print the extensionobject header
            UA_NodeId_encodeBinary(&vt->typeId, dst, offset);
            UA_Byte  eoEncoding       = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING;
            UA_Byte_encodeBinary(&eoEncoding, dst, offset);
            UA_Int32 eoEncodingLength = vt->encodings[UA_ENCODING_BINARY].calcSize(csrc);
            UA_Int32_encodeBinary(&eoEncodingLength, dst, offset);
        }
        retval |= vt->encodings[UA_ENCODING_BINARY].encode(csrc, dst, offset);
        csrc   += memSize;
    }
    return retval;
}

UA_StatusCode UA_Array_decodeBinary(const UA_ByteString *src, UA_UInt32 *offset, UA_Int32 length,
                                    const UA_TypeVTable *vt, void **dst) {
    if(length <= 0) {
        *dst = UA_NULL;
        return UA_STATUSCODE_GOOD;
    }

    UA_StatusCode retval = UA_Array_new(dst, length, vt);
    if(retval)
        return retval;
    
    UA_Byte  *arr     = (UA_Byte *)*dst;
    UA_Int32  i       = 0;
    UA_UInt32 memSize = vt->memSize;
    for(;i < length && !retval;i++) {
        retval |= vt->encodings[UA_ENCODING_BINARY].decode(src, offset, arr);
        arr    += memSize;
    }

    /* If dynamically sized elements have already been decoded into the array. */
    if(retval) {
        arr = (UA_Byte*) *dst;
        for(UA_Int32 n=0;n<i;n++) {
            vt->deleteMembers(arr);
            arr += memSize;
        }
        UA_free(*dst);
        *dst = UA_NULL;
    }

    return retval;
}

/************/
/* Built-In */
/************/

#define UA_TYPE_CALCSIZEBINARY_SIZEOF(TYPE) \
    UA_UInt32 TYPE##_calcSizeBinary(TYPE const *p) { return sizeof(TYPE); }

#define UA_TYPE_ENCODEBINARY(TYPE, CODE)                                \
    UA_StatusCode TYPE##_encodeBinary(TYPE const *src, UA_ByteString * dst, UA_UInt32 *offset) { \
        UA_StatusCode retval = UA_STATUSCODE_GOOD;                      \
        do { CODE } while(0);                                           \
        return retval;                                                  \
    }

// Attention! this macro works only for TYPEs with memSize = encodingSize
#define UA_TYPE_DECODEBINARY(TYPE, CODE)                                \
    UA_StatusCode TYPE##_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, TYPE * dst) { \
        UA_StatusCode retval = UA_STATUSCODE_GOOD;                      \
        do { CODE } while(0);                                           \
        if(retval)                                                      \
            TYPE##_deleteMembers(dst);                                  \
        return retval;                                                  \
    }

/* Boolean */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Boolean)
UA_TYPE_ENCODEBINARY(UA_Boolean,
                     UA_Boolean tmpBool = ((*src > 0) ? UA_TRUE : UA_FALSE);
                     UA_memcpy(&dst->data[(*offset)++], &tmpBool, sizeof(UA_Boolean)); )
UA_TYPE_DECODEBINARY(UA_Boolean,
                     if(*offset + sizeof(UA_Boolean) > (UA_UInt32)src->length )
                         return UA_STATUSCODE_BADDECODINGERROR;
                     *dst = ((UA_Boolean)(src->data[(*offset)++]) > (UA_Byte)0) ? UA_TRUE : UA_FALSE; )

/* SByte */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_SByte)
UA_TYPE_ENCODEBINARY(UA_SByte, dst->data[(*offset)++] = *src; )
UA_TYPE_DECODEBINARY(UA_SByte,
                     if(*offset + sizeof(UA_SByte) > (UA_UInt32)src->length )
                         return UA_STATUSCODE_BADDECODINGERROR;
                     *dst = src->data[(*offset)++]; )

/* Byte */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Byte)
UA_TYPE_ENCODEBINARY(UA_Byte, dst->data[(*offset)++] = *src; )
UA_TYPE_DECODEBINARY(UA_Byte,
                     if(*offset + sizeof(UA_Byte) > (UA_UInt32)src->length )
                         return UA_STATUSCODE_BADDECODINGERROR;
                     *dst = src->data[(*offset)++]; )

/* Int16 */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Int16)
UA_TYPE_ENCODEBINARY(UA_Int16, retval = UA_UInt16_encodeBinary((UA_UInt16 const *)src, dst, offset); )
UA_TYPE_DECODEBINARY(UA_Int16,
                     if(*offset + sizeof(UA_Int16) > (UA_UInt32)src->length )
                         return UA_STATUSCODE_BADDECODINGERROR;
                     *dst  = (UA_Int16)(((UA_SByte)(src->data[(*offset)++]) & 0xFF) << 0);
                     *dst |= (UA_Int16)(((UA_SByte)(src->data[(*offset)++]) & 0xFF) << 8); )

/* UInt16 */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_UInt16)
UA_TYPE_ENCODEBINARY(UA_UInt16,
                     dst->data[(*offset)++] = (*src & 0x00FF) >> 0;
                     dst->data[(*offset)++] = (*src & 0xFF00) >> 8; )
UA_TYPE_DECODEBINARY(UA_UInt16,
                     if(*offset + sizeof(UA_UInt16) > (UA_UInt32)src->length )
                         return UA_STATUSCODE_BADDECODINGERROR;
                     *dst =  (UA_UInt16)src->data[(*offset)++] << 0;
                     *dst |= (UA_UInt16)src->data[(*offset)++] << 8; )

/* Int32 */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Int32)
UA_TYPE_ENCODEBINARY(UA_Int32,
                     dst->data[(*offset)++] = (*src & 0x000000FF) >> 0;
                     dst->data[(*offset)++] = (*src & 0x0000FF00) >> 8;
                     dst->data[(*offset)++] = (*src & 0x00FF0000) >> 16;
                     dst->data[(*offset)++] = (*src & 0xFF000000) >> 24; )
UA_TYPE_DECODEBINARY(UA_Int32,
                     if(*offset + sizeof(UA_Int32) > (UA_UInt32)src->length )
                         return UA_STATUSCODE_BADDECODINGERROR;
                     *dst  = (UA_Int32)(((UA_SByte)(src->data[(*offset)++]) & 0xFF) << 0);
                     *dst |= (UA_Int32)(((UA_SByte)(src->data[(*offset)++]) & 0xFF) << 8);
                     *dst |= (UA_Int32)(((UA_SByte)(src->data[(*offset)++]) & 0xFF) << 16);
                     *dst |= (UA_Int32)(((UA_SByte)(src->data[(*offset)++]) & 0xFF) << 24); )

/* UInt32 */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_UInt32)
UA_TYPE_ENCODEBINARY(UA_UInt32, retval = UA_Int32_encodeBinary((UA_Int32 const *)src, dst, offset); )
UA_TYPE_DECODEBINARY(UA_UInt32,
                     if(*offset + sizeof(UA_UInt32) > (UA_UInt32)src->length )
                         return UA_STATUSCODE_BADDECODINGERROR;
                     UA_UInt32 t1 = (UA_UInt32)((UA_Byte)(src->data[(*offset)++] & 0xFF));
                     UA_UInt32 t2 = (UA_UInt32)((UA_Byte)(src->data[(*offset)++]& 0xFF) << 8);
                     UA_UInt32 t3 = (UA_UInt32)((UA_Byte)(src->data[(*offset)++]& 0xFF) << 16);
                     UA_UInt32 t4 = (UA_UInt32)((UA_Byte)(src->data[(*offset)++]& 0xFF) << 24);
                     *dst = t1 + t2 + t3 + t4; )

/* Int64 */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Int64)
UA_TYPE_ENCODEBINARY(UA_Int64,
                     dst->data[(*offset)++] = (*src & 0x00000000000000FF) >> 0;
                     dst->data[(*offset)++] = (*src & 0x000000000000FF00) >> 8;
                     dst->data[(*offset)++] = (*src & 0x0000000000FF0000) >> 16;
                     dst->data[(*offset)++] = (*src & 0x00000000FF000000) >> 24;
                     dst->data[(*offset)++] = (*src & 0x000000FF00000000) >> 32;
                     dst->data[(*offset)++] = (*src & 0x0000FF0000000000) >> 40;
                     dst->data[(*offset)++] = (*src & 0x00FF000000000000) >> 48;
                     dst->data[(*offset)++] = (*src & 0xFF00000000000000) >> 56; )
UA_TYPE_DECODEBINARY(UA_Int64,
                     if(*offset + sizeof(UA_Int64) > (UA_UInt32)src->length )
                         return UA_STATUSCODE_BADDECODINGERROR;
                     *dst  = (UA_Int64)src->data[(*offset)++] << 0;
                     *dst |= (UA_Int64)src->data[(*offset)++] << 8;
                     *dst |= (UA_Int64)src->data[(*offset)++] << 16;
                     *dst |= (UA_Int64)src->data[(*offset)++] << 24;
                     *dst |= (UA_Int64)src->data[(*offset)++] << 32;
                     *dst |= (UA_Int64)src->data[(*offset)++] << 40;
                     *dst |= (UA_Int64)src->data[(*offset)++] << 48;
                     *dst |= (UA_Int64)src->data[(*offset)++] << 56; )

/* UInt64 */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_UInt64)
UA_TYPE_ENCODEBINARY(UA_UInt64, return UA_Int64_encodeBinary((UA_Int64 const *)src, dst, offset); )
UA_TYPE_DECODEBINARY(UA_UInt64,
                     if(*offset + sizeof(UA_UInt64) > (UA_UInt32)src->length )
                         return UA_STATUSCODE_BADDECODINGERROR;
                     UA_UInt64 t1 = (UA_UInt64)src->data[(*offset)++];
                     UA_UInt64 t2 = (UA_UInt64)src->data[(*offset)++] << 8;
                     UA_UInt64 t3 = (UA_UInt64)src->data[(*offset)++] << 16;
                     UA_UInt64 t4 = (UA_UInt64)src->data[(*offset)++] << 24;
                     UA_UInt64 t5 = (UA_UInt64)src->data[(*offset)++] << 32;
                     UA_UInt64 t6 = (UA_UInt64)src->data[(*offset)++] << 40;
                     UA_UInt64 t7 = (UA_UInt64)src->data[(*offset)++] << 48;
                     UA_UInt64 t8 = (UA_UInt64)src->data[(*offset)++] << 56;
                     *dst = t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8; )

/* Float */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Float)
// FIXME: Implement NaN, Inf and Zero(s)
UA_Byte UA_FLOAT_ZERO[] = { 0x00, 0x00, 0x00, 0x00 };
UA_TYPE_DECODEBINARY(UA_Float,
                     if(*offset + sizeof(UA_Float) > (UA_UInt32)src->length )
                         return UA_STATUSCODE_BADDECODINGERROR;
                     UA_Float mantissa;
                     UA_UInt32 biasedExponent;
                     UA_Float sign;
                     if(memcmp(&src->data[*offset], UA_FLOAT_ZERO, 4) == 0) return UA_Int32_decodeBinary(src, offset, (UA_Int32 *)dst);

                     mantissa = (UA_Float)(src->data[*offset] & 0xFF);                         // bits 0-7
                     mantissa = (mantissa / 256.0 ) + (UA_Float)(src->data[*offset+1] & 0xFF); // bits 8-15
                     mantissa = (mantissa / 256.0 ) + (UA_Float)(src->data[*offset+2] & 0x7F); // bits 16-22
                     biasedExponent  = (src->data[*offset+2] & 0x80) >>  7;                    // bits 23
                     biasedExponent |= (src->data[*offset+3] & 0x7F) <<  1;                    // bits 24-30
                     sign = ( src->data[*offset+ 3] & 0x80 ) ? -1.0 : 1.0;                     // bit 31
                     if(biasedExponent >= 127)
                         *dst = (UA_Float)sign * (1 << (biasedExponent-127)) * (1.0 + mantissa / 128.0 );
                     else
                         *dst = (UA_Float)sign * 2.0 *
                                (1.0 + mantissa / 128.0 ) / ((UA_Float)(biasedExponent-127));
                     *offset += 4; )
UA_TYPE_ENCODEBINARY(UA_Float, return UA_UInt32_encodeBinary((UA_UInt32 *)src, dst, offset); )

/* Double */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Double)
// FIXME: Implement NaN, Inf and Zero(s)
UA_Byte UA_DOUBLE_ZERO[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UA_TYPE_DECODEBINARY(UA_Double,
                     if(*offset + sizeof(UA_Double) > (UA_UInt32)src->length )
                         return UA_STATUSCODE_BADDECODINGERROR;
                     UA_Double sign;
                     UA_Double mantissa;
                     UA_UInt32 biasedExponent;
                     if(memcmp(&src->data[*offset], UA_DOUBLE_ZERO, 8) == 0) return UA_Int64_decodeBinary(src, offset, (UA_Int64 *)dst);
                     mantissa = (UA_Double)(src->data[*offset] & 0xFF);                         // bits 0-7
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+1] & 0xFF); // bits 8-15
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+2] & 0xFF); // bits 16-23
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+3] & 0xFF); // bits 24-31
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+4] & 0xFF); // bits 32-39
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+5] & 0xFF); // bits 40-47
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+6] & 0x0F); // bits 48-51
                     DBG_VERBOSE(printf("UA_Double_decodeBinary - mantissa=%f\n", mantissa));
                     biasedExponent  = (src->data[*offset+6] & 0xF0) >>  4; // bits 52-55
                     DBG_VERBOSE(printf("UA_Double_decodeBinary - biasedExponent52-55=%d, src=%d\n",
                                        biasedExponent,
                                        src->data[*offset+6]));
                     biasedExponent |= ((UA_UInt32)(src->data[*offset+7] & 0x7F)) <<  4; // bits 56-62
                     DBG_VERBOSE(printf("UA_Double_decodeBinary - biasedExponent56-62=%d, src=%d\n",
                                        biasedExponent,
                                        src->data[*offset+7]));
                     sign = ( src->data[*offset+7] & 0x80 ) ? -1.0 : 1.0; // bit 63
                     if(biasedExponent >= 1023)
                         *dst = (UA_Double)sign * (1 << (biasedExponent-1023)) * (1.0 + mantissa / 8.0 );
                     else
                         *dst = (UA_Double)sign * 2.0 *
                                (1.0 + mantissa / 8.0 ) / ((UA_Double)(biasedExponent-1023));
                     *offset += 8; )
UA_TYPE_ENCODEBINARY(UA_Double, return UA_UInt64_encodeBinary((UA_UInt64 *)src, dst, offset); )

/* String */
UA_UInt32 UA_String_calcSizeBinary(UA_String const *string) {
    if(string->length > 0)
        return sizeof(UA_Int32) + string->length * sizeof(UA_Byte);
    else
        return sizeof(UA_Int32);
}

UA_StatusCode UA_String_encodeBinary(UA_String const *src, UA_ByteString *dst, UA_UInt32 *offset) {
    if((UA_Int32)(*offset + UA_String_calcSizeBinary(src)) > dst->length)
        return UA_STATUSCODE_BADENCODINGERROR;

    UA_StatusCode retval = UA_Int32_encodeBinary(&src->length, dst, offset);
    if(src->length > 0) {
        UA_memcpy(&dst->data[*offset], src->data, src->length);
        *offset += src->length;
    }
    return retval;
}

UA_StatusCode UA_String_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_String *dst) {
    UA_String_init(dst);
    UA_Int32 length;
    if(UA_Int32_decodeBinary(src, offset, &length))
        return UA_STATUSCODE_BADINTERNALERROR;
    if(length <= 0)
        return UA_STATUSCODE_GOOD;
        
    if(length > (UA_Int32)(src->length - *offset))
        return UA_STATUSCODE_BADINTERNALERROR;
    
    if(!(dst->data = UA_alloc(length)))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_memcpy(dst->data, &src->data[*offset], length);
    dst->length = length;
    *offset += length;
    return UA_STATUSCODE_GOOD;
}

/* DateTime */
UA_TYPE_BINARY_ENCODING_AS(UA_DateTime, UA_Int64)

/* Guid */
UA_UInt32 UA_Guid_calcSizeBinary(UA_Guid const *p) {
    return 16;
}

UA_TYPE_ENCODEBINARY(UA_Guid,
                     retval |= UA_UInt32_encodeBinary(&src->data1, dst, offset);
                     retval |= UA_UInt16_encodeBinary(&src->data2, dst, offset);
                     retval |= UA_UInt16_encodeBinary(&src->data3, dst, offset);
                     for(UA_Int32 i = 0;i < 8;i++)
                         retval |= UA_Byte_encodeBinary(&src->data4[i], dst, offset);)

UA_TYPE_DECODEBINARY(UA_Guid,
                     // TODO: This could be done with a single memcpy (if the compiler does no fancy realigning of structs)
                     retval |= UA_UInt32_decodeBinary(src, offset, &dst->data1);
                     retval |= UA_UInt16_decodeBinary(src, offset, &dst->data2);
                     retval |= UA_UInt16_decodeBinary(src, offset, &dst->data3);
                     for(UA_Int32 i = 0;i < 8;i++)
                         retval |= UA_Byte_decodeBinary(src, offset, &dst->data4[i]);
                     )

/* ByteString */
UA_TYPE_BINARY_ENCODING_AS(UA_ByteString, UA_String)

/* XmlElement */
UA_TYPE_BINARY_ENCODING_AS(UA_XmlElement, UA_String)

/* NodeId */

/* The shortened numeric nodeid types. */
#define UA_NODEIDTYPE_TWOBYTE 0
#define UA_NODEIDTYPE_FOURBYTE 1

UA_UInt32 UA_NodeId_calcSizeBinary(UA_NodeId const *p) {
    UA_UInt32 length = 0;
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
        length = sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_ByteString_calcSizeBinary(
                                                                                    &p->identifier.byteString);
        break;

    default:
        UA_assert(UA_FALSE); // this must never happen
        break;
    }
    return length;
}

UA_TYPE_ENCODEBINARY(UA_NodeId,
                     // temporary variables for endian-save code
                     UA_Byte srcByte;
                     UA_UInt16 srcUInt16;

                     UA_StatusCode retval = UA_STATUSCODE_GOOD;
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
                         UA_assert(UA_FALSE); // must never happen
                     }
                     )

UA_StatusCode UA_NodeId_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_NodeId *dst) {
    // temporary variables to overcome decoder's non-endian-saveness for datatypes with different length
    UA_Byte   dstByte = 0;
    UA_UInt16 dstUInt16 = 0;
    UA_Byte   encodingByte = 0;

    UA_StatusCode retval = UA_Byte_decodeBinary(src, offset, &encodingByte); // will be cleaned up in the end if sth goes wrong
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
UA_UInt32 UA_ExpandedNodeId_calcSizeBinary(UA_ExpandedNodeId const *p) {
    UA_UInt32 length = UA_NodeId_calcSizeBinary(&p->nodeId);
    if(p->namespaceUri.length > 0)
        length += UA_String_calcSizeBinary(&p->namespaceUri);
    if(p->serverIndex > 0)
        length += sizeof(UA_UInt32);
    return length;
}

#define UA_EXPANDEDNODEID_NAMESPACEURI_FLAG 0x80
#define UA_EXPANDEDNODEID_SERVERINDEX_FLAG 0x40

UA_TYPE_ENCODEBINARY(UA_ExpandedNodeId,
                     UA_Byte flags = 0;
                     UA_UInt32 start = *offset;
                     retval |= UA_NodeId_encodeBinary(&src->nodeId, dst, offset);
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
                     )

UA_StatusCode UA_ExpandedNodeId_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_ExpandedNodeId *dst) {
    UA_ExpandedNodeId_init(dst);
    // get encodingflags and leave a "clean" nodeidtype
    if((UA_Int32)*offset >= src->length)
        return UA_STATUSCODE_BADDECODINGERROR;

    // prepare decoding of the nodeid
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
UA_UInt32 UA_QualifiedName_calcSizeBinary(UA_QualifiedName const *p) {
    UA_UInt32 length = sizeof(UA_UInt16); //qualifiedName->namespaceIndex
    // length += sizeof(UA_UInt16); //qualifiedName->reserved
    length += UA_String_calcSizeBinary(&p->name); //qualifiedName->name
    return length;
}
UA_StatusCode UA_QualifiedName_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_QualifiedName *dst) {
    UA_QualifiedName_init(dst);
    UA_StatusCode retval = UA_UInt16_decodeBinary(src, offset, &dst->namespaceIndex);
    retval |= UA_String_decodeBinary(src, offset, &dst->name);
    if(retval)
        UA_QualifiedName_deleteMembers(dst);
    return retval;
}
UA_TYPE_ENCODEBINARY(UA_QualifiedName,
                     retval |= UA_UInt16_encodeBinary(&src->namespaceIndex, dst, offset);
                     retval |= UA_String_encodeBinary(&src->name, dst, offset); )

/* LocalizedText */
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE 0x01
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT 0x02

UA_UInt32 UA_LocalizedText_calcSizeBinary(UA_LocalizedText const *p) {
    UA_UInt32 length = 1; // for encodingMask
    if(p->locale.data != UA_NULL)
        length += UA_String_calcSizeBinary(&p->locale);
    if(p->text.data != UA_NULL)
        length += UA_String_calcSizeBinary(&p->text);
    return length;
}

UA_TYPE_ENCODEBINARY(UA_LocalizedText,
                     UA_Byte encodingMask = 0;
                     if(src->locale.data != UA_NULL)
                         encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE;
                     if(src->text.data != UA_NULL)
                         encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
                     retval |= UA_Byte_encodeBinary(&encodingMask, dst, offset);
                     if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
                         retval |= UA_String_encodeBinary(&src->locale, dst, offset);
                     if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
                         retval |= UA_String_encodeBinary(&src->text, dst, offset); )

UA_StatusCode UA_LocalizedText_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_LocalizedText *dst) {
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
UA_UInt32 UA_ExtensionObject_calcSizeBinary(UA_ExtensionObject const *p) {
    UA_Int32 length = UA_NodeId_calcSizeBinary(&p->typeId);
    length += 1; // encoding
    switch(p->encoding) {
    case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING:
        length += UA_ByteString_calcSizeBinary(&p->body);
        break;

    case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML:
        length += UA_XmlElement_calcSizeBinary((UA_XmlElement *)&p->body);
        break;

    default:
        break;
    }
    return length;
}

UA_TYPE_ENCODEBINARY(UA_ExtensionObject,
                     retval |= UA_NodeId_encodeBinary(&src->typeId, dst, offset);
                     UA_Byte encoding = src->encoding;
                     retval |= UA_Byte_encodeBinary(&encoding, dst, offset);
                     switch(src->encoding) {
                     case UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED:
                         break;

                     case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING:
                         // FIXME: This code is valid for numeric nodeIds in ns0 only!
                         //retval |= UA_TYPES[UA_ns0ToVTableIndex(&src->typeId)].encodings[UA_ENCODING_BINARY].encode(src->body.data, dst, offset);
                         //break;

                     case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML:
                         retval |= UA_ByteString_encodeBinary(&src->body, dst, offset);
                         break;

                     default:
                         UA_assert(UA_FALSE);
                     }
                     )

UA_StatusCode UA_ExtensionObject_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_ExtensionObject *dst) {
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
//TODO: place this define at the server configuration
UA_UInt32 UA_DataValue_calcSizeBinary(UA_DataValue const *p) {
    UA_UInt32 length = sizeof(UA_Byte);
    if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_VARIANT)
        length += UA_Variant_calcSizeBinary(&p->value);
    if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_STATUSCODE)
        length += sizeof(UA_UInt32);   //dataValue->status
    if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_SOURCETIMESTAMP)
        length += sizeof(UA_DateTime);  //dataValue->sourceTimestamp
    if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_SOURCEPICOSECONDS)
        length += sizeof(UA_Int64);    //dataValue->sourcePicoseconds
    if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_SERVERTIMESTAMP)
        length += sizeof(UA_DateTime);  //dataValue->serverTimestamp
    if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_SERVERPICOSECONDS)
        length += sizeof(UA_Int64);    //dataValue->serverPicoseconds
    return length;
}

UA_TYPE_ENCODEBINARY(UA_DataValue,
                     retval |= UA_Byte_encodeBinary(&src->encodingMask, dst, offset);
                     if(src->encodingMask & UA_DATAVALUE_ENCODINGMASK_VARIANT)
                         retval |= UA_Variant_encodeBinary(&src->value, dst, offset);
                     if(src->encodingMask & UA_DATAVALUE_ENCODINGMASK_STATUSCODE)
                         retval |= UA_StatusCode_encodeBinary(&src->status, dst, offset);
                     if(src->encodingMask & UA_DATAVALUE_ENCODINGMASK_SOURCETIMESTAMP)
                         retval |= UA_DateTime_encodeBinary(&src->sourceTimestamp, dst, offset);
                     if(src->encodingMask & UA_DATAVALUE_ENCODINGMASK_SOURCEPICOSECONDS)
                         retval |= UA_Int16_encodeBinary(&src->sourcePicoseconds, dst, offset);
                     if(src->encodingMask & UA_DATAVALUE_ENCODINGMASK_SERVERTIMESTAMP)
                         retval |= UA_DateTime_encodeBinary(&src->serverTimestamp, dst, offset);
                     if(src->encodingMask & UA_DATAVALUE_ENCODINGMASK_SERVERPICOSECONDS)
                         retval |= UA_Int16_encodeBinary(&src->serverPicoseconds, dst, offset);
                     )

#define MAX_PICO_SECONDS 1000
UA_StatusCode UA_DataValue_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_DataValue *dst) {
    UA_DataValue_init(dst);
    UA_StatusCode retval = UA_Byte_decodeBinary(src, offset, &dst->encodingMask);
    if(dst->encodingMask & UA_DATAVALUE_ENCODINGMASK_VARIANT)
        retval |= UA_Variant_decodeBinary(src, offset, &dst->value);
    if(dst->encodingMask & UA_DATAVALUE_ENCODINGMASK_STATUSCODE)
        retval |= UA_StatusCode_decodeBinary(src, offset, &dst->status);
    if(dst->encodingMask & UA_DATAVALUE_ENCODINGMASK_SOURCETIMESTAMP)
        retval |= UA_DateTime_decodeBinary(src, offset, &dst->sourceTimestamp);
    if(dst->encodingMask & UA_DATAVALUE_ENCODINGMASK_SOURCEPICOSECONDS) {
        retval |= UA_Int16_decodeBinary(src, offset, &dst->sourcePicoseconds);
        if(dst->sourcePicoseconds > MAX_PICO_SECONDS)
            dst->sourcePicoseconds = MAX_PICO_SECONDS;
    }
    if(dst->encodingMask & UA_DATAVALUE_ENCODINGMASK_SERVERTIMESTAMP)
        retval |= UA_DateTime_decodeBinary(src, offset, &dst->serverTimestamp);
    if(dst->encodingMask & UA_DATAVALUE_ENCODINGMASK_SERVERPICOSECONDS) {
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

UA_UInt32 UA_Variant_calcSizeBinary(UA_Variant const *p) {
    UA_UInt32 arrayLength, length;
    const UA_VariantData *data;
    if(p->storageType == UA_VARIANT_DATA)
        data = &p->storage.data;
    else {
        if(p->storage.datasource.read(p->storage.datasource.identifier, &data) != UA_STATUSCODE_GOOD)
            return 0;
    }
        
    arrayLength = data->arrayLength;
    if(data->dataPtr == UA_NULL)
        arrayLength = -1;

    length = sizeof(UA_Byte); //p->encodingMask
    if(arrayLength != 1) {
        // array length + the array itself
        length += UA_Array_calcSizeBinary(arrayLength, p->vt, data->dataPtr);
    } else {
        length += p->vt->encodings[UA_ENCODING_BINARY].calcSize(data->dataPtr);
        if(!is_builtin(&p->vt->typeId))
            length += 9;  // 4 byte nodeid + 1 byte encoding + 4 byte bytestring length
    }

    if(arrayLength != 1 && data->arrayDimensions != UA_NULL) {
        if(is_builtin(&p->vt->typeId))
            length += UA_Array_calcSizeBinary(data->arrayDimensionsLength, &UA_TYPES[UA_INT32], data->arrayDimensions);
        else
            length += UA_Array_calcSizeBinary_asExtensionObject(data->arrayDimensionsLength, &UA_TYPES[UA_INT32], data->arrayDimensions);
    }
    
    if(p->storageType == UA_VARIANT_DATASOURCE)
        p->storage.datasource.release(p->storage.datasource.identifier, data);

    return length;
}

UA_TYPE_ENCODEBINARY(UA_Variant,
                     UA_Byte encodingByte;
                     UA_Boolean isArray;
                     UA_Boolean hasDimensions;
                     UA_Boolean isBuiltin;

                     const UA_VariantData  *data;
                     if(src->storageType == UA_VARIANT_DATA)
                         data = &src->storage.data;
                     else {
                         if(src->storage.datasource.read(src->storage.datasource.identifier, &data) != UA_STATUSCODE_GOOD)
                             return UA_STATUSCODE_BADENCODINGERROR;
                     }

                     isArray       = data->arrayLength != 1;  // a single element is not an array
                     hasDimensions = isArray && data->arrayDimensions != UA_NULL;
                     isBuiltin     = is_builtin(&src->vt->typeId);

                     encodingByte = 0;
                     if(isArray) {
                         encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
                         if(hasDimensions)
                             encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS;
                     }

                     if(isBuiltin)
                         encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK & (UA_Byte)src->vt->typeId.identifier.numeric;
                     else
                         encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK & (UA_Byte)22;  // ExtensionObject

                     retval |= UA_Byte_encodeBinary(&encodingByte, dst, offset);

                     if(isArray)
                         retval |= UA_Array_encodeBinary(data->dataPtr, data->arrayLength, src->vt, dst, offset);
                     else if(!data->dataPtr)
                         retval = UA_STATUSCODE_BADENCODINGERROR; // an array can be empty. a single element must be present.
                     else {
                         if(!isBuiltin) {
                             // print the extensionobject header
                             UA_NodeId_encodeBinary(&src->vt->typeId, dst, offset);
                             UA_Byte eoEncoding = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING;
                             UA_Byte_encodeBinary(&eoEncoding, dst, offset);
                             UA_Int32 eoEncodingLength = src->vt->encodings[UA_ENCODING_BINARY].calcSize(data->dataPtr);
                             UA_Int32_encodeBinary(&eoEncodingLength, dst, offset);
                         }
                         retval |= src->vt->encodings[UA_ENCODING_BINARY].encode(data->dataPtr, dst, offset);
                     }

                     if(hasDimensions) {
                         if(isBuiltin)
                             retval |= UA_Array_encodeBinary(data->arrayDimensions, data->arrayDimensionsLength, &UA_TYPES[UA_INT32], dst, offset);
                         else
                             retval |= UA_Array_encodeBinary_asExtensionObject(data->arrayDimensions, data->arrayDimensionsLength, &UA_TYPES[UA_INT32], dst, offset);
                     }

                     if(src->storageType == UA_VARIANT_DATASOURCE)
                         src->storage.datasource.release(src->storage.datasource.identifier, data);
                     
                     )

/* For decoding, we read extensionobjects as is. The resulting variant always has the storagetype UA_VARIANT_DATA. */
UA_StatusCode UA_Variant_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_Variant *dst) {
    UA_Variant_init(dst);
    UA_Byte encodingByte;
    UA_StatusCode retval = UA_Byte_decodeBinary(src, offset, &encodingByte);
    if(retval)
        return retval;

    UA_VariantData *data = &dst->storage.data;
    UA_Boolean isArray = encodingByte & UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
    UA_Boolean hasDimensions = isArray && (encodingByte & UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS);
    UA_NodeId typeid = { .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
                         .identifier.numeric = encodingByte & UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK };
    UA_Int32 typeNs0Id = UA_ns0ToVTableIndex(&typeid );
    const UA_TypeVTable *vt = &UA_TYPES[typeNs0Id];

    if(!isArray) {
        if(!(data->dataPtr = UA_alloc(vt->memSize)))
            return UA_STATUSCODE_BADOUTOFMEMORY;
        retval |= vt->encodings[UA_ENCODING_BINARY].decode(src, offset, data->dataPtr);
        if(retval) {
            UA_free(data->dataPtr);
            return retval;
        }
        data->arrayLength = 1;
    } else {
        retval |= UA_Int32_decodeBinary(src, offset, &data->arrayLength);
        if(retval == UA_STATUSCODE_GOOD)
            retval |= UA_Array_decodeBinary(src, offset, data->arrayLength, vt, &data->dataPtr);
        if(retval)
            data->arrayLength = -1; // for deleteMembers
    }

    if(hasDimensions && retval == UA_STATUSCODE_GOOD) {
        retval |= UA_Int32_decodeBinary(src, offset, &data->arrayDimensionsLength);
        if(retval == UA_STATUSCODE_GOOD)
            retval |= UA_Array_decodeBinary(src, offset, data->arrayDimensionsLength, &UA_TYPES[UA_INT32], &data->dataPtr);
        if(retval)
            data->arrayLength = -1; // for deleteMembers
    }

    dst->vt = vt;
    if(retval)
        UA_Variant_deleteMembers(dst);
    return retval;
}

/* DiagnosticInfo */
UA_UInt32 UA_DiagnosticInfo_calcSizeBinary(UA_DiagnosticInfo const *ptr) {
    UA_UInt32 length = sizeof(UA_Byte); // EncodingMask
    if(!ptr->encodingMask)
        return length;
    if(ptr->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_SYMBOLICID)
        length += sizeof(UA_Int32);
    if(ptr->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_NAMESPACE)
        length += sizeof(UA_Int32);
    if(ptr->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALIZEDTEXT)
        length += sizeof(UA_Int32);
    if(ptr->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALE)
        length += sizeof(UA_Int32);
    if(ptr->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_ADDITIONALINFO)
        length += UA_String_calcSizeBinary(&ptr->additionalInfo);
    if(ptr->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_INNERSTATUSCODE)
        length += sizeof(UA_StatusCode);
    if(ptr->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_INNERDIAGNOSTICINFO)
        length += UA_DiagnosticInfo_calcSizeBinary(ptr->innerDiagnosticInfo);
    return length;
}

UA_TYPE_ENCODEBINARY(UA_DiagnosticInfo,
                     retval |= UA_Byte_encodeBinary(&src->encodingMask, dst, offset);
                     if(!retval && !src->encodingMask)
                         return retval;

                     if(src->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_SYMBOLICID)
                        retval |= UA_Int32_encodeBinary(&src->symbolicId, dst, offset);
                     if(src->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_NAMESPACE)
                         retval |= UA_Int32_encodeBinary( &src->namespaceUri, dst, offset);
                     if(src->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALIZEDTEXT)
                         retval |= UA_Int32_encodeBinary(&src->localizedText, dst, offset);
                     if(src->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALE)
                         retval |= UA_Int32_encodeBinary(&src->locale, dst, offset);
                     if(src->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_ADDITIONALINFO)
                         retval |= UA_String_encodeBinary(&src->additionalInfo, dst, offset);
                     if(src->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_INNERSTATUSCODE)
                         retval |= UA_StatusCode_encodeBinary(&src->innerStatusCode, dst, offset);
                     if(src->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_INNERDIAGNOSTICINFO)
                         retval |= UA_DiagnosticInfo_encodeBinary(src->innerDiagnosticInfo, dst, offset);
                     )

UA_StatusCode UA_DiagnosticInfo_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_DiagnosticInfo *dst) {
    UA_DiagnosticInfo_init(dst);
    UA_StatusCode retval = UA_Byte_decodeBinary(src, offset, &dst->encodingMask);
    if(!retval && !dst->encodingMask) // in most cases, the DiagnosticInfo is empty
        return retval;
    if(dst->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_SYMBOLICID)
        retval |= UA_Int32_decodeBinary(src, offset, &dst->symbolicId);
    if(dst->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_NAMESPACE)
        retval |= UA_Int32_decodeBinary(src, offset, &dst->namespaceUri);
    if(dst->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALIZEDTEXT)
        retval |= UA_Int32_decodeBinary(src, offset, &dst->localizedText);
    if(dst->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALE)
        retval |= UA_Int32_decodeBinary(src, offset, &dst->locale);
    if(dst->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_ADDITIONALINFO)
        retval |= UA_String_decodeBinary(src, offset, &dst->additionalInfo);
    if(dst->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_INNERSTATUSCODE)
        retval |= UA_StatusCode_decodeBinary(src, offset, &dst->innerStatusCode);
    if(dst->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_INNERDIAGNOSTICINFO) {
        // innerDiagnosticInfo is a pointer to struct, therefore allocate
        if((dst->innerDiagnosticInfo = UA_alloc(sizeof(UA_DiagnosticInfo)))) {
            if(UA_DiagnosticInfo_decodeBinary(src, offset, dst->innerDiagnosticInfo) != UA_STATUSCODE_GOOD) {
                UA_free(dst->innerDiagnosticInfo);
                dst->innerDiagnosticInfo = UA_NULL;
                retval |= UA_STATUSCODE_BADINTERNALERROR;
            }
        } else {
            retval |= UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }
    if(retval)
        UA_DiagnosticInfo_deleteMembers(dst);
    return retval;
}

/* InvalidType */
UA_UInt32 UA_InvalidType_calcSizeBinary(UA_InvalidType const *p) {
    return 0;
}
UA_TYPE_ENCODEBINARY(UA_InvalidType, retval = UA_STATUSCODE_BADINTERNALERROR; )
UA_TYPE_DECODEBINARY(UA_InvalidType, retval = UA_STATUSCODE_BADINTERNALERROR; )
