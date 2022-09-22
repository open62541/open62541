/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2014-2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include <open62541/plugin/nodesetloader_default.h>
#include <NodesetLoader/backendOpen62541.h>
#include <open62541/server.h>

void
UA_NodesetLoader_Init(UA_Server *server) {}

UA_StatusCode
UA_NodesetLoader_LoadNodeset(UA_Server *server, const char *nodeset2XmlFilePath) {
    return NodesetLoader_loadFile(server, nodeset2XmlFilePath, NULL);
}

static void
cleanupCustomTypes(const UA_DataTypeArray *types) {
    while (types) {
        const UA_DataTypeArray *next = types->next;
        if (types->types) {
            for (const UA_DataType *type = types->types;
                 type != types->types + types->typesSize; type++)
            {
                free((void*)(uintptr_t)type->typeName);
                UA_UInt32 mSize = type->membersSize;
                
                UA_DataTypeMember *m = type->members;
                for (; m != type->members + mSize; m++) {
                    free((void*)(uintptr_t)m->memberName);
                    m->memberName = NULL;
                }
                free((void*)(uintptr_t)type->members);
            }
        }
        free((void*)(uintptr_t)types->types);
        free((void*)(uintptr_t)types);
        types = next;
    }
}

void UA_NodesetLoader_Delete(UA_Server *server) {
    cleanupCustomTypes(UA_Server_getConfig(server)->customDataTypes);
}