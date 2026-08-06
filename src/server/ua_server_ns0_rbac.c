/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025-2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/server.h>
#include <open62541/nodeids.h>
#include <open62541/plugin/accesscontrol.h>
#include "ua_server_internal.h"

#ifdef UA_ENABLE_RBAC

#include "ua_server_rbac.h"

/* RBAC NS0 information model integration.
 * Known RBAC limitations are documented in ua_server_rbac.c. */

/* Resolve the Role Object owning a property (inverse HasProperty), so the data
 * source callbacks need no per-node context to release on node deletion. */
static UA_StatusCode
getRoleIdOfProperty(UA_Server *server, const UA_NodeId *propertyId,
                    UA_NodeId *roleId) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = *propertyId;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    bd.includeSubtypes = false;
    bd.browseDirection = UA_BROWSEDIRECTION_INVERSE;
    bd.nodeClassMask = UA_NODECLASS_OBJECT;
    bd.resultMask = UA_BROWSERESULTMASK_NONE;

    UA_BrowseResult br = UA_Server_browse(server, 1, &bd);
    UA_StatusCode res = br.statusCode;
    if(res == UA_STATUSCODE_GOOD) {
        if(br.referencesSize > 0)
            res = UA_NodeId_copy(&br.references[0].nodeId.nodeId, roleId);
        else
            res = UA_STATUSCODE_BADNOTFOUND;
    }
    UA_BrowseResult_clear(&br);
    return res;
}

/* Find the Variable child with the given BrowseName name */
static UA_StatusCode
findPropertyChild(UA_Server *server, const UA_NodeId parentId,
                  const char *name, UA_NodeId *childId) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = parentId;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    bd.includeSubtypes = false;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.nodeClassMask = UA_NODECLASS_VARIABLE;
    bd.resultMask = UA_BROWSERESULTMASK_BROWSENAME;

    UA_BrowseResult br = UA_Server_browse(server, 100, &bd);
    UA_StatusCode res = br.statusCode;
    if(res == UA_STATUSCODE_GOOD) {
        res = UA_STATUSCODE_BADNOTFOUND;
        UA_String nameStr = UA_STRING((char*)(uintptr_t)name);
        for(size_t i = 0; i < br.referencesSize; i++) {
            if(UA_String_equal(&br.references[i].browseName.name, &nameStr)) {
                res = UA_NodeId_copy(&br.references[i].nodeId.nodeId, childId);
                break;
            }
        }
    }
    UA_BrowseResult_clear(&br);
    return res;
}

static UA_StatusCode
findMethodChild(UA_Server *server, const UA_NodeId parentId,
                const char *name, UA_NodeId *childId) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = parentId;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    bd.includeSubtypes = false;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.nodeClassMask = UA_NODECLASS_METHOD;
    bd.resultMask = UA_BROWSERESULTMASK_BROWSENAME;

    UA_BrowseResult br = UA_Server_browse(server, 100, &bd);
    UA_StatusCode res = br.statusCode;
    if(res == UA_STATUSCODE_GOOD) {
        res = UA_STATUSCODE_BADNOTFOUND;
        UA_String nameStr = UA_STRING((char*)(uintptr_t)name);
        for(size_t i = 0; i < br.referencesSize; i++) {
            if(UA_String_equal(&br.references[i].browseName.name, &nameStr)) {
                res = UA_NodeId_copy(&br.references[i].nodeId.nodeId, childId);
                break;
            }
        }
    }
    UA_BrowseResult_clear(&br);
    return res;
}

static UA_StatusCode
ensureRoleTypeMethods(UA_Server *server, const UA_NodeId *roleId,
                      UA_Boolean applyPermissions);

