/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "gds_certificates.h"

#if defined(UA_ENABLE_DRIVER_GDS_RECEIVER) || defined(UA_ENABLE_DRIVER_GDS_PULL)

#define GDS_MAX_ENDPOINTS 32

UA_CertificateGroup *
UA_GDS_getCertificateGroup(UA_ServerConfig *sc,
                           const UA_NodeId *certificateGroupId) {
    UA_NodeId applicationGroup =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId userTokenGroup =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);
    if(UA_NodeId_equal(certificateGroupId, &applicationGroup))
        return &sc->secureChannelPKI;
    if(UA_NodeId_equal(certificateGroupId, &userTokenGroup))
        return &sc->sessionPKI;
    return NULL;
}

UA_SecurityPolicy *
UA_GDS_getSecPolicyByUri(UA_ServerConfig *sc, const UA_String *securityPolicyUri) {
    for(size_t i = 0; i < sc->securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &sc->securityPolicies[i];
        if(UA_String_equal(securityPolicyUri, &sp->policyUri))
            return sp;
    }
    return NULL;
}

UA_StatusCode
UA_GDS_applyCertificateToPolicies(UA_ServerConfig *sc,
                                  const UA_NodeId *certificateTypeId,
                                  const UA_ByteString certificate,
                                  const UA_ByteString privateKey) {
    if(sc->endpointsSize > GDS_MAX_ENDPOINTS) {
        UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Cannot update the certificate for more than %u endpoints",
                     (unsigned)GDS_MAX_ENDPOINTS);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    UA_SecurityPolicy *policies[GDS_MAX_ENDPOINTS];
    size_t policiesSize = 0;
    UA_ByteString endpointCertificates[GDS_MAX_ENDPOINTS] = {0};
    UA_Boolean updateEndpoint[GDS_MAX_ENDPOINTS] = {0};

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < sc->endpointsSize; i++) {
        UA_EndpointDescription *ed = &sc->endpoints[i];
        UA_SecurityPolicy *sp = UA_GDS_getSecPolicyByUri(sc, &ed->securityPolicyUri);
        if(!sp) {
            res = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
        if(!UA_NodeId_equal(&sp->certificateTypeId, certificateTypeId))
            continue;

        res = UA_ByteString_copy(&certificate, &endpointCertificates[i]);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
        updateEndpoint[i] = true;

        size_t j = 0;
        for(; j < policiesSize; j++) {
            if(policies[j] == sp)
                break;
        }
        if(j == policiesSize)
            policies[policiesSize++] = sp;
    }

    /* Endpoint resolution and allocations cannot fail from here onwards. */
    for(size_t i = 0; i < policiesSize; i++) {
        res = policies[i]->updateCertificate(policies[i], certificate,
                                              privateKey);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Updating the certificate failed after %u of %u "
                         "SecurityPolicies were updated",
                         (unsigned)i, (unsigned)policiesSize);
            goto cleanup;
        }
    }

    /* Commit the preallocated endpoint certificates without further errors. */
    for(size_t i = 0; i < sc->endpointsSize; i++) {
        if(!updateEndpoint[i])
            continue;
        UA_ByteString_clear(&sc->endpoints[i].serverCertificate);
        sc->endpoints[i].serverCertificate = endpointCertificates[i];
        endpointCertificates[i] = UA_BYTESTRING_NULL;
    }

cleanup:
    for(size_t i = 0; i < sc->endpointsSize; i++)
        UA_ByteString_clear(&endpointCertificates[i]);
    return res;
}

#endif /* UA_ENABLE_DRIVER_GDS_RECEIVER || UA_ENABLE_DRIVER_GDS_PULL */
