/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef OPEN62541_TESTING_POLICY_H
#define OPEN62541_TESTING_POLICY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_plugin_securitypolicy.h"
#include "ua_plugin_log.h"

typedef struct funcs_called {
    bool asym_enc;
    bool asym_dec;

    bool sym_enc;
    bool sym_dec;

    bool asym_sign;
    bool asym_verify;

    bool sym_sign;
    bool sym_verify;

    bool newContext;
    bool deleteContext;

    bool makeCertificateThumbprint;
    bool generateKey;

    bool setLocalSymEncryptingKey;
    bool setLocalSymSigningKey;
    bool setLocalSymIv;
    bool setRemoteSymEncryptingKey;
    bool setRemoteSymSigningKey;
    bool setRemoteSymIv;
} funcs_called;

UA_StatusCode UA_EXPORT
TestingPolicy(UA_SecurityPolicy *policy, const UA_ByteString localCertificate,
              funcs_called *fCalled);

#ifdef __cplusplus
}
#endif

#endif //OPEN62541_TESTING_POLICY_H
