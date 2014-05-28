/*
 * ua_xml.h
 *
 *  Created on: 03.05.2014
 *      Author: mrt
 */

#ifndef __UA_XML_H__
#define __UA_XML_H__

#include <expat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strlen
#include <ctype.h> // isspace
#include <unistd.h> // read

#include "opcua.h"
#include "ua_namespace.h"

UA_Int32 UA_Boolean_copycstring(cstring src, UA_Boolean* dst);
UA_Int32 UA_Int16_copycstring(cstring src, UA_Int16* dst);
UA_Int32 UA_UInt16_copycstring(cstring src, UA_UInt16* dst) ;
UA_Boolean UA_NodeId_isBuiltinType(UA_NodeId* nodeid);
void print_node(UA_Node const * node);

/** @brief an object to hold a typed array */
typedef struct UA_TypedArray {
	UA_Int32 size;
	UA_VTable* vt;
	void** elements;
} UA_TypedArray;

/** @brief init typed array with size=-1 and an UA_INVALIDTYPE */
UA_Int32 UA_TypedArray_init(UA_TypedArray* p);

/** @brief allocate memory for the array header only */
UA_Int32 UA_TypedArray_new(UA_TypedArray** p);
UA_Int32 UA_TypedArray_setType(UA_TypedArray* p, UA_Int32 type);
UA_Int32 UA_TypedArray_decodeXML(XML_Stack* s, XML_Attr* attr, UA_TypedArray* dst, _Bool isStart);

UA_Int32 UA_NodeSetAlias_init(UA_NodeSetAlias* p);
UA_Int32 UA_NodeSetAlias_new(UA_NodeSetAlias** p);
UA_Int32 UA_NodeSetAlias_decodeXML(XML_Stack* s, XML_Attr* attr, UA_NodeSetAlias* dst, _Bool isStart);

UA_Int32 UA_NodeSetAliases_init(UA_NodeSetAliases* p);
UA_Int32 UA_NodeSetAliases_new(UA_NodeSetAliases** p);
UA_Int32 UA_NodeSetAliases_println(cstring label, UA_NodeSetAliases *p);
UA_Int32 UA_NodeSetAliases_decodeXML(XML_Stack* s, XML_Attr* attr, UA_NodeSetAliases* dst, _Bool isStart);

typedef struct UA_NodeSet {
	Namespace* ns;
	UA_NodeSetAliases aliases;
} UA_NodeSet;

/** @brief init typed array with size=-1 and an UA_INVALIDTYPE */
UA_Int32 UA_NodeSet_init(UA_NodeSet* p, UA_UInt32 nsid);
UA_Int32 UA_NodeSet_new(UA_NodeSet** p, UA_UInt32 nsid);
UA_Int32 UA_NodeId_copycstring(cstring src, UA_NodeId* dst, UA_NodeSetAliases* aliases);
UA_Int32 UA_NodeSet_decodeXML(XML_Stack* s, XML_Attr* attr, UA_NodeSet* dst, _Bool isStart);

UA_Int32 UA_ExpandedNodeId_copycstring(cstring src, UA_ExpandedNodeId* dst, UA_NodeSetAliases* aliases);

void XML_Stack_init(XML_Stack* p, cstring name);
void XML_Stack_print(XML_Stack* s);

/** @brief add a reference to a handler (@see XML_Stack_addChildHandler) for text data
 *
 * Assume a XML structure such as
 *     <LocalizedText>
 *          <Locale></Locale>
 *          <Text>Server</Text>
 *     </LocalizedText>
 * which might be abbreviated as
 *     <LocalizedText>Server</LocalizedText>
 *
 * We would add two (@ref XML_Stack_addChildHandler), one for Locale (index 0) and one for Text (index 1),
 * both to be handled by (@ref UA_String_decodeXML) with elements "Data" and "Length". To handle the
 * abbreviation we add
 *   	XML_Stack_handleTextAsElementOf(s,"Data",1)
 *
 * @param[in] s the stack
 * @param[in] textAttrib the name of the element of the handler at position textAttribIdx
 * @param[in] textAttribIdx the index of the handler
 */
void XML_Stack_handleTextAsElementOf(XML_Stack* p, cstring textAttrib, unsigned int textAttribIdx);

/** @brief make a handler known to the XML-stack on the current level
 *
 * The current level is given by s->depth, the maximum number of children is a predefined constant.
 * A combination of type=UA_INVALIDTYPE and dst=UA_NULL is valid for special handlers only
 *
 * @param[in] s the stack
 * @param[in] name the name of the element
 * @param[in] nameLength the length of the element name
 * @param[in] handler the decoder routine for this element
 * @param[in] type the open62541-type of the element, UA_INVALIDTYPE if not in the VTable
 * @param[out] dst the address of the object for the data, handlers will allocate object if UA_NULL
 */
void XML_Stack_addChildHandler(XML_Stack* p, cstring name, UA_Int32 nameLength, XML_decoder handler, UA_Int32 type, void* dst);

void XML_Stack_startElement(void * data, const char *el, const char **attr);
UA_Int32 XML_isSpace(cstring s, int len);
void XML_Stack_handleText(void * data, const char *txt, int len);
void XML_Stack_endElement(void *data, const char *el);

/** @brief load a namespace from an XML-File
 *
 * @param[in/out] ns the address of the namespace ptr
 * @param[in] namespaceId the numeric id of the namespace
 * @param[in] rootName the name of the root element of the hierarchy (not used?)
 * @param[in] fileName the name of an existing file, e.g. Opc.Ua.NodeSet2.xml
 */
UA_Int32 Namespace_loadFromFile(Namespace **ns,UA_UInt32 namespaceId,const char* rootName,const char* fileName);


#endif // __UA_XML_H__
