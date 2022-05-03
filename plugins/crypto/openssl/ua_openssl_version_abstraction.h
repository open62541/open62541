/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *
 */

#ifndef UA_OPENSSL_VERSION_ABSTRACTION_H_
#define UA_OPENSSL_VERSION_ABSTRACTION_H_

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)

#include <openssl/x509.h>

#if !defined(OPENSSL_VERSION_NUMBER)
#error "OPENSSL_VERSION_NUMBER is not defined."
#endif

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
#define X509_STORE_CTX_set0_trusted_stack(STORE_CTX, CTX_SKTRUSTED) X509_STORE_CTX_trusted_stack(STORE_CTX, CTX_SKTRUSTED)
#endif

#if OPENSSL_VERSION_NUMBER < 0x1010000fL || defined(LIBRESSL_VERSION_NUMBER)
#define X509_STORE_CTX_get_check_issued(STORE_CTX) STORE_CTX->check_issued
#endif

#if OPENSSL_VERSION_NUMBER < 0x1010000fL || defined(LIBRESSL_VERSION_NUMBER)
#define get_pkey_rsa(evp) ((evp)->pkey.rsa)
#else
#define get_pkey_rsa(evp) EVP_PKEY_get0_RSA(evp)
#endif

#if OPENSSL_VERSION_NUMBER < 0x1010000fL || defined(LIBRESSL_VERSION_NUMBER)
#define X509_get0_subject_key_id(PX509_CERT) (const ASN1_OCTET_STRING *)X509_get_ext_d2i(PX509_CERT, NID_subject_key_identifier, NULL, NULL);
#endif

#endif /* defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL) */
#endif /* UA_OPENSSL_VERSION_ABSTRACTION_H_ */
