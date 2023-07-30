/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Kai Huebl)
 */

#ifndef UA_PLUGIN_CERTIFICATE_MANAGER_H_
#define UA_PLUGIN_CERTIFICATE_MANAGER_H_

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/plugin/certstore.h>

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
 * ``clear`` method is called automatically when the config is destroyed. */

struct UA_CertificateManager;
typedef struct UA_CertificateManager UA_CertificateManager;

struct UA_CertificateManager {
    /* Verify the certificate against the configured policies and trust chain. */
    UA_StatusCode (*verifyCertificate)(UA_CertificateManager *certificateManager,
    	    	                       UA_PKIStore *pkiStore,
                                       const UA_ByteString *certificate);

    /* Verify that the certificate has the applicationURI in the subject name. */
    UA_StatusCode (*verifyApplicationURI)(UA_CertificateManager *certificateManager,
    		                              UA_PKIStore *pkiStore,
                                          const UA_ByteString *certificate,
                                          const UA_String *applicationURI);

    /* Get the expire date from certificate */
    UA_StatusCode (*getExpirationDate)(UA_DateTime *expiryDateTime,
                                       UA_ByteString *certificate);

    /* Reloads the trust list from storage, discarding all unsaved changes. */
    UA_StatusCode (*reloadTrustList)(void *certificateManager);

    /* Create certificate signing request */
    UA_StatusCode (*createCertificateSigningRequest)(
    	UA_CertificateManager *certificateManager,
		UA_PKIStore* pkiStore,
		const UA_NodeId certificateTypeId,
        const UA_String *subject,
        const UA_ByteString *entropy,
        UA_ByteString **csr
	);

    /* Delete the certificate verification context */
    void (*clear)(UA_CertificateManager *certificateManager);
};

_UA_END_DECLS

#endif /* UA_PLUGIN_PKI_H_ */
