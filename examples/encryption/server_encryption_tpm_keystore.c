/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2021 (c) Kalycito Infotech Private Limited
 *
 */

#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

#include <openssl/evp.h>
#include <pkcs11.h>

#include "common.h"

static void get_MAC(const uint8_t *message, size_t message_len, unsigned char **message_digest,
                    unsigned int *message_digest_len)
{
    EVP_MD_CTX *md_ctx;
    if((md_ctx = EVP_MD_CTX_new()) == NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Error while EVP_MD_CTX_new");
        return;
    }
    if(1 != EVP_DigestInit_ex(md_ctx, EVP_sha256(), NULL)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Error while EVP_DigestInit_ex");
        return;
    }
    if(1 != EVP_DigestUpdate(md_ctx, message, message_len)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Error while EVP_DigestUpdate");
        return;
    }
    if((*message_digest = (unsigned char *)OPENSSL_malloc((size_t)EVP_MD_size(EVP_sha256()))) == NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Error while OPENSSL_malloc");
        return;
    }
    if(1 != EVP_DigestFinal_ex(md_ctx, *message_digest, message_digest_len)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Error while EVP_DigestFinal_ex");
        return;
    }
    EVP_MD_CTX_free(md_ctx);
}

/* If object is found the object_handle is set */
static UA_Boolean
find_object_by_label(CK_SESSION_HANDLE hSession, char *label, unsigned long *object_handle) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    UA_Boolean rtnval = UA_FALSE;
    unsigned long foundCount = 0;
    do
    {
        CK_OBJECT_HANDLE hObject = 0;
        rv = (UA_StatusCode)C_FindObjects(hSession, &hObject, 1, &foundCount );
        if (rv == UA_STATUSCODE_GOOD) {
            /* This will show the labels and values */
            CK_ATTRIBUTE attrTemplate[] = {
                {CKA_LABEL, NULL_PTR, 0}
            };
            rv = (UA_StatusCode)C_GetAttributeValue(hSession, hObject, attrTemplate, 1);
            if (attrTemplate[0].ulValueLen > 0)
                attrTemplate[0].pValue = (char *)UA_malloc(attrTemplate[0].ulValueLen);
            rv = (UA_StatusCode)C_GetAttributeValue(hSession, hObject, attrTemplate, 1);
            if (attrTemplate[0].ulValueLen > 0) {
                char * val = (char *)UA_malloc(attrTemplate[0].ulValueLen + 1);
                strncpy(val, (const char*)attrTemplate[0].pValue, attrTemplate[0].ulValueLen);
                val[attrTemplate[0].ulValueLen] = '\0';
                if (strcasecmp(val, (char *)label) == 0) rtnval = true;
                UA_free(val);
                *object_handle = hObject;
            }
            if (attrTemplate[0].pValue)
                UA_free(attrTemplate[0].pValue);
        } else {
            rtnval = UA_FALSE;
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                         "C_FindObjects failed = 0x%.8lx", (long unsigned int)rv);
        }
    } while( rv == UA_STATUSCODE_GOOD && foundCount > 0 && !rtnval);
    return rtnval;
}

