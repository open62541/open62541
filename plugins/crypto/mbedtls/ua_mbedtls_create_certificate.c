/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *
 */

#include "securitypolicy_mbedtls_common.h"

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS

UA_StatusCode
UA_CreateCertificate(const UA_Logger *logger,
                    UA_String subject[], UA_UInt32 lenSubject,
                    UA_String subjectAltName[], UA_UInt32 lenSubjectAltName,
                    UA_ByteString *outPKey, UA_ByteString *outCert,
                    enum UA_CertificateFormat certFormat) {
    if(!outPKey || !outCert || !logger || !subjectAltName
        || !subject || lenSubjectAltName == 0 || lenSubject == 0) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    // Missing SubjectAltName support in mbedTLS.
    // See https://github.com/ARMmbed/mbedtls/pull/731#issuecomment-403283720
    UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                       "Create Certificate: Not implemented for mbedTLS.");
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

#endif
