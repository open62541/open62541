/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025-2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#ifndef UA_SERVER_RBAC_H_
#define UA_SERVER_RBAC_H_

#include <open62541/server.h>
#include <open62541/plugin/nodestore.h>

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_RBAC

#include "ua_session.h"

/* Set roles on a session. Validates all role IDs against the server registry.
 * Must be called with the server lock held. */
UA_StatusCode
UA_Session_setRoles(UA_Server *server, UA_Session *session,
                    const UA_NodeId *roleIds, size_t rolesSize);

/* Decrement the refCount of a role permission entry at the given index.
 * Used during node deletion to keep refcounts consistent. */
void
UA_Server_decrementRolePermissionsRefCount(UA_Server *server,
                                           UA_PermissionIndex index);

/* Low-level permission index functions (internal, used by tests) */
UA_StatusCode
UA_Server_setNodePermissionIndex(UA_Server *server, const UA_NodeId nodeId,
                                 UA_PermissionIndex permissionIndex,
                                 UA_Boolean recursive);

UA_StatusCode
UA_Server_getNodePermissionIndex(UA_Server *server, const UA_NodeId nodeId,
                                 UA_PermissionIndex *permissionIndex);

UA_StatusCode
UA_Server_addRolePermissionConfig(UA_Server *server,
                                  size_t entriesSize,
                                  const UA_RolePermission *entries,
                                  UA_PermissionIndex *outIndex);

const UA_RolePermissionSet *
UA_Server_getRolePermissionConfig(UA_Server *server,
                                  UA_PermissionIndex index);

UA_StatusCode
UA_Server_updateRolePermissionConfig(UA_Server *server,
                                     UA_PermissionIndex index,
                                     size_t entriesSize,
                                     const UA_RolePermission *entries);

/* Effective permission queries (internal, used by attribute service and tests) */
UA_StatusCode
UA_Server_getEffectivePermissions(UA_Server *server,
                                  const UA_NodeId *sessionId,
                                  const UA_NodeId *nodeId,
                                  UA_PermissionType *effectivePermissions);

UA_StatusCode
UA_Server_getUserRolePermissions(UA_Server *server,
                                 const UA_NodeId *sessionId,
                                 const UA_NodeId *nodeId,
                                 size_t *entriesSize,
                                 UA_RolePermissionType **entries);

#endif /* UA_ENABLE_RBAC */

_UA_END_DECLS

#endif /* UA_SERVER_RBAC_H_ */
