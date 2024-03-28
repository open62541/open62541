/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright (c) 2023 Fraunhofer IOSB (Author: Noel Graf)
 *
 */

#include <open62541/plugin/create_certificate.h>
#include <time.h>

#include "securitypolicy_mbedtls_common.h"
#include "../../arch/eventloop_posix/eventloop_posix.h"

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)

#include <mbedtls/x509_crt.h>
#include <mbedtls/oid.h>
#include <mbedtls/asn1write.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/platform.h>
#include <mbedtls/version.h>

#define SET_OID(x, oid) \
    do { x.len = MBEDTLS_OID_SIZE(oid); x.p = (unsigned char *) oid; } while (0)

#define MBEDTLS_ASN1_CHK_CLEANUP_ADD(g, f) \
    do                                     \
    {                                      \
        if ((ret = (f)) < 0)               \
        goto cleanup;                      \
        else                               \
        (g) += ret;                        \
    } while (0)

#if MBEDTLS_VERSION_NUMBER < 0x02170000
#define MBEDTLS_X509_SAN_OTHER_NAME                      0
#define MBEDTLS_X509_SAN_RFC822_NAME                     1
#define MBEDTLS_X509_SAN_DNS_NAME                        2
#define MBEDTLS_X509_SAN_X400_ADDRESS_NAME               3
#define MBEDTLS_X509_SAN_DIRECTORY_NAME                  4
#define MBEDTLS_X509_SAN_EDI_PARTY_NAME                  5
#define MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER     6
#define MBEDTLS_X509_SAN_IP_ADDRESS                      7
#define MBEDTLS_X509_SAN_REGISTERED_ID                   8
#endif

#define MBEDTLS_SAN_MAX_LEN    64
typedef struct mbedtls_write_san_node{
    int type;
    char* host;
    size_t hostlen;
} mbedtls_write_san_node;

typedef struct mbedtls_write_san_list{
    mbedtls_write_san_node node;
    struct mbedtls_write_san_list* next;
} mbedtls_write_san_list;

static size_t mbedtls_get_san_list_deep(const mbedtls_write_san_list* sanlist);

int mbedtls_x509write_crt_set_subject_alt_name(mbedtls_x509write_cert *ctx, const mbedtls_write_san_list* sanlist);

#if MBEDTLS_VERSION_NUMBER < 0x03030000
int mbedtls_x509write_crt_set_ext_key_usage(mbedtls_x509write_cert *ctx,
                                            const mbedtls_asn1_sequence *exts);
#endif

static int write_certificate(mbedtls_x509write_cert *crt, UA_CertificateFormat certFormat,
                             UA_ByteString *outCertificate, int (*f_rng)(void *, unsigned char *, size_t),
                             void *p_rng);

static int write_private_key(mbedtls_pk_context *key, UA_CertificateFormat keyFormat, UA_ByteString *outPrivateKey);

