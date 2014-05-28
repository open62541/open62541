/*
 * xml2ns0.c
 *
 *  Created on: 21.04.2014
 *      Author: mrt
 */

#include "ua_xml.h"
#include <fcntl.h>
#include <ctype.h>

/** @brief some macros to lowercase the first character without copying around */
#define F_cls "%c%.*s"
#define LC_cls(str) tolower((str).data[0]), (str).length-1, &((str).data[1])

/** @brief check if node is root node by scanning it's references for hasProperty
 */
_Bool UA_Node_isRootNode(const UA_Node* node) {
	UA_Int32 i;
	for (i = 0; i < node->referencesSize; i++ ) {
		UA_Int32 refId = UA_NodeId_getIdentifier(&(node->references[i]->referenceTypeId));
		UA_Int32 isInverse = node->references[i]->isInverse;
		if (isInverse && (refId == 47 || refId == 46)) // HasComponent, HasProperty
			return UA_FALSE;
	}
	return UA_TRUE;
}

/** @brief declares all the top level objects in the server's application memory
 * FIXME: shall add only top level objects, i.e. those that have no parents
 */
void sam_declareAttribute(UA_Node const * node) {
	if ((node->nodeClass == UA_NODECLASS_VARIABLE || node->nodeClass == UA_NODECLASS_OBJECT) && UA_Node_isRootNode(node)) {
		UA_VariableNode* vn = (UA_VariableNode*) node;
		printf("\t%s " F_cls "; // i=%d\n", UA_[UA_ns0ToVTableIndex(vn->dataType.identifier.numeric)].name, LC_cls(node->browseName.name), node->nodeId.identifier.numeric);
	}
}

/** @brief declares all the buffers for string variables
 * FIXME: traverse down to top level objects and create a unique name such as cstr_serverState_buildInfo_version
 */
void sam_declareBuffer(UA_Node const * node) {
	if (node != UA_NULL && node->nodeClass == UA_NODECLASS_VARIABLE) {
		UA_VariableNode* vn = (UA_VariableNode*) node;
		switch (vn->dataType.identifier.numeric) {
		case UA_BYTESTRING_NS0:
		case UA_STRING_NS0:
		case UA_LOCALIZEDTEXT_NS0:
		case UA_QUALIFIEDNAME_NS0:
		printf("UA_Byte cstr_" F_cls "[] = \"\"\n",LC_cls(vn->browseName.name));
		break;
		default:
		break;
		}
	}
}

/** @brief assigns the c-strings to the ua type strings.
 * FIXME: traverse down to top level objects and create a unique name such as cstr_serverState_buildInfo_version
 */
void sam_assignBuffer(UA_Node const * node) {
	if (node != UA_NULL && node->nodeClass == UA_NODECLASS_VARIABLE) {
		UA_VariableNode* vn = (UA_VariableNode*) node;
		switch (vn->dataType.identifier.numeric) {
		case UA_BYTESTRING_NS0:
		case UA_STRING_NS0:
		printf("\tSAM_ASSIGN_CSTRING(cstr_" F_cls ",sam." F_cls ");\n",LC_cls(vn->browseName.name),LC_cls(vn->browseName.name));
		break;
		case UA_LOCALIZEDTEXT_NS0:
		printf("\tSAM_ASSIGN_CSTRING(cstr_" F_cls ",sam." F_cls ".text);\n",LC_cls(vn->browseName.name),LC_cls(vn->browseName.name));
		break;
		case UA_QUALIFIEDNAME_NS0:
		printf("\tSAM_ASSIGN_CSTRING(cstr_" F_cls ",sam." F_cls ".name);\n",LC_cls(vn->browseName.name),LC_cls(vn->browseName.name));
		break;
		default:
		break;
		}
	}
}

void sam_attachToNamespace(UA_Node const * node) {
	if (node != UA_NULL && node->nodeClass == UA_NODECLASS_VARIABLE) {
		UA_VariableNode* vn = (UA_VariableNode*) node;
		printf("\tsam_attach(ns,%d,%s,&sam." F_cls ");\n",node->nodeId.identifier.numeric,UA_[UA_ns0ToVTableIndex(vn->dataType.identifier.numeric)].name, LC_cls(vn->browseName.name));
	}
}

typedef struct pattern {
		char* s;
		Namespace_nodeVisitor v;
} pattern;

pattern p[] = {
{ "/** server application memory - generated but manually adapted */\n",UA_NULL },
{ "#define SAM_ASSIGN_CSTRING(src,dst) do { dst.length = strlen(src)-1; dst.data = (UA_Byte*) src; } while(0)\n",UA_NULL },
{ "struct sam {\n", UA_NULL },
{ UA_NULL, sam_declareAttribute },
{ "} sam;\n", UA_NULL },
{ UA_NULL, sam_declareBuffer },
{ "void sam_init(Namespace* ns) {", UA_NULL },
{ UA_NULL, sam_assignBuffer },
{ UA_NULL, sam_attachToNamespace },
{ "}\n", UA_NULL },
{UA_NULL, UA_NULL}
};


