/*
 * xml2ns0.c
 *
 *  Created on: 21.04.2014
 *      Author: mrt
 */

#include <expat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strlen
#include <ctype.h> // isspace
#include <unistd.h> // read

#include "opcua.h"
#include "ua_namespace.h"

// some typedefs for typical arguments in this module
typedef char const * const XML_Attr_t;
typedef char const * cstring_t;

// FIXME: We might want to have these classes and their methods defined in opcua.h
/** NodeSetAlias - a readable shortcut for References */
typedef struct UA_NodeSetAlias_T {
	UA_String alias;
	UA_String value;
} UA_NodeSetAlias;
UA_Int32 UA_NodeSetAlias_init(UA_NodeSetAlias* p) {
	UA_String_init(&(p->alias));
	UA_String_init(&(p->value));
	return UA_SUCCESS;
}
UA_Int32 UA_NodeSetAlias_new(UA_NodeSetAlias** p) {
	UA_alloc((void** )p, sizeof(UA_NodeSetAlias));
	UA_NodeSetAlias_init(*p);
	return UA_SUCCESS;
}
/* The current set of aliases */
typedef struct UA_NodeSetAliases_T {
	UA_Int32 size;
	UA_NodeSetAlias** aliases;
} UA_NodeSetAliases;
UA_Int32 UA_NodeSetAliases_init(UA_NodeSetAliases* p) {
	p->size = -1;
	p->aliases = UA_NULL;
	return UA_SUCCESS;
}
UA_Int32 UA_NodeSetAliases_new(UA_NodeSetAliases** p) {
	UA_alloc((void** )p, sizeof(UA_NodeSetAliases));
	UA_NodeSetAliases_init(*p);
	return UA_SUCCESS;
}
UA_Int32 UA_NodeSetAliases_println(cstring_t label, UA_NodeSetAliases *p) {
	UA_Int32 i;
	for (i = 0; i < p->size; i++) {
		UA_NodeSetAlias* a = p->aliases[i];
		printf("Alias addr=%p, ", (void*) a);
		if (a) {
			printf("alias='%.*s', value='%.*s'", a->alias.length, a->alias.data, a->value.length, a->value.data);
		}
		printf("\n");
	}
	return UA_SUCCESS;
}

/* A nodeset consist of a namespace and a list of aliases */
typedef struct T_UA_NodeSet {
	namespace* ns;
	UA_NodeSetAliases aliases;
} UA_NodeSet;
UA_Int32 UA_NodeSet_init(UA_NodeSet* p) {
	create_ns(&(p->ns), 100);
	p->aliases.size = -1;
	p->aliases.aliases = UA_NULL;
	return UA_SUCCESS;
}
UA_Int32 UA_NodeSet_new(UA_NodeSet** p) {
	UA_alloc((void** )p, sizeof(UA_NodeSet));
	UA_NodeSet_init(*p);
	return UA_SUCCESS;
}
UA_Int32 UA_NodeId_copycstring(cstring_t src, UA_NodeId* dst, UA_NodeSetAliases* aliases) {
	dst->encodingByte = UA_NODEIDTYPE_FOURBYTE;
	dst->namespace = 0;
	dst->identifier.numeric = 0;
	// FIXME: assumes i=nnnn, does not care for aliases as of now
	if (src[1] == '=') {
		dst->identifier.numeric = atoi(&src[2]);
	} else {
		UA_Int32 i;
		for (i = 0; i < aliases->size && dst->identifier.numeric == 0; ++i) {
			if (0 == strncmp((char const*)src, (char const*)aliases->aliases[i]->alias.data, aliases->aliases[i]->alias.length)) {
				dst->identifier.numeric = atoi((char const*) &(aliases->aliases[i]->value.data[2]));
			}
		}
	}
	DBG_VERBOSE(printf("UA_NodeId_copycstring src=%s,id=%d\n", src, dst->identifier.numeric));
	return UA_SUCCESS;
}

/* the stack and it's elements */
#define XML_STACK_MAX_DEPTH 10
#define XML_STACK_MAX_CHILDREN 40
struct XML_Stack;
typedef UA_Int32 (*XML_decoder) (struct XML_Stack* s, XML_Attr_t* attr, void* dst, _Bool isStart);
typedef struct XML_child {
	cstring_t name;
	UA_Int32 type;
	XML_decoder elementHandler;
	void* obj;
} XML_child_t;

