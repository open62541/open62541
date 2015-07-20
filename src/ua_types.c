#include "ua_util.h"
#include "ua_types.h"
#include "ua_statuscodes.h"
#include "ua_types_generated.h"

/*****************/
/* Helper Macros */
/*****************/

#define UA_TYPE_DEFAULT(TYPE)            \
    UA_TYPE_NEW_DEFAULT(TYPE)            \
    UA_TYPE_INIT_DEFAULT(TYPE)           \
    UA_TYPE_DELETEMEMBERS_NOACTION(TYPE) \
    UA_TYPE_DELETE_DEFAULT(TYPE)         \
    UA_TYPE_COPY_DEFAULT(TYPE)           \

#define UA_TYPE_NEW_DEFAULT(TYPE)                              \
    TYPE * TYPE##_new() {                                      \
        TYPE *p = UA_malloc(sizeof(TYPE));                     \
        if(p) TYPE##_init(p);                                  \
        return p;                                              \
    }

#define UA_TYPE_INIT_DEFAULT(TYPE) \
    void TYPE##_init(TYPE * p) {   \
        *p = (TYPE)0;              \
    }

#define UA_TYPE_DELETEMEMBERS_NOACTION(TYPE) \
    void TYPE##_deleteMembers(TYPE *p) {    \
    }

#define UA_TYPE_DELETE_DEFAULT(TYPE) \
    void TYPE##_delete(TYPE *p) {    \
        TYPE##_deleteMembers(p);     \
        UA_free(p);                  \
    }

#define UA_TYPE_COPY_DEFAULT(TYPE)                             \
    UA_StatusCode TYPE##_copy(TYPE const *src, TYPE *dst) {    \
        *dst = *src;                                           \
        return UA_STATUSCODE_GOOD;                             \
    }

/*****************/
/* Builtin Types */
/*****************/

/* Boolean */
UA_TYPE_DEFAULT(UA_Boolean)

/* SByte */
UA_TYPE_DEFAULT(UA_SByte)

/* Byte */
UA_TYPE_DEFAULT(UA_Byte)

/* Int16 */
UA_TYPE_DEFAULT(UA_Int16)

/* UInt16 */
UA_TYPE_DEFAULT(UA_UInt16)

/* Int32 */
UA_TYPE_DEFAULT(UA_Int32)

/* UInt32 */
UA_TYPE_DEFAULT(UA_UInt32)

/* Int64 */
UA_TYPE_DEFAULT(UA_Int64)

/* UInt64 */
UA_TYPE_DEFAULT(UA_UInt64)

/* Float */
UA_TYPE_DEFAULT(UA_Float)

/* Double */
UA_TYPE_DEFAULT(UA_Double)

/* String */
UA_TYPE_NEW_DEFAULT(UA_String)
UA_TYPE_DELETE_DEFAULT(UA_String)

void UA_String_init(UA_String *p) {
    p->length = -1;
    p->data   = UA_NULL;
}

void UA_String_deleteMembers(UA_String *p) {
    UA_free(p->data);
    p->data = UA_NULL;
}

UA_StatusCode UA_String_copy(UA_String const *src, UA_String *dst) {
    UA_String_init(dst);
    if(src->length > 0) {
        if(!(dst->data = UA_malloc((UA_UInt32)src->length)))
            return UA_STATUSCODE_BADOUTOFMEMORY;
        UA_memcpy((void *)dst->data, src->data, (UA_UInt32)src->length);
    }
    dst->length = src->length;
    return UA_STATUSCODE_GOOD;
}

UA_String UA_String_fromChars(char const src[]) {
    UA_String str;
    size_t length = strlen(src);
    if(length == 0) {
        str.length = 0;
        str.data = UA_NULL;
        return str;
    }
    str.data = UA_malloc(length);
    if(!str.data) {
        str.length = -1;
        return str;
    }
    UA_memcpy(str.data, src, length);
    str.length = length;
    return str;
}

#define UA_STRING_ALLOCPRINTF_BUFSIZE 1024
UA_StatusCode UA_String_copyprintf(char const fmt[], UA_String *dst, ...) {
    char src[UA_STRING_ALLOCPRINTF_BUFSIZE];
    va_list ap;
    va_start(ap, dst);
#if ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4 || defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
    // vsnprintf should only take a literal and no variable to be secure
    UA_Int32 len = vsnprintf(src, UA_STRING_ALLOCPRINTF_BUFSIZE, fmt, ap);
#if ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4 || defined(__clang__))
#pragma GCC diagnostic pop
#endif
    va_end(ap);
    if(len < 0)  // FIXME: old glibc 2.0 would return -1 when truncated
        return UA_STATUSCODE_BADINTERNALERROR;
    // since glibc 2.1 vsnprintf returns the len that would have resulted if buf were large enough
    len = ( len > UA_STRING_ALLOCPRINTF_BUFSIZE ? UA_STRING_ALLOCPRINTF_BUFSIZE : len );
    if(!(dst->data = UA_malloc((UA_UInt32)len)))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_memcpy((void *)dst->data, src, (UA_UInt32)len);
    dst->length = len;
    return UA_STATUSCODE_GOOD;
}

UA_Boolean UA_String_equal(const UA_String *string1, const UA_String *string2) {
    if(string1->length <= 0 && string2->length <= 0)
        return UA_TRUE;
    if(string1->length != string2->length)
        return UA_FALSE;

    // casts are needed to overcome signed warnings
    UA_Int32 is = memcmp((char const *)string1->data, (char const *)string2->data, (size_t)string1->length);
    return (is == 0) ? UA_TRUE : UA_FALSE;
}

/* DateTime */
#define UNIX_EPOCH_BIAS_SEC 11644473600LL // Number of seconds from 1 Jan. 1601 00:00 to 1 Jan 1970 00:00 UTC
#define HUNDRED_NANOSEC_PER_USEC 10LL
#define HUNDRED_NANOSEC_PER_SEC (HUNDRED_NANOSEC_PER_USEC * 1000000LL)

