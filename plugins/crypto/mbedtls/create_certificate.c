/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright (c) 2023 Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 *
 */

#include <open62541/plugin/create_certificate.h>

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)

#include "securitypolicy_common.h"
#include "securitypolicy_mbedtls_compat.h"
#include "../deps/musl_inet_pton.h"

#include <stdio.h>

#include <mbedtls/x509_crt.h>
#include <mbedtls/oid.h>
#include <mbedtls/asn1write.h>
#include <mbedtls/platform.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/psa_util.h>

#define SET_OID(x, oid) \
    do { x.len = MBEDTLS_OID_SIZE(oid); x.p = (unsigned char *) oid; } while (0)

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

static int write_certificate(mbedtls_x509write_cert *crt, UA_CertificateFormat certFormat,
                             UA_ByteString *outCertificate);

static int write_private_key(mbedtls_pk_context *key, UA_CertificateFormat keyFormat, UA_ByteString *outPrivateKey);

static void
clearSanList(mbedtls_write_san_list *head) {
    while(head) {
        mbedtls_write_san_list *next = head->next;
        if(head->node.type == MBEDTLS_X509_SAN_IP_ADDRESS)
            mbedtls_free(head->node.host);
        mbedtls_free(head);
        head = next;
    }
}

static UA_Boolean
formatCertificateTime(UA_DateTime time, char output[15]) {
    UA_DateTimeStruct value = UA_DateTime_toStruct(time);
    if(value.year < 0 || value.year > 9999)
        return false;
    return snprintf(output, 15, "%04d%02u%02u%02u%02u%02u",
                    (int)value.year, (unsigned)value.month,
                    (unsigned)value.day, (unsigned)value.hour,
                    (unsigned)value.min, (unsigned)value.sec) == 14;
}

