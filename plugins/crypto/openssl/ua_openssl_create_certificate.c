/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *
 */

#include <open62541/plugin/create_certificate.h>

#include "securitypolicy_openssl_common.h"

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)


#include <openssl/pem.h>
#include <openssl/x509v3.h>

/**
 * Join an array of UA_String to a single NULL-Terminated UA_String
 * separated by character sep
 */
static UA_StatusCode
join_string_with_sep(const UA_String *strings, size_t stringsSize,
                     char sep, UA_String *out) {
    if(!out)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_String_clear(out);
    size_t totalSize = stringsSize;
    for(size_t iStr = 0; iStr < stringsSize; ++iStr) {
        totalSize += strings[iStr].length;
    }

    UA_ByteString_allocBuffer(out, totalSize);
    if(!out->data) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    size_t pos = 0;
    for(size_t iStr = 0; iStr < stringsSize; ++iStr) {
        memcpy(&out->data[pos], strings[iStr].data, strings[iStr].length);
        pos += strings[iStr].length;
        out->data[pos] = (UA_Byte) sep;
        ++pos;
    }
    out->data[out->length-1] = 0;

    return UA_STATUSCODE_GOOD;
}

/**
 * Search for a character in a string (like strchr).
 * \todo Handle UTF-8
 *
 * \return index of the character or -1 on case of an error.
 */

static UA_Int32
UA_String_chr(const UA_String *pUaStr, char needl) {
    UA_Byte byteNeedl = (UA_Byte)needl;
    for(size_t i = 0; (size_t)i < pUaStr->length; ++i) {
        if(pUaStr->data[i] == byteNeedl) {
            return (UA_Int32) i;
        }
    }
    return -1;
}

