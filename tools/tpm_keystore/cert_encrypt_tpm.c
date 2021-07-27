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

#define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))

CK_RV encrypt(int slotNum, unsigned char *pin, unsigned char *label, binary_data *in_data, binary_data **encrypted_data, binary_data * iv_data);
boolean_t find_object_by_label(CK_SESSION_HANDLE hSession, unsigned char *label, CK_OBJECT_HANDLE *object_handle);
static void process_command_line(int argc, char** argv, unsigned char** slot, unsigned char** pin, unsigned char** label, unsigned char** in_file, unsigned char** out_file);
binary_data* read_input_file(const char * filename);
boolean_t write_output_file(const char * filename, binary_data* data);
void get_MAC(const uint8_t *message, size_t message_len, unsigned char **message_digest, unsigned int *message_digest_len);
void get_MAC_error();

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

    process_command_line(argc, argv, &slot, &pin, &label, &in_file, &out_file);
    printf("\n");
    if (slot[0] != '\0') {
        slotNum = atoi(slot);
    } else {
        printf("The slot number must be provided using the -s parameter.\n");
    }
    if (pin[0] == '\0') {
        printf("The pin for the token must be provided using the -p parameter.\n");
    }
    if (label[0] == '\0') {
        printf("A label for the key object must be provided using the -l parameter.\n");
    }
    if (in_file[0] == '\0') {
        printf("A file that contains the data to be encrypted must be provided using the -f parameter.\n");
    }
    if (out_file[0] == '\0') {
        printf("A file to write the encrypted data must be provided using the -o parameter.\n");
    }

    /* Put iv array in a struct that has data and length */
    iv_data = (binary_data *)malloc(sizeof(binary_data));
    iv_data->length = sizeof(iv);
    iv_data->data = malloc(sizeof(iv));
    for (int i=0; i < sizeof(iv); i++) {
        *((unsigned char *)(iv_data->data + i)) = iv[i];
    }

    if (slotNum > 0 && pin[0] != '\0' && label[0] != '\0'
        && in_file[0] != '\0' && out_file[0] != '\0') {
        boolean_t rv_write_file = false;
        /* Get the data from the input file. This data will be encrypted */
        in_data = read_input_file(in_file);
        out_data = (binary_data *)malloc(sizeof(binary_data));
        out_data->length = 0;
        out_data->data = NULL;
        rv = encrypt(slotNum, pin, label, in_data, &out_data, iv_data);
        if (out_data && out_data->data && out_data->length > 0) {
            /* Write the encrypted data to the output file */
            rv_write_file = write_output_file(out_file, out_data);
        }
        if (!rv_write_file) {
            printf("%s\n", "Failed to write to output file.");
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

/* The encrypted_data is a binary_data struct that will contain
   the length and contents of the encrypted data */
CK_RV encrypt(int slotNum, unsigned char *pin, unsigned char *label,
              binary_data *in_data, binary_data **encrypted_data, binary_data * iv_data) {
    static CK_SLOT_ID_PTR pSlotList = NULL_PTR;
    static CK_SLOT_ID slotID;
    CK_SESSION_HANDLE hSession;
    CK_ULONG ulSlotCount = 0;
    CK_FUNCTION_LIST_PTR pFunctionList;
    unsigned int expected_md_len = 32;
    binary_data *out_data = *encrypted_data;
    uint32_t i;
    CK_RV rv;

    CK_BYTE_PTR ptr_clear_data;
    CK_ULONG clear_data_length;
    CK_BYTE *data_encrypted;
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
        {CKA_LABEL, (void *)label, strlen(label)}
    };

    if (iv_data && iv_data->length > 0 && iv_data->data) {
        rv = C_Initialize(NULL_PTR);
        if (rv != CKR_OK) {
            printf("C_Initialize failed: Error = 0x%.8lX\n", rv);
            goto cleanup;
        }
    } else {
        printf("The initializtion vector is not valid.\n");
        goto cleanup;
    }

    rv = C_GetSlotList(CK_TRUE, NULL_PTR, &ulSlotCount);
    if ((rv == CKR_OK) && (ulSlotCount > 0)) {
        pSlotList = malloc(ulSlotCount * sizeof (CK_SLOT_ID));
        if (pSlotList == NULL) {
            fprintf(stderr, "System error: unable to allocate memory\n");
            goto cleanup;
        }

        rv = C_GetSlotList(CK_TRUE, pSlotList, &ulSlotCount);
        if (rv != CKR_OK) {
            fprintf(stderr, "GetSlotList failed: Error unable to get slot list for processing = 0x%.8lX\n", rv);
            goto cleanup;
        }
    } else {
        fprintf(stderr, "GetSlotList failed: Error unable to get slot count = 0x%.8lX\n", rv);
        goto cleanup;
    }

    for (i = 0; i < ulSlotCount; i++) {
        slotID = pSlotList[i];
        if (slotID == slotNum) {
            CK_TOKEN_INFO token_info;
            rv = C_GetTokenInfo(slotID, &token_info);
            if (rv != CKR_OK) {
                printf("C_GetTokenInfo failed: Error = 0x%.8lX\n", rv);
                goto cleanup;
            }
            break;
        }
    }

    rv = C_OpenSession(slotID, CKF_SERIAL_SESSION | CKF_RW_SESSION, (CK_VOID_PTR) NULL, NULL, &hSession);
    if (rv != CKR_OK) {
        fprintf(stderr, "C_OpenSession failed: Error = 0x%.8lX\n", rv);
        goto cleanup;
    }

    rv = C_Login(hSession, CKU_USER, pin, strlen((const char *)pin));
    if (rv != CKR_OK) {
        fprintf(stderr, "C_Login failed: Error = 0x%.8lX\n", rv);
        goto cleanup;
    }

    rv = C_FindObjectsInit(hSession, attrTemplate, sizeof(attrTemplate)/sizeof (CK_ATTRIBUTE));
    if (rv == CKR_OK) {
        key_object_found = find_object_by_label(hSession, label, &hObject);
        if (key_object_found == false){
            printf("ERROR: key object not found\n");
            goto cleanup;
        }

        rv = C_FindObjectsFinal(hSession);
        if (CKR_OK != rv) {
            fprintf(stderr, "C_FindObjectsFinal failed: Error = 0x%.8lX\n", rv);
            goto cleanup;
        }
    } else {
        fprintf(stderr, "C_FindObjectsInit failed: Error = 0x%.8lX\n", rv);
        goto cleanup;
    }

    for (int i=0; i < iv_data->length; i++) {
        iv[i] = *((CK_BYTE *)(iv_data->data + i));
    }
    CK_MECHANISM mechanism = {CKM_AES_CBC, iv, sizeof(iv)};

    clear_data_length = in_data->length;
    /* The data to encrypt must be a multiple of 16, required for AES to work */
    if (clear_data_length % 16) {
        clear_data_length += 16 - (clear_data_length % 16);
    }
    /* Add 16 bytes because encrypt final does not accept the data bytes */
    clear_data_length +=16;

    ptr_clear_data = (CK_BYTE *)(malloc(clear_data_length * sizeof(CK_BYTE)));
    memset(ptr_clear_data, 0, clear_data_length);
    /* Copy the data into the bytes that will be encrypted */
    memcpy(ptr_clear_data, (CK_BYTE *)(in_data->data), in_data->length);

    rv = C_EncryptInit(hSession, &mechanism, hObject);
    if (CKR_OK != rv) {
        fprintf(stderr, "C_EncryptInit failed: Error = 0x%.8lX\n", rv);
        goto cleanup;
    }

    /* For AES expect the encrypted size to be the same size as the clear data */
    encrypted_data_length = clear_data_length;
    data_encrypted = malloc(encrypted_data_length * sizeof(CK_BYTE));
    memset(data_encrypted, 0, encrypted_data_length);
    /* Keep track of how many parts have been encrypted */
    int part_number = 0;

    while (rv == CKR_OK && part_number * 16 < encrypted_data_length - 16) {
        rv = C_EncryptUpdate(hSession, ptr_clear_data + part_number * 16,
                             16, &data_encrypted[part_number*16], &enclen);
        if (CKR_OK != rv) {
            fprintf(stderr, "C_Encryptupdate failed: Error = 0x%.8lX\n", rv);
            goto cleanup;
        }
        part_number++;
    }

    C_EncryptFinal(hSession, &data_encrypted[part_number *16 ], &enclen);
    if (CKR_OK != rv) {
        fprintf(stderr, "C_EncryptFinal failed: Error = 0x%.8lX\n", rv);
        goto cleanup;
    }

    /* Add 56 more bytes. 16 bytes will hold the iv
       The next 8 bytes will be an unsigned long (uint64_t) that indicates the original data length
       The last 32 bytes are for the HMAC */
    long out_data_length = encrypted_data_length + iv_data->length + sizeof(uint64_t) + expected_md_len;
    if (out_data->data) {
        free(out_data->data);
        out_data->length = 0;
    }
    out_data->data = (void *)malloc(out_data_length);
    out_data->length = out_data_length;
    uint8_t * ptr_out_data = (uint8_t *)(out_data->data);
    memset(ptr_out_data, 255, out_data_length);

    /* Copy the encrypted bytes, leaving the last 56 bytes alone */
    memcpy(out_data->data, data_encrypted, encrypted_data_length);

    /* Copy the iv into the bytes after the encrypted data */
    for (int i=0; i < iv_data->length; i++) {
        memcpy(out_data->data + encrypted_data_length + i, &iv[i], 1);
    }

    /* In the next 8 bytes, write the original input data length
       before it was forced to be a multiple of 16
       This is so decrypt can know how much clear data there was */
    *((uint64_t *)(ptr_out_data + out_data_length - sizeof(uint64_t) - expected_md_len)) = in_data->length;

    /* Calculate the HMAC of the output */
    unsigned char *md_value;
    unsigned int md_len;
    /* Append the HMAC */
    get_MAC(ptr_out_data, out_data_length - expected_md_len, &md_value, &md_len);
    for (int i=0; i < md_len; i++) {
        memcpy(out_data->data + out_data_length - expected_md_len + i, &md_value[i], 1);
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

/* If object is found the object_handle is set */
boolean_t find_object_by_label(CK_SESSION_HANDLE hSession, unsigned char *label, CK_OBJECT_HANDLE *object_handle) {
    CK_RV rv;
    boolean_t rtnval = false;
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
                strncpy(val, attrTemplate[0].pValue, attrTemplate[0].ulValueLen);
                val[attrTemplate[0].ulValueLen] = '\0';
                printf("Label is <%s>\n", val);
                if (strcasecmp(val, (char *)label) == 0) rtnval = true;
                free(val);
                *object_handle = hObject;
            }
            if (attrTemplate[0].pValue)
                free(attrTemplate[0].pValue);
        } else {
            rtnval = false;
            printf("C_FindObjects failed: Error = 0x%.8lx\n", rv);
        }
    } while( CKR_OK == rv && foundCount > 0 && !rtnval);
    return rtnval;
}

