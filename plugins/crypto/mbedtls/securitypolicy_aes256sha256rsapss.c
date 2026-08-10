/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/securitypolicy_default.h>

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)

#include "securitypolicy_rsa.h"

static const UA_mbedTLS_RsaPolicyConfig rsaAes256Sha256RsaPssConfig = {
    "http://opcfoundation.org/UA/SecurityPolicy#Aes256_Sha256_RsaPss",
    "http://opcfoundation.org/UA/security/rsa-pss-sha2-256",
    "http://opcfoundation.org/UA/security/rsa-oaep-sha2-256",
    "http://www.w3.org/2000/09/xmldsig#hmac-sha2-256",
    "http://www.w3.org/2001/04/xmlenc#aes256-cbc",
    "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256",
    UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE,
    30,
    PSA_ALG_SHA_256,
    PSA_ALG_RSA_PSS(PSA_ALG_SHA_256),
    PSA_ALG_RSA_OAEP(PSA_ALG_SHA_256),
    PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256),
    66,
    32,
    32,
    32,
    32,
    256,
    512,
    NULL
};

UA_StatusCode
UA_SecurityPolicy_Aes256Sha256RsaPss(UA_SecurityPolicy *policy,
                              const UA_ByteString localCertificate,
                              const UA_ByteString localPrivateKey,
                              const UA_Logger *logger) {
    return UA_mbedTLS_SecurityPolicy_Rsa(
        policy, localCertificate, localPrivateKey, logger, &rsaAes256Sha256RsaPssConfig);
}

#endif
