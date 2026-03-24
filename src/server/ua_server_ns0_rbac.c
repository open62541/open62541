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

/* RBAC NS0 information model integration */

static UA_StatusCode
readRoleIdentities(UA_Server *server, const UA_NodeId *sessionId,
                   void *sessionContext,
                   const UA_NodeId *nodeId, void *nodeContext,
                   UA_Boolean includeSourceTimeStamp,
                   const UA_NumericRange *range,
                   UA_DataValue *value) {
    UA_NodeId roleId = *(UA_NodeId*)nodeContext;
    UA_Role role;
    UA_StatusCode res = UA_Server_getRoleById(server, roleId, &role);
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
    UA_NodeId roleId = *(UA_NodeId*)nodeContext;
    UA_Role role;
    UA_StatusCode res = UA_Server_getRoleById(server, roleId, &role);
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
    UA_NodeId roleId = *(UA_NodeId*)nodeContext;
    UA_Role role;
    UA_StatusCode res = UA_Server_getRoleById(server, roleId, &role);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Variant_setArrayCopy(&value->value, role.endpoints,
                            role.endpointsSize,
                            &UA_TYPES[UA_TYPES_ENDPOINTTYPE]);
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
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                  role->roleName,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE),
                                  oAttr, NULL, NULL);
    if(res != UA_STATUSCODE_GOOD)
        return res;

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

    UA_NodeId *identitiesCtx = UA_NodeId_new();
    if(!identitiesCtx) {
        UA_NodeId_clear(&identitiesNodeId);
        UA_Server_deleteNode(server, role->roleId, true);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_NodeId_copy(&role->roleId, identitiesCtx);

    UA_DataSource identitiesDataSource;
    identitiesDataSource.read = readRoleIdentities;
    identitiesDataSource.write = NULL;

    res = UA_Server_setVariableNode_dataSource(server, identitiesNodeId,
                                               identitiesDataSource);
    res |= UA_Server_setNodeContext(server, identitiesNodeId, identitiesCtx);
    UA_NodeId_clear(&identitiesNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_delete(identitiesCtx);
        UA_Server_deleteNode(server, role->roleId, true);
        return res;
    }

    /* Add optional Applications property with DataSource */
    UA_NodeId *applicationsCtx = UA_NodeId_new();
    if(!applicationsCtx) {
        UA_Server_deleteNode(server, role->roleId, true);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_NodeId_copy(&role->roleId, applicationsCtx);

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
                                              applicationsCtx, &applicationsNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_delete(applicationsCtx);
        UA_Server_deleteNode(server, role->roleId, true);
        return res;
    }

    /* Add optional Endpoints property with DataSource */
    UA_NodeId *endpointsCtx = UA_NodeId_new();
    if(!endpointsCtx) {
        UA_Server_deleteNode(server, role->roleId, true);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_NodeId_copy(&role->roleId, endpointsCtx);

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
                                              endpointsCtx, &endpointsNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_delete(endpointsCtx);
        UA_Server_deleteNode(server, role->roleId, true);
        return res;
    }

    return UA_STATUSCODE_GOOD;
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
    UA_String_copy(roleName, &role.roleName.name);

    /* Per specification, use NS1 if no namespaceUri is given */
    if(namespaceUri->length > 0) {
        size_t nsIdx = 0;
        UA_StatusCode res = UA_Server_getNamespaceByName(server, *namespaceUri, &nsIdx);
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

    res = UA_Server_removeRole(server, roleName);
    UA_QualifiedName_clear(&roleName);
    return res;
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

    UA_IdentityMappingRuleType *rule =
        (UA_IdentityMappingRuleType*)extObj->content.decoded.data;

    UA_Role role;
    UA_StatusCode res = UA_Server_getRoleById(server, *objectId, &role);
    if(res != UA_STATUSCODE_GOOD)
        return res;

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

    UA_IdentityMappingRuleType *rule =
        (UA_IdentityMappingRuleType*)extObj->content.decoded.data;

    UA_Role role;
    UA_StatusCode res = UA_Server_getRoleById(server, *objectId, &role);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Find and remove the matching identity rule */
    size_t idx = SIZE_MAX;
    for(size_t i = 0; i < role.identityMappingRulesSize; i++) {
        if(role.identityMappingRules[i].criteriaType == rule->criteriaType) {
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
                             const UA_NodeId *objectId, void *objectContext,
                             const UA_NodeId *methodId, void *methodContext,
                             const UA_NodeId *inputType, void *inputContext,
                             size_t inputSize, const UA_Variant *input,
                             size_t outputSize, UA_Variant *output) {
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
                                const UA_NodeId *objectId, void *objectContext,
                                const UA_NodeId *methodId, void *methodContext,
                                const UA_NodeId *inputType, void *inputContext,
                                size_t inputSize, const UA_Variant *input,
                                size_t outputSize, UA_Variant *output) {
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

    /* Ensure the 8 well-known role instance nodes exist under the RoleSet */
    struct { UA_UInt32 id; const char *name; } roles[] = {
        {UA_NS0ID_WELLKNOWNROLE_ANONYMOUS,         "Anonymous"},
        {UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER,  "AuthenticatedUser"},
        {UA_NS0ID_WELLKNOWNROLE_OBSERVER,           "Observer"},
        {UA_NS0ID_WELLKNOWNROLE_OPERATOR,           "Operator"},
        {UA_NS0ID_WELLKNOWNROLE_ENGINEER,           "Engineer"},
        {UA_NS0ID_WELLKNOWNROLE_SUPERVISOR,         "Supervisor"},
        {UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN,     "ConfigureAdmin"},
        {UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN,      "SecurityAdmin"}
    };
    for(size_t i = 0; i < 8; i++) {
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
    }

    retval |= UA_Server_setMethodNode_callback(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_ADDROLE),
        addRoleMethodCallback);
    retval |= UA_Server_setMethodNode_callback(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_REMOVEROLE),
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