typedef struct XML_Parent {
	cstring_t name;
	int textAttribIdx; // -1 - not set
	cstring_t textAttrib;
	int activeChild; // -1 - no active child
	int len; // -1 - empty set
	XML_child_t children[XML_STACK_MAX_CHILDREN];
} XML_Parent_t;

typedef struct XML_Stack {
	int depth;
	XML_Parent_t parent[XML_STACK_MAX_DEPTH];
	UA_NodeSetAliases* aliases; // shall point to the aliases of the NodeSet after reading
} XML_Stack_t;

void XML_Stack_init(XML_Stack_t* p, cstring_t name) {
	unsigned int i, j;
	p->depth = 0;
	for (i = 0; i < XML_STACK_MAX_DEPTH; i++) {
		p->parent[i].name = UA_NULL;
		p->parent[i].len = 0;
		p->parent[i].activeChild = -1;
		p->parent[i].textAttrib = UA_NULL;
		p->parent[i].textAttribIdx = -1;
		for (j = 0; j < XML_STACK_MAX_CHILDREN; j++) {
			p->parent[i].children[j].name = UA_NULL;
			p->parent[i].children[j].elementHandler = UA_NULL;
			p->parent[i].children[j].type = UA_INVALIDTYPE;
			p->parent[i].children[j].obj = UA_NULL;
		}
	}
	p->parent[0].name = name;
}

void XML_Stack_print(XML_Stack_t* s) {
	UA_Int32 i;
	for (i = 0; i <= s->depth; i++) {
		printf("%s.", s->parent[i].name);
	}
}

// FIXME: we might want to calculate textAttribIdx
void XML_Stack_handleTextAs(XML_Stack_t* p, cstring_t textAttrib, unsigned int textAttribIdx) {
	p->parent[p->depth].textAttrib = textAttrib;
	p->parent[p->depth].textAttribIdx = textAttribIdx;
}

void XML_Stack_addChildHandler(XML_Stack_t* p, cstring_t name, XML_decoder handler, UA_Int32 type, void* dst) {
	unsigned int len = p->parent[p->depth].len;
	p->parent[p->depth].children[len].name = name;
	p->parent[p->depth].children[len].elementHandler = handler;
	p->parent[p->depth].children[len].type = type;
	p->parent[p->depth].children[len].obj = dst;
	p->parent[p->depth].len++;
}

UA_Int32 UA_Array_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, void* dst, _Bool isStart) {
	// FIXME: Implement
	return UA_SUCCESS;
}

UA_Int32 UA_Int16_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, UA_Int16* dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_Int32 entered with dst=%p,isStart=%d\n", (void* ) dst, isStart));
	if (isStart) {
		if (dst == UA_NULL) {
			UA_Int16_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void*) dst;
		}
		*dst = atoi(attr[1]);
	} else {
		// TODO: It is a design flaw that we need to do this here, isn't it?
		s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj =
		UA_NULL;
	}
	return UA_SUCCESS;
}

UA_Int32 UA_Int32_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, UA_Int32* dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_Int32 entered with dst=%p,isStart=%d\n", (void* ) dst, isStart));
	if (isStart) {
		if (dst == UA_NULL) {
			UA_Int32_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void*) dst;
		}
		*dst = atoi(attr[1]);
	} else {
		// TODO: It is a design flaw that we need to do this here, isn't it?
		s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj =
		UA_NULL;
	}
	return UA_SUCCESS;
}

UA_Int32 UA_String_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, UA_String* dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_String entered with dst=%p,isStart=%d\n", (void* ) dst, isStart));
	UA_UInt32 i;
	if (isStart) {
		if (dst == UA_NULL) {
			UA_String_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void*) dst;
		}
		s->parent[s->depth].len = 0;
		XML_Stack_addChildHandler(s, "Data", (XML_decoder) UA_Array_decodeXML, UA_BYTE, &(dst->data));
		XML_Stack_addChildHandler(s, "Length", (XML_decoder) UA_Int32_decodeXML, UA_INT32, &(dst->length));
		XML_Stack_handleTextAs(s, "Data", 0);

		// set attributes
		for (i = 0; attr[i]; i += 2) {
			if (0 == strncmp("Data", attr[i], strlen("Data"))) {
				UA_String_copycstring(attr[i + 1], dst);
			} else {
				perror("Unknown attribute");
			}
		}
	} else {
		// TODO: It is a design flaw that we need to do this here, isn't it?
		DBG_VERBOSE(
				printf("UA_String clears %p\n",
						(void* ) (s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj)));
		s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = UA_NULL;
	}
	return UA_SUCCESS;
}

