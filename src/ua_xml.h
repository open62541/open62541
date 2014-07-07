#ifndef __UA_XML_H
#define __UA_XML_H

#include <expat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strlen
#include <ctype.h>  // isspace
#include <unistd.h> // read

#include "ua_types.h"

struct XML_Stack;
typedef char const *const XML_Attr;
typedef char const *cstring;
#define XML_STACK_MAX_DEPTH 10
#define XML_STACK_MAX_CHILDREN 40
typedef UA_Int32 (*XML_decoder)(struct XML_Stack *s, XML_Attr *attr, void *dst, UA_Boolean isStart);

/** @brief A readable shortcut for NodeIds. A list of aliases is intensively used in the namespace0-xml-files */
typedef struct UA_NodeSetAlias {
	UA_String alias;
	UA_String value;
} UA_NodeSetAlias;
//UA_TYPE_PROTOTYPES(UA_NodeSetAlias)

/** @brief UA_NodeSetAliases - a list of aliases */
typedef struct UA_NodeSetAliases {
	UA_Int32 size;
	UA_NodeSetAlias **aliases;
} UA_NodeSetAliases;
//UA_TYPE_PROTOTYPES(UA_NodeSetAliases)

typedef struct XML_child {
	cstring     name;
	UA_Int32    length;
	UA_Int32    type;
	XML_decoder elementHandler;
	void       *obj;
} XML_child;

typedef struct XML_Parent {
	cstring   name;
	int       textAttribIdx;  // -1 - not set
	cstring   textAttrib;
	int       activeChild;    // -1 - no active child
	int       len;            // -1 - empty set
	XML_child children[XML_STACK_MAX_CHILDREN];
} XML_Parent;

typedef struct XML_Stack {
	int depth;
	XML_Parent parent[XML_STACK_MAX_DEPTH];
	UA_NodeSetAliases *aliases;  // shall point to the aliases of the NodeSet after reading
} XML_Stack;

UA_Int32 UA_Boolean_copycstring(cstring src, UA_Boolean *dst);
UA_Int32 UA_Int16_copycstring(cstring src, UA_Int16 *dst);
UA_Int32 UA_UInt16_copycstring(cstring src, UA_UInt16 *dst);
UA_Boolean UA_NodeId_isBuiltinType(UA_NodeId *nodeid);

/** @brief an object to hold a typed array */
typedef struct UA_TypedArray {
	UA_Int32         size;
	UA_VTable_Entry *vt;
	void *elements;
} UA_TypedArray;

/** @brief init typed array with size=-1 and an UA_INVALIDTYPE */
UA_Int32 UA_TypedArray_init(UA_TypedArray *p);

/** @brief allocate memory for the array header only */
UA_Int32 UA_TypedArray_new(UA_TypedArray **p);
UA_Int32 UA_TypedArray_setType(UA_TypedArray *p, UA_Int32 type);
//UA_Int32 UA_TypedArray_decodeXML(XML_Stack *s, XML_Attr *attr, UA_TypedArray *dst, UA_Boolean isStart);

UA_Int32 UA_NodeSetAlias_init(UA_NodeSetAlias* p);
UA_Int32 UA_NodeSetAlias_new(UA_NodeSetAlias** p);

UA_Int32 UA_NodeSetAliases_init(UA_NodeSetAliases* p);
UA_Int32 UA_NodeSetAliases_new(UA_NodeSetAliases** p);
UA_Int32 UA_NodeSetAliases_println(cstring label, UA_NodeSetAliases *p);

UA_Int32 UA_ExpandedNodeId_copycstring(cstring src, UA_ExpandedNodeId* dst, UA_NodeSetAliases* aliases);

void XML_Stack_init(XML_Stack* p, UA_UInt32 nsid, cstring name);
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
 *      XML_Stack_handleTextAsElementOf(s,"Data",1)
 *
 * @param[in] s the stack
 * @param[in] textAttrib the name of the element of the handler at position textAttribIdx
 * @param[in] textAttribIdx the index of the handler
 */
void XML_Stack_handleTextAsElementOf(XML_Stack *p, cstring textAttrib, unsigned int textAttribIdx);

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
void XML_Stack_addChildHandler(XML_Stack *p, cstring name, UA_Int32 nameLength, XML_decoder handler, UA_Int32 type,
                               void *dst);

void XML_Stack_startElement(void *data, const char *el, const char **attr);
UA_Int32 XML_isSpace(cstring s, int len);
void XML_Stack_handleText(void *data, const char *txt, int len);
void XML_Stack_endElement(void *data, const char *el);

UA_Int32 UA_Text_decodeXmlFromStack(XML_Stack *s, XML_Attr *attr, UA_Byte **dst, _Bool isStart);
UA_Int32 UA_NodeId_copycstring(cstring src, UA_NodeId *dst, UA_NodeSetAliases *aliases);
UA_Int32 UA_TypedArray_decodeXmlFromStack(XML_Stack *s, XML_Attr *attr, UA_TypedArray *dst, _Bool isStart);

#endif // __UA_XML_H__