/* char *value cannot be const due to openssl 1.0 compatibility */
static UA_StatusCode
add_x509V3ext(X509 *x509, int nid, char *value) {
    X509_EXTENSION *ex;
    X509V3_CTX ctx;
    X509V3_set_ctx_nodb(&ctx);
    X509V3_set_ctx(&ctx, x509, x509, NULL, NULL, 0);
    ex = X509V3_EXT_conf_nid(NULL, &ctx, nid, value);
    if(!ex)
        return UA_STATUSCODE_BADINTERNALERROR;
    X509_add_ext(x509, ex, -1);
    X509_EXTENSION_free(ex);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_CreateCertificate(const UA_Logger *logger,
                     const UA_String *subject, size_t subjectSize,
                     const UA_String *subjectAltName, size_t subjectAltNameSize,
                     size_t keySizeBits, UA_CertificateFormat certFormat,
                     UA_ByteString *outPrivateKey, UA_ByteString *outCertificate) {
    if(!outPrivateKey || !outCertificate || !logger || !subjectAltName ||
       !subject || subjectAltNameSize == 0 || subjectSize == 0 ||
       (certFormat != UA_CERTIFICATEFORMAT_DER && certFormat != UA_CERTIFICATEFORMAT_PEM ))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Use the maximum size */
    if(keySizeBits == 0)
        keySizeBits = 4096;

    UA_ByteString_init(outPrivateKey);
    UA_ByteString_init(outCertificate);

    UA_String fullAltSubj = UA_STRING_NULL;
    UA_Int32 serial = 1;

    /** \TODO: Seed Random generator
    * See: (https://www.openssl.org/docs/man1.1.0/man3/RAND_add.html) */
    BIO *memCert = NULL;
    BIO *memPKey = NULL;

    UA_StatusCode errRet = UA_STATUSCODE_GOOD;

    BIGNUM *exponent = BN_new();
    EVP_PKEY *pkey = EVP_PKEY_new();
    X509 *x509 = X509_new();
    RSA *rsa = RSA_new();

    if(!pkey || !x509 || !exponent || !rsa) {
        errRet = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    UA_LOG_INFO(logger, UA_LOGCATEGORY_SECURECHANNEL,
                "Create Certificate: Generating RSA key. This may take a while.");

    if(BN_set_word(exponent, RSA_F4) != 1) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Setting RSA exponent failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    if(RSA_generate_key_ex(rsa, (int) keySizeBits, exponent, NULL) != 1) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Generating RSA key failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    if(EVP_PKEY_assign_RSA(pkey, rsa) != 1) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Assign RSA key failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }
    /* rsa will be freed by pkey */
    rsa = NULL;

    /* x509v3 has version 2
     * (https://www.openssl.org/docs/man1.1.0/man3/X509_set_version.html) */
    if(X509_set_version(x509, 2) != 1) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Setting version failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    if(ASN1_INTEGER_set(X509_get_serialNumber(x509), serial) != 1) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Setting serial number failed.");
        /* Only memory errors are possible */
        errRet = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    if(X509_gmtime_adj(X509_get_notBefore(x509), 0) == NULL) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Setting 'not before' failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    if(X509_gmtime_adj(X509_get_notAfter(x509), (UA_Int64) 60 * 60 * 24 * 365) == NULL) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Setting 'not before' failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    if(X509_set_pubkey(x509, pkey) != 1) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Setting publik key failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    X509_NAME *name = X509_get_subject_name(x509);
    if(name == NULL) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Getting name failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    for(UA_UInt32 iSubject = 0; iSubject < subjectSize; ++iSubject) {
        UA_Int32 sep = UA_String_chr(&subject[iSubject], '=');
        char field[16];
        if(sep == -1 || sep == 0 ||
            ((size_t) sep == (subject[iSubject].length - 1)) || sep >= 15) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                         "Create Certificate: Subject must contain one '=' with "
                         "content before and after.");
            errRet = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
        memcpy(field, subject[iSubject].data, (size_t) sep);
        field[sep] = 0;
        UA_Byte* pData = &subject[iSubject].data[sep + 1];
        if(X509_NAME_add_entry_by_txt(
               name, field, MBSTRING_ASC,
               (const unsigned char *)pData,
               (int) subject[iSubject].length - (int) sep - 1, -1, 0) != 1) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                         "Create Certificate: Setting subject failed.");
            errRet = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
    }
    /* Self signed, so issuer == subject */
    if(X509_set_issuer_name(x509, name) != 1) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Setting name failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    errRet = add_x509V3ext(x509, NID_basic_constraints, "CA:FALSE");
    if(errRet != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Setting 'Basic Constraints' failed.");
        goto cleanup;
    }

    /* See https://datatracker.ietf.org/doc/html/rfc5280#section-4.2.1.3 for
     * possible values */
    errRet = add_x509V3ext(x509, NID_key_usage,
                           "digitalSignature,nonRepudiation,keyEncipherment,dataEncipherment,keyCertSign");
    if(errRet != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Setting 'Key Usage' failed.");
        goto cleanup;
    }

    errRet = add_x509V3ext(x509, NID_ext_key_usage, "serverAuth,clientAuth");
    if(errRet != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Setting 'Extended Key Usage' failed.");
        goto cleanup;
    }

    errRet = add_x509V3ext(x509, NID_subject_key_identifier, "hash");
    if(errRet != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Setting 'Subject Key Identifier' failed.");
        goto cleanup;
    }

    errRet = join_string_with_sep(subjectAltName, subjectAltNameSize, ',', &fullAltSubj);
    if(errRet != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Joining altSubject failed.");
        goto cleanup;
    }

    errRet = add_x509V3ext(x509, NID_subject_alt_name, (char*) fullAltSubj.data);
    if(errRet != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Setting 'Subject Alternative Name:' failed.");
        goto cleanup;
    }

    if(X509_sign(x509, pkey, EVP_sha256()) == 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Signing failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    switch(certFormat) {
        case UA_CERTIFICATEFORMAT_DER: {
            int tmpLen = i2d_PrivateKey(pkey, &outPrivateKey->data);
            if(tmpLen <= 0) {
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                            "Create Certificate: Create private DER key failed.");
                errRet = UA_STATUSCODE_BADINTERNALERROR;
                goto cleanup;
            }
            outPrivateKey->length = (size_t) tmpLen;

            tmpLen = i2d_X509(x509, &outCertificate->data);
            if(tmpLen <= 0) {
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                            "Create Certificate: Create DER-certificate failed.");
                errRet = UA_STATUSCODE_BADINTERNALERROR;
                goto cleanup;
            }
            outCertificate->length = (size_t) tmpLen;
            break;
        }
        case UA_CERTIFICATEFORMAT_PEM: {
            /* Private Key */
            memPKey = BIO_new(BIO_s_mem());
            if(!memPKey) {
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                            "Create Certificate: Allocate Membuffer for PKey failed.");
                errRet = UA_STATUSCODE_BADOUTOFMEMORY;
                goto cleanup;
            }

            if(PEM_write_bio_PrivateKey(memPKey, pkey, NULL, NULL, 0, 0, NULL) != 1) {
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                            "Create Certificate: Generate PEM-PrivateKey failed.");
                errRet = UA_STATUSCODE_BADINTERNALERROR;
                goto cleanup;
            }

            UA_ByteString tmpPem = UA_BYTESTRING_NULL;
            tmpPem.length = (size_t) BIO_get_mem_data(memPKey, &tmpPem.data);
            errRet = UA_ByteString_copy(&tmpPem, outPrivateKey);
            if(errRet != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                            "Create Certificate: Copy PEM PKey failed.");
                goto cleanup;
            }

            /* Certificate */
            memCert = BIO_new(BIO_s_mem());
            if(!memCert) {
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                            "Create Certificate: Allocate Membuffer for Cert failed.");
                errRet = UA_STATUSCODE_BADOUTOFMEMORY;
                goto cleanup;
            }

            if(PEM_write_bio_X509(memCert, x509) != 1) {
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                            "Create Certificate: Generate PEM-Certifcate failed.");
                errRet = UA_STATUSCODE_BADINTERNALERROR;
                goto cleanup;
            }

            tmpPem.length = (size_t) BIO_get_mem_data(memCert, &tmpPem.data);
            errRet = UA_ByteString_copy(&tmpPem, outCertificate);
            if(errRet != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                            "Create Certificate: Copy PEM Certificate failed.");
                goto cleanup;
            }
            break;
        }
    }

cleanup:
    UA_String_clear(&fullAltSubj);
    RSA_free(rsa);
    X509_free(x509);
    EVP_PKEY_free(pkey);
    BIO_free(memCert);
    BIO_free(memPKey);
    BN_free(exponent);
    return errRet;
}

#endif
