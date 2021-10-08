/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Copyright (c) 2021 Kalycito Infotech Private Limited
 */

/* gcc cert_encrypt_tpm.c -o cert_encrypt_tpm -ltpm2_pkcs11 -lssl -lcrypto */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <stdbool.h>
#include <openssl/evp.h>

#include "pkcs11.h"

typedef enum { B_FALSE, B_TRUE } boolean_t;

typedef struct binary_data binary_data;
struct binary_data {
  long length;
  void *data;
};

void usage(void);
void get_MAC_error(void);

void get_MAC_error(void) {
    printf("%s\n", "Error while getting the message digest");
    return;
}

/* If message is binary do not use strlen(message) for message_len. */
static void get_MAC(const uint8_t *message, size_t message_len, unsigned char **message_digest,
             unsigned int *message_digest_len)
{
    EVP_MD_CTX *md_ctx;
    if((md_ctx = EVP_MD_CTX_new()) == NULL)
        get_MAC_error();
    if(1 != EVP_DigestInit_ex(md_ctx, EVP_sha256(), NULL))
        get_MAC_error();
    if(1 != EVP_DigestUpdate(md_ctx, message, message_len))
        get_MAC_error();
    if((*message_digest = (unsigned char *)OPENSSL_malloc((size_t)EVP_MD_size(EVP_sha256()))) == NULL)
        get_MAC_error();
    if(1 != EVP_DigestFinal_ex(md_ctx, *message_digest, message_digest_len))
        get_MAC_error();
    EVP_MD_CTX_free(md_ctx);
}

