/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef UA_SECURITYPOLICY_BASIC128RSA15_H_
#define UA_SECURITYPOLICY_BASIC128RSA15_H_

#include "ua_plugin_securitypolicy.h"
#include "ua_plugin_log.h"

_UA_BEGIN_DECLS

UA_EXPORT UA_StatusCode
UA_SecurityPolicy_Basic128Rsa15(UA_SecurityPolicy *policy,
                                UA_CertificateVerification *certificateVerification,
                                const UA_ByteString localCertificate,
                                const UA_ByteString localPrivateKey,
                                UA_Logger logger);

_UA_END_DECLS

#endif /* UA_SECURITYPOLICY_BASIC128RSA15_H_ */
