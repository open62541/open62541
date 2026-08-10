/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_SECURITYPOLICY_MBEDTLS_RSA_H_
#define UA_SECURITYPOLICY_MBEDTLS_RSA_H_

#include "securitypolicy_common.h"

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)

typedef struct {
    char *policyUri;
    char *asymmetricSignatureUri;
    char *asymmetricEncryptionUri;
    char *symmetricSignatureUri;
    char *symmetricEncryptionUri;
    char *certificateSignatureUri;
    UA_UInt32 certificateTypeId;
    UA_Byte securityLevel;
    psa_algorithm_t hashAlgorithm;
    psa_algorithm_t signatureAlgorithm;
    psa_algorithm_t encryptionAlgorithm;
    psa_algorithm_t certificateSignatureAlgorithm;
    size_t asymmetricPaddingOverhead;
    size_t hashLength;
    size_t symmetricSigningKeyLength;
    size_t symmetricEncryptionKeyLength;
    size_t nonceLength;
    size_t minimumAsymmetricKeyLength;
    size_t maximumAsymmetricKeyLength;
    char *deprecatedName;
} UA_mbedTLS_RsaPolicyConfig;

UA_StatusCode
UA_mbedTLS_SecurityPolicy_Rsa(UA_SecurityPolicy *policy,
                              UA_ByteString localCertificate,
                              UA_ByteString localPrivateKey,
                              const UA_Logger *logger,
                              const UA_mbedTLS_RsaPolicyConfig *config);

#endif

#endif /* UA_SECURITYPOLICY_MBEDTLS_RSA_H_ */
