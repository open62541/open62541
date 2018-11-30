/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2018 (c) Daniel Feist, Precitec GmbH & Co. KG
 */

#ifndef UA_SECURITYPOLICY_BASIC256SHA256_H_
#define UA_SECURITYPOLICY_BASIC256SHA256_H_

#include "ua_config.h"

#ifdef UA_ENABLE_ENCRYPTION

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_plugin_securitypolicy.h"
#include "ua_plugin_log.h"

UA_EXPORT UA_StatusCode
UA_SecurityPolicy_Basic256Sha256(UA_SecurityPolicy *policy,
                                 UA_CertificateVerification *certificateVerification,
                                 const UA_ByteString localCertificate,
                                 const UA_ByteString localPrivateKey,
                                 UA_Logger logger);

#ifdef __cplusplus
}
#endif

#endif /* UA_ENABLE_ENCRYPTION */

#endif // UA_SECURITYPOLICY_BASIC256SHA256_H_