static UA_StatusCode
decrypt_data(unsigned long slotNum, unsigned char *pin, char *label, UA_ByteString *in_data,
                   UA_ByteString **decrypted_data, UA_ByteString * iv_data, uint64_t orginal_data_length) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    UA_ByteString *out_data = *decrypted_data;
    unsigned long *pSlotList = NULL;
    unsigned long slotID = 0;
    unsigned long int ulSlotCount = 0;
    unsigned long hSession;
    CK_SESSION_INFO sessionInfo;
    unsigned long hObject = 0;

    unsigned char *ptr_encrypted_data = NULL;
    unsigned long encrypted_data_length = 0;
    unsigned char *data_clear = NULL;
    unsigned long clear_data_length = 0;
    unsigned long declen = 16;
    unsigned char iv[iv_data->length];

    UA_Boolean key_object_found = UA_FALSE;
    CK_OBJECT_CLASS key_class = CKO_SECRET_KEY;
    CK_KEY_TYPE key_type = CKK_AES;
    CK_ATTRIBUTE attrTemplate[] = {
        {CKA_CLASS, &key_class, sizeof(key_class)},
        {CKA_KEY_TYPE, &key_type, sizeof(key_type)},
        {CKA_LABEL, (void *)label, strlen(label)}
    };

    if (iv_data && iv_data->length > 0 && iv_data->data) {
            rv = (UA_StatusCode)C_Initialize(NULL_PTR);
            if (rv != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                             "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                             "C_Initialize failed: Error = 0x%.8lX", (long unsigned int)rv);
                goto cleanup;
            }
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                     "The initializtion vector is not valid");
        goto cleanup;
    }

    rv = (UA_StatusCode)C_GetSlotList(CK_TRUE, NULL, &ulSlotCount);
    if ((rv == UA_STATUSCODE_GOOD) && (ulSlotCount > 0)) {
        pSlotList = (unsigned long*)UA_malloc(ulSlotCount * sizeof (unsigned long));
        if (pSlotList == NULL) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                         "System error: Unable to allocate memory");
            goto cleanup;
        }

        rv = (UA_StatusCode)C_GetSlotList(CK_TRUE, pSlotList, &ulSlotCount);
        if (rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                         "GetSlotList failed: Error unable to get slot list = 0x%.8lX", (long unsigned int)rv);
            goto cleanup;
        }
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                     "GetSlotList failed: Error unable to get slot count = 0x%.8lX", (long unsigned int)rv);
        goto cleanup;
    }

    for (unsigned long int i = 0; i < ulSlotCount; i++) {
        slotID = pSlotList[i];
        if (slotID == slotNum) {
            CK_TOKEN_INFO token_info;
            rv = (UA_StatusCode)C_GetTokenInfo(slotID, &token_info);
            if (rv != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                             "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                             "C_GetTokenInfo failed = 0x%.8lX", (long unsigned int)rv);
                goto cleanup;
            }
            break;
        }
    }

    rv = (UA_StatusCode)C_OpenSession(slotID, CKF_SERIAL_SESSION | CKF_RW_SESSION, (CK_VOID_PTR) NULL, NULL, &hSession);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                     "C_OpenSession failed = 0x%.8lX", (long unsigned int)rv);
        goto cleanup;
    }

    rv = (UA_StatusCode)C_GetSessionInfo(hSession, &sessionInfo);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                     "C_GetSessionInfo failed = 0x%.8lX", (long unsigned int)rv);
        goto cleanup;
    }

    if(sessionInfo.state != CKS_RW_USER_FUNCTIONS) {
        /* Logs a user into a token */
        rv = (UA_StatusCode)C_Login(hSession, CKU_USER, pin, strlen((const char *)pin));
        if (rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                         "C_Login failed = 0x%.8lX", (long unsigned int)rv);
            goto cleanup;
        }
    }

    rv = (UA_StatusCode)C_FindObjectsInit(hSession, attrTemplate, sizeof(attrTemplate)/sizeof (CK_ATTRIBUTE));
    if (rv == UA_STATUSCODE_GOOD) {
        key_object_found = find_object_by_label(hSession, label, &hObject);
        if (key_object_found == UA_FALSE){
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Error: key object not found");
            goto cleanup;
        }

        rv = (UA_StatusCode)C_FindObjectsFinal(hSession);
        if (rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                         "C_FindObjectsFinal failed = 0x%.8lX", (long unsigned int)rv);
            goto cleanup;
        }
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                     "C_FindObjectsInit failed = 0x%.8lX", (long unsigned int)rv);
        goto cleanup;
    }

    for (size_t i=0; i < iv_data->length; i++) {
        iv[i] = *((CK_BYTE *)(iv_data->data + i));
    }

    CK_MECHANISM mechanism = {CKM_AES_CBC, iv, sizeof(iv)};
    encrypted_data_length = in_data->length;
    clear_data_length = encrypted_data_length;
    ptr_encrypted_data = (CK_BYTE *)(UA_malloc(encrypted_data_length * sizeof(CK_BYTE)));
    memset(ptr_encrypted_data, 0, encrypted_data_length);
    memcpy(ptr_encrypted_data, (CK_BYTE *)(in_data->data), in_data->length);

    rv = (UA_StatusCode)C_DecryptInit(hSession, &mechanism, hObject);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                     "C_DecryptInit failed = 0x%.8lX", (long unsigned int)rv);
        goto cleanup;
    }

    data_clear = (CK_BYTE *)UA_malloc(clear_data_length * sizeof(CK_BYTE));
    memset(data_clear, 0, clear_data_length);
    unsigned long part_number = 0;
    while (rv == UA_STATUSCODE_GOOD && part_number * 16 < clear_data_length - 16) {
        rv = (UA_StatusCode)C_DecryptUpdate(hSession, ptr_encrypted_data + part_number * 16,
                                            16, &data_clear[part_number*16], &declen);
        if (rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                         "C_Decryptupdate failed = 0x%.8lX", (long unsigned int)rv);
            goto cleanup;
        }
        part_number++;
    }

    rv = (UA_StatusCode)C_DecryptFinal(hSession, &data_clear[part_number *16], &declen);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                     "C_DecryptFinal failed = 0x%.8lX", (long unsigned int)rv);
        goto cleanup;
    }

    if (orginal_data_length > clear_data_length)
        orginal_data_length = 0;
    if (out_data->data) {
        UA_free(out_data->data);
        out_data->length = 0;
    }
    out_data->data = (UA_Byte *)UA_malloc(orginal_data_length);
    memcpy(out_data->data, data_clear, orginal_data_length);
    out_data->length = orginal_data_length;

    C_Logout(hSession);
    C_CloseSession(hSession);
    C_Finalize(NULL);
cleanup:
    if (data_clear)
        UA_free(data_clear);
    if (pSlotList)
        UA_free(pSlotList);
    if (ptr_encrypted_data)
        UA_free(ptr_encrypted_data);
    return rv;
}

