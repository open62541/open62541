/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include "ua_filestore_common.h"
#include <stdio.h>

#ifdef UA_ENABLE_ENCRYPTION

#ifdef __linux__ /* Linux only so far */

UA_StatusCode
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

UA_StatusCode
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

#endif

#endif
