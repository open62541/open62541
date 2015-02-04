#include <stdarg.h> // va_start, va_end
#include <time.h>
#include <stdio.h> // printf
#include <string.h> // strlen
#define __USE_POSIX
#include <stdlib.h> // malloc, free, rand

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "ua_util.h"

#ifdef _WIN32
#define RAND(SEED) (UA_UInt32)rand()
#else
#define RAND(SEED) (UA_UInt32)rand_r(SEED)
#endif

#include "ua_types.h"
#include "ua_statuscodes.h"
#include "ua_types_generated.h"

/*****************/
/* Helper Macros */
/*****************/

#define UA_TYPE_DEFAULT(TYPE)            \
    UA_TYPE_NEW_DEFAULT(TYPE)            \
    UA_TYPE_INIT_DEFAULT(TYPE)           \
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
void UA_String_init(UA_String *p) {
    p->length = -1;
    p->data   = UA_NULL;
}

UA_TYPE_DELETE_DEFAULT(UA_String)
void UA_String_deleteMembers(UA_String *p) {
    UA_free(p->data);
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

/* The c-string needs to be null-terminated. the string cannot be smaller than zero. */
UA_Int32 UA_String_copycstring(char const *src, UA_String *dst) {
    UA_UInt32 length = (UA_UInt32) strlen(src);
    if(length == 0) {
        dst->length = 0;
        dst->data = UA_NULL;
        return UA_STATUSCODE_GOOD;
    }
    dst->data = UA_malloc(length);
    if(dst->data != UA_NULL) {
        UA_memcpy(dst->data, src, length);
        dst->length = (UA_Int32) (length & ~(1<<31)); // the highest bit is always zero to avoid overflows into negative values
    } else {
        dst->length = -1;
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    return UA_STATUSCODE_GOOD;
}

#define UA_STRING_COPYPRINTF_BUFSIZE 1024
UA_StatusCode UA_String_copyprintf(char const *fmt, UA_String *dst, ...) {
    char src[UA_STRING_COPYPRINTF_BUFSIZE];
    va_list ap;
    va_start(ap, dst);
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
    // vsnprintf should only take a literal and no variable to be secure
    UA_Int32 len = vsnprintf(src, UA_STRING_COPYPRINTF_BUFSIZE, fmt, ap);
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
    va_end(ap);
    if(len < 0)  // FIXME: old glibc 2.0 would return -1 when truncated
        return UA_STATUSCODE_BADINTERNALERROR;
    // since glibc 2.1 vsnprintf returns the len that would have resulted if buf were large enough
    len = ( len > UA_STRING_COPYPRINTF_BUFSIZE ? UA_STRING_COPYPRINTF_BUFSIZE : len );
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
    UA_Int32 is = strncmp((char const *)string1->data, (char const *)string2->data, (size_t)string1->length);
    return (is == 0) ? UA_TRUE : UA_FALSE;
}

void UA_String_printf(char const *label, const UA_String *string) {
    printf("%s {Length=%d, Data=%.*s}\n", label, string->length,
           string->length, (char *)string->data);
}

void UA_String_printx(char const *label, const UA_String *string) {
    printf("%s {Length=%d, Data=", label, string->length);
    if(string->length > 0) {
        for(UA_Int32 i = 0;i < string->length;i++) {
            printf("%c%d", i == 0 ? '{' : ',', (string->data)[i]);
            // if (i > 0 && !(i%20)) { printf("\n\t"); }
        }
    } else
        printf("{");
    printf("}}\n");
}

void UA_String_printx_hex(char const *label, const UA_String *string) {
    printf("%s {Length=%d, Data=", label, string->length);
    if(string->length > 0) {
        for(UA_Int32 i = 0;i < string->length;i++)
            printf("%c%x", i == 0 ? '{' : ',', (string->data)[i]);
    } else
        printf("{");
    printf("}}\n");
}

/* DateTime */
#define UNIX_EPOCH_BIAS_SEC 11644473600LL // Number of seconds from 1 Jan. 1601 00:00 to 1 Jan 1970 00:00 UTC
#define HUNDRED_NANOSEC_PER_USEC 10LL
#define HUNDRED_NANOSEC_PER_SEC (HUNDRED_NANOSEC_PER_USEC * 1000000LL)

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
    sprintf((char*)timeString->data, "%2d/%2d/%4d %2d:%2d:%2d.%3d.%3d.%3d", tSt.month, tSt.day, tSt.year,
            tSt.hour, tSt.min, tSt.sec, tSt.milliSec, tSt.microSec, tSt.nanoSec);
    return UA_STATUSCODE_GOOD;
}

/* Guid */
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

UA_TYPE_NEW_DEFAULT(UA_Guid)
UA_StatusCode UA_Guid_copy(UA_Guid const *src, UA_Guid *dst) {
    UA_memcpy((void *)dst, (const void *)src, sizeof(UA_Guid));
    return UA_STATUSCODE_GOOD;
}

/* ByteString */
UA_Boolean UA_ByteString_equal(const UA_ByteString *string1, const UA_ByteString *string2) {
    return UA_String_equal((const UA_String *)string1, (const UA_String *)string2);
}

void UA_ByteString_printf(char *label, const UA_ByteString *string) {
    UA_String_printf(label, (const UA_String *)string);
}

void UA_ByteString_printx(char *label, const UA_ByteString *string) {
    UA_String_printx(label, (const UA_String *)string);
}

void UA_ByteString_printx_hex(char *label, const UA_ByteString *string) {
    UA_String_printx_hex(label, (const UA_String *)string);
}

/** Creates a ByteString of the indicated length. The content is not set to zero. */
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
void UA_NodeId_init(UA_NodeId *p) {
    p->identifierType = UA_NODEIDTYPE_NUMERIC;
    p->namespaceIndex = 0;
    memset(&p->identifier, 0, sizeof(p->identifier));
}

UA_TYPE_NEW_DEFAULT(UA_NodeId)
UA_StatusCode UA_NodeId_copy(UA_NodeId const *src, UA_NodeId *dst) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    switch(src->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        *dst = *src;
        return UA_STATUSCODE_GOOD;
        break;

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
    dst->identifierType = src->identifierType;
    if(retval)
        UA_NodeId_deleteMembers(dst);
    return retval;
}

static UA_Boolean UA_NodeId_isBasicType(UA_NodeId const *id) {
    return id ->namespaceIndex == 0 && 1 <= id ->identifier.numeric &&
        id ->identifier.numeric <= 25;
}

UA_TYPE_DELETE_DEFAULT(UA_NodeId)
void UA_NodeId_deleteMembers(UA_NodeId *p) {
    switch(p->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        // nothing to do
        break;
    case UA_NODEIDTYPE_STRING: // Table 6, second entry
        UA_String_deleteMembers(&p->identifier.string);
        break;

    case UA_NODEIDTYPE_GUID: // Table 6, third entry
        UA_Guid_deleteMembers(&p->identifier.guid);
        break;

    case UA_NODEIDTYPE_BYTESTRING: // Table 6, "OPAQUE"
        UA_ByteString_deleteMembers(&p->identifier.byteString);
        break;
    }
}

UA_Boolean UA_NodeId_equal(const UA_NodeId *n1, const UA_NodeId *n2) {
    if(n1->namespaceIndex != n2->namespaceIndex)
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
        if(p->namespaceIndex != 0 || p->identifier.string.length != 0)
            return UA_FALSE;
        break;

    case UA_NODEIDTYPE_GUID:
        if(p->namespaceIndex != 0 ||
           memcmp(&p->identifier.guid, (char[sizeof(UA_Guid)]) { 0 }, sizeof(UA_Guid)) != 0)
            return UA_FALSE;
        break;

    case UA_NODEIDTYPE_BYTESTRING:
        if(p->namespaceIndex != 0 || p->identifier.byteString.length != 0)
            return UA_FALSE;
        break;

    default:
        return UA_FALSE;
    }
    return UA_TRUE;
}

