#include <stdio.h>  // printf
#include <stdlib.h> // alloc, free, vsnprintf
#include <string.h>
#include <stdarg.h> // va_start, va_end
#include <time.h>
#include <sys/time.h>
#include "util/ua_util.h"
#include "ua_types.h"
#include "ua_types_encoding_binary.h"
#include "ua_namespace_0.h"

/************/
/* Built-In */
/************/

/* Boolean */
UA_Int32 UA_Boolean_init(UA_Boolean *p) {
	if(p == UA_NULL) return UA_ERROR;
	*p = UA_FALSE;
	return UA_SUCCESS;
}

UA_TYPE_DELETE_DEFAULT(UA_Boolean)
UA_TYPE_DELETEMEMBERS_NOACTION(UA_Boolean)
UA_TYPE_NEW_DEFAULT(UA_Boolean)
UA_TYPE_COPY_DEFAULT(UA_Boolean)

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
UA_TYPE_DELETE_DEFAULT(UA_Float)
UA_TYPE_DELETEMEMBERS_NOACTION(UA_Float)
UA_Int32 UA_Float_init(UA_Float *p) {
	if(p == UA_NULL) return UA_ERROR;
	*p = (UA_Float)0.0;
	return UA_SUCCESS;
}
UA_TYPE_NEW_DEFAULT(UA_Float)
UA_TYPE_COPY_DEFAULT(UA_Float)

/* Double */
UA_TYPE_DEFAULT(UA_Double)

/* String */
UA_TYPE_NEW_DEFAULT(UA_String)
UA_TYPE_DELETE_DEFAULT(UA_String)
UA_Int32 UA_String_deleteMembers(UA_String *p) {
	UA_Int32 retval = UA_SUCCESS;
	if(p!= UA_NULL && p->data != UA_NULL) {
		retval   |= UA_free(p->data);
		p->data   = UA_NULL;
		p->length = -1;
	}
	return retval;
}
UA_Int32 UA_String_copy(UA_String const *src, UA_String *dst) {
	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
	UA_Int32 retval = UA_SUCCESS;
	dst->data   = UA_NULL;
	dst->length = -1;
	if(src->length > 0) {
		retval |= UA_alloc((void **)&dst->data, src->length);
		if(retval == UA_SUCCESS) {
			retval     |= UA_memcpy((void *)dst->data, src->data, src->length);
			dst->length = src->length;
		}
	}
	return retval;
}
UA_Int32 UA_String_copycstring(char const *src, UA_String *dst) {
	UA_Int32 retval = UA_SUCCESS;
	dst->length = strlen(src);
	dst->data   = UA_NULL;
	if(dst->length > 0) {
		retval |= UA_alloc((void **)&dst->data, dst->length);
		if(retval == UA_SUCCESS)
			retval |= UA_memcpy((void *)dst->data, src, dst->length);
	}
	return retval;
}

#define UA_STRING_COPYPRINTF_BUFSIZE 1024
UA_Int32 UA_String_copyprintf(char const *fmt, UA_String *dst, ...) {
	UA_Int32 retval = UA_SUCCESS;
	char     src[UA_STRING_COPYPRINTF_BUFSIZE];
	UA_Int32 len;
	va_list  ap;
	va_start(ap, dst);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
	// vsnprintf should only take a literal and no variable to be secure
	len = vsnprintf(src, UA_STRING_COPYPRINTF_BUFSIZE, fmt, ap);
#pragma GCC diagnostic pop
	va_end(ap);
	if(len < 0) {  // FIXME: old glibc 2.0 would return -1 when truncated
		dst->length = 0;
		dst->data   = UA_NULL;
		retval      = UA_ERR_INVALID_VALUE;
	} else {
		// since glibc 2.1 vsnprintf returns len that would have resulted if buf were large enough
		dst->length = ( len > UA_STRING_COPYPRINTF_BUFSIZE ? UA_STRING_COPYPRINTF_BUFSIZE : len );
		retval     |= UA_alloc((void **)&dst->data, dst->length);
		if(retval == UA_SUCCESS)
			retval |= UA_memcpy((void *)dst->data, src, dst->length);
	}
	return retval;
}

UA_Int32 UA_String_init(UA_String *p) {
	if(p == UA_NULL) return UA_ERROR;
	p->length = -1;
	p->data   = UA_NULL;
	return UA_SUCCESS;
}

