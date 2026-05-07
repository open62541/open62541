/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/config.h>

#ifdef UA_ENABLE_ENCRYPTION

#include "custom_memory_manager.h"

#include <open62541/plugin/certificategroup.h>
#include <open62541/types.h>

/*
 * Fuzz X.509 certificate parsing utilities.
 * Tests parsing of DER-encoded certificates with various utility functions.
 * This targets the certificate validation code that processes untrusted
 * certificates from the network (OPN path).
 *
 * Byte 0: operation selector
 * Last 4 bytes: memory limit
 * Remaining: certificate data (DER format)
 */
extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(size <= 5)
        return 0;

    if(!UA_memoryManager_setLimitFromLast4Bytes(data, size))
        return 0;
    size -= 4;

    uint8_t op = data[0];
    data++;
    size--;

    UA_ByteString cert;
    cert.data = (UA_Byte*)(void*)data;
    cert.length = size;

    switch(op % 6) {
    case 0: {
        /* Parse subject name from certificate */
        UA_String subjectName = UA_STRING_NULL;
        UA_CertificateUtils_getSubjectName(&cert, &subjectName);
        UA_String_clear(&subjectName);
        break;
    }
    case 1: {
        /* Parse expiration date */
        UA_DateTime expiryDate = 0;
        UA_CertificateUtils_getExpirationDate(&cert, &expiryDate);
        break;
    }
    case 2: {
        /* Parse thumbprint */
        UA_String thumbprint = UA_STRING_NULL;
        UA_CertificateUtils_getThumbprint(&cert, &thumbprint);
        UA_String_clear(&thumbprint);
        break;
    }
    case 3: {
        /* Parse key size */
        size_t keySize = 0;
        UA_CertificateUtils_getKeySize(&cert, &keySize);
        break;
    }
    case 4: {
        /* Check if certificate is a CA certificate */
        UA_CertificateUtils_checkCA(&cert);
        break;
    }
    case 5: {
        /* Verify application URI */
        UA_String uri = UA_STRING((char*)"urn:test:app");
        UA_CertificateUtils_verifyApplicationUri(&cert, &uri);
        break;
    }
    }

    return 0;
}

#else /* UA_ENABLE_ENCRYPTION */

#include <stdint.h>
#include <stddef.h>

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    (void)data;
    (void)size;
    return 0;
}

#endif

