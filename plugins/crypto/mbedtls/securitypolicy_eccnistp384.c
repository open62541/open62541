/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/securitypolicy_default.h>

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)

#include "securitypolicy_ecc.h"

static const UA_mbedTLS_EccPolicyConfig eccNistP384Config = {
    "http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384",
    "http://www.w3.org/2000/09/xmldsig#hmac-sha2-384",
    "http://www.w3.org/2001/04/xmlenc#aes256-cbc",
    UA_NS0ID_ECCNISTP384APPLICATIONCERTIFICATETYPE,
    15,
    PSA_ECC_FAMILY_SECP_R1,
    384,
    PSA_ALG_SHA_384,
    48,
    32,
    96,
    96,
    "ECC_nistP384"
};

UA_StatusCode
UA_SecurityPolicy_EccNistP384(UA_SecurityPolicy *policy,
                              UA_ApplicationType applicationType,
                              UA_ByteString localCertificate,
                              UA_ByteString localPrivateKey,
                              const UA_Logger *logger) {
    return UA_mbedTLS_SecurityPolicy_Ecc(
        policy, applicationType, localCertificate, localPrivateKey,
        logger, &eccNistP384Config);
}

#endif
