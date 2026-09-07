/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_NODESET_LOADER_TEST_H_
#define UA_NODESET_LOADER_TEST_H_

#include <open62541/server.h>

#include <stdio.h>

static UA_StatusCode
loadNodesetFile(UA_Server *server, const char *path, const void *unused) {
    (void)unused;
    FILE *file = fopen(path, "rb");
    if(!file)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_StatusCode res = UA_STATUSCODE_BADINTERNALERROR;
    if(fseek(file, 0, SEEK_END) != 0)
        goto cleanup;
    long length = ftell(file);
    if(length < 0 || fseek(file, 0, SEEK_SET) != 0)
        goto cleanup;
    size_t size = (size_t)length;
    if((long)size != length) {
        res = UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
        goto cleanup;
    }

    UA_XmlElement xml = UA_STRING_NULL;
    res = UA_ByteString_allocBuffer((UA_ByteString *)&xml, size);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    if(fread(xml.data, 1, size, file) != size) {
        UA_String_clear(&xml);
        res = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    res = UA_Server_loadNodeset(server, xml);
    UA_String_clear(&xml);

cleanup:
    fclose(file);
    return res;
}

#endif /* UA_NODESET_LOADER_TEST_H_ */