UA_Int32 UA_String_equal(const UA_String *string1, const UA_String *string2) {
	UA_Int32 retval;
	if(string1->length == 0 && string2->length == 0)
		retval = UA_EQUAL;
	else if(string1->length == -1 && string2->length == -1)
		retval = UA_EQUAL;
	else if(string1->length != string2->length)
		retval = UA_NOT_EQUAL;
	else {
		// casts are needed to overcome signed warnings
		UA_Int32 is = strncmp((char const *)string1->data, (char const *)string2->data, string1->length);
		retval = (is == 0) ? UA_EQUAL : UA_NOT_EQUAL;
	}
	return retval;
}

void UA_String_printf(char const *label, const UA_String *string) {
	printf("%s {Length=%d, Data=%.*s}\n", label, string->length,
	       string->length, (char *)string->data);
}

void UA_String_printx(char const *label, const UA_String *string) {
	if(string == UA_NULL) {
		printf("%s {NULL}\n", label); return;
	}
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
UA_TYPE_AS(UA_DateTime, UA_Int64)

// Number of seconds from 1 Jan. 1601 00:00 to 1 Jan 1970 00:00 UTC
#define FILETIME_UNIXTIME_BIAS_SEC 11644473600LL
// Factors
#define HUNDRED_NANOSEC_PER_USEC 10LL
#define HUNDRED_NANOSEC_PER_SEC (HUNDRED_NANOSEC_PER_USEC * 1000000LL)

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
	//strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
	//printf("%s\n", buf);
	dateTimeStruct.sec    = ts.tm_sec;
	dateTimeStruct.min    = ts.tm_min;
	dateTimeStruct.hour   = ts.tm_hour;
	dateTimeStruct.day    = ts.tm_mday;
	dateTimeStruct.mounth = ts.tm_mon+1;
	dateTimeStruct.year   = ts.tm_year + 1900;

	return dateTimeStruct;
}

UA_Int32 UA_DateTime_toString(UA_DateTime time, UA_String *timeString) {
	char *charBuf         = (char *)(*timeString).data;
	UA_DateTimeStruct tSt = UA_DateTime_toStruct(time);
	sprintf(charBuf, "%2d/%2d/%4d %2d:%2d:%2d.%3d.%3d.%3d", tSt.mounth, tSt.day, tSt.year,
	        tSt.hour, tSt.min, tSt.sec, tSt.milliSec, tSt.microSec, tSt.nanoSec);
	return UA_SUCCESS;
}

/* Guid */
UA_TYPE_DELETE_DEFAULT(UA_Guid)
UA_Int32 UA_Guid_deleteMembers(UA_Guid *p) {
	return UA_SUCCESS;
}

UA_Int32 UA_Guid_equal(const UA_Guid *g1, const UA_Guid *g2) {
	return memcmp(g1, g2, sizeof(UA_Guid));
}

UA_Int32 UA_Guid_init(UA_Guid *p) {
	if(p == UA_NULL) return UA_ERROR;
	p->data1 = 0;
	p->data2 = 0;
	p->data3 = 0;
	memset(p->data4, 8, sizeof(UA_Byte));
	return UA_SUCCESS;
}

UA_TYPE_NEW_DEFAULT(UA_Guid)
UA_Int32 UA_Guid_copy(UA_Guid const *src, UA_Guid *dst) {
	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_memcpy((void *)dst, (void *)src, sizeof(UA_Guid));
	return retval;
}

/* ByteString */
UA_TYPE_AS(UA_ByteString, UA_String)
UA_Int32 UA_ByteString_equal(const UA_ByteString *string1, const UA_ByteString *string2) {
	return UA_String_equal((const UA_String *)string1, (const UA_String *)string2);
}

void UA_ByteString_printf(char *label, const UA_ByteString *string) {
	UA_String_printf(label, (UA_String *)string);
}

void UA_ByteString_printx(char *label, const UA_ByteString *string) {
	UA_String_printx(label, (UA_String *)string);
}

void UA_ByteString_printx_hex(char *label, const UA_ByteString *string) {
	UA_String_printx_hex(label, (UA_String *)string);
}