/* If object is found the object_handle is set */
static boolean_t
find_object_by_label(CK_SESSION_HANDLE hSession, unsigned char *label, CK_OBJECT_HANDLE *object_handle) {
    CK_RV rv;
    boolean_t rtnval = B_FALSE;
    CK_ULONG foundCount = 0;
    do
    {
        CK_OBJECT_HANDLE hObject = 0;
        rv = C_FindObjects( hSession, &hObject, 1, &foundCount );
        if (rv == CKR_OK) {
            /* This will show the labels and values */
            CK_ATTRIBUTE attrTemplate[] = {
                {CKA_LABEL, NULL_PTR, 0}
            };
            rv = C_GetAttributeValue(hSession, hObject, attrTemplate, 1);
            if (attrTemplate[0].ulValueLen > 0)
                attrTemplate[0].pValue = (char *)malloc(attrTemplate[0].ulValueLen);
            rv = C_GetAttributeValue(hSession, hObject, attrTemplate, 1);
            if (attrTemplate[0].ulValueLen > 0) {
                char * val = (char *)malloc(attrTemplate[0].ulValueLen + 1);
                strncpy(val, (const char*)attrTemplate[0].pValue, attrTemplate[0].ulValueLen);
                val[attrTemplate[0].ulValueLen] = '\0';
                printf("Label is <%s>\n", val);
                if (strcasecmp(val, (char *)label) == 0) rtnval = B_TRUE;
                free(val);
                *object_handle = hObject;
            }
            if (attrTemplate[0].pValue)
                free(attrTemplate[0].pValue);
        } else {
            rtnval = B_FALSE;
            printf("Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                   "C_FindObjects failed = 0x%.8lx\n", rv);
        }
    } while( CKR_OK == rv && foundCount > 0 && !rtnval);
    return rtnval;
}

/* The encrypted_data is a binary_data struct that will contain
   the length and contents of the encrypted data */
static CK_RV encrypt(int slotNum, unsigned char *pin, unsigned char *label,
              binary_data *in_data, binary_data **encrypted_data, binary_data * iv_data) {
    static CK_SLOT_ID_PTR pSlotList = NULL_PTR;
    static CK_SLOT_ID slotID;
    CK_SESSION_HANDLE hSession;
    CK_ULONG ulSlotCount = 0;
    unsigned int expected_md_len = 32;
    binary_data *out_data = *encrypted_data;
    uint32_t i;
    CK_RV rv;

    CK_BYTE *data_encrypted;
    CK_ULONG clear_data_length;
    CK_ULONG encrypted_data_length = 0;
    CK_ULONG enclen = 16;
    CK_BYTE iv[iv_data->length];

    CK_OBJECT_HANDLE hObject = 0;
    boolean_t key_object_found;
    CK_OBJECT_CLASS key_class = CKO_SECRET_KEY;
    CK_KEY_TYPE key_type = CKK_AES;
    CK_ATTRIBUTE attrTemplate[] = {
        {CKA_CLASS, &key_class, sizeof(key_class)},
        {CKA_KEY_TYPE, &key_type, sizeof(key_type)},
        {CKA_LABEL, (void *)label, strlen((char*)label)}
    };

    if (iv_data && iv_data->length > 0 && iv_data->data) {
        rv = C_Initialize(NULL_PTR);
        if (rv != CKR_OK) {
            printf("Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                   "C_Initialize failed = 0x%.8lX\n", rv);
            goto cleanup;
        }
    } else {
        printf("The initializtion vector is not valid\n");
        goto cleanup;
    }

    rv = C_GetSlotList(CK_TRUE, NULL_PTR, &ulSlotCount);
    if ((rv == CKR_OK) && (ulSlotCount > 0)) {
        pSlotList = (CK_SLOT_ID_PTR)malloc(ulSlotCount * sizeof (CK_SLOT_ID));
        if (pSlotList == NULL) {
            printf("System error: unable to allocate memory\n");
            goto cleanup;
        }

        rv = C_GetSlotList(CK_TRUE, pSlotList, &ulSlotCount);
        if (rv != CKR_OK) {
            printf("Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                   "GetSlotList failed: Unable to get slot list for processing = 0x%.8lX\n", rv);
            goto cleanup;
        }
    } else {
        printf("Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
               "GetSlotList failed: Unable to get slot count = 0x%.8lX\n", rv);
        goto cleanup;
    }

    for (i = 0; i < ulSlotCount; i++) {
        slotID = pSlotList[i];
        if (slotID == (CK_SLOT_ID)slotNum) {
            CK_TOKEN_INFO token_info;
            rv = C_GetTokenInfo(slotID, &token_info);
            if (rv != CKR_OK) {
                printf("Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                       "C_GetTokenInfo failed = 0x%.8lX\n", rv);
                goto cleanup;
            }
            break;
        }
    }

    rv = C_OpenSession(slotID, CKF_SERIAL_SESSION | CKF_RW_SESSION, (CK_VOID_PTR) NULL, NULL, &hSession);
    if (rv != CKR_OK) {
        printf("Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
               "C_OpenSession failed = 0x%.8lX\n", rv);
        goto cleanup;
    }

    rv = C_Login(hSession, CKU_USER, pin, strlen((const char *)pin));
    if (rv != CKR_OK) {
        printf("Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
               "C_Login failed = 0x%.8lX\n", rv);
        goto cleanup;
    }

    rv = C_FindObjectsInit(hSession, attrTemplate, sizeof(attrTemplate)/sizeof (CK_ATTRIBUTE));
    if (rv == CKR_OK) {
        key_object_found = find_object_by_label(hSession, label, &hObject);
        if (key_object_found == false){
            printf("Error: key object not found\n");
            goto cleanup;
        }

        rv = C_FindObjectsFinal(hSession);
        if (CKR_OK != rv) {
            printf("Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                   "C_FindObjectsFinal failed = 0x%.8lX\n", rv);
            goto cleanup;
        }
    } else {
        printf("Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
               "C_FindObjectsInit failed = 0x%.8lX\n", rv);
        goto cleanup;
    }

    for (i=0; i < iv_data->length; i++) {
        iv[i] = *((CK_BYTE *)((uint32_t*)iv_data->data + i));
    }
    CK_MECHANISM mechanism = {CKM_AES_CBC, iv, sizeof(iv)};

    clear_data_length = (CK_ULONG)in_data->length;
    /* The data to encrypt must be a multiple of 16, required for AES to work */
    if (clear_data_length % 16) {
        clear_data_length += 16 - (clear_data_length % 16);
    }
    /* Add 16 bytes because encrypt final does not accept the data bytes */
    clear_data_length +=16;

    CK_BYTE_PTR ptr_clear_data;
    ptr_clear_data = (CK_BYTE *)(malloc(clear_data_length * sizeof(CK_BYTE)));
    memset(ptr_clear_data, 0, clear_data_length);
    /* Copy the data into the bytes that will be encrypted */
    memcpy(ptr_clear_data, (CK_BYTE *)(in_data->data), (size_t)in_data->length);

    rv = C_EncryptInit(hSession, &mechanism, hObject);
    if (CKR_OK != rv) {
        printf("Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
               "C_EncryptInit failed = 0x%.8lX\n", rv);
        goto cleanup;
    }

    /* For AES expect the encrypted size to be the same size as the clear data */
    encrypted_data_length = clear_data_length;
    data_encrypted = (CK_BYTE *)malloc(encrypted_data_length * sizeof(CK_BYTE));
    memset(data_encrypted, 0, encrypted_data_length);
    /* Keep track of how many parts have been encrypted */
    int part_number = 0;

    while (rv == CKR_OK && part_number * 16 < (int)encrypted_data_length - 16) {
        rv = C_EncryptUpdate(hSession, ptr_clear_data + part_number * 16,
                             16, &data_encrypted[part_number*16], &enclen);
        if (CKR_OK != rv) {
            printf("Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                   "C_Encryptupdate failed = 0x%.8lX\n", rv);
            goto cleanup;
        }
        part_number++;
    }

    C_EncryptFinal(hSession, &data_encrypted[part_number *16 ], &enclen);
    if (CKR_OK != rv) {
        printf("Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
               "C_EncryptFinal failed = 0x%.8lX\n", rv);
        goto cleanup;
    }

    /* Add 56 more bytes. 16 bytes will hold the iv
       The next 8 bytes will be an unsigned long (uint64_t) that indicates the original data length
       The last 32 bytes are for the HMAC */
    long out_data_length = (long)(encrypted_data_length + (long unsigned int)iv_data->length + sizeof(uint64_t) + expected_md_len);
    if (out_data->data) {
        free(out_data->data);
        out_data->length = 0;
    }
    out_data->data = (void *)malloc((size_t)out_data_length);
    out_data->length = out_data_length;
    uint8_t * ptr_out_data = (uint8_t *)(out_data->data);
    memset(ptr_out_data, 255, (size_t)out_data_length);

    /* Copy the encrypted bytes, leaving the last 56 bytes alone */
    memcpy(out_data->data, data_encrypted, encrypted_data_length);

    /* Copy the iv into the bytes after the encrypted data */
    for (i=0; i < iv_data->length; i++) {
         memcpy((uint64_t *)(ptr_out_data + encrypted_data_length +i), &iv[i], 1);
    }

    /* In the next 8 bytes, write the original input data length
       before it was forced to be a multiple of 16
       This is so decrypt can know how much clear data there was */
    *((uint64_t *)(ptr_out_data + out_data_length - sizeof(uint64_t) - expected_md_len)) = (uint64_t)in_data->length;

    /* Calculate the HMAC of the output */
    unsigned char *md_value;
    unsigned int md_len;
    /* Append the HMAC */
    get_MAC(ptr_out_data, (size_t)out_data_length - expected_md_len, &md_value, &md_len);
    for (i=0; i < md_len; i++) {
        memcpy((uint64_t *)(ptr_out_data + out_data_length - expected_md_len + i), &md_value[i], 1);
    }

    if(md_value) OPENSSL_free(md_value);

    C_Logout(hSession);
    C_CloseSession(hSession);
    C_Finalize(NULL);
cleanup:
    if (pSlotList)
        free(pSlotList);
    if (ptr_clear_data)
        free(ptr_clear_data);
    if (data_encrypted)
        free(data_encrypted);
    return rv;
}

static binary_data* read_input_file(const char * filename) {
    binary_data *data = (binary_data*)malloc(sizeof(binary_data));
    if (data != NULL) {
        data->length = 0;
        void *buff = NULL;
        long end_position;
        /* Open the file for reading in binary mode */
        FILE *f_in = fopen(filename, "rb");
        if (f_in != NULL) {
            /* Go to the end of the file */
            const int seek_end_value = fseek(f_in, 0, SEEK_END);
            if (seek_end_value != -1) {

                /* Get the position in the file (in bytes). This is the length. */
                end_position = ftell(f_in);
                if (end_position != -1) {
                    /* Go back to the beginning of the file */
                    const int seek_start_value = fseek(f_in, 0, SEEK_SET);
                    if (seek_start_value != -1) {
                        /* Allocate enough space to read the whole file */
                        buff = (void*)malloc((size_t)end_position);
                        if (buff != NULL) {
                            /* Read the whole file to buffer */
                            const long length = (const long)fread(buff, 1, (size_t)end_position, f_in);
                            if (length == end_position) {
                                data->length = end_position;
                                data->data = buff;

                                fclose(f_in);
                                return data;
                            }
                            free(buff);
                        }
                    }
                }
            }
            fclose(f_in);
        }
        free(data);
    }
  return NULL;
}

static boolean_t write_output_file(const char * filename, binary_data* data) {
    boolean_t rv = B_FALSE;
    if (data && data->length > 0 && data->data) {
        FILE * f_out = fopen(filename, "wb");
        size_t num_to_write = (size_t)data->length;
        if (f_out) {
            size_t num_written = fwrite(data->data, sizeof(uint8_t), num_to_write, f_out);
            if (num_written == (size_t)data->length) {
                rv = B_TRUE;
            } else {
                printf("fwrite() failed: wrote only %zu out of %zu elements.\n",
                       num_written, data->length);
            }
            fclose(f_out);
        }
    }
    return rv;
}

static void process_command_line(int argc, char** argv, unsigned char** slot, unsigned char** pin,
                                 unsigned char** label, unsigned char** in_file, unsigned char** out_file) {
    int i;
    unsigned char *slotptr     = NULL;
    unsigned char *pinptr      = NULL;
    unsigned char *labelptr    = NULL;
    unsigned char *in_fileptr  = NULL;
    unsigned char *out_fileptr = NULL;
    for(i=1;i<argc;i++)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
            case 's' :
                if (*slot) free(*slot);
                slotptr = (unsigned char*)argv[i] + 2;
                *slot = (unsigned char *)malloc(strlen((const char*)slotptr) + 1);
                (*slot)[strlen((const char*)slotptr)] = '\0';
                strcpy((char*)*slot, (const char*)slotptr);
                break;
            case 'p' :
                if (*pin) free(*pin);
                pinptr = (unsigned char*)argv[i] + 2;
                *pin = (unsigned char *)malloc(strlen((const char*)pinptr) + 1);
                (*pin)[strlen((const char*)pinptr)] = '\0';
                strcpy((char*)*pin, (const char*)pinptr);
                break;
            case 'l' :
                if (*label) free(*label);
                labelptr = (unsigned char*)argv[i] + 2;
                *label = (unsigned char *)malloc(strlen((const char*)labelptr) + 1);
                (*label)[strlen((const char*)labelptr)] = '\0';
                strcpy((char*)*label, (const char*)labelptr);
                break;
            case 'f' :
                if (*in_file) free(*in_file);
                in_fileptr = (unsigned char*)argv[i] + 2;
                *in_file = (unsigned char *)malloc(strlen((const char*)in_fileptr) + 1);
                (*in_file)[strlen((const char*)in_fileptr)] = '\0';
                strcpy((char*)*in_file, (const char*)in_fileptr);
                break;
            case 'o' :
                if (*out_file) free(*out_file);
                out_fileptr = (unsigned char*)argv[i] + 2;
                *out_file = (unsigned char *)malloc(strlen((const char*)out_fileptr) + 1);
                (*out_file)[strlen((const char*)out_fileptr)] = '\0';
                strcpy((char*)*out_file, (const char*)out_fileptr);
                break;
            case 'h' :
                break;
            default :
                printf("Unknown option %s\n",argv[i]);
            }
        } else {
            printf("Unknown option %s\n",argv[i]);
        }
    }
}

/*
 * **Usage function**
 * The usage function gives the information to run the application.
 * ./cert_encrypt_tpm -s<slotNo.> -p<pin> -l<key_lable> -f<input_file> -o<output_file>
 * For more options, use ./cert_encrypt_tpm --help
 */
void usage(void)
{
    printf("The slot number must be provided using the -s parameter\n"
           "The pin for the token must be provided using the -p parameter\n"
           "A label for the key object must be provided using the -l parameter\n"
           "A file that contains the data to be encrypted must be provided using the -f parameter\n"
           "A file to write the encrypted data must be provided using the -o parameter\n");
}

int main(int argc, char** argv)
{
    unsigned char *slot = (unsigned char *)malloc(1);
    slot[0] = '\0';
    unsigned char *pin = (unsigned char *)malloc(1);
    pin[0] = '\0';
    unsigned char *label = (unsigned char *)malloc(1);
    label[0] = '\0';
    unsigned char *in_file = (unsigned char *)malloc(1);
    in_file[0] = '\0';
    unsigned char *out_file = (unsigned char *)malloc(1);
    out_file[0] = '\0';
    int slotNum = 0;
    binary_data *in_data = NULL;
    binary_data *out_data = NULL;
    binary_data *iv_data = NULL;
    unsigned char iv[] = {5, 9, 3, 6, 21, 0, 21, 4, 7, 42, 9, 2, 1, 8, 17, 16};
    CK_RV rv = CKR_OK;

    if((argc == 2) && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h")) == 0)
    {
        usage();
        return 0;
    }
    process_command_line(argc, argv, &slot, &pin, &label, &in_file, &out_file);
    printf("\n");
    if (slot[0] != '\0') {
        slotNum = atoi((const char*)slot);
    } else {
        usage();
        return 0;
    }
    if (pin[0] == '\0' || label[0] == '\0' || in_file[0] == '\0' || out_file[0] == '\0') {
        usage();
        return 0;
    }

    /* Put iv array in a struct that has data and length */
    iv_data = (binary_data *)malloc(sizeof(binary_data));
    iv_data->length = sizeof(iv);
    iv_data->data = malloc(sizeof(iv));
    for (unsigned long i=0; i < sizeof(iv); i++) {
        *((unsigned char *)((unsigned long)iv_data->data + i)) = iv[i];
    }

    if (slotNum > 0 && pin[0] != '\0' && label[0] != '\0'
        && in_file[0] != '\0' && out_file[0] != '\0') {
        boolean_t rv_write_file = B_FALSE;
        /* Get the data from the input file. This data will be encrypted */
        in_data = read_input_file((const char*)in_file);
        out_data = (binary_data *)malloc(sizeof(binary_data));
        out_data->length = 0;
        out_data->data = NULL;
        rv = encrypt(slotNum, pin, label, in_data, &out_data, iv_data);
        if (rv != CKR_OK){
            printf("Encryption failed\n");
        }
        if (out_data && out_data->data && out_data->length > 0) {
            /* Write the encrypted data to the output file */
            rv_write_file = write_output_file((const char*)out_file, out_data);
        }
        if (!rv_write_file) {
            printf("%s\n", "Failed to write to output file");
        }
    }

    if (slot) free(slot);
    if (pin) free(pin);
    if (label) free(label);
    if (in_file) free(in_file);
    if (out_file) free(out_file);
    if (in_data) {
        if (in_data->data) free(in_data->data);
        free(in_data);
    }
    if (out_data) {
        if (out_data->data) free(out_data->data);
        free(out_data);
    }
    if (iv_data) {
        if (iv_data->data) free(iv_data->data);
        free(iv_data);
    }
    return 0;
}
