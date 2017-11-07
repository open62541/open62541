/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_PLUGIN_ACCESS_CONTROL_H_
#define UA_PLUGIN_ACCESS_CONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"

/**
 * Access Control Plugin API
 * =========================
 * The access control callback is used to authenticate sessions and grant access
 * rights accordingly. */

typedef struct {
    /* These booleans are used to create endpoints for the possible
     * authentication methods */
    UA_Boolean enableAnonymousLogin;
    UA_Boolean enableUsernamePasswordLogin;

    /* Authenticate a session. The session context is attached to the session and
     * later passed into the node-based access control callbacks. */
    UA_StatusCode (*activateSession)(const UA_NodeId *sessionId,
                                     const UA_ExtensionObject *userIdentityToken,
                                     void **sessionContext);

    /* Deauthenticate a session and cleanup */
    void (*closeSession)(const UA_NodeId *sessionId, void *sessionContext);

    /* Access control for all nodes*/
    UA_UInt32 (*getUserRightsMask)(const UA_NodeId *sessionId, void *sessionContext,
                                   const UA_NodeId *nodeId, void *nodeContext);

    /* Additional access control for variable nodes */
    UA_Byte (*getUserAccessLevel)(const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *nodeId, void *nodeContext);

    /* Additional access control for method nodes */
    UA_Boolean (*getUserExecutable)(const UA_NodeId *sessionId, void *sessionContext,
                                    const UA_NodeId *methodId, void *methodContext);

    /* Additional access control for calling a method node in the context of a
     * specific object */
    UA_Boolean (*getUserExecutableOnObject)(const UA_NodeId *sessionId, void *sessionContext,
                                            const UA_NodeId *methodId, void *methodContext,
                                            const UA_NodeId *objectId, void *objectContext);

    /* Allow adding a node */
    UA_Boolean (*allowAddNode)(const UA_NodeId *sessionId, void *sessionContext,
                               const UA_AddNodesItem *item);

    /* Allow adding a reference */
    UA_Boolean (*allowAddReference)(const UA_NodeId *sessionId, void *sessionContext,
                                    const UA_AddReferencesItem *item);

    /* Allow deleting a node */
    UA_Boolean (*allowDeleteNode)(const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_DeleteNodesItem *item);

    /* Allow deleting a reference */
    UA_Boolean (*allowDeleteReference)(const UA_NodeId *sessionId, void *sessionContext,
                                       const UA_DeleteReferencesItem *item);
} UA_AccessControl;

#ifdef __cplusplus
}
#endif

#endif /* UA_PLUGIN_ACCESS_CONTROL_H_ */
