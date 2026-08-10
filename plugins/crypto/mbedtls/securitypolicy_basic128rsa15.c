/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/securitypolicy_default.h>

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)

#include "securitypolicy_rsa.h"

static const UA_mbedTLS_RsaPolicyConfig rsaBasic128Rsa15Config = {
    "http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15",
    "http://www.w3.org/2000/09/xmldsig#rsa-sha1",
    "http://www.w3.org/2001/04/xmlenc#rsa-1_5",
    "http://www.w3.org/2000/09/xmldsig#hmac-sha1",
    "http://www.w3.org/2001/04/xmlenc#aes128-cbc",
    "http://www.w3.org/2000/09/xmldsig#rsa-sha1",
    UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE,
    0,
    PSA_ALG_SHA_1,
    PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_1),
    PSA_ALG_RSA_PKCS1V15_CRYPT,
    PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_1),
    11,
    20,
    16,
    16,
    16,
    128,
    512,
    "Basic128Rsa15"
};

UA_StatusCode
UA_SecurityPolicy_Basic128Rsa15(UA_SecurityPolicy *policy,
                              const UA_ByteString localCertificate,
                              const UA_ByteString localPrivateKey,
                              const UA_Logger *logger) {
    return UA_mbedTLS_SecurityPolicy_Rsa(
        policy, localCertificate, localPrivateKey, logger, &rsaBasic128Rsa15Config);
}

#endif
