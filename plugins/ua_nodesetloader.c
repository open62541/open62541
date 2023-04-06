/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2014-2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include <open62541/plugin/nodesetloader.h>
#include <NodesetLoader/backendOpen62541.h>
#include <NodesetLoader/dataTypes.h>
#include <open62541/server.h>

UA_StatusCode
UA_Server_loadNodeset(UA_Server *server, const char *nodeset2XmlFilePath,
                      UA_NodeSetLoaderOptions *options) {
    if(!NodesetLoader_loadFile(server,
                               nodeset2XmlFilePath,
                               (NodesetLoader_ExtensionInterface*)options)) {
        return UA_STATUSCODE_BAD;
    }

    return UA_STATUSCODE_GOOD;
}