/* ExpandedNodeId */
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

UA_TYPE_NEW_DEFAULT(UA_ExpandedNodeId)
UA_StatusCode UA_ExpandedNodeId_copy(UA_ExpandedNodeId const *src, UA_ExpandedNodeId *dst) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_String_copy(&src->namespaceUri, &dst->namespaceUri);
    retval |= UA_NodeId_copy(&src->nodeId, &dst->nodeId);
    dst->serverIndex = src->serverIndex;
    if(retval)
        UA_ExpandedNodeId_deleteMembers(dst);
    return retval;
}

UA_Boolean UA_ExpandedNodeId_isNull(const UA_ExpandedNodeId *p) {
    return UA_NodeId_isNull(&p->nodeId);
}

/* StatusCode */

/* QualifiedName */
UA_TYPE_DELETE_DEFAULT(UA_QualifiedName)
void UA_QualifiedName_deleteMembers(UA_QualifiedName *p) {
    UA_String_deleteMembers(&p->name);
}

void UA_QualifiedName_init(UA_QualifiedName *p) {
    UA_String_init(&p->name);
    p->namespaceIndex = 0;
}

UA_TYPE_NEW_DEFAULT(UA_QualifiedName)
UA_StatusCode UA_QualifiedName_copy(UA_QualifiedName const *src, UA_QualifiedName *dst) {
    UA_StatusCode retval = UA_String_copy(&src->name, &dst->name);
    dst->namespaceIndex = src->namespaceIndex;
    if(retval)
        UA_QualifiedName_deleteMembers(dst);
    return retval;

}