UA_Byte       UA_Byte_securityPoliceNoneData[] = "http://opcfoundation.org/UA/SecurityPolicy#None";
// sizeof()-1 : discard the implicit null-terminator of the c-char-string
UA_ByteString UA_ByteString_securityPoliceNone =
{ sizeof(UA_Byte_securityPoliceNoneData)-1, UA_Byte_securityPoliceNoneData };

UA_Int32 UA_ByteString_newMembers(UA_ByteString *p, UA_Int32 length) {
	UA_Int32 retval = UA_SUCCESS;
	if((retval |= UA_alloc((void **)&p->data, length)) == UA_SUCCESS)
		p->length = length;
	else {
		p->length = -1;
		p->data   = UA_NULL;
	}
	return retval;
}

/* XmlElement */
UA_TYPE_AS(UA_XmlElement, UA_ByteString)

/* NodeId */
UA_Boolean UA_NodeId_isBasicType(UA_NodeId const *id) {
	return (id->namespace == 0 && id->identifier.numeric <= UA_DIAGNOSTICINFO);
}

UA_TYPE_DELETE_DEFAULT(UA_NodeId)
UA_Int32 UA_NodeId_deleteMembers(UA_NodeId *p) {
	UA_Int32 retval = UA_SUCCESS;
	if(p == UA_NULL) return retval;
	switch(p->encodingByte & UA_NODEIDTYPE_MASK) {
	case UA_NODEIDTYPE_TWOBYTE:
	case UA_NODEIDTYPE_FOURBYTE:
	case UA_NODEIDTYPE_NUMERIC:
		// nothing to do
		break;

	case UA_NODEIDTYPE_STRING: // Table 6, second entry
		retval |= UA_String_deleteMembers(&p->identifier.string);
		break;

	case UA_NODEIDTYPE_GUID: // Table 6, third entry
		retval |= UA_Guid_deleteMembers(&p->identifier.guid);
		break;

	case UA_NODEIDTYPE_BYTESTRING: // Table 6, "OPAQUE"
		retval |= UA_ByteString_deleteMembers(&p->identifier.byteString);
		break;
	}
	return retval;
}

void UA_NodeId_printf(char *label, const UA_NodeId *node) {
	UA_Int32 l;

	printf("%s {encodingByte=%d, namespace=%d,", label, (int)( node->encodingByte), (int)(node->namespace));
	switch(node->encodingByte & UA_NODEIDTYPE_MASK) {
	case UA_NODEIDTYPE_TWOBYTE:
	case UA_NODEIDTYPE_FOURBYTE:
	case UA_NODEIDTYPE_NUMERIC:
		printf("identifier=%d\n", node->identifier.numeric);
		break;
	case UA_NODEIDTYPE_STRING:
		l = ( node->identifier.string.length < 0 ) ? 0 : node->identifier.string.length;
		printf("identifier={length=%d, data=%.*s}",
			   node->identifier.string.length, l,
			   (char *)(node->identifier.string.data));
		break;
	case UA_NODEIDTYPE_BYTESTRING:
		l = ( node->identifier.byteString.length < 0 ) ? 0 : node->identifier.byteString.length;
		printf("identifier={Length=%d, data=%.*s}",
			   node->identifier.byteString.length, l,
			   (char *)(node->identifier.byteString.data));
		break;
	case UA_NODEIDTYPE_GUID:
		printf(
			   "guid={data1=%d, data2=%d, data3=%d, data4={length=%d, data=%.*s}}",
			   node->identifier.guid.data1, node->identifier.guid.data2,
			   node->identifier.guid.data3, 8,
			   8,
			   (char *)(node->identifier.guid.data4));
		break;
	default:
		printf("ups! shit happens");
		break;
	}
	printf("}\n");
}

