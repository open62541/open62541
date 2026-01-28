/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */


#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/accesscontrol.h>
#include "open62541/namespace0_generated.h"
#include "open62541/nodeids.h"
#include "ua_server_internal.h"

#ifdef UA_ENABLE_RBAC

/* OPC UA Part 18 Role-Based Access Control Implementation
 * 
 * Reference: OPC 10000-18: UA Part 18: Role-Based Security
 * https://reference.opcfoundation.org/Core/Part18/v105/docs/
 * 
 * This file implements the RBAC NS0 integration:
 * - Role object creation and deletion in namespace 0
 * - DataSource callbacks for role properties (Identities, Applications, Endpoints)
 * - Method callbacks for role management operations
 */

#ifdef UA_ENABLE_RBAC_INFORMATIONMODEL

static UA_StatusCode
readRoleIdentities(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                   const UA_NodeId *nodeId, void *nodeContext,
                   UA_Boolean includeSourceTimeStamp, const UA_NumericRange *range,
                   UA_DataValue *value) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    
    UA_NodeId roleId = *(UA_NodeId*)nodeContext;
    const UA_Role *role = UA_Server_getRoleById(server, roleId);
    if(!role)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    
    UA_Variant_setArrayCopy(&value->value, role->imrt, role->imrtSize,
                           &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE]);
    value->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readRoleApplications(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                     const UA_NodeId *nodeId, void *nodeContext,
                     UA_Boolean includeSourceTimeStamp, const UA_NumericRange *range,
                     UA_DataValue *value) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    
    UA_NodeId roleId = *(UA_NodeId*)nodeContext;
    const UA_Role *role = UA_Server_getRoleById(server, roleId);
    if(!role)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    
    UA_Variant_setArrayCopy(&value->value, role->applications, role->applicationsSize,
                           &UA_TYPES[UA_TYPES_STRING]);
    value->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readRoleEndpoints(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                  const UA_NodeId *nodeId, void *nodeContext,
                  UA_Boolean includeSourceTimeStamp, const UA_NumericRange *range,
                  UA_DataValue *value) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    
    UA_NodeId roleId = *(UA_NodeId*)nodeContext;
    const UA_Role *role = UA_Server_getRoleById(server, roleId);
    if(!role)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    
    UA_Variant_setArrayCopy(&value->value, role->endpoints, role->endpointsSize,
                           &UA_TYPES[UA_TYPES_ENDPOINTTYPE]);
    value->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

/* Add Role object to NS0 
 * Note: role->roleId MUST already be set by caller (UA_Server_addRole)
 * This function creates the NS0 information model representation and sets DataSources 
 * 
 * RoleType has only ONE mandatory property: Identities (ModellingRule=Mandatory)
 * All other properties (Applications, Endpoints, etc.) are Optional and must be added manually */
