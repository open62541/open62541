/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *    Copyright 2022 (c) Kalycito Infotech Private Limited
 */

#include "ua_server_internal.h"
#include "ua_session.h"
#include "ua_subscription.h"
#include "ua_server_role_access.h"

#ifdef UA_ENABLE_ROLE_PERMISSION
UA_StatusCode
UA_Server_addRoleAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionHandle,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output){
    UA_NodeId outNewNodeId;
    size_t    namespaceIndex;
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    UA_String *roleName = (UA_String*)input[0].data;
    UA_String *namespaceURI = (UA_String*)input[1].data;
    attr.description = UA_LOCALIZEDTEXT("en-US", (char *)roleName->data);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", (char *)roleName->data);
    UA_StatusCode retVal = UA_Server_getNamespaceByName(server, *namespaceURI, &namespaceIndex);
    if (retVal != UA_STATUSCODE_GOOD)
       return UA_STATUSCODE_BADINTERNALERROR;

    UA_Server_addObjectNode(server, UA_NODEID_STRING((UA_UInt16)namespaceIndex,(char*)roleName->data), 
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET), UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME((UA_UInt16)namespaceIndex, (char *)roleName->data), UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE),
                            attr, NULL, &outNewNodeId);
    UA_Variant_setScalarCopy(output, &outNewNodeId, &UA_TYPES[UA_TYPES_NODEID]);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeRoleAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionHandle,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output){
    UA_NodeId *roleNodeId = (UA_NodeId *)input[0].data;
    UA_Server_deleteNode(server, *roleNodeId, true);
    return UA_STATUSCODE_GOOD;        
}

UA_StatusCode
UA_Server_addRole(UA_Server *server, UA_NodeId parentNodeId, UA_NodeId targetNodeId, 
                  UA_ObjectAttributes attr, UA_QualifiedName browseName) {
     UA_Server_deleteNode(server, targetNodeId, true);
     UA_Server_addObjectNode(server, targetNodeId, 
                             parentNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                             browseName, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE),
                             attr, NULL, NULL);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeRole(UA_Server *server, UA_NodeId targetNodeId) {
    UA_Server_deleteNode(server, targetNodeId, true);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setDefaultRoles(UA_Server *server) {
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "RoleSet");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "RoleSet");
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET), true);
    UA_NodeId outNewNodeId;
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET), 
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                             UA_QUALIFIEDNAME(0, "RoleSet"), UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE),
                             attr, NULL, &outNewNodeId);
    UA_Server_setMethodNodeCallback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_ADDROLE), 
                                    UA_Server_addRoleAction);
    UA_Server_setMethodNodeCallback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_REMOVEROLE), 
                                    UA_Server_removeRoleAction);

    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "Anonymous");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Anonymous");
    UA_Server_addRole(server, outNewNodeId, 
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS), attr, UA_QUALIFIEDNAME(0, "Anonymous"));
    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "AuthenticatedUser");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "AuthenticatedUser");
    UA_Server_addRole(server, outNewNodeId, 
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER), attr, UA_QUALIFIEDNAME(0, "AuthenticatedUser"));
    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "ConfigureDomain");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "ConfigureDomain");
    UA_Server_addRole(server, outNewNodeId, 
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN), attr, UA_QUALIFIEDNAME(0, "ConfigureDomain"));
    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "Engineer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Engineer");
    UA_Server_addRole(server, outNewNodeId, 
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER), attr, UA_QUALIFIEDNAME(0, "Engineer"));
    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "Observer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Observer");
    UA_Server_addRole(server, outNewNodeId, 
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER), attr, UA_QUALIFIEDNAME(0, "Observer"));
    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "Operator");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Operator");
    UA_Server_addRole(server, outNewNodeId, 
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR), attr, UA_QUALIFIEDNAME(0, "Operator"));

    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "SecurityAdmin");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "SecurityAdmin");
    UA_Server_addRole(server, outNewNodeId, 
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN), attr, UA_QUALIFIEDNAME(0, "SecurityAdmin"));
    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "Supervisor");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Supervisor");
    UA_Server_addRole(server, outNewNodeId, 
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR), attr, UA_QUALIFIEDNAME(0, "Supervisor"));

    return UA_STATUSCODE_GOOD;

}
#endif