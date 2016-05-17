#include "ua_util.h"
#include "ua_types.h"
#include "ua_types_generated.h"

#include "pcg_basic.h"
#include "libc_time.h"

/* static variables */
UA_EXPORT const UA_String UA_STRING_NULL = {.length = 0, .data = NULL };
UA_EXPORT const UA_ByteString UA_BYTESTRING_NULL = {.length = 0, .data = NULL };
UA_EXPORT const UA_NodeId UA_NODEID_NULL = {0, UA_NODEIDTYPE_NUMERIC, {0}};
UA_EXPORT const UA_ExpandedNodeId UA_EXPANDEDNODEID_NULL = {
    .nodeId = { .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC, .identifier.numeric = 0 },
    .namespaceUri = {.length = 0, .data = NULL}, .serverIndex = 0 };

/***************************/
/* Random Number Generator */
/***************************/
static UA_THREAD_LOCAL pcg32_random_t UA_rng = PCG32_INITIALIZER;

UA_EXPORT void UA_random_seed(UA_UInt64 seed) {
    pcg32_srandom_r(&UA_rng, seed, (uint64_t)UA_DateTime_now());
}

/*****************/
/* Builtin Types */
/*****************/
UA_EXPORT UA_UInt32 UA_UInt32_random(void) {
    return (UA_UInt32)pcg32_random_r(&UA_rng);
}

UA_String UA_String_fromChars(char const src[]) {
    UA_String str = UA_STRING_NULL;
    size_t length = strlen(src);
    if(length > 0) {
        str.data = UA_malloc(length);
        if(!str.data)
            return str;
    } else
        str.data = UA_EMPTY_ARRAY_SENTINEL;
    memcpy(str.data, src, length);
    str.length = length;
    return str;
}

UA_Boolean UA_String_equal(const UA_String *string1, const UA_String *string2) {
    if(string1->length != string2->length)
        return false;
    UA_Int32 is = memcmp((char const*)string1->data, (char const*)string2->data, string1->length);
    return (is == 0) ? true : false;
}

/* DateTime */
UA_DateTime UA_DateTime_now(void) {
#ifdef _WIN32
    /* Windows filetime has the same definition as UA_DateTime */
    FILETIME ft;
    SYSTEMTIME st;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);
    ULARGE_INTEGER ul;
    ul.LowPart = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    return (UA_DateTime)ul.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * UA_SEC_TO_DATETIME) + (tv.tv_usec * UA_USEC_TO_DATETIME) + UA_DATETIME_UNIX_EPOCH;
#endif
}

UA_DateTime UA_DateTime_nowMonotonic(void) {
#ifdef _WIN32
    LARGE_INTEGER freq, ticks;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&ticks);
    UA_Double ticks2dt = UA_SEC_TO_DATETIME;
    ticks2dt /= freq.QuadPart;
    return (UA_DateTime)(ticks.QuadPart * ticks2dt);
#elif defined(__APPLE__) || defined(__MACH__) // OS X does not have clock_gettime, use clock_get_time
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    return (mts.tv_sec * UA_SEC_TO_DATETIME) + (mts.tv_nsec / 100);
#else
    struct timespec ts;
#ifdef __CYGWIN__    
    clock_gettime(CLOCK_MONOTONIC, &ts);
#else
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#endif
    return (ts.tv_sec * UA_SEC_TO_DATETIME) + (ts.tv_nsec / 100);
#endif
}

UA_DateTimeStruct UA_DateTime_toStruct(UA_DateTime t) {
    /* Calculating the the milli-, micro- and nanoseconds */
    UA_DateTimeStruct dateTimeStruct;
    dateTimeStruct.nanoSec  = (UA_UInt16)((t % 10) * 100);
    dateTimeStruct.microSec = (UA_UInt16)((t % 10000) / 10);
    dateTimeStruct.milliSec = (UA_UInt16)((t % 10000000) / 10000);

    /* Calculating the unix time with #include <time.h> */
    time_t secSinceUnixEpoch = (time_t)((t - UA_DATETIME_UNIX_EPOCH) / UA_SEC_TO_DATETIME);
    struct tm ts;
    memset(&ts, 0, sizeof(struct tm));
    __secs_to_tm(secSinceUnixEpoch, &ts);
    dateTimeStruct.sec    = (UA_UInt16)ts.tm_sec;
    dateTimeStruct.min    = (UA_UInt16)ts.tm_min;
    dateTimeStruct.hour   = (UA_UInt16)ts.tm_hour;
    dateTimeStruct.day    = (UA_UInt16)ts.tm_mday;
    dateTimeStruct.month  = (UA_UInt16)(ts.tm_mon + 1);
    dateTimeStruct.year   = (UA_UInt16)(ts.tm_year + 1900);
    return dateTimeStruct;
}

