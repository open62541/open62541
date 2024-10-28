/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Kai Huebl)
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/util.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/plugin/log_stdout.h>

#include "mp_printf.h"

#ifdef UA_ENABLE_ENCRYPTION

#ifdef __linux__ /* Linux only so far */

#include <stdio.h>
#include <linux/limits.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <bits/stdio_lim.h>
#include <errno.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * ( EVENT_SIZE + 16 ))

struct FileCertStore;
typedef struct FileCertStore FileCertStore;

struct FileCertStore {
    /* Memory cert store as a base */
    UA_CertificateGroup *store;

    int inotifyFd;

    UA_String trustedCertFolder;
    UA_String trustedCrlFolder;
    UA_String issuerCertFolder;
    UA_String issuerCrlFolder;
    UA_String rejectedCertFolder;
    UA_String ownCertFolder;
    UA_String ownKeyFolder;
    UA_String rootFolder;
};

static int
mkpath(char *dir, mode_t mode) {
    struct stat sb;

    if(dir == NULL)
        return 1;

    if(!stat(dir, &sb))
        return 0;  /* Directory already exist */

    size_t len = strlen(dir) + 1;
    char *tmp_dir = (char*)UA_malloc(len);
    if(!tmp_dir)
        return 1;
    memcpy(tmp_dir, dir, len);

    /* Before the actual target directory is created, the recursive call ensures
     * that all parent directories are created or already exist. */
    mkpath(dirname(tmp_dir), mode);
    UA_free(tmp_dir);

    return mkdir(dir, mode);
}

static UA_StatusCode
removeAllFilesFromDir(const char *const path, bool removeSubDirs) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Check parameter */
    if(path == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* remove all regular files from directory */
    DIR *dir = opendir(path);
    if(!dir)
        return UA_STATUSCODE_BADINTERNALERROR;

    struct dirent *dirent;
    while((dirent = readdir(dir)) != NULL) {
        if(dirent->d_type == DT_REG) {
            char file_name[FILENAME_MAX];
            mp_snprintf(file_name, FILENAME_MAX, "%s/%s", path, (char*)dirent->d_name);
            remove(file_name);
        }
        if(dirent->d_type == DT_DIR && removeSubDirs == true) {
            char *directory = (char*)dirent->d_name;

            char dir_name[FILENAME_MAX];
            mp_snprintf(dir_name, FILENAME_MAX, "%s/%s", path, (char*)dirent->d_name);

            if(strlen(directory) == 1 && directory[0] == '.')
                continue;
            if(strlen(directory) == 2 && directory[0] == '.' && directory[1] == '.')
                continue;

            retval = removeAllFilesFromDir(dir_name, removeSubDirs);
        }
    }
    closedir(dir);

    return retval;
}

