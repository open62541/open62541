/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include "ua_filestore_common.h"

#if defined(UA_ENABLE_ENCRYPTION) && defined(UA_ENABLE_CERTIFICATE_FILESTORE)

#ifdef UA_ARCHITECTURE_WIN32
/* TODO: Replace with a proper dirname implementation. This is a just minimal
 * implementation working with correct input data. */
char *
_UA_dirname_minimal(char *path) {
    char *lastSlash = strrchr(path, '/');
    *lastSlash = 0;
    return path;
}
#endif /* UA_ARCHITECTURE_WIN32 */

UA_StatusCode
readFileToByteString(const char *const path, UA_ByteString *data) {
    if(path == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Open the file */
    UA_FILE *fp = UA_fopen(path, "rb");
    if(!fp)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Get the file length, allocate the data and read */
    UA_fseek(fp, 0, UA_SEEK_END);
    UA_StatusCode retval = UA_ByteString_allocBuffer(data, (size_t)UA_ftell(fp));
    if(retval == UA_STATUSCODE_GOOD) {
        UA_fseek(fp, 0, UA_SEEK_SET);
        size_t read = UA_fread(data->data, sizeof(UA_Byte), data->length * sizeof(UA_Byte), fp);
        if(read != data->length) {
            UA_ByteString_clear(data);
        }
    } else {
        data->length = 0;
    }
    UA_fclose(fp);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
writeByteStringToFile(const char *const path, const UA_ByteString *data) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Open the file */
    UA_FILE *fp = UA_fopen(path, "wb");
    if(!fp)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Write byte string to file */
    size_t len = UA_fwrite(data->data, sizeof(UA_Byte), data->length * sizeof(UA_Byte), fp);
    if(len != data->length) {
        UA_fclose(fp);
        retval = UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_fclose(fp);
    return retval;
}

#endif /* UA_ENABLE_ENCRYPTION && UA_ENABLE_CERTIFICATE_FILESTORE */
