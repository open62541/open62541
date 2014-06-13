#include "ua_types_encoding_xml.h"
#include "ua_util.h"
#include "ua_namespace_0.h"
#include "ua_xml.h"

/* Boolean */

UA_Int32 UA_Boolean_copycstring(cstring src, UA_Boolean *dst) {
	*dst = UA_FALSE;
	if(0 == strncmp(src, "true", 4) || 0 == strncmp(src, "TRUE", 4))
		*dst = UA_TRUE;
	return UA_SUCCESS;
}

UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_Boolean)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_Boolean)
UA_Int32 UA_Boolean_decodeXML(XML_Stack *s, XML_Attr *attr, UA_Boolean *dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_Boolean entered with dst=%p,isStart=%d\n", (void * )dst, isStart));
	if(isStart) {
		if(dst == UA_NULL) {
			UA_Boolean_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void *)dst;
		}
		UA_Boolean_copycstring((cstring)attr[1], dst);
	}
	return UA_SUCCESS;
}

UA_TYPE_DECODEXML_FROM_BYTESTRING(UA_Boolean)

/* SByte */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_SByte)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_SByte)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(UA_SByte)

/* Byte */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_Byte)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_Byte)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(UA_Byte)

/* Int16 */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_Int16)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_Int16)

UA_Int32 UA_Int16_copycstring(cstring src, UA_Int16 *dst) {
	*dst = atoi(src);
	return UA_SUCCESS;
}

UA_Int32 UA_UInt16_copycstring(cstring src, UA_UInt16 *dst) {
	*dst = atoi(src);
	return UA_SUCCESS;
}

UA_Int32 UA_Int16_decodeXmlFromStack(XML_Stack *s, XML_Attr *attr, UA_Int16 *dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_Int32 entered with dst=%p,isStart=%d\n", (void * )dst, isStart));
	if(isStart) {
		if(dst == UA_NULL) {
			UA_Int16_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void *)dst;
		}
		UA_Int16_copycstring((cstring)attr[1], dst);
	}
	return UA_SUCCESS;
}

UA_TYPE_DECODEXML_FROM_BYTESTRING(UA_Int16)

/* UInt16 */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_UInt16)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_UInt16)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(UA_UInt16)

/* Int32 */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_Int32)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_Int32)

UA_Int32 UA_Int32_decodeXmlFromStack(XML_Stack *s, XML_Attr *attr, UA_Int32 *dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_Int32 entered with dst=%p,isStart=%d\n", (void * )dst, isStart));
	if(isStart) {
		if(dst == UA_NULL) {
			UA_Int32_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void *)dst;
		}
		*dst = atoi(attr[1]);
	}
	return UA_SUCCESS;
}
UA_TYPE_DECODEXML_FROM_BYTESTRING(UA_Int32)

/* UInt32 */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_UInt32)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_UInt32)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(UA_UInt32)

/* Int64 */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_Int64)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_Int64)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(UA_Int64)

/* UInt64 */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_UInt64)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_UInt64)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(UA_UInt64)

/* Float */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_Float)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_Float)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(UA_Float)

/* Double */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_Double)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_Double)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(UA_Double)

/* String */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_String)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_String)

UA_Int32 UA_String_decodeXmlFromStack(XML_Stack *s, XML_Attr *attr, UA_String *dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_String entered with dst=%p,isStart=%d\n", (void * )dst, isStart));
	UA_UInt32 i;
	if(isStart) {
		if(dst == UA_NULL) {
			UA_String_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void *)dst;
		}
		s->parent[s->depth].len = 0;
		XML_Stack_addChildHandler(s, "Data", strlen("Data"), (XML_decoder)UA_Text_decodeXmlFromStack, UA_BYTE,
		                          &(dst->data));
		XML_Stack_addChildHandler(s, "Length", strlen("Length"), (XML_decoder)UA_Int32_decodeXmlFromStack, UA_INT32,
		                          &(dst->length));
		XML_Stack_handleTextAsElementOf(s, "Data", 0);

		// set attributes
		for(i = 0;attr[i];i += 2) {
			if(0 == strncmp("Data", attr[i], strlen("Data")))
				UA_String_copycstring(attr[i + 1], dst);
			else
				printf("UA_String_decodeXml - Unknown attribute - name=%s, value=%s\n", attr[i], attr[i+1]);
		}
	} else {
		switch(s->parent[s->depth - 1].activeChild) {
		case 0:
			if(dst != UA_NULL && dst->data != UA_NULL && dst->length == -1)
				dst->length = strlen((char const *)dst->data);
			break;
		}
	}
	return UA_SUCCESS;
}