UA_StatusCode UA_QualifiedName_copycstring(char const *src, UA_QualifiedName *dst) {
    dst->namespaceIndex = 0;
    return UA_String_copycstring(src, &dst->name);
}

/* LocalizedText */
UA_TYPE_DELETE_DEFAULT(UA_LocalizedText)
void UA_LocalizedText_deleteMembers(UA_LocalizedText *p) {
    UA_String_deleteMembers(&p->locale);
    UA_String_deleteMembers(&p->text);
}

void UA_LocalizedText_init(UA_LocalizedText *p) {
    UA_String_init(&p->locale);
    UA_String_init(&p->text);
}

UA_TYPE_NEW_DEFAULT(UA_LocalizedText)
UA_StatusCode UA_LocalizedText_copycstring(char const *src, UA_LocalizedText *dst) {
    UA_StatusCode retval = UA_String_copycstring("en", &dst->locale); // TODO: Are language codes upper case?
    retval |= UA_String_copycstring(src, &dst->text);
    if(retval) {
        UA_LocalizedText_deleteMembers(dst);
        UA_LocalizedText_init(dst);
    }
    return retval;
}

UA_StatusCode UA_LocalizedText_copy(UA_LocalizedText const *src, UA_LocalizedText *dst) {
    UA_Int32 retval = UA_String_copy(&src->locale, &dst->locale);
    retval |= UA_String_copy(&src->text, &dst->text);
    if(retval)
        UA_LocalizedText_deleteMembers(dst);
    return retval;
}

/* ExtensionObject */
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

UA_TYPE_NEW_DEFAULT(UA_ExtensionObject)
UA_StatusCode UA_ExtensionObject_copy(UA_ExtensionObject const *src, UA_ExtensionObject *dst) {
    UA_StatusCode retval = UA_ByteString_copy(&src->body, &dst->body);
    retval |= UA_NodeId_copy(&src->typeId, &dst->typeId);
    dst->encoding = src->encoding;
    if(retval)
        UA_ExtensionObject_deleteMembers(dst);
    return retval;
}

/* DataValue */
UA_TYPE_DELETE_DEFAULT(UA_DataValue)
void UA_DataValue_deleteMembers(UA_DataValue *p) {
    UA_Variant_deleteMembers(&p->value);
}

void UA_DataValue_init(UA_DataValue *p) {
    p->encodingMask      = 0;
    p->serverPicoseconds = 0;
    UA_DateTime_init(&p->serverTimestamp);
    p->sourcePicoseconds = 0;
    UA_DateTime_init(&p->sourceTimestamp);
    UA_StatusCode_init(&p->status);
    UA_Variant_init(&p->value);
}

UA_TYPE_NEW_DEFAULT(UA_DataValue)
UA_StatusCode UA_DataValue_copy(UA_DataValue const *src, UA_DataValue *dst) {
    UA_StatusCode retval = UA_DateTime_copy(&src->serverTimestamp, &dst->serverTimestamp);
    retval |= UA_DateTime_copy(&src->sourceTimestamp, &dst->sourceTimestamp);
    retval |= UA_Variant_copy(&src->value, &dst->value);
    dst->encodingMask = src->encodingMask;
    dst->serverPicoseconds = src->serverPicoseconds;
    dst->sourcePicoseconds = src->sourcePicoseconds;
    dst->status = src->status;
    if(retval)
        UA_DataValue_deleteMembers(dst);
    return retval;
}

