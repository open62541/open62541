/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/securitypolicy_default.h>

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)

#include "securitypolicy_ecc.h"

static const UA_mbedTLS_EccPolicyConfig eccNistP256Config = {
    "http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256",
    "http://www.w3.org/2000/09/xmldsig#hmac-sha2-256",
    "http://www.w3.org/2001/04/xmlenc#aes128-cbc",
    UA_NS0ID_ECCNISTP256APPLICATIONCERTIFICATETYPE,
    10,
    PSA_ECC_FAMILY_SECP_R1,
    256,
    PSA_ALG_SHA_256,
    32,
    16,
    64,
    64,
    "ECC_nistP256"
};

UA_StatusCode
UA_SecurityPolicy_EccNistP256(UA_SecurityPolicy *policy,
                              UA_ApplicationType applicationType,
                              UA_ByteString localCertificate,
                              UA_ByteString localPrivateKey,
                              const UA_Logger *logger) {
    return UA_mbedTLS_SecurityPolicy_Ecc(
        policy, applicationType, localCertificate, localPrivateKey,
        logger, &eccNistP256Config);
}

#endif
