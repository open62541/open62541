#include "ua_namespace.h"
#include "ua_xml.h"

void print_node(UA_Node const * node);
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

/** @brief load a namespace from an XML-File
 *
 * @param[in/out] ns the address of the namespace ptr
 * @param[in] namespaceId the numeric id of the namespace
 * @param[in] rootName the name of the root element of the hierarchy (not used?)
 * @param[in] fileName the name of an existing file, e.g. Opc.Ua.NodeSet2.xml
 */
UA_Int32 Namespace_loadFromFile(Namespace **ns,UA_UInt32 namespaceId,const char* rootName,const char* fileName);