/* Case-insensitive comparison of a UA_String with a C string literal */
static UA_Boolean
uaStringEqualsCI_mbedtls(const UA_String *uaStr, const char *cStr) {
    size_t cLen = strlen(cStr);
    if(uaStr->length != cLen)
        return false;
    for(size_t i = 0; i < cLen; i++) {
        char a = (char)uaStr->data[i];
        char b = cStr[i];
        if(a >= 'A' && a <= 'Z') a = (char)(a + 32);
        if(b >= 'A' && b <= 'Z') b = (char)(b + 32);
        if(a != b) return false;
    }
    return true;
}

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
    /* Key type: 0 = RSA (default), 1 = EC */
    int keyTypeEC = 0;
    UA_String eccCurve = UA_STRING_STATIC("prime256v1");

    if(params) {
        const UA_UInt16 *keySizeBitsValue = (const UA_UInt16 *)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "key-size-bits"), &UA_TYPES[UA_TYPES_UINT16]);
        if(keySizeBitsValue)
            keySizeBits = *keySizeBitsValue;

        const UA_UInt16 *expiresInDaysValue = (const UA_UInt16 *)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "expires-in-days"), &UA_TYPES[UA_TYPES_UINT16]);
        if(expiresInDaysValue)
            expiresInDays = *expiresInDaysValue;

        const UA_String *keyTypeValue = (const UA_String *)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "key-type"), &UA_TYPES[UA_TYPES_STRING]);
        if(keyTypeValue && uaStringEqualsCI_mbedtls(keyTypeValue, "ec"))
            keyTypeEC = 1;

        const UA_String *eccCurveValue = (const UA_String *)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "ecc-curve"), &UA_TYPES[UA_TYPES_STRING]);
        if(eccCurveValue && eccCurveValue->length > 0)
            eccCurve = *eccCurveValue;
    }

    UA_ByteString_init(outPrivateKey);
    UA_ByteString_init(outCertificate);
    UA_ByteString privateKeyOutput = UA_BYTESTRING_NULL;
    UA_ByteString certificateOutput = UA_BYTESTRING_NULL;

    mbedtls_pk_context key;
    UA_mbedTLS_PsaKey generatedKey;
    mbedtls_x509write_cert crt;

    UA_StatusCode errRet = UA_STATUSCODE_GOOD;

    /* Set to sane values */
    mbedtls_pk_init(&key);
    UA_mbedTLS_PsaKey_init(&generatedKey);
    mbedtls_x509write_crt_init(&crt);

    if(UA_mbedTLS_PSA_Init() != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Failed to initialize PSA Crypto.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* Generate a key pair */
    psa_key_attributes_t keyAttributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_algorithm_t hashAlgorithm = PSA_ALG_SHA_256;
    if(keyTypeEC &&
       (uaStringEqualsCI_mbedtls(&eccCurve, "secp384r1") ||
        uaStringEqualsCI_mbedtls(&eccCurve, "nistp384") ||
        uaStringEqualsCI_mbedtls(&eccCurve, "brainpoolp384r1")))
        hashAlgorithm = PSA_ALG_SHA_384;

    if(keyTypeEC) {
        psa_ecc_family_t family = PSA_ECC_FAMILY_SECP_R1;
        size_t keyBits = 256;
        if(uaStringEqualsCI_mbedtls(&eccCurve, "prime256v1") ||
           uaStringEqualsCI_mbedtls(&eccCurve, "nistp256")) {
            /* Defaults already selected. */
        } else if(uaStringEqualsCI_mbedtls(&eccCurve, "secp384r1") ||
                  uaStringEqualsCI_mbedtls(&eccCurve, "nistp384")) {
            keyBits = 384;
        } else if(uaStringEqualsCI_mbedtls(&eccCurve, "brainpoolp256r1")) {
            family = PSA_ECC_FAMILY_BRAINPOOL_P_R1;
        } else if(uaStringEqualsCI_mbedtls(&eccCurve, "brainpoolp384r1")) {
            family = PSA_ECC_FAMILY_BRAINPOOL_P_R1;
            keyBits = 384;
        } else if(uaStringEqualsCI_mbedtls(&eccCurve, "ed25519") ||
                  uaStringEqualsCI_mbedtls(&eccCurve, "ed448")) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                         "EdDSA certificate generation is not supported with mbedTLS.");
            errRet = UA_STATUSCODE_BADNOTIMPLEMENTED;
            goto cleanup;
        } else {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                         "Create Certificate: Unsupported ECC curve for mbedTLS.");
            errRet = UA_STATUSCODE_BADINVALIDARGUMENT;
            goto cleanup;
        }
        psa_set_key_type(&keyAttributes, PSA_KEY_TYPE_ECC_KEY_PAIR(family));
        psa_set_key_bits(&keyAttributes, keyBits);
        psa_set_key_algorithm(&keyAttributes, PSA_ALG_ECDSA(hashAlgorithm));
    } else {
        psa_set_key_type(&keyAttributes, PSA_KEY_TYPE_RSA_KEY_PAIR);
        psa_set_key_bits(&keyAttributes, keySizeBits);
        psa_set_key_algorithm(&keyAttributes,
                              PSA_ALG_RSA_PKCS1V15_SIGN(hashAlgorithm));
    }
    psa_set_key_usage_flags(&keyAttributes,
                            PSA_KEY_USAGE_EXPORT | PSA_KEY_USAGE_SIGN_HASH);
    psa_status_t psaStatus = psa_generate_key(&keyAttributes, &generatedKey.id);
    psa_reset_key_attributes(&keyAttributes);
    if(psaStatus != PSA_SUCCESS ||
       mbedtls_pk_copy_from_psa(generatedKey.id, &key) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Failed to generate certificate key pair with PSA Crypto.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* Setting certificate values */
    mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
    /* P-384 / brainpoolP384r1 use SHA-384; everything else uses SHA-256 */
    if(keyTypeEC &&
       (uaStringEqualsCI_mbedtls(&eccCurve, "secp384r1") ||
        uaStringEqualsCI_mbedtls(&eccCurve, "nistp384") ||
        uaStringEqualsCI_mbedtls(&eccCurve, "brainpoolp384r1")))
        mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA384);
    else
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
        /* Copy and null-terminate */
        char *subAlt = (char *)UA_malloc(subjectAltName[i].length + 1);
        if(!subAlt) {
            errRet = UA_STATUSCODE_BADOUTOFMEMORY;
            clearSanList(head);
            goto cleanup;
        }
        memcpy(subAlt, subjectAltName[i].data, subjectAltName[i].length);
        subAlt[subjectAltName[i].length] = 0;

        /* split into SAN type and value */
        char *sanType = NULL;
        for(char *char_pos = subAlt; *char_pos != 0; char_pos++) {
            if(*char_pos == ':') {
                *char_pos = '\0';
                sanType = subAlt;
                break;
            }
        }

        if(!sanType) {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURECHANNEL, "Invalid Input format");
            UA_free(subAlt);
            continue;
        }

        char *sanValue = (char *)subjectAltName[i].data + strlen(sanType) + 1;
        const char *sanValueTerminated = subAlt + strlen(sanType) + 1;
        size_t sanValueLength = subjectAltName[i].length - strlen(sanType) - 1;

        cur_tmp = (mbedtls_write_san_list*)mbedtls_calloc(1, sizeof(mbedtls_write_san_list));
        if(!cur_tmp) {
            UA_free(subAlt);
            errRet = UA_STATUSCODE_BADOUTOFMEMORY;
            clearSanList(head);
            goto cleanup;
        }
        cur_tmp->next = NULL;
        cur_tmp->node.host = sanValue;
        cur_tmp->node.hostlen = sanValueLength;

        if(strcmp(sanType, "DNS") == 0) {
            cur_tmp->node.type = MBEDTLS_X509_SAN_DNS_NAME;
        } else if(strcmp(sanType, "URI") == 0) {
            cur_tmp->node.type = MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER;
        } else if(strcmp(sanType, "IP") == 0) {
            uint8_t *ip = (uint8_t *)mbedtls_calloc(1, 4);
            if(!ip) {
                mbedtls_free(cur_tmp);
                UA_free(subAlt);
                continue;
            }
            if(musl_inet_pton(AF_INET, sanValueTerminated, ip) <= 0) {
                UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURECHANNEL, "IP SAN preparation failed");
                mbedtls_free(ip);
                mbedtls_free(cur_tmp);
                UA_free(subAlt);
                continue;
            }
            cur_tmp->node.type = MBEDTLS_X509_SAN_IP_ADDRESS;
            cur_tmp->node.host = (char *)ip;
            cur_tmp->node.hostlen = 4;
        } else if(strcmp(sanType, "RFC822") == 0) {
            cur_tmp->node.type = MBEDTLS_X509_SAN_RFC822_NAME;
        } else {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURECHANNEL, "Given an unsupported SAN");
            mbedtls_free(cur_tmp);
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
        clearSanList(head);
        goto cleanup;
    }

    clearSanList(head);

    /* RFC 5280 requires a positive serial number that is unique per issuer. */
    unsigned char serial[16];
    UA_ByteString serialBytes = {sizeof(serial), serial};
    if(UA_mbedTLS_PsaRandom(&serialBytes) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Generating the certificate serial number failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }
    serial[0] &= 0x7f; /* Keep the ASN.1 INTEGER positive. */
    UA_Boolean serialIsZero = true;
    for(size_t i = 0; i < sizeof(serial); i++)
        serialIsZero &= (serial[i] == 0);
    if(serialIsZero)
        serial[sizeof(serial) - 1] = 1;
    if(mbedtls_x509write_crt_set_serial_raw(&crt, serial, sizeof(serial)) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Setting the certificate serial number failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* Use the open62541 UTC conversion. Unlike gmtime, this is reentrant. */
    UA_DateTime currentTime = UA_DateTime_now();
    UA_DateTime futureTime = currentTime +
        (UA_DateTime)expiresInDays * 24 * 60 * 60 * UA_DATETIME_SEC;
    char current_timestamp[15]; /* YYYYMMDDhhmmss + '\0' */
    if(!formatCertificateTime(currentTime, current_timestamp)) {
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }
    char future_timestamp[15]; /* YYYYMMDDhhmmss + '\0' */
    if(!formatCertificateTime(futureTime, future_timestamp)) {
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

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

    /* ECC certificates need keyAgreement for ECDH instead of keyEncipherment */
    unsigned int keyUsageFlags = keyTypeEC
        ? (MBEDTLS_X509_KU_DIGITAL_SIGNATURE | MBEDTLS_X509_KU_NON_REPUDIATION |
           MBEDTLS_X509_KU_KEY_AGREEMENT)
        : (MBEDTLS_X509_KU_DIGITAL_SIGNATURE | MBEDTLS_X509_KU_NON_REPUDIATION |
           MBEDTLS_X509_KU_KEY_ENCIPHERMENT | MBEDTLS_X509_KU_DATA_ENCIPHERMENT);
    if(mbedtls_x509write_crt_set_key_usage(&crt, keyUsageFlags) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Setting key usage failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    mbedtls_asn1_sequence *ext_key_usage;
    ext_key_usage = (mbedtls_asn1_sequence *)mbedtls_calloc(1, sizeof(mbedtls_asn1_sequence));
    if(!ext_key_usage) {
        errRet = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }
    ext_key_usage->buf.tag = MBEDTLS_ASN1_OID;
    SET_OID(ext_key_usage->buf, MBEDTLS_OID_SERVER_AUTH);
    ext_key_usage->next = (mbedtls_asn1_sequence *)mbedtls_calloc(1, sizeof(mbedtls_asn1_sequence));
    if(!ext_key_usage->next) {
        mbedtls_free(ext_key_usage);
        errRet = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }
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
    if((write_private_key(&key, certFormat, &privateKeyOutput)) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Writing private key failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* Write Certificate */
    if(write_certificate(&crt, certFormat, &certificateOutput) != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Create Certificate: Writing certificate failed.");
        errRet = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    *outPrivateKey = privateKeyOutput;
    UA_ByteString_init(&privateKeyOutput);
    *outCertificate = certificateOutput;
    UA_ByteString_init(&certificateOutput);

cleanup:
    UA_mbedTLS_clearSensitiveByteString(&privateKeyOutput);
    UA_ByteString_clear(&certificateOutput);
    UA_mbedTLS_PsaKey_clear(&generatedKey);
    mbedtls_x509write_crt_free(&crt);
    mbedtls_pk_free(&key);
    return errRet;
}

static int write_private_key(mbedtls_pk_context *key, UA_CertificateFormat keyFormat, UA_ByteString *outPrivateKey) {
    if(!key || !outPrivateKey)
        return -1;

    int ret = -1;
    UA_ByteString_init(outPrivateKey);
    unsigned char output_buf[16000] = {0};
    unsigned char *c = output_buf;
    size_t len = 0;

    switch(keyFormat) {
    case UA_CERTIFICATEFORMAT_DER: {
        ret = mbedtls_pk_write_key_der(key, output_buf, sizeof(output_buf));
        if(ret <= 0)
            goto cleanup;

        len = (size_t)ret;
        c = output_buf + sizeof(output_buf) - len;
        break;
    }
    case UA_CERTIFICATEFORMAT_PEM: {
        ret = mbedtls_pk_write_key_pem(key, output_buf, sizeof(output_buf));
        if(ret != 0)
            goto cleanup;

        len = strlen((char *)output_buf);
        break;
    }
    default:
        goto cleanup;
    }

    if(UA_ByteString_allocBuffer(outPrivateKey, len) != UA_STATUSCODE_GOOD) {
        ret = -1;
        goto cleanup;
    }
    memcpy(outPrivateKey->data, c, len);
    ret = 0;

cleanup:
    mbedtls_platform_zeroize(output_buf, sizeof(output_buf));
    if(ret != 0)
        UA_mbedTLS_clearSensitiveByteString(outPrivateKey);
    return ret;
}

static int write_certificate(mbedtls_x509write_cert *crt, UA_CertificateFormat certFormat,
                             UA_ByteString *outCertificate) {
    int ret;
    unsigned char output_buf[4096];
    unsigned char *c = output_buf;
    size_t len = 0;

    memset(output_buf, 0, sizeof(output_buf));
    switch(certFormat) {
    case UA_CERTIFICATEFORMAT_DER: {
        ret = UA_mbedTLS_compat_writeCertificateDer(
            crt, output_buf, sizeof(output_buf));
        if(ret < 0)
            return ret;

        len = (size_t)ret;
        c = output_buf + sizeof(output_buf) - len;
        break;
    }
    case UA_CERTIFICATEFORMAT_PEM: {
        ret = UA_mbedTLS_compat_writeCertificatePem(
            crt, output_buf, sizeof(output_buf));
        if(ret < 0)
            return ret;

        len = strlen((char *)output_buf);
        break;
    }
    }

    if(UA_ByteString_allocBuffer(outCertificate, len) != UA_STATUSCODE_GOOD)
        return -1;
    memcpy(outCertificate->data, c, len);

    return 0;
}

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
    int ret = 0;
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