static UA_StatusCode
decrypt(unsigned long slotNum, unsigned char *pin, char *label,
                      UA_ByteString *in_data, UA_ByteString **decrypted_data) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    /* For decrypt
       Calculate the HMAC of the output without the 32 byte (256 bit) md_value
       Check the calcualted md with the md from the input
       if they match continue to decrypt the input */
    unsigned char *md_value = NULL;
    unsigned int md_len;
    unsigned int expected_md_len = 32;
    UA_ByteString *enc_data = UA_ByteString_new();
    UA_ByteString *iv_data = UA_ByteString_new();

    /* Get the HMAC */
    get_MAC(in_data->data, in_data->length - expected_md_len, &md_value, &md_len);
    if (memcmp(in_data->data + in_data->length - expected_md_len, md_value, md_len) == 0) {
        /* Get the iv that was appended to the encrypted data
           find the iv. It was packed in the first 16 bytes of the last 56 in the file */
        UA_ByteString_allocBuffer(iv_data, 16);
        uint8_t * ptr_in_data = (uint8_t *)(in_data->data);
        memcpy(iv_data->data, ptr_in_data + in_data->length - sizeof(uint64_t) - expected_md_len - iv_data->length, iv_data->length);

        /* Find the data length in output */
        uint64_t clear_data_length;
        clear_data_length = *((uint64_t *)(ptr_in_data + in_data->length - sizeof(uint64_t) - expected_md_len));

        /* Remove the extra 56 bytes that were added at the end of the encrypted data */
        UA_ByteString_allocBuffer(enc_data, in_data->length - sizeof(uint64_t) - expected_md_len - iv_data->length);
        memcpy(enc_data->data, ptr_in_data, enc_data->length);

        rv = (UA_StatusCode)decrypt_data(slotNum, pin, label, enc_data, decrypted_data, iv_data, clear_data_length);
        if(rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "decrypt_data failed");
        }
        UA_ByteString_delete(iv_data);
        UA_ByteString_delete(enc_data);
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "HMAC does not match");
        return EXIT_FAILURE;
    }
    if(md_value) OPENSSL_free(md_value);
    return rv;
}

UA_Boolean running = true;
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    if(argc < 6) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Missing arguments. Arguments are "
                     "<server-certificate.der> <private-key.der> "
                     "<slotId> <userPin> <keyLable> "
                     "[<trustlist1.crl>, ...]");
        return EXIT_FAILURE;
    }

    char *keyLabel = NULL;
    unsigned long slotId = 0;
    unsigned char *userpin = NULL;
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString *certificate_out_data = NULL;
    UA_ByteString *encrypt_out_data = NULL;

    /* Load certificate and private key */
    UA_ByteString certificate_in_date = loadFile(argv[1]);
    UA_ByteString encrypt_in_data  = loadFile(argv[2]);
    slotId = (unsigned long)atoi(argv[3]);
    userpin = (unsigned char*)argv[4];
    keyLabel = argv[5];

    encrypt_out_data = (UA_ByteString *)UA_malloc(sizeof(UA_ByteString));
    encrypt_out_data->data = NULL;
    encrypt_out_data->length = 0;

    UA_StatusCode rv = decrypt(slotId, userpin, keyLabel, &encrypt_in_data, &encrypt_out_data);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Decrypt failed for RSA privare key");
        return EXIT_FAILURE;
    }
    privateKey.data = encrypt_out_data->data;
    privateKey.length = encrypt_out_data->length;

    certificate_out_data = (UA_ByteString *)UA_malloc(sizeof(UA_ByteString));
    certificate_out_data->data = NULL;
    certificate_out_data->length = 0;

    rv = decrypt(slotId, userpin, keyLabel, &certificate_in_date, &certificate_out_data);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Decrypt failed for certificate");
        return EXIT_FAILURE;
    }
    certificate.data = certificate_out_data->data;
    certificate.length = certificate_out_data->length;

    /* Load the trustlist */
    size_t trustListSize = 0;
    if(argc > 6)
        trustListSize = (size_t)argc-6;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize+1);
    for(size_t i = 0; i < trustListSize; i++)
        trustList[i] = loadFile(argv[i+6]);

    /* Loading of an issuer list, not used in this application */
    size_t issuerListSize = 0;
    UA_ByteString *issuerList = NULL;

    /* Revocation lists are supported, but not used for the example here */
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_StatusCode retval =
        UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840,
                                                       &certificate, &privateKey,
                                                       trustList, trustListSize,
                                                       issuerList, issuerListSize,
                                                       revocationList, revocationListSize);

    if (encrypt_out_data) {
        if (encrypt_out_data->data)
            UA_free(encrypt_out_data->data);
        UA_free(encrypt_out_data);
    }

    UA_ByteString_clear(&certificate_in_date);
    UA_ByteString_clear(&encrypt_in_data);
    for(size_t i = 0; i < trustListSize; i++)
        UA_ByteString_clear(&trustList[i]);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    retval = UA_Server_run(server, &running);

 cleanup:
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