UA_Int32 UA_NodeId_equal(const UA_NodeId *n1, const UA_NodeId *n2) {
	if(n1 == UA_NULL || n2 == UA_NULL || n1->encodingByte != n2->encodingByte || n1->namespace != n2->namespace)
		return UA_NOT_EQUAL;

	switch(n1->encodingByte & UA_NODEIDTYPE_MASK) {
	case UA_NODEIDTYPE_TWOBYTE:
	case UA_NODEIDTYPE_FOURBYTE:
	case UA_NODEIDTYPE_NUMERIC:
		if(n1->identifier.numeric == n2->identifier.numeric)
			return UA_EQUAL;
		else
			return UA_NOT_EQUAL;
	case UA_NODEIDTYPE_STRING:
		return UA_String_equal(&n1->identifier.string, &n2->identifier.string);
	case UA_NODEIDTYPE_GUID:
		return UA_Guid_equal(&n1->identifier.guid, &n2->identifier.guid);
	case UA_NODEIDTYPE_BYTESTRING:
		return UA_ByteString_equal(&n1->identifier.byteString, &n2->identifier.byteString);
	}
	return UA_NOT_EQUAL;
}

UA_Int32 UA_NodeId_init(UA_NodeId *p) {
	if(p == UA_NULL) return UA_ERROR;
	p->encodingByte = UA_NODEIDTYPE_TWOBYTE;
	p->namespace    = 0;
	memset(&p->identifier, 0, sizeof(p->identifier));
	return UA_SUCCESS;
}

UA_TYPE_NEW_DEFAULT(UA_NodeId)
UA_Int32 UA_NodeId_copy(UA_NodeId const *src, UA_NodeId *dst) {
	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Byte_copy(&src->encodingByte, &dst->encodingByte);

	switch(src->encodingByte & UA_NODEIDTYPE_MASK) {
	case UA_NODEIDTYPE_TWOBYTE:
	case UA_NODEIDTYPE_FOURBYTE:
	case UA_NODEIDTYPE_NUMERIC:
		// nothing to do
		retval |= UA_UInt16_copy(&src->namespace, &dst->namespace);
		retval |= UA_UInt32_copy(&src->identifier.numeric, &dst->identifier.numeric);
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
	}
	return retval;
}

UA_Boolean UA_NodeId_isNull(const UA_NodeId *p) {
	switch(p->encodingByte & UA_NODEIDTYPE_MASK) {
	case UA_NODEIDTYPE_TWOBYTE:
		if(p->identifier.numeric != 0) return UA_FALSE;
		break;

	case UA_NODEIDTYPE_FOURBYTE:
	case UA_NODEIDTYPE_NUMERIC:
		if(p->namespace != 0 || p->identifier.numeric != 0) return UA_FALSE;
		break;

	case UA_NODEIDTYPE_STRING:
		if(p->namespace != 0 || p->identifier.string.length != 0) return UA_FALSE;
		break;

	case UA_NODEIDTYPE_GUID:
		if(p->namespace != 0 ||
		   memcmp(&p->identifier.guid, (char[sizeof(UA_Guid)]) { 0 }, sizeof(UA_Guid)) != 0) return UA_FALSE;
		break;

	case UA_NODEIDTYPE_BYTESTRING:
		if(p->namespace != 0 || p->identifier.byteString.length != 0) return UA_FALSE;
		break;

	default:
		return UA_FALSE;
	}
	return UA_TRUE;
}

/* ExpandedNodeId */
UA_TYPE_DELETE_DEFAULT(UA_ExpandedNodeId)
UA_Int32 UA_ExpandedNodeId_deleteMembers(UA_ExpandedNodeId *p) {
	UA_Int32 retval = UA_SUCCESS;
	if(p == UA_NULL) return retval;
	retval |= UA_NodeId_deleteMembers(&p->nodeId);
	retval |= UA_String_deleteMembers(&p->namespaceUri);
	return retval;
}

UA_Int32 UA_ExpandedNodeId_init(UA_ExpandedNodeId *p) {
	if(p == UA_NULL) return UA_ERROR;
	UA_NodeId_init(&p->nodeId);
	UA_String_init(&p->namespaceUri);
	p->serverIndex = 0;
	return UA_SUCCESS;
}

UA_TYPE_NEW_DEFAULT(UA_ExpandedNodeId)
UA_Int32 UA_ExpandedNodeId_copy(UA_ExpandedNodeId const *src, UA_ExpandedNodeId *dst) {
	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
	UA_Int32 retval = UA_SUCCESS;
	UA_String_copy(&src->namespaceUri, &dst->namespaceUri);
	UA_NodeId_copy(&src->nodeId, &dst->nodeId);
	UA_UInt32_copy(&src->serverIndex, &dst->serverIndex);
	return retval;
}

