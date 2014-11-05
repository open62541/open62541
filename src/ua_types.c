#include <stdarg.h> // va_start, va_end
#include <time.h>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#ifdef DEBUG
#include <inttypes.h>
#endif

#include "ua_util.h"
#include "ua_types.h"
#include "ua_types_encoding_binary.h"
#include "ua_namespace_0.h"
#include "ua_statuscodes.h"

/* Boolean */
void UA_Boolean_init(UA_Boolean *p) {
    *p = UA_FALSE;
}

UA_TYPE_DELETE_DEFAULT(UA_Boolean)
UA_TYPE_DELETEMEMBERS_NOACTION(UA_Boolean)
UA_TYPE_NEW_DEFAULT(UA_Boolean)
UA_TYPE_COPY_DEFAULT(UA_Boolean)
#ifdef DEBUG
void UA_Boolean_print(const UA_Boolean *p, FILE *stream) {
    if(*p) fprintf(stream, "UA_TRUE");
    else fprintf(stream, "UA_FALSE");
}
#endif

/* SByte */
UA_TYPE_DEFAULT(UA_SByte)
#ifdef DEBUG
void UA_SByte_print(const UA_SByte *p, FILE *stream) {
    if(!p || !stream) return;
    UA_SByte x = *p;
    fprintf(stream, "%s%x\n", x < 0 ? "-" : "", x < 0 ? -x : x);
}
#endif

/* Byte */
UA_TYPE_DEFAULT(UA_Byte)
#ifdef DEBUG
void UA_Byte_print(const UA_Byte *p, FILE *stream) {
    if(!p || !stream) return;
    fprintf(stream, "%x", *p);
}
#endif

/* Int16 */
UA_TYPE_DEFAULT(UA_Int16)
#ifdef DEBUG
void UA_Int16_print(const UA_Int16 *p, FILE *stream) {
    if(!p || !stream) return;
    fprintf(stream, "%d", *p);
}
#endif

/* UInt16 */
UA_TYPE_DEFAULT(UA_UInt16)
#ifdef DEBUG
void UA_UInt16_print(const UA_UInt16 *p, FILE *stream) {
    if(!p || !stream) return;
    fprintf(stream, "%u", *p);
}
#endif

/* Int32 */
UA_TYPE_DEFAULT(UA_Int32)
#ifdef DEBUG
void UA_Int32_print(const UA_Int32 *p, FILE *stream) {
    if(!p || !stream) return;
    fprintf(stream, "%d", *p);
}

#endif

/* UInt32 */
UA_TYPE_DEFAULT(UA_UInt32)
#ifdef DEBUG
void UA_UInt32_print(const UA_UInt32 *p, FILE *stream) {
    if(!p || !stream) return;
    fprintf(stream, "%u", *p);
}
#endif

/* Int64 */
UA_TYPE_DEFAULT(UA_Int64)
#ifdef DEBUG
void UA_Int64_print(const UA_Int64 *p, FILE *stream) {
    if(!p || !stream) return;
    fprintf(stream, "%" PRIi64, *p);
}
#endif

/* UInt64 */
UA_TYPE_DEFAULT(UA_UInt64)
#ifdef DEBUG
void UA_UInt64_print(const UA_UInt64 *p, FILE *stream) {
    if(!p || !stream) return;
    fprintf(stream, "%" PRIu64, *p);
}
#endif

/* Float */
UA_TYPE_DEFAULT(UA_Float)
#ifdef DEBUG
void UA_Float_print(const UA_Float *p, FILE *stream) {
    if(!p || !stream) return;
    fprintf(stream, "%f", *p);
}
#endif

/* Double */
UA_TYPE_DEFAULT(UA_Double)
#ifdef DEBUG
void UA_Double_print(const UA_Double *p, FILE *stream) {
    if(!p || !stream) return;
    fprintf(stream, "%f", *p);
}
#endif

/* String */
UA_TYPE_NEW_DEFAULT(UA_String)
void UA_String_init(UA_String *p) {
    p->length = -1;
    p->data   = UA_NULL;
}

UA_TYPE_DELETE_DEFAULT(UA_String)
void UA_String_deleteMembers(UA_String *p) {
    UA_free(p->data);
    UA_String_init(p);
}

