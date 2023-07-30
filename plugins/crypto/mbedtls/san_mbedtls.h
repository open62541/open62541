/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Copyright 2023 (c) Fraunhofer IOSB (Author: Kai Huebl)
 *
 */

#ifndef san_mbedtls_H_
#define san_mbedtls_H_

#include <open62541/config.h>
#include <open62541/types.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS

#include <mbedtls/version.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>

#if MBEDTLS_VERSION_NUMBER < 0x02130000

#define MBEDTLS_X509_SAN_DNS_NAME 2
#define MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER 6
#define MBEDTLS_X509_SAN_IP_ADDRESS 7

typedef struct mbedtls_x509_san_other_name
{
     mbedtls_x509_buf type_id;
     union
     {
         struct
         {
             mbedtls_x509_buf oid;
             mbedtls_x509_buf val;
         }
         hardware_module_name;
     }
     value;
} mbedtls_x509_san_other_name;

typedef struct mbedtls_x509_subject_alternative_name
{
     int type;
     union {
         mbedtls_x509_san_other_name other_name;
         mbedtls_x509_buf   unstructured_name;
     }
     san;
} mbedtls_x509_subject_alternative_name;
#endif

typedef struct san_mbedtls_san_list_entry_s
{
	mbedtls_x509_subject_alternative_name san;
	struct san_mbedtls_san_list_entry_s* next;
} san_mbedtls_san_list_entry_t;

void san_mbedtls_san_list_entry_free(
	san_mbedtls_san_list_entry_t* san_list_entry
);

san_mbedtls_san_list_entry_t* san_mbedtls_get_san_list_from_cert(
	const mbedtls_x509_crt* cert
);

int san_mbedtls_set_san_list_to_csr(
	mbedtls_x509write_csr* req,
	const san_mbedtls_san_list_entry_t* san_list
);

bool san_mbedtls_get_uniform_resource_identifier(
	san_mbedtls_san_list_entry_t* san_list,
	UA_String* uniform_resource_identifier
);

#endif

#endif /* UA_SECURITYPOLICY_MBEDTLS_COMMON_H_ */