UA_Boolean UA_ExpandedNodeId_isNull(const UA_ExpandedNodeId *p) {
	return UA_NodeId_isNull(&p->nodeId);
}

/* StatusCode */
UA_TYPE_AS(UA_StatusCode, UA_UInt32)

/* QualifiedName */
UA_TYPE_DELETE_DEFAULT(UA_QualifiedName)
UA_Int32 UA_QualifiedName_deleteMembers(UA_QualifiedName *p) {
	UA_Int32 retval = UA_SUCCESS;
	if(p == UA_NULL) return retval;
	retval |= UA_String_deleteMembers(&p->name);
	return retval;
}

UA_Int32 UA_QualifiedName_init(UA_QualifiedName *p) {
	if(p == UA_NULL) return UA_ERROR;
	UA_String_init(&p->name);
	p->namespaceIndex = 0;
	return UA_SUCCESS;
}

UA_TYPE_NEW_DEFAULT(UA_QualifiedName)
UA_Int32 UA_QualifiedName_copy(UA_QualifiedName const *src, UA_QualifiedName *dst) {
	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_String_copy(&src->name, &dst->name);
	retval |= UA_UInt16_copy(&src->namespaceIndex, &dst->namespaceIndex);
	return retval;

}

void UA_QualifiedName_printf(char const *label, const UA_QualifiedName *qn) {
	printf("%s {NamespaceIndex=%u, Length=%d, Data=%.*s}\n", label, qn->namespaceIndex,
		   qn->name.length, qn->name.length, (char *)qn->name.data);
}

/* LocalizedText */
UA_TYPE_DELETE_DEFAULT(UA_LocalizedText)
UA_Int32 UA_LocalizedText_deleteMembers(UA_LocalizedText *p) {
	if(p == UA_NULL) return UA_SUCCESS;
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_String_delete(p->locale);
	p->locale = UA_NULL;
	retval |= UA_String_delete(p->text);
	p->text = UA_NULL;
	return retval;
}

UA_Int32 UA_LocalizedText_init(UA_LocalizedText *p) {
	if(p == UA_NULL) return UA_ERROR;
	p->locale = UA_NULL;
	p->text = UA_NULL;
	return UA_SUCCESS;
}

UA_TYPE_NEW_DEFAULT(UA_LocalizedText)
UA_Int32 UA_LocalizedText_copycstring(char const *src, UA_LocalizedText *dst) {
	if(dst == UA_NULL) return UA_ERROR;
	UA_Int32 retval = UA_SUCCESS;

	retval |= UA_LocalizedText_init(dst);
	retval |= UA_String_new(&dst->locale);
	if(retval != UA_SUCCESS) return retval;
	retval |= UA_String_copycstring("EN", dst->locale); // TODO: Are language codes upper case?
	if(retval != UA_SUCCESS) return retval;
	retval |= UA_String_new(&dst->text);
	if(retval != UA_SUCCESS) return retval;
	retval |= UA_String_copycstring(src, dst->text);
	return retval;
}

UA_Int32 UA_LocalizedText_copy(UA_LocalizedText const *src, UA_LocalizedText *dst) {
	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
	UA_Int32 retval = UA_SUCCESS;

	retval |= UA_LocalizedText_init(dst);
	if(src->locale != UA_NULL) {
		retval |= UA_String_new(&dst->locale);
		if(retval != UA_SUCCESS) return retval;
		retval |= UA_String_copy(src->locale, dst->locale);
		if(retval != UA_SUCCESS) return retval;
	}
	if(src->text != UA_NULL) {
		retval |= UA_String_new(&dst->text);
		if(retval != UA_SUCCESS) return retval;
		retval |= UA_String_copy(src->text, dst->text);
	}
	return retval;
}

/* ExtensionObject */
UA_TYPE_DELETE_DEFAULT(UA_ExtensionObject)
UA_Int32 UA_ExtensionObject_deleteMembers(UA_ExtensionObject *p) {
	UA_Int32 retval = UA_SUCCESS;
	if(p == UA_NULL) return retval;
	retval |= UA_NodeId_deleteMembers(&p->typeId);
	retval |= UA_ByteString_deleteMembers(&p->body);
	return retval;
}

