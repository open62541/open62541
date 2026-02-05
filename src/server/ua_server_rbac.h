/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Andreas Ebner
 */

#ifndef UA_SERVER_RBAC_H_
#define UA_SERVER_RBAC_H_

#include "ua_server_internal.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_RBAC

UA_StatusCode
UA_Server_initializeRBAC(UA_Server *server);

void
UA_Server_cleanupRBAC(UA_Server *server);

/* Decrement the refCount for a node's rolePermissions entry.
 * Called when a node is being deleted. */
void
UA_Server_decrementRolePermissionsRefCount(UA_Server *server,
                                           UA_PermissionIndex permissionIndex);

#endif /* UA_ENABLE_RBAC */

_UA_END_DECLS

#endif /* UA_SERVER_RBAC_H_ */