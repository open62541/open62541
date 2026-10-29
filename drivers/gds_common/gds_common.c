/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Bastian Engel)
 */

#include "gds_common.h"

#if defined(UA_ENABLE_DRIVER_GDS_RECEIVER) || defined(UA_ENABLE_DRIVER_GDS_PULL)

#include <string.h>

#define GDS_MAX_ENDPOINTS 32

static const char pemHeader[] = "-----BEGIN CERTIFICATE-----";
static const char pemFooter[] = "-----END CERTIFICATE-----";

/* Returns the position of the first match at or after start, or the length of
 * the ByteString if there is no match */
static size_t
findString(const UA_ByteString bs, size_t start, const char *str) {
    size_t strLen = strlen(str);
    for(size_t i = start; i + strLen <= bs.length; i++) {
        if(memcmp(&bs.data[i], str, strLen) == 0)
            return i;
    }
    return bs.length;
}

UA_ByteString
UA_GDSReceiver_getLeafCertificate(UA_ByteString chain) {
    /* Detect DER encoded X.509 v3 certificate. The certificate length is
     * encoded in the two bytes after the magic numbers. */
    if(chain.length >= 4 && chain.data[0] == 0x30 && chain.data[1] == 0x82) {
        size_t leafLen = 4; /* Magic numbers + length bytes */
        leafLen += (size_t)(((uint16_t)chain.data[2]) << 8);
        leafLen += chain.data[3];
        if(leafLen > chain.length)
            return UA_BYTESTRING_NULL;
        chain.length = leafLen;
        return chain;
    }

    /* Detect PEM. The leaf starts at the first header, so that the crypto
     * backends detect PEM by the first character. Text before the header is
     * cut off. */
    size_t begin = findString(chain, 0, pemHeader);
    if(begin == chain.length)
        return chain;
    size_t end = findString(chain, begin, pemFooter);
    if(end == chain.length)
        return UA_BYTESTRING_NULL;

    /* Keep the line break after the footer */
    end += strlen(pemFooter);
    if(end < chain.length && chain.data[end] == '\r')
        end++;
    if(end < chain.length && chain.data[end] == '\n')
        end++;

    chain.data += begin;
    chain.length = end - begin;
    return chain;
}

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

    /* No endpoint uses the certificate type. The update would have no
     * effect. */
    if(policiesSize == 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* A SecurityPolicy#None endpoint presents the certificate of the None
     * SecurityPolicy, which is usually the replaced certificate. Update it as
     * well, so that all endpoints present the same certificate. */
    for(size_t i = 0; i < sc->endpointsSize; i++) {
        if(updateEndpoint[i])
            continue;
        UA_SecurityPolicy *sp =
            UA_GDS_getSecPolicyByUri(sc, &sc->endpoints[i].securityPolicyUri);
        if(!sp || sp->policyType != UA_SECURITYPOLICYTYPE_NONE ||
           sp->localCertificate.length == 0)
            continue;

        UA_Boolean replaced = false;
        size_t j = 0;
        for(; j < policiesSize; j++) {
            if(policies[j] == sp)
                break;
            if(UA_ByteString_equal(&sp->localCertificate,
                                   &policies[j]->localCertificate))
                replaced = true;
        }
        if(j == policiesSize && !replaced)
            continue;

        res = UA_ByteString_copy(&certificate, &endpointCertificates[i]);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
        updateEndpoint[i] = true;
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

static void
removeChannel(UA_GDSChannel *ch) {
    LIST_REMOVE(ch, pointers);
    UA_NodeId_clear(&ch->certificateTypeId);
    UA_ByteString_clear(&ch->certificate);
    UA_free(ch);
}

void
UA_GDSChannels_notify(UA_GDSChannels *channels, UA_Server *server,
                      UA_ApplicationNotificationType type,
                      const UA_KeyValueMap payload) {
    const UA_UInt32 *channelId = (const UA_UInt32 *)UA_KeyValueMap_getScalar(
        &payload, UA_QUALIFIEDNAME(0, "securechannel-id"),
        &UA_TYPES[UA_TYPES_UINT32]);
    if(!channelId)
        return;

    if(type == UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_CLOSED) {
        UA_GDSChannel *ch;
        LIST_FOREACH(ch, channels, pointers) {
            if(ch->channelId == *channelId) {
                removeChannel(ch);
                break;
            }
        }
        return;
    }

    if(type != UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_OPENED)
        return;
    const UA_NodeId *certificateTypeId =
        (const UA_NodeId *)UA_KeyValueMap_getScalar(
            &payload, UA_QUALIFIEDNAME(0, "certificate-type-id"),
            &UA_TYPES[UA_TYPES_NODEID]);
    const UA_ByteString *certificate =
        (const UA_ByteString *)UA_KeyValueMap_getScalar(
            &payload, UA_QUALIFIEDNAME(0, "remote-certificate"),
            &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Boolean hasType = certificateTypeId &&
        !UA_NodeId_isNull(certificateTypeId);
    UA_Boolean hasCertificate = certificate && certificate->length > 0;
    if(!hasType && !hasCertificate)
        return;

    UA_GDSChannel *ch = (UA_GDSChannel *)UA_calloc(1, sizeof(UA_GDSChannel));
    UA_StatusCode res = UA_STATUSCODE_BADOUTOFMEMORY;
    if(ch) {
        res = UA_STATUSCODE_GOOD;
        if(hasType)
            res |= UA_NodeId_copy(certificateTypeId, &ch->certificateTypeId);
        if(hasCertificate)
            res |= UA_ByteString_copy(certificate, &ch->certificate);
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Server_getConfig(server)->logging,
                       UA_LOGCATEGORY_SERVER,
                       "Could not register SecureChannel %u (oom)",
                       (unsigned)*channelId);
        if(ch) {
            UA_NodeId_clear(&ch->certificateTypeId);
            UA_ByteString_clear(&ch->certificate);
        }
        UA_free(ch);
        return;
    }
    ch->channelId = *channelId;
    LIST_INSERT_HEAD(channels, ch, pointers);
}

void
UA_GDSChannels_closeCertificateType(UA_GDSChannels *channels, UA_Server *server,
                                    const UA_NodeId *certificateTypeId) {
    UA_GDSChannel *ch, *chTmp;
    LIST_FOREACH_SAFE(ch, channels, pointers, chTmp) {
        if(!UA_NodeId_equal(&ch->certificateTypeId, certificateTypeId))
            continue;
        UA_LOG_INFO(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER,
                    "Closing SecureChannel %u. It was opened for the replaced "
                    "certificate.", (unsigned)ch->channelId);
        UA_Server_closeSecureChannel(server, ch->channelId,
                                     UA_SHUTDOWNREASON_CLOSE);
    }
}

void
UA_GDSChannels_closeUntrusted(UA_GDSChannels *channels, UA_Server *server,
                              UA_CertificateGroup *certGroup) {
    UA_GDSChannel *ch, *chTmp;
    LIST_FOREACH_SAFE(ch, channels, pointers, chTmp) {
        if(ch->certificate.length == 0 ||
           certGroup->verifyCertificate(certGroup, &ch->certificate) ==
           UA_STATUSCODE_GOOD)
            continue;
        UA_LOG_INFO(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER,
                    "Closing SecureChannel %u. The client certificate is not "
                    "trusted by the new TrustList.", (unsigned)ch->channelId);
        UA_Server_closeSecureChannel(server, ch->channelId,
                                     UA_SHUTDOWNREASON_CLOSE);
    }
}

void
UA_GDSChannels_clear(UA_GDSChannels *channels) {
    UA_GDSChannel *ch, *chTmp;
    LIST_FOREACH_SAFE(ch, channels, pointers, chTmp)
        removeChannel(ch);
}

#endif /* UA_ENABLE_DRIVER_GDS_RECEIVER || UA_ENABLE_DRIVER_GDS_PULL */
