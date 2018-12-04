/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Markus Karch, Fraunhofer IOSB
 */

#include "ua_ca_gnutls.h"
#include <gnutls/x509.h>
#include "ua_types_generated_handling.h"

#ifdef UA_ENABLE_GDS_CM


#define UA_LOG_GNUERR                                                  \
    UA_LOG_WARNING(scg->logger, UA_LOGCATEGORY_SERVER, \
                   "gnuTLS returned an error: %s", gnutls_strerror(gnuErr));           \

#define UA_GNUTLS_ERRORHANDLING(errorcode)                             \
    if(gnuErr < 0) {                                                       \
        UA_LOG_GNUERR                                                  \
        ret = errorcode;                                             \
    }

#define UA_GNUTLS_ERRORHANDLING_RETURN(errorcode)                      \
    if(gnuErr < 0) {                                                       \
        UA_LOG_GNUERR                                                  \
        return errorcode;                                               \
    }


typedef struct {
    gnutls_x509_crt_t ca_crt;
    gnutls_x509_privkey_t ca_key;
    size_t serialNumberPosition;
    size_t serialNumberSize;
    char *serialNumber;
    int trustListSize;
    gnutls_x509_trust_list_t trustList;
    gnutls_x509_crl_t  crl;
} CaContext;

/*
void printCa_gnutls(GDS_CA *scg, const char *loc) {
    CaContext *cc = (CaContext *) scg->context;
    gnutls_datum_t crtdata = {0};
    gnutls_x509_crt_export(cc->ca_crt, GNUTLS_X509_FMT_DER, NULL, (size_t*)&crtdata.size);
    crtdata.data = (unsigned char *) malloc(crtdata.size);
    gnutls_x509_crt_export(cc->ca_crt, GNUTLS_X509_FMT_DER, crtdata.data, (size_t*)&crtdata.size);
    FILE *f = fopen(loc, "w");
    fwrite(crtdata.data, crtdata.size, 1, f);
    fclose(f);
    free(crtdata.data);
}
*/

static void deleteMembers_gnutls(UA_GDS_CA *cg) {
    if(cg == NULL)
        return;

    if(cg->context == NULL)
        return;

    CaContext *cc = (CaContext *) cg->context;

    gnutls_x509_crl_deinit(cc->crl);
    gnutls_x509_privkey_deinit(cc->ca_key);
    gnutls_x509_trust_list_deinit(cc->trustList, 1); //CA Certificate will be freed with this call
    gnutls_global_deinit();
    UA_free(cc->serialNumber);
    UA_free(cc);
    cg->context = NULL;
}


static UA_StatusCode generate_private_key(UA_GDS_CA *scg,
                                              gnutls_x509_privkey_t *privKey,
                                              unsigned int bits) {

    int gnuErr = gnutls_x509_privkey_init(privKey);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADOUTOFMEMORY);

    gnuErr = gnutls_x509_privkey_generate(*privKey, GNUTLS_PK_RSA, bits, 0);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADINTERNALERROR);

    return UA_STATUSCODE_GOOD;
}

/*
static void save_x509(gnutls_x509_crt_t crt, const char *loc) {
    gnutls_datum_t crtdata = {0};
    gnutls_x509_crt_export(crt, GNUTLS_X509_FMT_DER, NULL, (size_t*)&crtdata.size);
    crtdata.data = (unsigned char *) malloc(crtdata.size);
    gnutls_x509_crt_export(crt, GNUTLS_X509_FMT_DER, crtdata.data, (size_t*)&crtdata.size);
    FILE *f = fopen(loc, "w");
    fwrite(crtdata.data, crtdata.size, 1, f);
    fclose(f);
    free(crtdata.data);
}
*/

