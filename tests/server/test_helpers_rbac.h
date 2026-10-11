/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

/* In-process Sessions for the RBAC tests */

#ifndef TEST_HELPERS_RBAC_H_
#define TEST_HELPERS_RBAC_H_

#include <open62541/server.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#include "server/ua_subscription.h"

#include <check.h>

static UA_INLINE void
rbacSetRoles(UA_Server *server, const UA_NodeId *sessionId,
             size_t rolesSize, const UA_NodeId *roles) {
    UA_Variant v;
    UA_Variant_setArray(&v, (void*)(uintptr_t)roles, rolesSize,
                        &UA_TYPES[UA_TYPES_NODEID]);
    ck_assert_uint_eq(UA_Server_setSessionAttribute(server, sessionId,
                                                    UA_QUALIFIEDNAME(0, "roles"),
                                                    &v),
                      UA_STATUSCODE_GOOD);
}

/* A Session on the channel (may be NULL) that holds exactly the Role */
static UA_INLINE UA_Session *
rbacSession(UA_Server *server, UA_SecureChannel *channel, UA_NodeId role) {
    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);
    request.requestedSessionTimeout = UA_UINT32_MAX;
    UA_Session *session = NULL;
    lockServer(server);
    UA_StatusCode res = UA_Session_create(server, channel, &request, &session);
    unlockServer(server);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    rbacSetRoles(server, &session->sessionId, 1, &role);
    return session;
}

#endif /* TEST_HELPERS_RBAC_H_ */