UA_Int32 Namespace_getNumberOfComponents(Namespace const * ns, UA_NodeId const * id, UA_Int32* number) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Node const * node;
	if ((retval = Namespace_get(ns,id,&node,UA_NULL)) != UA_SUCCESS)
		return retval;
	if (node == UA_NULL)
		return UA_ERR_INVALID_VALUE;
	UA_Int32 i, n;
	for (i = 0, n = 0; i < node->referencesSize; i++ ) {
		if (node->references[i]->referenceTypeId.identifier.numeric == 47 && node->references[i]->isInverse != UA_TRUE) {
			n++;
		}
	}
	*number = n;
	return retval;
}

UA_Int32 Namespace_getComponent(Namespace const * ns, UA_NodeId const * id, UA_Int32 idx, UA_NodeId** result) {
	UA_Int32 retval = UA_SUCCESS;

	UA_Node const * node;
	if ((retval = Namespace_get(ns,id,&node,UA_NULL)) != UA_SUCCESS)
		return retval;

	UA_Int32 i, n;
	for (i = 0, n = 0; i < node->referencesSize; i++ ) {
		if (node->references[i]->referenceTypeId.identifier.numeric == 47 && node->references[i]->isInverse != UA_TRUE) {
			n++;
			if (n == idx) {
				*result = &(node->references[i]->targetId.nodeId);
				return retval;
			}
		}
	}
	return UA_ERR_INVALID_VALUE;
}


UA_Int32 UAX_NodeId_encodeBinaryByMetaData(Namespace const * ns, UA_NodeId const * id, UA_Int32* pos, UA_ByteString *dst) {
	UA_Int32 i, retval = UA_SUCCESS;
	if (UA_NodeId_isBasicType(id)) {
		UA_Node const * result;
		Namespace_Entry_Lock* lock;
		if ((retval = Namespace_get(ns,id,&result,&lock)) == UA_SUCCESS)
			UA_Variant_encodeBinary(&((UA_VariableNode *) result)->value,pos,dst);
	} else {
		UA_Int32 nComp = 0;
		if ((retval = Namespace_getNumberOfComponents(ns,id,&nComp)) == UA_SUCCESS) {
			for (i=0; i < nComp; i++) {
				UA_NodeId* comp = UA_NULL;
				Namespace_getComponent(ns,id,i,&comp);
				UAX_NodeId_encodeBinaryByMetaData(ns,comp, pos, dst);
			}
		}
	}
	return retval;
}

UA_Int32 UAX_NodeId_encodeBinary(Namespace const * ns, UA_NodeId const * id, UA_Int32* pos, UA_ByteString *dst) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Node const * node;
	Namespace_Entry_Lock* lock;

	if ((retval = Namespace_get(ns,id,&node,&lock)) == UA_SUCCESS) {
		if (node->nodeClass == UA_NODECLASS_VARIABLE) {
			retval = UA_Variant_encodeBinary(&((UA_VariableNode*) node)->value,pos,dst);
		}
		Namespace_Entry_Lock_release(lock);
	}
	return retval;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("usage: %s filename\n",argv[0]);
	} else {
		int f = open(argv[1], O_RDONLY);
		if (f==-1) {
			perror("file not found");
			exit(-1);
		}

		char buf[1024];
		int len; /* len is the number of bytes in the current bufferful of data */
		XML_Stack s;
		XML_Stack_init(&s, "ROOT");
		UA_NodeSet n;
		UA_NodeSet_init(&n, 0);
		XML_Stack_addChildHandler(&s, "UANodeSet", strlen("UANodeSet"), (XML_decoder) UA_NodeSet_decodeXML, UA_INVALIDTYPE, &n);

		XML_Parser parser = XML_ParserCreate(NULL);
		XML_SetUserData(parser, &s);
		XML_SetElementHandler(parser, XML_Stack_startElement, XML_Stack_endElement);
		XML_SetCharacterDataHandler(parser, XML_Stack_handleText);
		while ((len = read(f, buf, 1024)) > 0) {
			if (!XML_Parse(parser, buf, len, (len < 1024))) {
				return 1;
			}
		}
		XML_ParserFree(parser);
		close(f);

		for (pattern* pi = &p[0]; pi->s != UA_NULL || pi->v != UA_NULL; ++pi) {
			if (pi->v) {
				Namespace_iterate(n.ns, pi->v);
			} else {
				printf("%s\n",pi->s);
			}
		}
	}
	return 0;
}