/* Variant */
UA_TYPE_NEW_DEFAULT(UA_Variant)
void UA_Variant_init(UA_Variant *p) {
    p->storageType = UA_VARIANT_DATA;
    p->storage.data.arrayLength = -1;  // no element, p->data == UA_NULL
    p->storage.data.dataPtr        = UA_NULL;
    p->storage.data.arrayDimensions       = UA_NULL;
    p->storage.data.arrayDimensionsLength = -1;
    UA_NodeId_init(&p->dataTypeId);
    p->dataType = UA_NULL;
}

UA_TYPE_DELETE_DEFAULT(UA_Variant)
void UA_Variant_deleteMembers(UA_Variant *p) {
    if(p->storageType == UA_VARIANT_DATA) {
        if(p->storage.data.dataPtr) {
            UA_Array_delete(p->storage.data.dataPtr, p->storage.data.arrayLength, p->dataType);
            p->storage.data.dataPtr = UA_NULL;
            p->storage.data.arrayLength = 0;
        }

        if(p->storage.data.arrayDimensions) {
            UA_free(p->storage.data.arrayDimensions);
            p->storage.data.arrayDimensions = UA_NULL;
        }
        return;
    }

    UA_NodeId_deleteMembers(&p->dataTypeId);

    if(p->storageType == UA_VARIANT_DATASOURCE) {
        p->storage.datasource.delete(p->storage.datasource.handle);
    }
}


/** This function performs a deep copy. The resulting StorageType is UA_VARIANT_DATA. */
UA_StatusCode UA_Variant_copy(UA_Variant const *src, UA_Variant *dst) {
    UA_Variant_init(dst);
    /* 1) Uniform access to the data */
    UA_VariantData *dstdata = &dst->storage.data;
    const UA_VariantData *srcdata;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(src->storageType == UA_VARIANT_DATA || src->storageType == UA_VARIANT_DATA_NODELETE)
        srcdata = &src->storage.data;
    else {
        retval |= src->storage.datasource.read(src->storage.datasource.handle, &srcdata);
        if(retval)
            return retval;
    }

    /* 2) Copy the data to the destination */
    retval |= UA_Array_copy(srcdata->dataPtr, srcdata->arrayLength, &dstdata->dataPtr, src->dataType);
    if(retval == UA_STATUSCODE_GOOD) {
        dst->storageType = UA_VARIANT_DATA;
        dst->dataType= src->dataType;
        UA_NodeId_copy(&src->dataTypeId, &dst->dataTypeId);
        dstdata->arrayLength = srcdata->arrayLength;
        if(srcdata->arrayDimensions) {
            retval |= UA_Array_copy(srcdata->arrayDimensions, srcdata->arrayDimensionsLength,
                                    (void **)&dstdata->arrayDimensions, &UA_TYPES[UA_INT32]);
            if(retval == UA_STATUSCODE_GOOD)
                dstdata->arrayDimensionsLength = srcdata->arrayDimensionsLength;
            else {
                UA_Variant_deleteMembers(dst);
                UA_Variant_init(dst);
            }
        }
    } 

    /* 3) Release the data source if necessary */
    if(src->storageType == UA_VARIANT_DATASOURCE)
        src->storage.datasource.release(src->storage.datasource.handle, srcdata);

    return retval;
}

/** Copies data into a variant. The target variant has always a storagetype UA_VARIANT_DATA */
UA_StatusCode UA_Variant_copySetValue(UA_Variant *v, const void *p, UA_UInt16 typeIndex) {
    UA_Variant_init(v);
    v->dataType = &UA_TYPES[typeIndex];
    v->dataTypeId = (UA_NodeId){.namespaceIndex = 0,
                                .identifierType = UA_NODEIDTYPE_NUMERIC,
                                .identifier.numeric = UA_TYPES_IDS[typeIndex]};
    v->storage.data.arrayLength = 1; // no array but a single entry
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if((v->storage.data.dataPtr = UA_new(&UA_TYPES[typeIndex])))
       retval = UA_copy(p, v->storage.data.dataPtr, &UA_TYPES[typeIndex]);
    else
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
    if(retval) {
        UA_Variant_deleteMembers(v);
        UA_Variant_init(v);
    }
    return retval;
}

