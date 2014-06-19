#ifndef UA_TYPES_ENCODING_XML_H_
#define UA_TYPES_ENCODING_XML_H_

#include "ua_xml.h"
#include "ua_types.h"

#define UA_TYPE_XML_ENCODING(TYPE)							\
	UA_Int32 TYPE##_calcSizeXml(const void * p); \
    UA_Int32 TYPE##_encodeXml(const TYPE *src, UA_ByteString *dst, UA_UInt32 *offset); \
	UA_Int32 TYPE##_decodeXml(UA_ByteString *src, UA_UInt32 *offset, TYPE *dst); \
	UA_Int32 TYPE##_encodeXmlToStack(const TYPE *src, XML_Stack *s, XML_Attr *attr); \
	UA_Int32 TYPE##_decodeXmlFromStack(XML_Stack* s, XML_Attr* attr, TYPE* dst, UA_Boolean isStart);

#define UA_TYPE_METHOD_CALCSIZEXML_NOTIMPL(TYPE) \
	UA_Int32 TYPE##_calcSizeXml(const void * p) { \
	return -1; \
 }

#define UA_TYPE_METHOD_ENCODEXML_NOTIMPL(TYPE) \
    UA_Int32 TYPE##_encodeXml(const TYPE *src, UA_ByteString *dst, UA_UInt32 *offset) { \
        return UA_ERR_NOT_IMPLEMENTED; \
	}																	\
	UA_Int32 TYPE##_encodeXmlToStack(const TYPE *src, XML_Stack *s, XML_Attr *attr) { \
																					 DBG_VERBOSE(printf(#TYPE "_encodeXML entered with src=%p\n", (void* ) src)); \
     return UA_ERR_NOT_IMPLEMENTED;\
 }

#define UA_TYPE_METHOD_DECODEXML_NOTIMPL(TYPE) \
	UA_Int32 TYPE##_decodeXml(UA_ByteString *src, UA_UInt32 *offset, TYPE *dst) { \
        return UA_ERR_NOT_IMPLEMENTED;									\
	}																	\
																		\
 UA_Int32 TYPE##_decodeXmlFromStack(XML_Stack* s, XML_Attr* attr, TYPE* dst, UA_Boolean isStart) { \
																								  DBG_VERBOSE(printf(#TYPE "_decodeXML entered with dst=%p,isStart=%d\n", (void* ) dst, (_Bool) isStart)); \
     return UA_ERR_NOT_IMPLEMENTED; \
 }

#define UA_TYPE_DECODEXML_FROM_BYTESTRING(TYPE) \
	UA_Int32 TYPE##_decodeXml(UA_ByteString *src, UA_UInt32 *offset, TYPE *dst) { \
	/* // Init Stack here \
	UA_Stack *stack; \
	UA_Attr *attr; \
	TYPE##decodeXmlFromStack(stack, attr, dst, UA_TRUE); \
	*/ \
	return UA_ERR_NOT_IMPLEMENTED; \
} 

UA_TYPE_XML_ENCODING(UA_Boolean)
UA_TYPE_XML_ENCODING(UA_SByte)
UA_TYPE_XML_ENCODING(UA_Byte)
UA_TYPE_XML_ENCODING(UA_Int16)
UA_TYPE_XML_ENCODING(UA_UInt16)
UA_TYPE_XML_ENCODING(UA_Int32)
UA_TYPE_XML_ENCODING(UA_UInt32)
UA_TYPE_XML_ENCODING(UA_Int64)
UA_TYPE_XML_ENCODING(UA_UInt64)
UA_TYPE_XML_ENCODING(UA_Float)
UA_TYPE_XML_ENCODING(UA_Double)
UA_TYPE_XML_ENCODING(UA_String)
UA_TYPE_XML_ENCODING(UA_DateTime)
UA_TYPE_XML_ENCODING(UA_Guid)
UA_TYPE_XML_ENCODING(UA_ByteString)
UA_TYPE_XML_ENCODING(UA_XmlElement)
UA_TYPE_XML_ENCODING(UA_NodeId)
UA_TYPE_XML_ENCODING(UA_ExpandedNodeId)
UA_TYPE_XML_ENCODING(UA_StatusCode)
UA_TYPE_XML_ENCODING(UA_QualifiedName)
UA_TYPE_XML_ENCODING(UA_LocalizedText)
UA_TYPE_XML_ENCODING(UA_ExtensionObject)
UA_TYPE_XML_ENCODING(UA_DataValue)
UA_TYPE_XML_ENCODING(UA_Variant)
UA_TYPE_XML_ENCODING(UA_DiagnosticInfo)

/* Not built-in types */
UA_TYPE_XML_ENCODING(UA_InvalidType)
UA_TYPE_XML_ENCODING(UA_ReferenceDescription)
UA_TYPE_XML_ENCODING(UA_NodeClass)

#endif /* UA_TYPES_ENCODING_XML_H_ */
