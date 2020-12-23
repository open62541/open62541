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
 * The lifecycle of the plugin is attached to a server or client config. The
 * ``deleteMembers`` method is called automatically when the config is
 * destroyed. */

struct UA_CertificateVerification;
typedef struct UA_CertificateVerification UA_CertificateVerification;

struct UA_CertificateVerification {
    void *context;

    /* Verify the certificate against the configured policies and trust chain. */
    UA_StatusCode (*verifyCertificate)(void *verificationContext,
                                       const UA_ByteString *certificate);

    /* Verify that the certificate has the applicationURI in the subject name. */
    UA_StatusCode (*verifyApplicationURI)(void *verificationContext,
                                          const UA_ByteString *certificate,
                                          const UA_String *applicationURI);

    /* Delete the certificate verification context */
    void (*deleteMembers)(UA_CertificateVerification *cv);
};

_UA_END_DECLS

#endif /* UA_PLUGIN_PKI_H_ */