UA_TYPE_DECODEXML_FROM_BYTESTRING(UA_String)

/* DateTime */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_DateTime)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_DateTime)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(UA_DateTime)

/* Guid */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_Guid)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_Guid)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(UA_Guid)

/* ByteString */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_ByteString)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_ByteString)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(UA_ByteString)

/* XmlElement */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_XmlElement)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_XmlElement)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(UA_XmlElement)

/* NodeId */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_NodeId)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_NodeId)

UA_Int32 UA_NodeId_decodeXmlFromStack(XML_Stack *s, XML_Attr *attr, UA_NodeId *dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_NodeId entered with dst=%p,isStart=%d\n", (void * )dst, isStart));
	UA_UInt32 i;
	if(isStart) {
		if(dst == UA_NULL) {
			UA_NodeId_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void *)dst;
		}
		s->parent[s->depth].len = 0;
		XML_Stack_addChildHandler(s, "Namespace", strlen(
		                              "Namespace"), (XML_decoder)UA_Int16_decodeXmlFromStack, UA_INT16,
		                          &(dst->namespace));
		XML_Stack_addChildHandler(s, "Numeric", strlen(
		                              "Numeric"), (XML_decoder)UA_Int32_decodeXmlFromStack, UA_INT32,
		                          &(dst->identifier.numeric));
		XML_Stack_addChildHandler(s, "Identifier", strlen(
		                              "Identifier"), (XML_decoder)UA_String_decodeXmlFromStack, UA_STRING, UA_NULL);
		XML_Stack_handleTextAsElementOf(s, "Data", 2);

		// set attributes
		for(i = 0; attr[i]; i += 2) {
		if(0 == strncmp("Namespace", attr[i], strlen("Namespace")))
			dst->namespace = atoi(attr[i + 1]);
		else if(0 == strncmp("Numeric", attr[i], strlen("Numeric"))) {
		dst->identifier.numeric = atoi(attr[i + 1]);
		dst->encodingByte       = UA_NODEIDTYPE_FOURBYTE;
		} else
			printf("UA_NodeId_decodeXml - Unknown attribute name=%s, value=%s\n", attr[i], attr[i+1]);
		}
	} else {
		if(s->parent[s->depth - 1].activeChild == 2)
			UA_NodeId_copycstring((cstring)((UA_String *)attr)->data, dst, s->aliases);
	}
	return UA_SUCCESS;
}

UA_TYPE_DECODEXML_FROM_BYTESTRING(UA_NodeId)

/* ExpandedNodeId */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_ExpandedNodeId)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_ExpandedNodeId)

UA_Int32 UA_ExpandedNodeId_decodeXmlFromStack(XML_Stack *s, XML_Attr *attr, UA_ExpandedNodeId *dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_ExpandedNodeId entered with dst=%p,isStart=%d\n", (void * )dst, isStart));
	UA_UInt32 i;
	if(isStart) {
		if(dst == UA_NULL) {
			UA_ExpandedNodeId_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void *)dst;
		}
		s->parent[s->depth].len = 0;
		XML_Stack_addChildHandler(s, "NodeId", strlen(
		                              "NodeId"), (XML_decoder)UA_NodeId_decodeXmlFromStack, UA_NODEID, &(dst->nodeId));
		XML_Stack_addChildHandler(s, "Namespace", strlen(
		                              "Namespace"), (XML_decoder)UA_Int16_decodeXmlFromStack, UA_INT16,
		                          &(dst->nodeId.namespace));
		XML_Stack_addChildHandler(s, "Numeric", strlen("Numeric"), (XML_decoder)UA_Int32_decodeXmlFromStack, UA_INT32,
		                          &(dst->nodeId.identifier.numeric));
		XML_Stack_addChildHandler(s, "Id", strlen("Id"), (XML_decoder)UA_String_decodeXmlFromStack, UA_STRING, UA_NULL);
		XML_Stack_handleTextAsElementOf(s, "Data", 3);

		// set attributes
		for(i = 0; attr[i]; i += 2) {
		if(0 == strncmp("Namespace", attr[i], strlen("Namespace")))
			UA_UInt16_copycstring((cstring)attr[i + 1], &(dst->nodeId.namespace));
		else if(0 == strncmp("Numeric", attr[i], strlen("Numeric"))) {
		UA_NodeId_copycstring((cstring)attr[i + 1], &(dst->nodeId), s->aliases);
		} else if(0 == strncmp("NodeId", attr[i], strlen("NodeId")))
			UA_NodeId_copycstring((cstring)attr[i + 1], &(dst->nodeId), s->aliases);
		else
			printf("UA_ExpandedNodeId_decodeXml - unknown attribute name=%s, value=%s\n", attr[i], attr[i+1]);
		}
	}
	return UA_SUCCESS;
}