#ifdef __MINGW32__
#ifndef _TIMEZONE_DEFINED
#define _TIMEZONE_DEFINED
struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};
#endif
#endif
#ifdef _WIN32
static const unsigned __int64 epoch = 116444736000000000;
int gettimeofday(struct timeval *tp, struct timezone *tzp);
int gettimeofday(struct timeval *tp, struct timezone *tzp) {
    FILETIME       ft;
    SYSTEMTIME     st;
    ULARGE_INTEGER ul;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);
    ul.LowPart  = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    tp->tv_sec  = (ul.QuadPart - epoch) / 10000000L;
    tp->tv_usec = st.wMilliseconds * 1000;
    return 0;
}
#endif

UA_DateTime UA_DateTime_now() {
    UA_DateTime    dateTime;
    struct timeval tv;
    gettimeofday(&tv, UA_NULL);
    dateTime = (tv.tv_sec + UNIX_EPOCH_BIAS_SEC)
               * HUNDRED_NANOSEC_PER_SEC + tv.tv_usec * HUNDRED_NANOSEC_PER_USEC;
    return dateTime;
}

UA_DateTimeStruct UA_DateTime_toStruct(UA_DateTime atime) {
    UA_DateTimeStruct dateTimeStruct;
    //calcualting the the milli-, micro- and nanoseconds
    dateTimeStruct.nanoSec  = (UA_Int16)((atime % 10) * 100);
    dateTimeStruct.microSec = (UA_Int16)((atime % 10000) / 10);
    dateTimeStruct.milliSec = (UA_Int16)((atime % 10000000) / 10000);

    //calculating the unix time with #include <time.h>
    time_t secSinceUnixEpoch = (atime/10000000) - UNIX_EPOCH_BIAS_SEC;
    struct tm ts = *gmtime(&secSinceUnixEpoch);
    dateTimeStruct.sec    = (UA_Int16)ts.tm_sec;
    dateTimeStruct.min    = (UA_Int16)ts.tm_min;
    dateTimeStruct.hour   = (UA_Int16)ts.tm_hour;
    dateTimeStruct.day    = (UA_Int16)ts.tm_mday;
    dateTimeStruct.month  = (UA_Int16)(ts.tm_mon + 1);
    dateTimeStruct.year   = (UA_Int16)(ts.tm_year + 1900);
    return dateTimeStruct;
}

UA_StatusCode UA_DateTime_toString(UA_DateTime atime, UA_String *timeString) {
    // length of the string is 31 (incl. \0 at the end)
    if(!(timeString->data = UA_malloc(32)))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    timeString->length = 31;

    UA_DateTimeStruct tSt = UA_DateTime_toStruct(atime);
    sprintf((char*)timeString->data, "%02d/%02d/%04d %02d:%02d:%02d.%03d.%03d.%03d", tSt.month, tSt.day, tSt.year,
            tSt.hour, tSt.min, tSt.sec, tSt.milliSec, tSt.microSec, tSt.nanoSec);
    return UA_STATUSCODE_GOOD;
}

/* Guid */
UA_TYPE_NEW_DEFAULT(UA_Guid)
UA_TYPE_DELETEMEMBERS_NOACTION(UA_Guid)
UA_TYPE_DELETE_DEFAULT(UA_Guid)

UA_Boolean UA_Guid_equal(const UA_Guid *g1, const UA_Guid *g2) {
    if(memcmp(g1, g2, sizeof(UA_Guid)) == 0)
        return UA_TRUE;
    return UA_FALSE;
}

UA_Guid UA_Guid_random(UA_UInt32 *seed) {
    UA_Guid result;
    result.data1 = RAND(seed);
    UA_UInt32 r = RAND(seed);
    result.data2 = (UA_UInt16) r;
    result.data3 = (UA_UInt16) (r >> 16);
    r = RAND(seed);
    result.data4[0] = (UA_Byte)r;
    result.data4[1] = (UA_Byte)(r >> 4);
    result.data4[2] = (UA_Byte)(r >> 8);
    result.data4[3] = (UA_Byte)(r >> 12);
    r = RAND(seed);
    result.data4[4] = (UA_Byte)r;
    result.data4[5] = (UA_Byte)(r >> 4);
    result.data4[6] = (UA_Byte)(r >> 8);
    result.data4[7] = (UA_Byte)(r >> 12);
    return result;
}

void UA_Guid_init(UA_Guid *p) {
    p->data1 = 0;
    p->data2 = 0;
    p->data3 = 0;
    memset(p->data4, 0, sizeof(UA_Byte)*8);
}

UA_StatusCode UA_Guid_copy(UA_Guid const *src, UA_Guid *dst) {
    UA_memcpy((void *)dst, (const void *)src, sizeof(UA_Guid));
    return UA_STATUSCODE_GOOD;
}

/* ByteString */
UA_StatusCode UA_ByteString_newMembers(UA_ByteString *p, UA_Int32 length) {
    if(length > 0) {
        if(!(p->data = UA_malloc((UA_UInt32)length)))
            return UA_STATUSCODE_BADOUTOFMEMORY;
        p->length = length;
    } else {
        p->data = UA_NULL;
        if(length == 0)
            p->length = 0;
        else
            p->length = -1;
    }
    return UA_STATUSCODE_GOOD;
}

/* XmlElement */

/* NodeId */
UA_TYPE_NEW_DEFAULT(UA_NodeId)
UA_TYPE_DELETE_DEFAULT(UA_NodeId)

void UA_NodeId_init(UA_NodeId *p) {
    p->identifierType = UA_NODEIDTYPE_NUMERIC;
    p->namespaceIndex = 0;
    memset(&p->identifier, 0, sizeof(p->identifier));
}

UA_StatusCode UA_NodeId_copy(UA_NodeId const *src, UA_NodeId *dst) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    switch(src->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        *dst = *src;
        return UA_STATUSCODE_GOOD;
    case UA_NODEIDTYPE_STRING: // Table 6, second entry
        retval |= UA_String_copy(&src->identifier.string, &dst->identifier.string);
        break;
    case UA_NODEIDTYPE_GUID: // Table 6, third entry
        retval |= UA_Guid_copy(&src->identifier.guid, &dst->identifier.guid);
        break;
    case UA_NODEIDTYPE_BYTESTRING: // Table 6, "OPAQUE"
        retval |= UA_ByteString_copy(&src->identifier.byteString, &dst->identifier.byteString);
        break;
    default:
        UA_NodeId_init(dst);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    dst->namespaceIndex = src->namespaceIndex;
    dst->identifierType = src->identifierType;
    if(retval) {
        UA_NodeId_deleteMembers(dst);
        UA_NodeId_init(dst);
    }
    return retval;
}

