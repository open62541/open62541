/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_SECURITYPOLICY_MBEDTLS_ECC_H_
#define UA_SECURITYPOLICY_MBEDTLS_ECC_H_

#include "securitypolicy_common.h"

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)

typedef struct {
    char *policyUri;
    char *symmetricSignatureUri;
    char *symmetricEncryptionUri;
    UA_UInt32 certificateTypeId;
    UA_Byte securityLevel;
    psa_ecc_family_t family;
    size_t keyBits;
    psa_algorithm_t hashAlgorithm;
    size_t hashLength;
    size_t symmetricEncryptionKeyLength;
    size_t asymmetricSignatureLength;
    size_t nonceLength;
    char *deprecatedName;
} UA_mbedTLS_EccPolicyConfig;

UA_StatusCode
UA_mbedTLS_SecurityPolicy_Ecc(UA_SecurityPolicy *policy,
                              UA_ApplicationType applicationType,
                              UA_ByteString localCertificate,
                              UA_ByteString localPrivateKey,
                              const UA_Logger *logger,
                              const UA_mbedTLS_EccPolicyConfig *config);

#endif

#endif /* UA_SECURITYPOLICY_MBEDTLS_ECC_H_ */