UA_StatusCode
UA_CreateCertificate(const UA_Logger *logger, const UA_String *subject,
                     size_t subjectSize, const UA_String *subjectAltName,
                     size_t subjectAltNameSize, UA_CertificateFormat certFormat,
                     UA_KeyValueMap *params, UA_ByteString *outPrivateKey,
                     UA_ByteString *outCertificate) {
    if(!outPrivateKey || !outCertificate || !logger || !subjectAltName || !subject ||
       subjectAltNameSize == 0 || subjectSize == 0 ||
       (certFormat != UA_CERTIFICATEFORMAT_DER && certFormat != UA_CERTIFICATEFORMAT_PEM))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Use the maximum size */
    UA_UInt16 keySizeBits = 4096;
    /* Default to 1 year */
    UA_UInt16 expiresInDays = 365;

    if(params) {
        const UA_UInt16 *keySizeBitsValue = (const UA_UInt16 *)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "key-size-bits"), &UA_TYPES[UA_TYPES_UINT16]);
        if(keySizeBitsValue)
            keySizeBits = *keySizeBitsValue;

        const UA_UInt16 *expiresInDaysValue = (const UA_UInt16 *)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "expires-in-days"), &UA_TYPES[UA_TYPES_UINT16]);
        if(expiresInDaysValue)
            expiresInDays = *expiresInDaysValue;
    }

    UA_ByteString_init(outPrivateKey);
    UA_ByteString_init(outCertificate);

    mbedtls_pk_context key;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    const char *pers = "gen_key";
    mbedtls_x509write_cert crt;

    UA_StatusCode errRet = UA_STATUSCODE_GOOD;

    /* Set to sane values */
    mbedtls_pk_init(&key);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    mbedtls_x509write_crt_init(&crt);

    /* Seed the random number generator */
    if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers)) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Failed to initialize the random number generator.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* Generate an RSA key pair */
    if (mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)) != 0 ||
        mbedtls_rsa_gen_key(mbedtls_pk_rsa(key), mbedtls_ctr_drbg_random, &ctr_drbg, keySizeBits, 65537) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Failed to generate RSA key pair.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* Setting certificate values */
    mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);

    size_t subject_char_len = 0;
    for(size_t i = 0; i < subjectSize; i++) {
        subject_char_len += subject[i].length;
    }
    char *subject_char = (char*)UA_malloc(subject_char_len + subjectSize);
    if(!subject_char) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Cannot allocate memory for subject. Out of memory.");
        errRet = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    size_t pos = 0;
    for(size_t i = 0; i < subjectSize; i++) {
        subject_char_len += subject[i].length;
        memcpy(subject_char + pos, subject[i].data, subject[i].length);
        pos += subject[i].length;
        if(i < subjectSize - 1)
            subject_char[pos++] = ',';
        else
            subject_char[pos++] = '\0';
    }

    if((mbedtls_x509write_crt_set_subject_name(&crt, subject_char)) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Setting subject failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        UA_free(subject_char);
        goto cleanup;
    }

    if((mbedtls_x509write_crt_set_issuer_name(&crt, subject_char)) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Setting issuer failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        UA_free(subject_char);
        goto cleanup;
    }

    UA_free(subject_char);

    mbedtls_write_san_list *cur = NULL;
    mbedtls_write_san_list *cur_tmp = NULL;
    mbedtls_write_san_list *head = NULL;
    for(size_t i = 0; i < subjectAltNameSize; i++) {
        char *sanType;
        char *sanValue;
        size_t sanValueLength;
        char *subAlt = (char *)UA_malloc(subjectAltName[i].length + 1);
        memcpy(subAlt, subjectAltName[i].data, subjectAltName[i].length);

        /* null-terminate the copied string */
        subAlt[subjectAltName[i].length] = 0;
        /* split into SAN type and value */
        sanType = strtok(subAlt, ":");
        sanValue = (char *)subjectAltName[i].data + strlen(sanType) + 1;
        sanValueLength = strlen(sanValue);

        if(sanType) {
            cur_tmp = (mbedtls_write_san_list*)mbedtls_calloc(1, sizeof(mbedtls_write_san_list));
            cur_tmp->next = NULL;
            cur_tmp->node.host = sanValue;
            cur_tmp->node.hostlen = sanValueLength;

            if(strcmp(sanType, "DNS") == 0) {
                cur_tmp->node.type = MBEDTLS_X509_SAN_DNS_NAME;
            } else if(strcmp(sanType, "URI") == 0) {
                cur_tmp->node.type = MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER;
            } else if(strcmp(sanType, "IP") == 0) {
                uint8_t ip[4] = {0};
                if(UA_inet_pton(AF_INET, sanValue, ip) <= 0) {
                    UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURECHANNEL, "IP SAN preparation failed");
                    mbedtls_free(cur_tmp);
                    UA_free(subAlt);
                    continue;
                }
                cur_tmp->node.type = MBEDTLS_X509_SAN_IP_ADDRESS;
                cur_tmp->node.host = (char *)ip;
                cur_tmp->node.hostlen = sizeof(ip);
            } else if(strcmp(sanType, "RFC822") == 0) {
                cur_tmp->node.type = MBEDTLS_X509_SAN_RFC822_NAME;
            } else {
                UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURECHANNEL, "Given an unsupported SAN");
                mbedtls_free(cur_tmp);
                UA_free(subAlt);
                continue;
            }
        } else {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURECHANNEL, "Invalid Input format");
            UA_free(subAlt);
            continue;
        }

        if(!cur) {
            cur = cur_tmp;
            head = cur_tmp;
        } else {
            cur->next = cur_tmp;
            cur = cur->next;
        }

        UA_free(subAlt);
    }

    if((mbedtls_x509write_crt_set_subject_alt_name(&crt, head)) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Setting subject alternative name failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        while(head != NULL) {
            cur_tmp = head->next;
            mbedtls_free(head);
            head = cur_tmp;
        }
        goto cleanup;
    }

    while(head != NULL) {
        cur_tmp = head->next;
        mbedtls_free(head);
        head = cur_tmp;
    }