void UA_NodeId_deleteMembers(UA_NodeId *p) {
    switch(p->identifierType) {
    case UA_NODEIDTYPE_STRING:
    case UA_NODEIDTYPE_BYTESTRING:
        UA_ByteString_deleteMembers(&p->identifier.byteString);
        break;
    default:
        break;
    }
}

UA_Boolean UA_NodeId_equal(const UA_NodeId *n1, const UA_NodeId *n2) {
	if(n1->namespaceIndex != n2->namespaceIndex || n1->identifierType!=n2->identifierType)
        return UA_FALSE;

    switch(n1->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        if(n1->identifier.numeric == n2->identifier.numeric)
            return UA_TRUE;
        else
            return UA_FALSE;

    case UA_NODEIDTYPE_STRING:
        return UA_String_equal(&n1->identifier.string, &n2->identifier.string);

    case UA_NODEIDTYPE_GUID:
        return UA_Guid_equal(&n1->identifier.guid, &n2->identifier.guid);

    case UA_NODEIDTYPE_BYTESTRING:
        return UA_ByteString_equal(&n1->identifier.byteString, &n2->identifier.byteString);
    }
    return UA_FALSE;
}

UA_Boolean UA_NodeId_isNull(const UA_NodeId *p) {
    switch(p->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        if(p->namespaceIndex != 0 || p->identifier.numeric != 0)
            return UA_FALSE;
        break;

    case UA_NODEIDTYPE_STRING:
        if(p->namespaceIndex != 0 || p->identifier.string.length > 0)
            return UA_FALSE;
        break;

    case UA_NODEIDTYPE_GUID:
        if(p->namespaceIndex != 0 ||
           memcmp(&p->identifier.guid, (char[sizeof(UA_Guid)]) { 0 }, sizeof(UA_Guid)) != 0)
            return UA_FALSE;
        break;

    case UA_NODEIDTYPE_BYTESTRING:
        if(p->namespaceIndex != 0 || p->identifier.byteString.length > 0)
            return UA_FALSE;
        break;

    default:
        return UA_FALSE;
    }
    return UA_TRUE;
}

UA_NodeId UA_NodeId_fromInteger(UA_UInt16 nsIndex, UA_Int32 identifier) {
    return (UA_NodeId) { .namespaceIndex = nsIndex, .identifierType = UA_NODEIDTYPE_NUMERIC,
                         .identifier.numeric = identifier };
}

UA_NodeId UA_NodeId_fromCharString(UA_UInt16 nsIndex, char identifier[]) {
    return (UA_NodeId) { .namespaceIndex = nsIndex, .identifierType = UA_NODEIDTYPE_STRING,
                         .identifier.string = UA_STRING(identifier) };
}

UA_NodeId UA_NodeId_fromCharStringCopy(UA_UInt16 nsIndex, char const identifier[]) {
    return (UA_NodeId) {.namespaceIndex = nsIndex, .identifierType = UA_NODEIDTYPE_STRING,
                        .identifier.string = UA_STRING_ALLOC(identifier) };
}

UA_NodeId UA_NodeId_fromString(UA_UInt16 nsIndex, UA_String identifier) {
    return (UA_NodeId) { .namespaceIndex = nsIndex, .identifierType = UA_NODEIDTYPE_STRING,
                         .identifier.string = identifier };
}

UA_NodeId UA_NodeId_fromStringCopy(UA_UInt16 nsIndex, UA_String identifier) {
    UA_NodeId id;
    id.namespaceIndex = nsIndex;
    id.identifierType = UA_NODEIDTYPE_STRING;
    UA_String_copy(&identifier, &id.identifier.string);
    return id;
}

UA_NodeId UA_NodeId_fromGuid(UA_UInt16 nsIndex, UA_Guid identifier) {
    return (UA_NodeId) { .namespaceIndex = nsIndex, .identifierType = UA_NODEIDTYPE_GUID,
                         .identifier.guid = identifier };
}

UA_NodeId UA_NodeId_fromCharByteString(UA_UInt16 nsIndex, char identifier[]) {
    return (UA_NodeId) { .namespaceIndex = nsIndex, .identifierType = UA_NODEIDTYPE_BYTESTRING,
                         .identifier.byteString = UA_STRING(identifier) };
}

UA_NodeId UA_NodeId_fromCharByteStringCopy(UA_UInt16 nsIndex, char const identifier[]) {
    return (UA_NodeId) { .namespaceIndex = nsIndex, .identifierType = UA_NODEIDTYPE_BYTESTRING,
                         .identifier.byteString = UA_STRING_ALLOC(identifier) };
}

UA_NodeId UA_NodeId_fromByteString(UA_UInt16 nsIndex, UA_ByteString identifier) {
    return (UA_NodeId) { .namespaceIndex = nsIndex, .identifierType = UA_NODEIDTYPE_BYTESTRING,
                         .identifier.byteString = identifier };
}

UA_NodeId UA_NodeId_fromByteStringCopy(UA_UInt16 nsIndex, UA_ByteString identifier) {
    UA_NodeId id;
    id.namespaceIndex = nsIndex;
    id.identifierType = UA_NODEIDTYPE_BYTESTRING;
    UA_ByteString_copy(&identifier, &id.identifier.byteString);
    return id;
}

/* ExpandedNodeId */
UA_TYPE_NEW_DEFAULT(UA_ExpandedNodeId)
UA_TYPE_DELETE_DEFAULT(UA_ExpandedNodeId)

void UA_ExpandedNodeId_deleteMembers(UA_ExpandedNodeId *p) {
    UA_NodeId_deleteMembers(&p->nodeId);
    UA_String_deleteMembers(&p->namespaceUri);
}

void UA_ExpandedNodeId_init(UA_ExpandedNodeId *p) {
    UA_NodeId_init(&p->nodeId);
    UA_String_init(&p->namespaceUri);
    p->serverIndex = 0;
}

UA_StatusCode UA_ExpandedNodeId_copy(UA_ExpandedNodeId const *src, UA_ExpandedNodeId *dst) {
    UA_StatusCode retval = UA_NodeId_copy(&src->nodeId, &dst->nodeId);
    retval |= UA_String_copy(&src->namespaceUri, &dst->namespaceUri);
    dst->serverIndex = src->serverIndex;
    if(retval) {
        UA_ExpandedNodeId_deleteMembers(dst);
        UA_ExpandedNodeId_init(dst);
    }
    return retval;
}