static UA_StatusCode
getCertFileName(const char *path, const UA_ByteString *certificate,
                char *fileNameBuf, size_t fileNameLen) {
    /* Check parameter */
    if(path == NULL || certificate == NULL || fileNameBuf == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_String thumbprint = UA_STRING_NULL;
    thumbprint.length = 40;
    thumbprint.data = (UA_Byte*)UA_malloc(sizeof(UA_Byte)*thumbprint.length);

    UA_String subjectName = UA_STRING_NULL;

    UA_CertificateUtils_getThumbprint((UA_ByteString*)(uintptr_t)certificate, &thumbprint);
    UA_CertificateUtils_getSubjectName((UA_ByteString*)(uintptr_t)certificate, &subjectName);

    if((thumbprint.length + subjectName.length + 2) > fileNameLen) {
        UA_String_clear(&thumbprint);
        UA_String_clear(&subjectName);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    char *thumbprintBuffer = (char*)UA_malloc(thumbprint.length + 1);
    char *subjectNameBuffer = (char*)UA_malloc(subjectName.length + 1);

    memcpy(thumbprintBuffer, thumbprint.data, thumbprint.length);
    thumbprintBuffer[thumbprint.length] = '\0';
    memcpy(subjectNameBuffer, subjectName.data, subjectName.length);
    subjectNameBuffer[subjectName.length] = '\0';

    char *subName = NULL;
    char *substring = "CN=";
    char *ptr = strstr(subjectNameBuffer, substring);

    if(ptr != NULL) {
        subName = ptr + 3;
    } else {
        subName = subjectNameBuffer;
    }

    if(snprintf(fileNameBuf, fileNameLen, "%s/%s[%s]", path, subName, thumbprintBuffer) < 0)
        retval = UA_STATUSCODE_BADINTERNALERROR;

    UA_String_clear(&thumbprint);
    UA_String_clear(&subjectName);
    UA_free(thumbprintBuffer);
    UA_free(subjectNameBuffer);

    return retval;
}

static UA_StatusCode
readFileToByteString(const char *const path, UA_ByteString *data) {
    if(path == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Open the file */
    FILE *fp = fopen(path, "rb");
    if(!fp)
        return UA_STATUSCODE_BADNOTFOUND;

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

static UA_StatusCode
writeByteStringToFile(const char *const path, const UA_ByteString *data) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Open the file */
    FILE *fp = fopen(path, "wb");
    if(!fp)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Write byte string to file */
    size_t len = fwrite(data->data, sizeof(UA_Byte), data->length * sizeof(UA_Byte), fp);
    if(len != data->length) {
        fclose(fp);
        retval = UA_STATUSCODE_BADINTERNALERROR;
    }

    fclose(fp);
    return retval;
}

static UA_StatusCode
readCertificates(UA_ByteString **list, size_t *listSize, const UA_String path) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    char listPath[PATH_MAX] = {0};
    mp_snprintf(listPath, PATH_MAX, "%.*s",
                (int)path.length, (char*)path.data);

    /* Determine number of certificates */
    size_t numCerts = 0;
    DIR *dir = opendir(listPath);
    if(!dir)
        return UA_STATUSCODE_BADINTERNALERROR;

    struct dirent *dirent;
    while((dirent = readdir(dir)) != NULL) {
        if(dirent->d_type != DT_REG)
            continue;
        numCerts++;
    }

    retval = UA_Array_resize((void **)list, listSize, numCerts, &UA_TYPES[UA_TYPES_BYTESTRING]);
    if(retval != UA_STATUSCODE_GOOD) {
        closedir(dir);
        return retval;
    }

    /* Read files from directory */
    size_t numActCerts = 0;
    rewinddir(dir);

    while((dirent = readdir(dir)) != NULL) {
        if(dirent->d_type != DT_REG)
            continue;
        if(numActCerts < numCerts) {
            /* Create filename to load */
            char filename[PATH_MAX];
            if(mp_snprintf(filename, PATH_MAX, "%s/%s", listPath, dirent->d_name) < 0) {
                closedir(dir);
                return UA_STATUSCODE_BADINTERNALERROR;
            }

            /* Load data from file */
            retval = readFileToByteString(filename, &((*list)[numActCerts]));
            if(retval != UA_STATUSCODE_GOOD) {
                closedir(dir);
                return retval;
            }
        }
        numActCerts++;
    }
    closedir(dir);

    return retval;
}

static UA_StatusCode
readTrustStore(UA_CertificateGroup *certGroup, UA_TrustListDataType *trustList) {
    if(certGroup == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    FileCertStore *context = (FileCertStore *)certGroup->context;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= readCertificates(&trustList->trustedCertificates, &trustList->trustedCertificatesSize,
                               context->trustedCertFolder);
    retval |= readCertificates(&trustList->trustedCrls, &trustList->trustedCrlsSize,
                               context->trustedCrlFolder);
    retval |= readCertificates(&trustList->issuerCertificates, &trustList->issuerCertificatesSize,
                               context->issuerCertFolder);
    retval |= readCertificates(&trustList->issuerCrls, &trustList->issuerCrlsSize,
                               context->issuerCrlFolder);

    return retval;
}

static UA_StatusCode
reloadTrustStore(UA_CertificateGroup *certGroup) {
    if(certGroup == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    FileCertStore *context = (FileCertStore *)certGroup->context;

    char buffer[BUF_LEN];
    const int length = read(context->inotifyFd, buffer, BUF_LEN );
    if(length == -1 && errno != EAGAIN)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* No events, which means no changes to the pki folder */
    /* If the nonblocking read() found no events to read, then
     * it returns -1 with errno set to EAGAIN. In that case,
     * we exit the loop. */
    if(length <= 0)
        return UA_STATUSCODE_GOOD;

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;

    UA_StatusCode retval = readTrustStore(certGroup, &trustList);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_TrustListDataType_clear(&trustList);
        return retval;
    }

    return context->store->setTrustList(context->store, &trustList);
}

static UA_StatusCode
writeCertificates(UA_CertificateGroup *certGroup, const UA_ByteString *list,
                  size_t listSize, const char *listPath) {
    /* Check parameter */
    if(listPath == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(listSize > 0 && list == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < listSize; i++) {
        /* Create filename to load */
        char filename[PATH_MAX];
        retval = getCertFileName(listPath, &list[i], filename, PATH_MAX);
        if(retval != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADINTERNALERROR;

        /* Store data in file */
        retval = writeByteStringToFile(filename, &list[i]);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    return retval;
}

static UA_StatusCode
writeTrustList(UA_CertificateGroup *certGroup, const UA_ByteString *list,
               size_t listSize, const UA_String path) {
    /* Check parameter */
    if(path.length == 0)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(listSize > 0 && list == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    char listPath[PATH_MAX] = {0};
    mp_snprintf(listPath, PATH_MAX, "%.*s", (int)path.length, (char*)path.data);
    /* remove existing files in directory */
    UA_StatusCode retval = removeAllFilesFromDir(listPath, false);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return writeCertificates(certGroup, list, listSize, listPath);
}

static UA_StatusCode
writeTrustStore(UA_CertificateGroup *certGroup, const UA_UInt32 trustListMask) {
    /* Check parameter */
    if(certGroup == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    FileCertStore *context = (FileCertStore *)certGroup->context;

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = trustListMask;

    context->store->getTrustList(context->store, &trustList);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(trustList.specifiedLists & UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES) {
        retval = writeTrustList(certGroup, trustList.trustedCertificates,
                                trustList.trustedCertificatesSize, context->trustedCertFolder);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }
    if(trustList.specifiedLists & UA_TRUSTLISTMASKS_TRUSTEDCRLS) {
        retval = writeTrustList(certGroup, trustList.trustedCrls,
                                trustList.trustedCrlsSize, context->trustedCrlFolder);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }
    if(trustList.specifiedLists & UA_TRUSTLISTMASKS_ISSUERCERTIFICATES) {
        retval = writeTrustList(certGroup, trustList.issuerCertificates,
                                trustList.issuerCertificatesSize, context->issuerCertFolder);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }
    if(trustList.specifiedLists & UA_TRUSTLISTMASKS_ISSUERCRLS) {
        retval = writeTrustList(certGroup, trustList.issuerCrls,
                                trustList.issuerCrlsSize, context->issuerCrlFolder);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }
    UA_TrustListDataType_clear(&trustList);

    return retval;
}

static UA_StatusCode
FileCertStore_setupStorePath(char *directory, char *rootDirectory,
                             size_t rootDirectorySize, UA_String *out) {
    char path[PATH_MAX] = {0};
    size_t pathSize = 0;

    strncpy(path, rootDirectory, PATH_MAX);
    pathSize = strnlen(path, PATH_MAX);

    strncpy(&path[pathSize], directory, PATH_MAX - pathSize);

    *out = UA_STRING_ALLOC(path);

    mkpath(path, 0777);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
FileCertStore_createPkiDirectory(UA_CertificateGroup *certGroup, const UA_String directory) {
    if(certGroup == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    FileCertStore *context = (FileCertStore *)certGroup->context;
    if(!context)
        return UA_STATUSCODE_BADINTERNALERROR;

    char rootDirectory[PATH_MAX] = {0};
    size_t rootDirectorySize = 0;

    if(directory.length <= 0 || directory.length >= PATH_MAX)
        return UA_STATUSCODE_BADINTERNALERROR;

    memcpy(rootDirectory, directory.data, directory.length);
    rootDirectorySize = strnlen(rootDirectory, PATH_MAX);

    /* Add Certificate Group Id */
    UA_NodeId applCertGroup =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId httpCertGroup =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTHTTPSGROUP);
    UA_NodeId userTokenCertGroup =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);

    if(UA_NodeId_equal(&certGroup->certificateGroupId, &applCertGroup)) {
        strncpy(&rootDirectory[rootDirectorySize], "/ApplCerts", PATH_MAX - rootDirectorySize);
    } else if(UA_NodeId_equal(&certGroup->certificateGroupId, &httpCertGroup)) {
        strncpy(&rootDirectory[rootDirectorySize], "/HttpCerts", PATH_MAX - rootDirectorySize);
    } else if(UA_NodeId_equal(&certGroup->certificateGroupId, &userTokenCertGroup)) {
        strncpy(&rootDirectory[rootDirectorySize], "/UserTokenCerts", PATH_MAX - rootDirectorySize);
    } else {
        UA_String nodeIdStr;
        UA_String_init(&nodeIdStr);
        UA_NodeId_print(&certGroup->certificateGroupId, &nodeIdStr);
        strncpy(&rootDirectory[rootDirectorySize], (char*)nodeIdStr.data, PATH_MAX - rootDirectorySize);
        UA_String_clear(&nodeIdStr);
    }
    rootDirectorySize = strnlen(rootDirectory, PATH_MAX);

    context->rootFolder = UA_STRING_ALLOC(rootDirectory);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= FileCertStore_setupStorePath("/trusted/certs", rootDirectory,
                                           rootDirectorySize, &context->trustedCertFolder);
    retval |= FileCertStore_setupStorePath("/trusted/crl", rootDirectory,
                                           rootDirectorySize, &context->trustedCrlFolder);
    retval |= FileCertStore_setupStorePath("/issuer/certs", rootDirectory,
                                           rootDirectorySize, &context->issuerCertFolder);
    retval |= FileCertStore_setupStorePath("/issuer/crl", rootDirectory,
                                           rootDirectorySize, &context->issuerCrlFolder);
    retval |= FileCertStore_setupStorePath("/rejected/certs", rootDirectory,
                                           rootDirectorySize, &context->rejectedCertFolder);
    retval |= FileCertStore_setupStorePath("/own/certs", rootDirectory,
                                           rootDirectorySize, &context->ownCertFolder);
    retval |= FileCertStore_setupStorePath("/own/private", rootDirectory,
                                           rootDirectorySize, &context->ownKeyFolder);

    return retval;
}

static UA_StatusCode
FileCertStore_createInotifyEvent(UA_CertificateGroup *certGroup) {
    if(certGroup == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    FileCertStore *context = (FileCertStore *)certGroup->context;

    context->inotifyFd = inotify_init1(IN_NONBLOCK);
    if(context->inotifyFd == -1)
        return UA_STATUSCODE_BADINTERNALERROR;

    char rootFolder[PATH_MAX] = {0};
    mp_snprintf(rootFolder, PATH_MAX, "%.*s",
                (int)context->rootFolder.length, (char*)context->rootFolder.data);
    int wd = inotify_add_watch(context->inotifyFd, rootFolder, IN_ALL_EVENTS);
    if(wd == -1) {
        close(context->inotifyFd);
        context->inotifyFd = -1;
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
FileCertStore_getTrustList(UA_CertificateGroup *certGroup, UA_TrustListDataType *trustList) {
    /* Check parameter */
    if(certGroup == NULL || trustList == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    FileCertStore *context = (FileCertStore *)certGroup->context;
    /* It will only re-read the Cert store on the file system if there have been changes to files. */
    UA_StatusCode retval = reloadTrustStore(certGroup);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return context->store->getTrustList(context->store, trustList);
}

static UA_StatusCode
FileCertStore_setTrustList(UA_CertificateGroup *certGroup, const UA_TrustListDataType *trustList) {
    /* Check parameter */
    if(certGroup == NULL || trustList == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    FileCertStore *context = (FileCertStore *)certGroup->context;
    /* It will only re-read the Cert store on the file system if there have been changes to files. */
    UA_StatusCode retval = reloadTrustStore(certGroup);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = context->store->setTrustList(context->store, trustList);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return writeTrustStore(certGroup, trustList->specifiedLists);
}

static UA_StatusCode
FileCertStore_addToTrustList(UA_CertificateGroup *certGroup, const UA_TrustListDataType *trustList) {
    /* Check parameter */
    if(certGroup == NULL || trustList == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    FileCertStore *context = (FileCertStore *)certGroup->context;
    /* It will only re-read the Cert store on the file system if there have been changes to files. */
    UA_StatusCode retval = reloadTrustStore(certGroup);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = context->store->addToTrustList(context->store, trustList);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return writeTrustStore(certGroup, trustList->specifiedLists);
}

static UA_StatusCode
FileCertStore_removeFromTrustList(UA_CertificateGroup *certGroup, const UA_TrustListDataType *trustList) {
    /* Check parameter */
    if(certGroup == NULL || trustList == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    FileCertStore *context = (FileCertStore *)certGroup->context;
    /* It will only re-read the Cert store on the file system if there have been changes to files. */
    UA_StatusCode retval = reloadTrustStore(certGroup);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = context->store->removeFromTrustList(context->store, trustList);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return writeTrustStore(certGroup, trustList->specifiedLists);
}

static UA_StatusCode
FileCertStore_getRejectedList(UA_CertificateGroup *certGroup, UA_ByteString **rejectedList, size_t *rejectedListSize) {
    /* Check parameter */
    if(certGroup == NULL || rejectedList == NULL || rejectedListSize == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    FileCertStore *context = (FileCertStore *)certGroup->context;
    return context->store->getRejectedList(context->store, rejectedList, rejectedListSize);
}

static UA_StatusCode
FileCertStore_getCertificateCrls(UA_CertificateGroup *certGroup, const UA_ByteString *certificate,
                                 const UA_Boolean isTrusted, UA_ByteString **crls,
                                 size_t *crlsSize) {
    /* Check parameter */
    if(certGroup == NULL || certificate == NULL || crls == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    FileCertStore *context = (FileCertStore *)certGroup->context;
    /* It will only re-read the Cert store on the file system if there have been changes to files. */
    UA_StatusCode retval = reloadTrustStore(certGroup);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return context->store->getCertificateCrls(context->store, certificate, isTrusted, crls, crlsSize);
}

static UA_StatusCode
FileCertStore_verifyCertificate(UA_CertificateGroup *certGroup, const UA_ByteString *certificate) {
    /* Check parameter */
    if(certGroup == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    FileCertStore *context = (FileCertStore *)certGroup->context;
    /* It will only re-read the Cert store on the file system if there have been changes to files. */
    UA_StatusCode retval = reloadTrustStore(certGroup);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    retval = context->store->verifyCertificate(context->store, certificate);
    if(retval == UA_STATUSCODE_BADCERTIFICATEUNTRUSTED ||
       retval == UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED ||
       retval == UA_STATUSCODE_BADCERTIFICATEREVOCATIONUNKNOWN ||
       retval == UA_STATUSCODE_BADCERTIFICATEISSUERREVOCATIONUNKNOWN) {
        /* write rejectedList to filestore */
        UA_ByteString *rejectedList = NULL;
        size_t rejectedListSize = 0;
        context->store->getRejectedList(context->store, &rejectedList, &rejectedListSize);
        writeTrustList(certGroup, rejectedList, rejectedListSize, context->trustedCertFolder);
        UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    }

    return retval;
}

static void
FileCertStore_clear(UA_CertificateGroup *certGroup) {
    /* check parameter */
    if(!certGroup || !certGroup->context)
        return;

    UA_NodeId_clear(&certGroup->certificateGroupId);

    FileCertStore *context = (FileCertStore *)certGroup->context;

    if(context->store) {
        context->store->clear(context->store);
        UA_free(context->store);
    }
    UA_String_clear(&context->trustedCertFolder);
    UA_String_clear(&context->trustedCrlFolder);
    UA_String_clear(&context->issuerCertFolder);
    UA_String_clear(&context->issuerCrlFolder);
    UA_String_clear(&context->rejectedCertFolder);
    UA_String_clear(&context->ownCertFolder);
    UA_String_clear(&context->ownKeyFolder);
    UA_String_clear(&context->rootFolder);

    if(context->inotifyFd > 0)
        close(context->inotifyFd);

    UA_free(context);
    certGroup->context = NULL;
}

UA_StatusCode
UA_CertificateGroup_Filestore(UA_CertificateGroup *certGroup,
                              UA_NodeId *certificateGroupId,
                              const UA_String storePath,
                              const UA_Logger *logger,
                              const UA_KeyValueMap *params) {
    if(certGroup == NULL || certificateGroupId == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Clear if the plugin is already initialized */
    if(certGroup->clear)
        certGroup->clear(certGroup);

    UA_NodeId_copy(certificateGroupId, &certGroup->certificateGroupId);
    certGroup->logging = logger;

    certGroup->getTrustList = FileCertStore_getTrustList;
    certGroup->setTrustList = FileCertStore_setTrustList;
    certGroup->addToTrustList = FileCertStore_addToTrustList;
    certGroup->removeFromTrustList = FileCertStore_removeFromTrustList;
    certGroup->getRejectedList = FileCertStore_getRejectedList;
    certGroup->getCertificateCrls = FileCertStore_getCertificateCrls;
    certGroup->verifyCertificate = FileCertStore_verifyCertificate;
    certGroup->clear = FileCertStore_clear;

    /* Set PKI Store context data */
    FileCertStore *context = (FileCertStore *)UA_calloc(1, sizeof(FileCertStore));
    if(!context) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }
    certGroup->context = context;

    retval = FileCertStore_createPkiDirectory(certGroup, storePath);
    if(retval != UA_STATUSCODE_GOOD) {
        goto cleanup;
    }

    context->store = (UA_CertificateGroup*)UA_calloc(1, sizeof(UA_CertificateGroup));
    retval = UA_CertificateGroup_Memorystore(context->store, certificateGroupId, NULL, logger, params);
    if(retval != UA_STATUSCODE_GOOD) {
        goto cleanup;
    }

    retval = FileCertStore_createInotifyEvent(certGroup);
    if(retval != UA_STATUSCODE_GOOD) {
        goto cleanup;
    }

    return UA_STATUSCODE_GOOD;

    cleanup:
        certGroup->clear(certGroup);
    return retval;
}

#endif

#endif