UA_StatusCode UA_String_copy(UA_String const *src, UA_String *dst) {
    UA_String_init(dst);
    if(src->length > 0) {
        if(!(dst->data = UA_alloc(src->length)))
            return UA_STATUSCODE_BADOUTOFMEMORY;
        UA_memcpy((void *)dst->data, src->data, src->length);
    }
    dst->length = src->length;
    return UA_STATUSCODE_GOOD;
}

#ifdef DEBUG
void UA_String_print(const UA_String *p, FILE *stream) {
    fprintf(stream, "(UA_String){%d,", p->length);
    if(p->data)
        fprintf(stream, "\"%.*s\"}", p->length, p->data);
    else
        fprintf(stream, "UA_NULL}");
}
#endif

/* The c-string needs to be null-terminated. the string cannot be smaller than zero. */
UA_Int32 UA_String_copycstring(char const *src, UA_String *dst) {
    UA_Int32 length = strlen(src);
    if(length == 0) {
        dst->length = 0;
        dst->data = UA_NULL;
        return UA_STATUSCODE_GOOD;
    }
    dst->data = UA_alloc(length);
    if(dst->data != UA_NULL) {
        memcpy(dst->data, src, length);
        dst->length = length;
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
#ifndef __MINGW32__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
    // vsnprintf should only take a literal and no variable to be secure
    UA_Int32 len = vsnprintf(src, UA_STRING_COPYPRINTF_BUFSIZE, fmt, ap);
#ifndef __MINGW32__
#pragma GCC diagnostic pop
#endif
    va_end(ap);
    if(len < 0)  // FIXME: old glibc 2.0 would return -1 when truncated
        return UA_STATUSCODE_BADINTERNALERROR;
    // since glibc 2.1 vsnprintf returns len that would have resulted if buf were large enough
    len = ( len > UA_STRING_COPYPRINTF_BUFSIZE ? UA_STRING_COPYPRINTF_BUFSIZE : len );
    if(!(dst->data = UA_alloc(dst->length)))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_memcpy((void *)dst->data, src, len);
    dst->length = len;
    return UA_STATUSCODE_GOOD;
}

UA_Boolean UA_String_equal(const UA_String *string1, const UA_String *string2) {
    if(string1->length <= 0 && string2->length <= 0)
        return UA_TRUE;
    if(string1->length != string2->length)
        return UA_FALSE;

    // casts are needed to overcome signed warnings
    UA_Int32 is = strncmp((char const *)string1->data, (char const *)string2->data, string1->length);
    return (is == 0) ? UA_TRUE : UA_FALSE;
}

#ifdef DEBUG
void UA_String_printf(char const *label, const UA_String *string) {
    printf("%s {Length=%d, Data=%.*s}\n", label, string->length,
           string->length, (char *)string->data);
}
#endif

#ifdef DEBUG
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
#endif

#ifdef DEBUG
void UA_String_printx_hex(char const *label, const UA_String *string) {
    printf("%s {Length=%d, Data=", label, string->length);
    if(string->length > 0) {
        for(UA_Int32 i = 0;i < string->length;i++)
            printf("%c%x", i == 0 ? '{' : ',', (string->data)[i]);
    } else
        printf("{");
    printf("}}\n");
}
#endif

/* DateTime */
UA_TYPE_AS(UA_DateTime, UA_Int64)

// Number of seconds from 1 Jan. 1601 00:00 to 1 Jan 1970 00:00 UTC
#define FILETIME_UNIXTIME_BIAS_SEC 11644473600LL
// Factors
#define HUNDRED_NANOSEC_PER_USEC 10LL
#define HUNDRED_NANOSEC_PER_SEC (HUNDRED_NANOSEC_PER_USEC * 1000000LL)

#ifdef MSVC
static const unsigned __int64 epoch = 116444736000000000;
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

// IEC 62541-6 §5.2.2.5  A DateTime value shall be encoded as a 64-bit signed integer
// which represents the number of 100 nanosecond intervals since January 1, 1601 (UTC).
UA_DateTime UA_DateTime_now() {
    UA_DateTime    dateTime;
    struct timeval tv;
    gettimeofday(&tv, UA_NULL);
    dateTime = (tv.tv_sec + FILETIME_UNIXTIME_BIAS_SEC)
               * HUNDRED_NANOSEC_PER_SEC + tv.tv_usec * HUNDRED_NANOSEC_PER_USEC;
    return dateTime;
}

UA_DateTimeStruct UA_DateTime_toStruct(UA_DateTime time) {
    UA_DateTimeStruct dateTimeStruct;
    //calcualting the the milli-, micro- and nanoseconds
    UA_DateTime       timeTemp;
    timeTemp = (time-((time/10)*10))*100; //getting the last digit -> *100 for the 100 nanaseconds resolution
    dateTimeStruct.nanoSec  = timeTemp;   //123 456 7 -> 700 nanosec;
    timeTemp = (time-((time/10000)*10000))/10;
    dateTimeStruct.microSec = timeTemp;   //123 456 7 -> 456 microsec
    timeTemp = (time-((time/10000000)*10000000))/10000;
    dateTimeStruct.milliSec = timeTemp;   //123 456 7 -> 123 millisec

    //calculating the unix time with #include <time.h>
    time_t    timeInSec = time/10000000; //converting the nanoseconds time in unixtime
    struct tm ts;
    ts = *gmtime(&timeInSec);
    dateTimeStruct.sec    = ts.tm_sec;
    dateTimeStruct.min    = ts.tm_min;
    dateTimeStruct.hour   = ts.tm_hour;
    dateTimeStruct.day    = ts.tm_mday;
    dateTimeStruct.mounth = ts.tm_mon+1;
    dateTimeStruct.year   = ts.tm_year + 1900;

    return dateTimeStruct;
}

UA_StatusCode UA_DateTime_toString(UA_DateTime time, UA_String *timeString) {
    // length of the string is 31 (incl. \0 at the end)
    if(!(timeString->data = UA_alloc(32)))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    timeString->length = 31;

    UA_DateTimeStruct tSt = UA_DateTime_toStruct(time);
    sprintf((char*)timeString->data, "%2d/%2d/%4d %2d:%2d:%2d.%3d.%3d.%3d", tSt.mounth, tSt.day, tSt.year,
            tSt.hour, tSt.min, tSt.sec, tSt.milliSec, tSt.microSec, tSt.nanoSec);
    return UA_STATUSCODE_GOOD;
}

/* Guid */
UA_TYPE_DELETE_DEFAULT(UA_Guid)
UA_TYPE_DELETEMEMBERS_NOACTION(UA_Guid)

UA_Boolean UA_Guid_equal(const UA_Guid *g1, const UA_Guid *g2) {
    if(memcmp(g1, g2, sizeof(UA_Guid)) == 0)
        return UA_TRUE;
    return UA_FALSE;
}

void UA_Guid_init(UA_Guid *p) {
    p->data1 = 0;
    p->data2 = 0;
    p->data3 = 0;
    memset(p->data4, 0, sizeof(UA_Byte)*8);
}

UA_TYPE_NEW_DEFAULT(UA_Guid)
UA_StatusCode UA_Guid_copy(UA_Guid const *src, UA_Guid *dst) {
    UA_memcpy((void *)dst, (void *)src, sizeof(UA_Guid));
    return UA_STATUSCODE_GOOD;
}

#ifdef DEBUG
void UA_Guid_print(const UA_Guid *p, FILE *stream) {
    fprintf(stream, "(UA_Guid){%u, %u %u {%x,%x,%x,%x,%x,%x,%x,%x}}", p->data1, p->data2, p->data3, p->data4[0],
            p->data4[1], p->data4[2], p->data4[3], p->data4[4], p->data4[5], p->data4[6], p->data4[7]);
}
#endif

/* ByteString */
UA_TYPE_AS(UA_ByteString, UA_String)
UA_Boolean UA_ByteString_equal(const UA_ByteString *string1, const UA_ByteString *string2) {
    return UA_String_equal((const UA_String *)string1, (const UA_String *)string2);
}

#ifdef DEBUG
void UA_ByteString_printf(char *label, const UA_ByteString *string) {
    UA_String_printf(label, (UA_String *)string);
}
#endif

#ifdef DEBUG
void UA_ByteString_printx(char *label, const UA_ByteString *string) {
    UA_String_printx(label, (UA_String *)string);
}
#endif

#ifdef DEBUG
void UA_ByteString_printx_hex(char *label, const UA_ByteString *string) {
    UA_String_printx_hex(label, (UA_String *)string);
}
#endif

UA_Byte       UA_Byte_securityPoliceNoneData[] = "http://opcfoundation.org/UA/SecurityPolicy#None";
// sizeof()-1 : discard the implicit null-terminator of the c-char-string
UA_ByteString UA_ByteString_securityPoliceNone =
{ sizeof(UA_Byte_securityPoliceNoneData)-1, UA_Byte_securityPoliceNoneData };

/** Creates a ByteString of the indicated length. The content is not set to zero. */
UA_StatusCode UA_ByteString_newMembers(UA_ByteString *p, UA_Int32 length) {
    if(length > 0) {
        if(!(p->data = UA_alloc(length)))
            return UA_STATUSCODE_BADOUTOFMEMORY;
        p->length = length;
        return UA_STATUSCODE_GOOD;
    }

    p->data   = UA_NULL;
    if(length < 0)
        p->length = -1;
    else
        p->length = 0;
    return UA_STATUSCODE_GOOD;
}

/* XmlElement */
UA_TYPE_AS(UA_XmlElement, UA_ByteString)

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

UA_Boolean UA_NodeId_isBasicType(UA_NodeId const *id) {
    return id->namespaceIndex == 0 &&
           id->identifierType == UA_NODEIDTYPE_NUMERIC &&
           id->identifier.numeric <= UA_DIAGNOSTICINFO;
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

#ifdef DEBUG
void UA_NodeId_print(const UA_NodeId *p, FILE *stream) {
    fprintf(stream, "(UA_NodeId){");
    switch(p->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        fprintf(stream, "UA_NODEIDTYPE_NUMERIC");
        break;

    case UA_NODEIDTYPE_STRING:
        fprintf(stream, "UA_NODEIDTYPE_STRING");
        break;

    case UA_NODEIDTYPE_BYTESTRING:
        fprintf(stream, "UA_NODEIDTYPE_BYTESTRING");
        break;

    case UA_NODEIDTYPE_GUID:
        fprintf(stream, "UA_NODEIDTYPE_GUID");
        break;

    default:
        fprintf(stream, "ERROR");
        break;
    }
    fprintf(stream, ",%u,", p->namespaceIndex);
    switch(p->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        fprintf(stream, ".identifier.numeric=%u", p->identifier.numeric);
        break;

    case UA_NODEIDTYPE_STRING:
        fprintf(stream, ".identifier.string=%.*s", p->identifier.string.length, p->identifier.string.data);
        break;

    case UA_NODEIDTYPE_BYTESTRING:
        fprintf(stream, ".identifier.byteString=%.*s", p->identifier.byteString.length,
                p->identifier.byteString.data);
        break;

    case UA_NODEIDTYPE_GUID:
        fprintf(stream, ".identifer.guid=");
        UA_Guid_print(&p->identifier.guid, stream);
        break;

    default:
        fprintf(stream, "ERROR");
        break;
    }
    fprintf(stream, "}");
}
#endif

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

#ifdef DEBUG
void UA_ExpandedNodeId_print(const UA_ExpandedNodeId *p, FILE *stream) {
    fprintf(stream, "(UA_ExpandedNodeId){");
    UA_NodeId_print(&p->nodeId, stream);
    fprintf(stream, ",");
    UA_String_print(&p->namespaceUri, stream);
    fprintf(stream, ",");
    UA_UInt32_print(&p->serverIndex, stream);
    fprintf(stream, "}");
}
#endif

UA_Boolean UA_ExpandedNodeId_isNull(const UA_ExpandedNodeId *p) {
    return UA_NodeId_isNull(&p->nodeId);
}

/* StatusCode */
UA_TYPE_AS(UA_StatusCode, UA_UInt32)

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

#ifdef DEBUG
void UA_QualifiedName_print(const UA_QualifiedName *p, FILE *stream) {
    fprintf(stream, "(UA_QualifiedName){");
    UA_UInt16_print(&p->namespaceIndex, stream);
    fprintf(stream, ",");
    UA_String_print(&p->name, stream);
    fprintf(stream, "}");
}
#endif

#ifdef DEBUG
void UA_QualifiedName_printf(char const *label, const UA_QualifiedName *qn) {
    printf("%s {NamespaceIndex=%u, Length=%d, Data=%.*s}\n", label, qn->namespaceIndex,
           qn->name.length, qn->name.length, (char *)qn->name.data);
}
#endif

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

#ifdef DEBUG
void UA_LocalizedText_print(const UA_LocalizedText *p, FILE *stream) {
    fprintf(stream, "(UA_LocalizedText){");
    UA_String_print(&p->locale, stream);
    fprintf(stream, ",");
    UA_String_print(&p->text, stream);
    fprintf(stream, "}");
}
#endif

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

#ifdef DEBUG
void UA_ExtensionObject_print(const UA_ExtensionObject *p, FILE *stream) {
    fprintf(stream, "(UA_ExtensionObject){");
    UA_NodeId_print(&p->typeId, stream);
    if(p->encoding == UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING)
        fprintf(stream, ",UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING,");
    else if(p->encoding == UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML)
        fprintf(stream, ",UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML,");
    else
        fprintf(stream, ",UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED,");
    UA_ByteString_print(&p->body, stream);
    fprintf(stream, "}");
}
#endif

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

#ifdef DEBUG
void UA_DataValue_print(const UA_DataValue *p, FILE *stream) {
    fprintf(stream, "(UA_DataValue){");
    UA_Byte_print(&p->encodingMask, stream);
    fprintf(stream, ",");
    UA_Variant_print(&p->value, stream);
    fprintf(stream, ",");
    UA_StatusCode_print(&p->status, stream);
    fprintf(stream, ",");
    UA_DateTime_print(&p->sourceTimestamp, stream);
    fprintf(stream, ",");
    UA_Int16_print(&p->sourcePicoseconds, stream);
    fprintf(stream, ",");
    UA_DateTime_print(&p->serverTimestamp, stream);
    fprintf(stream, ",");
    UA_Int16_print(&p->serverPicoseconds, stream);
    fprintf(stream, "}");
}
#endif

/* Variant */
UA_TYPE_DELETE_DEFAULT(UA_Variant)
void UA_Variant_deleteMembers(UA_Variant *p) {
    if(p->storageType == UA_VARIANT_DATA) {
        if(p->storage.data.dataPtr) {
            if(p->storage.data.arrayLength == 1)
                p->vt->delete(p->storage.data.dataPtr);
            else
                UA_Array_delete(p->storage.data.dataPtr, p->storage.data.arrayLength, p->vt);
            p->storage.data.dataPtr = UA_NULL;
        }

        if(p->storage.data.arrayDimensions) {
            UA_free(p->storage.data.arrayDimensions);
            p->storage.data.arrayDimensions = UA_NULL;
        }
        return;
    }

    if(p->storageType == UA_VARIANT_DATASOURCE) {
        p->storage.datasource.delete(p->storage.datasource.identifier);
    }
}

UA_TYPE_NEW_DEFAULT(UA_Variant)
void UA_Variant_init(UA_Variant *p) {
    p->storageType = UA_VARIANT_DATA;
    p->storage.data.arrayLength = -1;  // no element, p->data == UA_NULL
    p->storage.data.dataPtr        = UA_NULL;
    p->storage.data.arrayDimensions       = UA_NULL;
    p->storage.data.arrayDimensionsLength = -1;
    p->vt = &UA_TYPES[UA_INVALIDTYPE];
}

/** This function performs a deep copy. The resulting StorageType is UA_VARIANT_DATA. */
UA_StatusCode UA_Variant_copy(UA_Variant const *src, UA_Variant *dst) {
    UA_Variant_init(dst);
    // get the data
    UA_VariantData *dstdata = &dst->storage.data;
    const UA_VariantData *srcdata;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(src->storageType == UA_VARIANT_DATA || src->storageType == UA_VARIANT_DATA_NODELETE)
        srcdata = &src->storage.data;
    else {
        retval |= src->storage.datasource.read(src->storage.datasource.identifier, &srcdata);
        if(retval)
            return retval;
    }

    // now copy the data to the destination
    retval |= UA_Array_copy(srcdata->dataPtr, srcdata->arrayLength, src->vt, &dstdata->dataPtr);
    if(retval == UA_STATUSCODE_GOOD) {
        dst->storageType = UA_VARIANT_DATA;
        dst->vt = src->vt;
        dstdata->arrayLength = srcdata->arrayLength;
        if(srcdata->arrayDimensions) {
            retval |= UA_Array_copy(srcdata->arrayDimensions, srcdata->arrayDimensionsLength, &UA_TYPES[UA_INT32],
                                    (void **)&dstdata->arrayDimensions);
            if(retval == UA_STATUSCODE_GOOD)
                dstdata->arrayDimensionsLength = srcdata->arrayDimensionsLength;
            else {
                UA_Variant_deleteMembers(dst);
                UA_Variant_init(dst);
            }
        }
    } 

    // release the data source if necessary
    if(src->storageType == UA_VARIANT_DATASOURCE)
        src->storage.datasource.release(src->storage.datasource.identifier, srcdata);

    return retval;
}

/** Copies data into a variant. The target variant has always a storagetype UA_VARIANT_DATA */
UA_StatusCode UA_Variant_copySetValue(UA_Variant *v, const UA_VTable_Entry *vt, const void *value) {
    UA_Variant_init(v);
    v->vt = vt;
    v->storage.data.arrayLength = 1; // no array but a single entry
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if((v->storage.data.dataPtr = vt->new()))
        retval |= vt->copy(value, v->storage.data.dataPtr);
    else
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
    if(retval) {
        UA_Variant_deleteMembers(v);
        UA_Variant_init(v);
    }
    return retval;
}

UA_StatusCode UA_Variant_copySetArray(UA_Variant *v, const UA_VTable_Entry *vt, UA_Int32 arrayLength, const void *array) {
    UA_Variant_init(v);
    v->vt = vt;
    v->storage.data.arrayLength = arrayLength;
    UA_StatusCode retval = UA_Array_copy(array, arrayLength, vt, &v->storage.data.dataPtr);
    if(retval) {
        UA_Variant_deleteMembers(v);
        UA_Variant_init(v);
    }
    return retval;
}

#ifdef DEBUG
void UA_Variant_print(const UA_Variant *p, FILE *stream) {
    UA_UInt32 ns0id = UA_ns0ToVTableIndex(&p->vt->typeId);
    if(p->storageType == UA_VARIANT_DATASOURCE) {
        fprintf(stream, "Variant from a Datasource");
        return;
    }
    fprintf(stream, "(UA_Variant){/*%s*/", p->vt->name);
    if(p->vt == &UA_TYPES[ns0id])
        fprintf(stream, "UA_TYPES[%d]", ns0id);
    else
        fprintf(stream, "ERROR (not a builtin type)");
    UA_Int32_print(&p->storage.data.arrayLength, stream);
    fprintf(stream, ",");
    UA_Array_print(p->storage.data.dataPtr, p->storage.data.arrayLength, p->vt, stream);
    fprintf(stream, ",");
    UA_Int32_print(&p->storage.data.arrayDimensionsLength, stream);
    fprintf(stream, ",");
    UA_Array_print(p->storage.data.arrayDimensions, p->storage.data.arrayDimensionsLength,
                   &UA_TYPES[UA_INT32], stream);
    fprintf(stream, "}");
}
#endif

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
        if((dst->innerDiagnosticInfo = UA_alloc(sizeof(UA_DiagnosticInfo))))
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

#ifdef DEBUG
void UA_DiagnosticInfo_print(const UA_DiagnosticInfo *p, FILE *stream) {
    fprintf(stream, "(UA_DiagnosticInfo){");
    UA_Byte_print(&p->encodingMask, stream);
    fprintf(stream, ",");
    UA_Int32_print(&p->symbolicId, stream);
    fprintf(stream, ",");
    UA_Int32_print(&p->namespaceUri, stream);
    fprintf(stream, ",");
    UA_Int32_print(&p->localizedText, stream);
    fprintf(stream, ",");
    UA_Int32_print(&p->locale, stream);
    fprintf(stream, ",");
    UA_String_print(&p->additionalInfo, stream);
    fprintf(stream, ",");
    UA_StatusCode_print(&p->innerStatusCode, stream);
    fprintf(stream, ",");
    if(p->innerDiagnosticInfo) {
        fprintf(stream, "&");
        UA_DiagnosticInfo_print(p->innerDiagnosticInfo, stream);
    } else
        fprintf(stream, "UA_NULL");
    fprintf(stream, "}");
}
#endif

/* InvalidType */
void UA_InvalidType_delete(UA_InvalidType *p) {
    return;
}

void UA_InvalidType_deleteMembers(UA_InvalidType *p) {
    return;
}

void UA_InvalidType_init(UA_InvalidType *p) {
    return;
}

UA_StatusCode UA_InvalidType_copy(UA_InvalidType const *src, UA_InvalidType *dst) {
    return UA_STATUSCODE_BADINTERNALERROR;
}

UA_InvalidType * UA_InvalidType_new() {
    return UA_NULL;
}

#ifdef DEBUG
void UA_InvalidType_print(const UA_InvalidType *p, FILE *stream) {
    fprintf(stream, "(UA_InvalidType){ERROR (invalid type)}");
}
#endif

/*********/
/* Array */
/*********/

UA_StatusCode UA_Array_new(void **p, UA_Int32 noElements, const UA_VTable_Entry *vt) {
    if(noElements <= 0) {
        *p = UA_NULL;
        return UA_STATUSCODE_GOOD;
    }
    
    // Arrays cannot be larger than 2^16 elements. This was randomly chosen so
    // that the development VM does not blow up during fuzzing tests.
    if(noElements > (1<<15)) {
        *p = UA_NULL;
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    if(!(*p = UA_alloc(vt->memSize * noElements)))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_Array_init(*p, noElements, vt);
    return UA_STATUSCODE_GOOD;
}

void UA_Array_init(void *p, UA_Int32 noElements, const UA_VTable_Entry *vt) {
    UA_Byte *cp = (UA_Byte *)p; // so compilers allow pointer arithmetic
    UA_UInt32 memSize = vt->memSize;
    for(UA_Int32 i = 0;i<noElements;i++) {
        vt->init(cp);
        cp += memSize;
    }
}

void UA_Array_delete(void *p, UA_Int32 noElements, const UA_VTable_Entry *vt) {
    UA_Byte *cp = (UA_Byte *)p; // so compilers allow pointer arithmetic
    UA_UInt32 memSize = vt->memSize;
    for(UA_Int32 i = 0;i<noElements;i++) {
        vt->deleteMembers(cp);
        cp += memSize;
    }
    if(noElements > 0)
        UA_free(p);
}

UA_StatusCode UA_Array_copy(const void *src, UA_Int32 noElements, const UA_VTable_Entry *vt, void **dst) {
    UA_StatusCode retval = UA_Array_new(dst, noElements, vt);
    if(retval)
        return retval;

    UA_Byte *csrc = (UA_Byte *)src; // so compilers allow pointer arithmetic
    UA_Byte *cdst = (UA_Byte *)*dst;
    UA_UInt32 memSize = vt->memSize;
    UA_Int32  i       = 0;
    for(;i < noElements && retval == UA_STATUSCODE_GOOD;i++) {
        retval |= vt->copy(csrc, cdst);
        csrc   += memSize;
        cdst   += memSize;
    }

    if(retval) {
        i--; // undo last increase
        UA_Array_delete(*dst, i, vt);
        *dst = UA_NULL;
    }

    return retval;
}

#ifdef DEBUG
void UA_Array_print(const void *p, UA_Int32 noElements, const UA_VTable_Entry *vt, FILE *stream) {
    fprintf(stream, "(%s){", vt->name);
    char     *cp      = (char *)p; // so compilers allow pointer arithmetic
    UA_UInt32 memSize = vt->memSize;
    for(UA_Int32 i = 0;i < noElements;i++) {
        vt->print(cp, stream);
        fprintf(stream, ",");
        cp += memSize;
    }
}
#endif
