/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Copyright 2023 (c) Fraunhofer IOSB (Author: Kai Huebl)
 *
 */

#include <open62541/plugin/pki_default.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS

#include "securitypolicy_mbedtls_common.h"
#include "san_mbedtls.h"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/oid.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>

typedef struct {
    mbedtls_pk_context privateKey;
    mbedtls_x509_crt certificate;
    mbedtls_ctr_drbg_context ctrDrbg;
    mbedtls_entropy_context entropy;
} CertificateManagerContext;



#endif