static void printNumber(UA_UInt16 n, UA_Byte *pos, size_t digits) {
    for(size_t i = digits; i > 0; i--) {
        pos[i-1] = (UA_Byte)((n % 10) + '0');
        n = n / 10;
    }
}

UA_String UA_DateTime_toString(UA_DateTime t) {
    UA_String str = UA_STRING_NULL;
    // length of the string is 31 (plus \0 at the end)
    if(!(str.data = UA_malloc(32)))
        return str;
    str.length = 31;
    UA_DateTimeStruct tSt = UA_DateTime_toStruct(t);
    printNumber(tSt.month, str.data, 2);
    str.data[2] = '/';
    printNumber(tSt.day, &str.data[3], 2);
    str.data[5] = '/';
    printNumber(tSt.year, &str.data[6], 4);
    str.data[10] = ' ';
    printNumber(tSt.hour, &str.data[11], 2);
    str.data[13] = ':';
    printNumber(tSt.min, &str.data[14], 2);
    str.data[16] = ':';
    printNumber(tSt.sec, &str.data[17], 2);
    str.data[19] = '.';
    printNumber(tSt.milliSec, &str.data[20], 3);
    str.data[23] = '.';
    printNumber(tSt.microSec, &str.data[24], 3);
    str.data[27] = '.';
    printNumber(tSt.nanoSec, &str.data[28], 3);
    return str;
}

/* Guid */
UA_Boolean UA_Guid_equal(const UA_Guid *g1, const UA_Guid *g2) {
    if(memcmp(g1, g2, sizeof(UA_Guid)) == 0)
        return true;
    return false;
}

UA_Guid UA_Guid_random(void) {
    UA_Guid result;
    result.data1 = (UA_UInt32)pcg32_random_r(&UA_rng);
    UA_UInt32 r = (UA_UInt32)pcg32_random_r(&UA_rng);
    result.data2 = (UA_UInt16) r;
    result.data3 = (UA_UInt16) (r >> 16);
    r = (UA_UInt32)pcg32_random_r(&UA_rng);
    result.data4[0] = (UA_Byte)r;
    result.data4[1] = (UA_Byte)(r >> 4);
    result.data4[2] = (UA_Byte)(r >> 8);
    result.data4[3] = (UA_Byte)(r >> 12);
    r = (UA_UInt32)pcg32_random_r(&UA_rng);
    result.data4[4] = (UA_Byte)r;
    result.data4[5] = (UA_Byte)(r >> 4);
    result.data4[6] = (UA_Byte)(r >> 8);
    result.data4[7] = (UA_Byte)(r >> 12);
    return result;
}

/* ByteString */
UA_StatusCode UA_ByteString_allocBuffer(UA_ByteString *bs, size_t length) {
    if(!(bs->data = UA_malloc(length)))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    bs->length = length;
    return UA_STATUSCODE_GOOD;
}

/* NodeId */
static void NodeId_deleteMembers(UA_NodeId *p, const UA_DataType *_) {
    switch(p->identifierType) {
    case UA_NODEIDTYPE_STRING:
    case UA_NODEIDTYPE_BYTESTRING:
        UA_free((void*)((uintptr_t)p->identifier.byteString.data & ~(uintptr_t)UA_EMPTY_ARRAY_SENTINEL));
        p->identifier.byteString = UA_BYTESTRING_NULL;
        break;
    default: break;
    }
}

static UA_StatusCode NodeId_copy(UA_NodeId const *src, UA_NodeId *dst, const UA_DataType *_) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    switch(src->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        *dst = *src;
        return UA_STATUSCODE_GOOD;
    case UA_NODEIDTYPE_STRING:
        retval |= UA_String_copy(&src->identifier.string, &dst->identifier.string);
        break;
    case UA_NODEIDTYPE_GUID:
        retval |= UA_Guid_copy(&src->identifier.guid, &dst->identifier.guid);
        break;
    case UA_NODEIDTYPE_BYTESTRING:
        retval |= UA_ByteString_copy(&src->identifier.byteString, &dst->identifier.byteString);
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    dst->namespaceIndex = src->namespaceIndex;
    dst->identifierType = src->identifierType;
    if(retval != UA_STATUSCODE_GOOD)
        NodeId_deleteMembers(dst, NULL);
    return retval;
}