static
UA_StatusCode setCommonCertificateFields(UA_GDS_CA *scg, gnutls_x509_crt_t *cert) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;

    int gnuErr = gnutls_x509_crt_set_version(*cert, 3);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    //TODO using time.h might be an issue
    gnuErr = gnutls_x509_crt_set_activation_time(*cert, time(NULL));
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    //TODO check if expiration time with ca certificate
    gnuErr = gnutls_x509_crt_set_expiration_time(*cert, time(NULL) + (60 * 60 * 24 * 365 * 5));
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    gnuErr = gnutls_x509_crt_set_ca_status (*cert, 0);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    gnuErr = gnutls_x509_crt_set_key_usage(*cert,
                                           GNUTLS_KEY_DIGITAL_SIGNATURE
                                           | GNUTLS_KEY_NON_REPUDIATION
                                           | GNUTLS_KEY_DATA_ENCIPHERMENT
                                           | GNUTLS_KEY_KEY_ENCIPHERMENT );

    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    gnuErr = gnutls_x509_crt_set_key_purpose_oid (*cert, GNUTLS_KP_TLS_WWW_SERVER, 0);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    gnuErr = gnutls_x509_crt_set_key_purpose_oid (*cert, GNUTLS_KP_TLS_WWW_CLIENT, 0);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    return ret;
}

static
UA_StatusCode export_cert_and_issuer(UA_GDS_CA *scg,
                                     gnutls_x509_crt_t *cert,
                                     gnutls_x509_crt_t *ca_crt,
                                     UA_ByteString *certificate,
                                     UA_ByteString **issuerCertificates){

    char buffer[1024 * 10];

    //Export issued certificate
    memset(buffer, 0, sizeof(buffer));
    size_t buffer_size = sizeof(buffer);
    int gnuErr = gnutls_x509_crt_export(*cert, GNUTLS_X509_FMT_DER, buffer, &buffer_size);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    UA_StatusCode ret = UA_ByteString_allocBuffer(certificate, buffer_size + 1);
    if (ret != UA_STATUSCODE_GOOD)
        return ret;

    memcpy(certificate->data, buffer, buffer_size);
    certificate->data[buffer_size] = '\0';
    certificate->length--;


    //Export issuer certificate
    *issuerCertificates = (UA_ByteString *) UA_calloc(sizeof(UA_ByteString), 1);
    memset(buffer, 0, sizeof(buffer));
    buffer_size = sizeof(buffer);
    gnuErr = gnutls_x509_crt_export(*ca_crt, GNUTLS_X509_FMT_DER, buffer, &buffer_size);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    UA_ByteString_allocBuffer(*issuerCertificates, buffer_size + 1);
    if (ret != UA_STATUSCODE_GOOD)
        return ret;

    memcpy((*issuerCertificates)[0].data, buffer, buffer_size);
    (*issuerCertificates)[0].data[buffer_size] = '\0';
    (*issuerCertificates)[0].length--;

    return UA_STATUSCODE_GOOD;
}