UA_StatusCode UA_Variant_copySetArray(UA_Variant *v, const void *array, UA_Int32 noElements, UA_UInt16 typeIndex) {
    UA_Variant_init(v);
    v->dataType = &UA_TYPES[typeIndex];
    v->dataTypeId = (UA_NodeId){.namespaceIndex = 0,
                                .identifierType = UA_NODEIDTYPE_NUMERIC,
                                .identifier.numeric = UA_TYPES_IDS[typeIndex]};
    v->storage.data.arrayLength = noElements;
    UA_StatusCode retval = UA_Array_copy(array, noElements, &v->storage.data.dataPtr, &UA_TYPES[typeIndex]);
    if(retval) {
        UA_Variant_deleteMembers(v);
        UA_Variant_init(v);
    }
    return retval;
}

/* DiagnosticInfo */
UA_TYPE_DELETE_DEFAULT(UA_DiagnosticInfo)
void UA_DiagnosticInfo_deleteMembers(UA_DiagnosticInfo *p) {
    UA_String_deleteMembers(&p->additionalInfo);
    if((p->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_INNERDIAGNOSTICINFO) && p->innerDiagnosticInfo) {
        UA_DiagnosticInfo_delete(p->innerDiagnosticInfo);
        p->innerDiagnosticInfo = UA_NULL;
    }
}

void UA_DiagnosticInfo_init(UA_DiagnosticInfo *p) {
    UA_String_init(&p->additionalInfo);
    p->encodingMask = 0;
    p->innerDiagnosticInfo = UA_NULL;
    UA_StatusCode_init(&p->innerStatusCode);
    p->locale              = 0;
    p->localizedText       = 0;
    p->namespaceUri        = 0;
    p->symbolicId          = 0;
}

UA_TYPE_NEW_DEFAULT(UA_DiagnosticInfo)
UA_StatusCode UA_DiagnosticInfo_copy(UA_DiagnosticInfo const *src, UA_DiagnosticInfo *dst) {
    UA_DiagnosticInfo_init(dst);
    UA_StatusCode retval = UA_String_copy(&src->additionalInfo, &dst->additionalInfo);
    dst->encodingMask = src->encodingMask;
    retval |= UA_StatusCode_copy(&src->innerStatusCode, &dst->innerStatusCode);
    if((src->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_INNERDIAGNOSTICINFO) && src->innerDiagnosticInfo) {
        if((dst->innerDiagnosticInfo = UA_malloc(sizeof(UA_DiagnosticInfo))))
            retval |= UA_DiagnosticInfo_copy(src->innerDiagnosticInfo, dst->innerDiagnosticInfo);
        else
            retval |= UA_STATUSCODE_BADOUTOFMEMORY;
    }
    dst->locale = src->locale;
    dst->localizedText = src->localizedText;
    dst->namespaceUri = src->namespaceUri;
    dst->symbolicId = src->symbolicId;
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
    UA_Byte *ptr = (UA_Byte *)p; // for pointer arithmetic

    /* Do not check if the index is a builtin-type here. Builtins will be called
       with their very own _init functions normally. In the off case, that the
       generic function is called with the index of a builtin, their layout
       contains a single member of the builtin type, that will be inited in the
       for loop. */

    for(int i=0;i<dataType->membersSize; i++) {
        const UA_DataTypeMember *member = &dataType->members[i];
        if(member->isArray) {
            /* Padding contains bit-magic to split into padding before and after
               the length integer */
            ptr += (member->padding >> 3);
            *((UA_Int32*)ptr) = -1;
            ptr += sizeof(UA_Int32) + (member->padding & 0x000fff);
            *((void**)ptr) = UA_NULL;
            ptr += sizeof(void*);
            continue;
        }

        ptr += member->padding;
        if(!member->namespaceZero) {
            // pointer arithmetic
            const UA_DataType *memberType = dataType - dataType->typeIndex + member->memberTypeIndex;
            UA_init(ptr, memberType);
            ptr += memberType->memSize;
            continue;
        }

        switch(member->memberTypeIndex) {
        case UA_BOOLEAN:
        case UA_SBYTE:
        case UA_BYTE:
            *ptr = 0;
            break;
        case UA_INT16:
        case UA_UINT16:
            *((UA_Int16*)ptr) = 0;
            break;
        case UA_INT32:
        case UA_UINT32:
        case UA_STATUSCODE:
        case UA_FLOAT:
            *((UA_Int32*)ptr) = 0;
            break;
        case UA_INT64:
        case UA_UINT64:
        case UA_DOUBLE:
        case UA_DATETIME:
            *((UA_Int64*)ptr) = 0;
            break;
        case UA_GUID:
            UA_Guid_init((UA_Guid*)ptr);
            break;
        case UA_NODEID:
            UA_NodeId_init((UA_NodeId*)ptr);
            break;
        case UA_EXPANDEDNODEID:
            UA_ExpandedNodeId_init((UA_ExpandedNodeId*)ptr);
            break;
        case UA_QUALIFIEDNAME:
            UA_QualifiedName_init((UA_QualifiedName*)ptr);
            break;
        case UA_LOCALIZEDTEXT:
            UA_LocalizedText_init((UA_LocalizedText*)ptr);
            break;
        case UA_EXTENSIONOBJECT:
            UA_ExtensionObject_init((UA_ExtensionObject*)ptr);
            break;
        case UA_DATAVALUE:
            UA_DataValue_init((UA_DataValue*)ptr);
            break;
        case UA_VARIANT:
            UA_Variant_init((UA_Variant*)ptr);
            break;
        case UA_DIAGNOSTICINFO:
            UA_DiagnosticInfo_init((UA_DiagnosticInfo*)ptr);
            break;
        case UA_STRING:
        case UA_BYTESTRING:
        case UA_XMLELEMENT:
            UA_String_init((UA_String*)ptr);
            break;
        default:
            UA_init(ptr, &UA_TYPES[member->memberTypeIndex]);
        }
        ptr += UA_TYPES[member->memberTypeIndex].memSize;
    }
}