UA_Int32 UA_ExtensionObject_init(UA_ExtensionObject *p) {
	if(p == UA_NULL) return UA_ERROR;
	UA_ByteString_init(&p->body);
	p->encoding = 0;
	UA_NodeId_init(&p->typeId);
	return UA_SUCCESS;
}

UA_TYPE_NEW_DEFAULT(UA_ExtensionObject)
UA_Int32 UA_ExtensionObject_copy(UA_ExtensionObject const *src, UA_ExtensionObject *dst) {
	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Byte_copy(&src->encoding, &dst->encoding);
	retval |= UA_ByteString_copy(&src->body, &dst->body);
	retval |= UA_NodeId_copy(&src->typeId, &dst->typeId);
	return retval;
}

/* DataValue */
UA_TYPE_DELETE_DEFAULT(UA_DataValue)
UA_Int32 UA_DataValue_deleteMembers(UA_DataValue *p) {
	UA_Int32 retval = UA_SUCCESS;
	if(p == UA_NULL) return retval;
	UA_Variant_deleteMembers(&p->value);
	return retval;
}

UA_Int32 UA_DataValue_init(UA_DataValue *p) {
	if(p == UA_NULL) return UA_ERROR;
	p->encodingMask      = 0;
	p->serverPicoseconds = 0;
	UA_DateTime_init(&p->serverTimestamp);
	p->sourcePicoseconds = 0;
	UA_DateTime_init(&p->sourceTimestamp);
	UA_StatusCode_init(&p->status);
	UA_Variant_init(&p->value);
	return UA_SUCCESS;
}

UA_TYPE_NEW_DEFAULT(UA_DataValue)
UA_Int32 UA_DataValue_copy(UA_DataValue const *src, UA_DataValue *dst) {
	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
	UA_Int32 retval = UA_SUCCESS;
	UA_Byte_copy(&src->encodingMask, &dst->encodingMask);
	UA_Int16_copy(&src->serverPicoseconds, &dst->serverPicoseconds);
	UA_DateTime_copy(&src->serverTimestamp, &dst->serverTimestamp);
	UA_Int16_copy(&src->sourcePicoseconds, &dst->sourcePicoseconds);
	UA_DateTime_copy(&src->sourceTimestamp, &dst->sourceTimestamp);
	UA_StatusCode_copy(&src->status, &dst->status);
	UA_Variant_copy(&src->value, &dst->value);
	return retval;
}

/* Variant */
UA_TYPE_DELETE_DEFAULT(UA_Variant)
UA_Int32 UA_Variant_deleteMembers(UA_Variant *p) {
	UA_Int32 retval = UA_SUCCESS;
	if(p == UA_NULL) return retval;

	UA_Boolean hasDimensions = p->arrayDimensions != UA_NULL;

	if(p->data != UA_NULL) {
		retval |= UA_Array_delete(p->data, p->arrayLength, p->vt);
		p->data = UA_NULL;
	}
	if(hasDimensions) {
		UA_free(p->arrayDimensions);
		p->arrayDimensions = UA_NULL;
	}
	return retval;
}

UA_Int32 UA_Variant_init(UA_Variant *p) {
	if(p == UA_NULL) return UA_ERROR;
	p->arrayLength  = -1; // no element, p->data == UA_NULL
	p->data         = UA_NULL;
	p->arrayDimensions       = UA_NULL;
	p->arrayDimensionsLength = -1;
	p->vt = &UA_.types[UA_INVALIDTYPE];
	return UA_SUCCESS;
}

UA_TYPE_NEW_DEFAULT(UA_Variant)
UA_Int32 UA_Variant_copy(UA_Variant const *src, UA_Variant *dst) {
	if(src == UA_NULL || dst == UA_NULL)
		return UA_ERROR;

	UA_Int32  retval = UA_SUCCESS;
	dst->vt = src->vt;
	retval |= UA_Int32_copy(&src->arrayLength, &dst->arrayLength);
	retval |= UA_Int32_copy(&src->arrayDimensionsLength, &dst->arrayDimensionsLength);
	retval |=  UA_Array_copy(src->data, src->arrayLength, src->vt, &dst->data);
	UA_Boolean hasDimensions = src->arrayDimensions != UA_NULL;
	if(hasDimensions)
		retval |= UA_Array_copy(src->arrayDimensions, src->arrayDimensionsLength,
		                        &UA_.types[UA_INT32], (void **)&dst->arrayDimensions);
	return retval;
}

