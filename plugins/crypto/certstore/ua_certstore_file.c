/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Mark Giraud, Fraunhofer IOSB
 */

#include <open62541/plugin/certstore.h>
#include <open62541/plugin/certstore_default.h>
#include <dirent.h>
#include <open62541/types_generated_handling.h>
#include <sys/stat.h>
#include <libgen.h>

// TODO: Move to util?
static UA_StatusCode
readFileToByteString(const char *const path, UA_ByteString *data) {
    /* Open the file */
    FILE *fp = fopen(path, "rb");
    if(!fp) {
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Get the file length, allocate the data and read */
    fseek(fp, 0, SEEK_END);
    UA_StatusCode retval = UA_ByteString_allocBuffer(data, (size_t)ftell(fp));
    if(retval == UA_STATUSCODE_GOOD) {
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(data->data, sizeof(UA_Byte), data->length * sizeof(UA_Byte), fp);
        if(read != data->length) {
            UA_ByteString_clear(data);
        }
    } else {
        data->length = 0;
    }
    fclose(fp);

    return UA_STATUSCODE_GOOD;
}

static int
mkpath(char *dir, mode_t mode) {
    struct stat sb;
    if(!dir) {
        errno = EINVAL;
        return 1;
    }
    if(!stat(dir, &sb))
        return 0;
    mkpath(dirname(strdupa(dir)), mode);
    return mkdir(dir, mode);
}

typedef struct FilePKIStore {
    char *trustedCertDir;
    size_t trustedCertDirLen;
    char *trustedCrlDir;
    size_t trustedCrlDirLen;
    char *trustedIssuerCertDir;
    size_t trustedIssuerCertDirLen;
    char *trustedIssuerCrlDir;
    size_t trustedIssuerCrlDirLen;
    char *certificateDir;
    size_t certificateDirLen;
    char *keyDir;
    size_t keyDirLen;
} FilePKIStore;

static UA_StatusCode
loadList(UA_ByteString **list, size_t *listSize, const char *listPath) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    size_t numCerts = 0;

    DIR *dir = opendir(listPath);
    if(dir) {
        struct dirent *dirent;
        while((dirent = readdir(dir)) != NULL) {
            if(dirent->d_type == DT_REG) {
                // TODO: Load cert file
            }
        }
        closedir(dir);
    }
    retval = UA_Array_resize((void **)list, listSize, numCerts, &UA_TYPES[UA_TYPES_BYTESTRING]);
    return retval;
}

static UA_StatusCode
loadTrustList_file(UA_PKIStore *certStore, UA_TrustListDataType *trustList) {
    FilePKIStore *context = (FilePKIStore *)certStore->context;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(trustList->specifiedLists & UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES) {
        retval = loadList(&trustList->trustedCertificates, &trustList->trustedCertificatesSize,
                          context->trustedCertDir);
        if(retval != UA_STATUSCODE_GOOD) {
            return retval;
        }
    }
    if(trustList->specifiedLists & UA_TRUSTLISTMASKS_TRUSTEDCRLS) {
        retval = loadList(&trustList->trustedCrls, &trustList->trustedCrlsSize,
                          context->trustedCrlDir);
        if(retval != UA_STATUSCODE_GOOD) {
            return retval;
        }
    }
    if(trustList->specifiedLists & UA_TRUSTLISTMASKS_ISSUERCERTIFICATES) {
        retval = loadList(&trustList->issuerCertificates, &trustList->issuerCertificatesSize,
                          context->trustedIssuerCertDir);
        if(retval != UA_STATUSCODE_GOOD) {
            return retval;
        }
    }
    if(trustList->specifiedLists & UA_TRUSTLISTMASKS_ISSUERCRLS) {
        retval = loadList(&trustList->issuerCrls, &trustList->issuerCrlsSize,
                          context->trustedIssuerCrlDir);
        if(retval != UA_STATUSCODE_GOOD) {
            return retval;
        }
    }
    return retval;
}