UA_Boolean UA_ExpandedNodeId_isNull(const UA_ExpandedNodeId *p) {
    return UA_NodeId_isNull(&p->nodeId);
}

/* StatusCode */

/* QualifiedName */
UA_TYPE_NEW_DEFAULT(UA_QualifiedName)
UA_TYPE_DELETE_DEFAULT(UA_QualifiedName)

void UA_QualifiedName_deleteMembers(UA_QualifiedName *p) {
    UA_String_deleteMembers(&p->name);
}

void UA_QualifiedName_init(UA_QualifiedName *p) {
    UA_String_init(&p->name);
    p->namespaceIndex = 0;
}

UA_StatusCode UA_QualifiedName_copy(UA_QualifiedName const *src, UA_QualifiedName *dst) {
    UA_StatusCode retval = UA_String_copy(&src->name, &dst->name);
    dst->namespaceIndex = src->namespaceIndex;
    if(retval) {
        UA_QualifiedName_deleteMembers(dst);
        UA_QualifiedName_init(dst);
    }
    return retval;
}

/* LocalizedText */
UA_TYPE_NEW_DEFAULT(UA_LocalizedText)
UA_TYPE_DELETE_DEFAULT(UA_LocalizedText)

void UA_LocalizedText_deleteMembers(UA_LocalizedText *p) {
    UA_String_deleteMembers(&p->locale);
    UA_String_deleteMembers(&p->text);
}

void UA_LocalizedText_init(UA_LocalizedText *p) {
    UA_String_init(&p->locale);
    UA_String_init(&p->text);
}

UA_StatusCode UA_LocalizedText_copy(UA_LocalizedText const *src, UA_LocalizedText *dst) {
    UA_StatusCode retval = UA_String_copy(&src->locale, &dst->locale);
    retval |= UA_String_copy(&src->text, &dst->text);
    if(retval) {
        UA_LocalizedText_deleteMembers(dst);
        UA_LocalizedText_init(dst);
    }
    return retval;
}

/* ExtensionObject */
UA_TYPE_NEW_DEFAULT(UA_ExtensionObject)
UA_TYPE_DELETE_DEFAULT(UA_ExtensionObject)

void UA_ExtensionObject_deleteMembers(UA_ExtensionObject *p) {
    UA_NodeId_deleteMembers(&p->typeId);
    UA_ByteString_deleteMembers(&p->body);
}

void UA_ExtensionObject_init(UA_ExtensionObject *p) {
    UA_NodeId_init(&p->typeId);
    p->encoding = UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED;
    UA_ByteString_init(&p->body);
}

UA_StatusCode UA_ExtensionObject_copy(UA_ExtensionObject const *src, UA_ExtensionObject *dst) {
    UA_StatusCode retval = UA_ByteString_copy(&src->body, &dst->body);
    retval |= UA_NodeId_copy(&src->typeId, &dst->typeId);
    dst->encoding = src->encoding;
    if(retval) {
        UA_ExtensionObject_deleteMembers(dst);
        UA_ExtensionObject_init(dst);
    }
    return retval;
}

/* DataValue */
UA_TYPE_NEW_DEFAULT(UA_DataValue)
UA_TYPE_DELETE_DEFAULT(UA_DataValue)

void UA_DataValue_deleteMembers(UA_DataValue *p) {
    UA_Variant_deleteMembers(&p->value);
}

void UA_DataValue_init(UA_DataValue *p) {
    *((UA_Byte*)p) = 0; // zero out the bitfield
    p->serverPicoseconds = 0;
    UA_DateTime_init(&p->serverTimestamp);
    p->sourcePicoseconds = 0;
    UA_DateTime_init(&p->sourceTimestamp);
    UA_StatusCode_init(&p->status);
    UA_Variant_init(&p->value);
}

UA_StatusCode UA_DataValue_copy(UA_DataValue const *src, UA_DataValue *dst) {
    UA_DataValue_init(dst);
    *((UA_Byte*)dst) = *((const UA_Byte*)src); // the bitfield
    UA_StatusCode retval = UA_DateTime_copy(&src->serverTimestamp, &dst->serverTimestamp);
    retval |= UA_DateTime_copy(&src->sourceTimestamp, &dst->sourceTimestamp);
    retval |= UA_Variant_copy(&src->value, &dst->value);
    dst->serverPicoseconds = src->serverPicoseconds;
    dst->sourcePicoseconds = src->sourcePicoseconds;
    dst->status = src->status;
    if(retval) {
        UA_DataValue_deleteMembers(dst);
        UA_DataValue_init(dst);
    }
    return retval;
}

/* Variant */
UA_TYPE_NEW_DEFAULT(UA_Variant)
UA_TYPE_DELETE_DEFAULT(UA_Variant)

void UA_Variant_init(UA_Variant *p) {
    p->storageType = UA_VARIANT_DATA;
    p->arrayLength = -1;
    p->data = UA_NULL;
    p->arrayDimensions = UA_NULL;
    p->arrayDimensionsSize = -1;
    p->type = &UA_TYPES[UA_TYPES_BOOLEAN];
}

void UA_Variant_deleteMembers(UA_Variant *p) {
    if(p->storageType != UA_VARIANT_DATA)
        return;
    if(p->data) {
        if(p->arrayLength == -1)
            p->arrayLength = 1;
        UA_Array_delete(p->data, p->type, p->arrayLength);
        p->data = UA_NULL;
        p->arrayLength = -1;
    }
    if(p->arrayDimensions) {
        UA_free(p->arrayDimensions);
        p->arrayDimensions = UA_NULL;
        p->arrayDimensionsSize = -1;
    }
}

