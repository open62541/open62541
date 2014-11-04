#include "ua_server_internal.h"
#include "ua_services_internal.h" // AddReferences
#include "ua_namespace_0.h"
#include "ua_securechannel_manager.h"
#include "ua_namespace_manager.h"
#include "ua_session_manager.h"
#include "ua_util.h"
#include "ua_services.h"

void UA_Server_delete(UA_Server *server) {
    UA_ApplicationDescription_deleteMembers(&server->description);
    UA_SecureChannelManager_deleteMembers(&server->secureChannelManager);
    UA_SessionManager_deleteMembers(&server->sessionManager);
    //UA_NodeStore_delete(server->nodestore);
    UA_ByteString_deleteMembers(&server->serverCertificate);
    UA_Array_delete(server->endpointDescriptions, server->endpointDescriptionsSize, &UA_TYPES[UA_ENDPOINTDESCRIPTION]);
    UA_free(server);
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
void ns0_addVariableNode(UA_Server *server, UA_NodeId refTypeNodeId,
		UA_ExpandedNodeId requestedNodeId, UA_ExpandedNodeId parentNodeId,
		UA_QualifiedName browseName, UA_LocalizedText displayName, UA_LocalizedText description,
		UA_DataValue *dataValue, UA_Int32 valueRank) {
	UA_VariableAttributes varAttr;
	UA_AddNodesItem addNodesItem;
	UA_Namespace *ns0 = UA_NULL;
	UA_NamespaceManager_getNamespace(server->namespaceManager, 0, &ns0);
	addNodesItem.parentNodeId = parentNodeId;
	addNodesItem.requestedNewNodeId = requestedNodeId;
	addNodesItem.referenceTypeId = refTypeNodeId;
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

void ADD_REFTYPENODE_NS0(UA_Server *server, UA_NodeId REFTYPE_NODEID,UA_ExpandedNodeId REQ_NODEID,UA_ExpandedNodeId PARENTNODEID,char* REFTYPE_BROWSENAME, char* REFTYPE_DISPLAYNAME, char*REFTYPE_DESCRIPTION,UA_Boolean IS_ABSTRACT,UA_Boolean IS_SYMMETRIC)
{
	UA_AddNodesItem addNodesItem;
    UA_Namespace *ns0;
    UA_NamespaceManager_getNamespace(server->namespaceManager,0,&ns0);
	UA_ReferenceTypeAttributes refTypeAttr;
    addNodesItem.parentNodeId= PARENTNODEID;
    addNodesItem.requestedNewNodeId = REQ_NODEID;
    addNodesItem.referenceTypeId = REFTYPE_NODEID;
    addNodesItem.nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QualifiedName_copycstring(REFTYPE_BROWSENAME, &addNodesItem.browseName);
    UA_LocalizedText_copycstring(REFTYPE_DISPLAYNAME, &refTypeAttr.displayName);
    UA_LocalizedText_copycstring(REFTYPE_DESCRIPTION, &refTypeAttr.description);
    refTypeAttr.isAbstract = IS_ABSTRACT;
    refTypeAttr.symmetric  = IS_SYMMETRIC;
    refTypeAttr.userWriteMask = 0;
    refTypeAttr.writeMask = 0;
    refTypeAttr.inverseName.locale.length = 0;
    refTypeAttr.inverseName.text.length = 0;
    refTypeAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_BROWSENAME;
    refTypeAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_DISPLAYNAME;
    refTypeAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_DESCRIPTION;
    refTypeAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_ISABSTRACT;
    refTypeAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_SYMMETRIC;
    UA_UInt32 offset = 0;
    UA_ByteString_newMembers(&addNodesItem.nodeAttributes.body,UA_ReferenceTypeAttributes_calcSizeBinary(&refTypeAttr));
    UA_ReferenceTypeAttributes_encodeBinary(&refTypeAttr,&addNodesItem.nodeAttributes.body,&offset);
    addSingleNode(ns0,&addNodesItem);
}

UA_Server * UA_Server_new(UA_String *endpointUrl, UA_ByteString *serverCertificate, UA_NodeStore *ns0Nodestore) {
    UA_Server *server = UA_alloc(sizeof(UA_Server));
    if(!server)
        return server;
    //add namespace zero
    UA_NamespaceManager_new(&server->namespaceManager);
    UA_NamespaceManager_addNamespace(server->namespaceManager,0,ns0Nodestore);
    

    // mockup application description
    UA_ApplicationDescription_init(&server->description);
    UA_String_copycstring("urn:servername:open62541:application", &server->description.productUri);
    UA_String_copycstring("http://open62541.info/applications/4711", &server->description.applicationUri);
    UA_LocalizedText_copycstring("The open62541 application", &server->description.applicationName);
    server->description.applicationType = UA_APPLICATIONTYPE_SERVER;

    UA_ByteString_init(&server->serverCertificate);
    if(serverCertificate)
        UA_ByteString_copy(serverCertificate, &server->serverCertificate);

    // mockup endpoint description
    server->endpointDescriptionsSize = 1;
    UA_EndpointDescription *endpoint = UA_alloc(sizeof(UA_EndpointDescription)); // todo: check return code

    endpoint->securityMode = UA_MESSAGESECURITYMODE_NONE;
    UA_String_copycstring("http://opcfoundation.org/UA/SecurityPolicy#None", &endpoint->securityPolicyUri);
    UA_String_copycstring("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary", &endpoint->transportProfileUri);

    endpoint->userIdentityTokensSize = 1;
    endpoint->userIdentityTokens = UA_alloc(sizeof(UA_UserTokenPolicy));
    UA_UserTokenPolicy_init(endpoint->userIdentityTokens);
    UA_String_copycstring("my-anonymous-policy", &endpoint->userIdentityTokens->policyId); // defined per server
    endpoint->userIdentityTokens->tokenType = UA_USERTOKENTYPE_ANONYMOUS;

    UA_String_copy(endpointUrl, &endpoint->endpointUrl);
    /* The standard says "the HostName specified in the Server Certificate is the
       same as the HostName contained in the endpointUrl provided in the
       EndpointDescription */
    UA_String_copy(&server->serverCertificate, &endpoint->serverCertificate);
    UA_ApplicationDescription_copy(&server->description, &endpoint->server);
    server->endpointDescriptions = endpoint;

#define MAXCHANNELCOUNT 100
#define STARTCHANNELID 1
#define TOKENLIFETIME 10000
#define STARTTOKENID 1
    UA_SecureChannelManager_init(&server->secureChannelManager, MAXCHANNELCOUNT,
                                 TOKENLIFETIME, STARTCHANNELID, STARTTOKENID, endpointUrl);


#define MAXSESSIONCOUNT 1000
#define SESSIONLIFETIME 10000
#define STARTSESSIONID 1
    UA_SessionManager_init(&server->sessionManager, MAXSESSIONCOUNT, SESSIONLIFETIME, STARTSESSIONID);

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
	serverArrayValue = UA_DataValue_new();
	UA_UInt32 namespaceArraySize = 2;

	UA_Array_new((void**)&serverArrayValue->value.storage.data.dataPtr, namespaceArraySize, &UA_TYPES[UA_STRING]);
	UA_String_copycstring("http://opcfoundation.org/UA/", &((UA_String *)(serverArrayValue->value.storage.data.dataPtr))[0]);
	UA_Int32 *arrayDim;
	UA_UInt32 arrayDimSize = 1;
	UA_Array_new((void**)&arrayDim, arrayDimSize, &UA_TYPES[UA_INT32]);

	serverArrayValue->value.vt = &UA_TYPES[UA_STRING];
	serverArrayValue->value.storage.data.arrayDimensions = arrayDim;
	serverArrayValue->value.storage.data.arrayDimensionsLength = 1; // added to ensure encoding in readreponse
	serverArrayValue->value.storage.data.arrayLength = 1;
	serverArrayValue->value.storageType = UA_VARIANT_DATA;
	 {
		 UA_QualifiedName *browseName;
		 browseName=UA_QualifiedName_new();
		 UA_LocalizedText *description;
		 description = UA_LocalizedText_new();
		 UA_LocalizedText *displayName;
		 displayName = UA_LocalizedText_new();

		 UA_QualifiedName_copycstring("NamespaceArray",browseName);
		 UA_LocalizedText_copycstring("NamespaceArray",description);
		 UA_LocalizedText_copycstring("NamespaceArray",displayName);

		 ns0_addVariableNode(server, RefTypeId_HasComponent.nodeId,
				 VarId_NamespaceArray,
				 ObjId_Server,
				 *browseName,
				 *description,
				 *displayName,
				 serverArrayValue,1);
	 }



	 // ServerStatus
	 UA_DataValue *serverStatusValue;
	 serverStatusValue = UA_DataValue_new();

	 UA_ServerStatusDataType *status;
	 status = UA_ServerStatusDataType_new();
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
	 serverStatusValue->value.vt          = &UA_TYPES[UA_SERVERSTATUSDATATYPE]; // gets encoded as an extensionobject
	 serverStatusValue->value.storage.data.arrayLength = 0;
	 serverStatusValue->value.storage.data.dataPtr        = status;
	 serverStatusValue->value.storage.data.arrayDimensionsLength = 0;
	 {
		 UA_QualifiedName *browseName = UA_QualifiedName_new();
		 UA_LocalizedText *description = UA_LocalizedText_new();
		 UA_LocalizedText *displayName = UA_LocalizedText_new();

		 UA_QualifiedName_copycstring("ServerStatus",browseName);
		 UA_LocalizedText_copycstring("ServerStatus",description);
		 UA_LocalizedText_copycstring("ServerStatus",displayName);

		 ns0_addVariableNode(server,RefTypeId_HasComponent.nodeId,VarId_ServerStatus,ObjId_Server, *browseName, *description, *displayName ,serverStatusValue,-1);
	}

	 // State (Component of ServerStatus)

	 UA_DataValue *sateValue = UA_DataValue_new();


	 sateValue->value.vt = &UA_TYPES[UA_SERVERSTATE];
	 sateValue->value.storage.data.arrayDimensionsLength = 0; // added to ensure encoding in readreponse
	 sateValue->value.storage.data.arrayLength = 0;
	 sateValue->value.storage.data.dataPtr = &status->state; // points into the other object.
	 sateValue->value.storageType = UA_VARIANT_DATA;

	 {
		 UA_QualifiedName *browseName = UA_QualifiedName_new();
		 UA_LocalizedText *description = UA_LocalizedText_new();
		 UA_LocalizedText *displayName = UA_LocalizedText_new();

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
	ADD_REFTYPENODE_NS0(server,RefTypeId_Organizes.nodeId, RefTypeId_References,
			ObjId_ReferenceTypesFolder, "References", "References",
			"References", UA_TRUE, UA_TRUE);

	ns0_addObjectNode(server,RefTypeId_Organizes.nodeId, ObjId_ViewsFolder, ObjId_Root,
			"Views", "Views", "Views");

	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId,
			RefTypeId_NonHierarchicalReferences, RefTypeId_References,
			"NonHierarchicalReferences", "NonHierarchicalReferences",
			"NonHierarchicalReferences", UA_TRUE, UA_FALSE);

	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_HasModelParent,
			RefTypeId_NonHierarchicalReferences, "HasModelParent",
			"HasModelParent", "HasModelParent", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_GeneratesEvent,
			RefTypeId_NonHierarchicalReferences, "GeneratesEvent",
			"GeneratesEvent", "GeneratesEvent", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_HasEncoding,
			RefTypeId_NonHierarchicalReferences, "HasEncoding", "HasEncoding",
			"HasEncoding", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_HasModellingRule,
			RefTypeId_NonHierarchicalReferences, "HasModellingRule",
			"HasModellingRule", "HasModellingRule", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_HasDescription,
			RefTypeId_NonHierarchicalReferences, "HasDescription",
			"HasDescription", "HasDescription", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId,
			RefTypeId_HasTypeDefinition, RefTypeId_NonHierarchicalReferences,
			"HasTypeDefinition", "HasTypeDefinition", "HasTypeDefinition",
			UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_FromState,
			RefTypeId_NonHierarchicalReferences, "FromState", "FromState",
			"FromState", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_ToState,
			RefTypeId_NonHierarchicalReferences, "ToState", "ToState",
			"ToState", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_HasCause,
			RefTypeId_NonHierarchicalReferences, "HasCause", "HasCause",
			"HasCause", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_HasEffect,
			RefTypeId_NonHierarchicalReferences, "HasEffect", "HasEffect",
			"HasEffect", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId,
			RefTypeId_HasHistoricalConfiguration,
			RefTypeId_NonHierarchicalReferences, "HasHistoricalConfiguration",
			"HasHistoricalConfiguration", "HasHistoricalConfiguration", UA_TRUE,
			UA_FALSE);

	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId,
			RefTypeId_HierarchicalReferences, RefTypeId_References,
			"HierarchicalReferences", "HierarchicalReferences",
			"HierarchicalReferences", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_HasEventSource,
			RefTypeId_HierarchicalReferences, "HasEventSource",
			"HasEventSource", "HasEventSource", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_HasNotifier,
			RefTypeId_HasEventSource, "HasEventSource", "HasEventSource",
			"HasEventSource", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_HasChild,
			RefTypeId_HierarchicalReferences, "HasChild", "HasChild",
			"HasChild", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_Aggregates,
			RefTypeId_HasChild, "Aggregates", "Aggregates", "Aggregates",
			UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_HasProperty,
			RefTypeId_Aggregates, "HasProperty", "HasProperty", "HasProperty",
			UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_HasComponent,
			RefTypeId_Aggregates, "HasComponent", "HasComponent",
			"HasComponent", UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId,
			RefTypeId_HasOrderedComponent, RefTypeId_HasComponent,
			"HasOrderedComponent", "HasOrderedComponent", "HasOrderedComponent",
			UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_HasSubtype,
			RefTypeId_HasChild, "HasSubtype", "HasSubtype", "HasSubtype",
			UA_TRUE, UA_FALSE);
	ADD_REFTYPENODE_NS0(server,RefTypeId_HasSubtype.nodeId, RefTypeId_Organizes,
			RefTypeId_HierarchicalReferences, "Organizes", "Organizes",
			"Organizes", UA_TRUE, UA_FALSE);
    return server;
}


void UA_Server_addScalarVariableNode(UA_Server *server, UA_QualifiedName *browseName, void *value,
                                                  const UA_VTable_Entry *vt, UA_ExpandedNodeId *parentNodeId,
                                                  UA_NodeId *referenceTypeId ) {
    UA_DataValue *dataValue = UA_DataValue_new();

    /*UA_LocalizedText_copycstring("integer value", &tmpNode->description); */
    UA_LocalizedText *displayName = UA_LocalizedText_new();
    UA_LocalizedText *description = UA_LocalizedText_new();

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

}

UA_Int32 UA_Server_addNamespace(UA_Server *server, UA_UInt16 namespaceIndex,
		UA_NodeStore *nodeStore) {

	return (UA_Int32)UA_NamespaceManager_addNamespace(server->namespaceManager,
			namespaceIndex, nodeStore);
}
