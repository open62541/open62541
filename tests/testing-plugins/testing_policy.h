/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef OPEN62541_TESTING_POLICY_H
#define OPEN62541_TESTING_POLICY_H

#include <open62541/plugin/log.h>
#include <open62541/plugin/securitypolicy.h>

_UA_BEGIN_DECLS

typedef struct funcs_called {
    UA_Boolean asym_enc;
    UA_Boolean asym_dec;

    UA_Boolean sym_enc;
    UA_Boolean sym_dec;

    UA_Boolean asym_sign;
    UA_Boolean asym_verify;

    UA_Boolean sym_sign;
    UA_Boolean sym_verify;

    UA_Boolean newContext;
    UA_Boolean deleteContext;

    UA_Boolean makeCertificateThumbprint;
    UA_Boolean generateKey;
    UA_Boolean generateNonce;

    UA_Boolean setLocalSymEncryptingKey;
    UA_Boolean setLocalSymSigningKey;
    UA_Boolean setLocalSymIv;
    UA_Boolean setRemoteSymEncryptingKey;
    UA_Boolean setRemoteSymSigningKey;
    UA_Boolean setRemoteSymIv;
} funcs_called;

typedef struct key_sizes {
    size_t sym_enc_blockSize;
    size_t sym_sig_keyLen;
    size_t sym_sig_size;
    size_t sym_enc_keyLen;

    size_t asym_rmt_sig_size;
    size_t asym_lcl_sig_size;
    size_t asym_rmt_ptext_blocksize;
    size_t asym_rmt_blocksize;
    size_t asym_rmt_enc_key_size;
    size_t asym_lcl_enc_key_size;
} key_sizes;

UA_StatusCode UA_EXPORT
TestingPolicy(UA_SecurityPolicy *policy, UA_ByteString localCertificate,
              funcs_called *fCalled, const key_sizes *kSizes);

_UA_END_DECLS

#endif //OPEN62541_TESTING_POLICY_H
