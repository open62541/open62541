/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Mark Giraud, Fraunhofer IOSB
 */

#include <open62541/nodeids.h>
#include <open62541/util.h>
#include <open62541/plugin/certstore.h>
#include <open62541/plugin/certstore_default.h>
#include <dirent.h>
#include <open62541/types_generated_handling.h>
#include <sys/stat.h>
#include <libgen.h>

static UA_StatusCode
readFileToByteString(const char *const path, UA_ByteString *data) {
	if (path == NULL || data == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}

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

static UA_StatusCode
writeByteStringToFile(const char *const path, const UA_ByteString *data) {
	UA_StatusCode retval = UA_STATUSCODE_GOOD;

	/* Open the file */
    FILE *fp = fopen(path, "wb");
    if(!fp) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

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
removeAllFilesFromDir(const char *const path, bool removeSubDirs) {
	UA_StatusCode retval = UA_STATUSCODE_GOOD;

	/* Check parameter */
	if (path == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}

	/* remove all regular files from directory */
	DIR *dir = opendir(path);
	if(dir) {
        struct dirent *dirent;
	    while((dirent = readdir(dir)) != NULL) {
	        if(dirent->d_type == DT_REG) {
	        	char file_name[FILENAME_MAX];
	            snprintf(file_name, FILENAME_MAX, "%s/%s", path, (char*)dirent->d_name);
	            remove(file_name);
	        }
	        else if (dirent->d_type == DT_DIR && removeSubDirs == true) {
	        	char* directory = (char*)dirent->d_name;

	        	char dir_name[FILENAME_MAX];
	            snprintf(dir_name, FILENAME_MAX, "%s/%s", path, (char*)dirent->d_name);

	        	if (strlen(directory) == 1 && directory[0] == '.') continue;
	        	if (strlen(directory) == 2 && directory[0] == '.' && directory[1] == '.') continue;

	            removeAllFilesFromDir(dir_name, removeSubDirs);
	            /*rmdir(dir_name);*/
	        }
	    }
	    closedir(dir);
	}
	return retval;
}

static char* copyStr(const char* s)
{
  size_t len = 1+strlen(s);
  char* p = (char*)malloc(len);
  p[len-1] = 0x00;
  return p ? (char*)memcpy(p, s, len) : NULL;
}

static int
mkpath(char *dir, mode_t mode) {
    struct stat sb;

    if(dir == NULL) {
        errno = EINVAL;
        return 1;
    }

    if(!stat(dir, &sb)) {
    	/* Directory already exist */
    	return 0;
    }

    char* tmp_dir = copyStr(dir);
    mkpath(dirname(tmp_dir), mode);
    free(tmp_dir);

    return mkdir(dir, mode);
}

static UA_StatusCode
setupPkiDir(char *directory, char *cwd, size_t cwdLen, char **out) {
	char path[PATH_MAX];
	size_t pathLen = 0;

	strncpy(path, cwd, PATH_MAX);
	pathLen = strnlen(path, PATH_MAX);

    strncpy(&path[pathLen], directory, PATH_MAX - pathLen);
    pathLen = strnlen(path, PATH_MAX);

    *out = strndup(path, pathLen+1);
    if(*out == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    mkpath(*out, 0777);
    return UA_STATUSCODE_GOOD;
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
    char *rejectedCertDir;
    size_t rejectedCertDirLen;
    char *keyDir;
    size_t keyDirLen;
    char *rootDir;
    size_t rootDirLen;
    UA_StatusCode (*makeCertThumbprint)(
    	const UA_ByteString* certificate,
    	UA_ByteString* thumbprint
    );
} FilePKIStore;

static UA_StatusCode
getCertFileName(
	UA_PKIStore *certStore,
	const char* path,
	const UA_ByteString* certificate,
	char* fileNameBuf,
	size_t fileNameLen
) {
	/* Check parameter */
	if (certStore == NULL || path == NULL || certificate == NULL || fileNameBuf == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}
	FilePKIStore *context = (FilePKIStore *)certStore->context;
	UA_StatusCode retval = UA_STATUSCODE_GOOD;

	char buf[20];
	UA_ByteString thumbprint = {20, (UA_Byte*)buf};
	if (context->makeCertThumbprint != NULL) {
		/* Create certificate Thumbprint */
		retval = context->makeCertThumbprint(certificate, &thumbprint);
		if (retval != UA_STATUSCODE_GOOD) {
			return retval;
		}
	}
	else {
		/* Create random buffer */
		size_t idx = 0;
		for (idx = 0; idx < 5; idx++) {
			UA_UInt32 number = UA_UInt32_random();
			memcpy(&thumbprint.data[idx*4], (char*)&number, 4);
		}
	}

	/* Convert bytes to hex string */
	size_t idx;
	char thumbprintBuf[41];
	memset(thumbprintBuf, 0x00, 41);
	for (idx = 0; idx < 20; idx++) {
		snprintf(&thumbprintBuf[idx*2], ((20-idx)*2), "%02X", thumbprint.data[idx] & 0xFF);
	}

	/* Create filename */
	if(snprintf(fileNameBuf, fileNameLen, "%s/%s", path, thumbprintBuf) < 0) {
	    return UA_STATUSCODE_BADINTERNALERROR;
	}

	return retval;
}

static UA_StatusCode
loadList(UA_ByteString **list, size_t *listSize, const char *listPath) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Determine number of certificates */
    size_t numCerts = 0;
    DIR *dir = opendir(listPath);
    if(dir) {
        struct dirent *dirent;
        while((dirent = readdir(dir)) != NULL) {
            if(dirent->d_type == DT_REG) {
            	numCerts++;
            }
        }
        closedir(dir);
    }

    retval = UA_Array_resize((void **)list, listSize, numCerts, &UA_TYPES[UA_TYPES_BYTESTRING]);
    if (retval != UA_STATUSCODE_GOOD) {
    	return retval;
    }

    /* Read files from directory */
    size_t numActCerts = 0;
    dir = opendir(listPath);
    if(dir) {
        struct dirent *dirent;
        while((dirent = readdir(dir)) != NULL) {
            if(dirent->d_type == DT_REG) {

            	if (numActCerts < numCerts) {
            		/* Create filename to load */
            		char filename[FILENAME_MAX];
            		if(snprintf(filename, FILENAME_MAX, "%s/%s", listPath, dirent->d_name) < 0) {
            			closedir(dir);
            			return UA_STATUSCODE_BADINTERNALERROR;
            		}

            		/* Load data from file */
            		retval = readFileToByteString(filename, &((*list)[numActCerts]));
            		if (retval != UA_STATUSCODE_GOOD) {
            			closedir(dir);
            			return retval;
            		}
            	}

                numActCerts++;
            }
        }
        closedir(dir);
    }

    return retval;
}

static UA_StatusCode
storeList(UA_PKIStore *certStore, const UA_ByteString *list, size_t listSize, const char *listPath) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Check parameter */
    if (listPath == NULL) {
    	return UA_STATUSCODE_BADINTERNALERROR;
    }
    if (listSize > 0 && list == NULL) {
    	return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* remove existing files in directory */
    retval = removeAllFilesFromDir(listPath, false);
    if (retval != UA_STATUSCODE_GOOD) {
    	return retval;
    }

    /* Store new byte strings */
    size_t idx = 0;
    for (idx = 0; idx < listSize; idx++) {
       	/* Create filename to load */
        char filename[FILENAME_MAX];
        retval = getCertFileName(certStore, listPath, &list[idx], filename, FILENAME_MAX);
        if(retval != UA_STATUSCODE_GOOD) {
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        /* Store data in file */
        retval = writeByteStringToFile(filename, &list[idx]);
        if (retval != UA_STATUSCODE_GOOD) {
        	return retval;
        }
    }

    return retval;
}

static UA_StatusCode
loadTrustList_file(UA_PKIStore *certStore, UA_TrustListDataType *trustList) {
    /* Check parameter */
	if (certStore == NULL || trustList == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}

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
storeTrustList_file(UA_PKIStore *certStore, const UA_TrustListDataType *trustList) {
	/* Check parameter */
	if (certStore == NULL || trustList == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}

    FilePKIStore *context = (FilePKIStore *)certStore->context;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    if(trustList->specifiedLists & UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES) {
        retval = storeList(certStore, trustList->trustedCertificates, trustList->trustedCertificatesSize,
                          context->trustedCertDir);
        if(retval != UA_STATUSCODE_GOOD) {
            return retval;
        }
    }
    if(trustList->specifiedLists & UA_TRUSTLISTMASKS_TRUSTEDCRLS) {
        retval = storeList(certStore, trustList->trustedCrls, trustList->trustedCrlsSize,
                          context->trustedCrlDir);
        if(retval != UA_STATUSCODE_GOOD) {
            return retval;
        }
    }
    if(trustList->specifiedLists & UA_TRUSTLISTMASKS_ISSUERCERTIFICATES) {
        retval = storeList(certStore, trustList->issuerCertificates, trustList->issuerCertificatesSize,
                          context->trustedIssuerCertDir);
        if(retval != UA_STATUSCODE_GOOD) {
            return retval;
        }
    }
    if(trustList->specifiedLists & UA_TRUSTLISTMASKS_ISSUERCRLS) {
        retval = storeList(certStore, trustList->issuerCrls, trustList->issuerCrlsSize,
                          context->trustedIssuerCrlDir);
        if(retval != UA_STATUSCODE_GOOD) {
            return retval;
        }
    }
    return retval;
}

static UA_StatusCode
loadRejectedList(UA_PKIStore *certStore, UA_ByteString **rejectedList, size_t *rejectedListSize)
{
    /* Check parameter */
	if (certStore == NULL || rejectedList == NULL || rejectedListSize == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}

    FilePKIStore *context = (FilePKIStore *)certStore->context;

    return loadList(rejectedList, rejectedListSize, context->rejectedCertDir);
}

static UA_StatusCode
storeRejectedList(UA_PKIStore *certStore, const UA_ByteString *rejectedList, size_t rejectedListSize)
{
    /* Check parameter */
	if (certStore == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}

	FilePKIStore *context = (FilePKIStore *)certStore->context;

	return storeList(certStore, rejectedList, rejectedListSize, context->rejectedCertDir);
}

static UA_StatusCode
appendRejectedList(UA_PKIStore *certStore, const UA_ByteString *certificate)
{
	UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Check parameter */
	if (certStore == NULL || certificate == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}

	FilePKIStore *context = (FilePKIStore *)certStore->context;

	/* check duplicate certificate */
	UA_ByteString* rejectedList = NULL;
	size_t rejectedListLen = 0;
	retval = loadList(&rejectedList, &rejectedListLen, context->rejectedCertDir);
	if (retval != UA_STATUSCODE_GOOD) {
		return retval;
	}

	size_t idx = 0;
	for (idx = 0; idx < rejectedListLen; idx++) {
		if (UA_ByteString_equal(&rejectedList[idx], certificate)) {
			UA_Array_delete(rejectedList, rejectedListLen, &UA_TYPES[UA_TYPES_BYTESTRING]);
			return UA_STATUSCODE_GOOD; /* certificate already exist */
		}
	}
	UA_Array_delete(rejectedList, rejectedListLen, &UA_TYPES[UA_TYPES_BYTESTRING]);

  	/* Create filename to store */
	char filename[FILENAME_MAX];
	retval = getCertFileName(certStore, context->rejectedCertDir, certificate, filename, FILENAME_MAX);
	if(retval != UA_STATUSCODE_GOOD) {
	    return UA_STATUSCODE_BADINTERNALERROR;
	}

    /* Store data in file */
    return writeByteStringToFile(filename, certificate);
}

static UA_StatusCode
getCertTypeFileName(
	const char* directory,
	const UA_NodeId certType,
	char* filenameBuf,
	size_t filenameLen
) {
    UA_NodeId applCertType = UA_NODEID_NUMERIC(0, UA_NS0ID_APPLICATIONCERTIFICATETYPE);
    UA_NodeId userCredentialCertType = UA_NODEID_NUMERIC(0, UA_NS0ID_USERCREDENTIALCERTIFICATETYPE);
    UA_NodeId httpsCertType = UA_NODEID_NUMERIC(0, UA_NS0ID_HTTPSCERTIFICATETYPE);
    UA_NodeId RSAMinCertType = UA_NODEID_NUMERIC(0, UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE);
    UA_NodeId RSASHA256CertType = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);

	/* Set filename */
	if (UA_NodeId_equal(&certType, &applCertType)) {
	    if (!snprintf(filenameBuf, filenameLen, "%s/ApplCert", directory)) {
	    	return UA_STATUSCODE_BADINTERNALERROR;
	    }
	}
	else if (UA_NodeId_equal(&certType, &userCredentialCertType)) {
	    if (!snprintf(filenameBuf, filenameLen, "%s/UserCredentialCert", directory)) {
	    	return UA_STATUSCODE_BADINTERNALERROR;
	    }
	}
	else if (UA_NodeId_equal(&certType, &httpsCertType)) {
	    if (!snprintf(filenameBuf, filenameLen, "%s/HttpCert", directory)) {
	    	return UA_STATUSCODE_BADINTERNALERROR;
	    }
	}
	else if (UA_NodeId_equal(&certType, &RSAMinCertType)) {
	    if (!snprintf(filenameBuf, filenameLen, "%s/RSAMinCert", directory)) {
	    	return UA_STATUSCODE_BADINTERNALERROR;
	    }
	}
	else if (UA_NodeId_equal(&certType, &RSASHA256CertType)) {
	    if (!snprintf(filenameBuf, filenameLen, "%s/RSASHA256Cert", directory)) {
	    	return UA_STATUSCODE_BADINTERNALERROR;
	    }
	}
	else {
	    UA_String nodeIdStr;
	    UA_String_init(&nodeIdStr);
	    UA_NodeId_print(&certType, &nodeIdStr);
	    if (!snprintf(filenameBuf, filenameLen, "%s/%.*s", directory, (int)nodeIdStr.length, nodeIdStr.data)) {
	    	UA_String_clear(&nodeIdStr);
	    	return UA_STATUSCODE_BADINTERNALERROR;
	    }
	    UA_String_clear(&nodeIdStr);
	}

	return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
loadCertificate_file(UA_PKIStore *pkiStore, const UA_NodeId certType, UA_ByteString *cert) {
	/* Check parameter */
    if(pkiStore == NULL || cert == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    FilePKIStore *context = (FilePKIStore *)pkiStore->context;
	if (context->certificateDir == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}
    UA_ByteString_clear(cert);

    /* Get certificate filename */
    char filename[FILENAME_MAX];
    UA_StatusCode retval = getCertTypeFileName(context->certificateDir, certType, filename, FILENAME_MAX);
    if (retval != UA_STATUSCODE_GOOD) {
    	return retval;
    }

    /* Read certificate from file */
    retval = readFileToByteString(filename, cert);
    return retval;
}

static UA_StatusCode
storeCertificate_file(UA_PKIStore *pkiStore, const UA_NodeId certType, const UA_ByteString *cert)
{
	/* Check parameter */
    if(pkiStore == NULL || cert == NULL || cert->length == 0) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    FilePKIStore *context = (FilePKIStore *)pkiStore->context;
	if (context->certificateDir == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}

    /* Get certificate filename */
    char filename[FILENAME_MAX];
    UA_StatusCode retval = getCertTypeFileName(context->certificateDir, certType, filename, FILENAME_MAX);
    if (retval != UA_STATUSCODE_GOOD) {
    	return retval;
    }

    /* Write certificate to file */
    return writeByteStringToFile(filename, cert);
}

static UA_StatusCode
loadPrivateKey_file(UA_PKIStore *pkiStore, const UA_NodeId certType, UA_ByteString *privateKey)
{
	/* Check parameter */
    if(pkiStore == NULL || privateKey == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    FilePKIStore *context = (FilePKIStore *)pkiStore->context;
	if (context->keyDir == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}
    UA_ByteString_clear(privateKey);

    /* Get private key filename */
    char filename[FILENAME_MAX];
    UA_StatusCode retval = getCertTypeFileName(context->keyDir, certType, filename, FILENAME_MAX);
    if (retval != UA_STATUSCODE_GOOD) {
    	return retval;
    }

    /* Read public key from file */
    return readFileToByteString(filename, privateKey);
}


static UA_StatusCode
storePrivateKey_file(UA_PKIStore *pkiStore, const UA_NodeId certType, const UA_ByteString *privateKey)
{
	/* Check parameter */
	if(pkiStore == NULL || privateKey == NULL || privateKey->length == 0) {
	    return UA_STATUSCODE_BADINTERNALERROR;
	}
	FilePKIStore *context = (FilePKIStore *)pkiStore->context;
	if (context->keyDir == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}

    /* Get private key filename */
    char filename[FILENAME_MAX];
    UA_StatusCode retval = getCertTypeFileName(context->keyDir, certType, filename, FILENAME_MAX);
    if (retval != UA_STATUSCODE_GOOD) {
    	return retval;
    }

    /* Write public key to file */
	return writeByteStringToFile(filename, privateKey);
}

static UA_StatusCode
removeContentAll(UA_PKIStore *pkiStore)
{
	/* check parameter */
	if (pkiStore == NULL || pkiStore->context == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}

	FilePKIStore *context = (FilePKIStore *)pkiStore->context;
	if (context->rootDir == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}

	/* Remove all files from PKI Store */
	return removeAllFilesFromDir(context->rootDir, true);
}

static UA_StatusCode
clear_file(UA_PKIStore *certStore) {
	/* check parameter */

	if (certStore == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}

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
        if(context->rejectedCertDir)
            UA_free(context->rejectedCertDir);
        if(context->keyDir)
            UA_free(context->keyDir);
        if (context->rootDir) {
        	UA_free(context->rootDir);
        }
        UA_free(context);
    }

    UA_NodeId_clear(&certStore->certificateGroupId);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
create_root_directory(
	UA_String* directory,
	const UA_NodeId *certificateGroupId,
	char** rootDir,
	size_t* rootDirLen
)
{
	char rootDirectory[PATH_MAX];
	*rootDir = NULL;
	*rootDirLen = 0;

	/* Set base directory */
	memset(rootDirectory, 0x00, PATH_MAX);
	if (directory != NULL) {
		if (directory->length >= PATH_MAX) {
			return UA_STATUSCODE_BADINTERNALERROR;
		}
		memcpy(rootDirectory, directory->data, directory->length);
	}
	else {
	    if(getcwd(rootDirectory, PATH_MAX) == NULL) {
	        return UA_STATUSCODE_BADINTERNALERROR;
	    }
	}
	size_t rootDirectoryLen = strnlen(rootDirectory, PATH_MAX);

	/* Add pki directory */
    strncpy(&rootDirectory[rootDirectoryLen], "/pki/", PATH_MAX - rootDirectoryLen);
    rootDirectoryLen = strnlen(rootDirectory, PATH_MAX);

    /* Add Certificate Group Id */
    UA_NodeId applCertGroup = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId httpCertGroup = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTHTTPSGROUP);
    UA_NodeId userTokenCertGroup = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);

    if (UA_NodeId_equal(certificateGroupId, &applCertGroup)) {
    	strncpy(&rootDirectory[rootDirectoryLen], "ApplCerts", PATH_MAX - rootDirectoryLen);
    }
    else if (UA_NodeId_equal(certificateGroupId, &httpCertGroup)) {
    	strncpy(&rootDirectory[rootDirectoryLen], "HttpCerts", PATH_MAX - rootDirectoryLen);
    }
    else if (UA_NodeId_equal(certificateGroupId, &userTokenCertGroup)) {
    	strncpy(&rootDirectory[rootDirectoryLen], "UserTokenCerts", PATH_MAX - rootDirectoryLen);
    }
    else {
    	UA_String nodeIdStr;
    	UA_String_init(&nodeIdStr);
    	UA_NodeId_print(certificateGroupId, &nodeIdStr);
    	strncpy(&rootDirectory[rootDirectoryLen], (char*)nodeIdStr.data, PATH_MAX - rootDirectoryLen);
    	UA_String_clear(&nodeIdStr);
    }
    rootDirectoryLen = strnlen(rootDirectory, PATH_MAX);

    *rootDir = (char*)malloc(rootDirectoryLen+1);
    if (*rootDir == NULL) {
    	return UA_STATUSCODE_BADINTERNALERROR;
    }
    memcpy(*rootDir, rootDirectory, rootDirectoryLen+1);

    *rootDirLen = strnlen(*rootDir, PATH_MAX);
    return UA_STATUSCODE_GOOD;
}


UA_StatusCode
UA_PKIStore_File_create(
	UA_PKIStore *pkiStore,
	UA_NodeId *certificateGroupId,
	UA_String* pkiDir,
	UA_StatusCode (*makeCertThumbprint)(
		const UA_ByteString* certificate,
		UA_ByteString* thumbprint
	)
) {
	/* Check parameter */
    if(pkiStore == NULL || certificateGroupId == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_PKIStore_File_clear(pkiStore);

    /* Create root directory */
    char* rootDir = NULL;
    size_t rootDirLen = 0;
    UA_StatusCode retval = create_root_directory(pkiDir, certificateGroupId, &rootDir, &rootDirLen);
    if (retval != UA_STATUSCODE_GOOD || rootDir == NULL) {
    	if (rootDir) UA_free(rootDir);
    	return retval;
    }

    /* Set PKi Store data */
    memset(pkiStore, 0, sizeof(UA_PKIStore));
    pkiStore->loadTrustList = loadTrustList_file;
    pkiStore->storeTrustList = storeTrustList_file;
    pkiStore->loadRejectedList = loadRejectedList;
    pkiStore->storeRejectedList = storeRejectedList;
    pkiStore->appendRejectedList = appendRejectedList;
    pkiStore->loadCertificate = loadCertificate_file;
    pkiStore->storeCertificate = storeCertificate_file;
    pkiStore->loadPrivateKey = loadPrivateKey_file;
    pkiStore->storePrivateKey = storePrivateKey_file;
    pkiStore->removeContentAll = removeContentAll;
    pkiStore->clear = clear_file;

    /* Set PKI Store context data */
    FilePKIStore *context = (FilePKIStore *)UA_malloc(sizeof(FilePKIStore));
    context->makeCertThumbprint = makeCertThumbprint;
    context->rootDir = rootDir;
    context->rootDirLen = rootDirLen;
    pkiStore->context = context;

    retval |= setupPkiDir("/trusted/certs", rootDir, rootDirLen, &context->trustedCertDir);
    retval |= setupPkiDir("/trusted/crls", rootDir, rootDirLen, &context->trustedCrlDir);
    retval |= setupPkiDir("/issuer/certs", rootDir, rootDirLen, &context->trustedIssuerCertDir);
    retval |= setupPkiDir("/issuer/crls", rootDir, rootDirLen, &context->trustedIssuerCrlDir);
    retval |= setupPkiDir("/rejected/certs", rootDir, rootDirLen, &context->rejectedCertDir);
    retval |= setupPkiDir("/own/certs", rootDir, rootDirLen, &context->certificateDir);
    retval |= setupPkiDir("/own/keys", rootDir, rootDirLen, &context->keyDir);
    if(retval != UA_STATUSCODE_GOOD) {
        goto error;
    }

    UA_NodeId_copy(certificateGroupId, &pkiStore->certificateGroupId);

    return UA_STATUSCODE_GOOD;

error:
	UA_PKIStore_File_clear(pkiStore);
    return UA_STATUSCODE_BADINTERNALERROR;
}

void
UA_PKIStore_File_clear(
	UA_PKIStore *pkiStore
)
{
	/* Check parameter */
	if (pkiStore == NULL) {
		return;
	}

	UA_NodeId_clear(&pkiStore->certificateGroupId);

	/* Delete context data */
	FilePKIStore* context = (FilePKIStore*)pkiStore->context;
	if (context != NULL) {
		if (context->trustedCertDir != NULL) {
			free(context->trustedCertDir);
			context->trustedCertDir = NULL;
		}
		if (context->trustedCrlDir != NULL) {
			free(context->trustedCrlDir);
			context->trustedCrlDir = NULL;
		}
		if (context->trustedIssuerCertDir != NULL) {
			free(context->trustedIssuerCertDir);
			context->trustedIssuerCertDir = NULL;
		}
		if (context->trustedIssuerCrlDir != NULL) {
			free(context->trustedIssuerCrlDir);
			context->trustedIssuerCrlDir = NULL;
		}
		if (context->rejectedCertDir != NULL) {
			free(context->rejectedCertDir);
			context->rejectedCertDir = NULL;
		}
		if (context->certificateDir != NULL) {
			free(context->certificateDir);
			context->certificateDir = NULL;
		}
		if (context->keyDir != NULL) {
			free(context->keyDir);
			context->keyDir = NULL;
		}
		if (context->rootDir != NULL) {
			free(context->rootDir);
			context->rootDir = NULL;
		}

		free(context);
		pkiStore->context = NULL;
	}
}