UA_StatusCode UA_Variant_copy(UA_Variant const *src, UA_Variant *dst) {
    UA_Variant_init(dst);
    UA_Int32 elements = src->arrayLength;
    if(UA_Variant_isScalar(src))
        elements = 1;
    UA_StatusCode retval = UA_Array_copy(src->data, &dst->data, src->type, elements);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Variant_deleteMembers(dst);
        UA_Variant_init(dst);
        return retval;
    }
    dst->arrayLength = src->arrayLength;
    dst->type = src->type;
    dst->storageType = UA_VARIANT_DATA;

    if(src->arrayDimensions) {
        retval |= UA_Array_copy(src->arrayDimensions, (void **)&dst->arrayDimensions,
                                &UA_TYPES[UA_TYPES_INT32], src->arrayDimensionsSize);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_Variant_deleteMembers(dst);
            UA_Variant_init(dst);
            return retval;
        }
        dst->arrayDimensionsSize = src->arrayDimensionsSize;
    }
    return retval;
}

UA_Boolean UA_Variant_isScalar(const UA_Variant *v) {
    return (v->data != UA_NULL && v->arrayLength == -1);
}

/**
 * Tests if a range is compatible with a variant. If yes, the following values are set:
 * - total: how many elements are indicated by the range
 * - block_size: how big is each contiguous block of elements in the variant denoted by the range
 * - block_distance: how many elements are between the blocks (beginning to beginning)
 * - first_elem: where does the first block begin
 */
