/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 */

#ifndef UA_ACCESSCONTROL_CUSTOM_H_
#define UA_ACCESSCONTROL_CUSTOM_H_

#include <open62541/plugin/accesscontrol.h>
#include <open62541/server.h>

#define ANONYMOUS_POLICY                  "open62541-anonymous-policy"
#define CERTIFICATE_POLICY                "open62541-certificate-policy"
#define USERNAME_POLICY                   "open62541-username-policy"
#define ANONYMOUS_WELL_KNOWN_RULE         "Anonymous"
#define AUTHENTICATEDUSER_WELL_KNOWN_RULE "AuthenticatedUser"
#define CONFIGUREADMIN_WELL_KNOWN_RULE    "ConfigureAdmin"
#define ENGINEER_WELL_KNOWN_RULE          "Engineer"
#define OBSERVER_WELL_KNOWN_RULE          "Observer"
#define OPERATOR_WELL_KNOWN_RULE          "Operator"
#define SECURITYADMIN_WELL_KNOWN_RULE     "SecurityAdmin"
#define SUPERVISOR_WELL_KNOWN_RULE        "Supervisor"

_UA_BEGIN_DECLS

typedef struct {
    UA_String username;
    UA_String password;
} UA_UsernamePasswordLogin;

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
