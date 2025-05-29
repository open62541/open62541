/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef UA_PLUGIN_PKI_H_
#define UA_PLUGIN_PKI_H_

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/plugin/log.h>

_UA_BEGIN_DECLS

/**
 * Public Key Infrastructure Integration
 * =====================================
 * This file contains interface definitions for integration in a Public Key
 * Infrastructure (PKI). Currently only one plugin interface is defined.
 *
 * Certificate Verification
 * ------------------------
 * This plugin verifies that the origin of the certificate is trusted. It does
 * not assign any access rights/roles to the holder of the certificate.
 *
 * Usually, implementations of the certificate verification plugin provide an
 * initialization method that takes a trust-list and a revocation-list as input.
 * This initialization method should call the ``clear`` method if valid, before
 * attempting to initialize it with new values.
 * The lifecycle of the plugin is attached to a server or client config. The
 * ``clear`` method is called automatically when the config is destroyed. */

struct UA_CertificateVerification;
typedef struct UA_CertificateVerification UA_CertificateVerification;

struct UA_CertificateVerification {
    void *context;

    /* Verify the certificate against the configured policies and trust chain. */
    UA_StatusCode (*verifyCertificate)(const UA_CertificateVerification *cv,
                                       const UA_ByteString *certificate);

    /* Verify that the certificate has the applicationURI in the subject name. */
    UA_StatusCode (*verifyApplicationURI)(const UA_CertificateVerification *cv,
                                          const UA_ByteString *certificate,
                                          const UA_String *applicationURI);

    /* Get the expire date from certificate */
    UA_StatusCode (*getExpirationDate)(UA_DateTime *expiryDateTime, 
                                       UA_ByteString *certificate);

    UA_StatusCode (*getSubjectName)(UA_String *subjectName,
                                    UA_ByteString *certificate);

    /* Delete the certificate verification context */
    void (*clear)(UA_CertificateVerification *cv);

    /* Pointer to logging pointer in the server/client configuration. If the
     * logging pointer is changed outside of the plugin, the new logger is used
     * automatically*/
    const UA_Logger *logging;
};

/* Decrypt a private key in PEM format using a password. The output is the key
 * in the binary DER format. Also succeeds if the PEM private key does not
 * require a password or is already in the DER format. The outDerKey memory is
 * allocated internally.
 *
 * Returns UA_STATUSCODE_BADSECURITYCHECKSFAILED if the password is wrong. */
UA_EXPORT UA_StatusCode
UA_PKI_decryptPrivateKey(const UA_ByteString privateKey,
                         const UA_ByteString password,
                         UA_ByteString *outDerKey);

_UA_END_DECLS

#endif /* UA_PLUGIN_PKI_H_ */
