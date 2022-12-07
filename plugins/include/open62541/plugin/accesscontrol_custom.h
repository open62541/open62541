/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 */

#ifndef UA_ACCESSCONTROL_CUSTOM_H_
#define UA_ACCESSCONTROL_CUSTOM_H_

#include <open62541/plugin/accesscontrol.h>
#include <open62541/server.h>

_UA_BEGIN_DECLS

typedef struct {
    UA_String username;
    UA_String password;
} UA_UsernamePasswordLogin;

#ifdef UA_ENABLE_ROLE_PERMISSION

typedef enum {
    UA_ANONYMOUS_WELL_KNOWN_RULE = 0,
    UA_AUTHENTICATEDUSER_WELL_KNOWN_RULE = 1,
    UA_CONFIGUREADMIN_WELL_KNOWN_RULE = 2,
    UA_ENGINEER_WELL_KNOWN_RULE = 3,
    UA_OBSERVER_WELL_KNOWN_RULE = 4,
    UA_OPERATOR_WELL_KNOWN_RULE = 5,
    UA_SECURITYADMIN_WELL_KNOWN_RULE = 6,
    UA_SUPERVISOR_WELL_KNOWN_RULE = 7,
}UA_AccessControlGroup;

typedef struct {
    UA_AccessControlGroup  accessControlGroup;
    UA_UInt32              accessPermissions;
    UA_Boolean             methodAccessPermission;
} UA_AccessControlSettings;

typedef struct {
    UA_String *username;
    UA_String *rolename;
    UA_AccessControlSettings *accessControlSettings;
} UA_UsernameRoleInfo;
#endif
/* Default access control. The log-in can be anonymous or username-password. A
 * logged-in user has all access rights.
 *
 * The certificate verification plugin lifecycle is moved to the access control
 * system. So it is cleared up eventually together with the AccessControl. */
UA_EXPORT UA_StatusCode
UA_AccessControl_custom(UA_ServerConfig *config,
                         UA_Boolean allowAnonymous,
                         UA_CertificateVerification *verifyX509,
                         const UA_ByteString *userTokenPolicyUri,
                         size_t usernamePasswordLoginSize,
                         const UA_UsernamePasswordLogin *usernamePasswordLogin);

_UA_END_DECLS

#endif /* UA_ACCESSCONTROL_CUSTOM_H_ */