UA_Boolean UA_NodeId_equal(const UA_NodeId *n1, const UA_NodeId *n2) {
	if(n1->namespaceIndex != n2->namespaceIndex || n1->identifierType!=n2->identifierType)
        return false;
    switch(n1->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        if(n1->identifier.numeric == n2->identifier.numeric)
            return true;
        else
            return false;
    case UA_NODEIDTYPE_STRING:
        return UA_String_equal(&n1->identifier.string, &n2->identifier.string);
    case UA_NODEIDTYPE_GUID:
        return UA_Guid_equal(&n1->identifier.guid, &n2->identifier.guid);
    case UA_NODEIDTYPE_BYTESTRING:
        return UA_ByteString_equal(&n1->identifier.byteString, &n2->identifier.byteString);
    }
    return false;
}

/* ExpandedNodeId */
static void ExpandedNodeId_deleteMembers(UA_ExpandedNodeId *p, const UA_DataType *_) {
    NodeId_deleteMembers(&p->nodeId, _);
    UA_String_deleteMembers(&p->namespaceUri);
}

static UA_StatusCode
ExpandedNodeId_copy(UA_ExpandedNodeId const *src, UA_ExpandedNodeId *dst, const UA_DataType *_) {
    UA_StatusCode retval = NodeId_copy(&src->nodeId, &dst->nodeId, NULL);
    retval |= UA_String_copy(&src->namespaceUri, &dst->namespaceUri);
    dst->serverIndex = src->serverIndex;
    if(retval != UA_STATUSCODE_GOOD)
        ExpandedNodeId_deleteMembers(dst, NULL);
    return retval;
}

/* ExtensionObject */
static void ExtensionObject_deleteMembers(UA_ExtensionObject *p, const UA_DataType *_) {
    switch(p->encoding) {
    case UA_EXTENSIONOBJECT_ENCODED_NOBODY:
    case UA_EXTENSIONOBJECT_ENCODED_BYTESTRING:
    case UA_EXTENSIONOBJECT_ENCODED_XML:
        NodeId_deleteMembers(&p->content.encoded.typeId, NULL);
        UA_free((void*)((uintptr_t)p->content.encoded.body.data & ~(uintptr_t)UA_EMPTY_ARRAY_SENTINEL));
        p->content.encoded.body = UA_BYTESTRING_NULL;
        break;
    case UA_EXTENSIONOBJECT_DECODED:
        if(!p->content.decoded.data)
            break;
        UA_delete(p->content.decoded.data, p->content.decoded.type);
        p->content.decoded.data = NULL;
        p->content.decoded.type = NULL;
        break;
    case UA_EXTENSIONOBJECT_DECODED_NODELETE:
        p->content.decoded.type = NULL;
    default:
        break;
    }
}

static UA_StatusCode
ExtensionObject_copy(UA_ExtensionObject const *src, UA_ExtensionObject *dst, const UA_DataType *_) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    switch(src->encoding) {
    case UA_EXTENSIONOBJECT_ENCODED_NOBODY:
    case UA_EXTENSIONOBJECT_ENCODED_BYTESTRING:
    case UA_EXTENSIONOBJECT_ENCODED_XML:
        dst->encoding = src->encoding;
        retval = NodeId_copy(&src->content.encoded.typeId, &dst->content.encoded.typeId, NULL);
        retval |= UA_ByteString_copy(&src->content.encoded.body, &dst->content.encoded.body);
        break;
    case UA_EXTENSIONOBJECT_DECODED:
    case UA_EXTENSIONOBJECT_DECODED_NODELETE:
        if(!src->content.decoded.type || !src->content.decoded.data)
            return UA_STATUSCODE_BADINTERNALERROR;
        dst->encoding = UA_EXTENSIONOBJECT_DECODED;
        dst->content.decoded.type = src->content.decoded.type;
        retval = UA_Array_copy(src->content.decoded.data, 1,
            &dst->content.decoded.data, src->content.decoded.type);
        break;
    default:
        break;
    }
    return retval;
}