void * UA_new(const UA_DataType *dataType) {
    void *p = UA_malloc(dataType->memSize);
    if(p) UA_init(p, dataType);
    return p;
}

UA_StatusCode UA_copy(const void *src, void *dst, const UA_DataType *dataType) {
    if(dataType->fixedSize) {
        memcpy(dst, src, dataType->memSize);
        return UA_STATUSCODE_GOOD;
    }
    const UA_Byte *ptrs = (const UA_Byte *)src;
    UA_Byte *ptrd = (UA_Byte *)dst;
    for(int i=0;i<dataType->membersSize; i++) {
        const UA_DataTypeMember *member = &dataType->members[i];
        const UA_DataType *memberType;
        if(member->namespaceZero)
            memberType = &UA_TYPES[member->memberTypeIndex];
        else
            memberType = dataType - dataType->typeIndex + member->memberTypeIndex;

        if(member->isArray) {
            ptrs += (member->padding >> 3);
            ptrd += (member->padding >> 3);
            UA_Int32 *dstNoElements = (UA_Int32*)ptrd;
            const UA_Int32 noElements = *((const UA_Int32*)ptrs);
            ptrs += sizeof(UA_Int32) + (member->padding & 0x000fff);
            ptrd += sizeof(UA_Int32) + (member->padding & 0x000fff);
            UA_StatusCode retval = UA_Array_copy(ptrs, noElements, (void**)&ptrd, memberType);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_deleteMembers(dst, dataType);
                return retval;
            }
            *dstNoElements = noElements;
            ptrs += sizeof(void*);
            ptrd += sizeof(void*);
        } else {
            ptrs += member->padding;
            ptrd += member->padding;
            UA_StatusCode retval = UA_copy(ptrs, ptrd, memberType);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_deleteMembers(dst, dataType);
                return retval;
            }
            ptrs += memberType->memSize;
            ptrd += memberType->memSize;
        }
    }
    return UA_STATUSCODE_GOOD;
}