UA_TYPE_DECODEXML_FROM_BYTESTRING(UA_ExpandedNodeId)

/* StatusCode */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_StatusCode)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_StatusCode)
UA_Int32 UA_StatusCode_decodeXML(XML_Stack *s, XML_Attr *attr, UA_StatusCode *dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_StatusCode_decodeXML entered with dst=%p,isStart=%d\n", (void * )dst, isStart));
	return UA_ERR_NOT_IMPLEMENTED;
}
UA_TYPE_DECODEXML_FROM_BYTESTRING(UA_StatusCode)

/* QualifiedName */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_QualifiedName)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_QualifiedName)

UA_Int32 UA_QualifiedName_decodeXmlFromStack(XML_Stack *s, XML_Attr *attr, UA_QualifiedName *dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_QualifiedName entered with dst=%p,isStart=%d\n", (void * )dst, isStart));
	UA_UInt32 i;
	if(isStart) {
		if(dst == UA_NULL) {
			UA_QualifiedName_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void *)dst;
		}
		s->parent[s->depth].len = 0;
		XML_Stack_addChildHandler(s, "Name", strlen("Name"), (XML_decoder)UA_String_decodeXmlFromStack, UA_STRING,
		                          &(dst->name));
		XML_Stack_addChildHandler(s, "NamespaceIndex", strlen(
		                              "NamespaceIndex"), (XML_decoder)UA_Int16_decodeXmlFromStack, UA_STRING,
		                          &(dst->namespaceIndex));
		XML_Stack_handleTextAsElementOf(s, "Data", 0);

		// set attributes
		for(i = 0;attr[i];i += 2) {
			if(0 == strncmp("NamespaceIndex", attr[i], strlen("NamespaceIndex")))
				dst->namespaceIndex = atoi(attr[i + 1]);
			else if(0 == strncmp("Name", attr[i], strlen("Name")))
				UA_String_copycstring(attr[i + 1], &(dst->name));
			else
				perror("Unknown attribute");
		}
	}
	return UA_SUCCESS;
}

UA_TYPE_DECODEXML_FROM_BYTESTRING(UA_QualifiedName)

/* LocalizedText */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_LocalizedText)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_LocalizedText)

UA_Int32 UA_LocalizedText_decodeXmlFromStack(XML_Stack *s, XML_Attr *attr, UA_LocalizedText *dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_LocalizedText entered with dst=%p,isStart=%d\n", (void * )dst, isStart));
	UA_UInt32 i;
	if(isStart) {
		if(dst == UA_NULL) {
			UA_LocalizedText_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void *)dst;
		}
		// s->parent[s->depth].len = 0;
		XML_Stack_addChildHandler(s, "Text", strlen("Text"), (XML_decoder)UA_String_decodeXmlFromStack, UA_STRING,
		                          &(dst->text));
		XML_Stack_addChildHandler(s, "Locale", strlen(
		                              "Locale"), (XML_decoder)UA_String_decodeXmlFromStack, UA_STRING, &(dst->locale));
		XML_Stack_handleTextAsElementOf(s, "Data", 0);

		// set attributes
		for(i = 0;attr[i];i += 2) {
			if(0 == strncmp("Text", attr[i], strlen("Text"))) {
				UA_String_copycstring(attr[i + 1], dst->text);
			} else if(0 == strncmp("Locale", attr[i], strlen("Locale"))) {
				UA_String_copycstring(attr[i + 1], dst->locale);
			} else
				perror("Unknown attribute");
		}
	} else {
		switch(s->parent[s->depth - 1].activeChild) {
		case 0:
			//dst->encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
			break;

		case 1:
			//dst->encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE;
			break;

		default:
			break;
		}
	}
	return UA_SUCCESS;
}