UA_Int32 UA_Variant_copySetValue(UA_Variant *v, UA_VTable_Entry *vt, const void *value) {
	if(v == UA_NULL || vt == UA_NULL || value == UA_NULL)
		return UA_ERROR;
	UA_Variant_init(v);
	v->vt = vt;
	v->arrayLength = 1; // no array but a single entry
	UA_Int32 retval = UA_SUCCESS;
	retval |= vt->new(&v->data);
	if(retval == UA_SUCCESS)
		retval |= vt->copy(value, v->data);
	return retval;
}

UA_Int32 UA_Variant_copySetArray(UA_Variant *v, UA_VTable_Entry *vt, UA_Int32 arrayLength, const void *array) {
	if(v == UA_NULL || vt == UA_NULL || array == UA_NULL)
		return UA_ERROR;
	UA_Variant_init(v);
	v->vt = vt;
	v->arrayLength = arrayLength;
	return UA_Array_copy(array, arrayLength, vt, &v->data);
}

UA_Int32 UA_Variant_borrowSetValue(UA_Variant *v, UA_VTable_Entry *vt, const void *value) {
	if(v == UA_NULL || vt == UA_NULL || value == UA_NULL)
		return UA_ERROR;
	UA_Variant_init(v);
	v->vt = &UA_borrowed_.types[UA_ns0ToVTableIndex(&vt->typeId)];
	v->arrayLength = 1; // no array but a single entry
	v->data = (void *)value;
	return UA_SUCCESS;
}

UA_Int32 UA_Variant_borrowSetArray(UA_Variant *v, UA_VTable_Entry *vt, UA_Int32 arrayLength, const void *array) {
	if(v == UA_NULL || vt == UA_NULL || array == UA_NULL)
		return UA_ERROR;
	UA_Variant_init(v);
	v->vt = &UA_borrowed_.types[UA_ns0ToVTableIndex(&vt->typeId)];
	v->arrayLength = arrayLength;
	v->data = (void *)array;
	return UA_SUCCESS;
}

/* DiagnosticInfo */
UA_TYPE_DELETE_DEFAULT(UA_DiagnosticInfo)
UA_Int32 UA_DiagnosticInfo_deleteMembers(UA_DiagnosticInfo *p) {
	UA_Int32 retval = UA_SUCCESS;
	if(p == UA_NULL) return retval;
	if((p->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASK_INNERDIAGNOSTICINFO) && p->innerDiagnosticInfo != UA_NULL) {
		retval |= UA_DiagnosticInfo_delete(p->innerDiagnosticInfo);
		retval |= UA_String_deleteMembers(&p->additionalInfo);
		p->innerDiagnosticInfo = UA_NULL;
	}
	return retval;
}

UA_Int32 UA_DiagnosticInfo_init(UA_DiagnosticInfo *p) {
	if(p == UA_NULL) return UA_ERROR;
	UA_String_init(&p->additionalInfo);
	p->encodingMask        = 0;
	p->innerDiagnosticInfo = UA_NULL;
	UA_StatusCode_init(&p->innerStatusCode);
	p->locale              = 0;
	p->localizedText       = 0;
	p->namespaceUri        = 0;
	p->symbolicId          = 0;
	return UA_SUCCESS;
}

UA_TYPE_NEW_DEFAULT(UA_DiagnosticInfo)
UA_Int32 UA_DiagnosticInfo_copy(UA_DiagnosticInfo const *src, UA_DiagnosticInfo *dst) {
	if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_String_copy(&src->additionalInfo, &dst->additionalInfo);
	retval |= UA_Byte_copy(&src->encodingMask, &dst->encodingMask);
	retval |= UA_StatusCode_copy(&src->innerStatusCode, &dst->innerStatusCode);
	if(src->innerDiagnosticInfo) {
		retval |= UA_alloc((void **)&dst->innerDiagnosticInfo, sizeof(UA_DiagnosticInfo));
		if(retval == UA_SUCCESS)
			retval |= UA_DiagnosticInfo_copy(src->innerDiagnosticInfo, dst->innerDiagnosticInfo);
	} else
		dst->innerDiagnosticInfo = UA_NULL;
	retval |= UA_Int32_copy(&src->locale, &dst->locale);
	retval |= UA_Int32_copy(&src->localizedText, &dst->localizedText);
	retval |= UA_Int32_copy(&src->namespaceUri, &dst->namespaceUri);
	retval |= UA_Int32_copy(&src->symbolicId, &dst->symbolicId);

	return retval;
}

