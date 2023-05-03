/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 */

#ifndef CREATE_CERTIFICATE_H_
#define CREATE_CERTIFICATE_H_

#include <open62541/plugin/log.h>
#include <open62541/types.h>
#include <open62541/util.h>

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_ENCRYPTION
typedef enum {
    UA_CERTIFICATEFORMAT_DER,
    UA_CERTIFICATEFORMAT_PEM
} UA_CertificateFormat;

/**
 * Create a self-signed certificate
 *
 * It is recommended to store the generated certificate on disk for reuse, so the
 * application can be recognized across several executions.
 *
 * \param subject Elements for the subject,
 *                  e.g. ["C=DE", "O=SampleOrganization", "CN=Open62541Server@localhost"]
 * \param subjectAltName Elements for SubjectAltName,
 *                  e.g. ["DNS:localhost", "URI:urn:open62541.server.application"]
 * \param params key value map with optional parameters:
 *                  - expires-in-days after these the cert expires default: 365
 *                  - key-size-bits Size of the generated key in bits. Possible values are:
 *                    [0, 1024 (deprecated), 2048, 4096] default: 4096
 */
UA_StatusCode UA_EXPORT
UA_CreateCertificate(const UA_Logger *logger, const UA_String *subject,
                     size_t subjectSize, const UA_String *subjectAltName,
                     size_t subjectAltNameSize, UA_CertificateFormat certFormat,
                     UA_KeyValueMap *params, UA_ByteString *outPrivateKey,
                     UA_ByteString *outCertificate);
#endif

_UA_END_DECLS

#endif /* CREATE_CERTIFICATE_H_ */
