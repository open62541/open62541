/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_PLUGIN_ACCESS_CONTROL_H_
#define UA_PLUGIN_ACCESS_CONTROL_H_

#include <open62541/util.h>

_UA_BEGIN_DECLS

struct UA_AccessControl;
typedef struct UA_AccessControl UA_AccessControl;

/**
 * .. _access-control:
 *
 * AccessControl Plugin API
 * ========================
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
     * SecureChannel.
     *
     * The following checks are performed in the server before calling
     * activateSession:
     *
     * - Select matching Endpoint/UserTokenPolicy (compare token type,
     *   SecureChannel and PolicyId from the UserIdentityToken)
     * - Cryptographic checks:
     *   - Check the encryption algortihm from the UserIdentityToken
     *   - UsernamePassword/IssuedToken: Decrypt the secret
     *   - Check the x509 auth certificate signature and validate the
     *     certificate against the server's sessionPKI */
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
                               const UA_AddNodesItem *item);

    /* Allow adding a reference */
    UA_Boolean (*allowAddReference)(UA_Server *server, UA_AccessControl *ac,
                                    const UA_NodeId *sessionId, void *sessionContext,
                                    const UA_AddReferencesItem *item);

    /* Allow deleting a node */
    UA_Boolean (*allowDeleteNode)(UA_Server *server, UA_AccessControl *ac,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_DeleteNodesItem *item);

    /* Allow deleting a reference */
    UA_Boolean (*allowDeleteReference)(UA_Server *server, UA_AccessControl *ac,
                                       const UA_NodeId *sessionId, void *sessionContext,
                                       const UA_DeleteReferencesItem *item);

    /* Allow browsing a node */
    UA_Boolean (*allowBrowseNode)(UA_Server *server, UA_AccessControl *ac,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *nodeId, void *nodeContext);

#ifdef UA_ENABLE_RBAC
    /* Return the GroupIds the session's user belongs to, used for the GroupId
     * identity mapping criterion (OPC UA Part 18 §4.4.2). Optional; may be NULL,
     * in which case GroupId criteria never match. The groups are captured at
     * ActivateSession. On success the callback allocates *groupIds (e.g. with
     * UA_Array_new of UA_String) and ownership is transferred to the caller. */
    UA_StatusCode (*getUserGroups)(UA_Server *server, UA_AccessControl *ac,
                                   const UA_NodeId *sessionId, void *sessionContext,
                                   UA_String **groupIds, size_t *groupIdsSize);

    /* Return validated Role claims from an IssuedIdentityToken for the Role
     * identity mapping criterion (OPC UA Part 18 section 4.4.2). Optional; the
     * server calls this only after activateSession has accepted an issued
     * token. For JWT, values use "<iss>/<role>" when an issuer exists and
     * "<role>" otherwise. On success ownership of the allocated array and its
     * strings is transferred to the caller. */
    UA_StatusCode (*getUserTokenRoles)(UA_Server *server, UA_AccessControl *ac,
                                       const UA_NodeId *sessionId,
                                       void *sessionContext,
                                       UA_String **roleClaims,
                                       size_t *roleClaimsSize);

    /* Optional Part 18 UserManagement provider. The core exposes the
     * UserManagement Object only when all mutation callbacks are configured.
     * Provider implementations own password hashing, persistence and rate
     * limiting. Returned Users arrays transfer ownership to the core. */
    UA_StatusCode (*getUsers)(UA_Server *server, UA_AccessControl *ac,
                              UA_UserManagementDataType **users,
                              size_t *usersSize);
    UA_StatusCode (*getPasswordPolicy)(UA_Server *server, UA_AccessControl *ac,
                                       UA_Range *passwordLength,
                                       UA_PasswordOptionsMask *passwordOptions,
                                       UA_LocalizedText *passwordRestrictions);
    UA_StatusCode (*getUserConfiguration)(UA_Server *server,
                                          UA_AccessControl *ac,
                                          const UA_String *userName,
                                          UA_UserConfigurationMask *configuration);
    UA_StatusCode (*addUser)(UA_Server *server, UA_AccessControl *ac,
                             const UA_String *userName,
                             const UA_String *password,
                             UA_UserConfigurationMask configuration,
                             const UA_String *description);
    UA_StatusCode (*modifyUser)(UA_Server *server, UA_AccessControl *ac,
                                const UA_String *userName,
                                UA_Boolean modifyPassword,
                                const UA_String *password,
                                UA_Boolean modifyConfiguration,
                                UA_UserConfigurationMask configuration,
                                UA_Boolean modifyDescription,
                                const UA_String *description);
    UA_StatusCode (*removeUser)(UA_Server *server, UA_AccessControl *ac,
                                const UA_String *userName);
    UA_StatusCode (*changePassword)(UA_Server *server, UA_AccessControl *ac,
                                    const UA_String *userName,
                                    const UA_String *oldPassword,
                                    const UA_String *newPassword);
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Allow creating a subscription */
    UA_Boolean (*allowCreateSubscription)(UA_Server *server, UA_AccessControl *ac,
                                          const UA_NodeId *sessionId, void *sessionContext);

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
};

_UA_END_DECLS

#endif /* UA_PLUGIN_ACCESS_CONTROL_H_ */