UA_Int32 UA_LocalizedText_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, UA_LocalizedText* dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_LocalizedText entered with dst=%p,isStart=%d\n", (void* ) dst, isStart));
	UA_UInt32 i;
	if (isStart) {
		if (dst == UA_NULL) {
			UA_LocalizedText_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void*) dst;
		}
		s->parent[s->depth].len = 0;
		XML_Stack_addChildHandler(s, "Text", (XML_decoder) UA_String_decodeXML, UA_STRING, &(dst->text));
		XML_Stack_addChildHandler(s, "Locale", (XML_decoder) UA_String_decodeXML, UA_STRING, &(dst->locale));
		XML_Stack_handleTextAs(s, "Data", 0);

		// set attributes
		for (i = 0; attr[i]; i += 2) {
			if (0 == strncmp("Text", attr[i], strlen("Text"))) {
				UA_String_copycstring(attr[i + 1], &(dst->text));
				dst->encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
			} else if (0 == strncmp("Locale", attr[i], strlen("Locale"))) {
				UA_String_copycstring(attr[i + 1], &(dst->locale));
				dst->encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE;
			} else {
				perror("Unknown attribute");
			}
		}
	} else {
		switch (s->parent[s->depth - 1].activeChild) {
		case 0:
		dst->encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
			break;
		case 1:
		dst->encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE;
			break;
		default:
			break;
		}
		// TODO: I think it is a design flaw that we need to do this here, isn't it?
		DBG_VERBOSE(
				printf("UA_LocalizedText clears %p\n",
						(void* ) (s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj)));
		s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = UA_NULL;
	}
	return UA_SUCCESS;
}

UA_Int32 UA_QualifiedName_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, UA_QualifiedName* dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_QualifiedName entered with dst=%p,isStart=%d\n", (void* ) dst, isStart));
	UA_UInt32 i;
	if (isStart) {
		if (dst == UA_NULL) {
			UA_QualifiedName_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void*) dst;
		}
		s->parent[s->depth].len = 0;
		XML_Stack_addChildHandler(s, "Name", (XML_decoder) UA_String_decodeXML, UA_STRING, &(dst->name));
		XML_Stack_addChildHandler(s, "NamespaceIndex", (XML_decoder) UA_Int16_decodeXML, UA_STRING,
				&(dst->namespaceIndex));
		XML_Stack_handleTextAs(s, "Data", 0);

		// set attributes
		for (i = 0; attr[i]; i += 2) {
			if (0 == strncmp("NamespaceIndex", attr[i], strlen("NamespaceIndex"))) {
				dst->namespaceIndex = atoi(attr[i + 1]);
			} else if (0 == strncmp("Name", attr[i], strlen("Name"))) {
				UA_String_copycstring(attr[i + 1], &(dst->name));
			} else {
				perror("Unknown attribute");
			}
		}
	} else {
		// TODO: I think it is a design flaw that we need to do this here, isn't it?
		DBG_VERBOSE(
				printf("UA_LocalizedText clears %p\n",
						(void* ) (s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj)));
		s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = UA_NULL;
	}
	return UA_SUCCESS;
}

