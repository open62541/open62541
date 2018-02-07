#ifndef UA_SECURITYPOLICY_BASIC256SHA256_H_
#define UA_SECURITYPOLICY_BASIC256SHA256_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_plugin_securitypolicy.h"
#include "ua_plugin_log.h"

UA_EXPORT UA_StatusCode
UA_SecurityPolicy_Basic256Sha256(UA_SecurityPolicy *policy,
                                const UA_ByteString localCertificate,
                                const UA_ByteString localPrivateKey,
                                UA_Logger logger);

#ifdef __cplusplus
}
#endif

#endif // UA_SECURITYPOLICY_BASIC256SHA256_H_