static UA_StatusCode
clear_file(UA_PKIStore *certStore) {
    FilePKIStore *context = (FilePKIStore *)certStore->context;
    if(context) {
        if(context->trustedCertDir)
            UA_free(context->trustedCertDir);
        if(context->trustedCrlDir)
            UA_free(context->trustedCrlDir);
        if(context->trustedIssuerCertDir)
            UA_free(context->trustedIssuerCertDir);
        if(context->trustedIssuerCrlDir)
            UA_free(context->trustedIssuerCrlDir);
        if(context->certificateDir)
            UA_free(context->certificateDir);
        if(context->keyDir)
            UA_free(context->keyDir);
        UA_free(context);
    }

    UA_NodeId_clear(&certStore->certificateGroupId);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
storeTrustList_file(UA_PKIStore *certStore, const UA_TrustListDataType *trustList) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
loadCertificate_file(UA_PKIStore *pkiStore, const UA_NodeId certType, UA_ByteString *cert) {
    if(pkiStore == NULL || cert == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_ByteString_clear(cert);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_String nodeIdStr;
    UA_String_init(&nodeIdStr);
    UA_NodeId_print(&certType, &nodeIdStr);

    FilePKIStore *context = (FilePKIStore *)pkiStore->context;
    char filename[FILENAME_MAX];
    if(snprintf(filename, FILENAME_MAX, "%s/%s", context->certificateDir, nodeIdStr.data) < 0) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    retval = readFileToByteString(filename, cert);

cleanup:

    UA_String_clear(&nodeIdStr);
    return retval;
}

static UA_StatusCode
setupPkiDir(char *directory, char *cwd, size_t cwdLen, char **out) {
    strncpy(&cwd[cwdLen], directory, PATH_MAX - cwdLen);
    *out = strndup(cwd, PATH_MAX - cwdLen);
    if(*out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    mkpath(*out, 0777);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_PKIStore_File(UA_PKIStore *pkiStore, UA_NodeId *certificateGroupId) {
    if(pkiStore == NULL || certificateGroupId == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    memset(pkiStore, 0, sizeof(UA_PKIStore));
    char cwd[PATH_MAX];
    if(getcwd(cwd, PATH_MAX) == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    size_t cwdLen = strnlen(cwd, PATH_MAX);

    FilePKIStore *context = (FilePKIStore *)UA_malloc(sizeof(FilePKIStore));
    pkiStore->loadTrustList = loadTrustList_file;
    pkiStore->storeTrustList = storeTrustList_file;
    pkiStore->loadCertificate = loadCertificate_file;
    pkiStore->clear = clear_file;
    pkiStore->context = context;

    strncpy(&cwd[cwdLen], "/pki/", PATH_MAX - cwdLen);
    cwdLen = strnlen(cwd, PATH_MAX);

    UA_String nodeIdStr;
    UA_String_init(&nodeIdStr);
    UA_NodeId_print(certificateGroupId, &nodeIdStr);
    strncpy(&cwd[cwdLen], (char *)nodeIdStr.data, PATH_MAX - cwdLen);
    cwdLen = strnlen(cwd, PATH_MAX);
    UA_String_clear(&nodeIdStr);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= setupPkiDir("/trusted/certs", cwd, cwdLen, &context->trustedCertDir);
    retval |= setupPkiDir("/trusted/crls", cwd, cwdLen, &context->trustedCrlDir);
    retval |= setupPkiDir("/issuer/certs", cwd, cwdLen, &context->trustedIssuerCertDir);
    retval |= setupPkiDir("/issuer/crls", cwd, cwdLen, &context->trustedIssuerCrlDir);
    retval |= setupPkiDir("/own/certs", cwd, cwdLen, &context->certificateDir);
    retval |= setupPkiDir("/own/keys", cwd, cwdLen, &context->keyDir);
    if(retval != UA_STATUSCODE_GOOD) {
        goto error;
    }

    UA_NodeId_copy(certificateGroupId, &pkiStore->certificateGroupId);

    return UA_STATUSCODE_GOOD;

error:
    pkiStore->clear(pkiStore);
    return UA_STATUSCODE_BADINTERNALERROR;
}