UA_Int32 UA_DataTypeNode_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, UA_DataTypeNode* dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_DataTypeNode entered with dst=%p,isStart=%d\n", (void* ) dst, isStart));
	UA_UInt32 i;

	if (isStart) {
		// create a new object if called with UA_NULL
		if (dst == UA_NULL) {
			UA_DataTypeNode_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void*) dst;
		}

		s->parent[s->depth].len = 0;
		XML_Stack_addChildHandler(s, "DisplayName", (XML_decoder) UA_LocalizedText_decodeXML, UA_LOCALIZEDTEXT,
				&(dst->displayName));
		XML_Stack_addChildHandler(s, "Description", (XML_decoder) UA_LocalizedText_decodeXML, UA_LOCALIZEDTEXT,
				&(dst->description));
		XML_Stack_addChildHandler(s, "BrowseName", (XML_decoder) UA_QualifiedName_decodeXML, UA_QUALIFIEDNAME,
				&(dst->description));

		// set missing default attributes
		dst->nodeClass = UA_NODECLASS_DATATYPE;

		// set attributes
		for (i = 0; attr[i]; i += 2) {
			if (0 == strncmp("NodeId", attr[i], strlen("NodeId"))) {
				UA_NodeId_copycstring(attr[i + 1], &(dst->nodeId), s->aliases);
			} else if (0 == strncmp("BrowseName", attr[i], strlen("BrowseName"))) {
				UA_String_copycstring(attr[i + 1], &(dst->browseName.name));
				dst->browseName.namespaceIndex = 0;
			} else if (0 == strncmp("DisplayName", attr[i], strlen("DisplayName"))) {
				UA_String_copycstring(attr[i + 1], &(dst->displayName.text));
				dst->displayName.encodingMask = UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
			} else if (0 == strncmp("Description", attr[i], strlen("Description"))) {
				UA_String_copycstring(attr[i + 1], &(dst->description.text));
				dst->description.encodingMask = UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
			} else {
				DBG_ERR(XML_Stack_print(s));DBG_ERR(printf("%s - unknown attribute\n", attr[i]));
			}
		}
	} else {
		// TODO: It is a design flaw that we need to do this here, isn't it?
		DBG_VERBOSE(
				printf("UA_DataTypeNode clears %p\n",
						(void* ) (s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj)));
		s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj =
		UA_NULL;
	}
	return UA_SUCCESS;
}

UA_Int32 UA_VariableNode_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, UA_VariableNode* dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_VariableNode entered with dst=%p,isStart=%d\n", (void* ) dst, isStart));
	UA_UInt32 i;

	if (isStart) {
		// create a new object if called with UA_NULL
		if (dst == UA_NULL) {
			UA_VariableNode_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void*) dst;
		}

		s->parent[s->depth].len = 0;
		XML_Stack_addChildHandler(s, "DisplayName", (XML_decoder) UA_LocalizedText_decodeXML, UA_LOCALIZEDTEXT,
				&(dst->displayName));
		XML_Stack_addChildHandler(s, "Description", (XML_decoder) UA_LocalizedText_decodeXML, UA_LOCALIZEDTEXT,
				&(dst->description));
		// XML_Stack_addChildHandler(s, "DataType", (XML_decoder) UA_NodeId_decodeXML, UA_NODEID, &(dst->dataType));
		XML_Stack_addChildHandler(s, "ValueRank", (XML_decoder) UA_Int32_decodeXML, UA_INT32, &(dst->valueRank));

		// set missing default attributes
		dst->nodeClass = UA_NODECLASS_VARIABLE;

		// set attributes
		for (i = 0; attr[i]; i += 2) {
			if (0 == strncmp("NodeId", attr[i], strlen("NodeId"))) {
				UA_NodeId_copycstring(attr[i + 1], &(dst->nodeId), s->aliases);
			} else if (0 == strncmp("DataType", attr[i], strlen("DataType"))) {
				UA_NodeId_copycstring(attr[i + 1], &(dst->dataType), s->aliases);
			} else if (0 == strncmp("ValueRank", attr[i], strlen("ValueRank"))) {
				dst->valueRank = atoi(attr[i + 1]);
			} else if (0 == strncmp("ParentNodeId", attr[i], strlen("ParentNodeId"))) {
				// FIXME: I do not know what to do with this parameter
			} else if (0 == strncmp("BrowseName", attr[i], strlen("BrowseName"))) {
				UA_String_copycstring(attr[i + 1], &(dst->browseName.name));
				dst->browseName.namespaceIndex = 0;
			} else if (0 == strncmp("DisplayName", attr[i], strlen("DisplayName"))) {
				UA_String_copycstring(attr[i + 1], &(dst->displayName.text));
				dst->displayName.encodingMask = UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
			} else if (0 == strncmp("Description", attr[i], strlen("Description"))) {
				UA_String_copycstring(attr[i + 1], &(dst->description.text));
				dst->description.encodingMask = UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
			} else {
				DBG_ERR(XML_Stack_print(s));DBG_ERR(printf("%s - unknown attribute\n", attr[i]));
			}
		}
	} else {
		// TODO: It is a design flaw that we need to do this here, isn't it?
		DBG_VERBOSE(
				printf("UA_DataTypeNode clears %p\n",
						(void* ) (s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj)));
		s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj =
		UA_NULL;
	}
	return UA_SUCCESS;
}