#if MBEDTLS_VERSION_NUMBER >= 0x03040000
    unsigned char *serial = (unsigned char *)"1";
    size_t serial_len = 1;
    mbedtls_x509write_crt_set_serial_raw(&crt, serial, serial_len);
#else
    mbedtls_mpi serial_mpi;
    mbedtls_mpi_init(&serial_mpi);
    mbedtls_mpi_lset(&serial_mpi, 1);
    mbedtls_x509write_crt_set_serial(&crt, &serial_mpi);
    mbedtls_mpi_free(&serial_mpi);
#endif

    /* Get the current time */
    time_t rawTime;
    struct tm *timeInfo;
    time(&rawTime);
    timeInfo = gmtime(&rawTime);

    /* Format the current timestamp */
    char current_timestamp[15];  // YYYYMMDDhhmmss + '\0'
    strftime(current_timestamp, sizeof(current_timestamp), "%Y%m%d%H%M%S", timeInfo);

    /* Calculate the future timestamp */
    timeInfo->tm_mday += expiresInDays;
    time_t future_time = mktime(timeInfo);

    /* Format the future timestamp */
    char future_timestamp[15];  // YYYYMMDDhhmmss + '\0'
    strftime(future_timestamp, sizeof(future_timestamp), "%Y%m%d%H%M%S", gmtime(&future_time));

    if(mbedtls_x509write_crt_set_validity(&crt, current_timestamp, future_timestamp) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Setting 'not before' and 'not after' failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    if(mbedtls_x509write_crt_set_basic_constraints(&crt, 0, -1) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Setting basic constraints failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    if(mbedtls_x509write_crt_set_key_usage(&crt, MBEDTLS_X509_KU_DIGITAL_SIGNATURE | MBEDTLS_X509_KU_NON_REPUDIATION
                                            | MBEDTLS_X509_KU_KEY_ENCIPHERMENT | MBEDTLS_X509_KU_DATA_ENCIPHERMENT
                                            | MBEDTLS_X509_KU_KEY_CERT_SIGN | MBEDTLS_X509_KU_CRL_SIGN) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Setting key usage failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    mbedtls_asn1_sequence *ext_key_usage;
    ext_key_usage = (mbedtls_asn1_sequence *)mbedtls_calloc(1, sizeof(mbedtls_asn1_sequence));
    ext_key_usage->buf.tag = MBEDTLS_ASN1_OID;
    SET_OID(ext_key_usage->buf, MBEDTLS_OID_SERVER_AUTH);
    ext_key_usage->next = (mbedtls_asn1_sequence *)mbedtls_calloc(1, sizeof(mbedtls_asn1_sequence));
    ext_key_usage->next->buf.tag = MBEDTLS_ASN1_OID;
    SET_OID(ext_key_usage->next->buf, MBEDTLS_OID_CLIENT_AUTH);

    if(mbedtls_x509write_crt_set_ext_key_usage(&crt, ext_key_usage) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Setting extended key usage failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        mbedtls_free(ext_key_usage->next);
        mbedtls_free(ext_key_usage);
        goto cleanup;
    }

    mbedtls_free(ext_key_usage->next);
    mbedtls_free(ext_key_usage);

    mbedtls_x509write_crt_set_subject_key(&crt, &key);
    mbedtls_x509write_crt_set_issuer_key(&crt, &key);


    /* Write private key */
    if ((write_private_key(&key, certFormat, outPrivateKey)) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Writing private key failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* Write Certificate */
    if ((write_certificate(&crt, certFormat, outCertificate,
                                 mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Writing certificate failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    mbedtls_x509write_crt_free(&crt);
    mbedtls_pk_free(&key);

cleanup:
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    mbedtls_x509write_crt_free(&crt);
    mbedtls_pk_free(&key);
    return errRet;
}

static int write_private_key(mbedtls_pk_context *key, UA_CertificateFormat keyFormat, UA_ByteString *outPrivateKey) {
    int ret;
    unsigned char output_buf[16000];
    unsigned char *c = output_buf;
    size_t len = 0;

    memset(output_buf, 0, 16000);
    switch(keyFormat) {
    case UA_CERTIFICATEFORMAT_DER: {
        if((ret = mbedtls_pk_write_key_pem(key, output_buf, 16000)) != 0) {
            return ret;
        }

        len = strlen((char *) output_buf);
        break;
    }
    case UA_CERTIFICATEFORMAT_PEM: {
        if((ret = mbedtls_pk_write_key_der(key, output_buf, 16000)) < 0) {
            return ret;
        }

        len = ret;
        c = output_buf + sizeof(output_buf) - len;
        break;
    }
    }

    outPrivateKey->length = len;
    UA_ByteString_allocBuffer(outPrivateKey, outPrivateKey->length);
    memcpy(outPrivateKey->data, c, outPrivateKey->length);

    return 0;
}

static int write_certificate(mbedtls_x509write_cert *crt, UA_CertificateFormat certFormat,
                      UA_ByteString *outCertificate, int (*f_rng)(void *, unsigned char *, size_t),
                      void *p_rng) {
    int ret;
    unsigned char output_buf[4096];
    unsigned char *c = output_buf;
    size_t len = 0;

    memset(output_buf, 0, 4096);
    switch(certFormat) {
    case UA_CERTIFICATEFORMAT_DER: {
        if((ret = mbedtls_x509write_crt_der(crt, output_buf, 4096, f_rng, p_rng)) < 0) {
            return ret;
        }

        len = ret;
        c = output_buf + 4096 - len;
        break;
    }
    case UA_CERTIFICATEFORMAT_PEM: {
        if((ret = mbedtls_x509write_crt_pem(crt, output_buf, 4096, f_rng, p_rng)) < 0) {
            return ret;
        }

        len = strlen((char *)output_buf);
        break;
    }
    }

    outCertificate->length = len;
    UA_ByteString_allocBuffer(outCertificate, outCertificate->length);
    memcpy(outCertificate->data, c, outCertificate->length);

    return 0;
}

#if MBEDTLS_VERSION_NUMBER < 0x03030000
int mbedtls_x509write_crt_set_ext_key_usage(mbedtls_x509write_cert *ctx,
                                            const mbedtls_asn1_sequence *exts) {
    unsigned char buf[256];
    unsigned char *c = buf + sizeof(buf);
    int ret;
    size_t len = 0;
    const mbedtls_asn1_sequence *last_ext = NULL;
    const mbedtls_asn1_sequence *ext;

    memset(buf, 0, sizeof(buf));

    /* We need at least one extension: SEQUENCE SIZE (1..MAX) OF KeyPurposeId */
    if(!exts) {
        return MBEDTLS_ERR_X509_BAD_INPUT_DATA;
    }

    /* Iterate over exts backwards, so we write them out in the requested order */
    while(last_ext != exts) {
        for(ext = exts; ext->next != last_ext; ext = ext->next) {
        }
        if(ext->buf.tag != MBEDTLS_ASN1_OID) {
            return MBEDTLS_ERR_X509_BAD_INPUT_DATA;
        }
        MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_raw_buffer(&c, buf, ext->buf.p, ext->buf.len));
        MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf, ext->buf.len));
        MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(&c, buf, MBEDTLS_ASN1_OID));
        last_ext = ext;
    }

    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf, len));
    MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(&c, buf, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

    return mbedtls_x509write_crt_set_extension(ctx, MBEDTLS_OID_EXTENDED_KEY_USAGE,
                                               MBEDTLS_OID_SIZE(MBEDTLS_OID_EXTENDED_KEY_USAGE), 1, c, len);
}

#endif

static size_t mbedtls_get_san_list_deep(const mbedtls_write_san_list* sanlist) {
    size_t ret = 0;
    const mbedtls_write_san_list* cur = sanlist;
    while (cur) {
        ++ret;
        cur = cur->next;
    }

    return ret;
}

int mbedtls_x509write_crt_set_subject_alt_name(mbedtls_x509write_cert *ctx, const mbedtls_write_san_list* sanlist) {
    int	ret = 0;
    size_t sandeep = 0;
    const mbedtls_write_san_list* cur = sanlist;
    unsigned char* buf;
    unsigned char* pc;
    size_t len;
    size_t buflen = 0;

    /* How many alt names to be written */
    sandeep = mbedtls_get_san_list_deep(sanlist);
    if (sandeep == 0)
        return ret;

    buflen = MBEDTLS_SAN_MAX_LEN * sandeep + sandeep;
    buf = (unsigned char *)mbedtls_calloc(1, buflen);
    if(!buf)
        return MBEDTLS_ERR_ASN1_ALLOC_FAILED;

    memset(buf, 0, buflen);
    pc = buf + buflen;

    len = 0;
    while(cur) {
        switch (cur->node.type) {
        case MBEDTLS_X509_SAN_DNS_NAME:
        case MBEDTLS_X509_SAN_RFC822_NAME:
        case MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER:
        case MBEDTLS_X509_SAN_IP_ADDRESS:
            MBEDTLS_ASN1_CHK_CLEANUP_ADD(len,
                                         mbedtls_asn1_write_raw_buffer(&pc, buf, (const unsigned char *)cur->node.host,
                                                                       cur->node.hostlen));
            MBEDTLS_ASN1_CHK_CLEANUP_ADD(len, mbedtls_asn1_write_len(&pc, buf, cur->node.hostlen));
            MBEDTLS_ASN1_CHK_CLEANUP_ADD(len, mbedtls_asn1_write_tag(&pc, buf,
                                                                     MBEDTLS_ASN1_CONTEXT_SPECIFIC | cur->node.type));
            break;
        default:
            /* Error out on an unsupported SAN */
            ret = MBEDTLS_ERR_X509_FEATURE_UNAVAILABLE;
            goto cleanup;
        }

        cur = cur->next;
    }

    MBEDTLS_ASN1_CHK_CLEANUP_ADD(len, mbedtls_asn1_write_len(&pc, buf, len));
    MBEDTLS_ASN1_CHK_CLEANUP_ADD(len, mbedtls_asn1_write_tag(&pc, buf, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

    ret = mbedtls_x509write_crt_set_extension(ctx, MBEDTLS_OID_SUBJECT_ALT_NAME,
                                              MBEDTLS_OID_SIZE(MBEDTLS_OID_SUBJECT_ALT_NAME), 0, buf + buflen - len, len);

    mbedtls_free(buf);
    return ret;

cleanup:
    mbedtls_free(buf);
    return ret;
}

#endif
