/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2023 (c) Asish Ganesh, Eclatron Technologies Private Limited
 */

#ifndef UA_PLUGIN_ACCESS_CONTROL_H_
#define UA_PLUGIN_ACCESS_CONTROL_H_

#include <open62541/util.h>

_UA_BEGIN_DECLS

struct UA_AccessControl;
typedef struct UA_AccessControl UA_AccessControl;

#ifdef UA_ENABLE_ROLE_PERMISSIONS

typedef enum {
    UA_ROLE_ANONYMOUS = 0,
    UA_ROLE_AUTHENTICATEDUSER = 1,
    UA_ROLE_CONFIGUREADMIN = 2,
    UA_ROLE_ENGINEER = 3,
    UA_ROLE_OBSERVER = 4,
    UA_ROLE_OPERATOR = 5,
    UA_ROLE_SECURITYADMIN = 6,
    UA_ROLE_SUPERVISOR = 7,
}UA_ROLE;

static const UA_Int32 UA_ROLES[] = {
    UA_NS0ID_WELLKNOWNROLE_ANONYMOUS,
    UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER,
    UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN,
    UA_NS0ID_WELLKNOWNROLE_ENGINEER,
    UA_NS0ID_WELLKNOWNROLE_OBSERVER,
    UA_NS0ID_WELLKNOWNROLE_OPERATOR,
    UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN,
    UA_NS0ID_WELLKNOWNROLE_SUPERVISOR
};

typedef struct {
    UA_ROLE                accessControlGroup;
    UA_UInt32              accessPermissions;
    UA_Boolean             methodAccessPermission;
    UA_IdentityMappingRuleType identityMappingRule;
    UA_RolePermissionType  role;
} UA_AccessControlSettings;

typedef struct {
    UA_String *username;
    UA_String *rolename;
    UA_AccessControlSettings *accessControlSettings;
} UA_UsernameRoleInfo;
#endif

/**
 * .. _access-control:
 *
 * Access Control Plugin API
 * =========================
 * The access control callback is used to authenticate sessions and grant access
 * rights accordingly.
 *
 * The ``sessionId`` and ``sessionContext`` can be both NULL. This is the case
 * when, for example, a MonitoredItem (the underlying Subscription) is detached
 * from its Session but continues to run. */

struct UA_AccessControl {
    void *context;
    void (*clear)(UA_AccessControl *ac);

    /* Supported login mechanisms. The server endpoints are created from here. */
    size_t userTokenPoliciesSize;
    UA_UserTokenPolicy *userTokenPolicies;

    /* Authenticate a session. The session context is attached to the session
     * and later passed into the node-based access control callbacks. The new
     * session is rejected if a StatusCode other than UA_STATUSCODE_GOOD is
     * returned.
     *
     * Note that this callback can be called several times for a Session. For
     * example when a Session is recovered (activated) on a new
     * SecureChannel. */
    UA_StatusCode (*activateSession)(UA_Server *server, UA_AccessControl *ac,
                                     const UA_EndpointDescription *endpointDescription,
                                     const UA_ByteString *secureChannelRemoteCertificate,
                                     const UA_NodeId *sessionId,
                                     const UA_ExtensionObject *userIdentityToken,
                                     void **sessionContext);

    /* Deauthenticate a session and cleanup */
    void (*closeSession)(UA_Server *server, UA_AccessControl *ac,
                         const UA_NodeId *sessionId, void *sessionContext);

    /* Access control for all nodes*/
    UA_UInt32 (*getUserRightsMask)(UA_Server *server, UA_AccessControl *ac,
                                   const UA_NodeId *sessionId, void *sessionContext,
                                   const UA_NodeId *nodeId, void *nodeContext);

    /* Additional access control for variable nodes */
    UA_Byte (*getUserAccessLevel)(UA_Server *server, UA_AccessControl *ac,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *nodeId, void *nodeContext);

    /* Additional access control for method nodes */
    UA_Boolean (*getUserExecutable)(UA_Server *server, UA_AccessControl *ac,
                                    const UA_NodeId *sessionId, void *sessionContext,
                                    const UA_NodeId *methodId, void *methodContext);

