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

/* Example access control management. Anonymous and username / password login.
 * The access rights are maximally permissive. A logged-in user has
 * read/write/execute/nodemanagement access under the "Objects" folder (but not
 * in the "Server" object) and read/execute access everywhere else.
 *
 * FOR PRODUCTION USE, THIS EXAMPLE PLUGIN SHOULD BE REPLACED WITH LESS
 * PERMISSIVE ACCESS CONTROL.
 *
 * For TransferSubscriptions, we check whether the transfer happens between
 * Sessions for the same user. */

UA_EXPORT UA_StatusCode
UA_AccessControl_default(UA_ServerConfig *config, UA_Boolean allowAnonymous,
                         const UA_ByteString *userTokenPolicyUri,
                         size_t usernamePasswordLoginSize,
                         const UA_UsernamePasswordLogin *usernamePasswordLogin);

_UA_END_DECLS

#endif /* UA_ACCESSCONTROL_DEFAULT_H_ */