/* InvalidType */
UA_Int32 UA_InvalidType_free(UA_InvalidType *p) {
	return UA_ERR_INVALID_VALUE;
}

UA_Int32 UA_InvalidType_delete(UA_InvalidType *p) {
	return UA_ERR_INVALID_VALUE;
}

UA_Int32 UA_InvalidType_deleteMembers(UA_InvalidType *p) {
	return UA_ERR_INVALID_VALUE;
}

UA_Int32 UA_InvalidType_init(UA_InvalidType *p) {
	return UA_ERR_INVALID_VALUE;
}

UA_Int32 UA_InvalidType_copy(UA_InvalidType const *src, UA_InvalidType *dst) {
	return UA_ERR_INVALID_VALUE;
}

UA_Int32 UA_InvalidType_new(UA_InvalidType **p) {
	return UA_ERR_INVALID_VALUE;
}

/*********/
/* Array */
/*********/

UA_Int32 UA_Array_new(void **p, UA_Int32 noElements, UA_VTable_Entry *vt) {
	if(vt == UA_NULL)
		return UA_ERROR;
			
	if(noElements <= 0) {
		*p = UA_NULL;
		return UA_SUCCESS;
	}

	// FIXME! Arrays cannot be larger than 2**20.
	// This was randomly chosen so that the development VM does not blow up.
	if(noElements > 1048576) {
		*p = UA_NULL;
		return UA_ERROR;
	}
	
	UA_Int32 retval = UA_SUCCESS;
	retval = UA_alloc(p, vt->memSize * noElements);
	if(retval != UA_SUCCESS)
		return retval;

	retval = UA_Array_init(*p, noElements, vt);
	if(retval != UA_SUCCESS) {
		UA_free(*p);
		*p = UA_NULL;
	}
	return retval;
}

UA_Int32 UA_Array_init(void *p, UA_Int32 noElements, UA_VTable_Entry *vt) {
	if(p == UA_NULL || vt == UA_NULL)
		return UA_ERROR;

	UA_Int32 retval = UA_SUCCESS;
	char    *cp     = (char *)p; // so compilers allow pointer arithmetic
	UA_UInt32 memSize = vt->memSize;
	for(UA_Int32 i = 0;i < noElements && retval == UA_SUCCESS;i++) {
		retval |= vt->init(cp);
		cp     += memSize;
	}
	return retval;
}

UA_Int32 UA_Array_delete(void *p, UA_Int32 noElements, UA_VTable_Entry *vt) {
	if(p == UA_NULL)
		return UA_SUCCESS;
	if(vt == UA_NULL)
		return UA_ERROR;

	char    *cp     = (char *)p; // so compilers allow pointer arithmetic
	UA_UInt32 memSize = vt->memSize;
	UA_Int32 retval = UA_SUCCESS;
	for(UA_Int32 i = 0;i < noElements;i++) {
		retval |= vt->deleteMembers(cp);
		cp     += memSize;
	}
	UA_free(p);
	return retval;
}

UA_Int32 UA_Array_copy(const void *src, UA_Int32 noElements, UA_VTable_Entry *vt, void **dst) {
	if(src == UA_NULL || dst == UA_NULL || vt == UA_NULL)
		return UA_ERROR;

	UA_Int32 retval = UA_Array_new(dst, noElements, vt);
	if(retval != UA_SUCCESS){
		*dst = UA_NULL;
		return retval;
	}

	char *csrc = (char *)src; // so compilers allow pointer arithmetic
	char *cdst = (char *)*dst;
	UA_UInt32 memSize = vt->memSize;
	UA_Int32 i = 0;
	for(;i < noElements && retval == UA_SUCCESS;i++) {
		retval |= vt->copy(csrc, cdst);
		csrc   += memSize;
		cdst   += memSize;
	}

	if(retval != UA_SUCCESS) {
		i--; // undo last increase
		UA_Array_delete(*dst, i, vt);
		*dst = UA_NULL;
	}

	return retval;
}