/* Variant */
static void Variant_deletemembers(UA_Variant *p, const UA_DataType *_) {
    if(p->storageType != UA_VARIANT_DATA)
        return;
    if(p->data > UA_EMPTY_ARRAY_SENTINEL) {
        if(p->arrayLength == 0)
            p->arrayLength = 1;
        UA_Array_delete(p->data, p->arrayLength, p->type);
        p->data = NULL;
        p->arrayLength = 0;
    }
    if(p->arrayDimensions) {
        UA_Array_delete(p->arrayDimensions, p->arrayDimensionsSize, &UA_TYPES[UA_TYPES_INT32]);
        p->arrayDimensions = NULL;
        p->arrayDimensionsSize = 0;
    }
}

static UA_StatusCode
Variant_copy(UA_Variant const *src, UA_Variant *dst, const UA_DataType *_) {
    size_t length = src->arrayLength;
    if(UA_Variant_isScalar(src))
        length = 1;
    UA_StatusCode retval = UA_Array_copy(src->data, length, &dst->data, src->type);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    dst->arrayLength = src->arrayLength;
    dst->type = src->type;
    if(src->arrayDimensions) {
        retval = UA_Array_copy(src->arrayDimensions, src->arrayDimensionsSize,
            (void**)&dst->arrayDimensions, &UA_TYPES[UA_TYPES_INT32]);
        if(retval == UA_STATUSCODE_GOOD)
            dst->arrayDimensionsSize = src->arrayDimensionsSize;
        else
            Variant_deletemembers(dst, NULL);
    }
    return retval;
}

/**
 * Test if a range is compatible with a variant. If yes, the following values are set:
 * - total: how many elements are in the range
 * - block: how big is each contiguous block of elements in the variant that maps into the range
 * - stride: how many elements are between the blocks (beginning to beginning)
 * - first: where does the first block begin
 */
