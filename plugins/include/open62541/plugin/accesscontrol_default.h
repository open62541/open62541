/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_ACCESSCONTROL_DEFAULT_H_
#define UA_ACCESSCONTROL_DEFAULT_H_

#include <open62541/plugin/accesscontrol.h>
#include <open62541/server.h>

_UA_BEGIN_DECLS

typedef struct {
    UA_String username;
    UA_String password;
} UA_UsernamePasswordLogin;

typedef UA_StatusCode (*UA_UsernamePasswordLoginCallback)
    (const UA_String *userName, const UA_ByteString *password,
    size_t usernamePasswordLoginSize, const UA_UsernamePasswordLogin
    *usernamePasswordLogin, void **sessionContext, void *loginContext);

/* Default access control. The login can be anonymous, username-password or
 * certificate-based. A logged-in user has all access rights.
 *
 * The plugin stores the UserIdentityToken in the session context. So that
 * cannot be used for other purposes.
 *
 * The certificate verification plugin lifecycle is moved to the access control
 * system. So it is cleared up eventually together with the AccessControl. */
UA_EXPORT UA_StatusCode
UA_AccessControl_default(UA_ServerConfig *config,
                         UA_Boolean allowAnonymous,
                         const UA_ByteString *userTokenPolicyUri,
                         size_t usernamePasswordLoginSize,
                         const UA_UsernamePasswordLogin *usernamePasswordLogin);

UA_EXPORT UA_StatusCode
UA_AccessControl_defaultWithLoginCallback(UA_ServerConfig *config,
                                          UA_Boolean allowAnonymous,
                                          const UA_ByteString *userTokenPolicyUri,
                                          size_t usernamePasswordLoginSize,
                                          const UA_UsernamePasswordLogin *usernamePasswordLogin,
                                          UA_UsernamePasswordLoginCallback loginCallback,
                                          void *loginContext);

_UA_END_DECLS

#endif /* UA_ACCESSCONTROL_DEFAULT_H_ */
