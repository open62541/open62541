/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Kalycito Infotech Private Limited
 *    Copyright 2023 (c) Asish Ganesh, Eclatron Technologies Private Limited
 */

#ifndef UA_SERVER_ROLE_ACCESS_H_
#define UA_SERVER_ROLE_ACCESS_H_

#include "ua_server_internal.h"
#include "ua_session.h"
#include "ua_subscription.h"
#include <open62541/plugin/accesscontrol_custom.h>

_UA_BEGIN_DECLS
#ifdef UA_ENABLE_ROLE_PERMISSIONS
UA_StatusCode
UA_Server_setDefaultRoles(UA_Server *server);
UA_StatusCode
UA_Server_removeDefaultRoles(UA_Server *server);
UA_StatusCode
UA_Server_addRole(UA_Server *server, UA_NodeId parentNodeId, UA_NodeId targetNodeId,
                  UA_ObjectAttributes attr, UA_QualifiedName browseName);
UA_StatusCode
UA_Server_removeRole(UA_Server *server, UA_NodeId targetNodeId);

UA_StatusCode setRuntimeUserPermission(UA_Server *server, UA_UsernameRoleInfo *userAndRoleInfo);

UA_StatusCode
UA_Server_addRoleAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionHandle,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output);

UA_StatusCode
UA_Server_removeRoleAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionHandle,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output);

UA_StatusCode
UA_Server_addIdentityActionForWellKonwnRules(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionHandle,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output);

UA_StatusCode
UA_Server_removeIdentityActionForWellKonwnRules(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionHandle,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output);

UA_StatusCode
UA_Server_AddIdentity_method(UA_Server *server, UA_NodeId requested_node_id, UA_NodeId parent_node_id);

UA_StatusCode
UA_Server_RemoveIdentity_method(UA_Server *server, UA_NodeId requested_node_id, UA_NodeId parent_node_id);

UA_NodeId
findRoleIdentityNodeID(UA_Server *server, UA_NodeId startingNode);

#endif
_UA_END_DECLS

#endif /* UA_SERVER_ROLE_ACCESS_H_ */
