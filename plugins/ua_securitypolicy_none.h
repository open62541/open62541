/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_SECURITYPOLICY_NONE_H_
#define UA_SECURITYPOLICY_NONE_H_

#include "ua_plugin_securitypolicy.h"

_UA_BEGIN_DECLS

UA_StatusCode UA_EXPORT
UA_SecurityPolicy_None(UA_SecurityPolicy *policy, UA_CertificateVerification *certificateVerification,
                       const UA_ByteString localCertificate, UA_Logger logger);

_UA_END_DECLS

#endif /* UA_SECURITYPOLICY_NONE_H_ */
