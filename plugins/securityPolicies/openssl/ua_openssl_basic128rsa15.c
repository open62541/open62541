/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Wind River Systems, Inc.
 */

/*
modification history
--------------------
01feb20,lan  written
*/

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#ifdef UA_ENABLE_ENCRYPTION_OPENSSL

/* clear the policy context */

static void
UA_Policy_Clear_Context (UA_SecurityPolicy *policy) {
}

UA_StatusCode
UA_SecurityPolicy_Basic128Rsa15(UA_SecurityPolicy *policy,
                                const UA_ByteString localCertificate,
                                const UA_ByteString localPrivateKey, 
                                const UA_Logger *logger) {
    policy->policyUri = UA_STRING("Obsolete, not supported\0");  
    policy->clear = UA_Policy_Clear_Context;
    return UA_STATUSCODE_BADNOTSUPPORTED;
}

#endif