void print_node(UA_Node const * node) {
	if (node != UA_NULL) {
		UA_NodeId_printf("--- node.nodeId=", &(node->nodeId));
		printf("\t.browseName='%.*s'\n", node->browseName.name.length, node->browseName.name.data);
		printf("\t.displayName='%.*s'\n", node->displayName.text.length, node->displayName.text.data);
		printf("\t.description='%.*s%s'\n", node->description.text.length > 40 ? 40 : node->description.text.length,
				node->description.text.data, node->description.text.length > 40 ? "..." : "");
		printf("\t.nodeClass=%d\n", node->nodeClass);
		printf("\t.writeMask=%d\n", node->writeMask);
		printf("\t.userWriteMask=%d\n", node->userWriteMask);
		printf("\t.referencesSize=%d\n", node->referencesSize);
		switch (node->nodeClass) {
		case UA_NODECLASS_VARIABLE: {
			UA_VariableNode const * p = (UA_VariableNode const *) node;
			printf("\t----- UA_VariableNode ----- \n");
			UA_NodeId_printf("\t.dataType=", &(p->dataType));
			printf("\t.valueRank=%d\n", p->valueRank);
			printf("\t.accessLevel=%d\n", p->accessLevel);
			printf("\t.userAccessLevel=%d\n", p->userAccessLevel);
			printf("\t.arrayDimensionsSize=%d\n", p->arrayDimensionsSize);
			printf("\t.minimumSamplingInterval=%f\n", p->minimumSamplingInterval);
			printf("\t.historizing=%d\n", p->historizing);
		}
			break;
		// case UA_NODECLASS_DATATYPE:
		default:
			break;
		}
	}
}

UA_Int32 UA_NodeSetAlias_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, UA_NodeSetAlias* dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_NodeSetAlias entered with dst=%p,isStart=%d\n", (void* ) dst, isStart));
	if (isStart) {
		// create if necessary
		if (dst == UA_NULL) {
			UA_NodeSetAlias_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void*) dst;
		}
		// set handlers
		s->parent[s->depth].len = 0;
		XML_Stack_addChildHandler(s, "Alias", (XML_decoder) UA_String_decodeXML, UA_STRING, &(dst->alias));
		XML_Stack_addChildHandler(s, "Value", (XML_decoder) UA_String_decodeXML, UA_STRING, &(dst->value));
		XML_Stack_handleTextAs(s, "Data", 1);

		// set attributes
		UA_Int32 i;
		for (i = 0; attr[i]; i += 2) {
			if (0 == strncmp("Alias", attr[i], strlen("Alias"))) {
				UA_String_copycstring(attr[i + 1], &(dst->alias));
			} else if (0 == strncmp("Value", attr[i], strlen("Value"))) {
				UA_String_copycstring(attr[i + 1], &(dst->value));
			} else {
				DBG_ERR(XML_Stack_print(s));DBG_ERR(printf("%s - unknown attribute\n", attr[i]));
			}
		}
	} else {
		// sub element is ready
		// TODO: It is a design flaw that we need to do this here, isn't it?
		DBG_VERBOSE(
				printf("UA_NodeSetAlias clears %p\n",
						(void* ) (s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj)));
		s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = UA_NULL;
	}
	return UA_SUCCESS;
}