    /* Additional access control for calling a method node in the context of a
     * specific object */
    UA_Boolean (*getUserExecutableOnObject)(UA_Server *server, UA_AccessControl *ac,
                                            const UA_NodeId *sessionId, void *sessionContext,
                                            const UA_NodeId *methodId, void *methodContext,
                                            const UA_NodeId *objectId, void *objectContext);

    /* Allow adding a node */
    UA_Boolean (*allowAddNode)(UA_Server *server, UA_AccessControl *ac,
                               const UA_NodeId *sessionId, void *sessionContext,
                               const UA_AddNodesItem *item, UA_RolePermissionType *userRolePermission, size_t userRoleSize);

    /* Allow adding a reference */
    UA_Boolean (*allowAddReference)(UA_Server *server, UA_AccessControl *ac,
                                    const UA_NodeId *sessionId, void *sessionContext,
                                    const UA_AddReferencesItem *item, UA_RolePermissionType *userRolePermission, size_t userRoleSize);

    /* Allow deleting a node */
    UA_Boolean (*allowDeleteNode)(UA_Server *server, UA_AccessControl *ac,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_DeleteNodesItem *item, UA_RolePermissionType *userRolePermission,size_t userRolePermissionSize);

    /* Allow deleting a reference */
    UA_Boolean (*allowDeleteReference)(UA_Server *server, UA_AccessControl *ac,
                                       const UA_NodeId *sessionId, void *sessionContext,
                                       const UA_DeleteReferencesItem *item, UA_RolePermissionType *userRolePermission, size_t userRoleSize);

    /* Allow browsing a node */
    UA_Boolean (*allowBrowseNode)(UA_Server *server, UA_AccessControl *ac,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *nodeId, void *nodeContext, UA_RolePermissionType *userRolePermission,size_t userRoleSize);

    /* Check access to Node */
    UA_Boolean (*hasAccessToNode)(UA_Server *server, UA_AccessControl *ac,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *nodeId, void *nodeContext, UA_Byte* serviceAccessLevel);
    /* Check access to Node */
    UA_Boolean (*hasAccessToMethod)(UA_Server *server, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Allow transfer of a subscription to another session. The Server shall
     * validate that the Client of that Session is operating on behalf of the
     * same user */
    UA_Boolean (*allowTransferSubscription)(UA_Server *server, UA_AccessControl *ac,
                                            const UA_NodeId *oldSessionId, void *oldSessionContext,
                                            const UA_NodeId *newSessionId, void *newSessionContext);
#endif

#ifdef UA_ENABLE_HISTORIZING
    /* Allow insert,replace,update of historical data */
    UA_Boolean (*allowHistoryUpdateUpdateData)(UA_Server *server, UA_AccessControl *ac,
                                               const UA_NodeId *sessionId, void *sessionContext,
                                               const UA_NodeId *nodeId,
                                               UA_PerformUpdateType performInsertReplace,
                                               const UA_DataValue *value);

    /* Allow delete of historical data */
    UA_Boolean (*allowHistoryUpdateDeleteRawModified)(UA_Server *server, UA_AccessControl *ac,
                                                      const UA_NodeId *sessionId, void *sessionContext,
                                                      const UA_NodeId *nodeId,
                                                      UA_DateTime startTimestamp,
                                                      UA_DateTime endTimestamp,
                                                      bool isDeleteModified);
#endif
    UA_Boolean (*checkUserDatabase)(const UA_UserNameIdentityToken *userToken, UA_String *roleName);

#ifdef UA_ENABLE_ROLE_PERMISSIONS
    UA_StatusCode (*setRoleAccessPermission)(UA_String roleName, UA_AccessControlSettings* accessControlSettings);
    UA_PermissionType (*readUserDefinedRolePermission)(UA_Server *server, UA_AccessControlSettings* accessControlSettings);
    UA_StatusCode (*checkTheRoleSessionLoggedIn)(UA_Server *server);
#endif /* UA_ENABLE_ROLE_PERMISSIONS */
};

_UA_END_DECLS

#endif /* UA_PLUGIN_ACCESS_CONTROL_H_ */