static UA_StatusCode
testRangeWithVariant(const UA_Variant *v, const UA_NumericRange range, size_t *total,
                     size_t *block_size, size_t *block_distance, size_t *first_elem)
{
    /* Test the integrity of the source variant dimensions */
    UA_Int32 dims_count = 1;
    const UA_Int32 *dims = &v->arrayLength; // default: the array has only one dimension
    if(v->arrayDimensionsSize > 0) {
        dims_count = v->arrayDimensionsSize;
        dims = v->arrayDimensions;
        UA_Int32 elements = 1;
        for(UA_Int32 i = 0; i < dims_count; i++)
            elements *= dims[i];
        if(elements != v->arrayLength)
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Test the integrity of the range */
    size_t count = 1;
    if(range.dimensionsSize != dims_count)
        return UA_STATUSCODE_BADINDEXRANGEINVALID;
    for(UA_Int32 i = 0; i < dims_count; i++) {
        if(range.dimensions[i].min > range.dimensions[i].max)
            return UA_STATUSCODE_BADINDEXRANGEINVALID;
        if(range.dimensions[i].max > (UA_UInt32)*(dims+i))
            return UA_STATUSCODE_BADINDEXRANGENODATA;
        count *= (range.dimensions[i].max - range.dimensions[i].min) + 1;
    }

    /* Compute the block size and the position of the first element */
    size_t bs = 0;
    size_t bd = 0;
    size_t fe = 0;
    size_t running_dimssize = 1; // elements per block of dimensions k to k_max
    UA_Boolean found_contiguous = UA_FALSE;
    for(UA_Int32 k = dims_count - 1; k >= 0; k--) {
        if(!found_contiguous && (range.dimensions[k].min != 0 ||
                                 range.dimensions[k].max + 1 != (UA_UInt32)dims[k])) {
            found_contiguous = UA_TRUE;
            bs = (range.dimensions[k].max - range.dimensions[k].min + 1) * running_dimssize;
            bd = dims[k] * running_dimssize;
        } 
        fe += running_dimssize * range.dimensions[k].min;
        running_dimssize *= dims[k];
    }
    *total = count;
    *block_size = bs;
    *block_distance = bd;
    *first_elem = fe;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Variant_copyRange(const UA_Variant *src, UA_Variant *dst, const UA_NumericRange range) {
    size_t count, block_size, block_distance, first_elem;
    UA_StatusCode retval = testRangeWithVariant(src, range, &count, &block_size, &block_distance, &first_elem);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_Variant_init(dst);
    size_t elem_size = src->type->memSize;
    dst->data = UA_malloc(elem_size * count);
    if(!dst->data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Copy the range */
    size_t block_count = count / block_size;
    uintptr_t nextdst = (uintptr_t)dst->data;
    uintptr_t nextsrc = (uintptr_t)src->data + (elem_size * first_elem);
    if(src->type->fixedSize) {
        for(size_t i = 0; i < block_count; i++) {
            memcpy((void*)nextdst, (void*)nextsrc, elem_size * block_size);
            nextdst += block_size * elem_size;
            nextsrc += block_distance * elem_size;
        }
    } else {
        for(size_t i = 0; i < block_count; i++) {
            for(size_t j = 0; j < block_size && retval == UA_STATUSCODE_GOOD; j++) {
                retval = UA_copy((const void*)nextsrc, (void*)nextdst, src->type);
                nextdst += elem_size;
                nextsrc += elem_size;
            }
            nextsrc += (block_distance - block_size) * elem_size;
        }
        if(retval != UA_STATUSCODE_GOOD) {
            size_t copied = ((nextdst - elem_size) - (uintptr_t)dst->data) / elem_size;
            UA_Array_delete(dst->data, src->type, copied);
            return retval;
        }
    }

    /* Copy the range dimensions*/
    if(src->arrayDimensionsSize > 0) {
        dst->arrayDimensions = UA_malloc(sizeof(UA_Int32) * src->arrayDimensionsSize);
        if(!dst->arrayDimensions) {
            UA_Array_delete(dst->data, src->type, count);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        for(UA_Int32 k = 0; k < src->arrayDimensionsSize; k++)
            dst->arrayDimensions[k] = range.dimensions[k].max - range.dimensions[k].min + 1;
        dst->arrayDimensionsSize = src->arrayDimensionsSize;
    }
    dst->arrayLength = count;
    dst->type = src->type;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Variant_setRange(UA_Variant *v, void *dataArray, UA_Int32 dataArraySize,
                                  const UA_NumericRange range) {
    size_t count, block_size, block_distance, first_elem;
    UA_StatusCode retval = testRangeWithVariant(v, range, &count, &block_size, &block_distance, &first_elem);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if((UA_Int32)count != dataArraySize)
        return UA_STATUSCODE_BADINDEXRANGEINVALID;

    size_t block_count = count / block_size;
    size_t elem_size = v->type->memSize;
    uintptr_t nextdst = (uintptr_t)v->data + (first_elem * elem_size);
    uintptr_t nextsrc = (uintptr_t)dataArray;
    for(size_t i = 0; i < block_count; i++) {
        if(!v->type->fixedSize) {
            for(size_t j = 0; j < block_size; j++) {
                UA_deleteMembers((void*)nextdst, v->type);
                nextdst += elem_size;
            }
            nextdst -= block_size * elem_size;
        }
        memcpy((void*)nextdst, (void*)nextsrc, elem_size * block_size);
        nextsrc += block_size * elem_size;
        nextdst += block_distance * elem_size;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_EXPORT UA_Variant_setRangeCopy(UA_Variant *v, const void *dataArray, UA_Int32 dataArraySize,
                                                const UA_NumericRange range) {
    size_t count, block_size, block_distance, first_elem;
    UA_StatusCode retval = testRangeWithVariant(v, range, &count, &block_size, &block_distance, &first_elem);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if((UA_Int32)count != dataArraySize)
        return UA_STATUSCODE_BADINDEXRANGEINVALID;

    size_t block_count = count / block_size;
    size_t elem_size = v->type->memSize;
    uintptr_t nextdst = (uintptr_t)v->data + (first_elem * elem_size);
    uintptr_t nextsrc = (uintptr_t)dataArray;
    if(v->type->fixedSize) {
        for(size_t i = 0; i < block_count; i++) {
            memcpy((void*)nextdst, (void*)nextsrc, elem_size * block_size);
            nextsrc += block_size * elem_size;
            nextdst += block_distance * elem_size;
        }
    } else {
        for(size_t i = 0; i < block_count; i++) {
            for(size_t j = 0; j < block_size; j++) {
                UA_deleteMembers((void*)nextdst, v->type);
                UA_copy((void*)nextsrc, (void*)nextdst, v->type);
                nextdst += elem_size;
                nextsrc += elem_size;
            }
            nextdst += (block_distance - block_size) * elem_size;
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Variant_setScalar(UA_Variant *v, void *p, const UA_DataType *type) {
    return UA_Variant_setArray(v, p, -1, type);
}

UA_StatusCode UA_Variant_setScalarCopy(UA_Variant *v, const void *p, const UA_DataType *type) {
    void *new = UA_malloc(type->memSize);
    if(!new)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_StatusCode retval = UA_copy(p, new, type);
	if(retval != UA_STATUSCODE_GOOD) {
		UA_delete(new, type);
		return retval;
	}
    return UA_Variant_setArray(v, new, -1, type);
}

UA_StatusCode UA_Variant_setArray(UA_Variant *v, void *array, UA_Int32 elements,
                                  const UA_DataType *type) {
    v->type = type;
    v->arrayLength = elements;
    v->data = array;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Variant_setArrayCopy(UA_Variant *v, const void *array, UA_Int32 elements,
                                      const UA_DataType *type) {
    void *new;
    UA_StatusCode retval = UA_Array_copy(array, &new, type, elements);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    return UA_Variant_setArray(v, new, elements, type);
}

/* DiagnosticInfo */
UA_TYPE_NEW_DEFAULT(UA_DiagnosticInfo)
UA_TYPE_DELETE_DEFAULT(UA_DiagnosticInfo)

void UA_DiagnosticInfo_deleteMembers(UA_DiagnosticInfo *p) {
    UA_String_deleteMembers(&p->additionalInfo);
    if(p->hasInnerDiagnosticInfo && p->innerDiagnosticInfo) {
        UA_DiagnosticInfo_delete(p->innerDiagnosticInfo);
        p->innerDiagnosticInfo = UA_NULL;
    }
}

void UA_DiagnosticInfo_init(UA_DiagnosticInfo *p) {
	*((UA_Byte*)p) = 0; // zero out the bitfield
    p->symbolicId          = 0;
    p->namespaceUri        = 0;
    p->localizedText       = 0;
    p->locale              = 0;
    UA_String_init(&p->additionalInfo);
    p->innerDiagnosticInfo = UA_NULL;
    UA_StatusCode_init(&p->innerStatusCode);
}

UA_StatusCode UA_DiagnosticInfo_copy(UA_DiagnosticInfo const *src, UA_DiagnosticInfo *dst) {
    UA_DiagnosticInfo_init(dst);
    *((UA_Byte*)dst) = *((const UA_Byte*)src); // the bitfield
    dst->symbolicId = src->symbolicId;
    dst->namespaceUri = src->namespaceUri;
    dst->localizedText = src->localizedText;
    dst->locale = src->locale;
    dst->innerStatusCode = src->innerStatusCode;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(src->hasAdditionalInfo)
       retval = UA_String_copy(&src->additionalInfo, &dst->additionalInfo);
    if(src->hasInnerDiagnosticInfo && src->innerDiagnosticInfo) {
        if((dst->innerDiagnosticInfo = UA_malloc(sizeof(UA_DiagnosticInfo)))) {
            retval |= UA_DiagnosticInfo_copy(src->innerDiagnosticInfo, dst->innerDiagnosticInfo);
            dst->hasInnerDiagnosticInfo = src->hasInnerDiagnosticInfo;
        }
        else
            retval |= UA_STATUSCODE_BADOUTOFMEMORY;
    }
    if(retval) {
        UA_DiagnosticInfo_deleteMembers(dst);
        UA_DiagnosticInfo_init(dst);
    }
    return retval;
}

/*******************/
/* Structure Types */
/*******************/

void UA_init(void *p, const UA_DataType *dataType) {
    /* Do not check if the index is a builtin-type here. Builtins will be called
       with their very own _init functions normally. In the off case, that the
       generic function is called with the index of a builtin, their layout
       contains a single member of the builtin type, that will be inited in the
       for loop. */

    uintptr_t ptr = (uintptr_t)p;
    UA_Byte membersSize = dataType->membersSize;
    for(size_t i=0;i<membersSize; i++) {
        const UA_DataTypeMember *member = &dataType->members[i];
        if(member->isArray) {
            /* Padding contains bit-magic to split into padding before and after
               the length integer */
            ptr += (member->padding >> 3);
            *((UA_Int32*)ptr) = -1;
            ptr += sizeof(UA_Int32) + (member->padding & 0x07);
            *((void**)ptr) = UA_NULL;
            ptr += sizeof(void*);
            continue;
        }

        ptr += member->padding;
        if(!member->namespaceZero) {
            // pointer arithmetic
            const UA_DataType *memberType = &dataType[member->memberTypeIndex - dataType->typeIndex];
            UA_init((void*)ptr, memberType);
            ptr += memberType->memSize;
            continue;
        }

        switch(member->memberTypeIndex) {
        case UA_TYPES_BOOLEAN:
        case UA_TYPES_SBYTE:
        case UA_TYPES_BYTE:
            *(UA_Byte*)ptr = 0;
            break;
        case UA_TYPES_INT16:
        case UA_TYPES_UINT16:
            *(UA_Int16*)ptr = 0;
            break;
        case UA_TYPES_INT32:
        case UA_TYPES_UINT32:
        case UA_TYPES_STATUSCODE:
        case UA_TYPES_FLOAT:
            *(UA_Int32*)ptr = 0;
            break;
        case UA_TYPES_INT64:
        case UA_TYPES_UINT64:
        case UA_TYPES_DOUBLE:
        case UA_TYPES_DATETIME:
            *(UA_Int64*)ptr = 0;
            break;
        case UA_TYPES_GUID:
            UA_Guid_init((UA_Guid*)ptr);
            break;
        case UA_TYPES_NODEID:
            UA_NodeId_init((UA_NodeId*)ptr);
            break;
        case UA_TYPES_EXPANDEDNODEID:
            UA_ExpandedNodeId_init((UA_ExpandedNodeId*)ptr);
            break;
        case UA_TYPES_LOCALIZEDTEXT:
            UA_LocalizedText_init((UA_LocalizedText*)ptr);
            break;
        case UA_TYPES_EXTENSIONOBJECT:
            UA_ExtensionObject_init((UA_ExtensionObject*)ptr);
            break;
        case UA_TYPES_DATAVALUE:
            UA_DataValue_init((UA_DataValue*)ptr);
            break;
        case UA_TYPES_VARIANT:
            UA_Variant_init((UA_Variant*)ptr);
            break;
        case UA_TYPES_DIAGNOSTICINFO:
            UA_DiagnosticInfo_init((UA_DiagnosticInfo*)ptr);
            break;
        default:
            // QualifiedName, LocalizedText and strings are treated as structures, also
            UA_init((void*)ptr, &UA_TYPES[member->memberTypeIndex]);
        }
        ptr += UA_TYPES[member->memberTypeIndex].memSize;
    }
}

void * UA_new(const UA_DataType *dataType) {
    void *p = UA_malloc(dataType->memSize);
    if(p)
        UA_init(p, dataType);
    return p;
}

UA_StatusCode UA_copy(const void *src, void *dst, const UA_DataType *dataType) {
    if(dataType->fixedSize) {
        memcpy(dst, src, dataType->memSize);
        return UA_STATUSCODE_GOOD;
    }
    UA_init(dst, dataType);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    uintptr_t ptrs = (uintptr_t)src;
    uintptr_t ptrd = (uintptr_t)dst;
    UA_Byte membersSize = dataType->membersSize;
    for(size_t i=0;i<membersSize; i++) {
        const UA_DataTypeMember *member = &dataType->members[i];
        const UA_DataType *memberType;
        if(member->namespaceZero)
            memberType = &UA_TYPES[member->memberTypeIndex];
        else
            memberType = &dataType[member->memberTypeIndex - dataType->typeIndex];

        if(member->isArray) {
            ptrs += (member->padding >> 3);
            ptrd += (member->padding >> 3);
            UA_Int32 *dstNoElements = (UA_Int32*)ptrd;
            const UA_Int32 elements = *((const UA_Int32*)ptrs);
            ptrs += sizeof(UA_Int32) + (member->padding & 0x07);
            ptrd += sizeof(UA_Int32) + (member->padding & 0x07);
            retval = UA_Array_copy(*(void* const*)ptrs, (void**)ptrd, memberType, elements);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_deleteMembers(dst, dataType);
                UA_init(dst, dataType);
                return retval;
            }
            *dstNoElements = elements;
            ptrs += sizeof(void*);
            ptrd += sizeof(void*);
            continue;
        }

        ptrs += member->padding;
        ptrd += member->padding;
        if(!member->namespaceZero) {
            retval = UA_copy((const void*)ptrs, (void*)ptrd, memberType);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_deleteMembers(dst, dataType);
                UA_init(dst, dataType);
                return retval;
            }
            ptrs += memberType->memSize;
            ptrd += memberType->memSize;
            continue;
        }

        switch(member->memberTypeIndex) {
        case UA_TYPES_BOOLEAN:
        case UA_TYPES_SBYTE:
        case UA_TYPES_BYTE:
            *((UA_Byte*)ptrd) = *((const UA_Byte*)ptrs);
            break;
        case UA_TYPES_INT16:
        case UA_TYPES_UINT16:
            *((UA_Int16*)ptrd) = *((const UA_Byte*)ptrs);
            break;
        case UA_TYPES_INT32:
        case UA_TYPES_UINT32:
        case UA_TYPES_STATUSCODE:
        case UA_TYPES_FLOAT:
            *((UA_Int32*)ptrd) = *((const UA_Int32*)ptrs);
            break;
        case UA_TYPES_INT64:
        case UA_TYPES_UINT64:
        case UA_TYPES_DOUBLE:
        case UA_TYPES_DATETIME:
            *((UA_Int64*)ptrd) = *((const UA_Int64*)ptrs);
            break;
        case UA_TYPES_GUID:
            *((UA_Guid*)ptrd) = *((const UA_Guid*)ptrs);
            break;
        case UA_TYPES_NODEID:
            retval |= UA_NodeId_copy((const UA_NodeId*)ptrs, (UA_NodeId*)ptrd);
            break;
        case UA_TYPES_EXPANDEDNODEID:
            retval |= UA_ExpandedNodeId_copy((const UA_ExpandedNodeId*)ptrs, (UA_ExpandedNodeId*)ptrd);
            break;
        case UA_TYPES_LOCALIZEDTEXT:
            retval |= UA_LocalizedText_copy((const UA_LocalizedText*)ptrs, (UA_LocalizedText*)ptrd);
            break;
        case UA_TYPES_EXTENSIONOBJECT:
            retval |= UA_ExtensionObject_copy((const UA_ExtensionObject*)ptrs, (UA_ExtensionObject*)ptrd);
            break;
        case UA_TYPES_DATAVALUE:
            retval |= UA_DataValue_copy((const UA_DataValue*)ptrs, (UA_DataValue*)ptrd);
            break;
        case UA_TYPES_VARIANT:
            retval |= UA_Variant_copy((const UA_Variant*)ptrs, (UA_Variant*)ptrd);
            break;
        case UA_TYPES_DIAGNOSTICINFO:
            retval |= UA_DiagnosticInfo_copy((const UA_DiagnosticInfo*)ptrs, (UA_DiagnosticInfo*)ptrd);
            break;
        default:
            // QualifiedName, LocalizedText and strings are treated as structures, also
            retval |= UA_copy((const void *)ptrs, (void*)ptrd, memberType);
        }
        ptrs += memberType->memSize;
        ptrd += memberType->memSize;
    }
    if(retval != UA_STATUSCODE_GOOD) {
        UA_deleteMembers(dst, dataType);
        UA_init(dst, dataType);
    }
    return retval;
}

void UA_deleteMembers(void *p, const UA_DataType *dataType) {
    UA_deleteMembersUntil(p, dataType, -1);
}

void UA_deleteMembersUntil(void *p, const UA_DataType *dataType, UA_Int32 lastMember) {
    uintptr_t ptr = (uintptr_t)p;
    if(dataType->fixedSize)
        return;
    UA_Byte membersSize = dataType->membersSize;
    for(size_t i=0;i<membersSize; i++) {
        if(lastMember > -1 && (UA_Int32)i > lastMember){
            return;
        }
        const UA_DataTypeMember *member = &dataType->members[i];
        const UA_DataType *memberType;
        if(member->namespaceZero)
            memberType = &UA_TYPES[member->memberTypeIndex];
        else
            memberType = &dataType[member->memberTypeIndex - dataType->typeIndex];

        if(member->isArray) {
            ptr += (member->padding >> 3);
            UA_Int32 elements = *((UA_Int32*)ptr);
            ptr += sizeof(UA_Int32) + (member->padding & 0x07);
            UA_Array_delete(*(void**)ptr, memberType, elements);
            *(void**)ptr = UA_NULL;
            ptr += sizeof(void*);
            continue;
        }

        ptr += member->padding;
        if(!member->namespaceZero) {
            UA_deleteMembers((void*)ptr, memberType);
            ptr += memberType->memSize;
            continue;
        }

        switch(member->memberTypeIndex) {
        case UA_TYPES_BOOLEAN:
        case UA_TYPES_SBYTE:
        case UA_TYPES_BYTE:
        case UA_TYPES_INT16:
        case UA_TYPES_UINT16:
        case UA_TYPES_INT32:
        case UA_TYPES_UINT32:
        case UA_TYPES_STATUSCODE:
        case UA_TYPES_FLOAT:
        case UA_TYPES_INT64:
        case UA_TYPES_UINT64:
        case UA_TYPES_DOUBLE:
        case UA_TYPES_DATETIME:
        case UA_TYPES_GUID:
            break;
        case UA_TYPES_NODEID:
            UA_NodeId_deleteMembers((UA_NodeId*)ptr);
            break;
        case UA_TYPES_EXPANDEDNODEID:
            UA_ExpandedNodeId_deleteMembers((UA_ExpandedNodeId*)ptr);
            break;
        case UA_TYPES_LOCALIZEDTEXT:
            UA_LocalizedText_deleteMembers((UA_LocalizedText*)ptr);
            break;
        case UA_TYPES_EXTENSIONOBJECT:
            UA_ExtensionObject_deleteMembers((UA_ExtensionObject*)ptr);
            break;
        case UA_TYPES_DATAVALUE:
            UA_DataValue_deleteMembers((UA_DataValue*)ptr);
            break;
        case UA_TYPES_VARIANT:
            UA_Variant_deleteMembers((UA_Variant*)ptr);
            break;
        case UA_TYPES_DIAGNOSTICINFO:
            UA_DiagnosticInfo_deleteMembers((UA_DiagnosticInfo*)ptr);
            break;
        default:
            // QualifiedName, LocalizedText and strings are treated as structures, also
            if(lastMember > -1){
                UA_deleteMembersUntil((void*)ptr, memberType, lastMember-i);
            }
            else
                UA_deleteMembers((void*)ptr, memberType);
        }
        ptr += memberType->memSize;
    }
}

void UA_delete(void *p, const UA_DataType *dataType) {
    UA_deleteMembers(p, dataType);
    UA_free(p);
}

/******************/
/* Array Handling */
/******************/

void* UA_Array_new(const UA_DataType *dataType, UA_Int32 elements) {
    if((UA_Int32)dataType->memSize * elements < 0 || dataType->memSize * elements > MAX_ARRAY_SIZE )
        return UA_NULL;

    if(dataType->fixedSize)
        return UA_calloc(elements, dataType->memSize);

    void *p = UA_malloc(dataType->memSize * (size_t)elements);
    if(!p)
        return p;

    uintptr_t ptr = (uintptr_t)p;
    for(int i = 0; i<elements; i++) {
        UA_init((void*)ptr, dataType);
        ptr += dataType->memSize;
    }
    return p;
}

UA_StatusCode UA_Array_copy(const void *src, void **dst, const UA_DataType *dataType, UA_Int32 elements) {
    if(elements <= 0) {
        *dst = UA_NULL;
        return UA_STATUSCODE_GOOD;
    }

    if(!(*dst = UA_malloc((size_t)elements * dataType->memSize)))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    if(dataType->fixedSize) {
        memcpy(*dst, src, dataType->memSize * (size_t)elements);
        return UA_STATUSCODE_GOOD;
    }

    uintptr_t ptrs = (uintptr_t)src;
    uintptr_t ptrd = (uintptr_t)*dst;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(int i = 0; i < elements; i++) {
        retval |= UA_copy((void*)ptrs, (void*)ptrd, dataType);
        ptrs += dataType->memSize;
        ptrd += dataType->memSize;
    }

    if(retval != UA_STATUSCODE_GOOD)
        UA_Array_delete(*dst, dataType, elements);

    return retval;
}

void UA_Array_delete(void *p, const UA_DataType *dataType, UA_Int32 elements) {
    if(!dataType->fixedSize) {
        uintptr_t ptr = (uintptr_t)p;
        for(UA_Int32 i = 0; i < elements; i++) {
            UA_deleteMembers((void*)ptr, dataType);
            ptr += dataType->memSize;
        }
    }
    UA_free(p);
}