UA_Int32 UA_NodeSetAliases_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, UA_NodeSetAliases* dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_NodeSetALiases entered with dst=%p,isStart=%d\n", (void* ) dst, isStart));
	if (isStart) {
		if (dst == UA_NULL) {
			UA_NodeSetAliases_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void*) dst;
		}
		s->parent[s->depth].len = 0;
		XML_Stack_addChildHandler(s, "Alias", (XML_decoder) UA_NodeSetAlias_decodeXML, UA_INVALIDTYPE, UA_NULL);
	} else {
		// sub element is ready, add to array
		if (dst->size < 0 || dst->size == 0) {
			dst->size = 1;
			UA_alloc((void** )&(dst->aliases), dst->size * sizeof(UA_NodeSetAlias*));
			DBG_VERBOSE(
					printf("allocate aliases:dst=%p, aliases=%p, size=%d\n", (void* )dst, (void* )(dst->aliases),
							dst->size));
		} else {
			dst->size++;
			dst->aliases = realloc(dst->aliases, dst->size * sizeof(UA_NodeSetAlias*));
			DBG_VERBOSE(
					printf("reallocate aliases:dst=%p, aliases=%p, size=%d\n", (void* )dst, (void* )(dst->aliases),
							dst->size));
		}
		// index starts with 0, therefore size-1
		DBG_VERBOSE(printf("assign alias:dst=%p, src=%p\n", (void* )dst->aliases[dst->size - 1], (void* )attr));
		dst->aliases[dst->size - 1] = (UA_NodeSetAlias*) attr;
		// TODO: It is a design flaw that we need to do this here, isn't it?
		DBG_VERBOSE(
				printf("UA_NodeSetAliases clears %p\n",
						(void* ) (s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj)));
		s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj =
		UA_NULL;
	}
	return UA_SUCCESS;
}

UA_Int32 UA_NodeSet_decodeXML(XML_Stack_t* s, XML_Attr_t* attr, UA_NodeSet* dst, _Bool isStart) {
	DBG_VERBOSE(printf("UA_NodeSet entered with dst=%p,isStart=%d\n", (void* ) dst, isStart));
	if (isStart) {
		if (dst == UA_NULL) {
			UA_NodeSet_new(&dst);
			s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = (void*) dst;
		}
		s->parent[s->depth].len = 0;
		XML_Stack_addChildHandler(s, "Aliases", (XML_decoder) UA_NodeSetAliases_decodeXML, UA_INVALIDTYPE,
				&(dst->aliases));
		XML_Stack_addChildHandler(s, "UADataType", (XML_decoder) UA_DataTypeNode_decodeXML, UA_DATATYPENODE, UA_NULL);
		XML_Stack_addChildHandler(s, "UAVariable", (XML_decoder) UA_VariableNode_decodeXML, UA_VARIABLENODE, UA_NULL);
	} else {
		switch (s->parent[s->depth - 1].activeChild) {
		case 0: // Aliases
		if (attr != UA_NULL) {
			UA_NodeSetAliases* aliases = (UA_NodeSetAliases*) attr;
			DBG_VERBOSE(
					printf("finished aliases: aliases=%p, size=%d\n",(void*)aliases,(aliases==UA_NULL)?-1:aliases->size));
			s->aliases = aliases;
		}
			break;
		case 1:
		case 2:
		if (attr != UA_NULL) {
			UA_Node* node = (UA_Node*) attr;
			DBG_VERBOSE(printf("finished node: node=%p\n", (void* )node));
			insert_node(dst->ns, node);
			DBG_VERBOSE(printf("Inserting "));DBG_VERBOSE(print_node(node));
		}
			break;
		default:
			break;
		}
		// TODO: It is a design flaw that we need to do this here, isn't it?
		DBG_VERBOSE(
				printf("UA_NodeSet clears %p\n",
						(void* ) (s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj)));
		s->parent[s->depth - 1].children[s->parent[s->depth - 1].activeChild].obj = UA_NULL;
	}
	return UA_SUCCESS;
}

/** lookup if element is a known child of parent, if yes go for it otherwise ignore */
void XML_Stack_startElement(void * data, const char *el, const char **attr) {
	XML_Stack_t* s = (XML_Stack_t*) data;
	int i;

// scan expected children
	XML_Parent_t* cp = &s->parent[s->depth];
	for (i = 0; i < cp->len; i++) {
		if (0 == strncmp(cp->children[i].name, el, strlen(cp->children[i].name))) {
			DBG_VERBOSE(XML_Stack_print(s));
			DBG_VERBOSE(printf("%s - processing\n", el));

			cp->activeChild = i;

			s->depth++;
			s->parent[s->depth].name = el;
			s->parent[s->depth].len = 0;
			s->parent[s->depth].textAttribIdx = -1;
			s->parent[s->depth].activeChild = -1;

			// finally call the elementHandler and return
			cp->children[i].elementHandler(data, attr, cp->children[i].obj,
			TRUE);
			return;
		}
	}
// if we come here we rejected the processing of el
	DBG_VERBOSE(XML_Stack_print(s));
	DBG_VERBOSE(printf("%s - rejected\n", el));
	s->depth++;
	s->parent[s->depth].name = el;
// this should be sufficient to reject the children as well
	s->parent[s->depth].len = 0;
}

