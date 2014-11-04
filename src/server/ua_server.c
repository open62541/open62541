#include "ua_server.h"

#include "ua_services_internal.h" // AddReferences
#include "ua_namespace_0.h"
#include "ua_securechannel_manager.h"
#include "ua_namespace_manager.h"
#include "ua_session_manager.h"
#include "ua_util.h"
#include "ua_services.h"

UA_StatusCode UA_Server_deleteMembers(UA_Server *server) {
	UA_ApplicationDescription_deleteMembers(&server->description);
	UA_SecureChannelManager_delete(server->secureChannelManager);
	UA_SessionManager_delete(server->sessionManager);
	//   UA_NodeStoreExample_delete(server->nodestore);
	UA_ByteString_deleteMembers(&server->serverCertificate);
	return UA_STATUSCODE_GOOD;
}
void addSingleReference(UA_Namespace *namespace,
		UA_AddReferencesItem *addReferencesItem) {
	UA_UInt32 indices = 1;
	UA_UInt32 indicesSize = 1;
	UA_DiagnosticInfo diagnosticInfo;
	UA_StatusCode result;
	UA_RequestHeader tmpRequestHeader;
	namespace->nodeStore->addReferences(&tmpRequestHeader,addReferencesItem, &indices,
			indicesSize, &result, &diagnosticInfo);
}
void addSingleNode(UA_Namespace *namespace, UA_AddNodesItem *addNodesItem) {
	UA_UInt32 indices = 0;
	UA_UInt32 indicesSize = 1;
	UA_DiagnosticInfo diagnosticInfo;
	UA_AddNodesResult result;
	UA_RequestHeader tmpRequestHeader;

	namespace->nodeStore->addNodes(&tmpRequestHeader,addNodesItem, &indices, indicesSize, &result,
			&diagnosticInfo);
}

void ns0_addObjectNode(UA_Server *server, UA_NodeId REFTYPE_NODEID,
		UA_ExpandedNodeId REQ_NODEID, UA_ExpandedNodeId PARENTNODEID,
		char* BROWSENAME, char* DISPLAYNAME, char* DESCRIPTION) {
	UA_ObjectAttributes objAttr;
	UA_AddNodesItem addNodesItem;
	UA_Namespace *ns0 = UA_NULL;
	UA_NamespaceManager_getNamespace(server->namespaceManager, 0, &ns0);
	addNodesItem.parentNodeId = PARENTNODEID;
	addNodesItem.requestedNewNodeId = REQ_NODEID;
	addNodesItem.referenceTypeId = REFTYPE_NODEID;
	addNodesItem.nodeClass = UA_NODECLASS_OBJECT;\
	UA_QualifiedName_copycstring(BROWSENAME, &addNodesItem.browseName);
	UA_LocalizedText_copycstring(DISPLAYNAME, &objAttr.displayName);
	UA_LocalizedText_copycstring(DESCRIPTION, &objAttr.description);
	objAttr.userWriteMask = 0;
	objAttr.writeMask = 0;
	objAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_BROWSENAME;
	objAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_DISPLAYNAME;
	objAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_DESCRIPTION;
	UA_UInt32 offset = 0;
	UA_ByteString_newMembers(&addNodesItem.nodeAttributes.body,
			UA_ObjectAttributes_calcSizeBinary(&objAttr));
	UA_ObjectAttributes_encodeBinary(&objAttr,
			&addNodesItem.nodeAttributes.body, &offset);
	addSingleNode(ns0, &addNodesItem);
}
void ns0_addVariableNode(UA_Server *server, UA_NodeId REFTYPE_NODEID,
		UA_ExpandedNodeId REQ_NODEID, UA_ExpandedNodeId PARENTNODEID,
		UA_QualifiedName browseName, UA_LocalizedText displayName, UA_LocalizedText description,
		UA_DataValue *dataValue, UA_Int32 valueRank) {
	UA_VariableAttributes varAttr;
	UA_AddNodesItem addNodesItem;
	UA_Namespace *ns0 = UA_NULL;
	UA_NamespaceManager_getNamespace(server->namespaceManager, 0, &ns0);
	addNodesItem.parentNodeId = PARENTNODEID;
	addNodesItem.requestedNewNodeId = REQ_NODEID;
	addNodesItem.referenceTypeId = REFTYPE_NODEID;
	addNodesItem.nodeClass = UA_NODECLASS_VARIABLE;
	addNodesItem.browseName = browseName;
	varAttr.displayName = displayName ;
	varAttr.description = description;
	varAttr.value = dataValue->value;
	varAttr.userWriteMask = 0;
	varAttr.writeMask = 0;

	varAttr.dataType = dataValue->value.vt->typeId;

	varAttr.valueRank = valueRank;
	varAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_VALUERANK;

	varAttr.arrayDimensions = (UA_UInt32*)dataValue->value.storage.data.arrayDimensions;
	varAttr.arrayDimensionsSize = dataValue->value.storage.data.arrayDimensionsLength;

	varAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_BROWSENAME;
	varAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_DISPLAYNAME;
	varAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_DESCRIPTION;
	varAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_VALUE;
	varAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_DATATYPE;
	varAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_ARRAYDIMENSIONS;




	UA_UInt32 offset = 0;
	UA_ByteString_newMembers(&addNodesItem.nodeAttributes.body,
			UA_VariableAttributes_calcSizeBinary(&varAttr));
	UA_VariableAttributes_encodeBinary(&varAttr,
			&addNodesItem.nodeAttributes.body, &offset);
	addSingleNode(ns0, &addNodesItem);
}

