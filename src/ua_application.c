/*
 * ua_application.c
 *
 *  Created on: 16.04.2014
 *      Author: mrt
 */
#include "ua_application.h"
#include "ua_namespace.h"

UA_indexedList_List nsMockup;
Application appMockup = {
		( UA_ApplicationDescription*) UA_NULL,
		&nsMockup
};

void appMockup_init() {
	namespace* ns0;
	UA_ByteString name_ns0 = { 28, (UA_Byte*) "http://opcfoundation.org/UA/" };
	namespace* local;
	UA_ByteString name_local = { 9, (UA_Byte*) "open62541" };
	create_ns(&ns0,100);
	ns0->namespaceId = 0;
	ns0->namespaceUri.length = name_ns0.length;
	ns0->namespaceUri.data= name_ns0.data;
	create_ns(&local,100);
	local->namespaceId = 1;
	local->namespaceUri.length = name_local.length;
	local->namespaceUri.data= name_local.data;

	UA_indexedList_addValueToFront(appMockup.namespaces,0,ns0);
	UA_indexedList_addValueToFront(appMockup.namespaces,1,local);
	UA_Node server = {
		(UA_NodeId) { UA_NODEIDTYPE_FOURBYTE, 0, { 2253 }}, // nodeId;
		(UA_NodeClass) UA_NODECLASS_OBJECT, // nodeClass;
		(UA_QualifiedName) { 0,0, (UA_String) { 6, (UA_Byte*) "Server"}}, // UA_QualifiedName browseName;
		(UA_LocalizedText)	{ UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT, (UA_String) {-1, UA_NULL }, (UA_String) { 9, (UA_Byte*) "open62541"}}, // 	UA_LocalizedText displayName;
		(UA_LocalizedText)	{ UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT, (UA_String) {-1, UA_NULL }, (UA_String) { 9, (UA_Byte*) "open62541"}}, // 	UA_LocalizedText description;
		(UA_UInt32) 0, // writeMask
		(UA_UInt32) 0, // userWriteMask;
		(UA_Int32) -1, // referencesSize;
		(UA_ReferenceNode**) UA_NULL // references;
	};

	UA_ByteString* name_table[] = { &name_ns0, &name_local};

	UA_VariableNode server_NamespaceArray = {
		(UA_NodeId) { UA_NODEIDTYPE_FOURBYTE, 0, { 2255 }}, // nodeId;
		(UA_NodeClass) UA_NODECLASS_VARIABLE, // nodeClass;
		(UA_QualifiedName) { 0,0, (UA_String) { 21, (UA_Byte*) "Server_NamespaceArray"}}, // UA_QualifiedName browseName;
		(UA_LocalizedText)	{ UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT, (UA_String) {-1, UA_NULL }, (UA_String) { 9, (UA_Byte*) "open62541"}}, // 	UA_LocalizedText displayName;
		(UA_LocalizedText)	{ UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT, (UA_String) {-1, UA_NULL }, (UA_String) { 9, (UA_Byte*) "open62541"}}, // 	UA_LocalizedText description;
		(UA_UInt32) 0, // writeMask
		(UA_UInt32) 0, // userWriteMask;
		(UA_Int32) -1, // referencesSize;
		(UA_ReferenceNode**) UA_NULL, // references;
		(UA_Variant) {
			(UA_VTable*) &UA_[UA_STRING],
			(UA_Byte) UA_VARIANT_ENCODINGMASKTYPE_ARRAY || UA_STRING_NS0,
			(UA_Int32) 2,
			(void**) name_table
		},
		(UA_NodeId) { UA_NODEIDTYPE_FOURBYTE, 0, { UA_STRING_NS0 }},//dataType;
		(UA_Int32) 1,//valueRank;
		(UA_Int32) -1,//arrayDimensionsSize;
		(UA_UInt32**) UA_NULL,//arrayDimensions;
		(UA_Byte) 0,//accessLevel;
		(UA_Byte) 0,//userAccessLevel;
		(UA_Double) 1.0,//minimumSamplingInterval;
		(UA_Boolean) UA_FALSE//historizing;
	};
	insert_node(ns0,&server);
	insert_node(ns0,(UA_Node*) &server_NamespaceArray);
}
