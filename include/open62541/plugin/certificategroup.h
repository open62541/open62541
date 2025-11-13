/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright (c) 2025 Pilz GmbH & Co. KG, Author: Marcel Patzlaff
 */

#ifndef UA_PLUGIN_CERTIFICATEGROUP_H
#define UA_PLUGIN_CERTIFICATEGROUP_H

#include <open62541/types.h>
#include <open62541/plugin/log.h>

_UA_BEGIN_DECLS

typedef enum
{
    /* Apply integrity checks that target only the given certificate:
     * * Certificate Structure (1)
     * * Certificate Usage (9)
     * Those two checks are always applied!*/
    UA_CERTIFICATEVERIFICATION_BASIC_INTEGRITY = 0,

    /* Additionally apply checks to ensure the integrity of the certificate and its chain:
     * * Build Certificate Chain (2)
     * * Signature (3)
     * * Security Policy Check (4)*/
    UA_CERTIFICATEVERIFICATION_INTEGRITY,

    /* Additionally apply checks to ensure the validity of the certificate:
     * * Validity Period (6)
     * * Find Revocation List (10)
     * * Revocation Check (11)*/
    UA_CERTIFICATEVERIFICATION_VALIDITY,

    /* Additionally apply checks to ensure that the certificate can be trusted:
     * * Host Name (7)
     * * URI (Application or Product URI check) (8)
     * * Trust List Check (5)*/
    UA_CERTIFICATEVERIFICATION_TRUST,
} UA_CertificateVerification;

typedef struct
{
    UA_Boolean allowInstanceUsage: 1;
    UA_Boolean allowIssuerUsage: 1;
    UA_Boolean allowUserUsage: 1;
    UA_CertificateVerification verificationLevel: 2;
} UA_CertificateVerificationSettings;

struct UA_CertificateGroup;
typedef struct UA_CertificateGroup UA_CertificateGroup;

/**
 * CertificateGroup Plugin API
 * ---------------------------
 *
 * This plugin verifies that the origin of the certificate is trusted. It does
 * not assign any access rights/roles to the holder of the certificate.
 *
 * Usually, implementations of the CertificateGroup plugin provide an
 * initialization method that takes a trust-list and a revocation-list as input.
 * This initialization method takes a pointer to the plugin location and therein
 * calls the ``clear`` method if valid, before attempting to initialize it anew.
 * The lifecycle of the plugin is attached to a server or client config. The
 * ``clear`` method is called automatically when the config is destroyed. */

struct UA_CertificateGroup {
    /* The NodeId of the certificate group this pki store is associated with */
    UA_NodeId certificateGroupId;

    /* Context-pointer to be set by the CertificateGroup plugin implementation */
    void *context;

    /* Pointer to logging pointer in the server/client configuration. If the
     * logging pointer is changed outside of the plugin, the new logger is used
     * automatically. */
    const UA_Logger *logging;

    UA_StatusCode (*getTrustList)(UA_CertificateGroup *certGroup,
                                  UA_TrustListDataType *trustList);

    UA_StatusCode (*setTrustList)(UA_CertificateGroup *certGroup,
                                  const UA_TrustListDataType *trustList);

    UA_StatusCode (*addToTrustList)(UA_CertificateGroup *certGroup,
                                    const UA_TrustListDataType *trustList);

    UA_StatusCode (*removeFromTrustList)(UA_CertificateGroup *certGroup,
                                         const UA_TrustListDataType *trustList);

    UA_StatusCode (*getRejectedList)(UA_CertificateGroup *certGroup,
                                     UA_ByteString **rejectedList,
                                     size_t *rejectedListSize);

    /* Provides all associated CRLs for a CA certificate. */
    UA_StatusCode (*getCertificateCrls)(UA_CertificateGroup *certGroup,
                                        const UA_ByteString *certificate,
                                        const UA_Boolean isTrusted,
                                        UA_ByteString **crls,
                                        size_t *crlsSize);

    UA_StatusCode (*verifyCertificate)(UA_CertificateGroup *certGroup,
                                       const UA_ByteString *certificate,
                                       UA_CertificateVerificationSettings settings);

    void (*clear)(UA_CertificateGroup *certGroup);
};

/* Verify that the certificate has the applicationURI in the subject name. */
UA_EXPORT UA_StatusCode
UA_CertificateUtils_verifyApplicationURI(UA_RuleHandling ruleHandling,
                                         const UA_ByteString *certificate,
                                         const UA_String *applicationURI,
                                         UA_Logger *logger);

/* Get the expire date from certificate */
UA_EXPORT UA_StatusCode
UA_CertificateUtils_getExpirationDate(UA_ByteString *certificate,
                                      UA_DateTime *expiryDateTime);

UA_EXPORT UA_StatusCode
UA_CertificateUtils_getSubjectName(UA_ByteString *certificate,
                                   UA_String *subjectName);

UA_EXPORT UA_StatusCode
UA_CertificateUtils_getThumbprint(UA_ByteString *certificate,
                                  UA_String *thumbprint);

UA_EXPORT UA_StatusCode
UA_CertificateUtils_getKeySize(UA_ByteString *certificate,
                               size_t *keySize);

/* Compares the public keys from two byte strings, which can represent either
 * certificates or Certificate Signing Requests (CSR). This function extracts
 * the public keys from the provided byte strings and compares them to determine
 * if they are identical.
 *
 * @param certificate1 Containing either a certificate or a CSR.
 * @param certificate2 Containing either a certificate or a CSR.
 * @return UA_STATUSCODE_GOOD if the public keys are identical,
 *         UA_STATUSCODE_BADNOMATCH if the public keys do not match,
 *         UA_STATUSCODE_BADINTERNALERROR if an error occurs. */
UA_EXPORT UA_StatusCode
UA_CertificateUtils_comparePublicKeys(const UA_ByteString *certificate1,
                                      const UA_ByteString *certificate2);

UA_EXPORT UA_StatusCode
UA_CertificateUtils_checkKeyPair(const UA_ByteString *certificate,
                                 const UA_ByteString *privateKey);

UA_EXPORT UA_StatusCode
UA_CertificateUtils_checkCA(const UA_ByteString *certificate);

/* Decrypt a private key in PEM format using a password. The output is the key
 * in the binary DER format. Also succeeds if the PEM private key does not
 * require a password or is already in the DER format. The outDerKey memory is
 * allocated internally.
 *
 * Returns UA_STATUSCODE_BADSECURITYCHECKSFAILED if the password is wrong. */
UA_EXPORT UA_StatusCode
UA_CertificateUtils_decryptPrivateKey(const UA_ByteString privateKey,
                                      const UA_ByteString password,
                                      UA_ByteString *outDerKey);

_UA_END_DECLS

#endif /* UA_PLUGIN_CERTIFICATEGROUP_H */