//TODO csr_gnutls check key size within csr
static
UA_StatusCode csr_gnutls(UA_GDS_CA *scg,
                                unsigned int supposedKeySize,
                                UA_ByteString *certificateSigningRequest,
                                UA_ByteString *certificate,
                                size_t *issuerCertificateSize,
                                UA_ByteString **issuerCertificates) {

    UA_StatusCode ret = UA_STATUSCODE_GOOD;

    if(scg == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    CaContext *cc = (CaContext *) scg->context;

    gnutls_x509_crq_t crq;
    gnutls_x509_crt_t cert;

    gnutls_datum_t data = {NULL, 0};
    data.data = certificateSigningRequest->data;
    data.size = (unsigned int) certificateSigningRequest->length ;

    int gnuErr = gnutls_x509_crq_init(&crq);
    if (gnuErr < 0) {
        gnutls_x509_crq_deinit(crq);
        UA_LOG_GNUERR;
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    gnuErr = gnutls_x509_crt_init(&cert);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADOUTOFMEMORY);
    if(ret != UA_STATUSCODE_GOOD)
        goto deinit_csr;

    gnuErr = gnutls_x509_crq_import(crq, &data, GNUTLS_X509_FMT_DER);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(ret != UA_STATUSCODE_GOOD)
        goto deinit_csr;

    //verify signature of CSR
    gnuErr = gnutls_x509_crq_verify(crq, 0);
    if (GNUTLS_E_PK_SIG_VERIFY_FAILED == gnuErr) {
        ret = UA_STATUSCODE_BADREQUESTNOTALLOWED;
        goto deinit_csr;
    }

    //Create Certificate
    gnuErr = gnutls_x509_crt_set_crq(cert, crq);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(ret != UA_STATUSCODE_GOOD)
        goto deinit_csr;

    //    char bufferDN[1024];
    //    size_t bufferDN_size = sizeof(bufferDN);
    //    gnuErr = gnutls_x509_crq_get_dn(crq, bufferDN, &bufferDN_size);
    //    gnuErr = gnutls_x509_crt_set_dn (cert, bufferDN, NULL);

    //Increase the serial number
    if (cc->serialNumber[cc->serialNumberPosition] < CHAR_MAX) {
        cc->serialNumber[cc->serialNumberPosition]++;
    }
    else {
        cc->serialNumberPosition--;
        for (int index = (int) cc->serialNumberPosition; index >= 0; index--) {
            if (cc->serialNumber[index] < CHAR_MAX) {
                for (int i = index + 1; i < (int) cc->serialNumberSize; i++) {
                    cc->serialNumber[i] = 0;
                }
                cc->serialNumber[index]++;
                cc->serialNumberPosition = cc->serialNumberSize - 1;
                break;
            }
        }
        if (cc->serialNumberPosition != cc->serialNumberSize - 1){
            UA_LOG_WARNING(scg->logger, UA_LOGCATEGORY_SERVER, "Serial number overflow");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }
    gnuErr = gnutls_x509_crt_set_serial(cert, cc->serialNumber, cc->serialNumberSize);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    ret = setCommonCertificateFields(scg, &cert);
    if (ret != UA_STATUSCODE_GOOD)
        goto deinit_csr;

    unsigned char buf[20]; // SHA-1 with 20 bytes
    size_t size = sizeof(buf);
    gnuErr = gnutls_x509_crq_get_key_id(crq, 0, buf, &size );
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(ret != UA_STATUSCODE_GOOD)
        goto deinit_csr;

    gnuErr = gnutls_x509_crt_set_subject_key_id (cert, buf, size);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(ret != UA_STATUSCODE_GOOD)
        goto deinit_csr;

    size = sizeof(buf);
    gnuErr = gnutls_x509_crt_get_key_id(cc->ca_crt, 0, buf, &size);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(ret != UA_STATUSCODE_GOOD)
        goto deinit_csr;

    gnuErr = gnutls_x509_crt_set_authority_key_id(cert, buf, size);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(ret != UA_STATUSCODE_GOOD)
        goto deinit_csr;

    //get SAN from CSR
    unsigned int index = 0;
    char buffer[1024 * 10];
    size_t buffer_size = sizeof(buffer);
    unsigned int sanType;
    unsigned int critical = 0;
    while (gnutls_x509_crq_get_subject_alt_name(crq, index, buffer,
                                                &buffer_size, &sanType, &critical)
            != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
        gnuErr = gnutls_x509_crt_set_subject_alt_name (cert,
                                                       (gnutls_x509_subject_alt_name_t) sanType,
                                                        buffer,
                                                       (unsigned int) buffer_size,
                                                       GNUTLS_FSAN_APPEND);
        if(ret != UA_STATUSCODE_GOOD)
            goto deinit_csr;
        buffer_size = sizeof(buffer); //important otherwise there are parsing issues
        index++;
    }

    gnuErr = gnutls_x509_crt_sign2(cert, cc->ca_crt, cc->ca_key, GNUTLS_DIG_SHA256, 0);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    //Export certificate
    *issuerCertificateSize = 1;
    export_cert_and_issuer(scg, &cert, &cc->ca_crt,certificate, issuerCertificates);

deinit_csr:
    gnutls_x509_crq_deinit(crq);
    gnutls_x509_crt_deinit(cert);

    return ret;
}




//TODO insufficient detection - this has to be improved
static
gnutls_x509_subject_alt_name_t detectSubjectAltName(UA_String *name) {
    char *str = (char *) name->data;

    if (strncmp("urn:", str, 4) == 0)
        return GNUTLS_SAN_URI;

    struct sockaddr_in sa;
    if (inet_pton(AF_INET, str, &(sa.sin_addr)))
        return GNUTLS_SAN_IPADDRESS;

    return GNUTLS_SAN_DNSNAME;
}



//TODO implement privateKey password
//TODO implement pfx support for private key, right now only pem is implemented (part12/p.34)
//example for pfx generation: https://www.gnutls.org/manual/gnutls.html#PKCS12-structure-generation-example
static
UA_StatusCode createNewKeyPair_gnutls (UA_GDS_CA *scg,
                                   UA_String *subjectName,
                                   UA_String *privateKeyFormat,
                                   UA_String *privateKeyPassword,
                                   unsigned  int keySize,
                                   size_t domainNamesSize,
                                   UA_String *domainNamesArray,
                                   UA_ByteString *certificate,
                                   UA_ByteString *privateKey,
                                   size_t *issuerCertificateSize,
                                   UA_ByteString **issuerCertificates) {



    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_String subjectName_nullTerminated;

    if(scg == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    CaContext *cc = (CaContext *) scg->context;
    if(cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    gnutls_x509_crt_t cert;
    gnutls_x509_privkey_t privkey;
    int gnuErr = gnutls_x509_crt_init(&cert);
    if (gnuErr < 0) {
        gnutls_x509_crt_deinit(cert);
        UA_LOG_GNUERR;
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    ret = generate_private_key(scg, &privkey, keySize);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if (ret != UA_STATUSCODE_GOOD){
        return ret;
    }

    //export privatekey
    unsigned char buffer[10 * 1024];
    size_t buf_size = sizeof(buffer);
    memset(buffer, 0, buf_size);
    gnuErr = gnutls_x509_privkey_export(privkey, GNUTLS_X509_FMT_DER, buffer, &buf_size);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(ret != UA_STATUSCODE_GOOD)
        goto deinit_create;

    UA_ByteString_allocBuffer(privateKey, buf_size + 1 );
    memcpy(privateKey->data, buffer, buf_size);
    privateKey->data[buf_size] = '\0';
    privateKey->length--;

    //gnutls_x509_crt_set_dn requires null terminated string
    subjectName_nullTerminated.length = subjectName->length + 1;
    subjectName_nullTerminated.data = (UA_Byte *)
            UA_calloc(subjectName_nullTerminated.length, sizeof(UA_Byte));
    memcpy(subjectName_nullTerminated.data, subjectName->data, subjectName->length);
    subjectName_nullTerminated.length--;

    gnuErr = gnutls_x509_crt_set_dn(cert, (char *) subjectName_nullTerminated.data, NULL);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(ret != UA_STATUSCODE_GOOD)
        goto deinit_create;

    gnuErr = gnutls_x509_crt_set_key(cert, privkey);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(ret != UA_STATUSCODE_GOOD)
        goto deinit_create;

    if (cc->serialNumber[cc->serialNumberPosition] < CHAR_MAX) {
        cc->serialNumber[cc->serialNumberPosition]++;
    }
    else {
       cc->serialNumberPosition--;
       for (int index = (int) cc->serialNumberPosition; index >= 0; index--) {
           if (cc->serialNumber[index] < CHAR_MAX) {
               for (int i = index + 1; i < (int) cc->serialNumberSize; i++) {
                   cc->serialNumber[i] = 0;
               }
               cc->serialNumber[index]++;
               cc->serialNumberPosition = cc->serialNumberSize - 1;
               break;
           }
       }
       if (cc->serialNumberPosition != cc->serialNumberSize - 1){
           UA_LOG_WARNING(scg->logger, UA_LOGCATEGORY_SERVER, "Serial number overflow");
           return UA_STATUSCODE_BADINTERNALERROR;
       }
    }
    gnuErr = gnutls_x509_crt_set_serial(cert, cc->serialNumber, cc->serialNumberSize);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    ret = setCommonCertificateFields(scg, &cert);
    if (ret != UA_STATUSCODE_GOOD)
        goto deinit_create;

    unsigned char buf[20]; // SHA-1 with 20 bytes
    size_t size = sizeof(buf);
    gnuErr = gnutls_x509_privkey_get_key_id(privkey,0, buf, &size);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(ret != UA_STATUSCODE_GOOD)
        goto deinit_create;

    gnuErr = gnutls_x509_crt_set_subject_key_id (cert, buf, size);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(ret != UA_STATUSCODE_GOOD)
        goto deinit_create;

    size = sizeof(buf);
    gnuErr = gnutls_x509_crt_get_key_id(cc->ca_crt, 0, buf, &size);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(ret != UA_STATUSCODE_GOOD)
        goto deinit_create;

    gnuErr = gnutls_x509_crt_set_authority_key_id(cert, buf, size);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(ret != UA_STATUSCODE_GOOD)
        goto deinit_create;

    for (size_t i = 0; i < domainNamesSize; i++) {
        UA_String san_nullTerminated;
        san_nullTerminated.length = domainNamesArray[i].length + 1;
        san_nullTerminated.data = (UA_Byte *)
                UA_calloc(san_nullTerminated.length, sizeof(UA_Byte));
        memcpy(san_nullTerminated.data, domainNamesArray[i].data, domainNamesArray[i].length);
        san_nullTerminated.length--;

        gnutls_x509_subject_alt_name_t san =
                detectSubjectAltName(&san_nullTerminated);

        if (san == GNUTLS_SAN_IPADDRESS){
            struct sockaddr_in sa;
            inet_pton(AF_INET, (char *) san_nullTerminated.data, &(sa.sin_addr));

            gnuErr = gnutls_x509_crt_set_subject_alt_name (cert,
                                                           san,
                                                           &sa.sin_addr,
                                                           sizeof(sa.sin_addr),
                                                           GNUTLS_FSAN_APPEND);
        }
        else {

            gnuErr = gnutls_x509_crt_set_subject_alt_name (cert,
                                                           san,
                                                           domainNamesArray[i].data,
                                                           (unsigned int)domainNamesArray[i].length,
                                                           GNUTLS_FSAN_APPEND);

        }

        UA_String_deleteMembers(&san_nullTerminated);

        UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
        if(ret != UA_STATUSCODE_GOOD)
            goto deinit_create;
    }

    gnuErr = gnutls_x509_crt_sign2(cert, cc->ca_crt, cc->ca_key, GNUTLS_DIG_SHA256, 0);
    UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    if(ret != UA_STATUSCODE_GOOD)
        goto deinit_create;


    *issuerCertificateSize = 1;
    export_cert_and_issuer(scg, &cert, &cc->ca_crt, certificate, issuerCertificates);

deinit_create:
    if (!UA_String_equal(&subjectName_nullTerminated, &UA_STRING_NULL)) {
        UA_String_deleteMembers(&subjectName_nullTerminated);
    }
    gnutls_x509_crt_deinit(cert);
    gnutls_x509_privkey_deinit(privkey);
    return ret;

}

static
UA_StatusCode addCertificatetoCRL_gnutls(UA_GDS_CA *scg,
                                  size_t serialNumberSize,
                                  char *serialNumber,
                                  UA_ByteString certificate) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;

    //gnutls supports serial numbers up to 40 bytes
    if (scg == NULL || serialNumberSize < 1 || serialNumberSize > 40) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    CaContext *cc = (CaContext *) scg->context;
    if (cc == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    int gnuErr = gnutls_x509_crl_set_crt_serial(cc->crl, serialNumber, serialNumberSize, time(NULL));
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADINVALIDARGUMENT);

    gnuErr = gnutls_x509_crl_sign2(cc->crl, cc->ca_crt, cc->ca_key, GNUTLS_DIG_SHA256, 0);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

/*
    char buffer[1024 * 10];
    //Export issued certificate
    memset(buffer, 0, sizeof(buffer));
    size_t buffer_size = sizeof(buffer);
    gnuErr = gnutls_x509_crl_export(cc->crl, GNUTLS_X509_FMT_DER, buffer, &buffer_size);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);


    FILE *f = fopen("test2.crl", "w");
    fwrite(buffer, buffer_size, 1, f);
    fclose(f);
*/

    return ret;
}

//TODO: Implement function
static
UA_StatusCode getCertificateStatus_gnutls(UA_GDS_CA *scg,
                                    UA_ByteString *certificate,
                                    UA_Boolean *updateRequired) {
    return UA_STATUSCODE_GOOD;
}
//TODO: Implement function
static
UA_StatusCode isCertificatefromCA_gnutls(UA_GDS_CA *scg,
                                  UA_ByteString certificate,
                                  UA_Boolean *result) {
    return UA_STATUSCODE_GOOD;
}

static
UA_StatusCode removeCertificateFromTrustlist_gnutls(UA_GDS_CA *scg,
                                                    UA_String *thumbprint,
                                                    UA_Boolean isTrustedCertificate){
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    //Plugin does not implement intermediate CAs at the moment
    if (scg == NULL || thumbprint == NULL || !isTrustedCertificate){
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if (thumbprint->length != 20) { //Shall be a sha-1 value
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    CaContext *cc = (CaContext *) scg->context;
    if (cc == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    gnutls_x509_trust_list_iter_t trust_iter = NULL;
    gnutls_x509_crt_t crt;
    gnutls_datum_t get_datum;
    char hash[20];
    size_t size = sizeof(hash);
    while (gnutls_x509_trust_list_iter_get_ca(cc->trustList, &trust_iter, &crt) !=
           GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
        
        int gnuErr = gnutls_x509_crt_export2(crt, GNUTLS_X509_FMT_DER, &get_datum);
        UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
        if (ret != UA_STATUSCODE_GOOD) {
            gnutls_x509_crt_deinit(crt);
            gnutls_free(get_datum.data);
            break;
        }

        gnuErr = gnutls_fingerprint(GNUTLS_DIG_SHA1, &get_datum, hash, &size);
        UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
        if (ret != UA_STATUSCODE_GOOD) {
            gnutls_x509_crt_deinit(crt);
            gnutls_free(get_datum.data);
            break;
        }

        if (!memcmp(hash, thumbprint->data, 20)) {
            gnuErr = gnutls_x509_trust_list_remove_cas(cc->trustList, &crt, 1);
            UA_GNUTLS_ERRORHANDLING(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
            if (gnuErr == 1){
                cc->trustListSize--;
            }
            gnutls_x509_crt_deinit(crt);
            gnutls_free(get_datum.data);
            break;
        }

        gnutls_x509_crt_deinit(crt);
        gnutls_free(get_datum.data);
    }
    gnutls_x509_trust_list_iter_deinit(trust_iter);

    return ret;
}

static
UA_StatusCode addCertificateToTrustList_gnutls(UA_GDS_CA *scg,
                                UA_ByteString *certificate,
                                UA_Boolean isTrustedCertificate){
    UA_StatusCode ret = UA_STATUSCODE_GOOD;

    //Plugin does not implement intermediate CAs at the moment
    if (scg == NULL || certificate == NULL || !isTrustedCertificate){
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    CaContext *cc = (CaContext *) scg->context;
    if (cc == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    gnutls_x509_trust_list_t trustList = cc->trustList;
    if (trustList == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    gnutls_x509_crt_t cert;
    gnutls_datum_t data = {NULL, 0};
    data.data = certificate->data;
    data.size = (unsigned int) certificate->length ;
    gnutls_x509_crt_init(&cert);
    int gnuErr = gnutls_x509_crt_import(cert, &data, GNUTLS_X509_FMT_DER);
    if (gnuErr < 0){
       goto error;
    }

    int num = gnutls_x509_trust_list_add_cas(cc->trustList, &cert, (size_t) 1, GNUTLS_TL_NO_DUPLICATES);
    if (num != 1) {
        goto error;
    }
    cc->trustListSize += num;

    return ret;

error:
    gnutls_x509_crt_deinit(cert);
    UA_LOG_GNUERR;
    return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
}
static
UA_StatusCode getTrustList_gnutls(UA_GDS_CA *scg,
                           UA_TrustListDataType *tl) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;

    CaContext *cc = (CaContext *) scg->context;
    gnutls_x509_trust_list_iter_t trust_iter = NULL;
    gnutls_x509_crt_t get_ca_crt;
    gnutls_datum_t get_datum;
    size_t n_get_ca_crts = 0;
    int gnuErr = 0;

    tl->trustedCertificatesSize = (size_t) cc->trustListSize;
    tl->trustedCertificates = (UA_ByteString *)
            UA_calloc(tl->trustedCertificatesSize, sizeof(UA_ByteString));

    while (gnutls_x509_trust_list_iter_get_ca(cc->trustList, &trust_iter, &get_ca_crt) !=
           GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
        gnuErr = gnutls_x509_crt_export2(get_ca_crt, GNUTLS_X509_FMT_DER, &get_datum);
        UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

        tl->trustedCertificates[n_get_ca_crts].length = get_datum.size;
        tl->trustedCertificates[n_get_ca_crts].data =
                (UA_Byte *) UA_malloc(get_datum.size * sizeof(UA_Byte));
        memcpy(tl->trustedCertificates[n_get_ca_crts].data, get_datum.data, get_datum.size);

        gnutls_x509_crt_deinit(get_ca_crt);
        gnutls_free(get_datum.data);

        ++n_get_ca_crts;
    }

    tl->trustedCrlsSize = 1;
    tl->trustedCrls = (UA_ByteString *)
            UA_calloc(1, sizeof(UA_ByteString));

    gnuErr = gnutls_x509_crl_export2(cc->crl, GNUTLS_X509_FMT_DER, &get_datum);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    tl->trustedCrls[0].length = get_datum.size;
    tl->trustedCrls[0].data = (UA_Byte *) UA_malloc(get_datum.size * sizeof(UA_Byte));
    memcpy(tl->trustedCrls[0].data, get_datum.data, get_datum.size);

    tl->issuerCrlsSize = 0;
    tl->issuerCrls = NULL;
    tl->issuerCertificatesSize = 0;
    tl->issuerCertificates = NULL;

    gnutls_x509_crt_deinit(get_ca_crt);
    gnutls_free(get_datum.data);

    return ret;
}


static
UA_StatusCode create_TrustList(UA_GDS_CA *scg){
    CaContext *cc = (CaContext *) scg->context;
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    int gnuErr = gnutls_x509_trust_list_init(&cc->trustList, 0);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    int num = gnutls_x509_trust_list_add_cas(cc->trustList,
                                             &cc->ca_crt, (size_t) 1,
                                             GNUTLS_TL_NO_DUPLICATES);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    cc->trustListSize += num;

    return ret;
}


static
UA_StatusCode create_CACertificate(UA_GDS_CA *scg,
                                    UA_String caName,
                                    unsigned int caDays,
                                    size_t serialNumberSize,
                                    char *serialNumber,
                                    unsigned int caBitKeySize){
    CaContext *cc = (CaContext *) scg->context;

    int gnuErr = gnutls_x509_crt_init (&cc->ca_crt);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    UA_StatusCode ret = generate_private_key(scg, &cc->ca_key, caBitKeySize);
    if (ret != UA_STATUSCODE_GOOD)
        return ret;

    gnuErr = gnutls_x509_crt_set_dn (cc->ca_crt, (char *) caName.data, NULL);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    gnuErr = gnutls_x509_crt_set_key(cc->ca_crt, cc->ca_key);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    gnutls_x509_crt_set_version(cc->ca_crt, 3);

    cc->serialNumberSize = serialNumberSize;
    cc->serialNumber = (char *) UA_malloc(serialNumberSize * sizeof(char));
    cc->serialNumberPosition = serialNumberSize - 1;
    memcpy(cc->serialNumber, serialNumber, serialNumberSize);
    gnuErr = gnutls_x509_crt_set_serial(cc->ca_crt, cc->serialNumber, cc->serialNumberSize);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    //TODO: This might be an issue (using time.h)
    gnuErr = gnutls_x509_crt_set_activation_time(cc->ca_crt, time(NULL));
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    gnuErr = gnutls_x509_crt_set_expiration_time(cc->ca_crt, time(NULL) + caDays);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    unsigned char buff[20];
    size_t size = sizeof(buff);
    gnuErr = gnutls_x509_crt_get_key_id(cc->ca_crt, 0, buff, &size);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);


    gnuErr = gnutls_x509_crt_set_subject_key_id (cc->ca_crt, buff, size);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);


    gnuErr = gnutls_x509_crt_set_authority_key_id(cc->ca_crt, buff, size);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    gnuErr = gnutls_x509_crt_set_ca_status (cc->ca_crt, 1);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    gnuErr = gnutls_x509_crt_set_key_usage(cc->ca_crt,
                                           GNUTLS_KEY_DIGITAL_SIGNATURE
                                           | GNUTLS_KEY_CRL_SIGN
                                           | GNUTLS_KEY_KEY_CERT_SIGN);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    gnuErr = gnutls_x509_crt_sign2(cc->ca_crt, cc->ca_crt, cc->ca_key, GNUTLS_DIG_SHA256, 0);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    ////////Create CRL
    gnuErr = gnutls_x509_crl_init(&cc->crl);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    gnuErr = gnutls_x509_crl_set_authority_key_id(cc->crl, buff, size);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    gnuErr = gnutls_x509_crl_set_this_update(cc->crl, time(NULL));
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    gnuErr = gnutls_x509_crl_set_next_update(cc->crl, time(NULL) + (60 * 60 * 24 * 365 * 1));
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    gnuErr = gnutls_x509_crl_set_version (cc->crl, 2);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);


    char crlSerialNumber = 1;
    gnuErr = gnutls_x509_crl_set_number(cc->crl, &crlSerialNumber, sizeof(char));
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    gnuErr = gnutls_x509_crl_sign2(cc->crl, cc->ca_crt, cc->ca_key, GNUTLS_DIG_SHA256, 0);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

/*
    char buffer[1024 * 10];
    //Export issued certificate
    memset(buffer, 0, sizeof(buffer));
    size_t buffer_size = sizeof(buffer);
    gnuErr = gnutls_x509_crl_export(cc->crl, GNUTLS_X509_FMT_DER, buffer, &buffer_size);
    UA_GNUTLS_ERRORHANDLING_RETURN(UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    FILE *f = fopen("test.crl", "w");
    fwrite(buffer, buffer_size, 1, f);
    fclose(f);
*/
    return ret;
}

static
UA_StatusCode create_caContext(UA_GDS_CA *scg,
                               UA_String caName,
                               unsigned int caDays,
                               size_t serialNumberSize,
                               char *serialNumber,
                               unsigned int caBitKeySize) {
    UA_StatusCode ret;

    if(scg == NULL || serialNumberSize < 1 || serialNumberSize > 40)
        return UA_STATUSCODE_BADINTERNALERROR;

    CaContext *cc = (CaContext *) UA_calloc(sizeof(CaContext), 1);
    scg->context = (void *)cc;
    if(!cc) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    /* Initialize the CaContext */
    gnutls_global_init();

    ret = create_CACertificate(scg, caName, caDays, serialNumberSize, serialNumber, caBitKeySize);
    if (ret != UA_STATUSCODE_GOOD)
        goto error;

    ret = create_TrustList(scg);
    if (ret != UA_STATUSCODE_GOOD)
        goto error;


    return UA_STATUSCODE_GOOD;

    error:
    UA_LOG_ERROR(scg->logger, UA_LOGCATEGORY_SECURITYPOLICY, "Could not create CaContext");
    if(scg->context!= NULL)
        deleteMembers_gnutls(scg);
    return ret;
}

UA_StatusCode UA_initCA(UA_GDS_CA *scg,
                        UA_String caName,
                        unsigned int caDays,
                        size_t serialNumberSize,
                        char *serialNumber,
                        unsigned int caBitKeySize,
                        UA_Logger *logger) {
    memset(scg, 0, sizeof(UA_GDS_CA));

    scg->logger = logger;
    scg->certificateSigningRequest = csr_gnutls;
    scg->createNewKeyPair = createNewKeyPair_gnutls;
    scg->addCertificateToTrustList = addCertificateToTrustList_gnutls;
    scg->removeCertificateFromTrustlist = removeCertificateFromTrustlist_gnutls;
    scg->addCertificatetoCRL = addCertificatetoCRL_gnutls;
    scg->getTrustList = getTrustList_gnutls;
    scg->getCertificateStatus = getCertificateStatus_gnutls;
    scg->isCertificatefromCA = isCertificatefromCA_gnutls;
    scg->deleteMembers = deleteMembers_gnutls;

    return create_caContext(scg, caName, caDays, serialNumberSize, serialNumber, caBitKeySize);
}
#endif