void UA_Server_init(UA_Server *server, UA_String *endpointUrl) {
	UA_ExpandedNodeId_init(&server->objectsNodeId);
	server->objectsNodeId.nodeId.identifier.numeric = 85;

	UA_NodeId_init(&server->hasComponentReferenceTypeId);
	server->hasComponentReferenceTypeId.identifier.numeric = 47;

	UA_ApplicationDescription_init(&server->description);
	UA_ByteString_init(&server->serverCertificate);
#define MAXCHANNELCOUNT 100
#define STARTCHANNELID 1
#define TOKENLIFETIME 10000
#define STARTTOKENID 1
	UA_SecureChannelManager_new(&server->secureChannelManager, MAXCHANNELCOUNT,
	TOKENLIFETIME, STARTCHANNELID, STARTTOKENID, endpointUrl);

#define MAXSESSIONCOUNT 1000
#define SESSIONLIFETIME 10000
#define STARTSESSIONID 1
	UA_SessionManager_new(&server->sessionManager, MAXSESSIONCOUNT,
			SESSIONLIFETIME,
			STARTSESSIONID);

	//ns0: C2UA_STRING("http://opcfoundation.org/UA/"));
	//ns1: C2UA_STRING("http://localhost:16664/open62541/"));

	/**************/
	/* References */
	/**************/

	// ReferenceType Ids
	UA_ExpandedNodeId RefTypeId_References;
	NS0EXPANDEDNODEID(RefTypeId_References, 31);
	UA_ExpandedNodeId RefTypeId_NonHierarchicalReferences;
	NS0EXPANDEDNODEID(RefTypeId_NonHierarchicalReferences, 32);
	UA_ExpandedNodeId RefTypeId_HierarchicalReferences;
	NS0EXPANDEDNODEID(RefTypeId_HierarchicalReferences, 33);
	UA_ExpandedNodeId RefTypeId_HasChild;
	NS0EXPANDEDNODEID(RefTypeId_HasChild, 34);
	UA_ExpandedNodeId RefTypeId_Organizes;
	NS0EXPANDEDNODEID(RefTypeId_Organizes, 35);
	UA_ExpandedNodeId RefTypeId_HasEventSource;
	NS0EXPANDEDNODEID(RefTypeId_HasEventSource, 36);
	UA_ExpandedNodeId RefTypeId_HasModellingRule;
	NS0EXPANDEDNODEID(RefTypeId_HasModellingRule, 37);
	UA_ExpandedNodeId RefTypeId_HasEncoding;
	NS0EXPANDEDNODEID(RefTypeId_HasEncoding, 38);
	UA_ExpandedNodeId RefTypeId_HasDescription;
	NS0EXPANDEDNODEID(RefTypeId_HasDescription, 39);
	UA_ExpandedNodeId RefTypeId_HasTypeDefinition;
	NS0EXPANDEDNODEID(RefTypeId_HasTypeDefinition, 40);
	UA_ExpandedNodeId RefTypeId_GeneratesEvent;
	NS0EXPANDEDNODEID(RefTypeId_GeneratesEvent, 41);
	UA_ExpandedNodeId RefTypeId_Aggregates;
	NS0EXPANDEDNODEID(RefTypeId_Aggregates, 44);
	UA_ExpandedNodeId RefTypeId_HasSubtype;
	NS0EXPANDEDNODEID(RefTypeId_HasSubtype, 45);
	UA_ExpandedNodeId RefTypeId_HasProperty;
	NS0EXPANDEDNODEID(RefTypeId_HasProperty, 46);
	UA_ExpandedNodeId RefTypeId_HasComponent;
	NS0EXPANDEDNODEID(RefTypeId_HasComponent, 47);
	UA_ExpandedNodeId RefTypeId_HasNotifier;
	NS0EXPANDEDNODEID(RefTypeId_HasNotifier, 48);
	UA_ExpandedNodeId RefTypeId_HasOrderedComponent;
	NS0EXPANDEDNODEID(RefTypeId_HasOrderedComponent, 49);
	UA_ExpandedNodeId RefTypeId_HasModelParent;
	NS0EXPANDEDNODEID(RefTypeId_HasModelParent, 50);
	UA_ExpandedNodeId RefTypeId_FromState;
	NS0EXPANDEDNODEID(RefTypeId_FromState, 51);
	UA_ExpandedNodeId RefTypeId_ToState;
	NS0EXPANDEDNODEID(RefTypeId_ToState, 52);
	UA_ExpandedNodeId RefTypeId_HasCause;
	NS0EXPANDEDNODEID(RefTypeId_HasCause, 53);
	UA_ExpandedNodeId RefTypeId_HasEffect;
	NS0EXPANDEDNODEID(RefTypeId_HasEffect, 54);
	UA_ExpandedNodeId RefTypeId_HasHistoricalConfiguration;
	NS0EXPANDEDNODEID(RefTypeId_HasHistoricalConfiguration, 56);

#define ADD_REFTYPENODE_NS0(REFTYPE_NODEID,REQ_NODEID,PARENTNODEID,REFTYPE_BROWSENAME,REFTYPE_DISPLAYNAME,REFTYPE_DESCRIPTION, IS_ABSTRACT, IS_SYMMETRIC) do{\
	UA_AddNodesItem addNodesItem; \
    UA_Namespace *ns0; \
    UA_NamespaceManager_getNamespace(server->namespaceManager,0,&ns0); \
	UA_ReferenceTypeAttributes refTypeAttr; \
    addNodesItem.parentNodeId= PARENTNODEID;\
    addNodesItem.requestedNewNodeId = REQ_NODEID; \
    addNodesItem.referenceTypeId = REFTYPE_NODEID; \
    addNodesItem.nodeClass = UA_NODECLASS_REFERENCETYPE; \
    UA_QualifiedName_copycstring(REFTYPE_BROWSENAME, &addNodesItem.browseName); \
    UA_LocalizedText_copycstring(REFTYPE_DISPLAYNAME, &refTypeAttr.displayName); \
    UA_LocalizedText_copycstring(REFTYPE_DESCRIPTION, &refTypeAttr.description); \
    refTypeAttr.isAbstract = IS_ABSTRACT; \
    refTypeAttr.symmetric  = IS_SYMMETRIC; \
    refTypeAttr.userWriteMask = 0; \
    refTypeAttr.writeMask = 0; \
    refTypeAttr.inverseName.locale.length = 0; \
    refTypeAttr.inverseName.text.length = 0; \
    refTypeAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_BROWSENAME; \
    refTypeAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_DISPLAYNAME; \
    refTypeAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_DESCRIPTION; \
    refTypeAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_ISABSTRACT; \
    refTypeAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_SYMMETRIC; \
    UA_UInt32 offset = 0; \
    UA_ByteString_newMembers(&addNodesItem.nodeAttributes.body,UA_ReferenceTypeAttributes_calcSizeBinary(&refTypeAttr));\
    UA_ReferenceTypeAttributes_encodeBinary(&refTypeAttr,&addNodesItem.nodeAttributes.body,&offset); \
    addSingleNode(ns0,&addNodesItem);\
}while(1==0);

	/*
	 #define ADDREFERENCE(NODE, REFTYPE, INVERSE, TARGET_NODEID) do { \
    static struct UA_ReferenceNode NODE##REFTYPE##TARGET_NODEID;    \
    UA_ReferenceNode_init(&NODE##REFTYPE##TARGET_NODEID);       \
    NODE##REFTYPE##TARGET_NODEID.referenceTypeId = REFTYPE.nodeId;     \
    NODE##REFTYPE##TARGET_NODEID.isInverse       = INVERSE; \
    NODE##REFTYPE##TARGET_NODEID.targetId = TARGET_NODEID; \
    AddReference(server->nodestore, (UA_Node *)NODE, &NODE##REFTYPE##TARGET_NODEID); \
    } while(0)


	 */

	// ObjectTypes (Ids only)
//    UA_ExpandedNodeId ObjTypeId_FolderType; NS0EXPANDEDNODEID(ObjTypeId_FolderType, 61);
	// Objects (Ids only)
	UA_ExpandedNodeId ObjId_Null;
	NS0EXPANDEDNODEID(ObjId_Null, 0);
	UA_ExpandedNodeId ObjId_Root;
	NS0EXPANDEDNODEID(ObjId_Root, 84);
	UA_ExpandedNodeId ObjId_ObjectsFolder;
	NS0EXPANDEDNODEID(ObjId_ObjectsFolder, 85);
	UA_ExpandedNodeId ObjId_TypesFolder;
	NS0EXPANDEDNODEID(ObjId_TypesFolder, 86);
	UA_ExpandedNodeId ObjId_ViewsFolder;
	NS0EXPANDEDNODEID(ObjId_ViewsFolder, 87);
	UA_ExpandedNodeId ObjId_ReferenceTypesFolder;
	NS0EXPANDEDNODEID(ObjId_ReferenceTypesFolder, 91);
	UA_ExpandedNodeId ObjId_Server;
	NS0EXPANDEDNODEID(ObjId_Server, 2253);
	//UA_ExpandedNodeId VarId_ServerArray;
	//NS0EXPANDEDNODEID(VarId_ServerArray, 2254);
    UA_ExpandedNodeId VarId_NamespaceArray; NS0EXPANDEDNODEID(VarId_NamespaceArray, 2255);
    UA_ExpandedNodeId VarId_ServerStatus; NS0EXPANDEDNODEID(VarId_ServerStatus, 2256);