UA_Int32 XML_isSpace(cstring_t s, int len) {
	int i;
	for (i = 0; i < len; i++) {
		if (!isspace(s[i])) {
			return UA_FALSE;
		}
	}
	return UA_TRUE;
}

/* simulates startElement, endElement behaviour */
void XML_Stack_handleText(void * data, const char *txt, int len) {
	XML_Stack_t* s = (XML_Stack_t*) data;

	if (len > 0 && !XML_isSpace(txt, len)) {
		XML_Parent_t* cp = &(s->parent[s->depth]);
		if (cp->textAttribIdx >= 0) {
			cp->activeChild = cp->textAttribIdx;
			char* buf; // need to copy txt to add 0 as string terminator
			UA_alloc((void** )&buf, len + 1);
			strncpy(buf, txt, len);
			buf[len] = 0;
			XML_Attr_t attr[3] = { cp->textAttrib, buf, UA_NULL };
			DBG_VERBOSE(
					printf("handleText calls start elementHandler %s with dst=%p, attr=%p\n",
							cp->children[cp->activeChild].name, cp->children[cp->activeChild].obj, (void* ) attr));
			cp->children[cp->activeChild].elementHandler(s, attr, cp->children[cp->activeChild].obj, TRUE);
			// FIXME: The indices of this call are simply wrong, so no finishing as of yet
			// DBG_VERBOSE(printf("handleText calls finish elementHandler %s with dst=%p, attr=(nil)\n", cp->children[cp->activeChild].name, cp->children[cp->activeChild].obj));
			// cp->children[cp->activeChild].elementHandler(s, UA_NULL, cp->children[cp->activeChild].obj, FALSE);
			UA_free(buf);
		} else {
			DBG_VERBOSE(XML_Stack_print(s));
			DBG_VERBOSE(printf("textData - ignore text data '%.*s'\n", len, txt));
		}
	}
}

/** if we are an activeChild of a parent we call the child-handler */
void XML_Stack_endElement(void *data, const char *el) {
	XML_Stack_t* s = (XML_Stack_t*) data;

// the parent of the parent of the element knows the elementHandler, therefore depth-2!
	if (s->depth > 1) {
		// inform parents elementHandler that everything is done
		XML_Parent_t* cp = &(s->parent[s->depth - 1]);
		XML_Parent_t* cpp = &(s->parent[s->depth - 2]);
		if (cpp->activeChild >= 0 && cp->activeChild >= 0) {
			DBG_VERBOSE(XML_Stack_print(s));
			DBG_VERBOSE(
					printf(" - inform pop %s, arg=%p\n", cpp->children[cpp->activeChild].name,
							(void* ) cp->children[cp->activeChild].obj));
			cpp->children[cpp->activeChild].elementHandler(s, (XML_Attr_t*) cp->children[cp->activeChild].obj,
					cpp->children[cpp->activeChild].obj, FALSE);
		}
		// reset
		cp->activeChild = -1;
	}
	s->depth--;
}

int main() {
	char buf[1024];
	int len; /* len is the number of bytes in the current bufferful of data */
	XML_Stack_t s;
	XML_Stack_init(&s, "ROOT");
	UA_NodeSet n;
	UA_NodeSet_init(&n);
	XML_Stack_addChildHandler(&s, "UANodeSet", (XML_decoder) UA_NodeSet_decodeXML, UA_INVALIDTYPE, &n);

	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetUserData(parser, &s);
	XML_SetElementHandler(parser, XML_Stack_startElement, XML_Stack_endElement);
	XML_SetCharacterDataHandler(parser, XML_Stack_handleText);
	while ((len = read(0, buf, 1024)) > 0) {
		if (!XML_Parse(parser, buf, len, (len < 1024))) {
			return 1;
		}
	}
	XML_ParserFree(parser);
	iterate_ns(n.ns, print_node);
	printf("aliases addr=%p, size=%d\n", (void*) &(n.aliases), n.aliases.size);
	UA_NodeSetAliases_println("aliases in nodeset: ", &n.aliases);
	return 0;
}