static UA_StatusCode
readRoleIdentities(UA_Server *server, const UA_NodeId *sessionId,
                   void *sessionContext,
                   const UA_NodeId *nodeId, void *nodeContext,
                   UA_Boolean includeSourceTimeStamp,
                   const UA_NumericRange *range,
                   UA_DataValue *value) {
    UA_NodeId roleId;
    UA_StatusCode res = getRoleIdOfProperty(server, nodeId, &roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Role role;
    res = UA_Server_getRoleById(server, roleId, &role);
    UA_NodeId_clear(&roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Variant_setArrayCopy(&value->value, role.identityMappingRules,
                            role.identityMappingRulesSize,
                            &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE]);
    value->hasValue = true;
    UA_Role_clear(&role);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readRoleApplications(UA_Server *server, const UA_NodeId *sessionId,
                     void *sessionContext,
                     const UA_NodeId *nodeId, void *nodeContext,
                     UA_Boolean includeSourceTimeStamp,
                     const UA_NumericRange *range,
                     UA_DataValue *value) {
    UA_NodeId roleId;
    UA_StatusCode res = getRoleIdOfProperty(server, nodeId, &roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Role role;
    res = UA_Server_getRoleById(server, roleId, &role);
    UA_NodeId_clear(&roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Variant_setArrayCopy(&value->value, role.applications,
                            role.applicationsSize,
                            &UA_TYPES[UA_TYPES_STRING]);
    value->hasValue = true;
    UA_Role_clear(&role);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readRoleEndpoints(UA_Server *server, const UA_NodeId *sessionId,
                  void *sessionContext,
                  const UA_NodeId *nodeId, void *nodeContext,
                  UA_Boolean includeSourceTimeStamp,
                  const UA_NumericRange *range,
                  UA_DataValue *value) {
    UA_NodeId roleId;
    UA_StatusCode res = getRoleIdOfProperty(server, nodeId, &roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Role role;
    res = UA_Server_getRoleById(server, roleId, &role);
    UA_NodeId_clear(&roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Variant_setArrayCopy(&value->value, role.endpoints,
                            role.endpointsSize,
                            &UA_TYPES[UA_TYPES_ENDPOINTTYPE]);
    value->hasValue = true;
    UA_Role_clear(&role);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readRoleApplicationsExclude(UA_Server *server, const UA_NodeId *sessionId,
                            void *sessionContext,
                            const UA_NodeId *nodeId, void *nodeContext,
                            UA_Boolean includeSourceTimeStamp,
                            const UA_NumericRange *range,
                            UA_DataValue *value) {
    UA_NodeId roleId;
    UA_StatusCode res = getRoleIdOfProperty(server, nodeId, &roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Role role;
    res = UA_Server_getRoleById(server, roleId, &role);
    UA_NodeId_clear(&roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Variant_setScalarCopy(&value->value, &role.applicationsExclude,
                             &UA_TYPES[UA_TYPES_BOOLEAN]);
    value->hasValue = true;
    UA_Role_clear(&role);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeRoleApplicationsExclude(UA_Server *server, const UA_NodeId *sessionId,
                             void *sessionContext,
                             const UA_NodeId *nodeId, void *nodeContext,
                             const UA_NumericRange *range,
                             const UA_DataValue *value) {
    if(range)
        return UA_STATUSCODE_BADINDEXRANGEINVALID;
    if(!value || !value->hasValue ||
       value->value.type != &UA_TYPES[UA_TYPES_BOOLEAN] ||
       !UA_Variant_isScalar(&value->value))
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_NodeId roleId;
    UA_StatusCode res = getRoleIdOfProperty(server, nodeId, &roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Role role;
    res = UA_Server_getRoleById(server, roleId, &role);
    UA_NodeId_clear(&roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    role.applicationsExclude = *(UA_Boolean*)value->value.data;
    res = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    return res;
}

static UA_StatusCode
readRoleEndpointsExclude(UA_Server *server, const UA_NodeId *sessionId,
                         void *sessionContext,
                         const UA_NodeId *nodeId, void *nodeContext,
                         UA_Boolean includeSourceTimeStamp,
                         const UA_NumericRange *range,
                         UA_DataValue *value) {
    UA_NodeId roleId;
    UA_StatusCode res = getRoleIdOfProperty(server, nodeId, &roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Role role;
    res = UA_Server_getRoleById(server, roleId, &role);
    UA_NodeId_clear(&roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Variant_setScalarCopy(&value->value, &role.endpointsExclude,
                             &UA_TYPES[UA_TYPES_BOOLEAN]);
    value->hasValue = true;
    UA_Role_clear(&role);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeRoleEndpointsExclude(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext,
                          const UA_NodeId *nodeId, void *nodeContext,
                          const UA_NumericRange *range,
                          const UA_DataValue *value) {
    if(range)
        return UA_STATUSCODE_BADINDEXRANGEINVALID;
    if(!value || !value->hasValue ||
       value->value.type != &UA_TYPES[UA_TYPES_BOOLEAN] ||
       !UA_Variant_isScalar(&value->value))
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_NodeId roleId;
    UA_StatusCode res = getRoleIdOfProperty(server, nodeId, &roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Role role;
    res = UA_Server_getRoleById(server, roleId, &role);
    UA_NodeId_clear(&roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    role.endpointsExclude = *(UA_Boolean*)value->value.data;
    res = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    return res;
}

static UA_StatusCode
readRoleCustomConfiguration(UA_Server *server, const UA_NodeId *sessionId,
                            void *sessionContext,
                            const UA_NodeId *nodeId, void *nodeContext,
                            UA_Boolean includeSourceTimeStamp,
                            const UA_NumericRange *range,
                            UA_DataValue *value) {
    UA_NodeId roleId;
    UA_StatusCode res = getRoleIdOfProperty(server, nodeId, &roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Role role;
    res = UA_Server_getRoleById(server, roleId, &role);
    UA_NodeId_clear(&roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Variant_setScalarCopy(&value->value, &role.customConfiguration,
                             &UA_TYPES[UA_TYPES_BOOLEAN]);
    value->hasValue = true;
    UA_Role_clear(&role);
    return UA_STATUSCODE_GOOD;
}

/* Add Role object to NS0. The role->roleId must already be set by the
 * caller. Identities is mandatory, Applications and Endpoints are added
 * as optional properties with DataSources. */
UA_StatusCode
addRoleRepresentation(UA_Server *server, UA_Role *role) {
    if(!server || !role)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(UA_NodeId_isNull(&role->roleId))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode res = UA_STATUSCODE_GOOD;

    /* Add Role object instance using the pre-assigned roleId */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName.locale = UA_STRING("en-US");
    oAttr.displayName.text = role->roleName.name;
    oAttr.description = UA_LOCALIZEDTEXT("en-US", "");

    res = UA_Server_addObjectNode(server, role->roleId,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  role->roleName,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE),
                                  oAttr, NULL, NULL);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Back the mandatory Identities property with the role registry */
    UA_NodeId identitiesNodeId;
    res = findPropertyChild(server, role->roleId, "Identities", &identitiesNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Server_deleteNode(server, role->roleId, true);
        return res;
    }

    UA_DataSource identitiesDataSource;
    identitiesDataSource.read = readRoleIdentities;
    identitiesDataSource.write = NULL;

    res = UA_Server_setVariableNode_dataSource(server, identitiesNodeId,
                                               identitiesDataSource);
    UA_NodeId_clear(&identitiesNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Server_deleteNode(server, role->roleId, true);
        return res;
    }

    /* Add optional Applications property with DataSource */
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Applications");
    vAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    vAttr.valueRank = UA_VALUERANK_ONE_OR_MORE_DIMENSIONS;
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ;

    UA_DataSource applicationsDataSource;
    applicationsDataSource.read = readRoleApplications;
    applicationsDataSource.write = NULL;

    res = UA_Server_addDataSourceVariableNode(server, UA_NODEID_NULL,
                                              role->roleId,
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                              UA_QUALIFIEDNAME(0, "Applications"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),
                                              vAttr, applicationsDataSource,
                                              NULL, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Server_deleteNode(server, role->roleId, true);
        return res;
    }

    /* Add optional ApplicationsExclude property with DataSource */
    vAttr = UA_VariableAttributes_default;
    vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ApplicationsExclude");
    vAttr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    vAttr.valueRank = UA_VALUERANK_SCALAR;
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_DataSource applicationsExcludeDataSource;
    applicationsExcludeDataSource.read = readRoleApplicationsExclude;
    applicationsExcludeDataSource.write = writeRoleApplicationsExclude;

    res = UA_Server_addDataSourceVariableNode(server, UA_NODEID_NULL,
                                              role->roleId,
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                              UA_QUALIFIEDNAME(0, "ApplicationsExclude"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),
                                              vAttr, applicationsExcludeDataSource,
                                              NULL, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Server_deleteNode(server, role->roleId, true);
        return res;
    }

    /* Add optional Endpoints property with DataSource */
    vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Endpoints");
    vAttr.dataType = UA_TYPES[UA_TYPES_ENDPOINTTYPE].typeId;
    vAttr.valueRank = UA_VALUERANK_ONE_OR_MORE_DIMENSIONS;
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ;

    UA_DataSource endpointsDataSource;
    endpointsDataSource.read = readRoleEndpoints;
    endpointsDataSource.write = NULL;

    res = UA_Server_addDataSourceVariableNode(server, UA_NODEID_NULL,
                                              role->roleId,
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                              UA_QUALIFIEDNAME(0, "Endpoints"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),
                                              vAttr, endpointsDataSource,
                                              NULL, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Server_deleteNode(server, role->roleId, true);
        return res;
    }

    /* Add optional EndpointsExclude property with DataSource */
    vAttr = UA_VariableAttributes_default;
    vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "EndpointsExclude");
    vAttr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    vAttr.valueRank = UA_VALUERANK_SCALAR;
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_DataSource endpointsExcludeDataSource;
    endpointsExcludeDataSource.read = readRoleEndpointsExclude;
    endpointsExcludeDataSource.write = writeRoleEndpointsExclude;

    res = UA_Server_addDataSourceVariableNode(server, UA_NODEID_NULL,
                                              role->roleId,
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                              UA_QUALIFIEDNAME(0, "EndpointsExclude"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),
                                              vAttr, endpointsExcludeDataSource,
                                              NULL, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Server_deleteNode(server, role->roleId, true);
        return res;
    }

    /* Add optional CustomConfiguration property with DataSource (Part 18 §4.4.1).
     * Boolean scalar; read-only. */
    vAttr = UA_VariableAttributes_default;
    vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "CustomConfiguration");
    vAttr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    vAttr.valueRank = UA_VALUERANK_SCALAR;
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ;

    UA_DataSource customConfigDataSource;
    customConfigDataSource.read = readRoleCustomConfiguration;
    customConfigDataSource.write = NULL;

    res = UA_Server_addDataSourceVariableNode(server, UA_NODEID_NULL,
                                              role->roleId,
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                              UA_QUALIFIEDNAME(0, "CustomConfiguration"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),
                                              vAttr, customConfigDataSource,
                                              NULL, NULL);
    if(res != UA_STATUSCODE_GOOD)
        UA_Server_deleteNode(server, role->roleId, true);
    if(res == UA_STATUSCODE_GOOD) {
        res = ensureRoleTypeMethods(server, &role->roleId, true);
        if(res != UA_STATUSCODE_GOOD)
            UA_Server_deleteNode(server, role->roleId, true);
    }
    return res;
}

/* Remove Role object from NS0 */
UA_StatusCode
removeRoleRepresentation(UA_Server *server, const UA_NodeId *roleId) {
    if(!server || !roleId)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_Server_deleteNode(server, *roleId, true);
}

/* Method callbacks */

static UA_StatusCode
addRoleMethodCallback(UA_Server *server,
                      const UA_NodeId *sessionId, void *sessionContext,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *objectId, void *objectContext,
                      size_t inputSize, const UA_Variant *input,
                      size_t outputSize, UA_Variant *output) {
    UA_StatusCode res = checkMethodOutputArguments(outputSize, 1);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_StatusCode access = checkRBACMethodAccess(server, sessionId);
    if(access != UA_STATUSCODE_GOOD)
        return access;
    if(inputSize != 2 ||
       input[0].type != &UA_TYPES[UA_TYPES_STRING] ||
       input[1].type != &UA_TYPES[UA_TYPES_STRING])
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_String *roleName = (UA_String*)input[0].data;
    UA_String *namespaceUri = (UA_String*)input[1].data;

    UA_Role role;
    UA_Role_init(&role);
    UA_String_copy(roleName, &role.roleName.name);

    /* Per specification, use NS1 if no namespaceUri is given */
    if(namespaceUri->length > 0) {
        size_t nsIdx = 0;
        res = UA_Server_getNamespaceByName(server, *namespaceUri, &nsIdx);
        if(res != UA_STATUSCODE_GOOD) {
            UA_Role_clear(&role);
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        }
        role.roleName.namespaceIndex = (UA_UInt16)nsIdx;
    } else {
        role.roleName.namespaceIndex = 1;
    }

    UA_NodeId newRoleId = UA_NODEID_NULL;
    UA_StatusCode retval = UA_Server_addRole(server, &role, &newRoleId);
    UA_Role_clear(&role);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* UA_Server_addRole already published the Role Object under the RoleSet
     * (Part 18 §4.2.2, §4.3). */
    UA_Variant_setScalarCopy(&output[0], &newRoleId, &UA_TYPES[UA_TYPES_NODEID]);

    UA_NodeId_clear(&newRoleId);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeRoleMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionContext,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    UA_StatusCode access = checkRBACMethodAccess(server, sessionId);
    if(access != UA_STATUSCODE_GOOD)
        return access;
    if(inputSize != 1 || input[0].type != &UA_TYPES[UA_TYPES_NODEID])
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_NodeId roleId = *(UA_NodeId*)input[0].data;
    UA_Role role;
    UA_StatusCode res = UA_Server_getRoleById(server, roleId, &role);
    if(res != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    UA_QualifiedName roleName;
    res = UA_QualifiedName_copy(&role.roleName, &roleName);
    UA_Role_clear(&role);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* UA_Server_removeRole also drops the published Role Object from the
     * AddressSpace (Part 18 §4.2.3, §4.3). */
    res = UA_Server_removeRole(server, roleName);
    UA_QualifiedName_clear(&roleName);
    return res;
}

static UA_StatusCode
addIdentityMethodCallback(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    UA_StatusCode access = checkRBACMethodAccess(server, sessionId);
    if(access != UA_STATUSCODE_GOOD)
        return access;
    if(inputSize != 1 || input[0].type != &UA_TYPES[UA_TYPES_EXTENSIONOBJECT])
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_ExtensionObject *extObj = (UA_ExtensionObject*)input[0].data;
    if(!extObj->content.decoded.data ||
       extObj->content.decoded.type != &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE])
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_IdentityMappingRuleType *rule =
        (UA_IdentityMappingRuleType*)extObj->content.decoded.data;

    UA_Role role;
    UA_StatusCode res = UA_Server_getRoleById(server, *objectId, &role);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Reject equivalent existing rules per Part 18 §4.4.5 (Bad_AlreadyExists).
     * Equality is on the full struct, not just the criteriaType, so rules that
     * differ only in criteria remain distinct. */
    for(size_t i = 0; i < role.identityMappingRulesSize; i++) {
        if(UA_IdentityMappingRuleType_equal(&role.identityMappingRules[i], rule)) {
            UA_Role_clear(&role);
            return UA_STATUSCODE_BADALREADYEXISTS;
        }
    }

    UA_IdentityMappingRuleType *newRules = (UA_IdentityMappingRuleType*)
        UA_realloc(role.identityMappingRules,
                   (role.identityMappingRulesSize + 1) *
                   sizeof(UA_IdentityMappingRuleType));
    if(!newRules) {
        UA_Role_clear(&role);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    role.identityMappingRules = newRules;
    res = UA_IdentityMappingRuleType_copy(
        rule, &role.identityMappingRules[role.identityMappingRulesSize]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Role_clear(&role);
        return res;
    }
    role.identityMappingRulesSize++;

    res = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    return res;
}

static UA_StatusCode
removeIdentityMethodCallback(UA_Server *server,
                             const UA_NodeId *sessionId, void *sessionContext,
                             const UA_NodeId *methodId, void *methodContext,
                             const UA_NodeId *objectId, void *objectContext,
                             size_t inputSize, const UA_Variant *input,
                             size_t outputSize, UA_Variant *output) {
    UA_StatusCode access = checkRBACMethodAccess(server, sessionId);
    if(access != UA_STATUSCODE_GOOD)
        return access;
    if(inputSize != 1 || input[0].type != &UA_TYPES[UA_TYPES_EXTENSIONOBJECT])
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_ExtensionObject *extObj = (UA_ExtensionObject*)input[0].data;
    if(!extObj->content.decoded.data ||
       extObj->content.decoded.type != &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE])
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_IdentityMappingRuleType *rule =
        (UA_IdentityMappingRuleType*)extObj->content.decoded.data;

    UA_Role role;
    UA_StatusCode res = UA_Server_getRoleById(server, *objectId, &role);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Find and remove the identity rule that matches in both criteriaType and
     * criteria; several rules may share a criteriaType. */
    size_t idx = SIZE_MAX;
    for(size_t i = 0; i < role.identityMappingRulesSize; i++) {
        if(UA_IdentityMappingRuleType_equal(&role.identityMappingRules[i], rule)) {
            idx = i;
            break;
        }
    }
    if(idx == SIZE_MAX) {
        UA_Role_clear(&role);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_IdentityMappingRuleType_clear(&role.identityMappingRules[idx]);
    if(idx < role.identityMappingRulesSize - 1)
        memmove(&role.identityMappingRules[idx],
                &role.identityMappingRules[idx + 1],
                (role.identityMappingRulesSize - idx - 1) *
                sizeof(UA_IdentityMappingRuleType));
    role.identityMappingRulesSize--;

    res = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    return res;
}

static UA_StatusCode
addApplicationMethodCallback(UA_Server *server,
                             const UA_NodeId *sessionId, void *sessionContext,
                             const UA_NodeId *methodId, void *methodContext,
                             const UA_NodeId *objectId, void *objectContext,
                             size_t inputSize, const UA_Variant *input,
                             size_t outputSize, UA_Variant *output) {
    UA_StatusCode access = checkRBACMethodAccess(server, sessionId);
    if(access != UA_STATUSCODE_GOOD)
        return access;
    if(inputSize != 1 || input[0].type != &UA_TYPES[UA_TYPES_STRING])
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_Role role;
    UA_StatusCode res = UA_Server_getRoleById(server, *objectId, &role);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_String *newApps = (UA_String*)
        UA_realloc(role.applications,
                   (role.applicationsSize + 1) * sizeof(UA_String));
    if(!newApps) {
        UA_Role_clear(&role);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    role.applications = newApps;
    res = UA_String_copy((UA_String*)input[0].data,
                         &role.applications[role.applicationsSize]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Role_clear(&role);
        return res;
    }
    role.applicationsSize++;

    res = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    return res;
}

static UA_StatusCode
removeApplicationMethodCallback(UA_Server *server,
                                const UA_NodeId *sessionId, void *sessionContext,
                                const UA_NodeId *methodId, void *methodContext,
                                const UA_NodeId *objectId, void *objectContext,
                                size_t inputSize, const UA_Variant *input,
                                size_t outputSize, UA_Variant *output) {
    UA_StatusCode access = checkRBACMethodAccess(server, sessionId);
    if(access != UA_STATUSCODE_GOOD)
        return access;
    if(inputSize != 1 || input[0].type != &UA_TYPES[UA_TYPES_STRING])
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_String *uri = (UA_String*)input[0].data;

    UA_Role role;
    UA_StatusCode res = UA_Server_getRoleById(server, *objectId, &role);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    size_t idx = SIZE_MAX;
    for(size_t i = 0; i < role.applicationsSize; i++) {
        if(UA_String_equal(&role.applications[i], uri)) {
            idx = i;
            break;
        }
    }
    if(idx == SIZE_MAX) {
        UA_Role_clear(&role);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_String_clear(&role.applications[idx]);
    if(idx < role.applicationsSize - 1)
        memmove(&role.applications[idx], &role.applications[idx + 1],
                (role.applicationsSize - idx - 1) * sizeof(UA_String));
    role.applicationsSize--;

    res = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    return res;
}

static UA_StatusCode
addEndpointMethodCallback(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    UA_StatusCode access = checkRBACMethodAccess(server, sessionId);
    if(access != UA_STATUSCODE_GOOD)
        return access;
    if(inputSize != 1 || input[0].type != &UA_TYPES[UA_TYPES_EXTENSIONOBJECT])
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_ExtensionObject *extObj = (UA_ExtensionObject*)input[0].data;
    if(!extObj->content.decoded.data ||
       extObj->content.decoded.type != &UA_TYPES[UA_TYPES_ENDPOINTTYPE])
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_Role role;
    UA_StatusCode res = UA_Server_getRoleById(server, *objectId, &role);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_EndpointType *newEps = (UA_EndpointType*)
        UA_realloc(role.endpoints,
                   (role.endpointsSize + 1) * sizeof(UA_EndpointType));
    if(!newEps) {
        UA_Role_clear(&role);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    role.endpoints = newEps;
    res = UA_EndpointType_copy((UA_EndpointType*)extObj->content.decoded.data,
                               &role.endpoints[role.endpointsSize]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Role_clear(&role);
        return res;
    }
    role.endpointsSize++;

    res = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    return res;
}

static UA_StatusCode
removeEndpointMethodCallback(UA_Server *server,
                             const UA_NodeId *sessionId, void *sessionContext,
                             const UA_NodeId *methodId, void *methodContext,
                             const UA_NodeId *objectId, void *objectContext,
                             size_t inputSize, const UA_Variant *input,
                             size_t outputSize, UA_Variant *output) {
    UA_StatusCode access = checkRBACMethodAccess(server, sessionId);
    if(access != UA_STATUSCODE_GOOD)
        return access;
    if(inputSize != 1 || input[0].type != &UA_TYPES[UA_TYPES_EXTENSIONOBJECT])
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_ExtensionObject *extObj = (UA_ExtensionObject*)input[0].data;
    if(!extObj->content.decoded.data ||
       extObj->content.decoded.type != &UA_TYPES[UA_TYPES_ENDPOINTTYPE])
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_EndpointType *ep = (UA_EndpointType*)extObj->content.decoded.data;

    UA_Role role;
    UA_StatusCode res = UA_Server_getRoleById(server, *objectId, &role);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    size_t idx = SIZE_MAX;
    for(size_t i = 0; i < role.endpointsSize; i++) {
        if(UA_EndpointType_equal(&role.endpoints[i], ep)) {
            idx = i;
            break;
        }
    }
    if(idx == SIZE_MAX) {
        UA_Role_clear(&role);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_EndpointType_clear(&role.endpoints[idx]);
    if(idx < role.endpointsSize - 1)
        memmove(&role.endpoints[idx], &role.endpoints[idx + 1],
                (role.endpointsSize - idx - 1) * sizeof(UA_EndpointType));
    role.endpointsSize--;

    res = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    return res;
}

static UA_StatusCode
addRoleManagementPermissions(UA_Server *server, const UA_NodeId *nodeId) {
    const UA_NodeId secAdmin =
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    const UA_NodeId publicRoles[] = {
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS),
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER)
    };

    UA_StatusCode retval =
        UA_Server_addRolePermissions(server, *nodeId, secAdmin,
                                     UA_PERMISSIONTYPE_BROWSE |
                                     UA_PERMISSIONTYPE_CALL,
                                     false, false);
    if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_BADNODEIDUNKNOWN)
        return retval;

    for(size_t i = 0; i < sizeof(publicRoles) / sizeof(publicRoles[0]); i++) {
        retval = UA_Server_addRolePermissions(server, *nodeId, publicRoles[i],
                                              UA_PERMISSIONTYPE_BROWSE,
                                              false, false);
        if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_BADNODEIDUNKNOWN)
            return retval;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addOrBindRoleMethod(UA_Server *server, const UA_NodeId *roleId,
                    const char *name, UA_MethodCallback callback,
                    const char *inputName, size_t inputTypeIndex,
                    UA_Boolean applyPermissions) {
    UA_NodeId methodId = UA_NODEID_NULL;
    UA_StatusCode res = findMethodChild(server, *roleId, name, &methodId);
    if(res == UA_STATUSCODE_GOOD) {
        res = UA_Server_setMethodNode_callback(server, methodId, callback);
    } else if(res == UA_STATUSCODE_BADNOTFOUND) {
        UA_MethodAttributes attr = UA_MethodAttributes_default;
        attr.displayName = UA_LOCALIZEDTEXT("en-US", (char*)(uintptr_t)name);
        attr.executable = true;
        attr.userExecutable = true;

        UA_Argument inputArgument;
        UA_Argument_init(&inputArgument);
        inputArgument.name = UA_STRING((char*)(uintptr_t)inputName);
        inputArgument.dataType = UA_TYPES[inputTypeIndex].typeId;
        inputArgument.valueRank = UA_VALUERANK_SCALAR;

        res = UA_Server_addMethodNode(server, UA_NODEID_NULL, *roleId,
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                      UA_QUALIFIEDNAME(0, (char*)(uintptr_t)name),
                                      attr, callback, 1, &inputArgument,
                                      0, NULL, NULL, &methodId);
    }

    if(res == UA_STATUSCODE_GOOD && applyPermissions)
        res = addRoleManagementPermissions(server, &methodId);
    UA_NodeId_clear(&methodId);
    return res;
}

static UA_StatusCode
ensureRoleTypeMethods(UA_Server *server, const UA_NodeId *roleId,
                      UA_Boolean applyPermissions) {
    if(applyPermissions) {
        UA_StatusCode res = addRoleManagementPermissions(server, roleId);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    struct RoleMethodDef {
        const char *name;
        UA_MethodCallback callback;
        const char *inputName;
        size_t inputTypeIndex;
    } methods[] = {
        {"AddIdentity", addIdentityMethodCallback, "Rule",
         UA_TYPES_IDENTITYMAPPINGRULETYPE},
        {"RemoveIdentity", removeIdentityMethodCallback, "Rule",
         UA_TYPES_IDENTITYMAPPINGRULETYPE},
        {"AddApplication", addApplicationMethodCallback, "ApplicationUri",
         UA_TYPES_STRING},
        {"RemoveApplication", removeApplicationMethodCallback, "ApplicationUri",
         UA_TYPES_STRING},
        {"AddEndpoint", addEndpointMethodCallback, "Endpoint",
         UA_TYPES_ENDPOINTTYPE},
        {"RemoveEndpoint", removeEndpointMethodCallback, "Endpoint",
         UA_TYPES_ENDPOINTTYPE}
    };

    for(size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); i++) {
        UA_StatusCode res = addOrBindRoleMethod(server, roleId,
                                                methods[i].name,
                                                methods[i].callback,
                                                methods[i].inputName,
                                                methods[i].inputTypeIndex,
                                                applyPermissions);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    return UA_STATUSCODE_GOOD;
}

/* Restrict the RoleSet Object and the security-sensitive RoleSet/RoleType
 * Methods to the SecurityAdmin Role (OPC UA Part 18). The RoleSet stays
 * browsable for the Anonymous/AuthenticatedUser Roles. Skipped when the NS0
 * RBAC information model is unavailable. */
UA_StatusCode
initRoleSetRolePermissions(UA_Server *server) {
    UA_NodeId roleSetId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET);
    UA_QualifiedName bn;
    if(UA_Server_readBrowseName(server, roleSetId, &bn) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_GOOD; /* no NS0 RBAC model -> nothing to protect */
    UA_QualifiedName_clear(&bn);

    const UA_NodeId secAdmin =
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    const UA_NodeId publicRoles[] = {
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS),
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER)
    };

    /* Nodes whose CALL is restricted to SecurityAdmin. The RoleSet Object is
     * included because the Call service checks CALL on both the Object and the
     * Method node. BROWSE is granted back to the public Roles so the nodes
     * stay visible. */
    const UA_UInt32 callNodes[] = {
        UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET_ADDROLE,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET_REMOVEROLE,
        UA_NS0ID_ROLETYPE_ADDIDENTITY,
        UA_NS0ID_ROLETYPE_REMOVEIDENTITY,
        UA_NS0ID_ROLETYPE_ADDAPPLICATION,
        UA_NS0ID_ROLETYPE_REMOVEAPPLICATION,
        UA_NS0ID_ROLETYPE_ADDENDPOINT,
        UA_NS0ID_ROLETYPE_REMOVEENDPOINT
    };

    /* StatusCodes are not bit flags, so check each result individually.
     * A missing node (BadNodeIdUnknown) is tolerated: a reduced nodeset may
     * omit individual Methods. Any other failure aborts. */
    UA_StatusCode retval;

    /* Admin may additionally read the RolePermissions attribute of the RoleSet */
    retval = UA_Server_addRolePermissions(server, roleSetId, secAdmin,
                                          UA_PERMISSIONTYPE_READROLEPERMISSIONS,
                                          false, false);
    if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_BADNODEIDUNKNOWN)
        return retval;

    for(size_t i = 0; i < sizeof(callNodes) / sizeof(callNodes[0]); i++) {
        UA_NodeId nodeId = UA_NODEID_NUMERIC(0, callNodes[i]);

        /* SecurityAdmin: browse + call */
        retval = UA_Server_addRolePermissions(server, nodeId, secAdmin,
                                              UA_PERMISSIONTYPE_BROWSE |
                                              UA_PERMISSIONTYPE_CALL,
                                              false, false);
        if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_BADNODEIDUNKNOWN)
            return retval;

        /* Public Roles: browse only (visible but not callable) */
        for(size_t j = 0; j < sizeof(publicRoles) / sizeof(publicRoles[0]); j++) {
            retval = UA_Server_addRolePermissions(server, nodeId, publicRoles[j],
                                                  UA_PERMISSIONTYPE_BROWSE,
                                                  false, false);
            if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_BADNODEIDUNKNOWN)
                return retval;
        }
    }

    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = roleSetId;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    bd.includeSubtypes = false;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.nodeClassMask = UA_NODECLASS_OBJECT;
    bd.resultMask = UA_BROWSERESULTMASK_NONE;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    retval = br.statusCode;
    if(retval == UA_STATUSCODE_GOOD) {
        for(size_t i = 0; i < br.referencesSize; i++) {
            retval = ensureRoleTypeMethods(server,
                                           &br.references[i].nodeId.nodeId,
                                           true);
            if(retval != UA_STATUSCODE_GOOD)
                break;
        }
    }
    UA_BrowseResult_clear(&br);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
initNS0RBAC(UA_Server *server) {
    /* RBAC NS0 wiring requires UA_NAMESPACE_ZERO=FULL.
     * Without it the C API still works, but we skip the NS0 objects. */
    UA_NodeId roleSetTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE);
    UA_QualifiedName typebn;
    UA_Boolean hasFullRbacNS0 =
        (UA_Server_readBrowseName(server, roleSetTypeId, &typebn) == UA_STATUSCODE_GOOD);
    if(hasFullRbacNS0)
        UA_QualifiedName_clear(&typebn);

    if(!hasFullRbacNS0) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "RBAC: RoleSetType (NS0 i=%u) not present - NS0 RBAC "
                       "information model skipped (requires UA_NAMESPACE_ZERO=FULL)",
                       UA_NS0ID_ROLESETTYPE);
        return UA_STATUSCODE_GOOD;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Ensure the RoleSet instance node exists */
    UA_NodeId roleSetId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET);
    UA_QualifiedName bn;
    if(UA_Server_readBrowseName(server, roleSetId, &bn) != UA_STATUSCODE_GOOD) {
        UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
        oAttr.displayName = UA_LOCALIZEDTEXT("", "RoleSet");
        retval |= UA_Server_addObjectNode(
            server, roleSetId,
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(0, "RoleSet"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE),
            oAttr, NULL, NULL);
    } else {
        UA_QualifiedName_clear(&bn);
    }

    /* Ensure the well-known role instance nodes exist under the RoleSet */
    struct { UA_UInt32 id; const char *name; } roles[] = {
        {UA_NS0ID_WELLKNOWNROLE_ANONYMOUS,          "Anonymous"},
        {UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER,  "AuthenticatedUser"},
        {UA_NS0ID_WELLKNOWNROLE_TRUSTEDAPPLICATION, "TrustedApplication"},
        {UA_NS0ID_WELLKNOWNROLE_OBSERVER,           "Observer"},
        {UA_NS0ID_WELLKNOWNROLE_OPERATOR,           "Operator"},
        {UA_NS0ID_WELLKNOWNROLE_ENGINEER,           "Engineer"},
        {UA_NS0ID_WELLKNOWNROLE_SUPERVISOR,         "Supervisor"},
        {UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN,     "ConfigureAdmin"},
        {UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN,      "SecurityAdmin"}
#ifdef UA_NS0ID_WELLKNOWNROLE_SECURITYKEYSERVERADMIN
        ,{UA_NS0ID_WELLKNOWNROLE_SECURITYKEYSERVERADMIN,  "SecurityKeyServerAdmin"}
        ,{UA_NS0ID_WELLKNOWNROLE_SECURITYKEYSERVERPUSH,   "SecurityKeyServerPush"}
        ,{UA_NS0ID_WELLKNOWNROLE_SECURITYKEYSERVERACCESS, "SecurityKeyServerAccess"}
#endif
    };
    for(size_t i = 0; i < sizeof(roles) / sizeof(roles[0]); i++) {
        UA_NodeId rId = UA_NODEID_NUMERIC(0, roles[i].id);
        if(UA_Server_readBrowseName(server, rId, &bn) != UA_STATUSCODE_GOOD) {
            UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
            oAttr.displayName = UA_LOCALIZEDTEXT("", (char*)(uintptr_t)roles[i].name);
            retval |= UA_Server_addObjectNode(
                server, rId, roleSetId,
                UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                UA_QUALIFIEDNAME(0, (char*)(uintptr_t)roles[i].name),
                UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE),
                oAttr, NULL, NULL);
        } else {
            UA_QualifiedName_clear(&bn);
        }

        /* Back the Identities property with the role registry so that reads
         * return the currently configured identity mapping rules */
        UA_NodeId identitiesId;
        if(findPropertyChild(server, rId, "Identities",
                             &identitiesId) == UA_STATUSCODE_GOOD) {
            UA_DataSource identitiesDataSource;
            identitiesDataSource.read = readRoleIdentities;
            identitiesDataSource.write = NULL;
            retval |= UA_Server_setVariableNode_dataSource(server, identitiesId,
                                                           identitiesDataSource);
            UA_NodeId_clear(&identitiesId);
        }

        UA_NodeId applicationsExcludeId;
        if(findPropertyChild(server, rId, "ApplicationsExclude",
                             &applicationsExcludeId) == UA_STATUSCODE_GOOD) {
            UA_DataSource applicationsExcludeDataSource;
            applicationsExcludeDataSource.read = readRoleApplicationsExclude;
            applicationsExcludeDataSource.write = writeRoleApplicationsExclude;
            retval |= UA_Server_setVariableNode_dataSource(server, applicationsExcludeId,
                                                           applicationsExcludeDataSource);
            UA_NodeId_clear(&applicationsExcludeId);
        }

        UA_NodeId endpointsExcludeId;
        if(findPropertyChild(server, rId, "EndpointsExclude",
                             &endpointsExcludeId) == UA_STATUSCODE_GOOD) {
            UA_DataSource endpointsExcludeDataSource;
            endpointsExcludeDataSource.read = readRoleEndpointsExclude;
            endpointsExcludeDataSource.write = writeRoleEndpointsExclude;
            retval |= UA_Server_setVariableNode_dataSource(server, endpointsExcludeId,
                                                           endpointsExcludeDataSource);
            UA_NodeId_clear(&endpointsExcludeId);
        }

        /* Back the CustomConfiguration property with the role registry so
         * reads return the configured value (Part 18 §4.4.1). */
        UA_NodeId customConfigId;
        if(findPropertyChild(server, rId, "CustomConfiguration",
                             &customConfigId) == UA_STATUSCODE_GOOD) {
            UA_DataSource customConfigDataSource;
            customConfigDataSource.read = readRoleCustomConfiguration;
            customConfigDataSource.write = NULL;
            retval |= UA_Server_setVariableNode_dataSource(server, customConfigId,
                                                           customConfigDataSource);
            UA_NodeId_clear(&customConfigId);
        }

        retval |= ensureRoleTypeMethods(server, &rId, false);
    }

    /* The method callbacks must be attached to the RoleSet *instance* methods.
     * A Call resolves the object's own HasComponent method (the instance node),
     * not the type method, so a callback on the type node would never fire. */
    retval |= UA_Server_setMethodNode_callback(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET_ADDROLE),
        addRoleMethodCallback);
    retval |= UA_Server_setMethodNode_callback(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET_REMOVEROLE),
        removeRoleMethodCallback);

    retval |= UA_Server_setMethodNode_callback(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDIDENTITY),
        addIdentityMethodCallback);
    retval |= UA_Server_setMethodNode_callback(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_REMOVEIDENTITY),
        removeIdentityMethodCallback);

    retval |= UA_Server_setMethodNode_callback(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDAPPLICATION),
        addApplicationMethodCallback);
    retval |= UA_Server_setMethodNode_callback(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_REMOVEAPPLICATION),
        removeApplicationMethodCallback);

    retval |= UA_Server_setMethodNode_callback(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDENDPOINT),
        addEndpointMethodCallback);
    retval |= UA_Server_setMethodNode_callback(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_REMOVEENDPOINT),
        removeEndpointMethodCallback);

    return retval;
}

#endif /* UA_ENABLE_RBAC */