//    UA_ExpandedNodeId ObjId_ServerCapabilities; NS0EXPANDEDNODEID(ObjId_ServerCapabilities, 2268);
    UA_ExpandedNodeId VarId_State; NS0EXPANDEDNODEID(VarId_State, 2259);

	ns0_addObjectNode(server,RefTypeId_Organizes.nodeId, ObjId_Root, ObjId_Null,
			"Root", "Root", "Root");
	ns0_addObjectNode(server,RefTypeId_Organizes.nodeId, ObjId_ObjectsFolder,
			ObjId_Root, "Objects", "Objects", "Objects");
	ns0_addObjectNode(server,RefTypeId_Organizes.nodeId, ObjId_Server,
			ObjId_ObjectsFolder, "Server", "Server", "Server");


	UA_DataValue *serverArrayValue;
	UA_DataValue_new(&serverArrayValue);
	UA_UInt32 namespaceArraySize = 2;

	UA_Array_new((void**)&serverArrayValue->value.storage.data.dataPtr, namespaceArraySize, &UA_[UA_STRING]);
	UA_String_copycstring("http://opcfoundation.org/UA/", &((UA_String *)(serverArrayValue->value.storage.data.dataPtr))[0]);
	UA_Int32 *arrayDim;
	UA_UInt32 arrayDimSize = 1;
	UA_Array_new((void**)&arrayDim, arrayDimSize, &UA_[UA_INT32]);

	serverArrayValue->value.vt = &UA_[UA_STRING];
	serverArrayValue->value.storage.data.arrayDimensions = arrayDim;
	serverArrayValue->value.storage.data.arrayDimensionsLength = 1; // added to ensure encoding in readreponse
	serverArrayValue->value.storage.data.arrayLength = 1;
	serverArrayValue->value.storageType = UA_VARIANT_DATA;
	 {
		 UA_QualifiedName *browseName;
		 UA_QualifiedName_new(&browseName);
		 UA_LocalizedText *description;
		 UA_LocalizedText_new(&description);
		 UA_LocalizedText *displayName;
		 UA_LocalizedText_new(&displayName);

		 UA_QualifiedName_copycstring("NamespaceArray",browseName);
		 UA_LocalizedText_copycstring("NamespaceArray",description);
		 UA_LocalizedText_copycstring("NamespaceArray",displayName);

		 ns0_addVariableNode(server,RefTypeId_HasComponent.nodeId,VarId_NamespaceArray,ObjId_Server, *browseName,*description, *displayName,serverArrayValue,1);
	 }



	 // ServerStatus
	 UA_DataValue *serverStatusValue;
	 UA_DataValue_new(&serverStatusValue);

	 UA_ServerStatusDataType *status;
	 UA_ServerStatusDataType_new(&status);
	 status->startTime   = UA_DateTime_now();
	 status->currentTime = UA_DateTime_now();
	 status->state       = UA_SERVERSTATE_RUNNING;
	 UA_String_copycstring("open62541.org", &status->buildInfo.productUri);
	 UA_String_copycstring("open62541", &status->buildInfo.manufacturerName);
	 UA_String_copycstring("open62541", &status->buildInfo.productName);
	 UA_String_copycstring("0.0", &status->buildInfo.softwareVersion);
	 UA_String_copycstring("0.0", &status->buildInfo.buildNumber);
	 status->buildInfo.buildDate     = UA_DateTime_now();
	 status->secondsTillShutdown     = 99999999;
	 UA_LocalizedText_copycstring("because", &status->shutdownReason);
	 serverStatusValue->value.vt          = &UA_[UA_SERVERSTATUSDATATYPE]; // gets encoded as an extensionobject
	 serverStatusValue->value.storage.data.arrayLength = 0;
	 serverStatusValue->value.storage.data.dataPtr        = status;
	 serverStatusValue->value.storage.data.arrayDimensionsLength = 0;
	 {
		 UA_QualifiedName *browseName;
		 UA_QualifiedName_new(&browseName);
		 UA_LocalizedText *description;
		 UA_LocalizedText_new(&description);
		 UA_LocalizedText *displayName;
		 UA_LocalizedText_new(&displayName);

		 UA_QualifiedName_copycstring("ServerStatus",browseName);
		 UA_LocalizedText_copycstring("ServerStatus",description);
		 UA_LocalizedText_copycstring("ServerStatus",displayName);

		 ns0_addVariableNode(server,RefTypeId_HasComponent.nodeId,VarId_ServerStatus,ObjId_Server, *browseName, *description, *displayName ,serverStatusValue,-1);
	}

	 // State (Component of ServerStatus)

	 UA_DataValue *sateValue;
	 UA_DataValue_new(&sateValue);

	 sateValue->value.vt = &UA_[UA_SERVERSTATE];
	 sateValue->value.storage.data.arrayDimensionsLength = 0; // added to ensure encoding in readreponse
	 sateValue->value.storage.data.arrayLength = 0;
	 sateValue->value.storage.data.dataPtr = &status->state; // points into the other object.
	 sateValue->value.storageType = UA_VARIANT_DATA;

	 {
		 UA_QualifiedName *browseName;
		 UA_QualifiedName_new(&browseName);
		 UA_LocalizedText *description;
		 UA_LocalizedText_new(&description);
		 UA_LocalizedText *displayName;
		 UA_LocalizedText_new(&displayName);

		 UA_QualifiedName_copycstring("State",browseName);
		 UA_LocalizedText_copycstring("State",description);
		 UA_LocalizedText_copycstring("State",displayName);

		 ns0_addVariableNode(server,RefTypeId_HasComponent.nodeId,VarId_State,ObjId_Server, *browseName, *description,*displayName ,sateValue,-1);
	 }

	ns0_addObjectNode(server,RefTypeId_Organizes.nodeId, ObjId_TypesFolder, ObjId_Root,
			"Types", "Types", "Types");

	ns0_addObjectNode(server,RefTypeId_Organizes.nodeId, ObjId_TypesFolder, ObjId_Root,
			"Types", "Types", "Types");
	ns0_addObjectNode(server,RefTypeId_Organizes.nodeId, ObjId_ReferenceTypesFolder,
			ObjId_TypesFolder, "ReferenceTypes", "ReferenceTypes",
			"ReferenceTypes");
	ADD_REFTYPENODE_NS0(RefTypeId_Organizes.nodeId, RefTypeId_References,
			ObjId_ReferenceTypesFolder, "References", "References",
			"References", UA_TRUE, UA_TRUE);

	ns0_addObjectNode(server,RefTypeId_Organizes.nodeId, ObjId_ViewsFolder, ObjId_Root,
			"Views", "Views", "Views");

	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId,
			RefTypeId_NonHierarchicalReferences, RefTypeId_References,
			"NonHierarchicalReferences", "NonHierarchicalReferences",
			"NonHierarchicalReferences", UA_TRUE, UA_FALSE);

	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_HasModelParent,
			RefTypeId_NonHierarchicalReferences, "HasModelParent",
			"HasModelParent", "HasModelParent", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_GeneratesEvent,
			RefTypeId_NonHierarchicalReferences, "GeneratesEvent",
			"GeneratesEvent", "GeneratesEvent", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_HasEncoding,
			RefTypeId_NonHierarchicalReferences, "HasEncoding", "HasEncoding",
			"HasEncoding", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_HasModellingRule,
			RefTypeId_NonHierarchicalReferences, "HasModellingRule",
			"HasModellingRule", "HasModellingRule", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_HasDescription,
			RefTypeId_NonHierarchicalReferences, "HasDescription",
			"HasDescription", "HasDescription", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId,
			RefTypeId_HasTypeDefinition, RefTypeId_NonHierarchicalReferences,
			"HasTypeDefinition", "HasTypeDefinition", "HasTypeDefinition",
			UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_FromState,
			RefTypeId_NonHierarchicalReferences, "FromState", "FromState",
			"FromState", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_ToState,
			RefTypeId_NonHierarchicalReferences, "ToState", "ToState",
			"ToState", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_HasCause,
			RefTypeId_NonHierarchicalReferences, "HasCause", "HasCause",
			"HasCause", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_HasEffect,
			RefTypeId_NonHierarchicalReferences, "HasEffect", "HasEffect",
			"HasEffect", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId,
			RefTypeId_HasHistoricalConfiguration,
			RefTypeId_NonHierarchicalReferences, "HasHistoricalConfiguration",
			"HasHistoricalConfiguration", "HasHistoricalConfiguration", UA_TRUE,
			UA_FALSE);

	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId,
			RefTypeId_HierarchicalReferences, RefTypeId_References,
			"HierarchicalReferences", "HierarchicalReferences",
			"HierarchicalReferences", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_HasEventSource,
			RefTypeId_HierarchicalReferences, "HasEventSource",
			"HasEventSource", "HasEventSource", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_HasNotifier,
			RefTypeId_HasEventSource, "HasEventSource", "HasEventSource",
			"HasEventSource", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_HasChild,
			RefTypeId_HierarchicalReferences, "HasChild", "HasChild",
			"HasChild", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_Aggregates,
			RefTypeId_HasChild, "Aggregates", "Aggregates", "Aggregates",
			UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_HasProperty,
			RefTypeId_Aggregates, "HasProperty", "HasProperty", "HasProperty",
			UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_HasComponent,
			RefTypeId_Aggregates, "HasComponent", "HasComponent",
			"HasComponent", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId,
			RefTypeId_HasOrderedComponent, RefTypeId_HasComponent,
			"HasOrderedComponent", "HasOrderedComponent", "HasOrderedComponent",
			UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_HasSubtype,
			RefTypeId_HasChild, "HasSubtype", "HasSubtype", "HasSubtype",
			UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(RefTypeId_HasSubtype.nodeId, RefTypeId_Organizes,
			RefTypeId_HierarchicalReferences, "Organizes", "Organizes",
			"Organizes", UA_TRUE, UA_FALSE);

	/*UA_ReferenceTypeNode *references;
	 UA_ReferenceTypeNode_new(&references);
	 references->nodeId    = RefTypeId_References.nodeId;
	 references->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("References", &references->browseName);
	 UA_LocalizedText_copycstring("References", &references->displayName);
	 UA_LocalizedText_copycstring("References", &references->description);
	 references->isAbstract = UA_TRUE;
	 references->symmetric  = UA_TRUE;
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&references, UA_NODESTORE_INSERT_UNIQUE);



	 //addSingleNode()

	 UA_ReferenceTypeNode *hierarchicalreferences;
	 UA_ReferenceTypeNode_new(&hierarchicalreferences);
	 hierarchicalreferences->nodeId    = RefTypeId_HierarchicalReferences.nodeId;
	 hierarchicalreferences->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HierarchicalReferences", &hierarchicalreferences->browseName);
	 UA_LocalizedText_copycstring("HierarchicalReferences", &hierarchicalreferences->displayName);
	 UA_LocalizedText_copycstring("HierarchicalReferences", &hierarchicalreferences->description);
	 hierarchicalreferences->isAbstract = UA_TRUE;
	 hierarchicalreferences->symmetric  = UA_FALSE;

	 ADDREFERENCE(hierarchicalreferences, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_References);


	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&hierarchicalreferences, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *nonhierarchicalreferences;
	 UA_ReferenceTypeNode_new(&nonhierarchicalreferences);
	 nonhierarchicalreferences->nodeId    = RefTypeId_NonHierarchicalReferences.nodeId;
	 nonhierarchicalreferences->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("NonHierarchicalReferences", &nonhierarchicalreferences->browseName);
	 UA_LocalizedText_copycstring("NonHierarchicalReferences", &nonhierarchicalreferences->displayName);
	 UA_LocalizedText_copycstring("NonHierarchicalReferences", &nonhierarchicalreferences->description);
	 nonhierarchicalreferences->isAbstract = UA_TRUE;
	 nonhierarchicalreferences->symmetric  = UA_FALSE;
	 ADDREFERENCE(nonhierarchicalreferences, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_References);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&nonhierarchicalreferences, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *haschild;
	 UA_ReferenceTypeNode_new(&haschild);
	 haschild->nodeId    = RefTypeId_HasChild.nodeId;
	 haschild->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HasChild", &haschild->browseName);
	 UA_LocalizedText_copycstring("HasChild", &haschild->displayName);
	 UA_LocalizedText_copycstring("HasChild", &haschild->description);
	 haschild->isAbstract = UA_TRUE;
	 haschild->symmetric  = UA_FALSE;
	 ADDREFERENCE(haschild, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_HierarchicalReferences);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&haschild, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *organizes;
	 UA_ReferenceTypeNode_new(&organizes);
	 organizes->nodeId    = RefTypeId_Organizes.nodeId;
	 organizes->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("Organizes", &organizes->browseName);
	 UA_LocalizedText_copycstring("Organizes", &organizes->displayName);
	 UA_LocalizedText_copycstring("Organizes", &organizes->description);
	 organizes->isAbstract = UA_FALSE;
	 organizes->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("OrganizedBy", &organizes->inverseName);
	 ADDREFERENCE(organizes, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_HierarchicalReferences);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&organizes, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *haseventsource;
	 UA_ReferenceTypeNode_new(&haseventsource);
	 haseventsource->nodeId    = RefTypeId_HasEventSource.nodeId;
	 haseventsource->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HasEventSource", &haseventsource->browseName);
	 UA_LocalizedText_copycstring("HasEventSource", &haseventsource->displayName);
	 UA_LocalizedText_copycstring("HasEventSource", &haseventsource->description);
	 haseventsource->isAbstract = UA_FALSE;
	 haseventsource->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("EventSourceOf", &haseventsource->inverseName);
	 ADDREFERENCE(haseventsource, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_HierarchicalReferences);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&haseventsource, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *hasmodellingrule;
	 UA_ReferenceTypeNode_new(&hasmodellingrule);
	 hasmodellingrule->nodeId    = RefTypeId_HasModellingRule.nodeId;
	 hasmodellingrule->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HasModellingRule", &hasmodellingrule->browseName);
	 UA_LocalizedText_copycstring("HasModellingRule", &hasmodellingrule->displayName);
	 UA_LocalizedText_copycstring("HasModellingRule", &hasmodellingrule->description);
	 hasmodellingrule->isAbstract = UA_FALSE;
	 hasmodellingrule->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("ModellingRuleOf", &hasmodellingrule->inverseName);
	 ADDREFERENCE(hasmodellingrule, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_NonHierarchicalReferences);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&hasmodellingrule, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *hasencoding;
	 UA_ReferenceTypeNode_new(&hasencoding);
	 hasencoding->nodeId    = RefTypeId_HasEncoding.nodeId;
	 hasencoding->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HasEncoding", &hasencoding->browseName);
	 UA_LocalizedText_copycstring("HasEncoding", &hasencoding->displayName);
	 UA_LocalizedText_copycstring("HasEncoding", &hasencoding->description);
	 hasencoding->isAbstract = UA_FALSE;
	 hasencoding->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("EncodingOf", &hasencoding->inverseName);
	 ADDREFERENCE(hasencoding, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_NonHierarchicalReferences);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&hasencoding, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *hasdescription;
	 UA_ReferenceTypeNode_new(&hasdescription);
	 hasdescription->nodeId    = RefTypeId_HasDescription.nodeId;
	 hasdescription->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HasDescription", &hasdescription->browseName);
	 UA_LocalizedText_copycstring("HasDescription", &hasdescription->displayName);
	 UA_LocalizedText_copycstring("HasDescription", &hasdescription->description);
	 hasdescription->isAbstract = UA_FALSE;
	 hasdescription->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("DescriptionOf", &hasdescription->inverseName);
	 ADDREFERENCE(hasdescription, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_NonHierarchicalReferences);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&hasdescription, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *hastypedefinition;
	 UA_ReferenceTypeNode_new(&hastypedefinition);
	 hastypedefinition->nodeId    = RefTypeId_HasTypeDefinition.nodeId;
	 hastypedefinition->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HasTypeDefinition", &hastypedefinition->browseName);
	 UA_LocalizedText_copycstring("HasTypeDefinition", &hastypedefinition->displayName);
	 UA_LocalizedText_copycstring("HasTypeDefinition", &hastypedefinition->description);
	 hastypedefinition->isAbstract = UA_FALSE;
	 hastypedefinition->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("TypeDefinitionOf", &hastypedefinition->inverseName);
	 ADDREFERENCE(hastypedefinition, RefTypeId_HasSubtype, UA_TRUE,  RefTypeId_NonHierarchicalReferences);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&hastypedefinition, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *generatesevent;
	 UA_ReferenceTypeNode_new(&generatesevent);
	 generatesevent->nodeId    = RefTypeId_GeneratesEvent.nodeId;
	 generatesevent->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("GeneratesEvent", &generatesevent->browseName);
	 UA_LocalizedText_copycstring("GeneratesEvent", &generatesevent->displayName);
	 UA_LocalizedText_copycstring("GeneratesEvent", &generatesevent->description);
	 generatesevent->isAbstract = UA_FALSE;
	 generatesevent->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("GeneratedBy", &generatesevent->inverseName);
	 ADDREFERENCE(generatesevent, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_NonHierarchicalReferences);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&generatesevent, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *aggregates;
	 UA_ReferenceTypeNode_new(&aggregates);
	 aggregates->nodeId    = RefTypeId_Aggregates.nodeId;
	 aggregates->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("Aggregates", &aggregates->browseName);
	 UA_LocalizedText_copycstring("Aggregates", &aggregates->displayName);
	 UA_LocalizedText_copycstring("Aggregates", &aggregates->description);
	 aggregates->isAbstract = UA_TRUE;
	 aggregates->symmetric  = UA_FALSE;
	 ADDREFERENCE(aggregates, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_HasChild);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&aggregates, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *hassubtype;
	 UA_ReferenceTypeNode_new(&hassubtype);
	 hassubtype->nodeId    = RefTypeId_HasSubtype.nodeId;
	 hassubtype->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HasSubtype", &hassubtype->browseName);
	 UA_LocalizedText_copycstring("HasSubtype", &hassubtype->displayName);
	 UA_LocalizedText_copycstring("HasSubtype", &hassubtype->description);
	 hassubtype->isAbstract = UA_FALSE;
	 hassubtype->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("SubtypeOf", &hassubtype->inverseName);
	 ADDREFERENCE(hassubtype, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_HasChild);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&hassubtype, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *hasproperty;
	 UA_ReferenceTypeNode_new(&hasproperty);
	 hasproperty->nodeId    = RefTypeId_HasProperty.nodeId;
	 hasproperty->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HasProperty", &hasproperty->browseName);
	 UA_LocalizedText_copycstring("HasProperty", &hasproperty->displayName);
	 UA_LocalizedText_copycstring("HasProperty", &hasproperty->description);
	 hasproperty->isAbstract = UA_FALSE;
	 hasproperty->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("PropertyOf", &hasproperty->inverseName);
	 ADDREFERENCE(hasproperty, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_Aggregates);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&hasproperty, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *hascomponent;
	 UA_ReferenceTypeNode_new(&hascomponent);
	 hascomponent->nodeId    = RefTypeId_HasComponent.nodeId;
	 hascomponent->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HasComponent", &hascomponent->browseName);
	 UA_LocalizedText_copycstring("HasComponent", &hascomponent->displayName);
	 UA_LocalizedText_copycstring("HasComponent", &hascomponent->description);
	 hascomponent->isAbstract = UA_FALSE;
	 hascomponent->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("ComponentOf", &hascomponent->inverseName);
	 ADDREFERENCE(hascomponent, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_Aggregates);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&hascomponent, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *hasnotifier;
	 UA_ReferenceTypeNode_new(&hasnotifier);
	 hasnotifier->nodeId    = RefTypeId_HasNotifier.nodeId;
	 hasnotifier->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HasNotifier", &hasnotifier->browseName);
	 UA_LocalizedText_copycstring("HasNotifier", &hasnotifier->displayName);
	 UA_LocalizedText_copycstring("HasNotifier", &hasnotifier->description);
	 hasnotifier->isAbstract = UA_FALSE;
	 hasnotifier->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("NotifierOf", &hasnotifier->inverseName);
	 ADDREFERENCE(hasnotifier, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_HasEventSource);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&hasnotifier, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *hasorderedcomponent;
	 UA_ReferenceTypeNode_new(&hasorderedcomponent);
	 hasorderedcomponent->nodeId    = RefTypeId_HasOrderedComponent.nodeId;
	 hasorderedcomponent->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HasOrderedComponent", &hasorderedcomponent->browseName);
	 UA_LocalizedText_copycstring("HasOrderedComponent", &hasorderedcomponent->displayName);
	 UA_LocalizedText_copycstring("HasOrderedComponent", &hasorderedcomponent->description);
	 hasorderedcomponent->isAbstract = UA_FALSE;
	 hasorderedcomponent->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("OrderedComponentOf", &hasorderedcomponent->inverseName);
	 ADDREFERENCE(hasorderedcomponent, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_HasComponent);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&hasorderedcomponent, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *hasmodelparent;
	 UA_ReferenceTypeNode_new(&hasmodelparent);
	 hasmodelparent->nodeId    = RefTypeId_HasModelParent.nodeId;
	 hasmodelparent->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HasModelParent", &hasmodelparent->browseName);
	 UA_LocalizedText_copycstring("HasModelParent", &hasmodelparent->displayName);
	 UA_LocalizedText_copycstring("HasModelParent", &hasmodelparent->description);
	 hasmodelparent->isAbstract = UA_FALSE;
	 hasmodelparent->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("ModelParentOf", &hasmodelparent->inverseName);
	 ADDREFERENCE(hasmodelparent, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_NonHierarchicalReferences);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&hasmodelparent, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *fromstate;
	 UA_ReferenceTypeNode_new(&fromstate);
	 fromstate->nodeId    = RefTypeId_FromState.nodeId;
	 fromstate->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("FromState", &fromstate->browseName);
	 UA_LocalizedText_copycstring("FromState", &fromstate->displayName);
	 UA_LocalizedText_copycstring("FromState", &fromstate->description);
	 fromstate->isAbstract = UA_FALSE;
	 fromstate->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("ToTransition", &fromstate->inverseName);
	 ADDREFERENCE(fromstate, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_NonHierarchicalReferences);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&fromstate, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *tostate;
	 UA_ReferenceTypeNode_new(&tostate);
	 tostate->nodeId    = RefTypeId_ToState.nodeId;
	 tostate->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("ToState", &tostate->browseName);
	 UA_LocalizedText_copycstring("ToState", &tostate->displayName);
	 UA_LocalizedText_copycstring("ToState", &tostate->description);
	 tostate->isAbstract = UA_FALSE;
	 tostate->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("FromTransition", &tostate->inverseName);
	 ADDREFERENCE(tostate, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_NonHierarchicalReferences);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&tostate, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *hascause;
	 UA_ReferenceTypeNode_new(&hascause);
	 hascause->nodeId    = RefTypeId_HasCause.nodeId;
	 hascause->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HasCause", &hascause->browseName);
	 UA_LocalizedText_copycstring("HasCause", &hascause->displayName);
	 UA_LocalizedText_copycstring("HasCause", &hascause->description);
	 hascause->isAbstract = UA_FALSE;
	 hascause->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("MayBeCausedBy", &hascause->inverseName);
	 ADDREFERENCE(hascause, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_NonHierarchicalReferences);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&hascause, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *haseffect;
	 UA_ReferenceTypeNode_new(&haseffect);
	 haseffect->nodeId    = RefTypeId_HasEffect.nodeId;
	 haseffect->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HasEffect", &haseffect->browseName);
	 UA_LocalizedText_copycstring("HasEffect", &haseffect->displayName);
	 UA_LocalizedText_copycstring("HasEffect", &haseffect->description);
	 haseffect->isAbstract = UA_FALSE;
	 haseffect->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("MayBeEffectedBy", &haseffect->inverseName);
	 ADDREFERENCE(haseffect, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_NonHierarchicalReferences);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&haseffect, UA_NODESTORE_INSERT_UNIQUE);

	 UA_ReferenceTypeNode *hashistoricalconfiguration;
	 UA_ReferenceTypeNode_new(&hashistoricalconfiguration);
	 hashistoricalconfiguration->nodeId    = RefTypeId_HasHistoricalConfiguration.nodeId;
	 hashistoricalconfiguration->nodeClass = UA_NODECLASS_REFERENCETYPE;
	 UA_QualifiedName_copycstring("HasHistoricalConfiguration", &hashistoricalconfiguration->browseName);
	 UA_LocalizedText_copycstring("HasHistoricalConfiguration", &hashistoricalconfiguration->displayName);
	 UA_LocalizedText_copycstring("HasHistoricalConfiguration", &hashistoricalconfiguration->description);
	 hashistoricalconfiguration->isAbstract = UA_FALSE;
	 hashistoricalconfiguration->symmetric  = UA_FALSE;
	 UA_LocalizedText_copycstring("HistoricalConfigurationOf", &hashistoricalconfiguration->inverseName);
	 ADDREFERENCE(hashistoricalconfiguration, RefTypeId_HasSubtype, UA_TRUE, RefTypeId_Aggregates);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&hashistoricalconfiguration, UA_NODESTORE_INSERT_UNIQUE);

	 */
	/*

	 // FolderType
	 UA_ObjectNode *folderType;
	 UA_ObjectNode_new(&folderType);
	 folderType->nodeId    = NS0NODEID(61);
	 folderType->nodeClass = UA_NODECLASS_OBJECTTYPE; // I should not have to set this manually
	 UA_QualifiedName_copycstring("FolderType", &folderType->browseName);
	 UA_LocalizedText_copycstring("FolderType", &folderType->displayName);
	 UA_LocalizedText_copycstring("FolderType", &folderType->description);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&folderType, UA_NODESTORE_INSERT_UNIQUE);

	 // Root
	 UA_ObjectNode *root;
	 UA_ObjectNode_new(&root);
	 root->nodeId    = NS0NODEID(84);
	 root->nodeClass = UA_NODECLASS_OBJECT; // I should not have to set this manually
	 UA_QualifiedName_copycstring("Root", &root->browseName);
	 UA_LocalizedText_copycstring("Root", &root->displayName);
	 UA_LocalizedText_copycstring("Root", &root->description);
	 ADDREFERENCE(root, RefTypeId_HasTypeDefinition, UA_FALSE, ObjTypeId_FolderType);
	 ADDREFERENCE(root, RefTypeId_Organizes, UA_FALSE, ObjId_ObjectsFolder);
	 ADDREFERENCE(root, RefTypeId_Organizes, UA_FALSE, ObjId_TypesFolder);
	 ADDREFERENCE(root, RefTypeId_Organizes, UA_FALSE, ObjId_ViewsFolder);
	 */
	/* root becomes a managed node. we need to release it at the end.*/
	/*
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&root, UA_NODESTORE_INSERT_UNIQUE | UA_NODESTORE_INSERT_GETMANAGED);



	 // Objects
	 UA_ObjectNode *objects;
	 UA_ObjectNode_new(&objects);
	 objects->nodeId    = ObjId_ObjectsFolder.nodeId;
	 objects->nodeClass = UA_NODECLASS_OBJECT;
	 UA_QualifiedName_copycstring("Objects", &objects->browseName);
	 UA_LocalizedText_copycstring("Objects", &objects->displayName);
	 UA_LocalizedText_copycstring("Objects", &objects->description);
	 ADDREFERENCE(objects, RefTypeId_HasTypeDefinition, UA_FALSE, ObjTypeId_FolderType);
	 ADDREFERENCE(objects, RefTypeId_Organizes, UA_FALSE, ObjId_Server);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&objects, UA_NODESTORE_INSERT_UNIQUE);

	 // Types
	 UA_ObjectNode *types;
	 UA_ObjectNode_new(&types);
	 types->nodeId    = ObjId_TypesFolder.nodeId;
	 types->nodeClass = UA_NODECLASS_OBJECT;
	 UA_QualifiedName_copycstring("Types", &types->browseName);
	 UA_LocalizedText_copycstring("Types", &types->displayName);
	 UA_LocalizedText_copycstring("Types", &types->description);
	 ADDREFERENCE(types, RefTypeId_HasTypeDefinition, UA_FALSE, ObjTypeId_FolderType);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&types, UA_NODESTORE_INSERT_UNIQUE);

	 // Views
	 UA_ObjectNode *views;
	 UA_ObjectNode_new(&views);
	 views->nodeId    = ObjId_ViewsFolder.nodeId;
	 views->nodeClass = UA_NODECLASS_OBJECT;
	 UA_QualifiedName_copycstring("Views", &views->browseName);
	 UA_LocalizedText_copycstring("Views", &views->displayName);
	 UA_LocalizedText_copycstring("Views", &views->description);
	 ADDREFERENCE(views, RefTypeId_HasTypeDefinition, UA_FALSE, ObjTypeId_FolderType);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&views, UA_NODESTORE_INSERT_UNIQUE);

	 // Server
	 UA_ObjectNode *servernode;
	 UA_ObjectNode_new(&servernode);
	 servernode->nodeId    = ObjId_Server.nodeId;
	 servernode->nodeClass = UA_NODECLASS_OBJECT;
	 UA_QualifiedName_copycstring("Server", &servernode->browseName);
	 UA_LocalizedText_copycstring("Server", &servernode->displayName);
	 UA_LocalizedText_copycstring("Server", &servernode->description);
	 ADDREFERENCE(servernode, RefTypeId_HasComponent, UA_FALSE, ObjId_ServerCapabilities);
	 ADDREFERENCE(servernode, RefTypeId_HasComponent, UA_FALSE, ObjId_NamespaceArray);
	 ADDREFERENCE(servernode, RefTypeId_HasProperty, UA_FALSE, ObjId_ServerStatus);
	 ADDREFERENCE(servernode, RefTypeId_HasProperty, UA_FALSE, ObjId_ServerArray);
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&servernode, UA_NODESTORE_INSERT_UNIQUE);

	 // NamespaceArray
	 UA_VariableNode *namespaceArray;
	 UA_VariableNode_new(&namespaceArray);
	 namespaceArray->nodeId    = ObjId_NamespaceArray.nodeId;
	 namespaceArray->nodeClass = UA_NODECLASS_VARIABLE; //FIXME: this should go into _new?
	 UA_QualifiedName_copycstring("NodeStoreArray", &namespaceArray->browseName);
	 UA_LocalizedText_copycstring("NodeStoreArray", &namespaceArray->displayName);
	 UA_LocalizedText_copycstring("NodeStoreArray", &namespaceArray->description);
	 UA_Array_new((void **)&namespaceArray->value.storage.data.dataPtr, 2, &UA_[UA_STRING]);
	 namespaceArray->value.vt = &UA_[UA_STRING];
	 namespaceArray->value.storage.data.arrayLength = 2;
	 UA_String_copycstring("http://opcfoundation.org/UA/", &((UA_String *)(namespaceArray->value.storage.data.dataPtr))[0]);
	 UA_String_copycstring("http://localhost:16664/open62541/", &((UA_String *)(namespaceArray->value.storage.data.dataPtr))[1]);
	 namespaceArray->arrayDimensionsSize = 1;
	 UA_UInt32 *dimensions = UA_NULL;
	 dimensions = UA_alloc(sizeof(UA_UInt32));
	 *dimensions = 2;
	 namespaceArray->arrayDimensions = dimensions;
	 namespaceArray->dataType = NS0NODEID(UA_STRING_NS0);
	 namespaceArray->valueRank       = 1;
	 namespaceArray->minimumSamplingInterval = 1.0;
	 namespaceArray->historizing     = UA_FALSE;
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&namespaceArray, UA_NODESTORE_INSERT_UNIQUE);

	 // ServerStatus
	 UA_VariableNode *serverstatus;
	 UA_VariableNode_new(&serverstatus);
	 serverstatus->nodeId    = ObjId_ServerStatus.nodeId;
	 serverstatus->nodeClass = UA_NODECLASS_VARIABLE;
	 UA_QualifiedName_copycstring("ServerStatus", &serverstatus->browseName);
	 UA_LocalizedText_copycstring("ServerStatus", &serverstatus->displayName);
	 UA_LocalizedText_copycstring("ServerStatus", &serverstatus->description);
	 UA_ServerStatusDataType *status;
	 UA_ServerStatusDataType_new(&status);
	 status->startTime   = UA_DateTime_now();
	 status->currentTime = UA_DateTime_now();
	 status->state       = UA_SERVERSTATE_RUNNING;
	 UA_String_copycstring("open62541.org", &status->buildInfo.productUri);
	 UA_String_copycstring("open62541", &status->buildInfo.manufacturerName);
	 UA_String_copycstring("open62541", &status->buildInfo.productName);
	 UA_String_copycstring("0.0", &status->buildInfo.softwareVersion);
	 UA_String_copycstring("0.0", &status->buildInfo.buildNumber);
	 status->buildInfo.buildDate     = UA_DateTime_now();
	 status->secondsTillShutdown     = 99999999;
	 UA_LocalizedText_copycstring("because", &status->shutdownReason);
	 serverstatus->value.vt          = &UA_[UA_SERVERSTATUSDATATYPE]; // gets encoded as an extensionobject
	 serverstatus->value.storage.data.arrayLength = 1;
	 serverstatus->value.storage.data.dataPtr        = status;
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&serverstatus, UA_NODESTORE_INSERT_UNIQUE);

	 // State (Component of ServerStatus)
	 UA_VariableNode *state;
	 UA_VariableNode_new(&state);
	 state->nodeId    = ObjId_State.nodeId;
	 state->nodeClass = UA_NODECLASS_VARIABLE;
	 UA_QualifiedName_copycstring("State", &state->browseName);
	 UA_LocalizedText_copycstring("State", &state->displayName);
	 UA_LocalizedText_copycstring("State", &state->description);
	 state->value.vt = &UA_[UA_SERVERSTATE];
	 state->value.storage.data.arrayDimensionsLength = 1; // added to ensure encoding in readreponse
	 state->value.storage.data.arrayLength = 1;
	 state->value.storage.data.dataPtr = &status->state; // points into the other object.
	 state->value.storageType = UA_VARIANT_DATA_NODELETE;
	 UA_NodeStoreExample_insert(server->nodestore, (UA_Node**)&state, UA_NODESTORE_INSERT_UNIQUE);

	 UA_NodeStoreExample_releaseManagedNode((const UA_Node *)root);



	 UA_Namespace *namespace;
	 UA_NamespaceManager_getNamespace(server->namespaceManager,0,&namespace);
	 */

	/*****
	 UA_Namespace *namespace;
	 UA_NamespaceManager_getNamespace(server->namespaceManager,0,&namespace);

	 UA_AddNodesItem addNodesItem;
	 UA_ReferenceTypeAttributes refTypeAttr;


	 addNodesItem.parentNodeId.nodeId = root->nodeId;

	 addNodesItem.requestedNewNodeId.nodeId.identifier.numeric = 999;
	 addNodesItem.requestedNewNodeId.nodeId.namespaceIndex = 0;
	 addNodesItem.requestedNewNodeId.nodeId.identifierType = UA_NODEIDTYPE_NUMERIC;
	 addNodesItem.referenceTypeId = RefTypeId_Organizes.nodeId;
	 addNodesItem.nodeClass = UA_NODECLASS_REFERENCETYPE;

	 UA_QualifiedName_copycstring("myNewNode", &addNodesItem.browseName);
	 UA_LocalizedText_copycstring("myNewNode", &refTypeAttr.displayName);
	 UA_LocalizedText_copycstring("myNewNode", &refTypeAttr.description);
	 refTypeAttr.isAbstract = UA_TRUE;
	 refTypeAttr.symmetric  = UA_TRUE;
	 refTypeAttr.userWriteMask = 0;
	 refTypeAttr.writeMask = 0;

	 refTypeAttr.inverseName.locale.length = 0;
	 refTypeAttr.inverseName.text.length = 0;

	 refTypeAttr.specifiedAttributes |= UA_ATTRIBUTEID_BROWSENAME;
	 refTypeAttr.specifiedAttributes |= UA_ATTRIBUTEID_DISPLAYNAME;
	 refTypeAttr.specifiedAttributes |= UA_ATTRIBUTEID_DESCRIPTION;
	 refTypeAttr.specifiedAttributes |= UA_ATTRIBUTEID_ISABSTRACT;
	 refTypeAttr.specifiedAttributes |= UA_ATTRIBUTEID_SYMMETRIC;
	 refTypeAttr.specifiedAttributes |= UA_ATTRIBUTEID_INVERSENAME;

	 UA_UInt32 offset = 0;

	 UA_ByteString_newMembers(&addNodesItem.nodeAttributes.body,UA_ReferenceTypeAttributes_calcSizeBinary(&refTypeAttr));

	 UA_ReferenceTypeAttributes_encodeBinary(&refTypeAttr,&addNodesItem.nodeAttributes.body,&offset);

	 addSingleNode(namespace,&addNodesItem);
	 */
	/*   UA_ObjectAttributes objAttr;
	 UA_AddNodesItem addNodesItem;
	 addNodesItem.parentNodeId.nodeId = root->nodeId;
	 addNodesItem.requestedNewNodeId.nodeId.identifier.numeric = 222;
	 addNodesItem.requestedNewNodeId.nodeId.namespaceIndex = 0;
	 addNodesItem.requestedNewNodeId.nodeId.identifierType = UA_NODEIDTYPE_NUMERIC;
	 addNodesItem.referenceTypeId = RefTypeId_Organizes.nodeId;
	 addNodesItem.nodeClass = UA_NODECLASS_OBJECT;

	 UA_QualifiedName_copycstring("myNewNodeObject", &addNodesItem.browseName);
	 UA_LocalizedText_copycstring("myNewNodeObject", &objAttr.displayName);
	 UA_LocalizedText_copycstring("myNewNodeObject", &objAttr.description);

	 objAttr.userWriteMask = 0;
	 objAttr.writeMask = 0;

	 objAttr.specifiedAttributes |= UA_ATTRIBUTEID_BROWSENAME;
	 objAttr.specifiedAttributes |= UA_ATTRIBUTEID_DISPLAYNAME;
	 objAttr.specifiedAttributes |= UA_ATTRIBUTEID_DESCRIPTION;
	 UA_UInt32 offset = 0;

	 UA_ByteString_newMembers(&addNodesItem.nodeAttributes.body,UA_ObjectAttributes_calcSizeBinary(&objAttr));

	 UA_ObjectAttributes_encodeBinary(&objAttr, &addNodesItem.nodeAttributes.body,&offset);

	 UA_Namespace *ns0;
	 UA_NamespaceManager_getNamespace(server->namespaceManager,0,&ns0);
	 addSingleNode(ns0,&addNodesItem);
	 */

}

