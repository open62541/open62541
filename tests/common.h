/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/types.h>
#include <stdio.h>
#include <errno.h>

/* loadFile parses the certificate file.
 *
 * @param  path               specifies the file name given in argv[]
 * @return Returns the file content after parsing */
static UA_INLINE UA_ByteString
loadFile(const char *const path) {
    UA_ByteString fileContents = UA_STRING_NULL;

    /* Open the file */
    FILE *fp = fopen(path, "rb");
    if(!fp) {
        errno = 0; /* We read errno also from the tcp layer... */
        return fileContents;
    }

    /* Get the file length, allocate the data and read */
    if(fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        errno = 0;
        return fileContents;
    }

    long length = ftell(fp);
    if(length < 0) {
        fclose(fp);
        errno = 0;
        return fileContents;
    }

    fileContents.length = (size_t)length;
    fileContents.data = (UA_Byte *)UA_malloc(fileContents.length * sizeof(UA_Byte));
    if(fileContents.data) {
        if(fseek(fp, 0, SEEK_SET) != 0) {
            fclose(fp);
            UA_ByteString_clear(&fileContents);
            errno = 0;
            return fileContents;
        }
        size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
        if(read != fileContents.length)
            UA_ByteString_clear(&fileContents);
    } else {
        fileContents.length = 0;
    }
    fclose(fp);

    return fileContents;
}
