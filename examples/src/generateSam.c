/*
 * xml2ns0.c
 *
 *  Created on: 21.04.2014
 *      Author: mrt
 */

#include "ua_xml.h"
#include <ctype.h> // tolower

/** @brief we need a variable global to the module to make it possible for the visitors to access the namespace */
static Namespace* theNamespace;

UA_Int32 UA_Node_getParent(const UA_Node* node, const UA_Node** parent) {
	UA_Int32 i = 0;
	DBG_VERBOSE(printf("// UA_Node_getParent - node={i=%d}",UA_NodeId_getIdentifier(&(node->nodeId))));
	for (; i < node->referencesSize; i++ ) {
		UA_Int32 refId = UA_NodeId_getIdentifier(&(node->references[i]->referenceTypeId));
		UA_Int32 isInverse = node->references[i]->isInverse;
		if (isInverse && (refId == 47 || refId == 46)) {
			Namespace_Entry_Lock* lock = UA_NULL;
			UA_Int32 retval;
			retval = Namespace_get(theNamespace, &(node->references[i]->targetId.nodeId),parent,&lock);
			Namespace_Entry_Lock_release(lock);
			if (retval == UA_SUCCESS) {
				DBG_VERBOSE(printf(" has parent={i=%d}\n",UA_NodeId_getIdentifier(&((*parent)->nodeId))));
			} else {
				DBG_VERBOSE(printf(" has non-existing parent={i=%d}\n",UA_NodeId_getIdentifier(&(node->references[i]->targetId.nodeId))));
			}
			return retval;
		}
	}
	// there is no parent, we are root
	DBG_VERBOSE(printf(" is root\n"));
	*parent = UA_NULL;
	return UA_SUCCESS;
}

/** @brief recurse down to root and return root node */
UA_Int32 UA_Node_getRoot(const UA_Node* node, const UA_Node** root) {
	UA_Int32 retval = UA_SUCCESS;
	const UA_Node* parent = UA_NULL;
	if ( (retval = UA_Node_getParent(node,&parent)) == UA_SUCCESS ) {
		if (parent != UA_NULL) {	// recurse down to root node
			retval = UA_Node_getRoot(parent,root);
		} else {					// node is root, terminate recursion
			*root = node;
		}
	}
	return retval;
}

/** @brief check if VariableNode needs a memory object. This is the
 * case if the parent is of type object and the root is type object
 **/
_Bool UA_VariableNode_needsObject(const UA_VariableNode* node) {
	const UA_Node* parent = UA_NULL;
	if ( UA_Node_getParent((UA_Node*)node,&parent) == UA_SUCCESS ) {
		if (parent == UA_NULL)
			return UA_TRUE;
		if (parent->nodeClass == UA_NODECLASS_OBJECT ) {
			const UA_Node* root;
			if (UA_Node_getRoot(parent,&root) == UA_SUCCESS)
				if (root == UA_NULL || root->nodeClass == UA_NODECLASS_OBJECT )
					return UA_TRUE;
		}
	}
	return UA_FALSE;
}

/** @brief recurse down to root and get full qualified name */
UA_Int32 UA_Node_getPath(const UA_Node* node, UA_list_List* list) {
	UA_Int32 retval = UA_SUCCESS;
	const UA_Node* parent = UA_NULL;
	if ( (retval = UA_Node_getParent(node,&parent)) == UA_SUCCESS ) {
		if (parent != UA_NULL) {
			// recurse down to root node
			UA_Int32 retval = UA_Node_getPath(parent,list);
			// and add our own name when we come back
			if (retval == UA_SUCCESS) {
				UA_list_addPayloadToBack(list,(void*)&(node->browseName.name));
				printf("// UA_Node_getPath - add id={i=%d},class=%d",UA_NodeId_getIdentifier(&(node->nodeId)),node->nodeClass);
				UA_String_printf(",name=",&(node->browseName.name));
			}
		} else {
			// node is root, terminate recursion by adding own name
			UA_list_addPayloadToBack(list,(void*)&node->browseName.name);
			printf("// UA_Node_getPath - add id={i=%d},class=%d",UA_NodeId_getIdentifier(&(node->nodeId)),node->nodeClass);
			UA_String_printf(",name=",&(node->browseName.name));
		}
	}
	return retval;
}


/** @brief some macros to lowercase the first character without copying around */
#define F_cls "%c%.*s"
#define LC_cls(str) tolower((str).data[0]), (str).length-1, &((str).data[1])

void listPrintName(void * payload) {
	UA_ByteString* name = (UA_ByteString*) payload;
	if (name->length > 0) {
		printf("_" F_cls, LC_cls(*name));
	}
}

/** @brief declares all the top level objects in the server's application memory */
void sam_declareAttribute(UA_Node const * node) {
	if (node->nodeClass == UA_NODECLASS_VARIABLE && UA_VariableNode_needsObject((UA_VariableNode*)node)) {
		UA_list_List list; UA_list_init(&list);
		UA_Int32 retval = UA_Node_getPath(node,&list);
		if (retval == UA_SUCCESS) {
			UA_VariableNode* vn = (UA_VariableNode*) node;
			printf("\t%s ", UA_[UA_ns0ToVTableIndex(vn->dataType.identifier.numeric)].name);
			UA_list_iteratePayload(&list,listPrintName);
			printf("; // i=%d\n", node->nodeId.identifier.numeric);
		} else {
			printf("// could not determine path for i=%d\n",node->nodeId.identifier.numeric);
		}
		UA_list_destroy(&list,UA_NULL);
	}
}

/** @brief declares all the buffers for string variables
 * FIXME: shall traverse down to the root object and create a unique name such as cstr_serverState_buildInfo_version
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


/** @ brief poor man's text template processor
 * for p in patterns: print p.s, iterate over namespace with p.v */
typedef struct pattern {
		char* s;
		Namespace_nodeVisitor v;
} pattern;

pattern p[] = {
{ "/** server application memory - generated but manually adapted */\n",UA_NULL },
{ "#define SAM_ASSIGN_CSTRING(src,dst) do { dst.length = strlen(src)-1; dst.data = (UA_Byte*) src; } while(0)\n",UA_NULL },
{ "struct sam {\n", sam_declareAttribute },
{ "} sam;\n", UA_NULL },
{ UA_NULL, sam_declareBuffer },
{ "void sam_init(Namespace* ns) {\n", sam_assignBuffer },
{ UA_NULL, sam_attachToNamespace },
{ "}\n", UA_NULL },
{UA_NULL, UA_NULL} // terminal node : both elements UA_NULL
};

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("usage: %s filename\n",argv[0]);
	} else {
		Namespace* ns;
		if (Namespace_loadFromFile(&ns,0,"ROOT",argv[1]) != UA_SUCCESS) {
			printf("error loading file {%s}\n", argv[1]);
		} else {
			theNamespace = ns;
			for (pattern* pi = &p[0]; pi->s != UA_NULL || pi->v != UA_NULL; ++pi) {
				if (pi->s) {
					printf("%s",pi->s);
				}
				if (pi->v) {
					Namespace_iterate(ns, pi->v);
				}
			}
			// FIXME: crashes with a seg fault
			// Namespace_delete(ns);
		}
	}
	return 0;
}