/* If message is binary do not use strlen(message) for message_len. */
void get_MAC(const uint8_t *message, size_t message_len, unsigned char **message_digest,
             unsigned int *message_digest_len)
{
    EVP_MD_CTX *md_ctx;
    if((md_ctx = EVP_MD_CTX_new()) == NULL)
        get_MAC_error();
    if(1 != EVP_DigestInit_ex(md_ctx, EVP_sha256(), NULL))
        get_MAC_error();
    if(1 != EVP_DigestUpdate(md_ctx, message, message_len))
        get_MAC_error();
    if((*message_digest = (unsigned char *)OPENSSL_malloc(EVP_MD_size(EVP_sha256()))) == NULL)
        get_MAC_error();
    if(1 != EVP_DigestFinal_ex(md_ctx, *message_digest, message_digest_len))
        get_MAC_error();
    EVP_MD_CTX_free(md_ctx);
}

void get_MAC_error() {
    printf("%s\n", "An error happened while getting the message digest.");
    return;
}

static void process_command_line(int argc, char** argv, unsigned char** slot, unsigned char** pin,
                                 unsigned char** label, unsigned char** in_file, unsigned char** out_file) {
    int i;
    unsigned char *slotptr     = NULL;
    unsigned char *pinptr      = NULL;
    unsigned char *labelptr    = NULL;
    unsigned char *in_fileptr  = NULL;
    unsigned char *out_fileptr = NULL;
    unsigned char *iv_fileptr  = NULL;
    for(i=1;i<argc;i++)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
            case 's' :
                if (*slot) free(*slot);
                slotptr = argv[i] + 2;
                *slot = (unsigned char *)malloc(strlen(slotptr) + 1);
                (*slot)[strlen(slotptr)] = '\0';
                strcpy(*slot,slotptr);
                break;
            case 'p' :
                if (*pin) free(*pin);
                pinptr = argv[i] + 2;
                *pin = (unsigned char *)malloc(strlen(pinptr) + 1);
                (*pin)[strlen(pinptr)] = '\0';
                strcpy(*pin,pinptr);
                break;
            case 'l' :
                if (*label) free(*label);
                labelptr = argv[i] + 2;
                *label = (unsigned char *)malloc(strlen(labelptr) + 1);
                (*label)[strlen(labelptr)] = '\0';
                strcpy(*label,labelptr);
                break;
            case 'f' :
                if (*in_file) free(*in_file);
                in_fileptr = argv[i] + 2;
                *in_file = (unsigned char *)malloc(strlen(in_fileptr) + 1);
                (*in_file)[strlen(in_fileptr)] = '\0';
                strcpy(*in_file,in_fileptr);
                break;
            case 'o' :
                if (*out_file) free(*out_file);
                out_fileptr = argv[i] + 2;
                *out_file = (unsigned char *)malloc(strlen(out_fileptr) + 1);
                (*out_file)[strlen(out_fileptr)] = '\0';
                strcpy(*out_file,out_fileptr);
                break;
            default :
                printf("Unknown option %s\n",argv[i]);
            }
        } else {
            printf("Unknown option %s\n",argv[i]);
        }
    }
}

binary_data* read_input_file(const char * filename) {
    binary_data *data = malloc(sizeof(binary_data));
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
                        buff = malloc(end_position);
                        if (buff != NULL) {
                            /* Read the whole file to buffer */
                            const long length = fread(buff, 1, end_position, f_in);
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

boolean_t write_output_file(const char * filename, binary_data* data) {
    boolean_t rv = false;
    if (data && data->length > 0 && data->data) {
        FILE * f_out = fopen(filename, "wb");
        size_t num_to_write = data->length;
        if (f_out) {
            size_t num_written = fwrite(data->data, sizeof(uint8_t), num_to_write, f_out);
            if (num_written == data->length) {
                rv = true;
            } else {
                printf("fwrite() failed: wrote only %zu out of %zu elements.\n",
                       num_written, data->length);
            }
            fclose(f_out);
        }
    }
    return rv;
}