UA_TYPE_DECODEXML_FROM_BYTESTRING(UA_LocalizedText)

/* ExtensionObject */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_ExtensionObject)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_ExtensionObject)

UA_Int32 UA_ExtensionObject_decodeXmlFromStack(XML_Stack *s, XML_Attr *attr, UA_ExtensionObject *dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_ExtensionObject entered with dst=%p,isStart=%d\n", (void * )dst, isStart));
	UA_UInt32 i;

	if(isStart) {
		// create a new object if called with UA_NULL
		if(dst == UA_NULL) {
			UA_ExtensionObject_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void *)dst;
		}

		s->parent[s->depth].len = 0;
		XML_Stack_addChildHandler(s, "TypeId", strlen(
		                              "TypeId"), (XML_decoder)UA_NodeId_decodeXmlFromStack, UA_NODEID, &(dst->typeId));
		// XML_Stack_addChildHandler(s, "Body", strlen("Body"), (XML_decoder) UA_Body_decodeXml, UA_LOCALIZEDTEXT, UA_NULL);

		// set attributes
		for(i = 0;attr[i];i += 2) {
			{
				DBG_ERR(XML_Stack_print(s));
				DBG_ERR(printf("%s - unknown attribute\n", attr[i]));
			}
		}
	}
	return UA_SUCCESS;
}

UA_TYPE_DECODEXML_FROM_BYTESTRING(UA_ExtensionObject)

/* DataValue */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_DataValue)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_DataValue)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(UA_DataValue)

/* Variant */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_Variant)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_Variant)

UA_Int32 UA_Variant_decodeXmlFromStack(XML_Stack *s, XML_Attr *attr, UA_Variant *dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_Variant entered with dst=%p,isStart=%d\n", (void * )dst, isStart));
	UA_UInt32 i;

	if(isStart) {
		// create a new object if called with UA_NULL
		if(dst == UA_NULL) {
			UA_Variant_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void *)dst;
		}

		s->parent[s->depth].len = 0;
		XML_Stack_addChildHandler(s, "ListOfExtensionObject", strlen(
		                              "ListOfExtensionObject"), (XML_decoder)UA_TypedArray_decodeXmlFromStack,
		                          UA_EXTENSIONOBJECT, UA_NULL);
		XML_Stack_addChildHandler(s, "ListOfLocalizedText", strlen(
		                              "ListOfLocalizedText"), (XML_decoder)UA_TypedArray_decodeXmlFromStack,
		                          UA_LOCALIZEDTEXT, UA_NULL);

		// set attributes
		for(i = 0;attr[i];i += 2) {
			{
				DBG_ERR(XML_Stack_print(s));
				DBG_ERR(printf("%s - unknown attribute\n", attr[i]));
			}
		}
	} else {
		if(s->parent[s->depth - 1].activeChild == 0 && attr != UA_NULL ) {  // ExtensionObject
			UA_TypedArray *array = (UA_TypedArray *)attr;
			DBG_VERBOSE(printf("UA_Variant_decodeXml - finished array: references=%p, size=%d\n", (void *)array,
			                   (array == UA_NULL) ? -1 : array->size));
			dst->arrayLength  = array->size;
			dst->data         = array->elements;
			dst->vt = &UA_.types[UA_EXTENSIONOBJECT];
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = UA_NULL;
		} else if(s->parent[s->depth - 1].activeChild == 1 && attr != UA_NULL ) {  // LocalizedText
			UA_TypedArray *array = (UA_TypedArray *)attr;
			DBG_VERBOSE(printf("UA_Variant_decodeXml - finished array: references=%p, size=%d\n", (void *)array,
			                   (array == UA_NULL) ? -1 : array->size));
			dst->arrayLength  = array->size;
			dst->data         = array->elements;
			dst->vt = &UA_.types[UA_LOCALIZEDTEXT];
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = UA_NULL;
		}
	}
	return UA_SUCCESS;
}

UA_TYPE_DECODEXML_FROM_BYTESTRING(UA_Variant)

/* DiagnosticInfo */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_DiagnosticInfo)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_DiagnosticInfo)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(UA_DiagnosticInfo)

/* InvalidType */
UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(UA_InvalidType)
UA_TYPE_METHOD_ENCODEXML_NOTIMPL(UA_InvalidType)
UA_TYPE_METHOD_DECODEXML_NOTIMPL(UA_InvalidType)
