/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Mark Giraud, Fraunhofer IOSB
 */


#ifndef OPEN62541_CERTSTORE_H
#define OPEN62541_CERTSTORE_H

#include <open62541/types.h>
#include <open62541/types_generated.h>

typedef struct UA_PKIStore UA_PKIStore;

struct UA_PKIStore {
    /* The nodeId of the certificate group this pki store is associated with */
    UA_NodeId certificateGroupId;

    UA_StatusCode (*loadTrustList)(UA_PKIStore *pkiStore, UA_TrustListDataType *trustList);

    UA_StatusCode (*storeTrustList)(UA_PKIStore *pkiStore, const UA_TrustListDataType *trustList);

    UA_StatusCode (*loadRejectedList)(UA_PKIStore *pkiStore, UA_ByteString **rejectedList, size_t *rejectedListSize);

    /* Overwrites the rejected list in the PKIStore with the supplied one */
    UA_StatusCode (*storeRejectedList)(UA_PKIStore *pkiStore,
                                       const UA_ByteString *rejectedList,
                                       size_t rejectedListSize);

    /* Appends the supplied certificate to the rejected list */
    UA_StatusCode (*appendRejectedList)(UA_PKIStore *pkiStore, const UA_ByteString *certificate);

    UA_StatusCode (*loadCertificate)(UA_PKIStore *pkiStore, const UA_NodeId certType, UA_ByteString *cert);

    UA_StatusCode (*storeCertificate)(UA_PKIStore *pkiStore, const UA_NodeId certType, const UA_ByteString *cert);

    UA_StatusCode (*loadPrivateKey)(UA_PKIStore *pkiStore, const UA_NodeId certType, UA_ByteString *privateKey);

    UA_StatusCode (*storePrivateKey)(UA_PKIStore *pkiStore, const UA_NodeId certType, const UA_ByteString *privateKey);

    UA_StatusCode (*clear)(UA_PKIStore *pkiStore);

    void *context;
};

#endif //OPEN62541_CERTSTORE_H
