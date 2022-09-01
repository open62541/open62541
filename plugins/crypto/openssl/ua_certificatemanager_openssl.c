#include <open62541/plugin/pki_default.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/evp.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)

#include "securitypolicy_openssl_common.h"

typedef struct {
    EVP_PKEY* privateKey;
    X509* certificate;
} CertificateManagerContext;

#endif