//UA_AddNodesResult UA_Server_addNode(UA_Server *server, UA_Node **node, UA_ExpandedNodeId *parentNodeId,
//                                    UA_NodeId *referenceTypeId) {
//    return AddNode(server, &adminSession, node, parentNodeId, referenceTypeId);
//}

//void UA_Server_addReference(UA_Server *server, const UA_AddReferencesRequest *request,
//                            UA_AddReferencesResponse *response) {
//    Service_AddReferences(server, &adminSession, request, response);
//}

void UA_Server_addScalarVariableNode(UA_Server *server, UA_QualifiedName *browseName, void *value,
                                                  const UA_VTable_Entry *vt, UA_ExpandedNodeId *parentNodeId,
                                                  UA_NodeId *referenceTypeId ) {
    UA_VariableNode *tmpNode;
    UA_VariableNode_new(&tmpNode);
    UA_DataValue *dataValue;
    UA_DataValue_new(&dataValue);

    /*UA_LocalizedText_copycstring("integer value", &tmpNode->description); */
    UA_LocalizedText *displayName;
    UA_LocalizedText *description;

    UA_LocalizedText_new(&displayName);
    UA_LocalizedText_new(&description);
    displayName->locale.length = 0;
    description->locale.length = 0;

    UA_String_copy(&browseName->name, &displayName->text);
    UA_String_copy(&browseName->name, &description->text);

    dataValue->value.vt = vt;
    dataValue->value.storage.data.dataPtr = value;
    dataValue->value.storageType = UA_VARIANT_DATA;
    dataValue->value.storage.data.arrayLength = 1;
    UA_ExpandedNodeId reqNodeId;
    reqNodeId.namespaceUri.length = 0;
    reqNodeId.nodeId.namespaceIndex = 0;
    UA_String_copy(&browseName->name,&reqNodeId.nodeId.identifier.string);
    reqNodeId.nodeId.identifierType = UA_NODEIDTYPE_STRING;
    ns0_addVariableNode(server,*referenceTypeId,reqNodeId, *parentNodeId,*browseName,*displayName,*description,dataValue,-1);
   // return UA_Server_addNode(server, (UA_Node**)&tmpNode, parentNodeId, referenceTypeId);
}

UA_Int32 UA_Server_addNamespace(UA_Server *server, UA_UInt16 namespaceIndex,
		UA_NodeStore *nodeStore) {
	return UA_NamespaceManager_addNamespace(server->namespaceManager,
			namespaceIndex, nodeStore);
}