static UA_StatusCode
processRangeDefinition(const UA_Variant *v, const UA_NumericRange range, size_t *total,
                       size_t *block, size_t *stride, size_t *first) {
    /* Test the integrity of the source variant dimensions */
    size_t dims_count = 1;
    UA_UInt32 elements = 1;
#if(MAX_SIZE > 0xffffffff) /* 64bit only */
    if(v->arrayLength > UA_UINT32_MAX)
        return UA_STATUSCODE_BADINTERNALERROR;
#endif
    UA_UInt32 arrayLength = (UA_UInt32)v->arrayLength;
    const UA_UInt32 *dims = &arrayLength;
    if(v->arrayDimensionsSize > 0) {
        dims_count = v->arrayDimensionsSize;
        dims = (UA_UInt32*)v->arrayDimensions;
        for(size_t i = 0; i < dims_count; i++) {
            /* dimensions can have negative size similar to array lengths */
            if(v->arrayDimensions[i] < 0)
                return UA_STATUSCODE_BADINDEXRANGEINVALID;
            elements *= dims[i];
        }
        if(elements != v->arrayLength)
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Test the integrity of the range */
    size_t count = 1;
    if(range.dimensionsSize != dims_count)
        return UA_STATUSCODE_BADINDEXRANGEINVALID;
    for(size_t i = 0; i < dims_count; i++) {
        if(range.dimensions[i].min > range.dimensions[i].max)
            return UA_STATUSCODE_BADINDEXRANGENODATA;
        if(range.dimensions[i].max >= dims[i])
            return UA_STATUSCODE_BADINDEXRANGEINVALID;
        count *= (range.dimensions[i].max - range.dimensions[i].min) + 1;
    }

    /* Compute the stride length and the position of the first element */
    size_t b = 1, s = elements, f = 0;
    size_t running_dimssize = 1;
    UA_Boolean found_contiguous = false;
    for(size_t k = dims_count - 1; ; k--) {
        if(!found_contiguous && (range.dimensions[k].min != 0 || range.dimensions[k].max + 1 != dims[k])) {
            found_contiguous = true;
            b = (range.dimensions[k].max - range.dimensions[k].min + 1) * running_dimssize;
            s = dims[k] * running_dimssize;
        } 
        f += running_dimssize * range.dimensions[k].min;
        running_dimssize *= dims[k];
        if(k == 0)
            break;
    }
    *total = count;
    *block = b;
    *stride = s;
    *first = f;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Variant_copyRange(const UA_Variant *src, UA_Variant *dst, const UA_NumericRange range) {
    size_t count, block, stride, first;
    UA_StatusCode retval = processRangeDefinition(src, range, &count, &block, &stride, &first);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_Variant_init(dst);
    size_t elem_size = src->type->memSize;
    dst->data = UA_malloc(elem_size * count);
    if(!dst->data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Copy the range */
    size_t block_count = count / block;
    uintptr_t nextdst = (uintptr_t)dst->data;
    uintptr_t nextsrc = (uintptr_t)src->data + (elem_size * first);
    if(src->type->fixedSize) {
        for(size_t i = 0; i < block_count; i++) {
            memcpy((void*)nextdst, (void*)nextsrc, elem_size * block);
            nextdst += block * elem_size;
            nextsrc += stride * elem_size;
        }
    } else {
        for(size_t i = 0; i < block_count; i++) {
            for(size_t j = 0; j < block && retval == UA_STATUSCODE_GOOD; j++) {
                retval = UA_copy((const void*)nextsrc, (void*)nextdst, src->type);
                nextdst += elem_size;
                nextsrc += elem_size;
            }
            nextsrc += (stride - block) * elem_size;
        }
        if(retval != UA_STATUSCODE_GOOD) {
            size_t copied = ((nextdst - elem_size) - (uintptr_t)dst->data) / elem_size;
            UA_Array_delete(dst->data, copied, src->type);
            dst->data = NULL;
            return retval;
        }
    }
    dst->arrayLength = count;
    dst->type = src->type;

    /* Copy the range dimensions */
    if(src->arrayDimensionsSize > 0) {
        dst->arrayDimensions = UA_Array_new(src->arrayDimensionsSize, &UA_TYPES[UA_TYPES_UINT32]);
        if(!dst->arrayDimensions) {
            Variant_deletemembers(dst, NULL);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        dst->arrayDimensionsSize = src->arrayDimensionsSize;
        for(size_t k = 0; k < src->arrayDimensionsSize; k++)
            dst->arrayDimensions[k] = (UA_Int32)(range.dimensions[k].max - range.dimensions[k].min + 1);
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Variant_setRange(UA_Variant *v, void * UA_RESTRICT array, size_t arraySize, const UA_NumericRange range) {
    size_t count, block, stride, first;
    UA_StatusCode retval = processRangeDefinition(v, range, &count, &block, &stride, &first);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(count != arraySize)
        return UA_STATUSCODE_BADINDEXRANGEINVALID;

    size_t block_count = count / block;
    size_t elem_size = v->type->memSize;
    uintptr_t nextdst = (uintptr_t)v->data + (first * elem_size);
    uintptr_t nextsrc = (uintptr_t)array;
    for(size_t i = 0; i < block_count; i++) {
        if(!v->type->fixedSize) {
            for(size_t j = 0; j < block; j++) {
                UA_deleteMembers((void*)nextdst, v->type);
                nextdst += elem_size;
            }
            nextdst -= block * elem_size;
        }
        memcpy((void*)nextdst, (void*)nextsrc, elem_size * block);
        nextsrc += block * elem_size;
        nextdst += stride * elem_size;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Variant_setRangeCopy(UA_Variant *v, const void *array, size_t arraySize, const UA_NumericRange range) {
    size_t count, block, stride, first;
    UA_StatusCode retval = processRangeDefinition(v, range, &count, &block, &stride, &first);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(count != arraySize)
        return UA_STATUSCODE_BADINDEXRANGEINVALID;

    size_t block_count = count / block;
    size_t elem_size = v->type->memSize;
    uintptr_t nextdst = (uintptr_t)v->data + (first * elem_size);
    uintptr_t nextsrc = (uintptr_t)array;
    if(v->type->fixedSize) {
        for(size_t i = 0; i < block_count; i++) {
            memcpy((void*)nextdst, (void*)nextsrc, elem_size * block);
            nextsrc += block * elem_size;
            nextdst += stride * elem_size;
        }
    } else {
        for(size_t i = 0; i < block_count; i++) {
            for(size_t j = 0; j < block; j++) {
                UA_deleteMembers((void*)nextdst, v->type);
                retval |= UA_copy((void*)nextsrc, (void*)nextdst, v->type);
                nextdst += elem_size;
                nextsrc += elem_size;
            }
            nextdst += (stride - block) * elem_size;
        }
    }
    return retval;
}

void UA_Variant_setScalar(UA_Variant *v, void * UA_RESTRICT p, const UA_DataType *type) {
    UA_Variant_init(v);
    v->type = type;
    v->arrayLength = 0;
    v->data = p;
}

UA_StatusCode UA_Variant_setScalarCopy(UA_Variant *v, const void *p, const UA_DataType *type) {
    void *new = UA_malloc(type->memSize);
    if(!new)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_StatusCode retval = UA_copy(p, new, type);
	if(retval != UA_STATUSCODE_GOOD) {
		UA_free(new);
        //cppcheck-suppress memleak
		return retval;
	}
    UA_Variant_setScalar(v, new, type);
    //cppcheck-suppress memleak
    return UA_STATUSCODE_GOOD;
}

void
UA_Variant_setArray(UA_Variant *v, void * UA_RESTRICT array,
                    size_t arraySize, const UA_DataType *type) {
    UA_Variant_init(v);
    v->data = array;
    v->arrayLength = arraySize;
    v->type = type;
}

UA_StatusCode
UA_Variant_setArrayCopy(UA_Variant *v, const void *array,
                        size_t arraySize, const UA_DataType *type) {
    UA_Variant_init(v);
    UA_StatusCode retval = UA_Array_copy(array, arraySize, &v->data, type);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    v->arrayLength = arraySize; 
    v->type = type;
    return UA_STATUSCODE_GOOD;
}

/* LocalizedText */
static void LocalizedText_deleteMembers(UA_LocalizedText *p, const UA_DataType *_) {
    UA_String_deleteMembers(&p->locale);
    UA_String_deleteMembers(&p->text);
}

static UA_StatusCode
LocalizedText_copy(UA_LocalizedText const *src, UA_LocalizedText *dst, const UA_DataType *_) {
    UA_StatusCode retval = UA_String_copy(&src->locale, &dst->locale);
    retval |= UA_String_copy(&src->text, &dst->text);
    return retval;
}

/* DataValue */
static void DataValue_deleteMembers(UA_DataValue *p, const UA_DataType *_) {
    Variant_deletemembers(&p->value, NULL);
}

static UA_StatusCode
DataValue_copy(UA_DataValue const *src, UA_DataValue *dst, const UA_DataType *_) {
    memcpy(dst, src, sizeof(UA_DataValue));
    UA_Variant_init(&dst->value);
    UA_StatusCode retval = Variant_copy(&src->value, &dst->value, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        DataValue_deleteMembers(dst, NULL);
    return retval;
}

/* DiagnosticInfo */
static void DiagnosticInfo_deleteMembers(UA_DiagnosticInfo *p, const UA_DataType *_) {
    UA_String_deleteMembers(&p->additionalInfo);
    if(p->hasInnerDiagnosticInfo && p->innerDiagnosticInfo) {
        DiagnosticInfo_deleteMembers(p->innerDiagnosticInfo, NULL);
        UA_free(p->innerDiagnosticInfo);
        p->innerDiagnosticInfo = NULL;
        p->hasInnerDiagnosticInfo = false;
    }
}

static UA_StatusCode
DiagnosticInfo_copy(UA_DiagnosticInfo const *src, UA_DiagnosticInfo *dst, const UA_DataType *_) {
    memcpy(dst, src, sizeof(UA_DiagnosticInfo));
    UA_String_init(&dst->additionalInfo);
    dst->innerDiagnosticInfo = NULL;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(src->hasAdditionalInfo)
       retval = UA_String_copy(&src->additionalInfo, &dst->additionalInfo);
    if(src->hasInnerDiagnosticInfo && src->innerDiagnosticInfo) {
        if((dst->innerDiagnosticInfo = UA_malloc(sizeof(UA_DiagnosticInfo)))) {
            retval |= DiagnosticInfo_copy(src->innerDiagnosticInfo, dst->innerDiagnosticInfo, NULL);
            dst->hasInnerDiagnosticInfo = true;
        } else {
            dst->hasInnerDiagnosticInfo = false;
            retval |= UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }
    if(retval != UA_STATUSCODE_GOOD)
        DiagnosticInfo_deleteMembers(dst, NULL);
    return retval;
}

/*******************/
/* Structure Types */
/*******************/

void * UA_new(const UA_DataType *type) {
    void *p = UA_calloc(1, type->memSize);
    return p;
}

static UA_StatusCode copyByte(const void *src, void *dst, const UA_DataType *_) {
    memcpy(dst, src, sizeof(UA_Byte));
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode copy2Byte(const void *src, void *dst, const UA_DataType *_) {
    memcpy(dst, src, sizeof(UA_UInt16));
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode copy4Byte(const void *src, void *dst, const UA_DataType *_) {
    memcpy(dst, src, sizeof(UA_UInt32));
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode copy8Byte(const void *src, void *dst, const UA_DataType *_) {
    memcpy(dst, src, sizeof(UA_UInt64));
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode copyFixedSize(const void *src, void *dst, const UA_DataType *type) {
    memcpy(dst, src, type->memSize);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode copyNoInit(const void *src, void *dst, const UA_DataType *type);

typedef UA_StatusCode (*UA_copySignature)(const void *src, void *dst, const UA_DataType *type);
static const UA_copySignature copyJumpTable[UA_BUILTIN_TYPES_COUNT + 1] = {
    (UA_copySignature)copyByte, // Boolean
    (UA_copySignature)copyByte, // SByte
    (UA_copySignature)copyByte, // Byte
    (UA_copySignature)copy2Byte, // Int16
    (UA_copySignature)copy2Byte, // UInt16 
    (UA_copySignature)copy4Byte, // Int32 
    (UA_copySignature)copy4Byte, // UInt32 
    (UA_copySignature)copy8Byte, // Int64
    (UA_copySignature)copy8Byte, // UInt64 
    (UA_copySignature)copy4Byte, // Float 
    (UA_copySignature)copy8Byte, // Double 
    (UA_copySignature)copyNoInit, // String
    (UA_copySignature)copy8Byte, // DateTime
    (UA_copySignature)copyFixedSize, // Guid 
    (UA_copySignature)copyNoInit, // ByteString
    (UA_copySignature)copyNoInit, // XmlElement
    (UA_copySignature)NodeId_copy,
    (UA_copySignature)ExpandedNodeId_copy,
    (UA_copySignature)copy4Byte, // StatusCode
    (UA_copySignature)copyNoInit, // QualifiedName
    (UA_copySignature)LocalizedText_copy, // LocalizedText
    (UA_copySignature)ExtensionObject_copy,
    (UA_copySignature)DataValue_copy,
    (UA_copySignature)Variant_copy,
    (UA_copySignature)DiagnosticInfo_copy,
    (UA_copySignature)copyNoInit // all others
};

static UA_StatusCode copyNoInit(const void *src, void *dst, const UA_DataType *type) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    uintptr_t ptrs = (uintptr_t)src;
    uintptr_t ptrd = (uintptr_t)dst;
    UA_Byte membersSize = type->membersSize;
    for(size_t i = 0; i < membersSize; i++) {
        const UA_DataTypeMember *member = &type->members[i];
        const UA_DataType *typelists[2] = { UA_TYPES, &type[-type->typeIndex] };
        const UA_DataType *memberType = &typelists[!member->namespaceZero][member->memberTypeIndex];
        if(!member->isArray) {
            ptrs += member->padding;
            ptrd += member->padding;
            size_t fi = memberType->builtin ? memberType->typeIndex : UA_BUILTIN_TYPES_COUNT;
            retval |= copyJumpTable[fi]((const void*)ptrs, (void*)ptrd, memberType);
            ptrs += memberType->memSize;
            ptrd += memberType->memSize;
        } else {
            ptrs += member->padding;
            ptrd += member->padding;
            size_t *dst_size = (size_t*)ptrd;
            const size_t size = *((const size_t*)ptrs);
            ptrs += sizeof(size_t);
            ptrd += sizeof(size_t);
            retval |= UA_Array_copy(*(void* const*)ptrs, size, (void**)ptrd, memberType);
            *dst_size = size;
            if(retval != UA_STATUSCODE_GOOD)
                *dst_size = 0;
            ptrs += sizeof(void*);
            ptrd += sizeof(void*);
        }
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_deleteMembers(dst, type);
    return retval;
}

UA_StatusCode UA_copy(const void *src, void *dst, const UA_DataType *type) {
    memset(dst, 0, type->memSize);
    return copyNoInit(src, dst, type);
}

typedef void (*UA_deleteMembersSignature)(void *p, const UA_DataType *type);
static void nopDeleteMembers(void *p, const UA_DataType *type) { }

static const UA_deleteMembersSignature deleteMembersJumpTable[UA_BUILTIN_TYPES_COUNT + 1] = {
    (UA_deleteMembersSignature)nopDeleteMembers, // Boolean
    (UA_deleteMembersSignature)nopDeleteMembers, // SByte
    (UA_deleteMembersSignature)nopDeleteMembers, // Byte
    (UA_deleteMembersSignature)nopDeleteMembers, // Int16
    (UA_deleteMembersSignature)nopDeleteMembers, // UInt16 
    (UA_deleteMembersSignature)nopDeleteMembers, // Int32 
    (UA_deleteMembersSignature)nopDeleteMembers, // UInt32 
    (UA_deleteMembersSignature)nopDeleteMembers, // Int64
    (UA_deleteMembersSignature)nopDeleteMembers, // UInt64 
    (UA_deleteMembersSignature)nopDeleteMembers, // Float 
    (UA_deleteMembersSignature)nopDeleteMembers, // Double 
    (UA_deleteMembersSignature)UA_deleteMembers, // String
    (UA_deleteMembersSignature)nopDeleteMembers, // DateTime
    (UA_deleteMembersSignature)nopDeleteMembers, // Guid 
    (UA_deleteMembersSignature)UA_deleteMembers, // ByteString
    (UA_deleteMembersSignature)UA_deleteMembers, // XmlElement
    (UA_deleteMembersSignature)NodeId_deleteMembers,
    (UA_deleteMembersSignature)ExpandedNodeId_deleteMembers, // ExpandedNodeId
    (UA_deleteMembersSignature)nopDeleteMembers, // StatusCode
    (UA_deleteMembersSignature)UA_deleteMembers, // QualifiedName
    (UA_deleteMembersSignature)LocalizedText_deleteMembers, // LocalizedText
    (UA_deleteMembersSignature)ExtensionObject_deleteMembers,
    (UA_deleteMembersSignature)DataValue_deleteMembers,
    (UA_deleteMembersSignature)Variant_deletemembers,
    (UA_deleteMembersSignature)DiagnosticInfo_deleteMembers,
    (UA_deleteMembersSignature)UA_deleteMembers,
};

void UA_deleteMembers(void *p, const UA_DataType *type) {
    uintptr_t ptr = (uintptr_t)p;
    UA_Byte membersSize = type->membersSize;
    for(size_t i = 0; i < membersSize; i++) {
        const UA_DataTypeMember *member = &type->members[i];
        const UA_DataType *typelists[2] = { UA_TYPES, &type[-type->typeIndex] };
        const UA_DataType *memberType = &typelists[!member->namespaceZero][member->memberTypeIndex];
        if(!member->isArray) {
            ptr += member->padding;
            size_t fi = memberType->builtin ? memberType->typeIndex : UA_BUILTIN_TYPES_COUNT;
            deleteMembersJumpTable[fi]((void*)ptr, memberType);
            ptr += memberType->memSize;
        } else {
            ptr += member->padding;
            size_t length = *(size_t*)ptr;
            *(size_t*)ptr = 0;
            ptr += sizeof(size_t);
            UA_Array_delete(*(void**)ptr, length, memberType);
            *(void**)ptr = NULL;
            ptr += sizeof(void*);
        }
    }
}

void UA_delete(void *p, const UA_DataType *type) {
    UA_deleteMembers(p, type);
    UA_free(p);
}

/******************/
/* Array Handling */
/******************/

void * UA_Array_new(size_t size, const UA_DataType *type) {
    if(size == 0)
        return UA_EMPTY_ARRAY_SENTINEL;
    return UA_calloc(size, type->memSize);
}

UA_StatusCode
UA_Array_copy(const void *src, size_t src_size, void **dst, const UA_DataType *type) {
    if(src_size == 0) {
        if(src == NULL)
            *dst = NULL;
        else
            *dst= UA_EMPTY_ARRAY_SENTINEL;
        return UA_STATUSCODE_GOOD;
    }

    /* calloc, so we don't have to check retval in every iteration of copying */
    *dst = UA_calloc(src_size, type->memSize);
    if(!*dst)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    if(type->fixedSize) {
        memcpy(*dst, src, type->memSize * src_size);
        return UA_STATUSCODE_GOOD;
    }

    uintptr_t ptrs = (uintptr_t)src;
    uintptr_t ptrd = (uintptr_t)*dst;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < src_size; i++) {
        retval |= UA_copy((void*)ptrs, (void*)ptrd, type);
        ptrs += type->memSize;
        ptrd += type->memSize;
    }
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(*dst, src_size, type);
        *dst = NULL;
    }
    return retval;
}

void UA_Array_delete(void *p, size_t size, const UA_DataType *type) {
    if(!type->fixedSize) {
        uintptr_t ptr = (uintptr_t)p;
        for(size_t i = 0; i < size; i++) {
            UA_deleteMembers((void*)ptr, type);
            ptr += type->memSize;
        }
    }
    UA_free((void*)((uintptr_t)p & ~(uintptr_t)UA_EMPTY_ARRAY_SENTINEL));
}