void UA_deleteMembers(void *p, const UA_DataType *dataType) {
    UA_Byte *ptr = (UA_Byte *)p; // for pointer arithmetic
    if(dataType->fixedSize)
        return;
    UA_Byte membersSize = dataType->membersSize;
    for(int i=0;i<membersSize; i++) {
        const UA_DataTypeMember *member = &dataType->members[i];
        const UA_DataType *memberType;
        if(member->namespaceZero)
            memberType = &UA_TYPES[member->memberTypeIndex];
        else
            memberType = dataType - dataType->typeIndex + member->memberTypeIndex;

        if(member->isArray) {
            ptr += (member->padding >> 3);
            UA_Int32 noElements = *((UA_Int32*)ptr);
            ptr += sizeof(UA_Int32) + (member->padding & 0x000fff);
            UA_Array_delete(ptr, noElements, memberType);
            ptr += sizeof(void*);
            continue;
        }

        ptr += member->padding;
            
        if(!member->namespaceZero) {
            UA_deleteMembers(ptr, memberType);
            ptr += memberType->memSize;
            continue;
        }
        
        switch(member->memberTypeIndex) {
            // the following types have a fixed size.
            /* UA_BOOLEAN, UA_SBYTE, UA_BYTE, UA_INT16, UA_UINT16, UA_INT32, UA_UINT32, */
            /* UA_STATUSCODE, UA_FLOAT, UA_INT64, UA_UINT64, UA_DOUBLE, UA_DATETIME, UA_GUID */
        case UA_NODEID:
            UA_NodeId_deleteMembers((UA_NodeId*)ptr);
            break;
        case UA_EXPANDEDNODEID:
            UA_ExpandedNodeId_deleteMembers((UA_ExpandedNodeId*)ptr);
            break;
        case UA_QUALIFIEDNAME:
            UA_QualifiedName_deleteMembers((UA_QualifiedName*)ptr);
            break;
        case UA_LOCALIZEDTEXT:
            UA_LocalizedText_deleteMembers((UA_LocalizedText*)ptr);
            break;
        case UA_EXTENSIONOBJECT:
            UA_ExtensionObject_deleteMembers((UA_ExtensionObject*)ptr);
            break;
        case UA_DATAVALUE:
            UA_DataValue_deleteMembers((UA_DataValue*)ptr);
            break;
        case UA_VARIANT:
            UA_Variant_deleteMembers((UA_Variant*)ptr);
            break;
        case UA_DIAGNOSTICINFO:
            UA_DiagnosticInfo_deleteMembers((UA_DiagnosticInfo*)ptr);
            break;
        case UA_STRING:
        case UA_BYTESTRING:
        case UA_XMLELEMENT:
            UA_String_deleteMembers((UA_String*)ptr);
            break;
        default:
            UA_deleteMembers(ptr, memberType);
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

UA_StatusCode UA_Array_new(void **p, UA_Int32 noElements, const UA_DataType *dataType) {
    if(noElements <= 0) {
        *p = UA_NULL;
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    *p = malloc(dataType->memSize * noElements);
    if(!p)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_Byte *ptr = *p;
    for(UA_Int32 i = 0; i<noElements; i++) {
        UA_init(ptr, dataType);
        ptr += dataType->memSize;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Array_copy(const void *src, UA_Int32 noElements, void **dst, const UA_DataType *dataType) {
    if(noElements <= 0) {
        *dst = UA_NULL;
        return UA_STATUSCODE_GOOD;
    }

    if(!(*dst = UA_malloc(noElements * dataType->memSize)))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    if(dataType->fixedSize) {
        memcpy(*dst, src, dataType->memSize * noElements);
        return UA_STATUSCODE_GOOD;
    }

    const UA_Byte *ptrs = (const UA_Byte*)src;
    UA_Byte *ptrd = (UA_Byte*)*dst;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(int i=0;i<noElements; i++) {
        retval = UA_copy(ptrs, ptrd, dataType);
        ptrs += dataType->memSize;
        ptrd += dataType->memSize;
    }

    if(retval != UA_STATUSCODE_GOOD)
        UA_Array_delete(*dst, noElements, dataType);
        
    return retval;
}

void UA_Array_delete(void *p, UA_Int32 noElements, const UA_DataType *dataType) {
    if(noElements <= 0 || !p)
        return;

    if(!dataType->fixedSize) {
        UA_Byte *ptr = p; // for pointer arithemetic
        for(UA_Int32 i = 0; i<noElements; i++) {
            UA_deleteMembers(ptr, dataType);
            ptr += dataType->memSize;
        }
    }
    UA_free(p);
}
