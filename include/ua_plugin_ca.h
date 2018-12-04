/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2018 (c) Markus Karch, Fraunhofer IOSB
 */

#ifndef OPEN62541_UA_PLUGIN_CA_H
#define OPEN62541_UA_PLUGIN_CA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"
#include "ua_plugin_log.h"
#include "ua_types_generated.h"

#ifdef UA_ENABLE_GDS_CM

/*
 * A GDSCertificateGroup represents an interface to one CA.
 * Currently only one plugin interface is implemented using GnuTLS.
 * */
struct UA_GDS_CA;
typedef struct UA_GDS_CA UA_GDS_CA;

struct UA_GDS_CA {
    void *context;
    UA_Logger *logger;

    UA_StatusCode (*createNewKeyPair) (UA_GDS_CA *scg,
                                       UA_String *subjectName,
                                       UA_String *privateKeyFormat,
                                       UA_String *privateKeyPassword,
                                       unsigned  int keySize,
                                       size_t domainNamesSize,
                                       UA_String *domainNamesArray,
                                       UA_ByteString *certificate,
                                       UA_ByteString *privateKey,
                                       size_t *issuerCertificateSize,
                                       UA_ByteString **issuerCertificates);

    UA_StatusCode (*certificateSigningRequest) (UA_GDS_CA *scg,
                                                unsigned int supposedKeySize,
                                                UA_ByteString *certificateSigningRequest,
                                                UA_ByteString *certificate,
                                                size_t *issuerCertificateSize,
                                                UA_ByteString **issuerCertificates);

    UA_StatusCode (*addCertificateToTrustList)(UA_GDS_CA *scg,
                                               UA_ByteString *certificate,
                                               UA_Boolean isTrustedCertificate);

    UA_StatusCode (*removeCertificateFromTrustlist)(UA_GDS_CA *scg,
                                                    UA_String *thumbprint,
                                                    UA_Boolean isTrustedCertificate);

    UA_StatusCode (*getTrustList)(UA_GDS_CA *scg,
                                  UA_TrustListDataType *list);

    UA_StatusCode (*addCertificatetoCRL)(UA_GDS_CA *scg,
                                        size_t serialNumberSize,
                                        char *serialNumber,
                                        UA_ByteString certificate);

    UA_StatusCode (*getCertificateStatus)(UA_GDS_CA *scg,
                                         UA_ByteString *certificate,
                                         UA_Boolean *updateRequired);

    UA_StatusCode (*isCertificatefromCA)(UA_GDS_CA *scg,
                                         UA_ByteString certificate,
                                         UA_Boolean *result);

    void (*deleteMembers)(UA_GDS_CA *cv);
};

typedef struct {
    UA_NodeId certificateGroupId;
    UA_NodeId trustListId;
    size_t certificateTypesSize;
    UA_NodeId *certificateTypes;
    UA_GDS_CA *ca;
} UA_GDS_CertificateGroup;

#endif

#ifdef __cplusplus
}
#endif

#endif //OPEN62541_UA_PLUGIN_CA_H