UA_StatusCode
addRoleRepresentation(UA_Server *server, UA_Role *role) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    
    if(!server || !role)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    /* role->roleId MUST be valid at this point */
    if(UA_NodeId_isNull(&role->roleId))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    

    
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    
    /* Add Role object instance using the pre-assigned roleId
     * Note: When instantiating RoleType, only MANDATORY properties are created automatically
     * In this case, only "Identities" is mandatory (ModellingRule i=78) */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName.locale = UA_STRING("en-US");
    oAttr.displayName.text = role->roleName.name;
    oAttr.description = UA_LOCALIZEDTEXT("en-US", "");
    
    res = UA_Server_addObjectNode(server, role->roleId,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                  role->roleName,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE),
                                  oAttr, NULL, NULL);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    
    /* Find and configure DataSource for Identities property (created by RoleType instantiation) */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = role->roleId;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    bd.includeSubtypes = false;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.nodeClassMask = UA_NODECLASS_VARIABLE;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    
    UA_BrowseResult br = UA_Server_browse(server, 100, &bd);
    
    UA_NodeId identitiesNodeId = UA_NODEID_NULL;
    UA_String identitiesStr = UA_STRING("Identities");
    for(size_t i = 0; i < br.referencesSize; i++) {
        if(UA_String_equal(&br.references[i].browseName.name, &identitiesStr)) {
            UA_NodeId_copy(&br.references[i].nodeId.nodeId, &identitiesNodeId);
            break;
        }
    }
    UA_BrowseResult_clear(&br);
    
    if(UA_NodeId_isNull(&identitiesNodeId)) {
        UA_Server_deleteNode(server, role->roleId, true);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    
    UA_NodeId *roleIdContext = UA_NodeId_new();
    if(!roleIdContext) {
        UA_NodeId_clear(&identitiesNodeId);
        UA_Server_deleteNode(server, role->roleId, true);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_NodeId_copy(&role->roleId, roleIdContext);
    
    UA_DataSource identitiesDataSource;
    identitiesDataSource.read = readRoleIdentities;
    identitiesDataSource.write = NULL;
    
    res = UA_Server_setVariableNode_dataSource(server, identitiesNodeId, identitiesDataSource);
    res |= UA_Server_setNodeContext(server, identitiesNodeId, roleIdContext);
    UA_NodeId_clear(&identitiesNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_delete(roleIdContext);
        UA_Server_deleteNode(server, role->roleId, true);
        return res;
    }
    
    /* Add optional Applications property with DataSource (not mandatory in RoleType) */
    UA_NodeId *roleIdContext2 = UA_NodeId_new();
    if(!roleIdContext2) {
        UA_Server_deleteNode(server, role->roleId, true);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_NodeId_copy(&role->roleId, roleIdContext2);
    
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Applications");
    vAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    vAttr.valueRank = UA_VALUERANK_ONE_OR_MORE_DIMENSIONS;
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ;
    
    UA_DataSource applicationsDataSource;
    applicationsDataSource.read = readRoleApplications;
    applicationsDataSource.write = NULL;
    
    UA_NodeId applicationsNodeId;
    res = UA_Server_addDataSourceVariableNode(server, UA_NODEID_NULL,
                                              role->roleId,
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                              UA_QUALIFIEDNAME(0, "Applications"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),
                                              vAttr, applicationsDataSource,
                                              roleIdContext2, &applicationsNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_delete(roleIdContext2);
        UA_Server_deleteNode(server, role->roleId, true);
        return res;
    }
    
    /* Add optional Endpoints property with DataSource (not mandatory in RoleType) */
    UA_NodeId *roleIdContext3 = UA_NodeId_new();
    if(!roleIdContext3) {
        UA_Server_deleteNode(server, role->roleId, true);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_NodeId_copy(&role->roleId, roleIdContext3);
    
    vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Endpoints");
    vAttr.dataType = UA_TYPES[UA_TYPES_ENDPOINTTYPE].typeId;
    vAttr.valueRank = UA_VALUERANK_ONE_OR_MORE_DIMENSIONS;
    
    UA_DataSource endpointsDataSource;
    endpointsDataSource.read = readRoleEndpoints;
    endpointsDataSource.write = NULL;
    
    UA_NodeId endpointsNodeId;
    res = UA_Server_addDataSourceVariableNode(server, UA_NODEID_NULL,
                                              role->roleId,
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                              UA_QUALIFIEDNAME(0, "Endpoints"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),
                                              vAttr, endpointsDataSource,
                                              roleIdContext3, &endpointsNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_delete(roleIdContext3);
        UA_Server_deleteNode(server, role->roleId, true);
        return res;
    }
    
    return UA_STATUSCODE_GOOD;
}

/* Remove Role object from NS0 */
UA_StatusCode
removeRoleRepresentation(UA_Server *server, const UA_NodeId *roleId) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    
    if(!server || !roleId)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    return UA_Server_deleteNode(server, *roleId, true);
}

#endif /* UA_ENABLE_RBAC_INFORMATIONMODEL */

/* Method callbacks */

static UA_StatusCode
addRoleMethodCallback(UA_Server *server,
                      const UA_NodeId *objectId, void *objectContext,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *inputType, void *inputContext,
                      size_t inputSize, const UA_Variant *input,
                      size_t outputSize, UA_Variant *output) {
    if(inputSize != 2 ||
       input[0].type != &UA_TYPES[UA_TYPES_STRING] ||
       input[1].type != &UA_TYPES[UA_TYPES_STRING])
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    UA_String *roleName = (UA_String*)input[0].data;
    UA_String *namespaceUri = (UA_String*)input[1].data;
    
    UA_Role role;
    UA_Role_init(&role);
    role.customConfiguration = true;
    UA_QualifiedName_init(&role.roleName);
    UA_String_copy(roleName, &role.roleName.name);
    
    UA_NodeId newRoleId = UA_NODEID_NULL;
    UA_StatusCode retval = UA_Server_addRole(server, *roleName, *namespaceUri, &role, &newRoleId);
    UA_Role_clear(&role);
    
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    
    if(outputSize >= 1)
        UA_Variant_setScalarCopy(&output[0], &newRoleId, &UA_TYPES[UA_TYPES_NODEID]);
    
    UA_NodeId_clear(&newRoleId);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeRoleMethodCallback(UA_Server *server,
                         const UA_NodeId *objectId, void *objectContext,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *inputType, void *inputContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    if(inputSize != 1 || input[0].type != &UA_TYPES[UA_TYPES_NODEID])
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    return UA_Server_removeRole(server, *(UA_NodeId*)input[0].data);
}

static UA_StatusCode
addIdentityMethodCallback(UA_Server *server,
                         const UA_NodeId *objectId, void *objectContext,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *inputType, void *inputContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    if(inputSize != 1 || input[0].type != &UA_TYPES[UA_TYPES_EXTENSIONOBJECT])
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    UA_ExtensionObject *extObj = (UA_ExtensionObject*)input[0].data;
    if(!extObj->content.decoded.data || 
       extObj->content.decoded.type != &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE])
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    UA_IdentityMappingRuleType *rule = (UA_IdentityMappingRuleType*)extObj->content.decoded.data;
    return UA_Server_addRoleIdentity(server, *objectId, rule->criteriaType, rule->criteria);
}

static UA_StatusCode
removeIdentityMethodCallback(UA_Server *server,
                            const UA_NodeId *objectId, void *objectContext,
                            const UA_NodeId *methodId, void *methodContext,
                            const UA_NodeId *inputType, void *inputContext,
                            size_t inputSize, const UA_Variant *input,
                            size_t outputSize, UA_Variant *output) {
    if(inputSize != 1 || input[0].type != &UA_TYPES[UA_TYPES_EXTENSIONOBJECT])
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    UA_ExtensionObject *extObj = (UA_ExtensionObject*)input[0].data;
    if(!extObj->content.decoded.data || 
       extObj->content.decoded.type != &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE])
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    UA_IdentityMappingRuleType *rule = (UA_IdentityMappingRuleType*)extObj->content.decoded.data;
    return UA_Server_removeRoleIdentity(server, *objectId, rule->criteriaType);
}

static UA_StatusCode
addApplicationMethodCallback(UA_Server *server,
                            const UA_NodeId *objectId, void *objectContext,
                            const UA_NodeId *methodId, void *methodContext,
                            const UA_NodeId *inputType, void *inputContext,
                            size_t inputSize, const UA_Variant *input,
                            size_t outputSize, UA_Variant *output) {
    if(inputSize != 1 || input[0].type != &UA_TYPES[UA_TYPES_STRING])
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    return UA_Server_addRoleApplication(server, *objectId, *(UA_String*)input[0].data);
}

static UA_StatusCode
removeApplicationMethodCallback(UA_Server *server,
                               const UA_NodeId *objectId, void *objectContext,
                               const UA_NodeId *methodId, void *methodContext,
                               const UA_NodeId *inputType, void *inputContext,
                               size_t inputSize, const UA_Variant *input,
                               size_t outputSize, UA_Variant *output) {
    if(inputSize != 1 || input[0].type != &UA_TYPES[UA_TYPES_STRING])
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    return UA_Server_removeRoleApplication(server, *objectId, *(UA_String*)input[0].data);
}

static UA_StatusCode
addEndpointMethodCallback(UA_Server *server,
                         const UA_NodeId *objectId, void *objectContext,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *inputType, void *inputContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    if(inputSize != 1 || input[0].type != &UA_TYPES[UA_TYPES_EXTENSIONOBJECT])
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    UA_ExtensionObject *extObj = (UA_ExtensionObject*)input[0].data;
    if(!extObj->content.decoded.data || 
       extObj->content.decoded.type != &UA_TYPES[UA_TYPES_ENDPOINTTYPE])
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    return UA_Server_addRoleEndpoint(server, *objectId, *(UA_EndpointType*)extObj->content.decoded.data);
}

static UA_StatusCode
removeEndpointMethodCallback(UA_Server *server,
                            const UA_NodeId *objectId, void *objectContext,
                            const UA_NodeId *methodId, void *methodContext,
                            const UA_NodeId *inputType, void *inputContext,
                            size_t inputSize, const UA_Variant *input,
                            size_t outputSize, UA_Variant *output) {
    if(inputSize != 1 || input[0].type != &UA_TYPES[UA_TYPES_EXTENSIONOBJECT])
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    UA_ExtensionObject *extObj = (UA_ExtensionObject*)input[0].data;
    if(!extObj->content.decoded.data || 
       extObj->content.decoded.type != &UA_TYPES[UA_TYPES_ENDPOINTTYPE])
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    return UA_Server_removeRoleEndpoint(server, *objectId, *(UA_EndpointType*)extObj->content.decoded.data);
}

/* Namespace DefaultRolePermissions Property Callback */

static UA_StatusCode
readNamespaceDefaultPermissions(UA_Server *server,
                               const UA_NodeId *sessionId, void *sessionContext,
                               const UA_NodeId *nodeId, void *nodeContext,
                               UA_Boolean includeSourceTimeStamp,
                               const UA_NumericRange *range,
                               UA_DataValue *value) {
    /* The namespace index is stored in the node context */
    if(!nodeContext) {
        UA_Variant_setArray(&value->value, NULL, 0,
                           &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
        value->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    
    UA_UInt16 nsIdx = *(UA_UInt16*)nodeContext;
    
    /* Validate namespace index */
    if(!server->namespaceMetadata || nsIdx >= server->namespaceMetadataSize) {
        UA_Variant_setArray(&value->value, NULL, 0,
                           &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
        value->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }

    /* Get the entries directly from namespace metadata */
    size_t entriesSize = server->namespaceMetadata[nsIdx].entriesSize;
    const UA_RolePermissionEntry *entries = server->namespaceMetadata[nsIdx].entries;

    if(!entries || entriesSize == 0) {
        UA_Variant_setArray(&value->value, NULL, 0, 
                           &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
        value->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }

    /* Convert to RolePermissionType array */
    UA_RolePermissionType *perms = (UA_RolePermissionType*)
        UA_Array_new(entriesSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    if(!perms)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    for(size_t i = 0; i < entriesSize; i++) {
        UA_RolePermissionType_init(&perms[i]);
        UA_NodeId_copy(&entries[i].roleId, &perms[i].roleId);
        perms[i].permissions = entries[i].permissions;
    }

    UA_Variant_setArray(&value->value, perms, entriesSize,
                       &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    value->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setNamespaceDefaultPermissionsProperty(UA_Server *server,
                                                 UA_UInt16 namespaceIndex) {
    if(!server || namespaceIndex >= server->namespacesSize)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#ifdef UA_GENERATED_NAMESPACE_ZERO
    /* Find the namespace object under Server.Namespaces (NodeId 11715).
     * Only namespaces with NamespaceMetadataType objects will have the
     * DefaultRolePermissions property defined by the spec. */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NS0ID(SERVER_NAMESPACES); /* 11715 */
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NS0ID(HASCOMPONENT);
    bd.includeSubtypes = true;
    bd.nodeClassMask = UA_NODECLASS_OBJECT;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    if(br.statusCode != UA_STATUSCODE_GOOD) {
        UA_BrowseResult_clear(&br);
        return br.statusCode;
    }

    /* Find the namespace object by checking NamespaceUri property */
    UA_NodeId namespaceObjId = UA_NODEID_NULL;
    for(size_t i = 0; i < br.referencesSize; i++) {
        /* Read the NamespaceUri property of this object */
        UA_QualifiedName namespaceUriName = UA_QUALIFIEDNAME(0, "NamespaceUri");
        UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(
            server, br.references[i].nodeId.nodeId, 1, &namespaceUriName);
        
        if(bpr.statusCode == UA_STATUSCODE_GOOD && bpr.targetsSize > 0) {
            UA_Variant val;
            UA_Variant_init(&val);
            UA_StatusCode readRes = UA_Server_readValue(server, 
                                                        bpr.targets[0].targetId.nodeId,
                                                        &val);
            
            if(readRes == UA_STATUSCODE_GOOD && val.type == &UA_TYPES[UA_TYPES_STRING]) {
                UA_String *nsUri = (UA_String*)val.data;
                if(UA_String_equal(nsUri, &server->namespaces[namespaceIndex])) {
                    UA_NodeId_copy(&br.references[i].nodeId.nodeId, &namespaceObjId);
                    UA_Variant_clear(&val);
                    UA_BrowsePathResult_clear(&bpr);
                    break;
                }
            }
            UA_Variant_clear(&val);
        }
        UA_BrowsePathResult_clear(&bpr);
    }
    
    UA_BrowseResult_clear(&br);

    if(UA_NodeId_isNull(&namespaceObjId))
        return UA_STATUSCODE_BADNOTFOUND;

    /* Find the DefaultRolePermissions property (defined by NamespaceMetadataType) */
    UA_QualifiedName defaultRolePerm = UA_QUALIFIEDNAME(0, "DefaultRolePermissions");
    UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(
        server, namespaceObjId, 1, &defaultRolePerm);
    
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize == 0) {
        UA_NodeId_clear(&namespaceObjId);
        UA_BrowsePathResult_clear(&bpr);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    
    UA_NodeId propNodeId;
    UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, &propNodeId);
    UA_BrowsePathResult_clear(&bpr);

    /* Store namespace index in node context to avoid recursion */
    UA_UInt16 *nsContext = (UA_UInt16*)UA_malloc(sizeof(UA_UInt16));
    if(!nsContext) {
        UA_NodeId_clear(&namespaceObjId);
        UA_NodeId_clear(&propNodeId);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    *nsContext = namespaceIndex;
    
    UA_StatusCode res = UA_Server_setNodeContext(server, propNodeId, nsContext);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(nsContext);
        UA_NodeId_clear(&namespaceObjId);
        UA_NodeId_clear(&propNodeId);
        return res;
    }

    /* Set up the data source */
    UA_CallbackValueSource dataSource;
    dataSource.read = readNamespaceDefaultPermissions;
    dataSource.write = NULL;
    
    res = UA_Server_setVariableNode_callbackValueSource(
        server, propNodeId, dataSource);
    
    UA_NodeId_clear(&namespaceObjId);
    UA_NodeId_clear(&propNodeId);
    
    return res;
#else
    return UA_STATUSCODE_BADNOTSUPPORTED;
#endif
}

UA_StatusCode
initNS0RBAC(UA_Server *server) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_ADDROLE), addRoleMethodCallback);
    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_REMOVEROLE), removeRoleMethodCallback);

    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDIDENTITY), addIdentityMethodCallback);
    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_REMOVEIDENTITY), removeIdentityMethodCallback);

    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDAPPLICATION), addApplicationMethodCallback);
    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_REMOVEAPPLICATION), removeApplicationMethodCallback);

    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDENDPOINT), addEndpointMethodCallback);
    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_REMOVEENDPOINT), removeEndpointMethodCallback);

    return retval;
}

#endif /* UA_ENABLE_RBAC */
